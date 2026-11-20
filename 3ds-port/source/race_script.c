#include "race_script.h"

/* Source bytes $E584-$E699; tbl_E55A/tbl_E56F pointers as offsets into it. */
static const uint8_t k_scriptData[0x116] = {
    0x04u, 0x87u, 0x08u, 0x88u, 0x08u, 0x44u, 0x29u, 0x06u, 0x29u, 0x00u, 0x31u, 0x85u,
    0x35u, 0x84u, 0x35u, 0x45u, 0x51u, 0x00u, 0x52u, 0x86u, 0xFFu, 0x04u, 0x07u, 0x04u,
    0x88u, 0x0Cu, 0x86u, 0x10u, 0x83u, 0x14u, 0x85u, 0x16u, 0x86u, 0xFFu, 0x04u, 0x88u,
    0x0Au, 0x89u, 0x0Au, 0x42u, 0x20u, 0x06u, 0x22u, 0x00u, 0x30u, 0x08u, 0x50u, 0x84u,
    0x52u, 0x83u, 0x54u, 0x43u, 0x6Cu, 0x00u, 0xFFu, 0x04u, 0x88u, 0x0Au, 0x89u, 0x0Au,
    0x42u, 0x23u, 0x06u, 0x2Au, 0x00u, 0x2Fu, 0x84u, 0x33u, 0x83u, 0x34u, 0x43u, 0x53u,
    0x00u, 0x55u, 0x86u, 0xFFu, 0x04u, 0x88u, 0x0Au, 0x89u, 0x0Au, 0x42u, 0x17u, 0x06u,
    0x1Au, 0x00u, 0x1Fu, 0x84u, 0x23u, 0x83u, 0x23u, 0x43u, 0x35u, 0x00u, 0x37u, 0x86u,
    0xFFu, 0x04u, 0x88u, 0x0Au, 0x89u, 0x0Au, 0x42u, 0x12u, 0x06u, 0x12u, 0x00u, 0x1Au,
    0x83u, 0x1Bu, 0x43u, 0x24u, 0x00u, 0x26u, 0x86u, 0xFFu, 0x04u, 0x88u, 0x0Au, 0x89u,
    0x0Au, 0x42u, 0x22u, 0x06u, 0x22u, 0x00u, 0x2Au, 0x82u, 0x2Bu, 0x4Au, 0x38u, 0x00u,
    0x3Au, 0x86u, 0xFFu, 0x04u, 0x88u, 0x0Au, 0x8Bu, 0x0Bu, 0x49u, 0x17u, 0x06u, 0x17u,
    0x00u, 0x25u, 0x83u, 0x26u, 0x43u, 0x3Bu, 0x00u, 0x3Du, 0x86u, 0xFFu, 0x04u, 0x8Bu,
    0x08u, 0x49u, 0x18u, 0x06u, 0x18u, 0x00u, 0x1Fu, 0x84u, 0x20u, 0x82u, 0x25u, 0x4Au,
    0x35u, 0x00u, 0x37u, 0x86u, 0xFFu, 0x02u, 0x01u, 0x04u, 0x88u, 0x0Au, 0x89u, 0x0Au,
    0x42u, 0x1Au, 0x06u, 0x1Au, 0x00u, 0xFFu, 0x04u, 0x8Bu, 0x08u, 0x49u, 0x18u, 0x06u,
    0x1Au, 0x00u, 0x1Fu, 0x0Bu, 0xFFu, 0x0Du, 0x0Cu, 0x17u, 0x0Cu, 0x28u, 0x0Cu, 0x2Bu,
    0x0Cu, 0xFFu, 0x04u, 0x8Bu, 0x08u, 0x49u, 0x22u, 0x06u, 0x22u, 0x00u, 0x22u, 0x14u,
    0x30u, 0x0Fu, 0x31u, 0x83u, 0x32u, 0x82u, 0x33u, 0x4Au, 0x4Bu, 0x00u, 0xFFu, 0x00u,
    0x0Bu, 0x18u, 0x06u, 0x18u, 0x84u, 0x1Au, 0x83u, 0x1Au, 0x43u, 0x3Bu, 0x00u, 0x42u,
    0x10u, 0x43u, 0x85u, 0x48u, 0x84u, 0x4Au, 0x83u, 0x4Au, 0x43u, 0x5Au, 0x00u, 0x5Au,
    0x0Eu, 0x5Cu, 0x86u, 0xFFu, 0x0Eu, 0x0Du, 0x0Fu, 0x0Eu, 0xFFu, 0x00u, 0x12u, 0xFFu,
    0x04u, 0x88u, 0x0Au, 0x89u, 0x0Au, 0x42u, 0x1Au, 0x13u, 0x1Au, 0x00u, 0xFFu, 0x1Eu,
    0x86u, 0xFFu
};
static const uint16_t k_scriptStart[21] = {
    0x00u, 0x15u, 0x15u, 0x22u, 0x37u, 0x4Cu, 0x61u,
    0x74u, 0x87u, 0x9Au, 0xADu, 0xBAu, 0xC5u, 0xCEu,
    0xE3u, 0x100u, 0x102u, 0x105u, 0x105u, 0x108u, 0x113u
};
/* tbl_E6AD, 16 source bytes ($E6AD-$E6BC). */
static const uint8_t k_tblE6AD[16] = {
    0x03u, 0x01u, 0x02u, 0x02u, 0x00u, 0x05u, 0x05u, 0x06u, 0x04u, 0x04u, 0x63u, 0xE9u,
    0xE3u, 0xE8u, 0x5Du, 0xE8u
};
/* tbl_C0C1 (deceleration amounts) and tbl_C0CC (zero-speed floor). */
static const uint8_t k_tblC0C1[8] = { 0x38u, 0x0Cu, 0x00u, 0x3Cu, 0x1Cu, 0xC0u, 0x7Fu, 0x06u };
static const uint8_t k_tblC0CC[4] = { 0x01u, 0xB0u, 0x20u, 0x40u };

enum { SCRIPT_OK = 0, SCRIPT_UNSUPPORTED = 1 };


#define M (&s->movement)

static void setBcE4(RaceScriptState *s, unsigned x, uint8_t a)
{
    M->vertical_reference[x] = a;
    s->slot_00E4[x] = a;
}

/* sub_E893: A = position_x - ram_00D0 after seeding 00D4/00B4. */
static uint8_t e893(RaceScriptState *s, unsigned x, uint8_t a)
{
    M->brake_state[x] = a;
    s->slot_00B4[x] = a;
    return (uint8_t)(s->position_x[x] - s->slot_00D0[x]);
}

/* loc_E86F */
static void e86f(RaceScriptState *s, unsigned x, uint8_t a)
{
    M->vertical_reference[x] = (uint8_t)(s->slot_00E4[x] - a);
}

/* sub_CE5C for slot x with Y = amount index. */
static void ce5c(RaceScriptState *s, unsigned x, uint8_t y)
{
    uint8_t before;
    if (M->speed_whole[x] == 0u &&
        M->speed_fraction[x] < k_tblC0CC[(M->object_status[x] >> 1) & 3u]) return;
    before = M->speed_fraction[x];
    M->speed_fraction[x] = (uint8_t)(before - k_tblC0C1[y & 7u]);
    if (before >= k_tblC0C1[y & 7u]) return;
    if (M->speed_whole[x] != 0u) { --M->speed_whole[x]; return; }
    M->speed_fraction[x] = 0u;
}

/* Returns SCRIPT_UNSUPPORTED for source states that are not reproduced. */
static int runHandler(RaceScriptState *s, unsigned x, uint8_t index)
{
    uint8_t a;
    switch (index) {
    case 0x00: /* E963 */
        M->lateral_state[x] = 0u; M->brake_state[x] = 0u; s->slot_00B4[x] = 0u;
        return SCRIPT_OK;
    case 0x01: /* E8E3 -> E8C3 */
        s->slot_00A0[x] = 1u;
        return SCRIPT_OK;
    case 0x02: /* E85D */
        a = e893(s, x, 6u); setBcE4(s, x, a); s->slot_00D8[x] = 0x60u;
        return SCRIPT_OK;
    case 0x03: /* E86A */
        a = e893(s, x, 1u); e86f(s, x, a);
        return SCRIPT_OK;
    case 0x04: /* E845 */
        s->slot_00D8[x] = 0x80u;
        a = e893(s, x, 5u); setBcE4(s, x, (uint8_t)(a >> 1));
        return SCRIPT_OK;
    case 0x05: /* E854 */
        a = e893(s, x, 2u); e86f(s, x, (uint8_t)(a >> 1));
        return SCRIPT_OK;
    case 0x06: { /* E934 */
        uint8_t y = 0u;
        if (M->object_status[x] != 0u) return SCRIPT_OK;
        if (M->speed_whole[x] == 0u) return SCRIPT_OK;
        if (M->speed_whole[x] < 2u && M->speed_fraction[x] < s->slot_00D8[x]) y = 1u;
        M->movement_mode[x] = y;
        if (s->slot_00A0[x] == 2u) bikePhysicsInitializeVerticalJump(M, (uint8_t)x);
        else bikePhysicsInitializeVerticalFall(M, (uint8_t)x);
        return SCRIPT_OK;
    }
    case 0x07: /* E818 */
        if (M->object_status[x] != 0u) return SCRIPT_OK;
        if ((uint8_t)(M->approach[x] - 7u) < 0x80u) return SCRIPT_OK;
        if (M->speed_whole[x] >= 3u ||
            (M->speed_whole[x] == 2u && (M->speed_fraction[x] & 0x80u) != 0u)) {
            M->collision_state[x] = 0xFFu;
        }
        return SCRIPT_OK;
    case 0x08: /* EA8F */
        if (x != 0u) return SCRIPT_OK;
        s->timer_003A = 0x1Du;
        if (s->ram_0057 != 0u) {
            if (s->timer_003C == 0u) {
                unsigned k;
                for (k = 0u; k < 7u; ++k) s->gate_tile[k] = s->cue_tiles_03D9[k];
                return SCRIPT_OK;
            }
        } else {
            s->timer_0032 = 0x10u;
            s->sound_request_00FD = 2u;
            M->track_finished = 1u;
        }
        s->timer_003A = 0u;
        return SCRIPT_OK;
    case 0x09: /* E879 */
        a = e893(s, x, 4u); setBcE4(s, x, (uint8_t)(a << 1));
        if (s->slot_00A0[x] != 0u) setBcE4(s, x, (uint8_t)(M->vertical_reference[x] + 0x10u));
        s->slot_00D8[x] = 0x40u;
        return SCRIPT_OK;
    case 0x0A: /* E89D */
        if (M->vertical_phase[x] == 1u) return SCRIPT_OK;
        a = e893(s, x, 3u); e86f(s, x, (uint8_t)(a << 1));
        if (M->vertical_phase[x] == 4u) {
            bikePhysicsInitializeVerticalJump(M, (uint8_t)x);
            M->vertical_phase[x] = 2u;
            M->rider_state[x] = 2u;
        }
        return SCRIPT_OK;
    case 0x0B: /* E8EE */
        s->slot_00A0[x] = 2u; s->slot_00D8[x] = 1u;
        setBcE4(s, x, 0x30u);
        return SCRIPT_OK;
    case 0x0C: { /* E8FF */
        uint8_t y;
        if (M->object_status[x] != 0u) return SCRIPT_OK;
        if (M->vertical_phase[x] != 0u && (M->vertical_phase[x] & 2u) == 0u) return SCRIPT_OK;
        if (M->speed_whole[x] == 0u) return SCRIPT_OK;
        y = (M->movement_flags[x] & 0x40u) ? 3u : 4u;
        s->slot_036C[x] = 1u;
        ce5c(s, x, (uint8_t)(y + 2u));
        return SCRIPT_OK;
    }
    case 0x0D: /* E956 */
        if (M->vertical_phase[x] != 1u) return SCRIPT_OK;
        M->vertical_reference[x] = 0u;
        bikePhysicsInitializeVerticalJump(M, (uint8_t)x);
        return SCRIPT_OK;
    case 0x0E: /* E8BF */
        M->vertical_phase[x] = 0u; s->slot_00A0[x] = 0u;
        return SCRIPT_OK;
    case 0x0F: /* E8C6 */
        M->vertical_phase[x] = 1u;
        if (M->object_y[x] < 0x20u) M->vertical_phase[x] = 2u;
        return SCRIPT_OK;
    case 0x10: /* E8FA */
        setBcE4(s, x, 0x10u);
        return SCRIPT_OK;
    case 0x12: /* E8D3 */
        if ((x | M->object_status[x] | s->player_overheat_03E0 | s->timer_003C) == 0u) s->ram_03B6 = 8u;
        return SCRIPT_OK;
    case 0x13: /* E8E7 -> DCFE */
        if (M->object_status[x] != 0u) return SCRIPT_OK;
        if (M->collision_state[x] != 0u) { bikePhysicsInitializeVerticalFall(M, (uint8_t)x); return SCRIPT_OK; }
        ++M->speed_whole[x];
        if (M->speed_whole[x] != 0u) bikePhysicsInitializeVerticalFall(M, (uint8_t)x);
        else bikePhysicsInitializeVerticalJump(M, (uint8_t)x);
        return SCRIPT_OK;
    case 0x14: /* E8F5 */
        setBcE4(s, x, 0x30u);
        return SCRIPT_OK;
    default: /* $11 and >= $15 jump into table data */
        return SCRIPT_UNSUPPORTED;
    }
}

/* sub_E73B for slot x. */
static void updateSlot(RaceScriptState *s, unsigned x)
{
    s->position_x[x] = (uint8_t)(s->position_x[x] + M->movement_delta[x]);
    if (M->object_action_state[x] == 0u) {
        uint8_t a = (uint8_t)(s->script_x_high[x] - 0x40u);
        if (a >= 0x80u) return;
        a = (uint8_t)(a >> 2);
        if (a >= 0x16u) return;
        M->object_action_state[x] = (uint8_t)(a + 1u);
        s->script_index[x] = 0u;
        s->position_x[x] = (uint8_t)(M->object_origin_x[x] - 1u);
    }
    for (;;) {
        const unsigned script = M->object_action_state[x];
        unsigned at;
        uint8_t pos, arg;
        if (script < 1u || script > 21u) { s->unsupported_mask |= (uint8_t)(1u << x); return; }
        at = (unsigned)k_scriptStart[script - 1u] + s->script_index[x];
        if (at >= sizeof(k_scriptData)) { s->unsupported_mask |= (uint8_t)(1u << x); return; }
        pos = k_scriptData[at];
        if (pos == 0xFFu) break; /* bra_E7D0 */
        if (pos > s->position_x[x]) return;
        if (at + 1u >= sizeof(k_scriptData)) { s->unsupported_mask |= (uint8_t)(1u << x); return; }
        arg = k_scriptData[at + 1u];
        if ((arg & 0x80u) != 0u) { /* bra_E7A3 */
            if ((M->object_status[x] | M->collision_state[x]) == 0u) {
                const uint8_t nib = (uint8_t)(arg & 0x0Fu);
                if (M->vertical_phase[x] != 1u) {
                    M->approach[x] = nib;
                    if (M->object_action_state[x] != 3u) {
                        if (nib < 2u) { s->unsupported_mask |= (uint8_t)(1u << x); return; }
                        M->brake_state[x] = k_tblE6AD[nib - 2u];
                    }
                }
            }
        } else if ((arg & 0x40u) != 0u) { /* bra_E7F2 */
            M->lateral_state[x] = (uint8_t)(arg & 0x0Fu);
            s->slot_00D0[x] = pos;
            if (pos >= 0x80u) return; /* falls into sub_E7FC, which only returns */
        } else {
            if (runHandler(s, x, arg) != SCRIPT_OK) { s->unsupported_mask |= (uint8_t)(1u << x); return; }
        }
        s->script_index[x] = (uint8_t)(s->script_index[x] + 2u);
        if (s->script_index[x] == 0u) break; /* BNE not taken: falls into bra_E7D0 */
    }
    /* bra_E7D0 */
    M->object_action_state[x] = 0u;
    M->brake_state[x] = 0u;
    if (s->slot_00A0[x] == 0u && M->vertical_phase[x] != 1u) {
        M->vertical_reference[x] = 0u;
        if (M->vertical_phase[x] == 2u) M->vertical_phase[x] = 3u;
    }
    s->slot_036C[x] = 0u;
}

void raceScriptUpdateOpponents(RaceScriptState *s)
{
    int x;
    if (s == 0) return;
    for (x = 3; x >= 0; --x) updateSlot(s, (unsigned)x);
}

void raceScriptDispatchLateral(RaceScriptState *s)
{
    int x;
    if (s == 0) return;
    for (x = 3; x >= 0; --x) {
        if (M->lateral_state[x] != 0u &&
            runHandler(s, (unsigned)x, (uint8_t)(M->lateral_state[x] & 0x7Fu)) != SCRIPT_OK) {
            s->unsupported_mask |= (uint8_t)(1u << x);
        }
    }
}
