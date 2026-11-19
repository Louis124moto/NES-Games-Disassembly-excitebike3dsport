#ifndef EXCITEBIKE_BIKE_PHYSICS_H
#define EXCITEBIKE_BIKE_PHYSICS_H

#include <stdint.h>
#include "game_state.h"
#include "input.h"

/* NES speed is a 16-bit fixed-point value split across $0090 (fraction)
 * and $0094 (whole). Keep the bytes explicit to preserve 6502 wraparound. */
typedef struct BikeSpeedState {
    uint8_t fraction;
    uint8_t whole;
} BikeSpeedState;

/*
 * Exact native translation of sub_CE29 for player object X=0.
 * frame_phase is ram_004C, derived by the original game from ram_003F & 3.
 * The player speed routine only updates when frame_phase == 0.
 * acceleration_mode is Y: 0 for the A-button path, 1 for the B-button path
 * as selected by the original sub_CD59 bit/shift logic.
 */
void bikePhysicsAcceleratePlayer(BikeSpeedState *speed,
                                 uint8_t frame_phase,
                                 uint8_t acceleration_mode);

/*
 * Exact native translation of the sub_CE58/sub_CE5C deceleration step for
 * player object X=0. zero_whole_threshold is the already-resolved
 * tbl_C0CC[ram_00B0_obj >> 1] value; the caller's object-state mapping is
 * deliberately kept outside this low-level helper until fully traced.
 */
void bikePhysicsDeceleratePlayer(BikeSpeedState *speed,
                                 uint8_t frame_phase,
                                 uint8_t deceleration_mode,
                                 uint8_t zero_whole_threshold);

/* Full per-frame bike integration is intentionally not exposed until all
 * input, heat, posture, collision and update-order dependencies are traced. */
void bikePhysicsUpdate(BikeState *bike, const InputState *input);

#endif
