#include <stdio.h>
#include <string.h>
#include "race_countdown.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

static void reset(BikeRaceCountdownState *c, BikeMovementState *m,
                  BikeStateMachineState *s)
{
    memset(c, 0, sizeof(*c));
    memset(m, 0, sizeof(*m));
    memset(s, 0, sizeof(*s));
}

int main(void)
{
    BikeRaceCountdownState c;
    BikeMovementState m;
    BikeStateMachineState s;

    reset(&c, &m, &s);
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_NONE, 1);

    /* DD8D timer == 8 branch. */
    c.timer_003C = 8u;
    s.overheat[0] = 0x7Fu;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_COUNTDOWN, 2);
    CHECK(s.overheat[0] == 0u && c.overheat_timer_03B6 == 5u, 3);
    CHECK(s.state[0] == 5u && m.object_lock_timer[0] == 5u, 4);

    /* Below eight, CD13 is requested even when the timer reaches zero if
     * the overheat gate remains set. */
    reset(&c, &m, &s);
    c.timer_003C = 7u;
    s.overheat[0] = 1u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_COUNTDOWN, 5);
    CHECK(s.overheat[0] == 1u && c.overheat_timer_03B6 == 0u, 6);
    c.timer_003C = 0u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_COUNTDOWN, 7);

    /* Above eight: active speed/state/collision gates prevent setup writes,
     * while odd timer values request the source start overlay. */
    reset(&c, &m, &s);
    c.timer_003C = 9u;
    m.speed_whole[0] = 1u;
    m.speed_fraction[0] = 0x55u;
    s.vertical_delta[0] = 0xA5u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_START_OVERLAY, 8);
    CHECK(m.speed_fraction[0] == 0x55u && s.vertical_delta[0] == 0xA5u, 9);

    reset(&c, &m, &s);
    c.timer_003C = 10u;
    m.object_action_state[0] = 1u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_NONE, 10);
    CHECK(m.speed_fraction[0] == 0xC0u, 11);

    reset(&c, &m, &s);
    c.timer_003C = 11u;
    s.y[0] = 0x39u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_START_OVERLAY, 12);
    CHECK(m.speed_fraction[0] == 0u && s.vertical_delta[0] == 0u, 13);

    reset(&c, &m, &s);
    c.timer_003C = 12u;
    s.y[0] = 0x20u;
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, &s) == BIKE_COUNTDOWN_RENDER_NONE, 14);
    CHECK(m.speed_fraction[0] == 0u && s.vertical_delta[0] == 1u, 15);

    CHECK(bikePhysicsUpdateRaceCountdown(0, &m, &s) == BIKE_COUNTDOWN_RENDER_NONE, 16);
    CHECK(bikePhysicsUpdateRaceCountdown(&c, 0, &s) == BIKE_COUNTDOWN_RENDER_NONE, 17);
    CHECK(bikePhysicsUpdateRaceCountdown(&c, &m, 0) == BIKE_COUNTDOWN_RENDER_NONE, 18);

    puts("PASS: native DD8D race-start countdown state and render events.");
    return 0;
}
