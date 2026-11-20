#include <string.h>
#include "race_sprites.h"

static const uint8_t s_oam_base[4] = {0x04u, 0x28u, 0x4Cu, 0x70u};
static const uint8_t s_bike_palette[4] = {0x00u, 0x01u, 0x01u, 0x02u};
static const uint8_t s_pose6_tiles[9] = {
    0xFCu, 0x0Au, 0x0Bu,
    0x0Cu, 0x0Du, 0xAEu,
    0xFCu, 0x0Fu, 0x10u
};

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
        unsigned int sprite;

        if (movement->object_active[bike] == 0u) continue;
        if (is_initial_pose6(movement, states, bike) == 0u) {
            unsupported_mask = (uint8_t)(unsupported_mask | (uint8_t)(1u << bike));
            continue;
        }

        for (sprite = 0u; sprite < 9u; ++sprite) {
            const unsigned int column = sprite / 3u;
            const unsigned int row = sprite % 3u;
            const unsigned int oam_offset = base + sprite * 4u;
            oam->bytes[oam_offset + 0u] =
                (uint8_t)(movement->y_low[bike] + row * 8u);
            oam->bytes[oam_offset + 1u] = s_pose6_tiles[sprite];
            oam->bytes[oam_offset + 2u] = s_bike_palette[bike];
            oam->bytes[oam_offset + 3u] =
                (uint8_t)(movement->x_low[bike] + column * 8u);
        }
    }

    return unsupported_mask;
}
