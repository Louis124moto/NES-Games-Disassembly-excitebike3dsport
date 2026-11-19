#ifndef EXCITEBIKE_BIKE_PHYSICS_H
#define EXCITEBIKE_BIKE_PHYSICS_H

#include <stdint.h>
#include "game_state.h"
typedef struct InputState InputState;

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
 * acceleration_mode is Y: 0 for controller bit 7, 1 for bit 6 alone,
 * or 2 for the post-finish speed limit path selected by sub_CD59.
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

/*
 * Native state for the player-only source gate sub_CDEE. The fields map
 * directly to ram_00C0_obj, ram_00A4_obj, ram_0070_obj, ram_obj_pos_Y_lo,
 * ram_036C_obj, and ram_00FD. Return value is the 6502 Z flag after RTS:
 * nonzero means the original caller may continue its acceleration path.
 */
typedef struct BikeTerrainGateState {
    uint8_t object_class;
    uint8_t surface;
    uint8_t surface_state;
    uint8_t vertical_position;
    uint8_t posture_state;
    uint8_t player_event_flag;
} BikeTerrainGateState;

uint8_t bikePhysicsTerrainGate(BikeTerrainGateState *state);

/*
 * Source-derived inputs for the player speed-control branch in sub_CD59.
 * These are gameplay fields, not a 6502 RAM image. Terrain state is updated
 * in place by the native sub_CDEE translation when throttle is applied.
 */
typedef struct BikePlayerSpeedContext {
    BikeSpeedState speed;
    BikeTerrainGateState terrain;
    uint8_t controller;
    uint8_t frame_phase;
    uint8_t race_finished;
    uint8_t object_lock_0374;
    uint8_t collision_lock_0098;
    uint8_t movement_state_009C;
    uint8_t object_status_00B0;
} BikePlayerSpeedContext;

void bikePhysicsUpdatePlayerSpeed(BikePlayerSpeedContext *player);

/* Full per-frame bike integration is intentionally not exposed until all
 * input, heat, posture, collision and update-order dependencies are traced. */
void bikePhysicsUpdate(BikeState *bike, const InputState *input);

#endif
