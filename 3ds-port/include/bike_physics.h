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
uint8_t bikePhysicsTerrainGateForObject(BikeTerrainGateState *state, uint8_t object_index);

/*
 * Source-derived inputs for the player speed-control branch in sub_CD59.
 * These are gameplay fields, not a 6502 RAM image. Terrain state is updated
 * in place by the native sub_CDEE translation when throttle is applied. The
 * later steering/timer side effects at loc_CDBD are a separate subsystem.
 */
typedef struct BikePlayerSpeedContext {
    uint8_t object_index; /* X in sub_CD59: player 0, opponents 1..3 */
    BikeSpeedState speed;
    BikeTerrainGateState terrain;
    uint8_t controller;
    uint8_t frame_phase;
    uint8_t race_finished;
    uint8_t object_lock_0374;
    uint8_t collision_lock_0098;
    uint8_t overheat_lock_03E0;
    uint8_t movement_state_009C;
    uint8_t object_status_00B0;
    /* State consumed by the steering/approach tail (loc_CDBD..). */
    uint8_t inactive_0058;
    uint8_t approach_00AC;
    uint8_t animation_state_0368;
    uint8_t movement_mode_0388;
    uint8_t steer_timer_0026;
    uint8_t approach_timer_002A;
} BikePlayerSpeedContext;

typedef BikePlayerSpeedContext BikeObjectSpeedContext;

/* Native translation of sub_CD59 for any bike slot: speed control, then the
 * steering/approach tail (loc_CDBD..). `controller` is ram_005C and
 * `race_finished` is ram_0052, which the tail also reads. */
void bikePhysicsUpdateObjectSpeed(BikeObjectSpeedContext *object);
/* Compatibility entry for the player slot (object_index is forced to zero). */
void bikePhysicsUpdatePlayerSpeed(BikePlayerSpeedContext *player);

/*
 * Per-object state consumed by the steering/approach tail of sub_CD59
 * (loc_CDBD-$CDEB), the leaf loc_CE83-$CECF and the shared loc_DCC7 entry.
 * Fields map to the source object arrays at index X. The tail only reads
 * ram_00B0_obj >> 1 for the tbl_C0C8..tbl_C0D5 lookups; the verified domain
 * is object_status_00B0 in 0..3 (table index 0..1), the values the speed
 * pass already restricts its tests to.
 */
typedef struct BikeSteeringState {
    uint8_t collision_state_0098;
    uint8_t object_status_00B0;
    uint8_t inactive_0058;
    uint8_t track_finished_0052;
    uint8_t speed_fraction_0090;
    uint8_t speed_whole_0094;
    uint8_t movement_flags_005C;
    uint8_t approach_00AC;
    uint8_t animation_state_0368;
    uint8_t movement_mode_0388;
    uint8_t steer_timer_0026;
    uint8_t approach_timer_002A;
} BikeSteeringState;

/* Native translation of loc_CDBD-$CDEB including loc_CE83 and loc_DCC7. */
void bikePhysicsSteeringTail(BikeSteeringState *state);

/* Four-bike speed-control pass from sub_CD1F. This models the active-object
 * loop and the speed-control prefix of sub_CD59; other per-frame systems
 * (steering, posture animation, collision, etc.) remain separate. */
typedef struct BikeRaceSpeedState {
    BikeObjectSpeedContext bikes[4];
    uint8_t active[4];
    uint8_t frame_phase;
    uint8_t race_started;
    uint8_t race_finished;
} BikeRaceSpeedState;

void bikePhysicsUpdateRaceSpeeds(BikeRaceSpeedState *race);


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
    uint8_t object_action_state[4];  /* ram_0058_obj */
    uint8_t vertical_phase[4];       /* ram_00A4_obj, E96C phase gate */
    uint8_t object_class[4];         /* ram_00E0_obj, assigned by C875 */
    uint8_t object_origin_x[4];      /* ram_0064, source start coordinate for E733 events */
    uint8_t setup_parameter[4];      /* ram_0074, initialized from $0019,X >> 1 */
    uint8_t x_low[4];                /* ram_0080_obj */
    uint8_t x_high[4];               /* ram_0084_obj */
    uint8_t previous_x[4];           /* ram_0390_obj */
    uint8_t player_scroll_x;         /* ram_0012_scroll_X */
    uint8_t base_nametable;          /* ram_004D_base_nametable */
    uint8_t secondary_scroll_fraction; /* ram_03BF */
    uint8_t secondary_scroll_x;         /* ram_0050 */
    uint8_t secondary_base_nametable;   /* ram_004E */
    uint8_t object_lock_timer[4];       /* ram_0374_obj */
    uint8_t start_lock_timer[4];        /* ram_0078, initialized by C875 */
    uint8_t movement_flags[4];          /* ram_005C_obj */
    /* State consumed by sub_DC1A/sub_DCF2 and their DD38/DD6F dependencies. */
    uint8_t object_status[4];            /* ram_00B0_obj */
    uint8_t vertical_reference[4];       /* ram_00BC_obj */
    uint8_t object_y[4];                 /* ram_obj_pos_Y_lo */
    uint8_t y_low[4];                      /* ram_008C */
    uint8_t rider_state[4];              /* ram_0364_obj */
    uint8_t collision_state[4];           /* ram_0098_obj */
    uint8_t movement_mode[4];              /* ram_0388_obj */
    uint8_t brake_state[4];               /* ram_00D4_obj */
    uint8_t approach[4];                  /* ram_00AC_obj */
    uint8_t animation_state[4];            /* ram_0368_obj */
    uint8_t lateral_state[4];              /* ram_00CC_obj */
    uint8_t motion_fraction[4];            /* ram_0380_obj */
    uint8_t motion_whole[4];               /* ram_0384_obj */
    uint8_t motion_fraction_start[4];      /* ram_0378_obj */
    uint8_t motion_whole_start[4];         /* ram_037C_obj */
    uint8_t motion_delta[4];               /* ram_038C_obj */
    uint8_t track_finished;                /* ram_0052 */
    uint8_t global_vertical_reference;     /* ram_03F1 */
    uint8_t frame_phase;                   /* ram_004C */
    uint8_t player_event;                   /* ram_00FF */

} BikeMovementState;

void bikePhysicsBuildMovementDeltas(BikeMovementState *movement);
void bikePhysicsAdvancePlayerScroll(BikeMovementState *movement);
void bikePhysicsAdvanceOpponentPositions(BikeMovementState *movement);
/* sub_DBFE: fractional parallax accumulator and secondary camera scroll. */
void bikePhysicsAdvanceSecondaryScroll(BikeMovementState *movement);
/* sub_DCDE: update the four per-bike lock timers after the object pass. */
void bikePhysicsUpdateObjectLockTimers(BikeMovementState *movement);
/* sub_DC1A/sub_DCF2: source-derived vertical recovery and per-bike movement. */
void bikePhysicsUpdateVerticalRecovery(BikeMovementState *movement, uint8_t object_index);
void bikePhysicsUpdateVerticalMotion(BikeMovementState *movement, uint8_t object_index);
/* sub_DD06: initialize a native jump arc and execute its first DD1A/DD6F step. */
void bikePhysicsInitializeVerticalJump(BikeMovementState *movement, uint8_t object_index);
/* Complete source-order movement/camera pass from sub_DA26 (timer path). */
void bikePhysicsUpdateRaceMovementPass(BikeMovementState *movement, uint8_t race_started);

/* Native translation of the per-bike state dispatcher sub_D918/sub_D924.
 * This is semantic gameplay state, not a live NES RAM image. */
typedef struct BikeStateMachineState {
    uint8_t state[4];                 /* ram_009C_obj */
    uint8_t rider_present[4];         /* ram_0370_obj */
    uint8_t x[4];                     /* ram_0080_obj */
    uint8_t previous_x[4];            /* ram_0390_obj */
    uint8_t target_index[4];          /* ram_03E4_obj */
    uint8_t y[4];                     /* ram_obj_pos_Y_lo */
    uint8_t vertical_delta[4];       /* ram_00DC_obj */
    uint8_t speed_whole[4];           /* ram_0094_obj */
    uint8_t timer_state[4];           /* ram_002A_timer */
    uint8_t timer_recovery[4];        /* ram_0036_timer */
    uint8_t vertical_state[4];        /* ram_0398_obj */
    uint8_t vertical_offset[4];       /* ram_00BC_obj */
    uint8_t overheat[4];              /* ram_03E0_obj */
    uint8_t global_overheat;          /* ram_03F7 */
    uint8_t global_timer_0022;         /* ram_0022_timer */
    uint8_t frame_counter;             /* ram_003F */
    uint8_t frame_phase;               /* ram_004C */
    uint8_t controller_hold;           /* ram_btn_hold */
    uint8_t race_started;               /* ram_004F */
    uint8_t remembered_controller;     /* ram_0049 */
    uint8_t player_event;              /* ram_00FD */
    uint8_t opponent_spawn_event;      /* ram_03A8 */
    uint8_t vertical_collision_ref[4]; /* ram_006C_obj */
} BikeStateMachineState;

/* Exact source order: X=3..0, dispatch state, then update DA15 reference. */
void bikePhysicsUpdateBikeStates(BikeStateMachineState *state);

#endif
