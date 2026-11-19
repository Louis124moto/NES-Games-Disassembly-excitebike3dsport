#include <stdio.h>
#include "native_menu.h"


/* Source: sub_C7DE ($C7DE-$C81F). */
uint8_t nativeMenuUpdateSelection(NativeMenuSelectionState *menu,
                               uint8_t pressed,
                               uint8_t entry_count)
{
    uint8_t selection;
    uint8_t confirmed = 0u;

    if (menu == 0 || entry_count == 0u) {
        return 0u;
    }

    selection = menu->selection;
    if ((pressed & 0x3Cu) == 0u) {
        menu->debounce = 0u;
        return 0u;
    }

    menu->input_timer = 0x5Du;
    if (menu->debounce != 0u) {
        return 0u;
    }

    if ((pressed & 0x20u) != 0u) {
        selection = (uint8_t)(selection + 1u);
        if (selection >= entry_count) {
            /* Source falls through from LDY #1 into DEY at $C811. */
            selection = 0u;
        }
        menu->selection_dirty = 1u;
    } else if ((pressed & 0x04u) != 0u) {
        /* Three LSRs in the source expose original bit 2 (NES Down). */
        selection = (uint8_t)(selection + 1u);
        if (selection >= entry_count) {
            selection = 1u;
        }
        menu->selection_dirty = 1u;
    } else if ((pressed & 0x08u) != 0u) {
        /* The fourth LSR exposes original bit 3 (NES Up). */
        selection = (uint8_t)(selection - 1u);
        if ((selection & 0x80u) != 0u) {
            selection = (uint8_t)(entry_count - 1u);
        }
        menu->selection_dirty = 1u;
    } else {
        /* Start is a timed action, not a selection movement. */
        menu->start_timer = 4u;
        confirmed = 1u;
    }

    menu->selection = selection;
    menu->debounce = (uint8_t)(menu->debounce + 1u);
    return confirmed;
}


static void nativeMenuResetSetupSequence(NativeMenuState *menu)
{
    menu->setup_command = 0u;
    menu->transition_phase = 0u;
}

void nativeMenuUpdate(NativeMenuState *menu, uint8_t pressed)
{
    uint8_t confirmed;

    if (menu == 0) {
        return;
    }

    switch (menu->source_state) {
    case 0u: /* ofs_000_C346_00 — title */
        if (menu->input.input_timer == 0u) {
            menu->selected_course = 0u;
            menu->cursor_state = 0u;
            menu->title_flag_03F7 = 0x0Fu;
            menu->source_state = 2u;
            nativeMenuResetSetupSequence(menu);
            return;
        }
        confirmed = nativeMenuUpdateSelection(&menu->input, pressed, 3u);
        if (confirmed != 0u) {
            const uint8_t choice = menu->input.selection;
            menu->selected_course = 0u;
            menu->cursor_state = 1u;
            menu->title_flag_03F7 = 0u;
            menu->input.selection = 0u;
            menu->title_flag_03F3 = 1u;
            menu->title_flag_03F0 = 1u;
            menu->selected_mode = choice;
            menu->source_state = choice == 2u ? 8u : 1u;
            nativeMenuResetSetupSequence(menu);
        }
        break;

    case 1u: /* ofs_000_C395_01 — selector remaps LEFT/RIGHT to UP/DOWN */
    {
        const uint8_t filtered = (uint8_t)(pressed & 0xF3u);
        const uint8_t selector_pressed =
            (uint8_t)(filtered | (uint8_t)((filtered & 0x03u) << 2));
        confirmed = nativeMenuUpdateSelection(&menu->input, selector_pressed, 5u);
        if (confirmed != 0u) {
            menu->selected_course = menu->input.selection;
            menu->source_state = 2u;
            nativeMenuResetSetupSequence(menu);
        }
        break;
    }

    case 8u: /* ofs_000_C3BD_08 — design/options */
        confirmed = nativeMenuUpdateSelection(&menu->input, pressed, 6u);
        if (confirmed != 0u) {
            const uint8_t choice = menu->input.selection;
            if (choice < 2u) {
                menu->selected_mode = choice;
                menu->source_state = 10u;
                nativeMenuResetSetupSequence(menu);
            } else if (choice == 2u) {
                menu->source_state = 9u;
                nativeMenuResetSetupSequence(menu);
            } else if (choice == 3u || choice == 4u) {
                menu->source_state = choice == 3u ? 12u : 13u;
                menu->input.selection = 0u;
                nativeMenuResetSetupSequence(menu);
            } else if (choice == 5u) {
                /* sub_CA85 clears $21-$FF but preserves ram_0042 via the stack. */
                menu->source_state = 0u;
                menu->input.selection = 2u;
                menu->selected_course = 0u;
                menu->input.debounce = 1u;
                menu->input.selection_dirty = 0u;
                menu->input.input_timer = 0u;
                menu->input.start_timer = 0u;
                menu->setup_command = 0u;
                menu->transition_phase = 0u;
                menu->cursor_state = 0u;
            }
        }
        break;

    default:
        /* Race/editor/tape states are implemented by their own native modules. */
        break;
    }
}

void nativeMenuRenderStatus(const GameState *state)
{
    (void)state;
    puts("EXCITEBIKE — NATIVE PORT");
    puts("------------------------");
    puts("The legacy NES runtime has been disconnected.");
    puts("Native menu reconstruction is the next implementation stage.");
}
