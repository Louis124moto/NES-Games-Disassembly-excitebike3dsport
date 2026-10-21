#include <stdio.h>
#include <3ds.h>
#include "menus.h"
#include "nes_routines.h"

/*
 * Source-derived menu dispatcher trace.
 *
 * State 8 is ofs_000_C3BD_08 ($C3BD); state 0 is ofs_000_C346_00
 * ($C346). Selection meanings and original title artwork are deliberately
 * not guessed. The cursor and state/reset side effects follow bank_FF.asm.
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

    state->menu_clear_ram_requested = 0u;

    /*
     * ofs_000_C346_00: when ram_0031_timer is zero, the ASM initializes
     * ram_0043/ram_004A, sets ram_03F7=$0F and enters state 2.
     */
    if (state->menu_asm_state == 0u && state->menu_timer31 == 0u) {
        state->menu_ram0043 = 0u;
        state->menu_ram004A = 0u;
        state->menu_ram03F7 = 0x0Fu;
        state->menu_ram0040 = 0u;
        state->menu_ram0044 = 0u;
        state->menu_asm_state = 2u;
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
        return;
    }

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

        /*
         * bra_C3E4 -> loc_CA85_clear_0021_00FF:
         * RAM $0021-$00FF is cleared, ram_0049 is then incremented,
         * and ram_0042 is restored from the stack. The modeled fields below
         * are the menu-visible bytes in that range.
         */
        if (state->menu_clear_ram_requested != 0u) {
            state->menu_asm_state = 0u;       /* ram_0041 was cleared */
            state->menu_repeat_guard = 1u;   /* INC ram_0049 after clear */
            state->menu_timer30 = 0u;
            state->menu_timer31 = 0u;
            state->menu_ram0040 = 0u;
            state->menu_ram0043 = 0u;
            state->menu_ram0044 = 0u;
            state->menu_ram004A = 0u;
            state->menu_action_x = 0u;
            state->menu_selection_changed = 0u;
            /* ram_0042=2 is preserved; ram_03F2 lives outside the cleared page. */
        }
    } else {
        /* Other dispatch states require their own verified ASM blocks. */
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    }

    if (pressed & KEY_X) state->show_help = !state->show_help;
}

void menusRender(const GameState *state)
{
    unsigned int i;
    if (state == 0) return;

    printf("\nEXCITEBIKE - ASM MENU TRACE\n");
    printf("---------------------------\n");
    printf("ram_0041 state: %u\n", (unsigned int)state->menu_asm_state);
    printf("ram_0042 selection: %u\n", (unsigned int)state->menu_selection);
    printf("ram_03F2 mode: %u | clear path: %u\n",
           (unsigned int)state->menu_ram03F2,
           (unsigned int)state->menu_clear_ram_requested);

    if (state->menu_asm_state == 8u) {
        for (i = 1u; i <= 5u; ++i) {
            printf("%c Option %u [ASM label unresolved]\n",
                   i == (unsigned int)state->menu_selection ? '>' : ' ',
                   i);
        }
        printf("UP/DOWN or SELECT: move; START: source confirm branch\n");
    } else if (state->menu_asm_state == 0u) {
        printf("State 0: source reset/entry transition pending next update.\n");
    } else if (state->menu_asm_state == 2u) {
        printf("State 2: original ASM dispatches to the gameplay loop.\n");
        printf("Native gameplay is still a separate incomplete subsystem.\n");
    } else {
        printf("ASM destination state %u reached; its native path is not ported.\n",
               (unsigned int)state->menu_asm_state);
    }

    if (state->show_help) {
        printf("Title graphics, palette/nametable mapping, and option names are not yet reconstructed.\n");
        printf("This is an input/state trace, not the original NES title screen.\n");
    }
}
