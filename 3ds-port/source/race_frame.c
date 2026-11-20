#include "race_frame.h"
#include "nes_routines.h"

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
