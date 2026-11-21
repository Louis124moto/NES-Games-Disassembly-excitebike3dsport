#ifndef EXCITEBIKE_RACE_FRAME_CONTROL_H
#define EXCITEBIKE_RACE_FRAME_CONTROL_H

#include <stdint.h>

/*
 * Native pause/input step (sub_CA38_pause_and_unpause, sub_CA1B) and the
 * finish-line palette/flow step (sub_CA9B). Presentation effects are outputs:
 * sound request bytes ($00FB/$00FC), the APU channel enable write ($4015) and
 * the PPU palette packet bytes $0312-$031B. The `ram_03F7` != 0 paths (restart
 * through loc_CA85 / loc_C36A) are unreachable in the first race and are
 * reported through `unsupported` instead of being guessed.
 */
typedef struct RaceFrameControlState {
    uint8_t pause_timer;      /* ram_pause_timer ($03B2) */
    uint8_t start_latch;      /* ram_03B0 */
    uint8_t pause_flag;       /* ram_pause_flag ($03B1) */
    uint8_t button_hold;      /* ram_btn_hold ($14) */
    uint8_t button_press;     /* ram_btn_press ($15) */
    uint8_t input_005C;       /* ram_005C_obj[0] */
    uint8_t sound_00FB;       /* ram_00FB */
    uint8_t sound_00FC;       /* ram_00FC */
    uint8_t apu_enable_write; /* 1 when $4015 was written this call */
    uint8_t apu_enable_value; /* value written to $4015 */
    uint8_t flag_03F7;        /* ram_03F7 */
    uint8_t timer_0032;       /* ram_0032_timer */
    uint8_t track_finished;   /* ram_0052_track_finished_flag */
    uint8_t finish_phase_003F;/* ram_003F */
    uint8_t stream_index;     /* ram_0043 */
    uint8_t course_id;        /* ram_0041 */
    uint8_t frame_flag_0040;  /* ram_0040 */
    uint8_t frame_flag_0044;  /* ram_0044 */
    uint8_t flag_0047;        /* ram_0047 */
    uint8_t counter_03A9;     /* ram_03A9 */
    uint8_t counter_03E0;     /* ram_03E0_obj[0] */
    uint8_t timer_003C;       /* ram_003C_timer */
    uint8_t ppu_packet[10];   /* RAM $0312-$031B */
    uint8_t unsupported;
} RaceFrameControlState;

/* sub_CA38_pause_and_unpause. */
void racePauseUpdate(RaceFrameControlState *state);
/* sub_CA1B. */
void raceHoldInput(RaceFrameControlState *state);
/* Tail of sub_CA08 after the per-bike loop: CA13-CA1F (03F7 == 0 path). */
void raceFrameInputStep(RaceFrameControlState *state);
/* sub_CA9B. */
void raceFinishUpdate(RaceFrameControlState *state);

#endif
