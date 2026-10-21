#include <stdio.h>
#include <3ds.h>
#include "menus.h"
#include "nes_routines.h"

/*
 * Source-derived menu dispatcher trace.
 *
 * State 8 is ofs_000_C3BD_08 ($C3BD); state 0 is ofs_000_C346_00
 * ($C346). Selection meanings and original title artwork are deliberately
 * not guessed. The menu receives held controls to match ram_btn_press sampling and the repeat guard; state/reset side effects follow bank_FF.asm.
 */
void menusUpdate(GameState *state, unsigned int heldKeys, unsigned int pressedKeys)
{
    uint8_t nesButtons = 0u;
    uint8_t changed = 0u;

    if (state == 0) return;

    /*
     * vec_C23B_NMI calls sub_D310 once per frame when unpaused. Preserve the
     * $20 divider phase and the $30/$31 timer bytes; other unmodeled timers
     * are zero in this menu-only trace.
     */
    if (!state->paused) {
        uint8_t timers[29] = {0};
        timers[0x30u - 0x21u] = state->menu_timer30;
        timers[0x31u - 0x21u] = state->menu_timer31;
        nesSubD310DecreaseAllTimers(&state->menu_timer20, timers);
        state->menu_timer30 = timers[0x30u - 0x21u];
        state->menu_timer31 = timers[0x31u - 0x21u];
    }

    if (heldKeys & KEY_SELECT) nesButtons |= 0x20u;
    if (heldKeys & KEY_START)  nesButtons |= 0x10u;
    if (heldKeys & KEY_UP)     nesButtons |= 0x08u;
    if (heldKeys & KEY_DOWN)   nesButtons |= 0x04u;
    if (heldKeys & KEY_LEFT)   nesButtons |= 0x02u;
    if (heldKeys & KEY_RIGHT)  nesButtons |= 0x01u;

    state->menu_clear_ram_requested = 0u;

    /*
     * ofs_000_C346_00: when ram_0031_timer is zero, the ASM initializes
     * ram_0043/ram_004A, sets ram_03F7=$0F and enters state 2.
     */
    if (state->menu_asm_state == 0u) {
        if (state->menu_timer31 == 0u) {
            state->menu_ram0043 = 0u;
            state->menu_ram004A = 0u;
            state->menu_ram03F7 = 0x0Fu;
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
            state->menu_asm_state = 2u;
            state->menu_action_x = 0u;
            state->menu_selection_changed = 0u;
            return;
        }

        /* ofs_000_C346_00: the actual three-entry title menu. */
        state->menu_action_x = nesSubC7DEMenuInput(
            nesButtons, 3u, &state->menu_selection,
            &state->menu_repeat_guard, &state->menu_timer30,
            &state->menu_timer31, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            static const uint8_t cursorY[3] = {0x80u, 0x90u, 0xA0u};
            if (state->menu_selection < 3u)
                state->menu_sprite_y = cursorY[state->menu_selection];
            state->menu_ram03F2 = 0u;
        } else {
            uint8_t selected = state->menu_selection;
            state->menu_ram03F7 = 0u;
            state->menu_ram0043 = 0u;
            state->menu_selection = 0u;
            state->menu_ram03F3 = 1u;
            state->menu_ram004A = 1u;
            state->menu_ram03F0 = 1u;
            state->menu_asm_state = (selected == 2u) ? 8u : 1u;
            state->menu_ram03F2 = selected;
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
        }
    } else if (state->menu_asm_state == 1u) {
        /* ofs_000_C395_01 remaps LEFT/RIGHT onto the vertical selector. */
        uint8_t selectorButtons = (uint8_t)((nesButtons & 0xF3u) |
                                           ((nesButtons & 0x03u) << 2));
        state->menu_action_x = nesSubC7DEMenuInput(
            selectorButtons, 5u, &state->menu_selection,
            &state->menu_repeat_guard, &state->menu_timer30,
            &state->menu_timer31, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            static const uint8_t cursorX[5] = {0x48u, 0x60u, 0x78u, 0x90u, 0xA8u};
            if (state->menu_selection < 5u)
                state->menu_sprite_x = cursorX[state->menu_selection];
            state->menu_sprite_attribute = 0u; /* bra_C411 writes zero */
            state->menu_ram0046 = 0u;
        } else {
            state->menu_ram0043 = state->menu_selection;
            nesSubC3AFIncrementState(&state->menu_asm_state);
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
        }
    } else if (state->menu_asm_state == 8u) {
        state->menu_action_x = nesSubC7DEMenuInput(
            nesButtons, 6u, &state->menu_selection,
            &state->menu_repeat_guard, &state->menu_timer30,
            &state->menu_timer31, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            /* tbl_C08B_spr_Y indexed by ram_0042, exactly as bra_C3EA. */
            static const uint8_t cursorY[6] = {
                0x30u, 0x48u, 0x78u, 0x90u, 0xA8u, 0xC0u
            };
            if (state->menu_selection < 6u)
                state->menu_sprite_y = cursorY[state->menu_selection];
        }
        if (state->menu_action_x != 0u) {
            nesSubC3BDConfirmSelection(
                state->menu_action_x, &state->menu_asm_state,
                &state->menu_selection, &state->menu_ram03F2,
                &state->menu_clear_ram_requested,
                &state->menu_ram0040, &state->menu_ram0044);
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

    if (pressedKeys & KEY_X) state->show_help = !state->show_help;
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
        for (i = 0u; i <= 5u; ++i) {
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
