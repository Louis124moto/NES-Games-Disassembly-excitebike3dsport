#include "race_vertical.h"

static uint8_t is_vertical_anchor(uint8_t y)
{
    return (uint8_t)(y == 0x0Eu || y == 0x1Au || y == 0x26u || y == 0x32u);
}

uint8_t bikePhysicsUpdateFirstCourseVertical(BikeMovementState *movement,
                                             BikeStateMachineState *states)
{
    int index;
    uint8_t unsupported_mask = 0u;

    if (movement == 0 || states == 0) return 0x0Fu;

    /* Source iterates X=3..0. */
    for (index = 3; index >= 0; --index) {
        const unsigned int i = (unsigned int)index;
        uint8_t y = movement->object_y[i];
        uint8_t delta = states->vertical_delta[i];

        /* The first course starts with $00A4=0. Other phase handlers require
         * DD06 and are deliberately reported to the caller for later porting. */
        if (movement->vertical_phase[i] != 0u) {
            unsupported_mask = (uint8_t)(unsupported_mask | (uint8_t)(1u << i));
            continue;
        }

        /* E96E-E97D: states $14/$15 suppress the Y+delta operation. */
        if (movement->object_action_state[i] != 0x14u &&
            movement->object_action_state[i] != 0x15u) {
            y = (uint8_t)(y + delta);
        }

        /* E981-E986: the four original bike lanes stop vertical drift. */
        if (is_vertical_anchor(y) != 0u) {
            states->vertical_delta[i] = 0u;
            movement->object_y[i] = y;
            states->y[i] = y;
            continue;
        }

        /* E9C4-E9D0 / E9E9-E9F9: source vertical bounds. */
        if (y < 0x08u) {
            if (states->state[i] == 0u) {
                y = 0x07u;
            } else if (y < 0x02u) {
                y = 0x01u;
            }
        } else if (y >= 0x3Au) {
            y = 0x39u;
        }

        movement->object_y[i] = y;
        states->y[i] = y;

        /* E9D2-E9E1: only a bike with no movement state or overheat toggles
         * the signed one-pixel vertical direction (0 -> 1, 1 <-> $FF). */
        if ((uint8_t)(states->state[i] | states->overheat[i]) == 0u) {
            if (delta == 0u) delta = 0xFFu;
            states->vertical_delta[i] = (uint8_t)(delta ^ 0xFEu);
        }
    }

    return unsupported_mask;
}
