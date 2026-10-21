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
    state->menu_asm_state = 0u;
    state->menu_ram03F2 = 0u;
    state->menu_clear_ram_requested = 0u;
    state->menu_selection = 0u;
    state->menu_repeat_guard = 0u;
    state->menu_timer30 = 0u;
    state->menu_timer31 = 0x38u; /* ofs_C2BD initializes the menu display timer */
    state->menu_selection_changed = 0u;
    state->menu_action_x = 0u;
    state->menu_ram0040 = 0u;
    state->menu_ram0043 = 0u;
    state->menu_ram0044 = 0u;
    state->menu_ram004A = 0u;
    state->menu_ram03F7 = 0u;
    state->menu_sprite_y = 0x80u; /* tbl_C080_spr_Y[0] */
    state->menu_sprite_x = 0x50u; /* tbl_C088_spr_X[0] */
    state->menu_sprite_attribute = 0u;
    state->menu_ram03F0 = 0u;
    state->menu_ram03F3 = 0u;
    state->menu_ram0046 = 0u;
}
