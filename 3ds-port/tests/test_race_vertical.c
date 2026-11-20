#include <stdio.h>
#include <string.h>
#include "race_vertical.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

static void reset(BikeMovementState *m, BikeStateMachineState *s)
{
    memset(m, 0, sizeof(*m));
    memset(s, 0, sizeof(*s));
}

int main(void)
{
    static const uint8_t initial_y[4] = {0x0Eu, 0x1Au, 0x26u, 0x32u};
    BikeMovementState m;
    BikeStateMachineState s;
    unsigned int i;

    reset(&m, &s);
    for (i = 0u; i < 4u; ++i) m.object_y[i] = initial_y[i];
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 1);
    for (i = 0u; i < 4u; ++i) {
        CHECK(m.object_y[i] == initial_y[i] && s.y[i] == initial_y[i], 2);
        CHECK(s.vertical_delta[i] == 0u, 3);
    }

    /* E836/DC97 screen projection, including the borrow chain between SBCs. */
    reset(&m, &s);
    m.global_vertical_reference = 0xA0u;
    m.object_y[0] = 0x0Eu;
    m.object_y[1] = 0x1Au;
    m.object_y[2] = 0x26u;
    m.object_y[3] = 0x32u;
    bikePhysicsUpdateScreenY(&m);
    CHECK(m.y_low[0] == 0x92u && m.y_low[1] == 0x86u &&
          m.y_low[2] == 0x7Au && m.y_low[3] == 0x6Eu, 5);
    reset(&m, &s);
    m.global_vertical_reference = 0x10u;
    m.vertical_reference[0] = 0x20u;
    m.object_y[0] = 0x05u;
    bikePhysicsUpdateScreenY(&m);
    CHECK(m.y_low[0] == 0xEAu, 6); /* second SBC consumes first SBC's borrow */
    reset(&m, &s);
    m.global_vertical_reference = 0xA0u;
    m.object_y[0] = 0x0Eu;
    m.object_status[0] = 1u;
    m.y_low[0] = 0x55u;
    bikePhysicsUpdateScreenY(&m);
    CHECK(m.y_low[0] == 0x55u, 7); /* E836 skips ram_00B0 != 0 */

    /* Source signed one-pixel drift toggles 0 -> +1 -> -1 -> +1. */
    reset(&m, &s);
    m.object_y[0] = 0x18u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 10);
    CHECK(m.object_y[0] == 0x18u && s.vertical_delta[0] == 1u, 11);
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 12);
    CHECK(m.object_y[0] == 0x19u && s.vertical_delta[0] == 0xFFu, 13);
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 14);
    CHECK(m.object_y[0] == 0x18u && s.vertical_delta[0] == 1u, 15);

    /* $14/$15 suppress the position addition but still execute later gates. */
    reset(&m, &s);
    m.object_y[0] = 0x18u;
    m.object_action_state[0] = 0x14u;
    s.vertical_delta[0] = 1u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 20);
    CHECK(m.object_y[0] == 0x18u && s.vertical_delta[0] == 0xFFu, 21);

    /* Low/high clamps and state/overheat gating. */
    reset(&m, &s);
    m.object_y[0] = 0x07u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 30);
    CHECK(m.object_y[0] == 0x07u && s.vertical_delta[0] == 1u, 31);

    reset(&m, &s);
    m.object_y[0] = 0x01u;
    s.state[0] = 1u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 32);
    CHECK(m.object_y[0] == 0x01u && s.vertical_delta[0] == 0u, 33);

    reset(&m, &s);
    m.object_y[0] = 0x3Au;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 34);
    CHECK(m.object_y[0] == 0x39u && s.vertical_delta[0] == 1u, 35);

    reset(&m, &s);
    m.object_y[0] = 0x18u;
    s.overheat[0] = 1u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0u, 36);
    CHECK(m.object_y[0] == 0x18u && s.vertical_delta[0] == 0u, 37);

    /* Nonzero $00A4 is reported and left untouched; no fake DD06 behavior. */
    reset(&m, &s);
    m.object_y[2] = 0x18u;
    s.y[2] = 0x18u;
    s.vertical_delta[2] = 1u;
    m.vertical_phase[2] = 1u;
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, &s) == 0x04u, 40);
    CHECK(m.object_y[2] == 0x18u && s.vertical_delta[2] == 1u, 41);

    CHECK(bikePhysicsUpdateFirstCourseVertical(0, &s) == 0x0Fu, 50);
    CHECK(bikePhysicsUpdateFirstCourseVertical(&m, 0) == 0x0Fu, 51);
    puts("PASS: native E96C first-course vertical phase-zero path.");
    return 0;
}
