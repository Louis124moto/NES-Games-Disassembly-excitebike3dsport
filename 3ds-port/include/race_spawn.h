#ifndef EXCITEBIKE_RACE_SPAWN_H
#define EXCITEBIKE_RACE_SPAWN_H

#include <stdint.h>

/*
 * Native opponent spawn/despawn (sub_DA9F, sub_DAE3, sub_DB50, sub_DB95,
 * sub_DBC4, loc_DC69). Semantic per-slot gameplay state; field comments name
 * the source RAM labels. Track lookups through sub_E7FF read the six 64-byte
 * course pages (RAM $0400-$057F in the original), kept here as data.
 * Domain: slot indices 1..3, x_high in 0..2 (the spawn code only ever stores 0 or 2).
 */
typedef struct RaceSpawnState {
    uint8_t active[4];            /* ram_00A8_obj */
    uint8_t y_low[4];             /* ram_obj_pos_Y_lo (ram_00B8) */
    uint8_t vertical_reference[4];/* ram_00BC_obj */
    uint8_t slot_00A0[4];         /* ram_00A0_obj */
    uint8_t vertical_phase[4];    /* ram_00A4_obj */
    uint8_t action_state[4];      /* ram_0058_obj */
    uint8_t lateral_state[4];     /* ram_00CC_obj */
    uint8_t dust_flags[4];        /* ram_036C_obj */
    uint8_t speed_fraction[4];    /* ram_0090_obj */
    uint8_t speed_whole[4];       /* ram_0094_obj */
    uint8_t collision_state[4];   /* ram_0098_obj */
    uint8_t movement_state[4];    /* ram_009C_obj */
    uint8_t object_status[4];     /* ram_00B0_obj */
    uint8_t brake_state[4];       /* ram_00D4_obj */
    uint8_t movement_mode[4];     /* ram_0388_obj */
    uint8_t script_x_high[4];     /* ram_00C0_obj */
    uint8_t block_039C[4];        /* ram_039C..039F (DB95 clears [x], DB42 writes [x-1]) */
    uint8_t lane_delta[4];        /* ram_00DC_obj */
    uint8_t position_0070[4];     /* ram_0070_obj */
    uint8_t rider_present[4];     /* ram_0370_obj */
    uint8_t x_high[4];            /* ram_0084_obj */
    uint8_t x_low[4];             /* ram_0080_obj */
    uint8_t screen_y[4];          /* ram_008C */
    uint8_t approach[4];          /* ram_00AC_obj */
    uint8_t object_class[4];      /* ram_00E0_obj */
    uint8_t origin_x[4];          /* ram_0064 */
    uint8_t start_lock[4];        /* ram_0078 */
    uint8_t setup_parameter[4];   /* ram_0074 */
    uint8_t rng[8];               /* ram_0018..001F (native RNG bytes) */
    uint8_t track_pages[6][64];   /* RAM $0400-$057F via tbl_E54E/tbl_E554 */
    uint8_t track_finished;       /* ram_0052 */
    uint8_t timer_0023;           /* ram_0023_timer */
    uint8_t timer_003B;           /* ram_003B_timer */
    uint8_t frame_phase;          /* ram_004C */
    uint8_t spawn_request_03A8;   /* ram_03A8 */
} RaceSpawnState;

/* sub_DB95 for slot x. */
void raceSpawnClearSlot(RaceSpawnState *state, unsigned x);
/* sub_DB50 for slot x with Y = x_high[x] at the call. */
void raceSpawnSetupSpeed(RaceSpawnState *state, unsigned x, unsigned y);
/* sub_DAE3: returns 1 when the opponent was placed (Z clear), 0 otherwise. */
uint8_t raceSpawnPlace(RaceSpawnState *state, unsigned x, unsigned y);
/* sub_DA9F: try to spawn opponents in slots 3..1. */
void raceSpawnUpdate(RaceSpawnState *state);
/* sub_DBC4: respawn/despawn handling of slot 3. */
void raceSpawnHandleSlot3(RaceSpawnState *state);

#endif
