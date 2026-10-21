#include <string.h>
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
    state->menu_timer31 = 0u; /* initialized by ofs_001_C2BD_00_01 */
    state->menu_timer33 = 0u;
    state->menu_timer34 = 0u;
    memset(state->menu_ram0300, 0, sizeof(state->menu_ram0300));
    state->menu_sequence_preface_pending = 1u;
    state->menu_selection_changed = 0u;
    state->menu_action_x = 0u;
    state->menu_ram0040 = 0u;
    state->menu_ram0043 = 0u;
    state->menu_ram0044 = 0u;
    state->menu_ram004A = 0u;
    state->menu_ram03F7 = 0u;
    state->menu_sprite_y = 0xF8u; /* sub_D255 hides OAM sprites during setup */
    state->menu_sprite_x = 0u;
    state->menu_sprite_attribute = 0u;
    /* ofs_001_C2BD_00_01 runs through the setup command sequencer. */
    state->menu_ram03F0 = 0u;
    state->menu_ram03F3 = 0u;
    state->menu_ram0046 = 0u;
    state->menu_ram0048 = 0u;
    state->menu_ram004B = 0u;
    memset(state->menu_ram05E0, 0, sizeof(state->menu_ram05E0));
    memset(state->menu_ram06E0, 0, sizeof(state->menu_ram06E0));
    state->menu_ram03F8 = 0u;
    state->menu_ram0045 = 0u;
    state->menu_ram00FB = 0u;
    state->menu_sprite_tile = 0u;
    state->menu_scroll_x = 0u;
    state->menu_scroll_y = 0u;
    state->menu_ppu_ctrl = 0u;
    state->menu_ppu_mask = 0u;
    memset(state->menu_oam, 0xF8, sizeof(state->menu_oam));
    memset(state->menu_ppu_nametables, 0, sizeof(state->menu_ppu_nametables));
}
