#include <string.h>
#include "race_sprites.h"

static const uint8_t s_oam_base[4] = {0x04u, 0x28u, 0x4Cu, 0x70u};
static const uint8_t s_bike_palette[4] = {0x00u, 0x01u, 0x01u, 0x02u};
static void writeInitialPose6(uint8_t *oam, unsigned int base,
                               uint8_t x, uint8_t y, uint8_t palette)
{
    uint8_t tile = 0x0Au; /* tbl_D892[6] */
    uint8_t mask = 0x09u; /* tbl_D8A9[6] */
    unsigned int sprite_index;
    unsigned int offset = 1u;

    /* D1C7 with shape $33: the mask emits six tile bytes. */
    for (sprite_index = 0u; sprite_index < 6u; ++sprite_index) {
        uint8_t output_tile;
        if ((sprite_index & 1u) != 0u) {
            output_tile = tile++;
        } else {
            const uint8_t carry = (uint8_t)(mask & 1u);
            mask >>= 1;
            output_tile = carry != 0u ? 0xFCu : tile++;
        }
        oam[base + offset] = output_tile;
        oam[base + offset + 1u] = palette;
        offset += 4u;
    }
    /* E1D7-E1F8: the zero-speed path leaves temp=1, so LSR sets carry
     * and the source overwrites the sixth tile with the special AE tile. */
    oam[base + 0x15u] = 0xAEu;

    /* D1C7 coordinate pass: three rows in each of three columns. */
    offset = 0u;
    for (sprite_index = 0u; sprite_index < 3u; ++sprite_index) {
        unsigned int row;
        uint8_t sprite_y = y;
        for (row = 0u; row < 3u; ++row) {
            oam[base + offset] = sprite_y;
            oam[base + offset + 3u] = x;
            sprite_y = (uint8_t)(sprite_y + 8u);
            offset += 4u;
        }
        x = (uint8_t)(x + 8u);
    }
}

static uint8_t is_initial_pose6(const BikeMovementState *movement,
                                const BikeStateMachineState *states,
                                unsigned int bike)
{
    if (states->state[bike] != 0u) return 0u;
    if (movement->track_finished != 0u) return 0u;
    if (movement->collision_state[bike] != 0u) return 0u;
    if (movement->object_status[bike] != 0u) return 0u;
    if (movement->object_action_state[bike] != 0u) return 0u;
    if (movement->speed_fraction[bike] != 0u ||
        movement->speed_whole[bike] != 0u) return 0u;
    if (movement->approach[bike] != 0x06u) return 0u;
    return 1u;
}

uint8_t nativeRaceBuildInitialBikeOam(const BikeMovementState *movement,
                                      const BikeStateMachineState *states,
                                      NativeOamState *oam)
{
    uint8_t order[4] = {0u, 1u, 2u, 3u};
    uint8_t sort_y[4];
    uint8_t unsupported_mask = 0u;
    unsigned int i, pass;

    if (movement == 0 || states == 0 || oam == 0) return 0x0Fu;
    memset(oam->bytes, 0xF8, sizeof(oam->bytes));

    for (i = 0u; i < 4u; ++i) {
        sort_y[i] = movement->object_y[i];
    }
    /* E3F4 substitutes bike 3's vertical collision reference when rider 3
     * is present, then bubble-sorts the four source object indices. */
    if (states->rider_present[3] != 0u)
        sort_y[3] = states->vertical_collision_ref[3];

    for (pass = 3u; pass > 0u; --pass) {
        unsigned int j;
        for (j = 0u; j < pass; ++j) {
            if (sort_y[order[j]] >= sort_y[order[j + 1u]]) {
                const uint8_t tmp = order[j];
                order[j] = order[j + 1u];
                order[j + 1u] = tmp;
            }
        }
    }

    /* E17F iterates slots 3..0. The final OAM bytes are independent of
     * iteration order, so emit the same four sorted slots from 0..3. */
    for (i = 0u; i < 4u; ++i) {
        const unsigned int bike = order[i];
        const unsigned int base = s_oam_base[i];
        if (movement->object_active[bike] == 0u) continue;
        if (is_initial_pose6(movement, states, bike) == 0u) {
            unsupported_mask = (uint8_t)(unsupported_mask | (uint8_t)(1u << bike));
            continue;
        }

        writeInitialPose6(oam->bytes, base,
                          movement->x_low[bike], movement->y_low[bike],
                          s_bike_palette[bike]);
    }

    return unsupported_mask;
}
