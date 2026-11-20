#ifndef EXCITEBIKE_RACE_FRAME_H
#define EXCITEBIKE_RACE_FRAME_H

#include <stdint.h>

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

#endif
