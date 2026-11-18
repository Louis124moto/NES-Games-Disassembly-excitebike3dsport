#ifndef CPU6502_H
#define CPU6502_H
/* Host-only reference 6502 core used as a differential-test oracle (never linked into the 3DS build).
 * Memory: 2 KiB RAM mirrored through $1FFF, PRG at $C000-$FFFF (NROM-128 mirrored at $8000),
 * everything else goes through the optional hooks (else plain bytes). */
#include <stdint.h>
typedef struct Cpu6502 Cpu6502;
struct Cpu6502 {
    uint8_t a, x, y, sp, p;
    uint16_t pc;
    uint8_t ram[0x800];
    uint8_t prg[0x4000];
    uint8_t io[0x10000];                  /* backing store for $2000-$7FFF accesses */
    uint8_t (*read_hook)(Cpu6502 *, uint16_t);   /* may be NULL */
    void (*write_hook)(Cpu6502 *, uint16_t, uint8_t); /* may be NULL */
    void *user;
    unsigned long cycles;
    int bad_opcode;                       /* set to the opcode if an unsupported one is fetched */
};
void cpu6502_reset_state(Cpu6502 *c);
uint8_t cpu6502_read(Cpu6502 *c, uint16_t a);
void cpu6502_write(Cpu6502 *c, uint16_t a, uint8_t v);
/* JSR-style call: pushes a sentinel return address and runs until the matching RTS.
 * Returns 0 on return, 1 on step limit, 2 on bad opcode. */
int cpu6502_call(Cpu6502 *c, uint16_t addr, unsigned long max_steps);
/* Single instruction. */
void cpu6502_step(Cpu6502 *c);
#endif
