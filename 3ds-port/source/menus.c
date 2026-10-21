#include <stdio.h>
#include <3ds.h>
#include "menus.h"
#include "nes_routines.h"

/*
 * This is an interactive trace of the original menu input helper, not yet
 * the complete NES title screen. Menu option meanings and PPU tile/palette
 * data must be traced before drawing authentic labels or applying transitions.
 */
void menusUpdate(GameState *state, unsigned int pressed)
{
    uint8_t nesButtons = 0u;
    uint8_t changed = 0u;

    if (state == 0) return;

    if (pressed & KEY_SELECT) nesButtons |= 0x20u; /* NES Select */
    if (pressed & KEY_START)  nesButtons |= 0x10u; /* NES Start */
    if (pressed & KEY_UP)     nesButtons |= 0x08u; /* NES Up */
    if (pressed & KEY_DOWN)   nesButtons |= 0x04u; /* NES Down */

    state->menu_action_x = nesSubC7DEMenuInput(
        nesButtons, 6u, &state->menu_selection,
        &state->menu_repeat_guard, &state->menu_timer30,
        &state->menu_timer31, &changed);
    state->menu_selection_changed = changed;

    if (pressed & KEY_X) state->show_help = !state->show_help;
}

void menusRender(const GameState *state)
{
    if (state == 0) return;

    printf("\n");
    printf("EXCITEBIKE - MAIN MENU INPUT TRACE\n");
    printf("----------------------------------\n");
    printf("ASM state under study: 08 / sub_C3BD\n");
    printf("Cursor index (ram_0042): %u\n", (unsigned int)state->menu_selection);
    printf("Action X model: %u | timer $0030: %u\n",
           (unsigned int)state->menu_action_x, (unsigned int)state->menu_timer30);
    printf("UP/DOWN or SELECT: move cursor\n");
    printf("START: return ASM action X=4\n");
    printf("X: toggle help text\n");
    if (state->show_help) {
        printf("Faithful title artwork and option labels are not yet wired.\n");
        printf("No menu labels or transitions are guessed.\n");
    }
}
