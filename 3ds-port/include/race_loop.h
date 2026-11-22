#ifndef EXCITEBIKE_RACE_LOOP_H
#define EXCITEBIKE_RACE_LOOP_H

#include <stdint.h>
#include "race_world.h"

/*
 * Native race frame (ofs_000_C99A_02) in source order, over the unified
 * RaceWorld. Each stage loads the semantic structs of the already
 * differential-tested routine from the world, runs it, and stores the result.
 * Presentation sync (sub_D19B) is the application's frame boundary and is not
 * modelled. The return value of raceLoopStep has bit `stage` set when a stage
 * reported a state outside its verified domain (never guessed).
 */
typedef enum RaceLoopStage {
    RACE_STAGE_CA08 = 0, RACE_STAGE_D918, RACE_STAGE_DD8D, RACE_STAGE_E733, RACE_STAGE_E927,
    RACE_STAGE_CA9B, RACE_STAGE_DA9F, RACE_STAGE_DBC4, RACE_STAGE_DEBB, RACE_STAGE_DF30,
    RACE_STAGE_EAC5, RACE_STAGE_DFD5, RACE_STAGE_E09F, RACE_STAGE_DDD1, RACE_STAGE_CD1F,
    RACE_STAGE_E96C, RACE_STAGE_DE31, RACE_STAGE_E836, RACE_STAGE_EA44, RACE_STAGE_F4FF,
    RACE_STAGE_E4C8, RACE_STAGE_DEE2, RACE_STAGE_E359, RACE_STAGE_E17F, RACE_STAGE_E456,
    RACE_STAGE_DA26, RACE_STAGE_E70B, RACE_STAGE_CED0, RACE_STAGE_E42B,
    RACE_STAGE_COUNT
} RaceLoopStage;

/* NMI prefix: frame counter and timer decrement (runs before the race frame). */
void raceLoopNmiTick(RaceWorld *world);
/* Run one stage; returns nonzero when it left its verified domain. */
uint8_t raceLoopRunStage(RaceWorld *world, RaceLoopStage stage);
/* Entire frame including the pause/finish gates of sub_C99A. */
uint32_t raceLoopStep(RaceWorld *world);

#endif
