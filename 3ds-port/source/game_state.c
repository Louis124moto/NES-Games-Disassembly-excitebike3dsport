#include "game_state.h"

void gameStateReset(GameState *state)
{
    if (state == 0) {
        return;
    }

    state->bike.speed = 0;
    state->bike.temperature = 0;
    state->bike.track_position = 0;
    state->bike.vertical_velocity = 0;
    state->bike.airborne = false;
    state->bike.crashed = false;
    state->paused = false;
    state->show_help = true;
    state->menu_selection = 1u;
    state->menu_repeat_guard = 0u;
    state->menu_timer30 = 0u;
    state->menu_timer31 = 0u;
    state->menu_selection_changed = 0u;
    state->menu_action_x = 0u;
}
