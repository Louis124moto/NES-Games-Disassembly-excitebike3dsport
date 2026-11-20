#ifndef EXCITEBIKE_NATIVE_RACE_SETUP_H
#define EXCITEBIKE_NATIVE_RACE_SETUP_H

#include <stdint.h>
#include "bike_physics.h"

/* Semantic inputs consumed by C875 from setup state established earlier. */
typedef struct NativeRaceSetupInput {
    uint8_t multiplayer_flag;       /* ram_03F2 */
    uint8_t object_param_0019[4];   /* ram_0019,X */
    uint8_t wrapped_param_0018;     /* ram_0019,$FF wraps to $0018 */
    uint8_t timer_0034;
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
