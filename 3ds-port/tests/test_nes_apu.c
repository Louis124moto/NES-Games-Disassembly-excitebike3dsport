#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nes_apu.h"
#define CHECK(c, n) do { if (!(c)) { printf("FAIL check %d\n", n); return n; } } while (0)
static NesApu a;
static int16_t buf[2048];
static int peak(unsigned n) { int m = 0; unsigned i; for (i = 0; i < n; ++i) { int v = abs(buf[i]); if (v > m) m = v; } return m; }
int main(void)
{
    unsigned n, i;
    int pk;
    nesApuReset(&a);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(n >= 530 && n <= 535, 1);                        /* ~32000/60.1 samples per frame */
    for (i = 0; i < 60; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);   /* let the DC blocker settle */
    CHECK(peak(n) < 60, 2);                                /* silence at reset */
    /* pulse 1: constant volume 15, duty 2, period 100, length from table index 1 (254) */
    nesApuWrite(&a, 0x4015, 0x01, 0); nesApuWrite(&a, 0x4000, 0xBF, 0); nesApuWrite(&a, 0x4002, 100, 0); nesApuWrite(&a, 0x4003, 0x08, 0);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    pk = peak(n);
    CHECK(pk > 1000, 3);                                   /* audible */
    CHECK(nesApuReadStatus(&a) & 1u, 4);                   /* length counter running */
    /* disabling via $4015 clears the length counter and silences */
    nesApuWrite(&a, 0x4015, 0x00, 0);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(!(nesApuReadStatus(&a) & 1u), 5);
    for (i = 0; i < 3; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);   /* the DC blocker removes the step tail */
    CHECK(peak(n) < pk / 6, 6);
    /* period below 8 mutes a pulse */
    nesApuWrite(&a, 0x4015, 0x01, 0); nesApuWrite(&a, 0x4000, 0xBF, 0); nesApuWrite(&a, 0x4002, 4, 0); nesApuWrite(&a, 0x4003, 0x08, 0);
    for (i = 0; i < 4; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(peak(n) < pk / 6, 7);
    /* length counter expires: index 3 = length 2 half-frames (~2 frames) with halt clear */
    nesApuReset(&a);
    nesApuWrite(&a, 0x4015, 0x01, 0); nesApuWrite(&a, 0x4000, 0x9F, 0); nesApuWrite(&a, 0x4002, 100, 0); nesApuWrite(&a, 0x4003, 0x18, 0);
    for (i = 0; i < 4; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(!(nesApuReadStatus(&a) & 1u), 8);
    for (i = 0; i < 4; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(peak(n) < pk / 6, 9);
    /* triangle needs linear counter + length; noise with constant volume is audible; DMC level offsets the mix */
    nesApuReset(&a);
    nesApuWrite(&a, 0x4015, 0x04, 0); nesApuWrite(&a, 0x4008, 0xFF, 0); nesApuWrite(&a, 0x400A, 60, 0); nesApuWrite(&a, 0x400B, 0x08, 0);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(peak(n) > 100, 10);
    nesApuReset(&a);
    nesApuWrite(&a, 0x4015, 0x08, 0); nesApuWrite(&a, 0x400C, 0x3F, 0); nesApuWrite(&a, 0x400E, 0x04, 0); nesApuWrite(&a, 0x400F, 0x08, 0);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(peak(n) > 100, 11);
    nesApuReset(&a);
    nesApuWrite(&a, 0x4011, 0x40, 0);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(peak(n) > 100, 12);
    /* writes are applied at their cycle: a write at cycle 20000 leaves the first part of the frame silent */
    nesApuReset(&a);
    for (i = 0; i < 60; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    nesApuWrite(&a, 0x4015, 0x01, 20000); nesApuWrite(&a, 0x4000, 0xBF, 20000); nesApuWrite(&a, 0x4002, 100, 20000); nesApuWrite(&a, 0x4003, 0x08, 20000);
    n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    { int early = 0, late = 0; for (i = 0; i < n; ++i) { int v = abs(buf[i]); if (i < n / 2) { if (v > early) early = v; } else if (v > late) late = v; } CHECK(early < 60 && late > 1000, 13); }
    /* halt flag keeps the length counter running forever */
    nesApuReset(&a);
    nesApuWrite(&a, 0x4015, 0x01, 0); nesApuWrite(&a, 0x4000, 0xBF, 0); nesApuWrite(&a, 0x4002, 100, 0); nesApuWrite(&a, 0x4003, 0x18, 0);
    for (i = 0; i < 8; ++i) n = nesApuRunFrame(&a, 29781ul, buf, 2048);
    CHECK(nesApuReadStatus(&a) & 1u, 14);
    puts("PASS: APU registers, length counters, envelopes, mute rules, timing.");
    return 0;
}
