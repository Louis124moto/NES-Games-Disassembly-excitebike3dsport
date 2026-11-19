/* Host smoke runner: boots the recompiled original program on the NES system layer and dumps frames (PPM).
 * Usage: test_nes_system_run rom.nes outprefix frames [pad-script]   pad-script = "frame:padbits,frame:padbits,..." */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_system.h"

static void ppm(const NesSystem *s, const char *prefix, unsigned frame)
{
    char name[256];
    FILE *f;
    snprintf(name, sizeof(name), "%s_%04u.ppm", prefix, frame);
    f = fopen(name, "wb");
    if (!f) return;
    fprintf(f, "P6\n256 240\n255\n");
    fwrite(s->fb, 1, 256u * 240u * 3u, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    static NesSystem s;
    static uint8_t rom[40976];
    unsigned frames, i, dumpEvery = 0;
    size_t n;
    FILE *f;
    if (argc < 4) return 2;
    f = fopen(argv[1], "rb");
    if (!f) return 2;
    n = fread(rom, 1, sizeof(rom), f); fclose(f);
    if (nesSystemInit(&s, rom, n) != NES_SYS_OK) { puts("init failed"); return 1; }
    frames = (unsigned)atoi(argv[3]);
    if (getenv("DUMP_EVERY")) dumpEvery = (unsigned)atoi(getenv("DUMP_EVERY"));
    for (i = 0; i < frames; ++i) {
        const char *p = argc > 4 ? argv[4] : "";
        uint8_t pad = 0;
        while (*p) {
            unsigned fr, bits;
            int used;
            if (sscanf(p, "%u:%u%n", &fr, &bits, &used) != 2) break;
            if (fr <= i) pad = (uint8_t)bits;
            p += used;
            if (*p == ',') ++p;
        }
        nesSystemSetPad(&s, pad);
        if (nesSystemRunFrame(&s) != NES_SYS_OK) { printf("code error at frame %u pc=%04X\n", i, s.m.pc); ppm(&s, argv[2], i); return 1; }
        if (dumpEvery && (i % dumpEvery) == 0) ppm(&s, argv[2], i);
    }
    ppm(&s, argv[2], frames);
    printf("ran %u frames ok; state ram41=%02X ram40=%02X\n", frames, s.m.ram[0x41], s.m.ram[0x40]);
    return 0;
}
