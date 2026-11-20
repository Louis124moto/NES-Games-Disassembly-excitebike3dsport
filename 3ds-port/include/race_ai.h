#ifndef EXCITEBIKE_RACE_AI_H
#define EXCITEBIKE_RACE_AI_H

#include <stdint.h>
#include "race_spawn.h"

/*
 * Native opponent AI controller: sub_CED0 with sub_CFEA, sub_CFCB, sub_D018,
 * sub_D000, sub_CF0C, sub_CF47, sub_CF6A, sub_CF88/CF8B, sub_CF96 and
 * sub_CFDB/CFE3. Shared per-slot fields live in the embedded RaceSpawnState
 * (same source RAM labels). Domain: vertical_band 0..5, first_slot_004A 0..3.
 */
typedef struct RaceAiState {
    RaceSpawnState spawn;
    uint8_t movement_flags[4];    /* ram_005C_obj */
    uint8_t desired_007C[4];      /* ram_007C_obj */
    uint8_t desired_0368[4];      /* ram_0368_obj */
    uint8_t vertical_band[4];     /* ram_0360_obj */
    uint8_t terrain_samples[4][4];/* ram_03C0, four bytes per slot */
    uint8_t flag_03A6;            /* ram_03A6_flag */
    uint8_t first_slot_004A;      /* ram_004A */
} RaceAiState;

/* sub_D018: classify the four terrain samples of slot x; returns the number
 * of obstacle samples found (Z set when zero) and updates ram_007C. */
uint8_t raceAiClassifyTerrain(RaceAiState *state, unsigned x);
/* sub_CED0: run the AI for slots first_slot_004A..3. */
void raceAiUpdate(RaceAiState *state);

#endif
