#include <string.h>
#include "race_sprites.h"

static const uint8_t s_oam_base[4] = {0x04u, 0x28u, 0x4Cu, 0x70u};
static const uint8_t s_bike_palette[4] = {0x00u, 0x01u, 0x01u, 0x02u};
static const uint8_t s_pose_tile[0x17] = {
    0x58u, 0x60u, 0x69u, 0x71u, 0x79u, 0x80u, 0x0Au, 0x18u,
    0x20u, 0x28u, 0x30u, 0x38u, 0x40u, 0x48u, 0x50u, 0x50u,
    0x97u, 0x90u, 0x89u, 0x11u, 0xA6u, 0x9Fu, 0xB3u
};
static const uint8_t s_pose_mask[0x17] = {
    0x01u, 0x00u, 0x02u, 0x02u, 0x03u, 0x00u, 0x09u, 0x08u,
    0x08u, 0x10u, 0x10u, 0x10u, 0x02u, 0x10u, 0x10u, 0x10u,
    0x01u, 0x09u, 0x09u, 0x09u, 0x01u, 0x09u, 0x03u
};
static const uint8_t s_overlay_tile[3] = {0xBFu, 0xC3u, 0xC6u};

static void writeD1C7(uint8_t *oam, unsigned int base,
                      uint8_t x, uint8_t y, uint8_t tile,
                      uint8_t mask, uint8_t shape, uint8_t palette)
{
    const unsigned int columns = shape & 0x0Fu;
    const unsigned int rows = shape >> 4;
    const unsigned int total = columns * rows;
    unsigned int i, offset = 1u;

    /* D1C7 first pass: X starts at zero after the rows*columns multiply. */
    for (i = 0u; i < total; ++i) {
        uint8_t output_tile;
        if ((i & 1u) == 0u) {
            const uint8_t carry = (uint8_t)(mask & 1u);
            mask >>= 1;
            output_tile = carry != 0u ? 0xFCu : tile++;
        } else {
            output_tile = tile++;
        }
        oam[base + offset] = output_tile;
        oam[base + offset + 1u] = palette;
        offset += 4u;
    }

    /* D1C7 second pass: row-major OAM order within each X column. */
    offset = 0u;
    for (i = 0u; i < columns; ++i) {
        unsigned int row;
        uint8_t sprite_y = y;
        for (row = 0u; row < rows; ++row) {
            oam[base + offset] = sprite_y;
            oam[base + offset + 3u] = x;
            sprite_y = (uint8_t)(sprite_y + 8u);
            offset += 4u;
        }
        x = (uint8_t)(x + 8u);
    }
}

static uint8_t selectPose(const BikeMovementState *movement,
                          const BikeStateMachineState *states,
                          unsigned int bike, uint8_t *temp)
{
    const uint8_t state = states->state[bike];
    const uint8_t approach = movement->approach[bike];

    *temp = 0u;
    if (state != 0u && state != 0x05u) {
        uint8_t pose = (movement->vertical_reference[bike] != 0u ||
                        states->rider_present[bike] == 0u) ? 0x02u : 0x06u;
        if (states->timer_recovery[bike] != 0u) {
            pose = (uint8_t)(states->timer_recovery[bike] - 1u);
        }
        return (uint8_t)(pose | 0x10u);
    }

    if (movement->track_finished == 1u && approach == 0x0Au) {
        *temp = 0x02u;
        return approach; /* LDY/STY changes temp, A still holds $0A. */
    }

    if ((uint8_t)(movement->collision_state[bike] |
                  movement->object_status[bike]) != 0u) {
        return approach;
    }

    if (movement->object_action_state[bike] != 0u) {
        if (movement->object_action_state[bike] != 0x0Du ||
            movement->object_y[bike] < 0x20u) {
            return approach;
        }
    }

    if ((uint8_t)(movement->speed_fraction[bike] |
                  movement->speed_whole[bike]) == 0u) {
        *temp = 0x01u;
        return approach;
    }

    if (approach != 0x06u) return approach;
    if (states->vertical_delta[bike] != 0u) {
        return (states->vertical_delta[bike] & 0x80u) != 0u ? 0x15u : 0x14u;
    }
    if ((states->frame_counter & 0x02u) != 0u) return 0x13u;
    return approach;
}

static void writeTempOverlay(uint8_t *oam, unsigned int base, uint8_t temp)
{
    unsigned int y;
    uint8_t tile = 0xB2u;

    if (temp == 0u) return;
    if ((temp & 1u) != 0u) {
        oam[base + 0x15u] = 0xAEu;
        return;
    }

    /* E1E2-E1F1 writes B2,B1,B0,AF and skips OAM tile offset $09. */
    for (y = 0x11u; ; y -= 4u) {
        if (y != 0x09u) {
            oam[base + y] = tile;
            tile = (uint8_t)(tile - 1u);
        }
        if (y < 4u) break;
    }
}

static void writeRiderOverlay(uint8_t *oam, unsigned int base,
                              const BikeMovementState *movement,
                              const BikeStateMachineState *states,
                              unsigned int bike, uint8_t palette)
{
    const uint8_t previous_x = movement->previous_x[bike];
    uint8_t overlay_palette;
    uint8_t mask;
    const uint8_t vertical_state = states->vertical_state[bike];

    /* E26A visibility gates. */
    if (movement->x_high[bike] == 0x02u) return;
    if (previous_x >= 0xF8u || previous_x < movement->x_low[bike]) return;
    if (vertical_state >= 3u) return;

    overlay_palette = (uint8_t)(palette |
        (movement->vertical_phase[bike] == 0u ? 0x00u : 0x20u));
    mask = vertical_state == 0u ? 0u : (uint8_t)(vertical_state ^ 0x03u);
    writeD1C7(oam, base, previous_x,
              (uint8_t)(0xA8u - states->vertical_collision_ref[bike]),
              s_overlay_tile[vertical_state], mask, 0x22u, overlay_palette);
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

    for (i = 0u; i < 4u; ++i) sort_y[i] = movement->object_y[i];
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

    /* E17F writes one sorted object into each fixed D8C0 OAM block. */
    for (i = 0u; i < 4u; ++i) {
        const unsigned int bike = order[i];
        const unsigned int oam_base = s_oam_base[i];
        uint8_t temp, pose;
        uint8_t base = (uint8_t)oam_base;
        uint8_t x, y, shape = 0x33u;
        uint8_t special_base = 0u;

        if (movement->object_active[bike] == 0u) continue;
        pose = selectPose(movement, states, bike, &temp);
        if (pose >= sizeof(s_pose_tile)) {
            unsupported_mask = (uint8_t)(unsupported_mask | (uint8_t)(1u << bike));
            continue;
        }

        x = movement->x_low[bike];
        y = movement->y_low[bike];
        if (pose == 0x16u) {
            special_base = base;
            base = (uint8_t)(base + 0x0Cu);
            y = (uint8_t)(y + 0x08u);
            shape = 0x23u;
        }

        writeD1C7(oam->bytes, base, x, y,
                  s_pose_tile[pose], s_pose_mask[pose], shape,
                  s_bike_palette[bike]);
        writeTempOverlay(oam->bytes, base, temp);

        /* E26A draws the extra rider overlay only for pose $16. */
        if (special_base != 0u) {
            writeRiderOverlay(oam->bytes, special_base, movement, states,
                              bike, s_bike_palette[bike]);
        }
    }

    return unsupported_mask;
}
