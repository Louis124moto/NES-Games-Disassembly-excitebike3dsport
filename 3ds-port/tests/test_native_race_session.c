#include <stdio.h>
#include <string.h>
#include "native_race_session.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

int main(void)
{
    NativeRaceSession session;
    NativeRaceSetupInput input;
    NativeRngState rng;
    unsigned int i;

    memset(&input, 0, sizeof(input));
    memset(&rng, 0, sizeof(rng));
    rng.bytes[0] = 0x01u;
    rng.bytes[1] = 0x10u;
    rng.bytes[2] = 0x20u;
    rng.bytes[3] = 0x30u;
    rng.bytes[4] = 0x40u;

    input.multiplayer_flag = 1u;
    input.setup_index_0041 = 2u;
    input.transition_flag_0046 = 0u;
    input.timer_0034 = 0x2Du; /* C3F3 selector setup leaves $0034=$2D. */
    input.countdown_timer_003C = 0u; /* Fresh first-course start: $003C is clear. */
    input.transition_counter_03F8 = 4u;
    input.setup_counter_0040 = 0u;
    input.setup_counter_0047 = 0u;

    CHECK(nativeRaceSessionInitFirstCourse(&session, &input, &rng) == 1, 1);
    CHECK(session.initialized == 1u, 2);
    CHECK(session.setup.timer_0034 == 0x2Du, 3);
    CHECK(session.setup.bike[0].y == 0x0Eu && session.setup.bike[3].y == 0x32u, 4);
    CHECK(session.movement.object_y[2] == 0x26u && session.states.y[2] == 0x26u, 5);
    CHECK(session.track.terrain_code[0] == 0x3Du && session.track.terrain_code[3] == 0x3Bu, 6);
    CHECK(session.countdown.timer_003C == 0u && session.countdown.overheat_timer_03B6 == 0u, 7);

    /* Source C9A5 -> D918 -> DD8D -> E733/E927 (no-op for this profile) -> E96C order. */
    nativeRaceSessionUpdateBikeStates(&session);
    CHECK(session.states.vertical_collision_ref[0] == 0x02u, 27);
    CHECK(session.states.vertical_collision_ref[1] == 0x0Fu, 28);
    CHECK(session.states.vertical_collision_ref[2] == 0x1Bu, 29);
    CHECK(session.states.vertical_collision_ref[3] == 0x27u, 30);

    CHECK(nativeRaceSessionUpdateCountdown(&session) == BIKE_COUNTDOWN_RENDER_NONE, 8);
    CHECK(session.movement.speed_fraction[0] == 0u && session.states.vertical_delta[0] == 0u, 9);
    for (i = 0u; i < 4u; ++i) CHECK(trackFirstCourseHasEvents(&session.track, (uint8_t)i) == 0u, 31);
    CHECK(session.movement.lateral_state[0] == 0u, 32); /* E927's ram_00CC gate */
    CHECK(nativeRaceSessionUpdateVertical(&session) == 0u, 10);
    for (i = 0u; i < 4u; ++i) {
        static const uint8_t y[4] = {0x0Eu, 0x1Au, 0x26u, 0x32u};
        uint8_t expected_y = y[i];
        uint8_t expected_delta = 0u;
        CHECK(session.movement.object_y[i] == expected_y, 11);
        CHECK(session.states.vertical_delta[i] == expected_delta, 12);
    }


    input.setup_index_0041 = 8u;
    CHECK(nativeRaceSessionInitFirstCourse(&session, &input, &rng) == 0, 20);
    CHECK(session.initialized == 0u, 21);
    CHECK(nativeRaceSessionUpdateCountdown(&session) == BIKE_COUNTDOWN_RENDER_NONE, 22);
    CHECK(nativeRaceSessionUpdateVertical(&session) == 0x0Fu, 23);
    CHECK(nativeRaceSessionInitFirstCourse(0, &input, &rng) == 0, 24);
    CHECK(nativeRaceSessionInitFirstCourse(&session, 0, &rng) == 0, 25);
    CHECK(nativeRaceSessionInitFirstCourse(&session, &input, 0) == 0, 26);

    puts("PASS: composed native first-course race session.");
    return 0;
}
