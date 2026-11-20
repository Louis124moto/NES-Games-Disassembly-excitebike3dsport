#ifndef EXCITEBIKE_RACE_SCRIPT_H
#define EXCITEBIKE_RACE_SCRIPT_H

#include <stdint.h>
#include "bike_physics.h"
#include "race_sprites.h"

/*
 * Native port of the opponent/course script interpreter of the race frame:
 * sub_E733 (slots 3,2,1 then 0 through the fall-through into sub_E73B),
 * sub_E927, sub_E794 and the 21 handlers of tbl_E6B7 ($E818-$E963, $EA8F).
 * Script bytes are the source-derived blobs at $E584-$E699 (21 scripts
 * addressed by tbl_E55A/tbl_E56F). Semantic state only; no NES RAM image.
 *
 * Unsupported source states (never produced by the first-course scripts) are
 * reported in unsupported_mask instead of being guessed: handler index $11 and
 * >= $15 jump into table data as code; setup arguments with low nibble < 2
 * index outside tbl_E6AD; a script index beyond the embedded blob.
 */
typedef struct RaceScriptState {
    BikeMovementState movement;
    uint8_t script_x_high[4];     /* ram_00C0_obj */
    uint8_t script_index[4];      /* ram_00C4_obj */
    uint8_t position_x[4];        /* ram_obj_pos_X_lo (ram_00C8) */
    uint8_t slot_00A0[4];         /* ram_00A0_obj */
    uint8_t slot_00B4[4];         /* ram_00B4_obj */
    uint8_t slot_00D0[4];         /* ram_00D0_obj */
    uint8_t slot_00D8[4];         /* ram_00D8 */
    uint8_t slot_00E4[4];         /* ram_00E4_obj */
    uint8_t slot_036C[4];         /* ram_036C_obj */
    uint8_t timer_003C;           /* ram_003C_timer */
    uint8_t player_overheat_03E0; /* ram_03E0_obj (player) */
    uint8_t ram_03B6;             /* ram_03B6 */
    uint8_t ram_0057;             /* ram_0057 */
    uint8_t timer_003A;           /* ram_003A_timer */
    uint8_t timer_0032;           /* ram_0032_timer */
    uint8_t sound_request_00FD;   /* ram_00FD */
    uint8_t cue_tiles_03D9[7];    /* ram_03D9..03DF */
    uint8_t gate_tile[7];         /* OAM tile bytes $02C1,$02C5,..,$02D9 */
    uint8_t unsupported_mask;     /* bit n = slot n hit an unsupported state */
} RaceScriptState;

/* sub_E733: run the script interpreter for slots 3,2,1,0. */
void raceScriptUpdateOpponents(RaceScriptState *state);
/* sub_E927: dispatch the pending lateral-state opcode of slots 3..0. */
void raceScriptDispatchLateral(RaceScriptState *state);

/* sub_E70B: consume movement delta against the per-slot origin counter,
 * stepping the object class (ram_00E0) every 8 pixels, and clear ram_00D8. */
void raceScriptAdvanceOrigins(RaceScriptState *state);

#endif
