#include <stdio.h>
#include <string.h>
#include "native_race_setup.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

int main(void)
{
    static const uint8_t expected_y[4] = {0x0Eu, 0x1Au, 0x26u, 0x32u};
    static const uint8_t expected_x[4] = {0x38u, 0x48u, 0x58u, 0x68u};
    static const uint8_t expected_class[4] = {0x09u, 0x0Bu, 0x0Du, 0x0Fu};
    NativeRaceSetupInput in;
    NativeRaceSetupState out;
    NativeRngState rng;
    BikeMovementState movement;
    BikeStateMachineState states;
    unsigned int i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&movement, 0xA5, sizeof(movement));
    memset(&states, 0xA5, sizeof(states));
    memset(&rng, 0, sizeof(rng));
    nativeRaceSetupClearFirstCourseState(&movement, &states);
    CHECK(movement.speed_whole[0] == 0u && movement.vertical_phase[0] == 0u, 45);
    CHECK(movement.object_lock_timer[0] == 0u && movement.motion_fraction[0] == 0u, 46);
    CHECK(states.state[0] == 0u && states.rider_present[0] == 0u && states.overheat[0] == 0u, 47);
    CHECK(movement.player_scroll_x == 0xA5u && movement.global_vertical_reference == 0xA5u, 48);
    CHECK(states.global_overheat == 0xA5u && states.controller_hold == 0xA5u, 49);
    CHECK(states.opponent_spawn_event == 0u, 50);
    rng.bytes[0] = 0x01u;
    rng.bytes[1] = 0x10u;
    rng.bytes[2] = 0x21u;
    rng.bytes[3] = 0x32u;
    rng.bytes[4] = 0x43u;
    in.multiplayer_flag = 1u;
    in.setup_index_0041 = 2u;
    in.timer_0034 = 0u;
    in.setup_counter_0040 = 9u;
    in.setup_counter_0047 = 5u;

    in.transition_counter_03F8 = 4u; /* C2BD seeds $03F8=$04 on title route. */
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states), 1);
    CHECK(out.setup_counter_0040 == 10u && out.setup_counter_0047 == 6u, 2);
    CHECK(out.timer_0034 == 5u && out.global_vertical_reference == 0xA0u, 3);
    CHECK(out.player_slot == 0u && out.first_opponent_slot == 1u && out.player_rider_present == 1u, 4);
    CHECK(out.camera_scroll_00E8 == 0x34u && out.camera_scroll_00E9 == 0x14u && out.camera_scroll_00EA == 0x25u && out.camera_scroll_00EB == 0x0Au, 9);
    CHECK(out.countdown_x_03B4 == 0x80u && out.race_setup_state_03B0 == 0x80u, 25);
    CHECK(out.countdown_limit_03BD == 0xFFu && out.countdown_index_03BE == 0x02u, 26);
    CHECK(out.course_setup_parameters[0] == 0x01u && out.course_setup_parameters[1] == 0x10u && out.course_setup_parameters[2] == 0x00u, 6);
    CHECK(out.race_object_parameters[0] == 0x01u && out.race_object_parameters[1] == 0x10u && out.race_object_parameters[2] == 0x00u, 8);
    CHECK(out.hud_glyphs[0] == 1u && out.hud_glyphs[1] == 0xFBu && out.hud_glyphs[2] == 1u && out.hud_glyphs[3] == 6u && out.hud_glyphs[4] == 0xFBu && out.hud_glyphs[5] == 0u && out.hud_glyphs[6] == 0u, 7);
    CHECK(out.half_lock_slot == 1u, 5);

    for (i = 0u; i < 4u; ++i) {
        CHECK(out.bike[i].y == expected_y[i], 10);
        CHECK(out.bike[i].x_low == expected_x[i] && out.bike[i].x_high == 1u, 11);
        CHECK(out.bike[i].object_class == expected_class[i], 12);
        CHECK(out.bike[i].approach == 6u && out.bike[i].active == 1u, 13);
        CHECK(out.bike[i].object_origin_x == 8u, 14);
        CHECK(out.bike[i].role == (i == 0u ? NATIVE_RACE_BIKE_PLAYER : NATIVE_RACE_BIKE_OPPONENT), 51);
        CHECK(out.bike[i].setup_parameter == (uint8_t)(rng.bytes[i + 1u] >> 1), 15);
        CHECK(movement.object_y[i] == expected_y[i] && states.y[i] == expected_y[i], 16);
        CHECK(movement.x_low[i] == expected_x[i] && movement.x_high[i] == 1u, 17);
        CHECK(movement.object_class[i] == expected_class[i], 18);
        CHECK(movement.object_origin_x[i] == 8u && movement.setup_parameter[i] == (uint8_t)(rng.bytes[i + 1u] >> 1), 27);
        CHECK(movement.object_active[i] == 1u && movement.approach[i] == 6u, 19);
    }
    CHECK(out.bike[0].lock_timer == 6u && out.bike[1].lock_timer == 3u, 20);
    CHECK(out.bike[2].lock_timer == 6u && out.bike[3].lock_timer == 6u, 21);
    CHECK(movement.start_lock_timer[1] == 3u, 22);
    CHECK(states.rider_present[0] == 1u && states.rider_present[1] == 0u, 23);
    CHECK(out.bike[0].role == NATIVE_RACE_BIKE_PLAYER && out.bike[1].role == NATIVE_RACE_BIKE_OPPONENT, 52);
    CHECK(movement.global_vertical_reference == 0xA0u, 24);

    /* Single-rider route initializes only slot zero; other native slots survive. */
    memset(&movement, 0, sizeof(movement));
    memset(&states, 0, sizeof(states));
    memset(&rng, 0, sizeof(rng));
    rng.bytes[0] = 0u; /* zero selects slot 2, per C8D3-C8DC */
    movement.object_active[3] = 0xA5u;
    movement.start_lock_timer[2] = 0xA6u;
    states.y[3] = 0x5Au;
    memset(&in, 0, sizeof(in));
    in.setup_index_0041 = 2u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states), 30);
    CHECK(out.bike[0].active == 1u && out.bike[1].active == 0u, 31);
    CHECK(movement.object_active[3] == 0xA5u && states.y[3] == 0x5Au, 32);
    CHECK(out.bike[2].lock_timer == 0x53u && movement.start_lock_timer[2] == 0x53u, 33);
    CHECK(states.rider_present[0] == 1u, 34);
    in.transition_flag_0046 = 1u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states), 35);
    CHECK(out.course_setup_parameters[0] == 0x01u && out.course_setup_parameters[1] == 0x18u && out.course_setup_parameters[2] == 0x00u, 36);
    CHECK(out.race_object_parameters[0] == 0x01u && out.race_object_parameters[1] == 0x18u && out.race_object_parameters[2] == 0x00u, 39);
    in.transition_flag_0046 = 0u;
    in.transition_counter_03F8 = 0x16u; /* C90E's temporary digits are overwritten by C53E/C935. */
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states), 37);
    CHECK(out.hud_glyphs[0] == 1u && out.hud_glyphs[2] == 1u && out.hud_glyphs[3] == 6u, 38);

    CHECK(nativeRaceSetupFirstCourse(0, &in, &rng, &movement, &states) == 0, 40);
    CHECK(nativeRaceSetupFirstCourse(&out, 0, &rng, &movement, &states) == 0, 41);
    CHECK(nativeRaceSetupFirstCourse(&out, &in, 0, &movement, &states) == 0, 42);
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, 0) == 0, 43);
    in.setup_index_0041 = 2u;
    in.course_index_0043 = 1u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states) == 1, 53); /* all five courses */
    in.course_index_0043 = 5u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states) == 0, 54);
    in.course_index_0043 = 0u;
    in.setup_index_0041 = 8u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &rng, &movement, &states) == 0, 44);

    puts("PASS: native C875 first-course semantic setup.");
    return 0;
}
