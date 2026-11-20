#ifndef EXCITEBIKE_RACE_SPRITES_H
#define EXCITEBIKE_RACE_SPRITES_H

#include <stdint.h>
#include "bike_physics.h"

typedef struct NativeOamState {
    uint8_t bytes[256];
} NativeOamState;

/*
 * Native E17F sprite path: sorted bikes, E1F9 pose selection, D1C7 OAM
 * generation, E1D7 temp overlays and E26A rider overlays. Returns one bit
 * per bike whose pose or auxiliary table is not yet supported.
 */
uint8_t nativeRaceBuildInitialBikeOam(const BikeMovementState *movement,
                                      const BikeStateMachineState *states,
                                      NativeOamState *oam);

#endif
