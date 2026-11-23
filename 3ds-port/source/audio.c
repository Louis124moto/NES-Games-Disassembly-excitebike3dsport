#include <3ds.h>
#include <stdint.h>
#include <string.h>
#include "audio.h"

/*
 * Native 3DS audio backend.
 *
 * This is a procedural engine/effect synthesizer driven by the source game's
 * semantic sound request bytes. It deliberately does not embed or reproduce
 * the commercial soundtrack. A source-accurate music sequence requires
 * separately licensed/user-supplied music assets and a dedicated native
 * sequence player.
 */
#define AUDIO_RATE 32728.0f
#define AUDIO_SAMPLES 512u
#define AUDIO_CHANNEL 0u

typedef struct AudioBuffer {
    ndspWaveBuf wave;
    int16_t samples[AUDIO_SAMPLES];
} AudioBuffer;

static AudioBuffer s_buffers[2];
static unsigned s_nextBuffer;
static uint32_t s_phase;
static uint32_t s_noise = 0x6D2B79F5u;
static uint16_t s_engineCode;
static uint16_t s_effectTicks;
static uint8_t s_effectType;
static uint8_t s_initialized;

static uint32_t phaseStep(uint32_t frequency)
{
    return (uint32_t)(((uint64_t)frequency << 32) / 32728u);
}

void audioInit(void)
{
    unsigned i;
    if (s_initialized) return;
    memset(s_buffers, 0, sizeof(s_buffers));
    s_nextBuffer = 0u;
    s_phase = 0u;
    s_engineCode = 0u;
    s_effectTicks = 0u;
    s_effectType = 0u;
    s_noise = 0x6D2B79F5u;
    if (R_FAILED(ndspInit())) return;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(AUDIO_CHANNEL, AUDIO_RATE);
    ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_MONO_PCM16);
    {
        float mix[12] = {
            0.18f, 0.18f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
        };
        ndspChnSetMix(AUDIO_CHANNEL, mix);
    }
    for (i = 0u; i < 2u; ++i) {
        s_buffers[i].wave.data_pcm16 = s_buffers[i].samples;
        s_buffers[i].wave.nsamples = AUDIO_SAMPLES;
        s_buffers[i].wave.looping = false;
    }
    s_initialized = 1u;
}

void audioSubmitSourceEvents(uint8_t engine, uint8_t request_fb,
                             uint8_t request_fd, uint8_t request_fe,
                             uint8_t request_ff)
{
    if (!s_initialized) return;
    s_engineCode = engine;
    if (request_fd != 0u) { s_effectType = 2u; s_effectTicks = 18u; }
    else if (request_fe != 0u) { s_effectType = 3u; s_effectTicks = 12u; }
    else if (request_fb != 0u) { s_effectType = 1u; s_effectTicks = 24u; }
    else if (request_ff != 0u) { s_effectType = 4u; s_effectTicks = 8u; }
}

static int16_t renderSample(void)
{
    uint32_t frequency = 0u;
    int32_t sample = 0;
    if (s_engineCode != 0u) {
        /* $00FC is the source engine-sound intensity/pitch request. */
        frequency = 58u + (uint32_t)(s_engineCode & 0x3Fu) * 7u;
        s_phase += phaseStep(frequency);
        sample = (s_phase & 0x80000000u) ? 3600 : -3600;
    }
    if (s_effectTicks != 0u) {
        int32_t effect = 0;
        --s_effectTicks;
        switch (s_effectType) {
        case 1u: /* countdown/start cue */
            effect = (s_phase & 0x40000000u) ? 4200 : -4200;
            break;
        case 2u: /* impact/collision */
            s_noise ^= s_noise << 13;
            s_noise ^= s_noise >> 17;
            s_noise ^= s_noise << 5;
            effect = (int32_t)(s_noise & 0x1FFFu) - 4096;
            break;
        case 3u: /* crash/alert */
            effect = (s_phase & 0x20000000u) ? 5200 : -5200;
            break;
        default: /* short opponent/event tick */
            effect = (s_effectTicks & 2u) ? 1800 : -1800;
            break;
        }
        sample += (effect * (int32_t)s_effectTicks) / 24;
    }
    if (sample > 12000) sample = 12000;
    if (sample < -12000) sample = -12000;
    return (int16_t)sample;
}

void audioUpdate(void)
{
    AudioBuffer *buffer;
    unsigned i;
    if (!s_initialized) return;
    buffer = &s_buffers[s_nextBuffer];
    if (buffer->wave.status != NDSP_WBUF_FREE &&
        buffer->wave.status != NDSP_WBUF_DONE) return;
    for (i = 0u; i < AUDIO_SAMPLES; ++i)
        buffer->samples[i] = renderSample();
    DSP_FlushDataCache(buffer->samples, sizeof(buffer->samples));
    buffer->wave.data_pcm16 = buffer->samples;
    buffer->wave.nsamples = AUDIO_SAMPLES;
    buffer->wave.looping = false;
    ndspChnWaveBufAdd(AUDIO_CHANNEL, &buffer->wave);
    s_nextBuffer ^= 1u;
}

void audioExit(void)
{
    if (!s_initialized) return;
    ndspChnReset(AUDIO_CHANNEL);
    ndspExit();
    memset(s_buffers, 0, sizeof(s_buffers));
    s_initialized = 0u;
}
