/* NES system layer + recompiled original program: boot, input, state progression, determinism.
 * Usage: test_nes_system rom.nes  (rom built from header.bin + PRG rebuilt from the ASM + CHR_ROM.chr by the CI step). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_system.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL check %d\n", n); return n; } } while (0)

static uint32_t hash(const uint8_t *d, size_t n) { uint32_t h = 2166136261u; size_t i; for (i = 0; i < n; ++i) { h ^= d[i]; h *= 16777619u; } return h; }

static uint8_t g_rom[40976];
static size_t g_romSize;
static NesSystem a, b;

static void runFrames(NesSystem *s, unsigned from, unsigned to, uint8_t pad)
{
    unsigned i;
    for (i = from; i < to; ++i) { nesSystemSetPad(s, pad); if (nesSystemRunFrame(s) != NES_SYS_OK) { printf("code error frame %u pc=%04X\n", i, s->m.pc); exit(1); } }
}

int main(int argc, char **argv)
{
    FILE *f;
    unsigned i;
    uint8_t seq[8];
    static uint8_t bad[40976];
    if (argc < 2 || !(f = fopen(argv[1], "rb"))) { puts("usage: test_nes_system rom.nes"); return 2; }
    g_romSize = fread(g_rom, 1, sizeof(g_rom), f); fclose(f);

    /* ROM validation */
    CHECK(nesSystemInit(&a, g_rom, g_romSize) == NES_SYS_OK, 1);
    memcpy(bad, g_rom, g_romSize); bad[16 + 0x1234] ^= 1u;
    CHECK(nesSystemInit(&b, bad, g_romSize) == NES_SYS_PRG_MISMATCH, 2);
    memcpy(bad, g_rom, g_romSize); bad[0] = 'X';
    CHECK(nesSystemInit(&b, bad, g_romSize) == NES_SYS_BAD_ROM, 3);
    memcpy(bad, g_rom, g_romSize); bad[4] = 2;
    CHECK(nesSystemInit(&b, bad, g_romSize) == NES_SYS_BAD_ROM, 4);
    CHECK(nesSystemInit(&b, g_rom, 100) == NES_SYS_BAD_ROM, 5);

    /* controller shift register: A,B,Select,Start,Up,Down,Left,Right */
    CHECK(nesSystemInit(&a, g_rom, g_romSize) == NES_SYS_OK, 6);
    nesSystemSetPad(&a, 0x09u);                       /* A + Start */
    a.m.io_write(&a.m, 0x4016u, 1u); a.m.io_write(&a.m, 0x4016u, 0u);
    for (i = 0; i < 8; ++i) seq[i] = (uint8_t)(a.m.io_read(&a.m, 0x4016u) & 1u);
    CHECK(seq[0] == 1 && seq[1] == 0 && seq[2] == 0 && seq[3] == 1 && seq[4] == 0 && seq[5] == 0 && seq[6] == 0 && seq[7] == 0, 7);

    /* cold boot: the original program reaches the title menu (state 0) and keeps running */
    CHECK(nesSystemInit(&a, g_rom, g_romSize) == NES_SYS_OK, 8);
    runFrames(&a, 0, 200, 0);
    CHECK(a.m.ram[0x41] == 0x00u, 9);                 /* ram_0041 = state 0: title menu */
    CHECK((a.mask & 0x18u) != 0u, 10);                /* rendering enabled */
    /* title screen has several colours (checkerboard, text, logo) */
    {
        uint8_t seen[64]; unsigned colours = 0;
        memset(seen, 0, sizeof(seen));
        for (i = 0; i < 256u * 240u; ++i) { const unsigned k = (a.fb[i * 3] >> 4) | ((a.fb[i * 3 + 1] >> 4) << 4); if (!seen[k & 63u]) { seen[k & 63u] = 1; ++colours; } }
        CHECK(colours >= 4, 11);
    }
    /* Start on SELECTION A -> track selector (state 1) -> Start -> race (state 2) per tbl_C000/C3BD routes */
    runFrames(&a, 200, 205, 0x08u); runFrames(&a, 205, 300, 0);
    CHECK(a.m.ram[0x41] == 0x01u, 12);
    runFrames(&a, 300, 305, 0x08u); runFrames(&a, 305, 450, 0);
    CHECK(a.m.ram[0x41] == 0x02u, 13);
    /* in the race the on-screen timer (ram $0F... runs) - hold A and check the original code advances time and state */
    {
        int pk = 0; unsigned fr, k;
        for (fr = 450; fr < 1100; ++fr) {
            runFrames(&a, fr, fr + 1, 0x01u);
            CHECK(a.audioCount >= 525u && a.audioCount <= 540u, 18);   /* 32 kHz samples per frame */
            for (k = 0; k < a.audioCount; ++k) { int v = a.audio[k] < 0 ? -a.audio[k] : a.audio[k]; if (v > pk) pk = v; }
        }
        CHECK(pk > 300, 19);                         /* the original sound engine produced audible output in the race */
    }
    CHECK(a.m.ram[0x41] == 0x02u, 14);

    /* determinism: identical inputs give identical pictures and RAM */
    CHECK(nesSystemInit(&b, g_rom, g_romSize) == NES_SYS_OK, 15);
    runFrames(&b, 0, 200, 0); runFrames(&b, 200, 205, 0x08u); runFrames(&b, 205, 300, 0);
    runFrames(&b, 300, 305, 0x08u); runFrames(&b, 305, 450, 0); runFrames(&b, 450, 1100, 0x01u);
    CHECK(hash(a.fb, sizeof(a.fb)) == hash(b.fb, sizeof(b.fb)), 16);
    CHECK(memcmp(a.m.ram, b.m.ram, 0x800) == 0, 17);
    printf("PASS: boot/title/selector/race progression, input shift order, ROM validation, determinism (fb hash %08X).\n", (unsigned)hash(a.fb, sizeof(a.fb)));
    return 0;
}
