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
    BikeMovementState movement;
    BikeStateMachineState states;
    unsigned int i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&movement, 0, sizeof(movement));
    memset(&states, 0, sizeof(states));
    in.multiplayer_flag = 1u;
    in.object_param_0019[0] = 0x10u;
    in.object_param_0019[1] = 0x21u;
    in.object_param_0019[2] = 0x32u;
    in.object_param_0019[3] = 0x43u;
    in.wrapped_param_0018 = 0x01u;
    in.timer_0034 = 0u;
    in.setup_counter_0040 = 9u;
    in.setup_counter_0047 = 5u;

    CHECK(nativeRaceSetupFirstCourse(&out, &in, &movement, &states), 1);
    CHECK(out.setup_counter_0040 == 10u && out.setup_counter_0047 == 6u, 2);
    CHECK(out.timer_0034 == 5u && out.global_vertical_reference == 0xA0u, 3);
    CHECK(out.player_slot == 0u && out.player_rider_present == 1u, 4);
    CHECK(out.course_setup_parameters[0] == 0x01u && out.course_setup_parameters[1] == 0x10u && out.course_setup_parameters[2] == 0x00u, 6);
    CHECK(out.half_lock_slot == 1u, 5);

    for (i = 0u; i < 4u; ++i) {
        CHECK(out.bike[i].y == expected_y[i], 10);
        CHECK(out.bike[i].x_low == expected_x[i] && out.bike[i].x_high == 1u, 11);
        CHECK(out.bike[i].object_class == expected_class[i], 12);
        CHECK(out.bike[i].approach == 6u && out.bike[i].active == 1u, 13);
        CHECK(out.bike[i].animation_seed == 8u, 14);
        CHECK(out.bike[i].speed_phase == (uint8_t)(in.object_param_0019[i] >> 1), 15);
        CHECK(movement.object_y[i] == expected_y[i] && states.y[i] == expected_y[i], 16);
        CHECK(movement.x_low[i] == expected_x[i] && movement.x_high[i] == 1u, 17);
        CHECK(movement.object_class[i] == expected_class[i], 18);
        CHECK(movement.object_active[i] == 1u && movement.approach[i] == 6u, 19);
    }
    CHECK(out.bike[0].lock_timer == 6u && out.bike[1].lock_timer == 3u, 20);
    CHECK(out.bike[2].lock_timer == 6u && out.bike[3].lock_timer == 6u, 21);
    CHECK(movement.start_lock_timer[1] == 3u, 22);
    CHECK(states.rider_present[0] == 1u && states.rider_present[1] == 0u, 23);
    CHECK(movement.global_vertical_reference == 0xA0u, 24);

    /* Single-rider route initializes only slot zero; other native slots survive. */
    memset(&movement, 0, sizeof(movement));
    memset(&states, 0, sizeof(states));
    movement.object_active[3] = 0xA5u;
    movement.start_lock_timer[2] = 0xA6u;
    states.y[3] = 0x5Au;
    memset(&in, 0, sizeof(in));
    in.wrapped_param_0018 = 0u; /* zero selects slot 2, per C8D3-C8DC */
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &movement, &states), 30);
    CHECK(out.bike[0].active == 1u && out.bike[1].active == 0u, 31);
    CHECK(movement.object_active[3] == 0xA5u && states.y[3] == 0x5Au, 32);
    CHECK(out.bike[2].lock_timer == 0x53u && movement.start_lock_timer[2] == 0x53u, 33);
    CHECK(states.rider_present[0] == 1u, 34);
    in.transition_flag_0046 = 1u;
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &movement, &states), 35);
    CHECK(out.course_setup_parameters[0] == 0x01u && out.course_setup_parameters[1] == 0x18u && out.course_setup_parameters[2] == 0x00u, 36);

    CHECK(nativeRaceSetupFirstCourse(0, &in, &movement, &states) == 0, 40);
    CHECK(nativeRaceSetupFirstCourse(&out, 0, &movement, &states) == 0, 41);
    CHECK(nativeRaceSetupFirstCourse(&out, &in, 0, &states) == 0, 42);
    CHECK(nativeRaceSetupFirstCourse(&out, &in, &movement, 0) == 0, 43);

    puts("PASS: native C875 first-course semantic setup.");
    return 0;
}
