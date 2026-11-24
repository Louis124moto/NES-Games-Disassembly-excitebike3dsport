#ifndef EXCITEBIKE_RACE_RESULTS_H
#define EXCITEBIKE_RACE_RESULTS_H

#include "race_world.h"

/*
 * Exact translation of the results-screen phase dispatcher sub_CB2C and its
 * four handlers, running on the same RaceWorld as the race:
 *   $0044 = 0  ofs_CB3B_00  (course/flag bookkeeping, 4-frame wait)
 *   $0044 = 1  ofs_CB77_01  (current-time row command, $03AD wait length)
 *   $0044 = 2  ofs_CBA4_02  (podium OAM, rank digits, record marker, CC15/CC7D)
 *   $0044 = 3  ofs_CC6B_03  (advance $0041)
 * One call equals one pass of the source state handler for states $03/$06/$0B.
 *
 * Returns 0 on success, or 1 when the source would leave the modelled domain
 * ($0044 >= 4 jumps into non-handler bytes, or the PPU command buffer at
 * $0300 would overflow the modelled $0300-$033F window); the world is then
 * left untouched.
 */
unsigned raceResultsPhaseStep(RaceWorld *world);

#endif
