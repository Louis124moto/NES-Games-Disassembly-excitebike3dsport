#include "race_ai.h"

static const uint8_t k_tblC0D6[5] = { 0x40u, 0x58u, 0x48u, 0x48u, 0x78u };
#define SP (&s->spawn)

static uint8_t lookup(const RaceAiState *s, unsigned page, unsigned index)
{
    return SP->track_pages[page % 6u][index & 0x3Fu];
}

/* sub_CF8B: carry set unless the value is $3B..$3D. */
static int notRoad(uint8_t a) { return a < 0x3Bu || a >= 0x3Eu; }

/* sub_CF96: carry set when another bike blocks the neighbouring lane. */
static int laneBlocked(RaceAiState *s, unsigned x, unsigned y, uint8_t lane)
{
    const uint8_t hi = (uint8_t)(SP->x_low[x] + k_tblC0D6[y]);
    const uint8_t lo = (uint8_t)(hi - k_tblC0D6[y + 1u]);
    unsigned k;
    for (k = s->first_slot_004A; k < 4u; ++k) {
        if (k == x) continue;
        if ((SP->x_high[k] & 1u) == 0u) continue;
        if (SP->position_0070[k] == 0u) continue;
        if (SP->position_0070[k] != lane) continue;
        if (SP->x_low[k] >= hi) continue;
        if (SP->x_low[k] >= lo) return 1;
    }
    return 0;
}

static void moveRight(RaceAiState *s, unsigned x);

/* sub_CF6A */
static void moveLeft(RaceAiState *s, unsigned x)
{
    const unsigned y = (unsigned)s->vertical_band[x] + 1u;
    if (s->vertical_band[x] > 5u) return; /* outside the reproduced domain */
    if ((uint8_t)(y - 5u) < 0x80u) { moveRight(s, x); return; }
    if (notRoad(lookup(s, y, SP->object_class[x]))) return;
    if (laneBlocked(s, x, 0u, (uint8_t)(SP->position_0070[x] - 1u))) return;
    SP->lane_delta[x] = 0xFFu;
}

/* sub_CF47 */
static void moveRight(RaceAiState *s, unsigned x)
{
    const unsigned y = (unsigned)s->vertical_band[x] - 1u;
    if (s->vertical_band[x] > 5u) return; /* outside the reproduced domain */
    if ((uint8_t)(y - 1u) >= 0x80u) { moveLeft(s, x); return; }
    if (notRoad(lookup(s, y, SP->object_class[x]))) return;
    if (laneBlocked(s, x, 0u, (uint8_t)(SP->position_0070[x] + 1u))) return;
    SP->lane_delta[x] = 1u;
}

/* bra_CF64 */
static void preferLeft(RaceAiState *s, unsigned x)
{
    if (SP->script_x_high[x] == 0xE6u) moveRight(s, x); else moveLeft(s, x);
}

/* sub_CFDB */
static void clearHighFlags(RaceAiState *s, unsigned x)
{
    if (SP->speed_whole[x] != 0u) s->movement_flags[x] &= 0x0Fu;
}

/* sub_CF0C */
static void aiLaneChange(RaceAiState *s, unsigned x)
{
    if ((SP->x_high[x] & 1u) == 0u) return;
    if (laneBlocked(s, x, 2u, SP->position_0070[x])) {
        clearHighFlags(s, x);
        if ((SP->x_low[x] & 0x80u) == 0u) return;
        if ((SP->rng[x] & 1u) != 0u) preferLeft(s, x); else moveRight(s, x);
        return;
    }
    if (SP->lane_delta[x] != 0u) return;
    if ((SP->x_high[x] & 1u) == 0u) return;
    if (SP->x_low[x] >= 0xF0u) return;
    if ((SP->x_low[x] & 0x80u) == 0u) return;
    {
        const uint8_t r = SP->rng[1u + x];
        if (r < 0xE0u) return;
        if (r < 0xF8u) preferLeft(s, x); else moveRight(s, x);
    }
}

uint8_t raceAiClassifyTerrain(RaceAiState *s, unsigned x)
{
    unsigned count = 0u, y, i = 4u * x;
    uint8_t a;
    for (y = 4u; y > 0u; --y, ++i) {
        a = s->terrain_samples[(i >> 2) & 3u][i & 3u];
        if (a == 0xC0u || a == 0xC1u || (a >= 0x70u && a < 0x74u)) {
            const unsigned index = (SP->object_class[x] + (i & 3u) + 1u) & 0x3Fu;
            unsigned page;
            ++count;
            page = s->vertical_band[x];
            for (;;) { /* bra_D06F_loop */
                uint8_t v;
                --page;
                if ((uint8_t)(page - 1u) >= 0x80u) break;
                v = lookup(s, page, index);
                if (v == 0xFAu) break;
                if (notRoad(v)) continue;
                moveRight(s, x);
                goto finish;
            }
            page = s->vertical_band[x];
            for (;;) { /* bra_D08B_loop */
                uint8_t v;
                ++page;
                if ((uint8_t)(page - 6u) < 0x80u) break;
                v = lookup(s, page, index);
                if (v == 0xE4u) break;
                if (notRoad(v)) continue;
                moveLeft(s, x);
                break;
            }
            goto finish;
        }
        if (a >= 0x48u && a < 0x4Bu) {
            s->desired_007C[x] = 9u;
            return (uint8_t)count;
        }
    }
finish:
    s->desired_007C[x] = (SP->action_state[x] == 0u) ? s->desired_0368[x] : 0u;
    return (uint8_t)count;
}

void raceAiUpdate(RaceAiState *s)
{
    unsigned x;
    if (s == 0) return;
    for (x = s->first_slot_004A; x < 4u; ++x) {
        uint8_t flags, y;
        if (SP->active[x] == 0u) continue;
        if ((SP->collision_state[x] | SP->movement_state[x]) != 0u) continue;
        /* sub_CFEA */
        s->movement_flags[x] &= 0xF0u;
        y = 1u;
        if (s->desired_007C[x] != 0u && s->desired_007C[x] != SP->approach[x]) {
            if (s->desired_007C[x] > SP->approach[x]) y = 2u;
            s->movement_flags[x] |= y;
        }
        if (SP->object_status[x] != 0u) {
            s->desired_007C[x] = s->desired_0368[x];
            continue;
        }
        /* sub_CFCB */
        flags = s->movement_flags[x];
        if (SP->speed_whole[x] < SP->start_lock[x] ||
            (SP->speed_whole[x] == SP->start_lock[x] && SP->speed_fraction[x] < SP->setup_parameter[x])) {
            s->movement_flags[x] = (uint8_t)(flags | 0x80u);
        } else {
            clearHighFlags(s, x);
        }
        if (raceAiClassifyTerrain(s, x) != 0u) continue;
        if (s->flag_03A6 == 0u) continue;
        /* sub_D000 */
        if (x == 3u) {
            raceSpawnSetupSpeed(SP, x, 0u);
        } else if ((SP->x_low[x] & 0x10u) != 0u && SP->rng[2u + x] >= 0xC0u) {
            raceSpawnSetupSpeed(SP, x, SP->rng[2u + x] == 0xC0u ? 1u : 0u);
        }
        if (SP->track_finished != 0u) continue;
        aiLaneChange(s, x);
    }
}
