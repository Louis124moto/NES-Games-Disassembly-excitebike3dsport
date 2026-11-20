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
    BikeRaceCountdownRenderEvent countdown_render_event;
    uint8_t vertical_unsupported_mask;
    uint8_t sprite_unsupported_mask;
    uint8_t initialized;
} NativeRaceSession;

/* Compose the native equivalents of C869, C820, C875 and D0C6 for course 0. */
int nativeRaceSessionInitFirstCourse(NativeRaceSession *session,
                                     const NativeRaceSetupInput *input,
                                     const NativeRngState *rng);

/* D918 then DD8D are separate, source-ordered steps. */
void nativeRaceSessionUpdateBikeStates(NativeRaceSession *session);
BikeRaceCountdownRenderEvent nativeRaceSessionUpdateCountdown(NativeRaceSession *session);
uint8_t nativeRaceSessionUpdateVertical(NativeRaceSession *session);
/* E836 + E17F/E456 native sprite preparation, after the caller's frame steps. */
uint8_t nativeRaceSessionUpdateSprites(NativeRaceSession *session);

#endif
