#include <3ds.h>
#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "sound_driver.h"
#include "sound_synth.h"

/*
 * Native 3DS audio backend.
 *
 * The source sound driver (sound_driver.c, a 1:1 port of sub_F844, verified
 * differentially against the original code) runs once per source frame and
 * logs its APU register writes. sound_synth.c turns each frame's log into PCM
 * and this file streams it through NDSP. Production is paced by the DSP queue
 * so the music keeps the source tempo (60.0988 Hz) whatever the display rate.
 */
#define AUDIO_CHANNEL 0u
#define AUDIO_BUFFERS 4u
#define AUDIO_MAX_QUEUED 3u

typedef struct AudioBuffer {
    ndspWaveBuf wave;
    int16_t samples[SOUND_SYNTH_MAX_FRAME_SAMPLES];
} AudioBuffer;

static AudioBuffer s_buffers[AUDIO_BUFFERS];
static unsigned s_nextBuffer;
static SoundDriver s_driver;
static SoundSynth s_synth;
static uint8_t s_pendingFb, s_pendingFd, s_pendingFe, s_pendingFf, s_engineFc;
static uint8_t s_initialized;
static uint8_t s_apuEnable = 0x0Fu;      /* last value forwarded to the synth */
static uint8_t s_apuEnablePending;
static uint8_t s_apuEnableValue;

void audioInit(void)
{
    unsigned i;
    if (s_initialized) return;
    memset(s_buffers, 0, sizeof(s_buffers));
    s_nextBuffer = 0u;
    s_pendingFb = s_pendingFd = s_pendingFe = s_pendingFf = s_engineFc = 0u;
    s_apuEnable = 0x0Fu; s_apuEnablePending = 0u;
    soundDriverReset(&s_driver, kSoundData);
    soundSynthReset(&s_synth);
    if (R_FAILED(ndspInit())) return;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(AUDIO_CHANNEL, (float)SOUND_SYNTH_RATE);
    ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_MONO_PCM16);
    {
        float mix[12] = {
            0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
        };
        ndspChnSetMix(AUDIO_CHANNEL, mix);
    }
    for (i = 0u; i < AUDIO_BUFFERS; ++i)
        s_buffers[i].wave.data_pcm16 = s_buffers[i].samples;
    s_initialized = 1u;
}

/* Latches the source request bytes ($FC engine level, $FB music, $FD noise,
 * $FE overheat flag, $FF effects); the driver consumes them on its next frame. */
void audioSubmitSourceEvents(uint8_t engine, uint8_t request_fb,
                             uint8_t request_fd, uint8_t request_fe,
                             uint8_t request_ff)
{
    if (!s_initialized) return;
    s_engineFc = engine;
    s_pendingFb |= request_fb;
    s_pendingFd |= request_fd;
    s_pendingFe |= request_fe;
    s_pendingFf |= request_ff;
}

void audioSubmitApuEnable(uint8_t value)
{
    if (!s_initialized || value == s_apuEnable) return;
    s_apuEnable = value;
    s_apuEnableValue = value;
    s_apuEnablePending = 1u;
}

static unsigned busyBuffers(void)
{
    unsigned i, busy = 0u;
    for (i = 0u; i < AUDIO_BUFFERS; ++i)
        if (s_buffers[i].wave.status != NDSP_WBUF_FREE &&
            s_buffers[i].wave.status != NDSP_WBUF_DONE) ++busy;
    return busy;
}

void audioUpdate(void)
{
    if (!s_initialized) return;
    while (busyBuffers() < AUDIO_MAX_QUEUED) {
        AudioBuffer *buffer = &s_buffers[s_nextBuffer];
        unsigned count;
        if (buffer->wave.status != NDSP_WBUF_FREE && buffer->wave.status != NDSP_WBUF_DONE) break;
        soundDriverSetRequests(&s_driver, s_pendingFb, s_engineFc, s_pendingFd,
                               s_pendingFe, s_pendingFf);
        s_pendingFb = s_pendingFd = s_pendingFe = s_pendingFf = 0u;
        soundDriverFrame(&s_driver);
        if (s_apuEnablePending) {          /* the main loop's $4015 write precedes the NMI driver */
            SoundApuWrite log[SOUND_MAX_WRITES + 1u];
            unsigned n = s_driver.write_count;
            log[0].reg = 0x4015u; log[0].value = s_apuEnableValue;
            memcpy(&log[1], s_driver.writes, n * sizeof(log[0]));
            s_apuEnablePending = 0u;
            count = soundSynthFrame(&s_synth, log, n + 1u, buffer->samples);
        } else {
            count = soundSynthFrame(&s_synth, s_driver.writes, s_driver.write_count, buffer->samples);
        }
        DSP_FlushDataCache(buffer->samples, count * sizeof(int16_t));
        buffer->wave.data_pcm16 = buffer->samples;
        buffer->wave.nsamples = count;
        buffer->wave.looping = false;
        ndspChnWaveBufAdd(AUDIO_CHANNEL, &buffer->wave);
        s_nextBuffer = (s_nextBuffer + 1u) % AUDIO_BUFFERS;
    }
}

void audioExit(void)
{
    if (!s_initialized) return;
    ndspChnReset(AUDIO_CHANNEL);
    ndspExit();
    memset(s_buffers, 0, sizeof(s_buffers));
    s_initialized = 0u;
}
