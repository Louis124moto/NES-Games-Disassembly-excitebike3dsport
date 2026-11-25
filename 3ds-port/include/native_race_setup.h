#ifndef EXCITEBIKE_NATIVE_RACE_SETUP_H
#define EXCITEBIKE_NATIVE_RACE_SETUP_H

#include <stdint.h>
#include "bike_physics.h"
#include "native_rng.h"

/* Semantic inputs consumed by C875 from setup state established earlier. */
typedef struct NativeRaceSetupInput {
    uint8_t multiplayer_flag;       /* ram_03F2: nonzero initializes slots 1..3 during C875; identity is fixed separately */
    uint8_t course_index_0043;       /* selected course 0..4 (tbl_ED3A stream / tbl_C091 row) */
    const uint8_t *records;          /* $0580-$05C1 record window (NULL = power-on seed) */
    uint8_t setup_index_0041;        /* C905: first-course path is below $08 */
    uint8_t transition_flag_0046;    /* ram_0046, consumed by sub_C522/sub_CB13 */
    uint8_t timer_0034;
    uint8_t countdown_timer_003C; /* ram_003C_timer, not cleared by C869/C820 */
    uint8_t flag_004B;               /* ram_004B: set after the first race of a session */
    uint8_t counter_03F3;            /* ram_03F3: course-5 replay counter (C8F3) */
    uint8_t transition_counter_03F8; /* ram_03F8 consumed by C90E */
    uint8_t setup_counter_0040;
    uint8_t setup_counter_0047;
} NativeRaceSetupInput;

typedef enum NativeRaceBikeRole {
    NATIVE_RACE_BIKE_UNUSED = 0,
    NATIVE_RACE_BIKE_PLAYER = 1,
    NATIVE_RACE_BIKE_OPPONENT = 2
} NativeRaceBikeRole;

typedef struct NativeRaceBikeSetup {
    uint8_t y;
    uint8_t x_low;
    uint8_t x_high;
    uint8_t object_class;
    uint8_t approach;
    uint8_t lock_timer;
    uint8_t active;
    uint8_t object_origin_x;
    uint8_t setup_parameter;
    uint8_t role;                 /* slot identity: slot 0 player; slots 1..3 AI, even when not currently active */
} NativeRaceBikeSetup;

typedef struct NativeRaceSetupState {
    NativeRaceBikeSetup bike[4];
    uint8_t setup_counter_0040;
    uint8_t setup_counter_0047;
    uint8_t timer_0034;
    uint8_t transition_flag_0046;
    uint8_t global_vertical_reference;
    uint8_t player_slot;
    uint8_t first_opponent_slot;    /* ram_004A from C36C: CED0 starts at slot 1 */
    uint8_t player_rider_present;
    uint8_t half_lock_slot;
    uint8_t camera_scroll_00E8;     /* ram_00E8 = $34 after C875 */
    uint8_t camera_scroll_00E9;     /* ram_00E9 = $14 after C875 */
    uint8_t camera_scroll_00EA;     /* ram_00EA = $25 from C820 */
    uint8_t camera_scroll_00EB;     /* ram_00EB = $0A from C820 */
    uint8_t countdown_x_03B4;       /* ram_03B4 = $80 */
    uint8_t race_setup_state_03B0;  /* ram_03B0 = $80 */
    uint8_t countdown_limit_03BD;   /* ram_03BD = $FF */
    uint8_t countdown_index_03BE;   /* ram_03BE = $02 */
    uint8_t course_setup_parameters[3]; /* sub_C522: tbl_C091 + course/transition offset */
    uint8_t race_object_parameters[3]; /* sub_C53E: C1FD-seeded $0580 lookup */
    uint8_t counter_visible;        /* C8E2: $0046 != 0 && course 4 queues PPU $2378 */
    uint8_t counter_tiles[4];       /* C158 text: $1D,$F8,tens|$FE,units (PPU $2378) */
    uint8_t hud_glyphs[7]; /* semantic digit/sentinel bytes produced by DF7F/C935 */
} NativeRaceSetupState;

/*
 * Native semantic projection of the first-course C875 path, including its
 * source-derived table parameters and HUD glyph values. Writes gameplay fields
 * into native movement/state models; it does not emulate PPU command buffers.
 */
/* Semantic reset of gameplay fields cleared by C869 and C820. */
void nativeRaceSetupClearFirstCourseState(BikeMovementState *movement,
                                          BikeStateMachineState *states);

int nativeRaceSetupFirstCourse(NativeRaceSetupState *out,
                               const NativeRaceSetupInput *in,
                               const NativeRngState *rng,
                               BikeMovementState *movement,
                               BikeStateMachineState *states);

#endif
