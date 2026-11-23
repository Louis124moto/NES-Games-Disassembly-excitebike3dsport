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
    /* DFA2 subtracts ten until negative, then adds ten back with carry clear;
     * this returns the ordinary base-10 remainder. */
    *remainder = (uint8_t)(value % 10u);
}

/* C935 runs twice during C875. The first output from C90E is transient;
 * C92C then reloads the final three values through C53E and C935 overwrites
 * all seven HUD bytes. Model that final, observable output only. */
static void build_first_course_hud_glyphs(NativeRaceSetupState *out)
{
    uint8_t quotient;
    uint8_t remainder;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        split_decimal(out->race_object_parameters[i], &quotient, &remainder);
        if (i == 0u) {
            out->hud_glyphs[0] = remainder;
            out->hud_glyphs[1] = 0xFBu;
        } else if (i == 1u) {
            out->hud_glyphs[2] = quotient;
            out->hud_glyphs[3] = remainder;
            out->hud_glyphs[4] = 0xFBu;
        } else {
            out->hud_glyphs[5] = quotient;
            out->hud_glyphs[6] = remainder;
        }
    }
}

void nativeRaceSetupClearFirstCourseState(BikeMovementState *movement,
                                          BikeStateMachineState *states)
{
    if (movement == 0 || states == 0) return;

    /* C869 clears $004D-$00EF; C820 clears $0340-$03EF. */
#define CLEAR_FIELD(object, field) memset((object)->field, 0, sizeof((object)->field))
    CLEAR_FIELD(movement, speed_whole);
    CLEAR_FIELD(movement, speed_fraction);
    CLEAR_FIELD(movement, fraction_accumulator);
    CLEAR_FIELD(movement, movement_delta);
    CLEAR_FIELD(movement, object_active);
    CLEAR_FIELD(movement, object_action_state);
    CLEAR_FIELD(movement, vertical_phase);
    CLEAR_FIELD(movement, object_class);
    CLEAR_FIELD(movement, object_origin_x);
    CLEAR_FIELD(movement, setup_parameter);
    CLEAR_FIELD(movement, x_low);
    CLEAR_FIELD(movement, x_high);
    CLEAR_FIELD(movement, previous_x);
    movement->base_nametable = 0u;
    movement->secondary_scroll_fraction = 0u;
    movement->secondary_scroll_x = 0u;
    movement->secondary_base_nametable = 0u;
    CLEAR_FIELD(movement, object_lock_timer);
    CLEAR_FIELD(movement, start_lock_timer);
    CLEAR_FIELD(movement, movement_flags);
    CLEAR_FIELD(movement, object_status);
    CLEAR_FIELD(movement, vertical_reference);
    CLEAR_FIELD(movement, object_y);
    CLEAR_FIELD(movement, y_low);
    CLEAR_FIELD(movement, rider_state);
    CLEAR_FIELD(movement, collision_state);
    CLEAR_FIELD(movement, movement_mode);
    CLEAR_FIELD(movement, brake_state);
    CLEAR_FIELD(movement, approach);
    CLEAR_FIELD(movement, animation_state);
    CLEAR_FIELD(movement, lateral_state);
    CLEAR_FIELD(movement, motion_fraction);
    CLEAR_FIELD(movement, motion_whole);
    CLEAR_FIELD(movement, motion_fraction_start);
    CLEAR_FIELD(movement, motion_whole_start);
    CLEAR_FIELD(movement, motion_delta);
    movement->track_finished = 0u;

    CLEAR_FIELD(states, state);
    CLEAR_FIELD(states, rider_present);
    CLEAR_FIELD(states, x);
    CLEAR_FIELD(states, previous_x);
    CLEAR_FIELD(states, target_index);
    CLEAR_FIELD(states, y);
    CLEAR_FIELD(states, vertical_delta);
    CLEAR_FIELD(states, speed_whole);
    CLEAR_FIELD(states, vertical_state);
    CLEAR_FIELD(states, vertical_offset);
    CLEAR_FIELD(states, overheat);
    states->opponent_spawn_event = 0u;
    CLEAR_FIELD(states, vertical_collision_ref);
#undef CLEAR_FIELD
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
    if (in->setup_index_0041 >= 0x08u || in->course_index_0043 != 0u)
        return 0;

    memset(out, 0, sizeof(*out));
    out->setup_counter_0040 = (uint8_t)(in->setup_counter_0040 + 1u);
    out->setup_counter_0047 = (uint8_t)(in->setup_counter_0047 + 1u);
    out->timer_0034 = in->timer_0034 != 0u ? in->timer_0034 : 0x05u;
    out->transition_flag_0046 = in->transition_flag_0046;
    out->global_vertical_reference = 0xA0u;
    out->player_slot = 0u;
    /* C36C writes $0001 to ram_004A; CED0 therefore controls only slots 1..3. */
    out->first_opponent_slot = 1u;
    out->player_rider_present = 1u;
    out->camera_scroll_00E8 = 0x34u;
    out->camera_scroll_00E9 = 0x14u;
    out->camera_scroll_00EA = 0x25u;
    out->camera_scroll_00EB = 0x0Au;
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
    build_first_course_hud_glyphs(out);

    /* C36C/CED0 define slot identity independently of current activation:
     * slot 0 is the player, and the AI pass begins at slot 1. */
    for (i = 0u; i < 4u; ++i)
        out->bike[i].role = i == 0u ? NATIVE_RACE_BIKE_PLAYER : NATIVE_RACE_BIKE_OPPONENT;

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
        out->bike[i].role = i == 0u ? NATIVE_RACE_BIKE_PLAYER : NATIVE_RACE_BIKE_OPPONENT;

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
