#!/usr/bin/env python3
"""Differential validation of cpu6502.c against py65 (pip install py65) on random straight-line programs.
Usage: validate_cpu6502.py path/to/libcpu.so [iterations]"""
import ctypes, random, sys
from py65.devices.mpu6502 import MPU

lib = ctypes.CDLL(sys.argv[1])
N = int(sys.argv[2]) if len(sys.argv) > 2 else 3000

class Cpu(ctypes.Structure):
    _fields_ = [('a', ctypes.c_uint8), ('x', ctypes.c_uint8), ('y', ctypes.c_uint8), ('sp', ctypes.c_uint8),
                ('p', ctypes.c_uint8), ('pc', ctypes.c_uint16), ('ram', ctypes.c_uint8 * 0x800),
                ('prg', ctypes.c_uint8 * 0x4000), ('io', ctypes.c_uint8 * 0x10000),
                ('rh', ctypes.c_void_p), ('wh', ctypes.c_void_p), ('user', ctypes.c_void_p),
                ('cycles', ctypes.c_ulong), ('bad', ctypes.c_int)]
lib.cpu6502_reset_state.argtypes = [ctypes.POINTER(Cpu)]
lib.cpu6502_step.argtypes = [ctypes.POINTER(Cpu)]

# opcode -> (mode, bytes) for non-control official opcodes
modes = {'imm': 2, 'zp': 2, 'zpx': 2, 'zpy': 2, 'abs': 3, 'absx': 3, 'absy': 3, 'indx': 2, 'indy': 2, 'imp': 1}
ops = []
g1 = ['indx', 'zp', 'imm', 'abs', 'indy', 'zpx', 'absy', 'absx']
for aaa in range(8):
    for bbb, m in enumerate(g1):
        op = (aaa << 5) | (bbb << 2) | 1
        if aaa == 4 and m == 'imm':
            continue
        ops.append((op, m))
g2 = {0: ['imm', 'zp', 'acc', 'abs', None, 'zpx', None, 'absx']}
for aaa in range(8):
    for bbb, m in enumerate(['imm', 'zp', 'acc', 'abs', None, 'zpx', None, 'absx']):
        op = (aaa << 5) | (bbb << 2) | 2
        if m is None: continue
        if aaa in (6, 7) and m in ('imm', 'acc'): continue       # DEC/INC have no imm/acc forms
        if aaa == 4 and m in ('imm', 'acc', 'absx'): continue    # STX forms handled below
        if aaa == 5: continue                                    # LDX handled below
        if m == 'imm': continue
        ops.append((op, 'imp' if m == 'acc' else m))
ops += [(0x86, 'zp'), (0x96, 'zpy'), (0x8E, 'abs'), (0xA2, 'imm'), (0xA6, 'zp'), (0xB6, 'zpy'), (0xAE, 'abs'), (0xBE, 'absy')]
ops += [(0xA0, 'imm'), (0xA4, 'zp'), (0xB4, 'zpx'), (0xAC, 'abs'), (0xBC, 'absx'), (0x84, 'zp'), (0x94, 'zpx'), (0x8C, 'abs')]
ops += [(0xC0, 'imm'), (0xC4, 'zp'), (0xCC, 'abs'), (0xE0, 'imm'), (0xE4, 'zp'), (0xEC, 'abs'), (0x24, 'zp'), (0x2C, 'abs')]
for op in (0x08, 0x48, 0x68, 0x88, 0xA8, 0xC8, 0xE8, 0x18, 0x38, 0x58, 0x78, 0x98, 0xB8, 0xEA, 0x8A, 0xAA, 0xCA, 0xBA):
    ops.append((op, 'imp'))
ops = [(o, m) for o, m in ops if o != 0x9A]
ops_ind = [(o, m) for o, m in ops if m in ('indx', 'indy')]
ops = [(o, m) for o, m in ops if m not in ('indx', 'indy')]   # sequences avoid indirect modes (random stores could corrupt the pointers); they are covered per-opcode below
branches = [0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0]

rnd = random.Random(12345)
def gen():
    code = []
    for _ in range(rnd.randint(10, 50)):
        if rnd.random() < 0.12:
            code += [rnd.choice(branches), 2, 0xA9, rnd.randint(0, 255)]
            continue
        op, m = rnd.choice(ops)
        code.append(op)
        if m in ('imm', 'zpx', 'zpy', 'indx', 'indy'): code.append(rnd.randint(0, 255))
        elif m == 'zp': code.append(rnd.randint(0, 255))
        elif m == 'abs': a = rnd.randint(0x100, 0x5FF) if rnd.random() < .5 else rnd.randint(0, 0x5FF); code += [a & 255, a >> 8]
        elif m in ('absx', 'absy'): a = rnd.randint(0x100, 0x4F0); code += [a & 255, a >> 8]
    return code

bad = 0
for it in range(N):
    code = gen()
    zp = [rnd.randint(0, 5) for _ in range(256)]                      # every zero-page pointer lands in $0000-$05FF
    for i in range(0, 256, 3): zp[i] = rnd.randint(0, 255)
    mem = [rnd.randint(0, 255) for _ in range(0x800)]
    mem[0:256] = zp
    for i in range(len(code)): mem[0x600 + i] = code[i]
    regs = (rnd.randint(0, 255), rnd.randint(0, 255), rnd.randint(0, 255), rnd.randint(0, 255) & ~0x08 & ~0x10)
    # reference
    m = MPU()
    for i in range(0x800): m.memory[i] = mem[i]
    m.a, m.x, m.y, m.sp = regs[0], regs[1], regs[2], 0xFD
    m.p = (regs[3] | 0x20) & ~0x08 & ~0x10 & 0xFF
    m.pc = 0x600
    end = 0x600 + len(code)
    steps = 0
    while m.pc < end and steps < 200:
        m.step(); steps += 1
    # mine
    c = Cpu()
    lib.cpu6502_reset_state(ctypes.byref(c))
    for i in range(0x800): c.ram[i] = mem[i]
    c.a, c.x, c.y, c.sp = regs[0], regs[1], regs[2], 0xFD
    c.p = (regs[3] | 0x20) & ~0x08 & ~0x10 & 0xFF
    c.pc = 0x600
    s2 = 0
    while c.pc < end and s2 < 200:
        lib.cpu6502_step(ctypes.byref(c)); s2 += 1
        if c.bad >= 0: break
    rp = m.p & ~0x10
    cp = c.p & ~0x10
    ok = (c.bad < 0 and (c.a, c.x, c.y, c.sp, cp, c.pc) == (m.a, m.x, m.y, m.sp, rp, m.pc)
          and all(c.ram[i] == m.memory[i] for i in range(0x800)))
    if not ok:
        bad += 1
        if bad <= 3:
            print('MISMATCH iter', it, 'bad', c.bad, 'code', bytes(code).hex())
            print(' ref', m.a, m.x, m.y, m.sp, hex(rp), hex(m.pc), ' mine', c.a, c.x, c.y, c.sp, hex(cp), hex(c.pc))

# per-opcode single-instruction tests for the indirect modes (valid pointers everywhere in zero page)
for op, m in ops_ind:
    for _ in range(300):
        mem = [rnd.randint(0, 255) for _ in range(0x800)]
        for i in range(256): mem[i] = rnd.randint(0, 5)
        mem[0x600] = op; mem[0x601] = rnd.randint(0, 255)
        r = (rnd.randint(0, 255), rnd.randint(0, 255), rnd.randint(0, 255), rnd.randint(0, 255) & ~0x18)
        mm = MPU()
        for i in range(0x800): mm.memory[i] = mem[i]
        mm.a, mm.x, mm.y, mm.sp = r[0], r[1], r[2], 0xFD; mm.p = (r[3] | 0x20) & 0xE7; mm.pc = 0x600
        mm.step()
        c = Cpu(); lib.cpu6502_reset_state(ctypes.byref(c))
        for i in range(0x800): c.ram[i] = mem[i]
        c.a, c.x, c.y, c.sp = r[0], r[1], r[2], 0xFD; c.p = (r[3] | 0x20) & 0xE7; c.pc = 0x600
        lib.cpu6502_step(ctypes.byref(c))
        if not (c.bad < 0 and (c.a, c.x, c.y, c.sp, c.p & ~0x10, c.pc) == (mm.a, mm.x, mm.y, mm.sp, mm.p & ~0x10, mm.pc)
                and all(c.ram[i] == mm.memory[i] for i in range(0x800))):
            bad += 1
            print('MISMATCH indirect op', hex(op))
print('mismatches:', bad, 'of', N)
sys.exit(1 if bad else 0)
