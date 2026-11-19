#ifndef NES_GEN_CODE_H
#define NES_GEN_CODE_H
#include "nes_machine.h"
/* Static recompilation of Excitebike/bank_FF.asm (generated).  Runs from `pc` until the sentinel return address is
 * pulled (returns 0), the budget of backward jumps is exhausted (returns 1, m->pc = resume address) or an unknown
 * address is reached (returns 2, m->pc = address). */
int nesRunCode(NesMachine *m, uint16_t pc, long *budget);
/* JSR-style call of the original routine at `addr` (pushes a sentinel return address). Returns nesRunCode's result. */
int nesCall(NesMachine *m, uint16_t addr, long budget);
int nesIsInstructionStart(uint16_t addr);
#endif
