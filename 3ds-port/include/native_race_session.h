#ifndef EXCITEBIKE_NATIVE_RACE_SESSION_H
#define EXCITEBIKE_NATIVE_RACE_SESSION_H

#include <stdint.h>
#include "native_race_setup.h"
#include "race_countdown.h"
#include "race_vertical.h"
#include "track.h"

typedef struct NativeRaceSession {
    NativeRngState rng;
    NativeRaceSetupState setup;
    BikeMovementState movement;
    BikeStateMachineState states;
    BikeRaceCountdownState countdown;
    NativeTrackState track;
    BikeRaceCountdownRenderEvent countdown_render_event;
    uint8_t vertical_unsupported_mask;
    uint8_t initialized;
} NativeRaceSession;

/* Compose the native equivalents of C869, C820, C875 and D0C6 for course 0. */
int nativeRaceSessionInitFirstCourse(NativeRaceSession *session,
                                     const NativeRaceSetupInput *input,
                                     const NativeRngState *rng);

/* Separate source-order slices; caller controls the exact frame-call order. */
BikeRaceCountdownRenderEvent nativeRaceSessionUpdateCountdown(NativeRaceSession *session);
uint8_t nativeRaceSessionUpdateVertical(NativeRaceSession *session);

#endif
