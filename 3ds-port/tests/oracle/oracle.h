#ifndef ORACLE_H
#define ORACLE_H
/* Differential-test harness: run a routine of the ORIGINAL game code (PRG rebuilt from the ASM
 * by tools/asm_to_prg.py) on the reference 6502 and compare with the C translation. Host only. */
#include "cpu6502.h"
#include "nes_machine.h"

typedef struct { uint16_t addr[4096]; uint8_t val[4096]; unsigned n; } OracleIoLog;

typedef struct {
    Cpu6502 cpu;
    NesMachine m;
    uint8_t prg[0x4000];
    OracleIoLog cpuLog, mLog;
    uint8_t ioPort[0x4020];                /* io register backing seen by the port side (snapshot taken at sync) */
    uint32_t rng;
} Oracle;

/* Load PRG image produced by tools/asm_to_prg.py. Returns 0 on success. */
int oracleInit(Oracle *o, const char *prgPath);
uint32_t oracleRand(Oracle *o);
/* Fill RAM and registers identically in both machines with pseudo-random values.
 * $0100-$01FF (stack) is zeroed and not compared. io backing bytes are randomised too. */
void oracleRandomize(Oracle *o);
/* Copy cpu state into m (RAM + a/x/y + flags) — used after the test sets up the CPU side manually. */
void oracleSync(Oracle *o);
/* Run original routine at `addr` on the CPU. Returns cpu6502_call result (0 ok). */
int oracleRunCpu(Oracle *o, uint16_t addr);
/* Compare RAM (excluding stack page) and, if cmpRegs, A/X/Y/C/Z/N; also compare IO logs.
 * ignoreLo..ignoreHi (inclusive, may be 0/0 for none) are skipped. Returns 0 if equal,
 * else prints the first difference and returns 1. */
int oracleCompare(Oracle *o, unsigned cmpRegs, const char *what);
void oracleIgnore(Oracle *o, uint16_t lo, uint16_t hi);   /* add an ignored RAM range */
/* Differential loop: `iters` random states; `prep` (may be NULL) edits the CPU-side state (cpu.ram/regs) to realistic
 * values BEFORE the sync to the port. Both sides then run; RAM (minus ignores), registers selected by cmpRegs
 * (bit0 A, 1 X, 2 Y, 3 C, 4 Z, 5 N) and io-write logs must match. Returns 0 if all iterations match.
 * Iterations whose original run hits the step limit are skipped (counted in *skipped if non-NULL). */
int oracleDiffRoutine(Oracle *o, uint16_t addr, int (*port)(NesMachine *, uint16_t), const char *name, unsigned iters,
                      void (*prep)(Oracle *), unsigned cmpRegs, unsigned *skipped);
#endif
