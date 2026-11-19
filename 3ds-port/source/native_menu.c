#include <string.h>
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
        /* Three LSRs expose original bit 2 (NES Down); overflow falls through
         * from LDY #1 into DEY, so the wrapped value is zero. */
        selection = (uint8_t)(selection + 1u);
        if (selection >= entry_count) {
            selection = 0u;
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



void nativeMenuInit(NativeMenuState *menu)
{
    if (menu == 0) {
        return;
    }
    memset(menu, 0, sizeof(*menu));
    menu->source_state = 0u;
    /* ofs_001_C2BD_00_01 initializes the title idle timer to $38. */
    menu->input.input_timer = 0x38u;
    menu->timer_values[16] = 0x38u;
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

    menu->timer_values[15] = menu->input.start_timer;
    menu->timer_values[16] = menu->input.input_timer;

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
        } else {
            /* ofs_000_C346 stores TXA (zero) to ram_03F2 on the idle path. */
            menu->selected_mode = 0u;
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
            } else if (choice == 5u) {
                /* sub_CA85 clears $21-$FF but preserves ram_0042 via the stack. */
                menu->source_state = 0u;
                menu->input.selection = 2u;
                menu->selected_course = 0u;
                menu->input.debounce = 1u;
                menu->input.selection_dirty = 0u;
                menu->input.input_timer = 0u;
                menu->input.start_timer = 0u;
                memset(menu->timer_values, 0, sizeof(menu->timer_values));
                menu->setup_command = 0u;
                menu->transition_phase = 0u;
                menu->cursor_state = 0u;
            } else {
                /* The source adds 9 to any other selection >= 3. */
                menu->source_state = (uint8_t)(choice + 9u);
                menu->input.selection = 0u;
                nativeMenuResetSetupSequence(menu);
            }
        }
        break;

    default:
        /* Race/editor/tape states are implemented by their own native modules. */
        break;
    }

    menu->timer_values[15] = menu->input.start_timer;
    menu->timer_values[16] = menu->input.input_timer;
}


/* Source: ofs_CB3B_00 ($CB3B-$CB75), phase 0 of sub_CB2C.
 * Shared by source states $03, $06 and $0B. */
void nativeMenuTransitionPhase0(NativeMenuState *menu)
{
    uint8_t course;
    uint8_t shifted_counter;

    if (menu == 0) {
        return;
    }

    if (menu->source_state < 0x08u) {
        menu->input.selection = menu->selected_mode;
    }

    if (menu->source_state < 0x08u && menu->transition_gate_0053 < 0x04u) {
        if (menu->transition_flag_0046 != 0u) {
            course = menu->selected_course;
            if (course == 0x04u) {
                menu->title_flag_03F3 =
                    (uint8_t)(menu->title_flag_03F3 + 1u);
                if ((menu->title_flag_03F3 & 1u) == 0u) {
                    shifted_counter =
                        (uint8_t)(menu->transition_counter_03F8 >> 1);
                    if (shifted_counter != 0u) {
                        menu->transition_counter_03F8 = shifted_counter;
                        /* SEC/BCS skips the course INC. */
                    }
                }
            } else if (course < 0x04u) {
                menu->selected_course = (uint8_t)(course + 1u);
            }
        }
        menu->transition_flag_0046 = 1u;
        menu->transition_flag_004B = 1u;
    }

    menu->timer_values[18] = 0x04u; /* ram_0033_timer */
    menu->transition_phase = (uint8_t)(menu->transition_phase + 1u);
}


/* Source: timer-expired branch of ofs_000_C465_04 / ofs_000_C465_07.
 * Executes sub_C3AF followed by the state clamp at $C477-$C484. */
void nativeMenuCompleteTimedTransition(NativeMenuState *menu)
{
    if (menu == 0) {
        return;
    }

    menu->source_state = (uint8_t)(menu->source_state + 1u);
    menu->setup_command = 0u;
    menu->transition_phase = 0u;
    if (menu->source_state >= 0x08u) {
        menu->source_state = 0x02u;
    }
}


/* Source: ofs_CC6B_03 ($CC6B-$CC7C), phase 3 of sub_CB2C. */
void nativeMenuTransitionPhase3(NativeMenuState *menu)
{
    if (menu == 0) {
        return;
    }

    menu->source_state = (uint8_t)(menu->source_state + 1u);
    menu->setup_command = 0u;
    menu->transition_phase = 0u;

    if (menu->transition_flag_004B == 0u &&
        menu->source_state >= 0x09u) {
        menu->source_state = 0x08u;
    }
}


/* Source: sub_D310 ($D310-$D325). Timers $21-$2F tick every frame;
 * timers $30-$3D tick only when the 11-frame divider underflows. */
void nativeMenuTickTimers(NativeMenuState *menu)
{
    unsigned last;
    unsigned i;

    if (menu == 0) {
        return;
    }

    /* Keep the two input timers in the shared source timer bank. */
    menu->timer_values[15] = menu->input.start_timer; /* $0030 */
    menu->timer_values[16] = menu->input.input_timer; /* $0031 */

    menu->frame_timer = (uint8_t)(menu->frame_timer - 1u);
    if ((menu->frame_timer & 0x80u) != 0u) {
        menu->frame_timer = 0x0Au;
        last = 28u;
    } else {
        last = 14u;
    }

    for (i = 0u; i <= last; ++i) {
        if (menu->timer_values[i] != 0u) {
            menu->timer_values[i] = (uint8_t)(menu->timer_values[i] - 1u);
        }
    }

    menu->input.start_timer = menu->timer_values[15];
    menu->input.input_timer = menu->timer_values[16];
}

