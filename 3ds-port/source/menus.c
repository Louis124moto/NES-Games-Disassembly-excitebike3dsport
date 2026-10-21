#include <stdio.h>
#include <3ds.h>
#include "menus.h"
#include "nes_routines.h"

/*
 * Interactive state-8 menu prototype based on ofs_000_C3BD_08 and sub_C7DE.
 * Option labels/art and subsequent states remain gated on tracing the original
 * PPU/CHR data; this screen deliberately does not invent their meanings.
 */
void menusUpdate(GameState *state, unsigned int pressed)
{
    uint8_t nesButtons = 0u;
    uint8_t changed = 0u;

    if (state == 0) return;

    if (pressed & KEY_SELECT) nesButtons |= 0x20u;
    if (pressed & KEY_START)  nesButtons |= 0x10u;
    if (pressed & KEY_UP)     nesButtons |= 0x08u;
    if (pressed & KEY_DOWN)   nesButtons |= 0x04u;

    if (state->menu_asm_state == 8u) {
        state->menu_action_x = nesSubC7DEMenuInput(
            nesButtons, 6u, &state->menu_selection,
            &state->menu_repeat_guard, &state->menu_timer30,
            &state->menu_timer31, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x != 0u) {
            nesSubC3BDConfirmSelection(
                state->menu_action_x, &state->menu_asm_state,
                &state->menu_selection, &state->menu_ram03F2,
                &state->menu_clear_ram_requested);
        }
    } else {
        /* State transition is recorded, but following ASM states are not yet ported. */
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    }

    if (pressed & KEY_X) state->show_help = !state->show_help;
}

void menusRender(const GameState *state)
{
    unsigned int i;
    if (state == 0) return;

    printf("\n");
    printf("EXCITEBIKE - MAIN MENU ASM TRACE\n");
    printf("--------------------------------\n");
    printf("State ram_0041: %u\n", (unsigned int)state->menu_asm_state);
    printf("Selection ram_0042: %u\n", (unsigned int)state->menu_selection);
    printf("Mode ram_03F2: %u | clear-RAM path: %u\n",
           (unsigned int)state->menu_ram03F2,
           (unsigned int)state->menu_clear_ram_requested);
    if (state->menu_asm_state == 8u) {
        for (i = 1u; i <= 5u; ++i) {
            printf("%c Option %u [label trace pending]\n",
                   i == (unsigned int)state->menu_selection ? '>' : ' ',
                   i);
        }
        printf("UP/DOWN or SELECT: cursor; START: ASM confirm branch\n");
    } else {
        printf("ASM transition recorded; destination state is not yet integrated.\n");
    }
    if (state->show_help) {
        printf("Authentic NES title art/palette and option names remain pending.\n");
        printf("No labels or follow-on transitions are guessed.\n");
    }
}
