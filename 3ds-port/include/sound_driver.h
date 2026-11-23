#ifndef EXCITEBIKE_SOUND_DRIVER_H
#define EXCITEBIKE_SOUND_DRIVER_H

#include <stddef.h>
#include <stdint.h>

/*
 * Native port of the source sound driver: sub_F844 and everything it calls
 * (F8D4 effects, FA74 engine, FC3D noise, FC8D music, FF53 note decode).
 *
 * The driver is exact state-machine code. Instead of poking APU registers it
 * appends (register, value) pairs to a per-frame log; the synthesizer
 * (sound_synth.c) turns that log into audio. The driver runs once per frame,
 * right after the controller read, exactly where the NMI calls sub_F844.
 *
 * Tables and music sequences come from kSoundData (CPU $F9A2-$FFFF of the
 * source PRG, generated from the disassembly like the other source tables).
 */
#define SOUND_DATA_BASE 0xF9A2u
#define SOUND_DATA_END 0x10000u
#define SOUND_DATA_SIZE (SOUND_DATA_END - SOUND_DATA_BASE)
#define SOUND_MAX_WRITES 96u

typedef struct SoundApuWrite {
    uint16_t reg;   /* $4000-$4017 */
    uint8_t value;
} SoundApuWrite;

typedef struct SoundDriver {
    uint8_t zp[0x100];    /* zero page: $F0-$FA driver state, $FB-$FF requests, $00/$01 scratch */
    uint8_t high[0x20];   /* $07E0-$07FF driver state */
    const uint8_t *data;  /* SOUND_DATA_SIZE bytes, or NULL (driver stays silent) */
    SoundApuWrite writes[SOUND_MAX_WRITES];
    uint16_t write_count;
} SoundDriver;

/* Source tables and music sequences (CPU $F9A2-$FFFF), see tools/gen_sound_data.py. */
extern const uint8_t kSoundData[SOUND_DATA_SIZE];

void soundDriverReset(SoundDriver *driver, const uint8_t *data);

/* Request bytes, consumed like the source: $FB music, $FC engine (kept),
 * $FD noise, $FE overheat flag, $FF effects. */
void soundDriverSetRequests(SoundDriver *driver, uint8_t music_fb,
                            uint8_t engine_fc, uint8_t noise_fd,
                            uint8_t flag_fe, uint8_t effect_ff);

/* One sub_F844 call with X = 2 (value left by sub_D347). Clears the
 * per-frame write log first. */
void soundDriverFrame(SoundDriver *driver);

#endif
