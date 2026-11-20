#ifndef EXCITEBIKE_RACE_COURSE_STREAM_H
#define EXCITEBIKE_RACE_COURSE_STREAM_H

#include <stdint.h>
#include "race_spawn.h"

/*
 * Native course stream reader (sub_F4FF with sub_F82E, sub_F676, sub_F681,
 * sub_F64E, sub_ECBF). Semantic state; comments name the source RAM labels.
 * The six 64-byte course pages ($0400-$057F) and ram_00E0_obj[0] /
 * ram_03A8 live in RaceSpawnState. `buffer` models $0300-$0311, the column
 * update buffer written by the reader and consumed by sub_F64E.
 * `unsupported` is set (and the call aborts) when the original would read
 * outside the embedded course data or touch RAM this model does not hold.
 */
typedef struct RaceCourseStreamState {
    uint8_t stream_index;         /* ram_0043 */
    uint8_t course_id;            /* ram_0041 */
    uint8_t flag_0046;            /* ram_0046_flag */
    uint8_t next_column_00EB;     /* ram_00EB */
    uint8_t repeat_count_00EC;    /* ram_00EC */
    uint8_t stream_offset_00ED;   /* ram_00ED */
    uint8_t template_00EE;        /* ram_00EE */
    uint8_t template_offset_00EF; /* ram_00EF */
    uint8_t column_00E8;          /* ram_00E8 */
    uint8_t row_00E9;             /* ram_00E9 */
    uint8_t nametable_00EA;       /* ram_00EA */
    uint8_t counter_0057;         /* ram_0057 */
    uint8_t flag_03A2;            /* ram_03A2_useless */
    uint8_t counter_03A4;         /* ram_03A4 */
    uint8_t counter_03A5;         /* ram_03A5 */
    uint8_t flag_03BC;            /* ram_03BC */
    uint8_t buffer[0x12];         /* RAM $0300-$0311 */
    uint8_t unsupported;          /* 1 when the call left the modelled domain */
} RaceCourseStreamState;

/* sub_F4FF. */
void raceCourseStreamUpdate(RaceCourseStreamState *state, RaceSpawnState *spawn);

#endif
