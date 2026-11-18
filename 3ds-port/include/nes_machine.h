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
    void *user;
};

static inline uint8_t nesRamRead(const NesMachine *m, uint16_t a) { return m->ram[a & 0x7FFu]; }
static inline void nesRamWrite(NesMachine *m, uint16_t a, uint8_t v) { m->ram[a & 0x7FFu] = v; }
static inline uint8_t nesPrgRead(const NesMachine *m, uint16_t a) { return m->prg[a & 0x3FFFu]; }
static inline void nesIoWrite(NesMachine *m, uint16_t a, uint8_t v) { if (m->io_write) m->io_write(m, a, v); }
static inline uint8_t nesIoRead(NesMachine *m, uint16_t a) { return m->io_read ? m->io_read(m, a) : 0u; }
/* Set N and Z from a value (what LDA/STA-free ALU ops do). */
static inline void nesSetNZ(NesMachine *m, uint8_t v) { m->z = (uint8_t)(v == 0u); m->n = (uint8_t)(v >> 7); }


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
