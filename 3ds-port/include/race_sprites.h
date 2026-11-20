#ifndef EXCITEBIKE_RACE_SPRITES_H
#define EXCITEBIKE_RACE_SPRITES_H

#include <stdint.h>
#include "bike_physics.h"

typedef struct NativeOamState {
    uint8_t bytes[256];
} NativeOamState;

/*
 * First-course initial pose only: translates E17F/E3F4/E1A3/E1F9/D1C7.
 * Returns one bit per bike whose current state is outside this verified pose.
 */
uint8_t nativeRaceBuildInitialBikeOam(const BikeMovementState *movement,
                                      const BikeStateMachineState *states,
                                      NativeOamState *oam);

#endif
