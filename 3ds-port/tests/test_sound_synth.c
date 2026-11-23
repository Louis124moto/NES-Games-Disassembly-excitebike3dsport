/* Host tests for the native voice synthesizer and the driver->synth chain. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sound_synth.h"

#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL %s\n", msg); return 1; } } while (0)

static int16_t pcm[SOUND_SYNTH_MAX_FRAME_SAMPLES];
static const SoundApuWrite kFrame[1] = { { 0x4017u, 0xC0u } };

/* Runs frames, returns rising edges per second of the signal (sign changes). */
static double frequency(SoundSynth *s, unsigned frames)
{
    unsigned f, i, edges = 0u, total = 0u;
    int prev = 0;
    for (f = 0u; f < frames; ++f) {
        const unsigned n = soundSynthFrame(s, kFrame, 1u, pcm);
        for (i = 0u; i < n; ++i) {
            const int hi = pcm[i] > 0 ? 1 : 0;
            if (hi && !prev) edges++;
            prev = hi;
        }
        total += n;
    }
    return (double)edges * SOUND_SYNTH_RATE / (double)total;
}

static int peakOf(SoundSynth *s, unsigned frames)
{
    unsigned f, i;
    int peak = 0;
    for (f = 0u; f < frames; ++f) {
        const unsigned n = soundSynthFrame(s, kFrame, 1u, pcm);
        for (i = 0u; i < n; ++i) if (abs(pcm[i]) > peak) peak = abs(pcm[i]);
    }
    return peak;
}

int main(void)
{
    SoundSynth s;
    SoundDriver drv;
    int16_t all[SOUND_SYNTH_MAX_FRAME_SAMPLES];
    unsigned f, i, totalSamples = 0u;
    double hz;
    int peak;
    uint32_t hash = 2166136261u;

    soundSynthReset(&s);
    CHECK(peakOf(&s, 10u) == 0, "silent without writes");

    soundSynthReset(&s);                       /* pulse 1, A4-ish */
    soundSynthWrite(&s, 0x4000u, 0xBFu); soundSynthWrite(&s, 0x4002u, 0xFDu); soundSynthWrite(&s, 0x4003u, 0x08u);
    hz = frequency(&s, 60u);
    printf("pulse1 %.1f Hz (expect 440.4)\n", hz);
    CHECK(fabs(hz - 440.4) < 3.0, "pulse1 frequency");

    soundSynthReset(&s);                       /* triangle */
    soundSynthWrite(&s, 0x4008u, 0xFFu); soundSynthWrite(&s, 0x400Au, 0xFDu); soundSynthWrite(&s, 0x400Bu, 0x08u);
    hz = frequency(&s, 60u);
    printf("triangle %.1f Hz (expect 220.2)\n", hz);
    CHECK(fabs(hz - 220.2) < 3.0, "triangle frequency");

    soundSynthReset(&s);                       /* length counter: index 3 = 2 half-frames */
    soundSynthWrite(&s, 0x4000u, 0x9Fu); soundSynthWrite(&s, 0x4002u, 0xFDu); soundSynthWrite(&s, 0x4003u, 0x18u);
    CHECK(peakOf(&s, 1u) > 0, "length-limited pulse sounds in frame 1");
    (void)peakOf(&s, 2u);                      /* let the DC-blocker tail die out */
    CHECK(peakOf(&s, 1u) < 100, "length counter silences the pulse");

    soundSynthReset(&s);                       /* noise */
    soundSynthWrite(&s, 0x400Cu, 0x3Fu); soundSynthWrite(&s, 0x400Eu, 0x00u); soundSynthWrite(&s, 0x400Fu, 0x08u);
    CHECK(peakOf(&s, 5u) > 0, "noise produces output");

    soundSynthReset(&s);                       /* $4015 disable mutes + clears length */
    soundSynthWrite(&s, 0x4000u, 0xBFu); soundSynthWrite(&s, 0x4002u, 0xFDu); soundSynthWrite(&s, 0x4003u, 0x08u);
    soundSynthWrite(&s, 0x4015u, 0x00u);
    (void)peakOf(&s, 2u);
    CHECK(peakOf(&s, 2u) < 100, "$4015 disable silences channels");

    /* Driver -> synth chain with the real music request ($FB = 1, title jingle). */
    soundDriverReset(&drv, kSoundData);
    soundSynthReset(&s);
    peak = 0;
    for (f = 0u; f < 600u; ++f) {
        unsigned n;
        soundDriverSetRequests(&drv, f == 0u ? 1u : 0u, 0u, 0u, 0u, 0u);
        soundDriverFrame(&drv);
        n = soundSynthFrame(&s, drv.writes, drv.write_count, all);
        totalSamples += n;
        for (i = 0u; i < n; ++i) {
            if (abs(all[i]) > peak) peak = abs(all[i]);
            hash = (hash ^ (uint16_t)all[i]) * 16777619u;
        }
    }
    printf("music chain: %u samples, peak %d, hash %08X\n", totalSamples, peak, (unsigned)hash);
    CHECK(peak > 2000 && peak < 30000, "music chain audible and unclipped");
    CHECK(totalSamples >= 319400u && totalSamples <= 319560u, "sample count tracks the 60.1 Hz source frame");
    puts("PASS: synthesizer voices, length/enable behaviour and the driver->synth chain.");
    return 0;
}
