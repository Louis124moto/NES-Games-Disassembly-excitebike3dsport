#include "race_frame.h"
#include "nes_routines.h"
#include "race_spawn.h"

/* Source: sub_DDD1 ($DDD1-$DE30). X runs 3..0. */
void raceCrashRecovery(RaceCrashRecoveryState *s)
{
    int x;
    if (s == 0) return;

    for (x = 3; x >= 0; --x) {
        uint8_t y;
        uint8_t a;

        if (s->collision_state[x] == 0u) continue;

        y = 4u;
        if (x == 0) {
            s->player_crashed_03A9 = 0u;
            s->sound_request_00FC = 0u;
            if (s->frame_phase_004C == 0u) s->sound_request_00FD = 1u;
            y = (uint8_t)(s->random_001B & 3u);
        }
        s->crash_variant[x] = y;

        if ((s->frame_phase_004C & 1u) != 0u) continue;

        if (s->speed_whole[x] == 0u) {
            a = (uint8_t)(s->object_status[x] | s->inactive[x]);
            if (a == 0u) {
                s->speed_fraction[x] = 0u;
                s->movement_delta[x] = 0u;
                s->collision_state[x] = 0u;
                s->approach[x] = 6u;
                s->previous_x[x] = (uint8_t)(s->x_low[x] + 8u);
                s->derived_y[x] = nesSubDA15DerivedY(s->previous_x[x], s->x_low[x], s->y_low[x]);
                s->recovery_flag[x] = 2u;
                s->movement_state[x] = 1u;
                continue;
            }
            s->speed_fraction[x] = 0x88u; /* bra_DE20 */
        }
        s->approach[x] = (uint8_t)((s->approach[x] + s->collision_state[x]) & 0x0Fu);
    }
}

/* Source: sub_DEBB ($DEBB-$DEE1). X runs 3..1. */
void raceOpponentCue(RaceOpponentCueState *s)
{
    int x;
    if (s == 0) return;
    for (x = 3; x >= 1; --x) {
        uint8_t value;
        if ((s->x_high[x] & 1u) == 0u) continue;
        if (s->x_low[x] >= 0x30u && s->x_low[x] < 0x40u) {
            value = 1u; /* bra_DED9 */
        } else {
            if (s->cue_flag[x - 1] == 0u) continue;
            s->sound_request_00FF = 0x40u;
            value = 0u;
        }
        s->cue_flag[x - 1] = value;
    }
}

/* Source: sub_DEE2 ($DEE2-$DF2F). */
void raceEngineSound(RaceEngineSoundState *s)
{
    uint8_t x = 0u;
    if (s == 0) return;
    if (s->player_crashed_03A9 != 0u) {
        uint8_t hi, lo, a, divisor, quotient, remainder;
        int i;
        if ((s->frame_phase_004C & 1u) != 0u) return; /* BCS to RTS: $00FC untouched */
        hi = s->speed_whole;
        lo = s->speed_fraction;
        for (i = 0; i < 3; ++i) {
            lo = (uint8_t)((lo >> 1) | ((hi & 1u) << 7));
            hi = (uint8_t)(hi >> 1);
        }
        divisor = s->object_status != 0u ? 4u : 2u;
        a = (uint8_t)(lo - 2u);
        if ((a & 0x80u) != 0u) {
            x = 1u;
        } else {
            nesSubDFA4(a, divisor, &quotient, &remainder);
            x = quotient != 0u ? quotient : 1u;
        }
        if (x >= 0x2Fu) {
            x = 0x2Fu;
            if ((s->frame_counter_003F & 8u) == 0u) x = 0x2Eu;
        }
        if ((s->controller & 0x80u) == 0u) x = (uint8_t)(x | 0x80u);
    }
    s->sound_request_00FC = x;
}

static const uint8_t k_tblD8E7[8] = { 0x9Eu, 0xA6u, 0x92u, 0x9Au, 0x86u, 0x8Eu, 0x7Au, 0x82u };

/* Source: sub_DE31 ($DE31-$DEBA). */
void raceStartBanner(RaceStartBannerState *s)
{
    uint8_t timer, tile_base, sprite_x;
    int y;
    if (s == 0) return;

    timer = s->timer_0024;
    if (timer == 1u) { /* bra_DEA6 */
        s->race_started_004F = 1u;
        {
            const uint8_t before = s->banner_x_03B4;
            s->banner_x_03B4 = (uint8_t)(before - s->player_delta_0060);
            if (before < s->player_delta_0060) return; /* BCC: borrow */
        }
        s->timer_0024 = 2u;
        timer = 2u;
    } else {
        if (s->race_started_004F != 0u) return;
        if (timer < 0x60u && s->player_crashed_03A9 != 0u &&
            (s->controller_005C & 0xC0u) != 0u) {
            s->sound_request_00FE = 1u;
        }
    }

    /* bra_DE4E */
    tile_base = 0xF0u;
    if (timer < 0x10u) tile_base = timer >= 0x08u ? 0xF2u : 0xF4u;
    if (timer == 0x72u) s->sound_request_00FB = 0x20u;
    sprite_x = s->banner_x_03B4;

    for (y = 7; y >= 0; --y) {
        RaceSprite *spr = &s->banner[y];
        spr->y = k_tblD8E7[y];
        spr->tile = tile_base;
        spr->attribute = 0u;
        spr->x = sprite_x;
        if ((y & 1) != 0) {
            spr->tile = (uint8_t)(spr->tile + 1u);
        } else {
            const uint8_t before = sprite_x;
            sprite_x = (uint8_t)(sprite_x - 0x10u);
            if (before < 0x10u) break; /* BCC bra_DE9D: borrow ends the loop */
        }
    }
    if (s->timer_0034 != 0u) s->timer_0024 = 0x88u; /* bra_DE9D */
}


static const uint8_t k_tblE69A[19] = { 0xCBu, 0xCDu, 0xCCu, 0xCEu, 0xCBu, 0xCBu, 0xCBu, 0x22u, 0x64u, 0x62u, 0xC0u, 0x22u, 0x74u, 0x63u, 0xE0u, 0x21u, 0x83u, 0xFCu, 0xC0u };

/* sub_DC9A: A - ram_00BC - ram_obj_pos_Y_lo with the SBC borrow chain. */
static uint8_t screenY(uint8_t a, uint8_t reference, uint8_t y)
{
    const unsigned first = (unsigned)a - reference;
    const unsigned borrow = (a < reference) ? 1u : 0u;
    return (uint8_t)(first - y - borrow);
}

/* sub_E2F6 for slot x (Y = 8 * x). */
static void effectDust(RaceEffectSpriteState *s, unsigned x)
{
    RaceSprite *a = &s->sprite[2u * x];
    RaceSprite *b = &s->sprite[2u * x + 1u];
    uint8_t px;
    if (s->dust_flags[x] == 0u) return;
    if ((s->speed_whole[x] | s->speed_fraction[x]) == 0u) return;
    if (s->vertical_reference[x] != 0u) return;
    if ((s->x_high[x] & 1u) == 0u) return;
    if (s->x_low[x] < 0x10u) return;
    px = (uint8_t)(s->x_low[x] + 0xF9u);
    a->x = px;
    b->x = (uint8_t)(px + 0xF8u);
    b->y = (uint8_t)(s->y_low[x] + 8u);
    a->y = (uint8_t)(b->y + 8u);
    a->tile = 0xF6u; b->tile = 0xF7u;
    if ((s->frame_counter_003F & 4u) != 0u) { a->tile = 0xF7u; b->tile = 0xF6u; }
    a->attribute = b->attribute = (uint8_t)(s->dust_flags[x] & 2u);
    if (s->speed_whole[x] == 0u) b->y = 0xF8u;
}

void raceEffectSprites(RaceEffectSpriteState *s)
{
    int x;
    if (s == 0) return;
    for (x = 3; x >= 0; --x) {
        int marker;
        if ((s->x_high[x] & 1u) != 0u) {
            marker = s->x_low[x] < 0xF4u;
        } else {
            marker = (s->x_high[x] & 2u) == 0u && s->x_low[x] >= 0xF6u;
        }
        if (marker && s->object_status[x] != 0u) {
            RaceSprite *a = &s->sprite[2 * x];
            a->y = screenY(0xB6u, s->vertical_reference[x], s->object_y[x]);
            a->x = (uint8_t)(s->x_low[x] + 0x0Au);
            a->tile = k_tblE69A[s->marker_index[x] % 19u];
            a->attribute = 0u;
        } else {
            effectDust(s, (unsigned)x);
        }
    }
}

/* sub_E52C: nonzero (Z clear) when the vertical difference is in the overlap band. */
static uint8_t overlapDiff(const RaceCollisionState *s, unsigned x, unsigned y)
{
    const uint8_t diff = (uint8_t)(s->y_low[x] - s->y_low[y]);
    /* The caller tests the Z flag left by the last instruction: CMP #$EC
     * leaves Z set only for a difference of exactly $EC, and CMP #$12 on the
     * BCC path never leaves Z set. */
    if (diff >= 0xECu) return diff == 0xECu ? 0u : 1u;
    if (diff < 0x12u) return 1u;
    return 0u;
}

void raceBikeCollision(RaceCollisionState *s)
{
    int x;
    if (s == 0) return;
    for (x = 1; x >= 0; --x) {
        unsigned count = 0u, y, history = 0u;
        const uint8_t hi = (uint8_t)(s->x_high[x] >> 1);
        if ((s->x_high[x] & 1u) == 0u) {
            if (hi != 0u) continue;
            if (s->x_low[x] < 0xF8u) continue;
        }
        for (y = (unsigned)x + 1u; y < 4u; ++y) {
            int near_edge = (s->x_high[y] & 1u) != 0u;
            if (!near_edge) {
                if ((s->x_high[y] >> 1) != 0u) continue;
                if (s->x_low[x] < 0xF8u) continue;
            }
            if (overlapDiff(s, (unsigned)x, y) != 0u) {
                ++count;
                history = ((history << 2) | y) & 0xFFu;
            }
        }
        if (count < 2u) continue;
        if (count == 2u) {
            if (overlapDiff(s, (history >> 2) & 3u, history & 3u) == 0u) continue;
        }
        s->collision_flag_03A3 = 1u;
        return;
    }
    s->collision_flag_03A3 = 0u;
}

static const uint8_t k_tblD8C0[4] = { 0x04u, 0x28u, 0x4Cu, 0x70u };
static const uint8_t k_tblD8DC[8] = { 0x00u, 0x0Cu, 0x18u, 0xA0u, 0x40u, 0x40u, 0xEDu, 0x21u };

void raceHideCollidedSprites(RaceHideSpritesState *s)
{
    int x;
    if (s == 0 || s->toggle_03B3 > 7u) return;
    if (s->collision_flag_03A3 == 0u) return;
    for (x = 3; x >= 0; --x) {
        const uint8_t v = s->slot_active_0088[x];
        unsigned base, k;
        if (v == 0u) continue;
        if (v == 3u && s->rider_present_0373 != 0u) continue;
        base = (unsigned)((k_tblD8C0[x] + k_tblD8DC[s->toggle_03B3]) & 0xFFu);
        for (k = 0u; k < 0x0Cu; k += 4u) s->oam.bytes[(0x200u + base + k) & 0xFFu] = 0xF8u;
    }
    s->toggle_03B3 ^= 2u;
}

/* Source: sub_E456 + sub_E49B + sub_E4AD. The three temporary bytes
 * begin as copies of x_high and receive the original threshold increments.
 * Each clear low bit hides one four-sprite group at D8C0[slot] + D8DC[group].
 */
static const uint8_t k_offscreen_oam_base[4] = { 0x04u, 0x28u, 0x4Cu, 0x70u };
static const uint8_t k_offscreen_sprite_offset[3] = { 0x00u, 0x0Cu, 0x18u };

void raceHideOffscreenOpponentSprites(RaceOffscreenSpriteState *s,
                                      RaceSpawnState *opponents)
{
    int slot;
    if (s == 0 || opponents == 0) return;

    for (slot = 3; slot >= 0; --slot) {
        const uint8_t object = s->slot_object_index[slot];
        uint8_t hide_mask[3];
        unsigned int group;

        if (object == 0u) continue;
        /* The source call sites use opponent slots 1..3. Reject malformed
         * semantic mappings rather than indexing outside the native arrays.
         */
        if (object >= 4u) continue;

        hide_mask[0] = opponents->x_high[object];
        hide_mask[1] = opponents->x_high[object];
        hide_mask[2] = opponents->x_high[object];

        {
            const uint8_t x = opponents->x_low[object];

            /* Only the $70-$8F opponent-clear window is gated by the
             * high-byte low bit (the carry from LSR at E46D). */
            if ((hide_mask[0] & 1u) == 0u && x >= 0x70u && x < 0x90u) {
                raceSpawnClearSlot(opponents, object); /* sub_DB95 */
                s->opponent_cleared_03A6 = 1u;
                goto hide_groups;
            }

            /* E484 is reached for BOTH carry paths. Its F8/F0 threshold
             * increments must therefore run even when x_high is odd.
             */
            if (x >= 0xF8u) hide_mask[1] = (uint8_t)(hide_mask[1] + 1u);
            if (x >= 0xF0u) hide_mask[2] = (uint8_t)(hide_mask[2] + 1u);
        }

hide_groups:
        for (group = 0u; group < 3u; ++group) {
            unsigned int sprite;
            const uint8_t base = (uint8_t)(k_offscreen_oam_base[slot] +
                                           k_offscreen_sprite_offset[group]);
            if ((hide_mask[group] & 1u) != 0u) continue;
            for (sprite = 0u; sprite < 3u; ++sprite) {
                const uint8_t offset = (uint8_t)(base + sprite * 4u);
                s->oam.bytes[offset] = 0xF8u;
            }
        }
    }
}
