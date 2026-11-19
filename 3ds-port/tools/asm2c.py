#!/usr/bin/env python3
"""Static recompiler: Excitebike/bank_FF.asm -> C (source/nes_gen_code.c).

Every instruction of the disassembly is translated mechanically into C from its OPCODE BYTES (operands are taken
from the bytes, not from symbol names), keeping the original line as a comment.  The result is one function
`int nesRunCode(NesMachine *m, uint16_t pc, long *budget)` with a `case` label on EVERY instruction address, a real
6502 stack in RAM page 1 (so JSR/RTS tricks such as 'PLA PLA' work exactly), computed jumps through `switch (pc)`.
Hardware accesses ($2000-$401F) go through NesMachine io hooks.  Usage: asm2c.py bank_FF.asm out.c
"""
import re, sys

asm, out = sys.argv[1], sys.argv[2]
line_re = re.compile(r'^[-A-Z] [-A-Z0-9 ]{9,12}0x[0-9A-F]{6} 00:([0-9A-F]{4}): ((?:[0-9A-F]{2} ?)+)\s+([A-Z]{3})\b(.*)$')
G1 = ['ORA', 'AND', 'EOR', 'ADC', 'STA', 'LDA', 'CMP', 'SBC']
M1 = ['indx', 'zp', 'imm', 'abs', 'indy', 'zpx', 'absy', 'absx']
G2 = ['ASL', 'ROL', 'LSR', 'ROR', 'STX', 'LDX', 'DEC', 'INC']
IMPL = {0x08: 'PHP', 0x28: 'PLP', 0x48: 'PHA', 0x68: 'PLA', 0x88: 'DEY', 0xA8: 'TAY', 0xC8: 'INY', 0xE8: 'INX',
        0x18: 'CLC', 0x38: 'SEC', 0x58: 'CLI', 0x78: 'SEI', 0x98: 'TYA', 0xB8: 'CLV', 0xD8: 'CLD', 0xF8: 'SED',
        0x8A: 'TXA', 0x9A: 'TXS', 0xAA: 'TAX', 0xBA: 'TSX', 0xCA: 'DEX', 0xEA: 'NOP', 0x60: 'RTS', 0x40: 'RTI', 0x00: 'BRK'}
BR = {0x10: 'BPL', 0x30: 'BMI', 0x50: 'BVC', 0x70: 'BVS', 0x90: 'BCC', 0xB0: 'BCS', 0xD0: 'BNE', 0xF0: 'BEQ'}
LEN = {'imm': 2, 'zp': 2, 'zpx': 2, 'zpy': 2, 'indx': 2, 'indy': 2, 'abs': 3, 'absx': 3, 'absy': 3, 'ind': 3,
       'acc': 1, 'impl': 1, 'rel': 2}

def decode(op):
    if op in IMPL: return IMPL[op], 'impl'
    if op in BR: return BR[op], 'rel'
    if op == 0x20: return 'JSR', 'abs'
    if op == 0x4C: return 'JMP', 'abs'
    if op == 0x6C: return 'JMP', 'ind'
    aaa, bbb, cc = op >> 5, (op >> 2) & 7, op & 3
    if cc == 1:
        mode = M1[bbb]
        if aaa == 4 and mode == 'imm': return None, None
        return G1[aaa], mode
    if cc == 2:
        mn = G2[aaa]
        mode = ['imm', 'zp', 'acc', 'abs', None, 'zpx', None, 'absx'][bbb]
        if mode is None: return None, None
        if mn in ('STX', 'LDX'):
            if bbb == 5: mode = 'zpy'
            if bbb == 7: mode = 'absy'
            if mode == 'acc': return None, None
            if mn == 'STX' and mode in ('imm', 'absy'): return None, None
        elif mode == 'imm': return None, None
        elif mn in ('DEC', 'INC') and mode == 'acc': return None, None
        return mn, mode
    if cc == 0 and aaa in (1, 4, 5, 6, 7):
        mn = {1: 'BIT', 4: 'STY', 5: 'LDY', 6: 'CPY', 7: 'CPX'}[aaa]
        mode = ['imm', 'zp', None, 'abs', None, 'zpx', None, 'absx'][bbb]
        if mode is None: return None, None
        if mn == 'BIT' and mode not in ('zp', 'abs'): return None, None
        if mn == 'STY' and mode in ('imm', 'absx'): return None, None
        if mn in ('CPY', 'CPX') and mode not in ('imm', 'zp', 'abs'): return None, None
        return mn, mode
    return None, None

def cycles(mn, mode):
    if mode in ('impl', 'acc'):
        return {'PHA': 3, 'PHP': 3, 'PLA': 4, 'PLP': 4, 'RTS': 6, 'RTI': 6, 'BRK': 7}.get(mn, 2)
    if mode == 'rel': return 2
    if mn == 'JSR': return 6
    if mn == 'JMP': return 3 if mode == 'abs' else 5
    base = {'imm': 2, 'zp': 3, 'zpx': 4, 'zpy': 4, 'abs': 4, 'absx': 4, 'absy': 4, 'indx': 6, 'indy': 5}[mode]
    if mn in ('ASL', 'LSR', 'ROL', 'ROR', 'INC', 'DEC'):
        return {'zp': 5, 'zpx': 6, 'abs': 6, 'absx': 7}[mode]
    if mn in ('STA', 'STX', 'STY') and mode in ('absx', 'absy'): return 5
    if mn == 'STA' and mode == 'indy': return 6
    return base

ins = {}          # addr -> (mn, mode, bytes, text)
for ln in open(asm, encoding='utf-8', errors='replace'):
    ln = ln.rstrip('\n')
    m = line_re.match(ln)
    if not m: continue
    addr = int(m.group(1), 16)
    data = bytes.fromhex(m.group(2).replace(' ', ''))
    op = data[0]
    mn, mode = decode(op)
    if mn is None:
        sys.exit('unsupported opcode %02X at %04X: %s' % (op, addr, ln))
    if len(data) != LEN[mode] or mn != m.group(3):
        sys.exit('decode mismatch at %04X: %s (got %s %s len %d)' % (addr, ln, mn, mode, len(data)))
    ins[addr] = (mn, mode, data, ln.split(':', 2)[-1].strip() if False else ln[ln.index('00:'):].strip())

addrs = sorted(ins)
targets = set()
for a in addrs:
    mn, mode, data, _ = ins[a]
    if mode == 'rel': targets.add((a + 2 + (data[1] - 256 if data[1] >= 128 else data[1])) & 0xFFFF)
    if mn in ('JMP', 'JSR') and mode == 'abs': targets.add(data[1] | data[2] << 8)

def ea(mode, data):
    b1 = data[1] if len(data) > 1 else 0
    w = (data[1] | data[2] << 8) if len(data) > 2 else 0
    if mode == 'zp': return '0x%04X' % b1
    if mode == 'zpx': return '(uint16_t)(uint8_t)(0x%02X + m->x)' % b1
    if mode == 'zpy': return '(uint16_t)(uint8_t)(0x%02X + m->y)' % b1
    if mode == 'abs': return '0x%04X' % w
    if mode == 'absx': return '(uint16_t)(0x%04X + m->x)' % w
    if mode == 'absy': return '(uint16_t)(0x%04X + m->y)' % w
    if mode == 'indx': return 'nesPtrX(m, 0x%02X)' % b1
    if mode == 'indy': return 'nesPtrY(m, 0x%02X)' % b1
    raise Exception(mode)

L = []
w = L.append
w('/* GENERATED by tools/asm2c.py from Excitebike/bank_FF.asm - do not edit. Mechanical translation of every instruction. */')
w('#include "nes_machine.h"')
w('#include "nes_gen_code.h"')
w('')
w('#pragma GCC diagnostic ignored "-Wunused-label"')
w('#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"')
w('')
w('int nesRunCode(NesMachine *m, uint16_t pc, long *budget)')
w('{')
w('    uint8_t t8; uint16_t t16; (void)t8; (void)t16;')
w('dispatch:')
w('    switch (pc) {')
prev_end = None
for a in addrs:
    mn, mode, data, text = ins[a]
    if prev_end is not None and prev_end != a:
        w('        m->pc = 0x%04X; return 2; /* gap (data) before %04X */' % (prev_end, a))
    prev_end = a + len(data)
    w('    case 0x%04X: L_%04X: /* %s */' % (a, a, text.replace('*/', '* /')))
    m_ = 'm->cyc += %d;' % cycles(mn, mode)
    def RD(): return 'nesRd(m, %s)' % ea(mode, data)
    s = None
    if mn == 'LDA': s = 'm->a = nesLoad(m, %s);' % ('0x%02X' % data[1] if mode == 'imm' else RD())
    elif mn == 'LDX': s = 'm->x = nesLoad(m, %s);' % ('0x%02X' % data[1] if mode == 'imm' else RD())
    elif mn == 'LDY': s = 'm->y = nesLoad(m, %s);' % ('0x%02X' % data[1] if mode == 'imm' else RD())
    elif mn == 'STA': s = 'nesWr(m, %s, m->a);' % ea(mode, data)
    elif mn == 'STX': s = 'nesWr(m, %s, m->x);' % ea(mode, data)
    elif mn == 'STY': s = 'nesWr(m, %s, m->y);' % ea(mode, data)
    elif mn in ('ADC', 'SBC'):
        v = '0x%02X' % data[1] if mode == 'imm' else RD()
        s = 'nes%s(m, %s);' % ('Adc' if mn == 'ADC' else 'Sbc', v)
    elif mn in ('AND', 'ORA', 'EOR'):
        v = '0x%02X' % data[1] if mode == 'imm' else RD()
        op = {'AND': '&=', 'ORA': '|=', 'EOR': '^='}[mn]
        s = 'm->a %s %s; nesSetNZ(m, m->a);' % (op, v)
    elif mn in ('CMP', 'CPX', 'CPY'):
        v = '0x%02X' % data[1] if mode == 'imm' else RD()
        r = {'CMP': 'm->a', 'CPX': 'm->x', 'CPY': 'm->y'}[mn]
        s = 'nesCmp(m, %s, %s);' % (r, v)
    elif mn == 'BIT': s = 't8 = %s; m->z = (uint8_t)((m->a & t8) == 0u); m->n = (uint8_t)(t8 >> 7); m->v = (uint8_t)((t8 >> 6) & 1u);' % RD()
    elif mn in ('ASL', 'LSR', 'ROL', 'ROR'):
        fn = 'nes' + mn.capitalize()
        if mode == 'acc': s = 'm->a = %s(m, m->a);' % fn
        else: s = 't16 = %s; nesWr(m, t16, %s(m, nesRd(m, t16)));' % (ea(mode, data), fn)
    elif mn in ('INC', 'DEC'):
        d = '+' if mn == 'INC' else '-'
        s = 't16 = %s; t8 = (uint8_t)(nesRd(m, t16) %s 1u); nesWr(m, t16, t8); nesSetNZ(m, t8);' % (ea(mode, data), d)
    elif mn == 'INX': s = 'm->x = nesLoad(m, (uint8_t)(m->x + 1u));'
    elif mn == 'INY': s = 'm->y = nesLoad(m, (uint8_t)(m->y + 1u));'
    elif mn == 'DEX': s = 'm->x = nesLoad(m, (uint8_t)(m->x - 1u));'
    elif mn == 'DEY': s = 'm->y = nesLoad(m, (uint8_t)(m->y - 1u));'
    elif mn == 'TAX': s = 'm->x = nesLoad(m, m->a);'
    elif mn == 'TAY': s = 'm->y = nesLoad(m, m->a);'
    elif mn == 'TXA': s = 'm->a = nesLoad(m, m->x);'
    elif mn == 'TYA': s = 'm->a = nesLoad(m, m->y);'
    elif mn == 'TSX': s = 'm->x = nesLoad(m, m->sp);'
    elif mn == 'TXS': s = 'm->sp = m->x;'
    elif mn == 'CLC': s = 'm->c = 0u;'
    elif mn == 'SEC': s = 'm->c = 1u;'
    elif mn == 'CLV': s = 'm->v = 0u;'
    elif mn == 'CLI': s = 'm->pother &= (uint8_t)~0x04u;'
    elif mn == 'SEI': s = 'm->pother |= 0x04u;'
    elif mn == 'CLD': s = 'm->pother &= (uint8_t)~0x08u;'
    elif mn == 'SED': s = 'm->pother |= 0x08u;'
    elif mn == 'NOP': s = ';'
    elif mn == 'PHA': s = 'nesPush(m, m->a);'
    elif mn == 'PLA': s = 'm->a = nesLoad(m, nesPull(m));'
    elif mn == 'PHP': s = 'nesPush(m, nesGetP(m, 1u));'
    elif mn == 'PLP': s = 'nesSetP(m, nesPull(m));'
    elif mn == 'JSR':
        t = data[1] | data[2] << 8
        ret = (a + 2) & 0xFFFF
        s = 'nesPush(m, 0x%02X); nesPush(m, 0x%02X); goto L_%04X;' % (ret >> 8, ret & 0xFF, t)
        if t not in ins: s = 'm->pc = 0x%04X; return 2; /* JSR into data */' % t
    elif mn == 'RTS': s = 't16 = nesPull(m); t16 = (uint16_t)(t16 | (nesPull(m) << 8)); pc = (uint16_t)(t16 + 1u); goto dispatch;'
    elif mn == 'RTI': s = 'nesSetP(m, nesPull(m)); t16 = nesPull(m); t16 = (uint16_t)(t16 | (nesPull(m) << 8)); pc = t16; goto dispatch;'
    elif mn == 'BRK': s = 'm->pc = 0x%04X; return 2;' % a
    elif mn == 'JMP':
        if mode == 'abs':
            t = data[1] | data[2] << 8
            if t not in ins: s = 'm->pc = 0x%04X; return 2; /* JMP into data */' % t
            else:
                back = t <= a
                s = ('if (--*budget <= 0 || (m->cyc_limit != 0ul && m->cyc >= m->cyc_limit)) { m->pc = 0x%04X; return 1; } ' % t if back else '') + 'goto L_%04X;' % t
        else:
            p = data[1] | data[2] << 8
            s = 'pc = nesReadVec(m, 0x%04X); goto dispatch;' % p
    elif mode == 'rel':
        t = (a + 2 + (data[1] - 256 if data[1] >= 128 else data[1])) & 0xFFFF
        cond = {'BPL': '!m->n', 'BMI': 'm->n', 'BVC': '!m->v', 'BVS': 'm->v', 'BCC': '!m->c', 'BCS': 'm->c', 'BNE': '!m->z', 'BEQ': 'm->z'}[mn]
        bud = 'if (--*budget <= 0 || (m->cyc_limit != 0ul && m->cyc >= m->cyc_limit)) { m->pc = 0x%04X; return 1; } ' % t if t <= a else ''
        s = 'if (%s) { m->cyc += 1; %s goto L_%04X; }' % (cond, bud, t)
        if t not in ins: s = 'if (%s) { m->pc = 0x%04X; return 2; }' % (cond, t)
    if s is None: sys.exit('no translation for %s %s at %04X' % (mn, mode, a))
    w('        %s %s' % (m_, s))
w('    default:')
w('        if (pc >= 0x8000u && pc < 0xC000u) { pc = (uint16_t)(pc | 0xC000u); goto dispatch; }   /* NROM-128 PRG mirror */')
w('        m->pc = pc; return 2;')
w('    case 0xFFFF: m->pc = 0xFFFFu; return 0;   /* sentinel return address used by nesCall */')
w('    }')
w('}')
w('')
w('/* 1 when `addr` is the first byte of an instruction of the original program (valid computed-jump target). */')
w('int nesIsInstructionStart(uint16_t addr)')
w('{')
w('    static const uint8_t bits[] = {')
bm = bytearray(0x4000 // 8)
for a in addrs:
    if a >= 0xC000: bm[(a - 0xC000) >> 3] |= 1 << ((a - 0xC000) & 7)
for i in range(0, len(bm), 24): w('        ' + ', '.join('0x%02X' % b for b in bm[i:i+24]) + ',')
w('    };')
w('    if (addr >= 0x8000u && addr < 0xC000u) addr = (uint16_t)(addr | 0xC000u);')
w('    if (addr < 0xC000u) return 0;')
w('    return (bits[(addr - 0xC000u) >> 3] >> ((addr - 0xC000u) & 7u)) & 1;')
w('}')
open(out, 'w').write('\n'.join(L) + '\n')
print('instructions:', len(ins), 'branch/jump targets:', len(targets))
