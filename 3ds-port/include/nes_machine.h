#ifndef NES_MACHINE_H
#define NES_MACHINE_H
/*
 * Flat NES machine model used by the source-translated race code (Stage 3).
 *
 * Every translated routine has the signature `void nesXxx(NesMachine *m)` and mirrors one
 * routine of Excitebike/bank_FF.asm: inputs/outputs in registers go through m->a/x/y and the
 * carry/zero/negative flags exactly where the 6502 code uses them, memory goes through m->ram
 * (2 KiB, $0000-$07FF; OAM is $0200-$02FF) and ROM tables are READ FROM THE PRG IMAGE (m->prg,
 * 16 KiB mapped at $C000-$FFFF), never retyped.  Hardware register accesses ($2000-$401F) go
 * through nesIoWrite/nesIoRead so hosts can log or emulate them.
 */
#include <stdint.h>

typedef struct NesMachine NesMachine;
struct NesMachine {
    uint8_t ram[0x800];
    uint8_t a, x, y;
    uint8_t c, z, n, v;                 /* carry/zero/negative/overflow, each 0 or 1 */
    const uint8_t *prg;                 /* 16 KiB image, CPU address $C000 */
    void (*io_write)(NesMachine *, uint16_t addr, uint8_t value);   /* may be NULL */
    uint8_t (*io_read)(NesMachine *, uint16_t addr);                /* may be NULL (reads 0) */
    uint8_t sp;                         /* 6502 stack pointer (stack lives in ram[$100-$1FF]) */
    uint8_t pother;                     /* I ($04) and D ($08) status bits, kept only for PHP/PLP */
    uint16_t pc;                        /* resume/error address reported by nesRunCode */
    unsigned long cyc;                  /* CPU cycles executed (approximate: base cycles + taken branches) */
    void *user;
};

static inline uint8_t nesRamRead(const NesMachine *m, uint16_t a) { return m->ram[a & 0x7FFu]; }
static inline void nesRamWrite(NesMachine *m, uint16_t a, uint8_t v) { m->ram[a & 0x7FFu] = v; }
static inline uint8_t nesPrgRead(const NesMachine *m, uint16_t a) { return m->prg[a & 0x3FFFu]; }
static inline void nesIoWrite(NesMachine *m, uint16_t a, uint8_t v) { if (m->io_write) m->io_write(m, a, v); }
static inline uint8_t nesIoRead(NesMachine *m, uint16_t a) { return m->io_read ? m->io_read(m, a) : 0u; }
/* Set N and Z from a value (what LDA/STA-free ALU ops do). */
static inline void nesSetNZ(NesMachine *m, uint8_t v) { m->z = (uint8_t)(v == 0u); m->n = (uint8_t)(v >> 7); }


/* Memory access as the 6502 sees it. */
static inline uint8_t nesRd(NesMachine *m, uint16_t a)
{
    if (a < 0x2000u) return m->ram[a & 0x7FFu];
    if (a >= 0x8000u) return m->prg[a & 0x3FFFu];
    if (a < 0x4020u) return nesIoRead(m, a);
    return 0u;
}
static inline void nesWr(NesMachine *m, uint16_t a, uint8_t v)
{
    if (a < 0x2000u) m->ram[a & 0x7FFu] = v;
    else if (a >= 0x2000u && a < 0x4020u) nesIoWrite(m, a, v);
}
static inline uint16_t nesPtrX(NesMachine *m, uint8_t z) { const uint8_t p = (uint8_t)(z + m->x); return (uint16_t)(m->ram[p] | (m->ram[(uint8_t)(p + 1u)] << 8)); }
static inline uint16_t nesPtrY(NesMachine *m, uint8_t z) { return (uint16_t)((m->ram[z] | (m->ram[(uint8_t)(z + 1u)] << 8)) + m->y); }
static inline void nesPush(NesMachine *m, uint8_t v) { m->ram[0x100u + m->sp] = v; m->sp = (uint8_t)(m->sp - 1u); }
static inline uint8_t nesPull(NesMachine *m) { m->sp = (uint8_t)(m->sp + 1u); return m->ram[0x100u + m->sp]; }
static inline uint8_t nesGetP(const NesMachine *m, unsigned brk) { return (uint8_t)(m->c | (m->z << 1) | m->pother | (brk ? 0x10u : 0u) | 0x20u | (m->v << 6) | (m->n << 7)); }
static inline void nesSetP(NesMachine *m, uint8_t p) { m->c = (uint8_t)(p & 1u); m->z = (uint8_t)((p >> 1) & 1u); m->pother = (uint8_t)(p & 0x0Cu); m->v = (uint8_t)((p >> 6) & 1u); m->n = (uint8_t)(p >> 7); }
/* JMP (ptr) with the 6502 page-wrap bug. */
static inline uint16_t nesReadVec(NesMachine *m, uint16_t p) { return (uint16_t)(nesRd(m, p) | (nesRd(m, (uint16_t)((p & 0xFF00u) | ((p + 1u) & 0xFFu))) << 8)); }

/* 6502 ALU helpers with exact flag behaviour (no decimal mode on the NES 2A03). */
static inline void nesAdc(NesMachine *m, uint8_t v)
{
    const unsigned s = (unsigned)m->a + v + m->c;
    const uint8_t r = (uint8_t)s;
    m->c = (uint8_t)(s > 0xFFu);
    m->v = (uint8_t)(((~(m->a ^ v)) & (m->a ^ r) & 0x80u) != 0u);
    m->a = r; nesSetNZ(m, r);
}
static inline void nesSbc(NesMachine *m, uint8_t v) { nesAdc(m, (uint8_t)~v); }
/* CMP/CPX/CPY: compare register value r with v. */
static inline void nesCmp(NesMachine *m, uint8_t r, uint8_t v) { m->c = (uint8_t)(r >= v); nesSetNZ(m, (uint8_t)(r - v)); }
/* ASL/LSR/ROL/ROR on a value, updating C,N,Z, returning the result. */
static inline uint8_t nesAsl(NesMachine *m, uint8_t v) { m->c = (uint8_t)(v >> 7); v = (uint8_t)(v << 1); nesSetNZ(m, v); return v; }
static inline uint8_t nesLsr(NesMachine *m, uint8_t v) { m->c = (uint8_t)(v & 1u); v = (uint8_t)(v >> 1); nesSetNZ(m, v); return v; }
static inline uint8_t nesRol(NesMachine *m, uint8_t v) { const uint8_t c = m->c; m->c = (uint8_t)(v >> 7); v = (uint8_t)((v << 1) | c); nesSetNZ(m, v); return v; }
static inline uint8_t nesRor(NesMachine *m, uint8_t v) { const uint8_t c = m->c; m->c = (uint8_t)(v & 1u); v = (uint8_t)((v >> 1) | (c << 7)); nesSetNZ(m, v); return v; }
/* LDA/LDX/LDY style loads: set N,Z, return value. */
static inline uint8_t nesLoad(NesMachine *m, uint8_t v) { nesSetNZ(m, v); return v; }

#endif
