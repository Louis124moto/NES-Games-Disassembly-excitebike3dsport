
#include "bike_physics.h"

/* Source tables: bank_FF.asm tbl_D90E and tbl_D8C4. */
static const uint8_t s_state1_distance[5] = { 0x08u, 0x0Au, 0x0Fu, 0x14u, 0x18u };
static const uint8_t s_state2_y[5] = { 0x39u, 0x01u, 0x01u, 0x01u, 0x07u };

static void update_vertical_collision_reference(BikeStateMachineState *s,
                                                 unsigned x)
{
    uint8_t a = (uint8_t)(s->previous_x[x] - s->x[x]);
    uint8_t carry = (uint8_t)(s->previous_x[x] >= s->x[x]);
    a = (uint8_t)(a - 0x08u - (carry == 0u ? 1u : 0u));
    a = (uint8_t)(a << 1);
    {
        unsigned sum = (unsigned)a + s->y[x];
        a = (uint8_t)sum;
        carry = (uint8_t)(sum > 0xFFu);
        sum = (unsigned)a + 0x04u + carry;
        s->vertical_collision_ref[x] = (uint8_t)sum;
    }
}

static void source_clear_zero_page_race_state(BikeStateMachineState *s)
{
    /* loc_CA85 clears $20-$FF, then increments $0049. High-page object
     * arrays ($03xx) and the controller byte at $0014 are not touched. */
    unsigned i;
    for (i = 0u; i < 4u; ++i) {
        s->state[i] = 0u;
        s->x[i] = 0u;
        s->y[i] = 0u;
        s->vertical_delta[i] = 0u;
        s->speed_whole[i] = 0u;
        s->timer_state[i] = 0u;
        s->timer_recovery[i] = 0u;
        s->vertical_offset[i] = 0u;
        s->vertical_collision_ref[i] = 0u;
    }
    s->global_timer_0022 = 0u;
    s->frame_counter = 0u;
    s->frame_phase = 0u;
    s->remembered_controller = 1u;
    s->player_event = 0u;
}

static uint8_t update_one_bike_state(BikeStateMachineState *s, unsigned x)
{
    uint8_t a;
    uint8_t y;

    switch (s->state[x]) {
    case 0u:
        break;

    case 1u:
        if (s->rider_present[x] == 0u) {
            ++s->state[x];
            if (s->state[x] != 0u) s->timer_state[x] = 0x08u;
            break;
        }
        if ((s->frame_counter & 1u) != 0u) break;
        ++s->previous_x[x];
        y = s->target_index[x];
        a = (uint8_t)(s->x[x] + s_state1_distance[y]);
        if (a < s->previous_x[x]) {
            ++s->state[x];
            if (s->state[x] != 0u) s->timer_state[x] = 0x08u;
        }
        break;

    case 2u:
        y = (uint8_t)x;
        if (x != 0u && s->vertical_offset[x] != 0u) y = 4u;
        if (s->y[x] != s_state2_y[y]) {
            s->vertical_delta[x] = s->y[x] < s_state2_y[y] ? 0x01u : 0xFFu;
            break;
        }
        s->vertical_state[x] = 0u;
        s->vertical_delta[x] = 0u;
        ++s->state[x];
        if (x == 0u) s->player_event = 0x02u;
        s->timer_state[x] = 0x08u;
        break;

    case 3u:
        if (s->rider_present[x] == 0u) {
            s->timer_recovery[x] = 0x03u;
            ++s->state[x];
            if (x == 3u) {
                s->opponent_spawn_event = 0u;
                s->rider_present[3] = 0u;
            }
            break;
        }
        if (s->timer_state[x] != 0u) break;
        {
            int to_d9a7 = 0;
            if ((x | s->global_overheat) == 0u) {
                uint8_t old = s->remembered_controller;
                a = (uint8_t)(s->controller_hold & 0xC0u);
                if (a != 0u) {
                    s->remembered_controller = a;
                    if (old == 0u) {
                        /* D99C TYA/BEQ D9A7: A = old = 0, so the DEC at D9A7 runs and
                         * D9AA sees A = 0 whatever ram_003F is. */
                        to_d9a7 = 1;
                    } else {
                        s->remembered_controller = old;
                    }
                } else {
                    s->remembered_controller = 0u;
                }
            }
            if (to_d9a7) {
                --s->previous_x[x];
                a = 0u;
            } else {
                a = (uint8_t)(s->frame_counter & 0x0Fu);
                if (a == 0u) --s->previous_x[x];
            }
        }
        a &= 0x07u;
        if (a == 0u) s->vertical_state[x] ^= 0x01u;
        a = (uint8_t)(s->previous_x[x] - 0x08u);
        if (a >= s->x[x]) break;
        s->timer_recovery[x] = 0x03u;
        ++s->state[x];
        if (x == 3u) {
            s->opponent_spawn_event = 0u;
            s->rider_present[3] = 0u;
        }
        break;

    case 4u:
        if (s->timer_recovery[x] != 0u) break;
        if (x == 0u) {
            if (s->overheat[0] != 0u) {
                s->state[x] = 0u;
                break;
            }
            if (s->global_overheat != 0u) {
                source_clear_zero_page_race_state(s);
                return 1u;
            }
            ++s->state[x];
            break;
        }
        if (s->global_timer_0022 != 0u) break;
        s->vertical_delta[x] = 0x01u;
        s->global_timer_0022 = 0x18u;
        ++s->state[x];
        break;

    case 5u:
        if ((s->speed_whole[x] | s->vertical_delta[x]) != 0u) {
            s->vertical_delta[x] = 0x01u;
            if (x == 0u || s->y[x] >= 0x08u) s->state[x] = 0u;
            break;
        }
        if (s->y[x] == s_state2_y[x]) break;
        s->vertical_delta[x] = 0x01u;
        if (x == 0u || s->y[x] >= 0x08u) s->state[x] = 0u;
        break;

    default:
        /* The original jump-table reads beyond its six entries for invalid
         * states. Native callers must validate state during setup instead of
         * reproducing an arbitrary indirect jump. */
        break;
    }
    return 0u;
}

void bikePhysicsUpdateBikeStates(BikeStateMachineState *state)
{
    int x;
    if (state == 0) return;
    for (x = 3; x >= 0; --x) {
        if (update_one_bike_state(state, (unsigned)x) == 0u) {
            update_vertical_collision_reference(state, (unsigned)x);
        }
    }
}
