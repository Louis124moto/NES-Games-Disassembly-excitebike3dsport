#include "race_spawn.h"

static const uint8_t k_tblD8D8[4] = { 0x0Eu, 0x1Au, 0x26u, 0x32u };
static const uint8_t k_tblD8DF[3] = { 0xA0u, 0x40u, 0x40u };
static const uint8_t k_tblD8E2[3] = { 0xEDu, 0x21u, 0x21u };
static const uint8_t k_tblD8D5[4] = { 0x02u, 0x01u, 0x01u, 0x0Eu };
static const uint8_t k_tblD8CD[16] = { 0x00u, 0x11u, 0x22u, 0x33u, 0x44u, 0x66u, 0x88u, 0xAAu, 0x02u, 0x01u, 0x01u, 0x0Eu, 0x1Au, 0x26u, 0x32u, 0x00u };

void raceSpawnClearSlot(RaceSpawnState *s, unsigned x)
{
    s->active[x] = 0u; s->y_low[x] = 0u; s->vertical_reference[x] = 0u;
    s->slot_00A0[x] = 0u; s->vertical_phase[x] = 0u; s->action_state[x] = 0u;
    s->lateral_state[x] = 0u; s->dust_flags[x] = 0u; s->speed_fraction[x] = 0u;
    s->speed_whole[x] = 0u; s->collision_state[x] = 0u; s->movement_state[x] = 0u;
    s->object_status[x] = 0u; s->brake_state[x] = 0u; s->movement_mode[x] = 0u;
    s->script_x_high[x] = 0u; s->block_039C[x] = 0u; s->lane_delta[x] = 0u;
    s->position_0070[x] = 0u; s->rider_present[x] = 0u;
}

void raceSpawnSetupSpeed(RaceSpawnState *s, unsigned x, unsigned y)
{
    uint8_t a;
    if (s->track_finished != 0u) { a = 3u; goto store; }
    if (x == 3u) { /* bra_DB6C */
        if ((s->x_high[x] & 2u) != 0u) goto table;
        if (s->speed_whole[0] == 0u) goto table;
        s->start_lock[x] = s->speed_whole[0];
        s->setup_parameter[x] = s->speed_fraction[0];
        if (s->x_high[x] != 0u) {
            if (s->x_low[x] >= 0x40u) goto table;
            if (s->position_0070[0] == s->position_0070[3]) return;
        }
        if (s->start_lock[x] < 4u) ++s->start_lock[x];
        return;
    }
table:
    a = k_tblD8D5[y & 3u];
store:
    s->start_lock[x] = a;
    s->setup_parameter[x] = k_tblD8CD[s->rng[3u + x] & 0x0Fu];
}

/* sub_E7FF: byte of course page `page` at the object class index. */
static uint8_t trackLookup(const RaceSpawnState *s, unsigned page, unsigned x)
{
    return s->track_pages[page][s->object_class[x] & 0x3Fu];
}

uint8_t raceSpawnPlace(RaceSpawnState *s, unsigned x, unsigned y)
{
    unsigned xh;
    raceSpawnClearSlot(s, x);
    s->y_low[x] = k_tblD8D8[(y - 1u) & 3u];
    xh = s->x_high[x];
    if (xh > 2u) return 0u; /* outside the reproduced domain */
    s->x_low[x] = k_tblD8DF[xh];
    s->approach[x] = 6u;
    s->object_class[x] = (uint8_t)((s->object_class[0] + k_tblD8E2[xh]) & 0x3Fu);
    s->origin_x[x] = s->origin_x[0];
    if (xh != 0u) {
        uint8_t a = trackLookup(s, 5u, x);
        if (a != 0xE4u) {
            if (a < 0x3Bu || a >= 0x93u) return 0u; /* bra_DB4D */
            if (trackLookup(s, 2u, x) == 0xE6u) {
                if (s->y_low[x] >= 0x20u) {
                    s->vertical_reference[x] = 0x30u;
                    s->vertical_phase[x] = 1u;
                } else {
                    s->vertical_phase[x] = 3u;
                }
            }
        }
    }
    raceSpawnSetupSpeed(s, x, s->x_high[x]);
    s->speed_whole[x] = (uint8_t)((s->x_high[x] ^ 2u) << 1);
    s->block_039C[x - 1u] = 0x20u;
    s->timer_0023 = 0x10u;
    s->active[x] = 1u;
    return 1u;
}

void raceSpawnUpdate(RaceSpawnState *s)
{
    int x = 3;
    if (s == 0) return;
    if (s->timer_003B == 0u) goto body;
    goto next;
    for (;;) {
body:
        {
            const uint8_t gate = (uint8_t)(((unsigned)x ^ s->frame_phase) | s->active[x] | s->timer_0023 |
                                           s->rider_present[3] | s->spawn_request_03A8);
            if (gate == 0u && s->rng[x] >= 0xA0u) {
                unsigned y = (unsigned)(s->rng[x] & 3u) + 1u, k;
                s->x_high[x] = (uint8_t)(s->speed_whole[0] & 2u);
                for (k = 3u; k >= 1u; --k) {
                    if (s->position_0070[k] == y) {
                        ++y;
                        if (y >= 5u) y = 1u;
                    }
                }
                (void)raceSpawnPlace(s, (unsigned)x, y);
            }
        }
next:
        --x;
        if (x == 0) return;
    }
}

void raceSpawnHandleSlot3(RaceSpawnState *s)
{
    if (s == 0 || s->spawn_request_03A8 == 0u) return;
    if (s->active[3] != 0u) {
        if ((s->x_high[3] & 1u) != 0u) return;
        if ((s->x_high[3] & 2u) == 0u && s->x_low[3] >= 0xEAu) return;
        raceSpawnClearSlot(s, 3u);
        return;
    }
    s->spawn_request_03A8 = 0u;
    s->x_high[3] = 2u;
    if (raceSpawnPlace(s, 3u, 1u) == 0u) return;
    s->speed_whole[3] = 0u;
    s->screen_y[3] = 0x92u;
    s->rider_present[3] = 0x92u;
    s->collision_state[3] = 0xFFu; /* loc_DC69 */
}
