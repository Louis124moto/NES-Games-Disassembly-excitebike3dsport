#ifndef EXCITEBIKE_BIKE_PHYSICS_H
#define EXCITEBIKE_BIKE_PHYSICS_H

#include <stdint.h>

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
 * in place by the native sub_CDEE translation when throttle is applied. The
 * later steering/timer side effects at loc_CDBD are a separate subsystem.
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

/* Player temperature/overheat meter state from ram_03B5/$03B6,
 * ram_03E0, ram_003C and ram_00FC. */
typedef struct BikeTemperatureState {
    uint8_t fraction;
    uint8_t whole;
    uint8_t overheat_state;
    uint8_t overheat_timer;
    uint8_t sound_flags;
} BikeTemperatureState;

/* Native gameplay-state translation of sub_E359; returns the source sprite
 * tile selector ($FE normal / blink-off, $FC blink-on). Sprite/OAM writes
 * remain the renderer's responsibility. */
uint8_t bikePhysicsUpdateTemperature(BikeTemperatureState *temperature,
                                     uint8_t controller,
                                     uint8_t race_started,
                                     uint8_t race_finished);

/* Native fixed-size state for the original per-frame movement routines. */
typedef struct BikeMovementState {
    uint8_t speed_whole[4];          /* ram_0094_obj */
    uint8_t speed_fraction[4];       /* ram_0090_obj */
    uint8_t fraction_accumulator[4]; /* ram_0394_obj */
    uint8_t movement_delta[4];       /* ram_0060_obj */
    uint8_t object_active[4];        /* ram_00A8_obj */
    uint8_t x_low[4];                /* ram_0080_obj */
    uint8_t x_high[4];               /* ram_0084_obj */
    uint8_t previous_x[4];           /* ram_0390_obj */
    uint8_t player_scroll_x;         /* ram_0012_scroll_X */
    uint8_t base_nametable;          /* ram_004D_base_nametable */
} BikeMovementState;

void bikePhysicsBuildMovementDeltas(BikeMovementState *movement);
void bikePhysicsAdvancePlayerScroll(BikeMovementState *movement);
void bikePhysicsAdvanceOpponentPositions(BikeMovementState *movement);

#endif
