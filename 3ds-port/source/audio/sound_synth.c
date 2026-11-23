#include "sound_synth.h"

#include <string.h>

#define CPU_HZ 1789773u
#define FRAME_CYCLES 29781u
#define CHUNK_CYCLES 8u

static const uint8_t kLength[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};
static const uint8_t kDuty[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 }, { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 }, { 1, 0, 0, 1, 1, 1, 1, 1 }
};
static const uint16_t kNoisePeriod[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static int mixSample(const SoundSynth *s);

void soundSynthReset(SoundSynth *s)
{
    memset(s, 0, sizeof(*s));
    s->noise_lfsr = 1u;
    s->noise_period = kNoisePeriod[0];
    /* Source init (C2E3) enables pulse1/pulse2/triangle/noise. */
    s->pulse[0].enabled = s->pulse[1].enabled = 1u;
    s->tri_enabled = s->noise_enabled = 1u;
    s->hp_in = 0;
    s->hp_out = 0;
    s->hp_in = (int32_t)mixSample(s);   /* start settled on the idle DC level */
}

static void clockEnvelope(uint8_t *start, uint8_t *divider, uint8_t *decay,
                          uint8_t period, uint8_t loop)
{
    if (*start) {
        *start = 0u;
        *decay = 15u;
        *divider = period;
    } else if (*divider == 0u) {
        *divider = period;
        if (*decay != 0u) (*decay)--;
        else if (loop) *decay = 15u;
    } else {
        (*divider)--;
    }
}

static unsigned sweepTarget(const SoundPulse *p, unsigned index)
{
    const unsigned delta = (unsigned)(p->timer >> p->sweep_shift);
    if (p->sweep_negate)
        return (unsigned)p->timer - delta - (index == 0u ? 1u : 0u);
    return (unsigned)p->timer + delta;
}

static void clockQuarter(SoundSynth *s)
{
    unsigned i;
    for (i = 0u; i < 2u; ++i)
        clockEnvelope(&s->pulse[i].env_start, &s->pulse[i].env_divider,
                      &s->pulse[i].env_decay, s->pulse[i].volume, s->pulse[i].halt);
    clockEnvelope(&s->noise_env_start, &s->noise_env_divider, &s->noise_env_decay,
                  s->noise_volume, s->noise_halt);
    if (s->tri_reload_flag) s->tri_linear = s->tri_reload_value;
    else if (s->tri_linear != 0u) s->tri_linear--;
    if (!s->tri_control) s->tri_reload_flag = 0u;
}

static void clockHalf(SoundSynth *s)
{
    unsigned i;
    for (i = 0u; i < 2u; ++i) {
        SoundPulse *p = &s->pulse[i];
        if (!p->halt && p->length != 0u) p->length--;
        if (p->sweep_divider == 0u && p->sweep_enable && p->sweep_shift != 0u) {
            const unsigned target = sweepTarget(p, i);
            if (p->timer >= 8u && target <= 0x7FFu) p->timer = (uint16_t)target;
        }
        if (p->sweep_divider == 0u || p->sweep_reload) {
            p->sweep_divider = p->sweep_period;
            p->sweep_reload = 0u;
        } else {
            p->sweep_divider--;
        }
    }
    if (!s->tri_control && s->tri_length != 0u) s->tri_length--;
    if (!s->noise_halt && s->noise_length != 0u) s->noise_length--;
}

void soundSynthWrite(SoundSynth *s, uint16_t reg, uint8_t v)
{
    if (reg >= 0x4000u && reg <= 0x4007u) {
        SoundPulse *p = &s->pulse[(reg >> 2) & 1u];
        switch (reg & 3u) {
        case 0u:
            p->duty = (uint8_t)(v >> 6); p->halt = (uint8_t)((v >> 5) & 1u);
            p->constant = (uint8_t)((v >> 4) & 1u); p->volume = (uint8_t)(v & 15u);
            break;
        case 1u:
            p->sweep_enable = (uint8_t)(v >> 7); p->sweep_period = (uint8_t)((v >> 4) & 7u);
            p->sweep_negate = (uint8_t)((v >> 3) & 1u); p->sweep_shift = (uint8_t)(v & 7u);
            p->sweep_reload = 1u;
            break;
        case 2u:
            p->timer = (uint16_t)((p->timer & 0x700u) | v);
            break;
        default:
            p->timer = (uint16_t)((p->timer & 0xFFu) | ((unsigned)(v & 7u) << 8));
            if (p->enabled) p->length = kLength[v >> 3];
            p->step = 0u; p->env_start = 1u;
            break;
        }
    } else if (reg == 0x4008u) {
        s->tri_control = (uint8_t)(v >> 7); s->tri_reload_value = (uint8_t)(v & 0x7Fu);
    } else if (reg == 0x400Au) {
        s->tri_timer = (uint16_t)((s->tri_timer & 0x700u) | v);
    } else if (reg == 0x400Bu) {
        s->tri_timer = (uint16_t)((s->tri_timer & 0xFFu) | ((unsigned)(v & 7u) << 8));
        if (s->tri_enabled) s->tri_length = kLength[v >> 3];
        s->tri_reload_flag = 1u;
    } else if (reg == 0x400Cu) {
        s->noise_halt = (uint8_t)((v >> 5) & 1u); s->noise_constant = (uint8_t)((v >> 4) & 1u);
        s->noise_volume = (uint8_t)(v & 15u);
    } else if (reg == 0x400Eu) {
        s->noise_mode = (uint8_t)(v >> 7); s->noise_period = kNoisePeriod[v & 15u];
    } else if (reg == 0x400Fu) {
        if (s->noise_enabled) s->noise_length = kLength[v >> 3];
        s->noise_env_start = 1u;
    } else if (reg == 0x4011u) {
        s->dac = (uint8_t)(v & 0x7Fu);
    } else if (reg == 0x4015u) {
        s->pulse[0].enabled = (uint8_t)(v & 1u); s->pulse[1].enabled = (uint8_t)((v >> 1) & 1u);
        s->tri_enabled = (uint8_t)((v >> 2) & 1u); s->noise_enabled = (uint8_t)((v >> 3) & 1u);
        if (!s->pulse[0].enabled) s->pulse[0].length = 0u;
        if (!s->pulse[1].enabled) s->pulse[1].length = 0u;
        if (!s->tri_enabled) s->tri_length = 0u;
        if (!s->noise_enabled) s->noise_length = 0u;
    } else if (reg == 0x4017u) {
        if (v & 0x80u) { clockQuarter(s); clockHalf(s); }  /* 5-step mode clocks at once */
    }
}

static unsigned pulseOut(const SoundPulse *p, unsigned index)
{
    if (!p->enabled || p->length == 0u || p->timer < 8u) return 0u;
    if (sweepTarget(p, index) > 0x7FFu) return 0u;
    if (!kDuty[p->duty][p->step]) return 0u;
    return p->constant ? p->volume : p->env_decay;
}

static void advance(SoundSynth *s, unsigned cycles)
{
    unsigned i;
    for (i = 0u; i < 2u; ++i) {
        SoundPulse *p = &s->pulse[i];
        const unsigned period = ((unsigned)p->timer + 1u) * 2u;
        unsigned left = cycles;
        while (left >= (unsigned)(period - p->counter)) {
            left -= (unsigned)(period - p->counter);
            p->counter = 0u;
            p->step = (uint8_t)((p->step + 1u) & 7u);
        }
        p->counter = (uint16_t)(p->counter + left);
    }
    {
        const unsigned period = (unsigned)s->tri_timer + 1u;
        unsigned left = cycles;
        while (left >= (unsigned)(period - s->tri_counter)) {
            left -= (unsigned)(period - s->tri_counter);
            s->tri_counter = 0u;
            if (s->tri_length != 0u && s->tri_linear != 0u)
                s->tri_step = (uint8_t)((s->tri_step + 1u) & 31u);
        }
        s->tri_counter = (uint16_t)(s->tri_counter + left);
    }
    {
        unsigned left = cycles;
        while (left >= (unsigned)(s->noise_period - s->noise_counter)) {
            unsigned feedback;
            left -= (unsigned)(s->noise_period - s->noise_counter);
            s->noise_counter = 0u;
            feedback = (unsigned)((s->noise_lfsr & 1u) ^ ((s->noise_lfsr >> (s->noise_mode ? 6 : 1)) & 1u));
            s->noise_lfsr = (uint16_t)((s->noise_lfsr >> 1) | (feedback << 14));
        }
        s->noise_counter = (uint16_t)(s->noise_counter + left);
    }
}

static int mixSample(const SoundSynth *s)
{
    const unsigned p1 = pulseOut(&s->pulse[0], 0u), p2 = pulseOut(&s->pulse[1], 1u);
    const unsigned tri = (s->tri_step < 16u) ? (15u - s->tri_step) : (s->tri_step - 16u);
    unsigned noise = 0u;
    float pulse = 0.0f, tnd = 0.0f;
    if (s->noise_enabled && s->noise_length != 0u && !(s->noise_lfsr & 1u))
        noise = s->noise_constant ? s->noise_volume : s->noise_env_decay;
    if (p1 + p2 != 0u) pulse = 95.88f / (8128.0f / (float)(p1 + p2) + 100.0f);
    {
        const float d = (float)tri / 8227.0f + (float)noise / 12241.0f + (float)s->dac / 22638.0f;
        if (d > 0.0f) tnd = 159.79f / (1.0f / d + 100.0f);
    }
    return (int)((pulse + tnd) * 26000.0f);
}

unsigned soundSynthFrame(SoundSynth *s, const SoundApuWrite *writes, unsigned count, int16_t *out)
{
    /* Exact frame length: 32000 / (1789773 / 29780.5) samples, carried as a remainder. */
    const uint32_t total = s->sample_remainder + SOUND_SYNTH_RATE * FRAME_CYCLES;
    unsigned samples = (unsigned)(total / CPU_HZ), n, i;
    unsigned chunk = 0u, cycleAt = 0u, nextEvent = 0u;
    static const unsigned kEvents[3] = { 7457u, 14913u, 22371u };

    s->sample_remainder = total % CPU_HZ;
    if (samples > SOUND_SYNTH_MAX_FRAME_SAMPLES) samples = SOUND_SYNTH_MAX_FRAME_SAMPLES;
    for (i = 0u; i < count; ++i)
        soundSynthWrite(s, writes[i].reg, writes[i].value);
    for (n = 0u; n < samples; ++n) {
        int acc = 0;
        const unsigned target = (unsigned)(((uint64_t)(n + 1u) * FRAME_CYCLES) / samples);
        unsigned slices = 0u;
        while (cycleAt < target) {
            unsigned step = target - cycleAt;
            if (step > CHUNK_CYCLES) step = CHUNK_CYCLES;
            if (nextEvent < 3u && cycleAt + step > kEvents[nextEvent]) step = kEvents[nextEvent] - cycleAt;
            if (step == 0u) step = 1u;
            advance(s, step);
            cycleAt += step;
            while (nextEvent < 3u && cycleAt >= kEvents[nextEvent]) {
                clockQuarter(s);
                if (nextEvent == 1u) clockHalf(s);
                nextEvent++;
            }
            acc += mixSample(s);
            slices++;
        }
        (void)chunk;
        {
            const int32_t x = (int32_t)(slices != 0u ? acc / (int)slices : mixSample(s));
            int32_t y = x - s->hp_in + s->hp_out - (s->hp_out >> 8);
            s->hp_in = x;
            s->hp_out = y;
            if (y > 32767) y = 32767;
            if (y < -32768) y = -32768;
            out[n] = (int16_t)y;
        }
    }
    return samples;
}
