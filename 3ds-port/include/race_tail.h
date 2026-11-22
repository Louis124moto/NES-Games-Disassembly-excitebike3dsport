#ifndef EXCITEBIKE_RACE_TAIL_H
#define EXCITEBIKE_RACE_TAIL_H

#include <stdint.h>

/*
 * Tail of sub_DA26 ($DA44-$DA55): when ram_003C_timer is zero it dispatches on
 * ram_03BC through tbl_D8EF/tbl_D8F1 to ofs_004_E0E6_00 (overhead banner
 * sprite) or ofs_004_E2AF_01 (finish sign countdown). Both draw their sprite
 * grids into OAM through sub_D1C7/sub_E16C. `unsupported` is set when
 * ram_03BC indexes beyond the two-entry table.
 */
typedef struct RaceTailState {
    uint8_t timer_003A;
    uint8_t track_finished_0052;
    uint8_t timer_0024;
    uint8_t flag_03A0;
    uint8_t timer_002E;
    uint8_t rng_0018;
    uint8_t tile_03A1;
    uint8_t x_03B9;
    uint8_t scroll_0060;
    uint8_t frame_003F;
    uint8_t timer_003C;
    uint8_t state_03BC;
    uint8_t scroll_03BD;
    uint8_t scroll_03BE;
    uint8_t blink_03BB;
    uint8_t timer_0020;
    uint8_t oam[256];
    uint8_t unsupported;
} RaceTailState;

void raceFrameTail(RaceTailState *state);

#endif
