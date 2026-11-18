#include "cpu6502.h"
#include <string.h>

#define FC 0x01
#define FZ 0x02
#define FI 0x04
#define FD 0x08
#define FB 0x10
#define FU 0x20
#define FV 0x40
#define FN 0x80

uint8_t cpu6502_read(Cpu6502 *c, uint16_t a)
{
    if (a < 0x2000u) return c->ram[a & 0x7FFu];
    if (a >= 0x8000u) return c->prg[a & 0x3FFFu];
    if (c->read_hook) return c->read_hook(c, a);
    return c->io[a];
}
void cpu6502_write(Cpu6502 *c, uint16_t a, uint8_t v)
{
    if (a < 0x2000u) { c->ram[a & 0x7FFu] = v; return; }
    if (a >= 0x8000u) return;
    if (c->write_hook) { c->write_hook(c, a, v); return; }
    c->io[a] = v;
}
void cpu6502_reset_state(Cpu6502 *c)
{
    memset(c->ram, 0, sizeof(c->ram));
    memset(c->io, 0, sizeof(c->io));
    c->a = c->x = c->y = 0; c->sp = 0xFD; c->p = FU | FI; c->pc = 0; c->cycles = 0; c->bad_opcode = -1;
}

static void setzn(Cpu6502 *c, uint8_t v) { c->p = (uint8_t)((c->p & ~(FZ | FN)) | (v == 0 ? FZ : 0) | (v & FN)); }
static uint8_t fetch(Cpu6502 *c) { return cpu6502_read(c, c->pc++); }
static uint16_t fetch16(Cpu6502 *c) { uint16_t l = fetch(c); return (uint16_t)(l | (fetch(c) << 8)); }
static void push(Cpu6502 *c, uint8_t v) { c->ram[0x100u + c->sp] = v; c->sp--; }
static uint8_t pull(Cpu6502 *c) { c->sp++; return c->ram[0x100u + c->sp]; }
static uint16_t rd16zp(Cpu6502 *c, uint8_t z) { return (uint16_t)(cpu6502_read(c, z) | (cpu6502_read(c, (uint8_t)(z + 1)) << 8)); }

static void adc(Cpu6502 *c, uint8_t m)
{
    unsigned s = c->a + m + (c->p & FC);
    uint8_t r = (uint8_t)s;
    c->p = (uint8_t)((c->p & ~(FC | FV)) | (s > 0xFFu ? FC : 0) | ((~(c->a ^ m) & (c->a ^ r) & 0x80u) ? FV : 0));
    c->a = r; setzn(c, r);
}
static void cmp(Cpu6502 *c, uint8_t r, uint8_t m)
{
    c->p = (uint8_t)((c->p & ~FC) | (r >= m ? FC : 0));
    setzn(c, (uint8_t)(r - m));
}
static void branch(Cpu6502 *c, int cond)
{
    int8_t o = (int8_t)fetch(c);
    if (cond) c->pc = (uint16_t)(c->pc + o);
}

/* Operand address computation. mode: 0 imm,1 zp,2 zpx,3 zpy,4 abs,5 absx,6 absy,7 indx,8 indy */
static uint16_t ea(Cpu6502 *c, int mode)
{
    switch (mode) {
    case 0: return c->pc++;
    case 1: return fetch(c);
    case 2: return (uint8_t)(fetch(c) + c->x);
    case 3: return (uint8_t)(fetch(c) + c->y);
    case 4: return fetch16(c);
    case 5: return (uint16_t)(fetch16(c) + c->x);
    case 6: return (uint16_t)(fetch16(c) + c->y);
    case 7: return rd16zp(c, (uint8_t)(fetch(c) + c->x));
    default: return (uint16_t)(rd16zp(c, fetch(c)) + c->y);
    }
}

void cpu6502_step(Cpu6502 *c)
{
    uint8_t op = fetch(c), m, t;
    uint16_t a;
    c->cycles += 2;
    /* group-structured decode (aaabbbcc) for the regular opcodes */
    {
        int aaa = op >> 5, bbb = (op >> 2) & 7, cc = op & 3;
        if (cc == 1) {
            static const int md[8] = {7, 1, 0, 4, 8, 2, 6, 5};
            int mode = md[bbb];
            if (aaa == 4) {            /* STA */
                if (mode == 0) { c->bad_opcode = op; return; }
                cpu6502_write(c, ea(c, mode), c->a); return;
            }
            a = ea(c, mode); m = cpu6502_read(c, a);
            switch (aaa) {
            case 0: c->a |= m; setzn(c, c->a); break;
            case 1: c->a &= m; setzn(c, c->a); break;
            case 2: c->a ^= m; setzn(c, c->a); break;
            case 3: adc(c, m); break;
            case 5: c->a = m; setzn(c, c->a); break;
            case 6: cmp(c, c->a, m); break;
            case 7: adc(c, (uint8_t)~m); break;
            }
            return;
        }
        if (cc == 2 && op != 0x8A && op != 0xAA && op != 0xCA && op != 0xEA && op != 0x9A && op != 0xBA) {
            static const int md[8] = {0, 1, -1, 4, -2, 2, -3, 5};
            int mode = md[bbb];
            if (bbb == 2) {            /* accumulator shifts */
                uint8_t v = c->a;
                switch (aaa) {
                case 0: c->p = (uint8_t)((c->p & ~FC) | (v >> 7)); v <<= 1; break;
                case 1: t = (uint8_t)(c->p & FC); c->p = (uint8_t)((c->p & ~FC) | (v >> 7)); v = (uint8_t)((v << 1) | t); break;
                case 2: c->p = (uint8_t)((c->p & ~FC) | (v & 1)); v >>= 1; break;
                case 3: t = (uint8_t)(c->p & FC); c->p = (uint8_t)((c->p & ~FC) | (v & 1)); v = (uint8_t)((v >> 1) | (t << 7)); break;
                default: c->bad_opcode = op; return;
                }
                c->a = v; setzn(c, v); return;
            }
            if (mode == -2) mode = 0;
            if (aaa == 4) {            /* STX */
                if (bbb == 5) mode = 3;
                if (bbb == 7) { c->bad_opcode = op; return; }
                cpu6502_write(c, ea(c, bbb == 0 ? 0 : mode), c->x); return;
            }
            if (aaa == 5) {            /* LDX */
                if (bbb == 5) mode = 3;
                if (bbb == 7) mode = 6;
                a = ea(c, mode); c->x = cpu6502_read(c, a); setzn(c, c->x); return;
            }
            if (bbb == 0 || (aaa < 4 && 0)) { c->bad_opcode = op; return; }
            if (aaa == 6 || aaa == 7) {  /* DEC / INC */
                a = ea(c, mode); m = cpu6502_read(c, a);
                m = (uint8_t)(aaa == 6 ? m - 1 : m + 1); cpu6502_write(c, a, m); setzn(c, m); return;
            }
            a = ea(c, mode); m = cpu6502_read(c, a);
            switch (aaa) {
            case 0: c->p = (uint8_t)((c->p & ~FC) | (m >> 7)); m <<= 1; break;
            case 1: t = (uint8_t)(c->p & FC); c->p = (uint8_t)((c->p & ~FC) | (m >> 7)); m = (uint8_t)((m << 1) | t); break;
            case 2: c->p = (uint8_t)((c->p & ~FC) | (m & 1)); m >>= 1; break;
            case 3: t = (uint8_t)(c->p & FC); c->p = (uint8_t)((c->p & ~FC) | (m & 1)); m = (uint8_t)((m >> 1) | (t << 7)); break;
            default: c->bad_opcode = op; return;
            }
            cpu6502_write(c, a, m); setzn(c, m); return;
        }
        if (cc == 0) {
            if (bbb == 4) {            /* branches */
                switch (aaa) {
                case 0: branch(c, !(c->p & FN)); return;
                case 1: branch(c, c->p & FN); return;
                case 2: branch(c, !(c->p & FV)); return;
                case 3: branch(c, c->p & FV); return;
                case 4: branch(c, !(c->p & FC)); return;
                case 5: branch(c, c->p & FC); return;
                case 6: branch(c, !(c->p & FZ)); return;
                default: branch(c, c->p & FZ); return;
                }
            }
            if (bbb == 2 || bbb == 6) { /* implied/stack/flag group */
                switch (op) {
                case 0x08: push(c, (uint8_t)(c->p | FB | FU)); return;
                case 0x28: c->p = (uint8_t)((pull(c) & ~FB) | FU); return;
                case 0x48: push(c, c->a); return;
                case 0x68: c->a = pull(c); setzn(c, c->a); return;
                case 0x88: c->y--; setzn(c, c->y); return;
                case 0xA8: c->y = c->a; setzn(c, c->y); return;
                case 0xC8: c->y++; setzn(c, c->y); return;
                case 0xE8: c->x++; setzn(c, c->x); return;
                case 0x18: c->p &= (uint8_t)~FC; return;
                case 0x38: c->p |= FC; return;
                case 0x58: c->p &= (uint8_t)~FI; return;
                case 0x78: c->p |= FI; return;
                case 0x98: c->a = c->y; setzn(c, c->a); return;
                case 0xB8: c->p &= (uint8_t)~FV; return;
                case 0xD8: c->p &= (uint8_t)~FD; return;
                case 0xF8: c->p |= FD; return;
                default: c->bad_opcode = op; return;
                }
            }
            switch (op) {
            case 0x20: { uint16_t t16 = fetch16(c); uint16_t r = (uint16_t)(c->pc - 1); push(c, (uint8_t)(r >> 8)); push(c, (uint8_t)r); c->pc = t16; return; }
            case 0x4C: c->pc = fetch16(c); return;
            case 0x6C: { uint16_t p16 = fetch16(c); uint16_t lo = cpu6502_read(c, p16), hi = cpu6502_read(c, (uint16_t)((p16 & 0xFF00u) | ((p16 + 1) & 0xFFu))); c->pc = (uint16_t)(lo | (hi << 8)); return; }
            case 0x60: { uint16_t l = pull(c); uint16_t h = pull(c); c->pc = (uint16_t)((l | (h << 8)) + 1); return; }
            case 0x40: c->p = (uint8_t)((pull(c) & ~FB) | FU); { uint16_t l = pull(c); uint16_t h = pull(c); c->pc = (uint16_t)(l | (h << 8)); } return;
            case 0x24: case 0x2C: a = ea(c, op == 0x24 ? 1 : 4); m = cpu6502_read(c, a);
                c->p = (uint8_t)((c->p & ~(FZ | FN | FV)) | ((c->a & m) == 0 ? FZ : 0) | (m & (FN | FV))); return;
            case 0xA0: case 0xA4: case 0xAC: case 0xB4: case 0xBC:
                a = ea(c, op == 0xA0 ? 0 : op == 0xA4 ? 1 : op == 0xAC ? 4 : op == 0xB4 ? 2 : 5);
                c->y = cpu6502_read(c, a); setzn(c, c->y); return;
            case 0x84: case 0x8C: case 0x94: cpu6502_write(c, ea(c, op == 0x84 ? 1 : op == 0x8C ? 4 : 2), c->y); return;
            case 0xC0: case 0xC4: case 0xCC: a = ea(c, op == 0xC0 ? 0 : op == 0xC4 ? 1 : 4); cmp(c, c->y, cpu6502_read(c, a)); return;
            case 0xE0: case 0xE4: case 0xEC: a = ea(c, op == 0xE0 ? 0 : op == 0xE4 ? 1 : 4); cmp(c, c->x, cpu6502_read(c, a)); return;
            default: c->bad_opcode = op; return;
            }
        }
        switch (op) {
        case 0x8A: c->a = c->x; setzn(c, c->a); return;
        case 0xAA: c->x = c->a; setzn(c, c->x); return;
        case 0xCA: c->x--; setzn(c, c->x); return;
        case 0xEA: return;
        case 0x9A: c->sp = c->x; return;
        case 0xBA: c->x = c->sp; setzn(c, c->x); return;
        default: c->bad_opcode = op; return;
        }
    }
}

int cpu6502_call(Cpu6502 *c, uint16_t addr, unsigned long max_steps)
{
    const uint8_t sp0 = c->sp;
    unsigned long n;
    /* sentinel: return address $FFFF-1+1 → pc == $0000 after RTS; we detect sp returning above sp0 */
    push(c, 0xFFu); push(c, 0xFEu);      /* RTS will jump to $FFFF */
    c->pc = addr;
    for (n = 0; n < max_steps; ++n) {
        if (c->pc == 0xFFFFu && c->sp == sp0) return 0;
        cpu6502_step(c);
        if (c->bad_opcode >= 0) return 2;
    }
    return 1;
}
