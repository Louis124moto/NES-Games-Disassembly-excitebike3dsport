#ifndef EXCITEBIKE_RACE_FRAME_H
#define EXCITEBIKE_RACE_FRAME_H

#include <stdint.h>
#include "race_sprites.h"
#include "race_spawn.h"

/*
 * Native translations of race-frame routines called from ofs_000_C99A_02
 * (bank_FF.asm). State is semantic: fields map to the source object arrays
 * (index = bike slot, 0 = player) with the RAM label in a comment. Sound
 * request bytes ($00FB-$00FF) are kept as semantic "sound request" fields for
 * the native audio engine; they are not APU registers.
 */

/* Source: sub_DDD1 ($DDD1-$DE30) — crash recovery / wipe-out countdown. */
typedef struct RaceCrashRecoveryState {
    uint8_t collision_state[4];     /* ram_0098_obj */
    uint8_t speed_whole[4];         /* ram_0094_obj */
    uint8_t speed_fraction[4];      /* ram_0090_obj */
    uint8_t movement_delta[4];      /* ram_0060 */
    uint8_t object_status[4];       /* ram_00B0_obj */
    uint8_t inactive[4];            /* ram_0058_obj */
    uint8_t approach[4];            /* ram_00AC_obj */
    uint8_t x_low[4];               /* ram_0080_obj */
    uint8_t previous_x[4];          /* ram_0390_obj */
    uint8_t y_low[4];               /* ram_obj_pos_Y_lo (ram_00B8) */
    uint8_t derived_y[4];           /* ram_006C_obj (output of sub_DA15) */
    uint8_t movement_state[4];      /* ram_009C_obj */
    uint8_t crash_variant[4];       /* ram_03E4_obj */
    uint8_t recovery_flag[4];       /* ram_0398_obj */
    uint8_t player_crashed_03A9;    /* ram_03A9 */
    uint8_t sound_request_00FC;     /* ram_00FC */
    uint8_t sound_request_00FD;     /* ram_00FD */
    uint8_t frame_phase_004C;       /* ram_004C */
    uint8_t random_001B;            /* ram_001B */
} RaceCrashRecoveryState;

void raceCrashRecovery(RaceCrashRecoveryState *state);

/* Source: sub_DEBB ($DEBB-$DEE1) — opponent passing sound cue, slots 3..1. */
typedef struct RaceOpponentCueState {
    uint8_t x_high[4];            /* ram_0084_obj */
    uint8_t x_low[4];             /* ram_0080_obj */
    uint8_t cue_flag[3];          /* ram_03AA_obj (index = slot - 1) */
    uint8_t sound_request_00FF;   /* ram_00FF */
} RaceOpponentCueState;

void raceOpponentCue(RaceOpponentCueState *state);

/* Source: sub_DEE2 ($DEE2-$DF2F) — engine sound pitch request from speed. */
typedef struct RaceEngineSoundState {
    uint8_t speed_whole;          /* ram_0094_obj (player) */
    uint8_t speed_fraction;       /* ram_0090_obj (player) */
    uint8_t object_status;        /* ram_00B0_obj (player) */
    uint8_t controller;           /* ram_005C_obj (player) */
    uint8_t player_crashed_03A9;  /* ram_03A9 */
    uint8_t frame_phase_004C;     /* ram_004C */
    uint8_t frame_counter_003F;   /* ram_003F */
    uint8_t sound_request_00FC;   /* ram_00FC */
} RaceEngineSoundState;

void raceEngineSound(RaceEngineSoundState *state);

/* Semantic sprite record (not an OAM image): the native renderer draws these. */
typedef struct RaceSprite {
    uint8_t y;
    uint8_t tile;
    uint8_t attribute;
    uint8_t x;
} RaceSprite;

/* Source: sub_DE31 ($DE31-$DEBA) — pre-race countdown/start-lights banner. */
typedef struct RaceStartBannerState {
    uint8_t timer_0024;             /* ram_0024_timer */
    uint8_t timer_0034;             /* ram_0034_timer */
    uint8_t race_started_004F;      /* ram_004F_race_started_flag */
    uint8_t banner_x_03B4;          /* ram_03B4 */
    uint8_t player_delta_0060;      /* ram_0060 (player movement delta) */
    uint8_t player_crashed_03A9;    /* ram_03A9 */
    uint8_t controller_005C;        /* ram_005C_obj (player) */
    uint8_t sound_request_00FB;     /* ram_00FB */
    uint8_t sound_request_00FE;     /* ram_00FE */
    RaceSprite banner[8];           /* OAM slots $02C0-$02DF as semantic sprites */
} RaceStartBannerState;

void raceStartBanner(RaceStartBannerState *state);

/* Source: sub_EA44 ($EA44-$EA8E) with sub_DC9A and sub_E2F6 — per-bike
 * marker/shadow sprite pairs at OAM $0294-$02B3 (two sprites per slot). */
typedef struct RaceEffectSpriteState {
    uint8_t x_high[4];              /* ram_0084_obj */
    uint8_t x_low[4];               /* ram_0080_obj */
    uint8_t object_status[4];       /* ram_00B0_obj */
    uint8_t vertical_reference[4];  /* ram_00BC_obj */
    uint8_t object_y[4];            /* ram_obj_pos_Y_lo */
    uint8_t marker_index[4];        /* ram_00B4_obj */
    uint8_t y_low[4];               /* ram_008C */
    uint8_t dust_flags[4];          /* ram_036C_obj */
    uint8_t speed_whole[4];         /* ram_0094_obj */
    uint8_t speed_fraction[4];      /* ram_0090_obj */
    uint8_t frame_counter_003F;     /* ram_003F */
    uint8_t global_vertical_reference_03F1; /* ram_03F1 */
    RaceSprite sprite[8];           /* OAM $0294-$02B3: slot x uses [2x],[2x+1] */
} RaceEffectSpriteState;

void raceEffectSprites(RaceEffectSpriteState *state);

/* Source: sub_E4C8 ($E4C8-$E52B) with sub_E52C — bike-to-bike collision flag.
 * Source quirks preserved: the Y loop tests ram_0080_obj,X (not Y) for the
 * off-screen case, and a vertical difference of exactly $EC is "no overlap" (Z flag). */
typedef struct RaceCollisionState {
    uint8_t x_high[4];            /* ram_0084_obj */
    uint8_t x_low[4];             /* ram_0080_obj */
    uint8_t y_low[4];             /* ram_008C */
    uint8_t collision_flag_03A3;  /* ram_03A3_flag (output) */
} RaceCollisionState;

void raceBikeCollision(RaceCollisionState *state);

/* Source: loc_E42B ($E42B-$E455) with sub_E4AD_hide_4_sprites. Domain:
 * toggle_03B3 in 0..7 (tbl_D8DC length). */
typedef struct RaceHideSpritesState {
    uint8_t collision_flag_03A3;  /* ram_03A3_flag */
    uint8_t slot_active_0088[4];  /* ram_0088 */
    uint8_t rider_present_0373;   /* ram_0370_obj + 3 */
    uint8_t toggle_03B3;          /* ram_03B3 */
    NativeOamState oam;           /* OAM byte layout used by the native renderer */
} RaceHideSpritesState;

void raceHideCollidedSprites(RaceHideSpritesState *state);

/* Source: sub_E456 ($E456-$E4C7). Hide off-screen opponent sprite groups,
 * and clear an opponent when it crosses the player transition window.
 * slot_object_index is the semantic mapping from ram_0088,Y (0 = unused).
 */
typedef struct RaceOffscreenSpriteState {
    uint8_t slot_object_index[4]; /* ram_0088 */
    uint8_t opponent_cleared_03A6; /* ram_03A6 */
    NativeOamState oam;
} RaceOffscreenSpriteState;

void raceHideOffscreenOpponentSprites(RaceOffscreenSpriteState *state,
                                      RaceSpawnState *opponents);

/* Source: sub_E70B ($E70B-$E732). Smooth four per-bike X targets and clear
 * the transient D8 field. Values are semantic per-bike state, not NES RAM.
 */
typedef struct RacePositionSmoothingState {
    uint8_t movement_delta[4]; /* ram_0060 */
    uint8_t target_x[4];        /* ram_0064 */
    uint8_t phase[4];           /* ram_00E0_obj */
    uint8_t transient[4];       /* ram_00D8_obj */
} RacePositionSmoothingState;

void raceSmoothBikePositions(RacePositionSmoothingState *state);


/* Source: sub_DF30 ($DF30-$DF7D) — elapsed race timer and time-up flag.
 * Decimal work/display bytes preserve the exact DF7F output layout.
 */
typedef struct RaceElapsedTimeState {
    uint8_t race_started;       /* ram_004F */
    uint8_t track_finished;     /* ram_0052 */
    uint8_t minutes;            /* ram_0068 */
    uint8_t seconds;            /* ram_0069 */
    uint8_t hundredths;         /* ram_006A */
    uint8_t frame_fraction;     /* ram_006B */
    uint8_t decimal_work[7];    /* ram_03D1-$03D7 */
    uint8_t display[7];         /* ram_03D9-$03DF */
} RaceElapsedTimeState;

void raceElapsedTimeUpdate(RaceElapsedTimeState *state);

/* Source: sub_DFD5 ($DFD5-$E097) with sub_E091/sub_E098 — bike-to-bike contact
 * resolution for slot pairs (X, Y > X). */
typedef struct RaceBikeContactState {
    uint8_t active[4];            /* ram_00A8_obj */
    uint8_t x_high[4];            /* ram_0084_obj */
    uint8_t position[4];          /* ram_0070_obj */
    uint8_t collision_state[4];   /* ram_0098_obj */
    uint8_t overheat_03E0[4];     /* ram_03E0_obj */
    uint8_t object_status[4];     /* ram_00B0_obj */
    uint8_t y_low[4];             /* ram_008C */
    uint8_t x_low[4];             /* ram_0080_obj */
    uint8_t timer_003B;           /* ram_003B_timer */
    uint8_t sound_request_00FD;   /* ram_00FD */
} RaceBikeContactState;

void raceBikeContacts(RaceBikeContactState *state);

/* Source: sub_E09F ($E09F-$E0E5) — player hitting the slot-3 obstacle. */
typedef struct RaceObstacleHitState {
    uint8_t movement_state[4];    /* ram_009C_obj */
    uint8_t x_high_slot3;         /* ram_0084_obj + $03 */
    uint8_t rider_present_0373;   /* ram_0370_obj + $03 */
    uint8_t object_y_player;      /* ram_obj_pos_Y_lo */
    uint8_t obstacle_y_006F;      /* ram_006C_obj + $03 */
    uint8_t x_low_player;         /* ram_0080_obj */
    uint8_t obstacle_x_0393;      /* ram_0390_obj + $03 */
    uint8_t global_vertical_reference_03F1; /* ram_03F1 */
    uint8_t vertical_reference_player;      /* ram_00BC_obj */
    uint8_t y_low_player;         /* ram_008C */
    uint8_t collision_player;     /* ram_0098_obj (output) */
    uint8_t lane_delta_slot3;     /* ram_00DC_obj + $03 (output) */
    uint8_t sound_request_00FD;   /* ram_00FD (output) */
    uint8_t state_039B;           /* ram_0398_obj + $03 (output) */
} RaceObstacleHitState;

void raceObstacleHit(RaceObstacleHitState *state);

#endif
