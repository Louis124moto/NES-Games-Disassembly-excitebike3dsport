#ifndef EXCITEBIKE_RACE_RESULTS_H
#define EXCITEBIKE_RACE_RESULTS_H

#include <stdint.h>
#include "race_world.h"

#define RACE_RECORD_BYTES 0x42u /* $0580-$05C1: widest record triplet read via CB06 */

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

/*
 * ofs_001_C485_03_05: result-state entry. Compares $0068-$006A with the saved
 * record at $0580 + sub_CB06 index (updating it when faster), derives the
 * rank/gate byte $0053 (sub_C522 thresholds, $03F8 scaling) and runs the
 * C514 tail. Returns 1 if the record index leaves the modelled $0580 window.
 */
unsigned raceResultsEnter(RaceWorld *world);

/* sub_C1FD/C229 power-on record seed (tbl_C091 twice, $05A0 = $0A). */
void raceRecordsDefault(uint8_t out[RACE_RECORD_BYTES]);

/* sub_CB06 record index and sub_C522 threshold triplet (course 0..4). */
unsigned raceRecordIndex(uint8_t course, uint8_t flag46, uint8_t multiplayer);
void raceThresholdTriplet(uint8_t course, uint8_t flag46, uint8_t out[3]);

#endif
