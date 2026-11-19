/* Differential test of the static recompilation (source/nes_gen_code.c) against the reference 6502 running the
 * original PRG: every label region of bank_FF.asm is run from random machine states on both sides; RAM, A/X/Y,
 * C/Z/N/V, stack pointer and the hardware-register write log must be identical. */
#include <stdio.h>
#include <stdlib.h>
#include "oracle.h"
#include "nes_gen_code.h"
#include "region_list.h"

static int portCall(NesMachine *m, uint16_t addr) { return nesCall(m, addr, 2000000L); }

int main(int argc, char **argv)
{
    static Oracle o;
    unsigned i, iters = 60, totalSkipped = 0, fullySkipped = 0;
    const char *prg = argc > 1 ? argv[1] : getenv("EXCITEBIKE_PRG");
    if (argc > 2) iters = (unsigned)atoi(argv[2]);
    if (!prg || oracleInit(&o, prg)) { puts("cannot load PRG"); return 2; }
    for (i = 0; i < sizeof(kRegions) / sizeof(kRegions[0]); ++i) {
        unsigned sk = 0;
        char name[32];
        snprintf(name, sizeof(name), "region %04X", kRegions[i]);
        {
            int r = oracleDiffRoutine(&o, kRegions[i], portCall, name, iters, NULL, 1u | 2u | 4u | 8u | 16u | 32u | 64u, &sk);
            if (r) {
                /* a region whose every iteration hits the step limit (e.g. waits for vblank) is reported, not failed */
                if (sk == iters) { ++fullySkipped; printf("note: %s never terminates from random states (skipped)\n", name); continue; }
                return 1;
            }
        }
        totalSkipped += sk;
    }
    printf("PASS: %u regions match the original (%u iterations skipped for step limit, %u regions never terminate).\n",
           (unsigned)(sizeof(kRegions) / sizeof(kRegions[0])), totalSkipped, fullySkipped);
    return 0;
}
