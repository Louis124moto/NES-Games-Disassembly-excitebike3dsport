#include <stdio.h>
#include <stdlib.h>
#include "oracle.h"
#include "nes_race_decl.h"

static void prepDivisor(Oracle *o)
{
    /* sub_DFA4 loops forever for X == 0; real callers always pass 10. */
    if (o->cpu.x == 0u) o->cpu.x = 10u;
}

int main(int argc, char **argv)
{
    static Oracle o;
    unsigned skipped = 0;
    const char *prg = argc > 1 ? argv[1] : getenv("EXCITEBIKE_PRG");
    if (!prg || oracleInit(&o, prg)) { puts("cannot load PRG (pass path or EXCITEBIKE_PRG)"); return 2; }
    /* regs: A X C Z N compared (Y untouched) */
    if (oracleDiffRoutine(&o, 0xDFA4, nesR_DFA4, "sub_DFA4", 3000, prepDivisor, 1u | 2u | 8u | 16u | 32u, &skipped)) return 1;
    if (oracleDiffRoutine(&o, 0xDFA2, nesR_DFA2, "sub_DFA2", 3000, NULL, 1u | 2u | 8u | 16u | 32u, &skipped)) return 1;
    puts("PASS: sub_DFA2/DFA4 match the original code (differential).");
    return 0;
}
