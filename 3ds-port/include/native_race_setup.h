#ifndef EXCITEBIKE_NATIVE_RACE_SETUP_H
#define EXCITEBIKE_NATIVE_RACE_SETUP_H

#include <stdint.h>
#include "bike_physics.h"

/* Semantic inputs consumed by C875 from setup state established earlier. */
typedef struct NativeRaceSetupInput {
    uint8_t multiplayer_flag;       /* ram_03F2 */
    uint8_t setup_index_0041;        /* C905: first-course path is below $08 */
    uint8_t transition_flag_0046;    /* ram_0046, consumed by sub_C522/sub_CB13 */
    uint8_t object_param_0019[4];   /* ram_0019,X */
    uint8_t wrapped_param_0018;     /* ram_0019,$FF wraps to $0018 */
    uint8_t timer_0034;
    uint8_t transition_counter_03F8; /* ram_03F8 consumed by C90E */
    uint8_t setup_counter_0040;
    uint8_t setup_counter_0047;
} NativeRaceSetupInput;

typedef struct NativeRaceBikeSetup {
    uint8_t y;
    uint8_t x_low;
    uint8_t x_high;
    uint8_t object_class;
    uint8_t approach;
    uint8_t lock_timer;
    uint8_t active;
    uint8_t animation_seed;
    uint8_t speed_phase;
} NativeRaceBikeSetup;

typedef struct NativeRaceSetupState {
    NativeRaceBikeSetup bike[4];
    uint8_t setup_counter_0040;
    uint8_t setup_counter_0047;
    uint8_t timer_0034;
    uint8_t global_vertical_reference;
    uint8_t player_slot;
    uint8_t player_rider_present;
    uint8_t half_lock_slot;
    uint8_t course_setup_parameters[3]; /* sub_C522: tbl_C091 + course/transition offset */
    uint8_t race_object_parameters[3]; /* sub_C53E: C1FD-seeded $0580 lookup */
    uint8_t hud_glyphs[7]; /* semantic digit/sentinel bytes produced by DF7F/C935 */
} NativeRaceSetupState;

/*
 * Native semantic projection of the C875 entry/setup loop through C8DC.
 * Writes the corresponding fields in the already-native movement/state models.
 * It intentionally does not emulate PPU command construction in C902-C935.
 */
int nativeRaceSetupFirstCourse(NativeRaceSetupState *out,
                               const NativeRaceSetupInput *in,
                               BikeMovementState *movement,
                               BikeStateMachineState *states);

#endif
