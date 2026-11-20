#include "race_vertical.h"

void bikePhysicsUpdateScreenY(BikeMovementState *movement)
{
    int index;
    if (movement == 0) return;

    /* E836 calls DC97 only for objects with ram_00B0 == 0.
     * DC97/DC9A performs two 6502 SBC operations, so the borrow from the
     * first subtraction must feed the second one. */
    for (index = 3; index >= 0; --index) {
        const unsigned int i = (unsigned int)index;
        uint8_t a;
        uint8_t carry;

        if (movement->object_status[i] != 0u) continue;
        carry = movement->global_vertical_reference >= movement->vertical_reference[i]
                    ? 1u : 0u;
        a = (uint8_t)(movement->global_vertical_reference -
                      movement->vertical_reference[i]);
        movement->y_low[i] = (uint8_t)(a - movement->object_y[i] -
                                       (carry != 0u ? 0u : 1u));
    }
}

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
        uint8_t update_direction = 0u;

        /* E96E-E97D: action states $14/$15 suppress the Y+delta addition. */
        if (movement->object_action_state[i] != 0x14u &&
            movement->object_action_state[i] != 0x15u) {
            y = (uint8_t)(y + delta);
        }

        /* E981-E986: all four indexed source anchors stop vertical motion. */
        if (is_vertical_anchor(y) != 0u) {
            states->vertical_delta[i] = 0u;
            movement->object_y[i] = y;
            states->y[i] = y;
            continue;
        }

        /* E96C phase dispatch. Phase 1 can start a jump via DD06; phases
         * other than 1/3 move Y toward $20 while it is at/above that value. */
        if (movement->vertical_phase[i] == 0x01u) {
            if (y < 0x20u) {
                movement->vertical_phase[i] = 0x04u;
                if (movement->object_action_state[i] == 0u ||
                    movement->object_action_state[i] == 0x12u ||
                    movement->object_action_state[i] == 0x10u ||
                    movement->object_action_state[i] == 0x0Du) {
                    movement->vertical_reference[i] = 0u;
                    bikePhysicsInitializeVerticalJump(movement, (uint8_t)i);
                    movement->vertical_phase[i] =
                        (uint8_t)(movement->vertical_phase[i] - 1u);
                }
                continue; /* E9E3: phase-one path skips E9C4 bounds. */
            }
        } else if (movement->vertical_phase[i] != 0x00u &&
                   movement->vertical_phase[i] != 0x03u &&
                   movement->vertical_phase[i] != 0x01u && y >= 0x20u) {
            y = (uint8_t)(y - 1u);
        }

        /* E9C4-E9D0 / E9E9-E9F9: source vertical bounds. */
        if (y < 0x08u) {
            update_direction = 2u; /* X=0 takes E9D9 directly, bypassing state gates. */
            if (states->state[i] == 0u) {
                y = 0x07u;
            } else if (y < 0x02u) {
                y = 0x01u;
            }
        } else if (y >= 0x3Au) {
            y = 0x39u;
            update_direction = 1u;
        }

        movement->object_y[i] = y;
        states->y[i] = y;

        /* E9D2-E9E1: direction toggles only after the low/high-bound paths.
         * In-range Y ($08..$39) branches directly to E9E3 and leaves delta
         * unchanged; low/high clamps reach the direction update. */
        if ((update_direction == 2u && i == 0u) ||
            (update_direction != 0u &&
             (uint8_t)(states->state[i] | states->overheat[i]) == 0u)) {
            if (delta == 0u) delta = 0xFFu;
            states->vertical_delta[i] = (uint8_t)(delta ^ 0xFEu);
        }
    }

    /* EA07-EA43: native player vertical-input handshake after all four
     * per-bike vertical updates. This is deliberately kept in the same
     * routine because the original calls it as the tail of E96C. */
    if (states->race_started != 0u &&
        (uint8_t)(movement->collision_state[0] |
                  states->overheat[0] |
                  states->global_overheat) == 0u) {
        if (movement->object_status[0] != 0u) {
            if (movement->movement_mode[0] != 0x02u) return unsupported_mask;
            movement->movement_mode[0] = (uint8_t)(movement->movement_mode[0] + 1u);
        }

        if (states->state[0] == 0u || states->state[0] == 0x05u) {
            const uint8_t vertical_buttons =
                (uint8_t)(states->controller_hold & 0x0Cu);
            if (vertical_buttons != 0u) {
                states->vertical_delta[0] =
                    (vertical_buttons & 0x04u) != 0u ? 0xFFu : 0x01u;
                states->player_event = (uint8_t)(states->player_event | 0x04u);
            }
        }
    }

    return unsupported_mask;
}
