/*
 * Differential test: native sound driver vs the original sub_F844 run on the
 * 6502 oracle. Both get identical request streams; every frame the full
 * driver RAM and the exact ordered APU write log must match.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sound_driver.h"
#include "cpu6502.h"

static uint32_t rng = 0x5EED1234u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

static SoundApuWrite oracleLog[256];
static unsigned oracleCount;

static void onWrite(Cpu6502 *c, uint16_t a, uint8_t v)
{
    if (a >= 0x4000u && a <= 0x4017u && oracleCount < 256u) {
        oracleLog[oracleCount].reg = a;
        oracleLog[oracleCount].value = v;
        oracleCount++;
    }
    c->io[a] = v;
}

static int compare(const SoundDriver *d, const Cpu6502 *c, unsigned seq, unsigned frame)
{
    unsigned i;
    for (i = 0xF0u; i <= 0xFFu; ++i)
        if (d->zp[i] != c->ram[i]) {
            printf("FAIL seq %u frame %u: $%02X native=%02X ASM=%02X\n", seq, frame, i, d->zp[i], c->ram[i]);
            return 1;
        }
    for (i = 0u; i < 2u; ++i)
        if (d->zp[i] != c->ram[i]) {
            printf("FAIL seq %u frame %u: scratch $%02X native=%02X ASM=%02X\n", seq, frame, i, d->zp[i], c->ram[i]);
            return 1;
        }
    for (i = 0x07E0u; i <= 0x07FFu; ++i)
        if (d->high[i - 0x07E0u] != c->ram[i]) {
            printf("FAIL seq %u frame %u: $%04X native=%02X ASM=%02X\n", seq, frame, i, d->high[i - 0x07E0u], c->ram[i]);
            return 1;
        }
    if (d->write_count != oracleCount) {
        printf("FAIL seq %u frame %u: write count native=%u ASM=%u\n", seq, frame, d->write_count, oracleCount);
        return 1;
    }
    for (i = 0u; i < oracleCount; ++i)
        if (d->writes[i].reg != oracleLog[i].reg || d->writes[i].value != oracleLog[i].value) {
            printf("FAIL seq %u frame %u: write %u native=%04X:%02X ASM=%04X:%02X\n", seq, frame, i,
                   d->writes[i].reg, d->writes[i].value, oracleLog[i].reg, oracleLog[i].value);
            return 1;
        }
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    static uint8_t window[SOUND_DATA_SIZE];
    static SoundDriver drv;
    unsigned seqs = argc > 2 ? (unsigned)atoi(argv[2]) : 400u;
    unsigned frames = argc > 3 ? (unsigned)atoi(argv[3]) : 3000u;
    unsigned seq, f;
    unsigned long totalWrites = 0u;
    unsigned char seenReg[0x18];
    FILE *fp;

    if (argc < 2) { puts("usage: test_sound_driver <prg> [sequences] [frames]"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    memcpy(window, cpu.prg + (SOUND_DATA_BASE - 0xC000u), sizeof(window));
    if (memcmp(window, kSoundData, sizeof(window)) != 0) { puts("FAIL source/sound_data.c is stale; rerun tools/gen_sound_data.py"); return 1; }
    memset(seenReg, 0, sizeof(seenReg));

    for (seq = 0u; seq < seqs; ++seq) {
        uint8_t fc = 0u;
        cpu6502_reset_state(&cpu);
        cpu.write_hook = onWrite;
        memset(cpu.ram, 0, sizeof(cpu.ram));
        soundDriverReset(&drv, window);
        for (f = 0u; f < frames; ++f) {
            uint8_t fb = 0u, fd = 0u, fe = 0u, ff = 0u;
            unsigned r = nb();
            if ((nb() & 0x3Fu) == 0u) fc = (nb() & 1u) ? nb() : 0u;
            if (r < 6u) fb = (uint8_t)(1u << (nb() & 7u));
            else if (r < 8u) fb = nb();
            if ((nb() & 0x1Fu) == 0u) fd = (uint8_t)(nb() & 0x3Fu);
            if ((nb() & 0x1Fu) == 0u) ff = (nb() & 1u) ? (uint8_t)(1u << (nb() & 7u)) : nb();
            if ((nb() & 0x7Fu) == 0u) fe = (uint8_t)(nb() & 1u ? 1u : 0u);
            soundDriverSetRequests(&drv, fb, fc, fd, fe, ff);
            cpu.ram[0xFB] = fb; cpu.ram[0xFC] = fc; cpu.ram[0xFD] = fd; cpu.ram[0xFE] = fe; cpu.ram[0xFF] = ff;
            oracleCount = 0u;
            cpu.a = 0u; cpu.x = 2u; cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.bad_opcode = -1;
            if (cpu6502_call(&cpu, 0xF844u, 200000u) != 0) { puts("FAIL oracle did not return"); return 1; }
            soundDriverFrame(&drv);
            if (compare(&drv, &cpu, seq, f)) { printf("req fb=%02X fc=%02X fd=%02X fe=%02X ff=%02X\n", fb, fc, fd, fe, ff); { unsigned k; for (k=0;k<oracleCount;++k) printf(" asm %04X:%02X", oracleLog[k].reg, oracleLog[k].value); printf("\n"); for (k=0;k<drv.write_count;++k) printf(" nat %04X:%02X", drv.writes[k].reg, drv.writes[k].value); printf("\n F8F9 asm=%02X%02X nat=%02X%02X\n", cpu.ram[0xF9], cpu.ram[0xF8], drv.zp[0xF9], drv.zp[0xF8]); } return 1; }
            for (unsigned i = 0u; i < drv.write_count; ++i) seenReg[drv.writes[i].reg - 0x4000u] = 1u;
            totalWrites += drv.write_count;
        }
    }
    {
        unsigned i, n = 0u;
        for (i = 0u; i < 0x18u; ++i) n += seenReg[i];
        printf("PASS: %u sequences x %u frames identical to ASM (%lu APU writes, %u distinct registers).\n",
               seqs, frames, totalWrites, n);
    }
    return 0;
}
