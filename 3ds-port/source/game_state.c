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
    state->menu_timer20 = 0u;
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
    /*
     * First command in the original startup sequence: ofs_001_C2BD_00_01.
     * The reset-cleared ram_03F0 is zero on entry, so this command sets
     * ram_00FB=1, advances ram_03F0 to 1, and initializes the cursor sprite.
     */
    state->menu_ram03F0 = 1u;
    state->menu_ram03F3 = 0u;
    state->menu_ram0046 = 0u;
    state->menu_ram03F8 = 0x04u;
    state->menu_ram0045 = 0x02u;
    state->menu_ram00FB = 1u;
    state->menu_sprite_tile = 0xFAu;
}
