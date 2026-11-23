#ifndef EXCITEBIKE_SOUND_SYNTH_H
#define EXCITEBIKE_SOUND_SYNTH_H

#include <stdint.h>
#include "sound_driver.h"

/*
 * Native voice synthesizer for the source sound hardware model: two pulse
 * voices, triangle, noise and the $4011 DAC, driven by the register log the
 * sound driver produces each frame. It does not execute any CPU/PPU code.
 * Pure C (no 3DS API) so it is host-testable.
 */
#define SOUND_SYNTH_RATE 32000u
#define SOUND_SYNTH_MAX_FRAME_SAMPLES 640u

typedef struct SoundPulse {
    uint8_t duty, halt, constant, volume, enabled;
    uint8_t env_start, env_divider, env_decay;
    uint8_t sweep_enable, sweep_period, sweep_negate, sweep_shift, sweep_reload, sweep_divider;
    uint16_t timer, counter;
    uint8_t length, step;
} SoundPulse;

typedef struct SoundSynth {
    SoundPulse pulse[2];
    uint8_t tri_control, tri_reload_value, tri_reload_flag, tri_linear, tri_length, tri_step, tri_enabled;
    uint16_t tri_timer, tri_counter;
    uint8_t noise_halt, noise_constant, noise_volume, noise_mode, noise_enabled;
    uint8_t noise_env_start, noise_env_divider, noise_env_decay, noise_length;
    uint16_t noise_period, noise_counter, noise_lfsr;
    uint8_t dac;
    int32_t hp_in, hp_out; /* DC-blocking high-pass (cutoff ~20 Hz) */
    uint32_t sample_remainder; /* fractional samples carried between frames */
} SoundSynth;

void soundSynthReset(SoundSynth *synth);
void soundSynthWrite(SoundSynth *synth, uint16_t reg, uint8_t value);
/* Applies one frame's driver writes (frame start), then renders one video
 * frame of mono PCM16 at SOUND_SYNTH_RATE. Returns the sample count (<= max). */
unsigned soundSynthFrame(SoundSynth *synth, const SoundApuWrite *writes,
                         unsigned count, int16_t *out);

#endif
