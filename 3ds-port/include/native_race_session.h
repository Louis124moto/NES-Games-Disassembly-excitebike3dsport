#ifndef EXCITEBIKE_NATIVE_RACE_SESSION_H
#define EXCITEBIKE_NATIVE_RACE_SESSION_H

#include <stdint.h>
#include "native_race_setup.h"
#include "race_countdown.h"
#include "race_vertical.h"
#include "race_sprites.h"
#include "track.h"

typedef struct NativeRaceSession {
    NativeRngState rng;
    NativeRaceSetupState setup;
    BikeMovementState movement;
    BikeStateMachineState states;
    BikeRaceCountdownState countdown;
    NativeTrackState track;
    NativeOamState oam;
    uint8_t course_column_buffer[0x12]; /* F4FF -> PPU column command */
    uint8_t result_record_glyphs[7];    /* C5A7: $03D1-$03D7, PPU $21F2 */
    uint8_t result_current_glyphs[7];   /* CB77: $03D9-$03DF, PPU $2232 */
    uint8_t result_active;
    uint8_t result_podium_active;
    uint32_t course_column_serial;      /* increments for each decoded column */
    BikeRaceCountdownRenderEvent countdown_render_event;
    uint8_t vertical_unsupported_mask;
    uint8_t sprite_unsupported_mask;
    uint8_t course;                     /* ram_0043: 0..4 */
    uint8_t flag_004B;
    uint8_t counter_03F3;
    uint8_t counter_03F8;
    uint8_t initialized;
} NativeRaceSession;

/* Compose the native equivalents of C869, C820, C875 and D0C6 for the selected course (0..4). */
int nativeRaceSessionInit(NativeRaceSession *session,
                                     const NativeRaceSetupInput *input,
                                     const NativeRngState *rng);

/* D918 then DD8D are separate, source-ordered steps. */
void nativeRaceSessionUpdateBikeStates(NativeRaceSession *session);
BikeRaceCountdownRenderEvent nativeRaceSessionUpdateCountdown(NativeRaceSession *session);
uint8_t nativeRaceSessionUpdateVertical(NativeRaceSession *session);
/* E836 + E17F/E456 native sprite preparation, after the caller's frame steps. */
uint8_t nativeRaceSessionUpdateSprites(NativeRaceSession *session);

#endif
