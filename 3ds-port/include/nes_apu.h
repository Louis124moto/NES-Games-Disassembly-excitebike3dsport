#ifndef NES_APU_H
#define NES_APU_H
/* NES 2A03 APU model (pulse x2, triangle, noise, DMC direct level) driven by timestamped register writes.
 * The original program only writes $4000-$400F, $4011, $4015 and $4017; the DMC sample player is not used by the game. */
#include <stdint.h>

#define NES_APU_RATE 32000u
#define NES_APU_MAX_WRITES 512u

typedef struct {
    uint8_t duty, constVol, halt, vol;       /* $4000/$4004/$400C fields */
    uint8_t sweepEn, sweepPeriod, sweepNeg, sweepShift, sweepReload; uint8_t sweepDiv;
    uint16_t timerPeriod, timer;
    uint8_t seq, length, envStart, envDiv, envDecay;
    uint8_t enabled;
} NesApuPulse;

typedef struct {
    NesApuPulse p[2];
    /* triangle */
    uint8_t triControl, triLinearReload, triLinear, triReloadFlag, triLength, triSeq, triEnabled;
    uint16_t triPeriod, triTimer;
    /* noise */
    uint8_t nHalt, nConst, nVol, nMode, nEnabled, nLength, nEnvStart, nEnvDiv, nEnvDecay;
    uint16_t nPeriod, nTimer, nShift;
    /* dmc */
    uint8_t dmcLevel;
    /* frame counter */
    uint8_t fcMode, fcIrqInhibit;
    unsigned long fcCycle;          /* CPU cycles into the 4/5-step sequence */
    uint8_t cpuOdd;                 /* pulse/noise timers run on every second CPU cycle */
    /* pending register writes for the frame being rendered */
    struct { unsigned long cycle; uint16_t addr; uint8_t val; } w[NES_APU_MAX_WRITES];
    unsigned nw;
    double acc; unsigned accN;                         /* resampler state */
    double hpIn, hpOut;                                /* DC-blocking high-pass (removes the mixer's constant offset) */
} NesApu;

void nesApuReset(NesApu *a);
/* Queue a register write that happened at CPU cycle `cycleInFrame` of the current frame. */
void nesApuWrite(NesApu *a, uint16_t addr, uint8_t val, unsigned long cycleInFrame);
/* $4015 read: length-counter status bits (bit0 pulse1, 1 pulse2, 2 triangle, 3 noise). */
uint8_t nesApuReadStatus(const NesApu *a);
/* Advance the APU by `cycles` CPU cycles (applying queued writes at their cycles) and append 32 kHz mono samples.
 * Returns the number of samples produced (<= cap). */
unsigned nesApuRunFrame(NesApu *a, unsigned long cycles, int16_t *out, unsigned cap);
#endif
