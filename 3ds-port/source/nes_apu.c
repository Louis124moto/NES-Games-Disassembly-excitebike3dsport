#include "nes_apu.h"
#include <string.h>

static const uint8_t kLength[32] = {10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                                    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};
static const uint8_t kDuty[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 0, 0, 0}, {1, 0, 0, 1, 1, 1, 1, 1}};
static const uint16_t kNoisePeriod[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};
static const uint8_t kTri[32] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

void nesApuReset(NesApu *a)
{
    memset(a, 0, sizeof(*a));
    a->nShift = 1u;
    a->p[0].sweepNeg = 0;
}

void nesApuWrite(NesApu *a, uint16_t addr, uint8_t val, unsigned long cycle)
{
    if (a->nw < NES_APU_MAX_WRITES) { a->w[a->nw].cycle = cycle; a->w[a->nw].addr = addr; a->w[a->nw].val = val; ++a->nw; }
}

uint8_t nesApuReadStatus(const NesApu *a)
{
    return (uint8_t)((a->p[0].length != 0u) | ((a->p[1].length != 0u) << 1) | ((a->triLength != 0u) << 2) | ((a->nLength != 0u) << 3));
}

static void applyWrite(NesApu *a, uint16_t addr, uint8_t v)
{
    if (addr >= 0x4000u && addr <= 0x4007u) {
        NesApuPulse *p = &a->p[(addr >> 2) & 1u];
        switch (addr & 3u) {
        case 0: p->duty = (uint8_t)(v >> 6); p->halt = (uint8_t)((v >> 5) & 1u); p->constVol = (uint8_t)((v >> 4) & 1u); p->vol = (uint8_t)(v & 15u); break;
        case 1: p->sweepEn = (uint8_t)(v >> 7); p->sweepPeriod = (uint8_t)((v >> 4) & 7u); p->sweepNeg = (uint8_t)((v >> 3) & 1u); p->sweepShift = (uint8_t)(v & 7u); p->sweepReload = 1u; break;
        case 2: p->timerPeriod = (uint16_t)((p->timerPeriod & 0x700u) | v); break;
        default:
            p->timerPeriod = (uint16_t)((p->timerPeriod & 0xFFu) | ((unsigned)(v & 7u) << 8));
            if (p->enabled) p->length = kLength[v >> 3];
            p->seq = 0u; p->envStart = 1u; break;
        }
    } else if (addr == 0x4008u) { a->triControl = (uint8_t)(v >> 7); a->triLinearReload = (uint8_t)(v & 0x7Fu); }
    else if (addr == 0x400Au) a->triPeriod = (uint16_t)((a->triPeriod & 0x700u) | v);
    else if (addr == 0x400Bu) { a->triPeriod = (uint16_t)((a->triPeriod & 0xFFu) | ((unsigned)(v & 7u) << 8)); if (a->triEnabled) a->triLength = kLength[v >> 3]; a->triReloadFlag = 1u; }
    else if (addr == 0x400Cu) { a->nHalt = (uint8_t)((v >> 5) & 1u); a->nConst = (uint8_t)((v >> 4) & 1u); a->nVol = (uint8_t)(v & 15u); }
    else if (addr == 0x400Eu) { a->nMode = (uint8_t)(v >> 7); a->nPeriod = kNoisePeriod[v & 15u]; }
    else if (addr == 0x400Fu) { if (a->nEnabled) a->nLength = kLength[v >> 3]; a->nEnvStart = 1u; }
    else if (addr == 0x4011u) a->dmcLevel = (uint8_t)(v & 0x7Fu);
    else if (addr == 0x4015u) {
        a->p[0].enabled = (uint8_t)(v & 1u); a->p[1].enabled = (uint8_t)((v >> 1) & 1u); a->triEnabled = (uint8_t)((v >> 2) & 1u); a->nEnabled = (uint8_t)((v >> 3) & 1u);
        if (!a->p[0].enabled) a->p[0].length = 0u;
        if (!a->p[1].enabled) a->p[1].length = 0u;
        if (!a->triEnabled) a->triLength = 0u;
        if (!a->nEnabled) a->nLength = 0u;
    } else if (addr == 0x4017u) {
        a->fcMode = (uint8_t)(v >> 7); a->fcIrqInhibit = (uint8_t)((v >> 6) & 1u); a->fcCycle = 0u;
        /* 5-step mode clocks quarter+half immediately; handled at the next check via flag */
        if (a->fcMode) a->fcCycle = 0xFFFFFFFFul;   /* marker: clock now */
    }
}

static void clockEnv(uint8_t *start, uint8_t *div, uint8_t *decay, uint8_t vol, uint8_t loop)
{
    if (*start) { *start = 0u; *decay = 15u; *div = vol; }
    else if (*div == 0u) { *div = vol; if (*decay) --*decay; else if (loop) *decay = 15u; }
    else --*div;
}
static unsigned sweepTarget(const NesApuPulse *p, unsigned ch)
{
    const unsigned d = p->timerPeriod >> p->sweepShift;
    if (p->sweepNeg) return p->timerPeriod - d - (ch == 0u ? 1u : 0u);
    return p->timerPeriod + d;
}
static void clockQuarter(NesApu *a)
{
    unsigned i;
    for (i = 0; i < 2u; ++i) clockEnv(&a->p[i].envStart, &a->p[i].envDiv, &a->p[i].envDecay, a->p[i].vol, a->p[i].halt);
    clockEnv(&a->nEnvStart, &a->nEnvDiv, &a->nEnvDecay, a->nVol, a->nHalt);
    if (a->triReloadFlag) a->triLinear = a->triLinearReload; else if (a->triLinear) --a->triLinear;
    if (!a->triControl) a->triReloadFlag = 0u;
}
static void clockHalf(NesApu *a)
{
    unsigned i;
    for (i = 0; i < 2u; ++i) {
        NesApuPulse *p = &a->p[i];
        if (!p->halt && p->length) --p->length;
        if (p->sweepDiv == 0u && p->sweepEn && p->sweepShift && p->timerPeriod >= 8u && sweepTarget(p, i) <= 0x7FFu) p->timerPeriod = (uint16_t)sweepTarget(p, i);
        if (p->sweepDiv == 0u || p->sweepReload) { p->sweepDiv = p->sweepPeriod; p->sweepReload = 0u; } else --p->sweepDiv;
    }
    if (!a->triControl && a->triLength) --a->triLength;
    if (!a->nHalt && a->nLength) --a->nLength;
}

static unsigned pulseOut(const NesApuPulse *p, unsigned ch)
{
    if (!p->length || p->timerPeriod < 8u || sweepTarget(p, ch) > 0x7FFu || !kDuty[p->duty][p->seq]) return 0u;
    return p->constVol ? p->vol : p->envDecay;
}

unsigned nesApuRunFrame(NesApu *a, unsigned long cycles, int16_t *out, unsigned cap)
{
    unsigned long c;
    unsigned n = 0u, wi = 0u;
    const double cyclesPerSample = 1789773.0 / (double)NES_APU_RATE;
    for (c = 0; c < cycles; ++c) {
        unsigned i;
        while (wi < a->nw && a->w[wi].cycle <= c) { applyWrite(a, a->w[wi].addr, a->w[wi].val); ++wi; }
        if (a->fcCycle == 0xFFFFFFFFul) { clockQuarter(a); clockHalf(a); a->fcCycle = 0u; }
        ++a->fcCycle;
        if (a->fcCycle == 7457ul || a->fcCycle == 22371ul) clockQuarter(a);
        if (a->fcCycle == 14913ul) { clockQuarter(a); clockHalf(a); }
        if (!a->fcMode && a->fcCycle == 29829ul) { clockQuarter(a); clockHalf(a); a->fcCycle = 0u; }
        if (a->fcMode && a->fcCycle == 37281ul) { clockQuarter(a); clockHalf(a); a->fcCycle = 0u; }
        /* triangle: every CPU cycle */
        if (a->triTimer == 0u) {
            a->triTimer = a->triPeriod;
            if (a->triLength && a->triLinear) a->triSeq = (uint8_t)((a->triSeq + 1u) & 31u);
        } else --a->triTimer;
        a->cpuOdd ^= 1u;
        if (a->cpuOdd) {
            for (i = 0; i < 2u; ++i) {
                NesApuPulse *p = &a->p[i];
                if (p->timer == 0u) { p->timer = p->timerPeriod; p->seq = (uint8_t)((p->seq + 1u) & 7u); } else --p->timer;
            }
            if (a->nTimer == 0u) {
                const unsigned fb = (a->nShift & 1u) ^ ((a->nShift >> (a->nMode ? 6u : 1u)) & 1u);
                a->nTimer = a->nPeriod; a->nShift = (uint16_t)((a->nShift >> 1) | (fb << 14));
            } else --a->nTimer;
        }
        if ((double)(c + 1u) >= (double)(n + 1u) * cyclesPerSample && n < cap) {
            double x;
            {
                const unsigned p1 = pulseOut(&a->p[0], 0u), p2 = pulseOut(&a->p[1], 1u);
                const unsigned tri = kTri[a->triSeq];
                const unsigned noi = (a->nLength && !(a->nShift & 1u)) ? (a->nConst ? a->nVol : a->nEnvDecay) : 0u;
                const double pulse = (p1 + p2) ? 95.88 / (8128.0 / (double)(p1 + p2) + 100.0) : 0.0;
                const double sum = tri / 8227.0 + noi / 12241.0 + a->dmcLevel / 22638.0;
                const double tnd = sum > 0.0 ? 159.79 / (1.0 / sum + 100.0) : 0.0;
                x = (pulse + tnd) * 40000.0;   /* point-sampled once per output sample */
            }
            const double y = x - a->hpIn + 0.995 * a->hpOut;
            a->hpIn = x; a->hpOut = y;
            out[n++] = (int16_t)(y > 32767.0 ? 32767.0 : (y < -32768.0 ? -32768.0 : y));
        }
    }
    /* writes scheduled after this chunk stay queued for the next one */
    {
        unsigned k = 0u;
        while (wi < a->nw) { a->w[k] = a->w[wi]; a->w[k].cycle -= cycles; ++k; ++wi; }
        a->nw = k;
    }
    return n;
}
