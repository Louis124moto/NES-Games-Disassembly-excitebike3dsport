#include "race_countdown.h"

BikeRaceCountdownRenderEvent bikePhysicsUpdateRaceCountdown(
    BikeRaceCountdownState *countdown,
    BikeMovementState *movement,
    BikeStateMachineState *states)
{
    uint8_t timer;

    if (countdown == 0 || movement == 0 || states == 0)
        return BIKE_COUNTDOWN_RENDER_NONE;

    timer = countdown->timer_003C;
    if ((uint8_t)(timer | states->overheat[0]) == 0u)
        return BIKE_COUNTDOWN_RENDER_NONE;

    if (timer == 0x08u) {
        /* DD8D: exact one-time transition at timer == 8. */
        states->overheat[0] = 0u;
        countdown->overheat_timer_03B6 = 0x05u;
        states->state[0] = 0x05u;
        movement->object_lock_timer[0] = 0x05u;
        return BIKE_COUNTDOWN_RENDER_COUNTDOWN;
    }

    if (timer < 0x08u)
        return BIKE_COUNTDOWN_RENDER_COUNTDOWN;

    /* DDA0: state/velocity/collision gates suppress countdown preparation. */
    if ((uint8_t)(movement->speed_whole[0] |
                  states->state[0] |
                  movement->collision_state[0]) == 0u) {
        if (movement->object_action_state[0] != 0u) {
            movement->speed_fraction[0] = 0xC0u;
        } else {
            movement->speed_fraction[0] = 0u;
            states->vertical_delta[0] =
                states->y[0] == 0x39u ? 0u : 0x01u;
        }
    }

    /* CCDD only draws on odd timer frames; rendering is native. */
    return (timer & 1u) != 0u
        ? BIKE_COUNTDOWN_RENDER_START_OVERLAY
        : BIKE_COUNTDOWN_RENDER_NONE;
}
