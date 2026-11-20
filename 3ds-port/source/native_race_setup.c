#include <string.h>
#include "native_race_setup.h"

/* bank_FF.asm: tbl_C0B0, tbl_C0B4, tbl_C0B8 and ofs_001_C875_02_06. */
static const uint8_t s_object_class[4] = { 0x09u, 0x0Bu, 0x0Du, 0x0Fu };
static const uint8_t s_start_y[4] = { 0x0Eu, 0x1Au, 0x26u, 0x32u };
static const uint8_t s_start_x[4] = { 0x38u, 0x48u, 0x58u, 0x68u };
/* tbl_C091 at offsets 0 and $10: sub_CB13 for course index zero. */
static const uint8_t s_course0_parameters[2][3] = {
    { 0x01u, 0x10u, 0x00u },
    { 0x01u, 0x18u, 0x00u }
};

/* Source sub_DFA2: unsigned 8-bit divide by ten, returning quotient/remainder. */
static void split_decimal(uint8_t value, uint8_t *quotient, uint8_t *remainder)
{
    *quotient = (uint8_t)(value / 10u);
    *remainder = (uint8_t)(value % 10u);
}

/* Source C90E-C91D then DF7F: create the seven glyph bytes consumed by C935. */
static void build_first_course_hud_glyphs(NativeRaceSetupState *out,
                                          uint8_t transition_counter)
{
    uint8_t first = out->course_setup_parameters[0];
    uint8_t second = (uint8_t)((uint8_t)(transition_counter << 1) +
                               out->course_setup_parameters[1]);
    uint8_t third = 0u;
    uint8_t quotient;
    uint8_t remainder;

    /* CMP #$3C / SBC #$3C / INC $0005: one source-defined carry step. */
    if (second >= 0x3Cu) {
        second = (uint8_t)(second - 0x3Cu);
        first = (uint8_t)(first + 1u);
    }

    split_decimal(first, &quotient, &remainder);
    out->hud_glyphs[0] = remainder; /* DFA2 stores only A for the first value. */
    out->hud_glyphs[1] = 0xFBu;      /* C935 overwrites the second copied byte with the $FB marker. */

    split_decimal(second, &quotient, &remainder);
    out->hud_glyphs[2] = quotient;
    out->hud_glyphs[3] = remainder;
    out->hud_glyphs[4] = 0xFBu;      /* C935 overwrites the fifth copied byte with the $FB marker. */

    split_decimal(third, &quotient, &remainder);
    out->hud_glyphs[5] = quotient;
    out->hud_glyphs[6] = remainder;
}

int nativeRaceSetupFirstCourse(NativeRaceSetupState *out,
                               const NativeRaceSetupInput *in,
                               const NativeRngState *rng,
                               BikeMovementState *movement,
                               BikeStateMachineState *states)
{
    unsigned int i;
    unsigned int last;
    unsigned int half_lock_slot;

    if (out == 0 || in == 0 || rng == 0 || movement == 0 || states == 0)
        return 0;
    /* C905 skips sub_C53A only while ram_0041 < $08. The first-course
     * selector path enters this setup list with ram_0041 = $02. */
    if (in->setup_index_0041 >= 0x08u)
        return 0;

    memset(out, 0, sizeof(*out));
    out->setup_counter_0040 = (uint8_t)(in->setup_counter_0040 + 1u);
    out->setup_counter_0047 = (uint8_t)(in->setup_counter_0047 + 1u);
    out->timer_0034 = in->timer_0034 != 0u ? in->timer_0034 : 0x05u;
    out->global_vertical_reference = 0xA0u;
    out->player_slot = 0u;
    out->player_rider_present = 1u;
    out->frame_timer_00E8 = 0x34u;
    out->frame_timer_00E9 = 0x14u;
    out->countdown_x_03B4 = 0x80u;
    out->race_setup_state_03B0 = 0x80u;
    out->countdown_limit_03BD = 0xFFu;
    out->countdown_index_03BE = 0x02u;
    memcpy(out->course_setup_parameters,
           s_course0_parameters[in->transition_flag_0046 != 0u ? 1u : 0u],
           sizeof(out->course_setup_parameters));
    /* C92C/C53E: CB06 selects $0580+$23 for ram_03F2=1; C1FD copied
     * the same tbl_C091 bytes to $05A3, so the first-course triplet matches. */
    memcpy(out->race_object_parameters,
           s_course0_parameters[in->transition_flag_0046 != 0u ? 1u : 0u],
           sizeof(out->race_object_parameters));
    build_first_course_hud_glyphs(out, in->transition_counter_03F8);

    /* C8A5 selects X=3..0 when ram_03F2 != 0, otherwise only X=0. */
    last = in->multiplayer_flag != 0u ? 3u : 0u;
    for (i = 0u; i <= last; ++i) {
        out->bike[i].y = s_start_y[i];
        out->bike[i].x_low = s_start_x[i];
        out->bike[i].x_high = 0x01u;
        out->bike[i].object_class = s_object_class[i];
        out->bike[i].approach = 0x06u;
        out->bike[i].lock_timer = 0x06u;
        out->bike[i].active = 0x01u;
        out->bike[i].object_origin_x = 0x08u;
        out->bike[i].setup_parameter = (uint8_t)(rng->bytes[i + 1u] >> 1);

        movement->object_y[i] = s_start_y[i];
        movement->x_low[i] = s_start_x[i];
        movement->x_high[i] = 0x01u;
        movement->object_class[i] = s_object_class[i];
        movement->object_origin_x[i] = 0x08u;
        movement->setup_parameter[i] = (uint8_t)(rng->bytes[i + 1u] >> 1);
        movement->approach[i] = 0x06u;
        movement->start_lock_timer[i] = 0x06u;
        movement->object_active[i] = 0x01u;

        states->y[i] = s_start_y[i];
    }

    /* C8D3: X wrapped to $FF, so ram_0019,X reads zero-page $0018. */
    half_lock_slot = (unsigned int)(rng->bytes[0] & 0x03u);
    if (half_lock_slot == 0u)
        half_lock_slot = 2u;
    out->half_lock_slot = (uint8_t)half_lock_slot;
    out->bike[half_lock_slot].lock_timer = movement->start_lock_timer[half_lock_slot];
    out->bike[half_lock_slot].lock_timer >>= 1;
    movement->start_lock_timer[half_lock_slot] >>= 1;

    /* C89B-C8A1 writes only the player rider-presence byte ($0370). */
    states->rider_present[0] = 0x01u;
    movement->global_vertical_reference = 0xA0u;
    return 1;
}
