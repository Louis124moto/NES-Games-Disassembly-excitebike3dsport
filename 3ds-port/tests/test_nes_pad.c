/* Differential test: native controller layer vs original sub_D347 (both pads). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_pad.h"
#include "cpu6502.h"

static uint32_t rng = 0xBADC0DE5u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

static uint8_t shifting[2], raw[2], lastStrobe;

static uint8_t onRead(Cpu6502 *c, uint16_t a)
{
    if (a == 0x4016u || a == 0x4017u) {
        const unsigned pad = a - 0x4016u;
        const uint8_t bit = (uint8_t)((shifting[pad] >> 7) & 1u);
        shifting[pad] = (uint8_t)(shifting[pad] << 1);
        return bit;
    }
    return c->io[a];
}

static void onWrite(Cpu6502 *c, uint16_t a, uint8_t v)
{
    if (a == 0x4016u) {
        if (lastStrobe && !(v & 1u)) { shifting[0] = raw[0]; shifting[1] = raw[1]; }
        lastStrobe = (uint8_t)(v & 1u);
    }
    c->io[a] = v;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    NesPad pad[2];
    unsigned frame, frames = argc > 2 ? (unsigned)atoi(argv[2]) : 200000u;
    FILE *fp;

    if (argc < 2) { puts("usage: test_nes_pad <prg> [frames]"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    cpu.read_hook = onRead; cpu.write_hook = onWrite;
    nesPadReset(&pad[0]); nesPadReset(&pad[1]);
    memset(cpu.ram, 0, sizeof(cpu.ram));

    for (frame = 0u; frame < frames; ++frame) {
        unsigned i;
        for (i = 0u; i < 2u; ++i) {
            const unsigned r = nb();
            if (r < 40u) raw[i] = nb();                         /* new random state */
            else if (r < 60u) raw[i] = (uint8_t)(raw[i] ^ (1u << (nb() & 7u)));
            else if (r < 70u) raw[i] = 0u;                      /* release everything */
            /* else: keep (long holds, A held across frames) */
        }
        cpu.a = cpu.y = 0u; cpu.x = 0x55u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.bad_opcode = -1;
        if (cpu6502_call(&cpu, 0xD347u, 20000u) != 0) { puts("FAIL oracle did not return"); return 1; }
        nesPadScan(&pad[0], raw[0]);
        nesPadScan(&pad[1], raw[1]);
        if (pad[0].hold != cpu.ram[0x14] || pad[0].press != cpu.ram[0x15] ||
            pad[1].hold != cpu.ram[0x16] || pad[1].press != cpu.ram[0x17]) {
            printf("FAIL frame %u raw=%02X/%02X native=%02X,%02X/%02X,%02X ASM=%02X,%02X/%02X,%02X\n", frame,
                   raw[0], raw[1], pad[0].hold, pad[0].press, pad[1].hold, pad[1].press,
                   cpu.ram[0x14], cpu.ram[0x15], cpu.ram[0x16], cpu.ram[0x17]);
            return 1;
        }
        if (cpu.x != 2u) { printf("FAIL X at exit is %u, driver assumes 2\n", cpu.x); return 1; }
    }
    printf("PASS: %u frames of two-pad input identical to sub_D347 (X=2 at exit confirmed).\n", frames);
    return 0;
}
