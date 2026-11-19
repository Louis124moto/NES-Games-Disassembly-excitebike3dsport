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


/* sub_C409: native semantic cursor-sprite setup used by C2BD/C3F3/C575. */
static void nativeMenuSetCursorSprite(NativeMenuState *menu, uint8_t tableIndex)
{
    static const uint8_t cursorX[3] = { 0x50u, 0x48u, 0x4Cu };
    if (tableIndex >= 3u) return;
    menu->cursor_sprite_tile = 0xFAu;
    menu->cursor_sprite_x = cursorX[tableIndex];
    menu->cursor_sprite_attribute = 0u;
    menu->transition_flag_0046 = 0u;
}

/* Source setup commands C2BD, C514, C55E, C3F3 and C575.
 * These update semantic menu state only; the renderer consumes the asset key
 * and cursor instance separately. No PPU command list or OAM memory is built.
 */
uint8_t nativeMenuStepSetup(NativeMenuState *menu)
{
    uint8_t command;
    if (menu == 0 || menu->setup_command != 0u) return 0u;
    if (menu->timer_values[15] != 0u) return 0u; /* sub_C2ED waits for $0030 */

    command = menu->transition_phase;
    switch (menu->source_state) {
    case 0u: /* setup list 0: C2BD, C514 */
        if (command == 0u) {
            menu->title_animation_counter = 0x04u;
            menu->source_visual_asset = 0x02u;
            nativeMenuSetCursorSprite(menu, 0u);
            if (menu->title_animation_flag == 0u) menu->title_animation_flag = 1u;
            menu->title_animation_flag = (uint8_t)(menu->title_animation_flag & 0xFFu);
            menu->title_animation_counter = 0x04u;
            menu->timer_values[16] = 0x38u;
            menu->input.input_timer = 0x38u;
            menu->cursor_sprite_y = 0x80u;
            menu->title_animation_counter = 0x04u;
            menu->transition_phase = 1u;
            return 1u;
        }
        if (command == 1u) {
            menu->source_visual_asset = 0x18u;
            menu->input.debounce = 1u;
            menu->setup_command = 1u;
            menu->transition_phase = 0u;
            return 1u;
        }
        break;

    case 1u: /* setup list 1: C55E, C3F3, C514 */
        if (command == 0u) {
            menu->source_visual_asset = 0x06u;
            menu->transition_phase = 1u;
            return 1u;
        }
        if (command == 1u) {
            menu->title_animation_flag = 0x04u;
            menu->source_visual_asset = 0x03u;
            nativeMenuSetCursorSprite(menu, 1u);
            menu->input.selection = 0u;
            menu->transition_timer_0034 = 0x2Du;
            menu->timer_values[19] = 0x2Du;
            menu->cursor_sprite_y = 0xB0u;
            menu->transition_phase = 2u;
            return 1u;
        }
        if (command == 2u) {
            menu->source_visual_asset = 0x18u;
            menu->input.debounce = 1u;
            menu->setup_command = 1u;
            menu->transition_phase = 0u;
            return 1u;
        }
        break;

    case 8u: /* setup list 6: C55E, C575, C514 */
        if (command == 0u) {
            menu->source_visual_asset = 0x06u;
            menu->transition_phase = 1u;
            return 1u;
        }
        if (command == 1u) {
            menu->selected_course = 0x05u;
            nativeMenuSetCursorSprite(menu, 2u);
            menu->source_visual_asset = 0x0Fu;
            menu->transition_phase = 2u;
            return 1u;
        }
        if (command == 2u) {
            menu->source_visual_asset = 0x18u;
            menu->input.debounce = 1u;
            menu->setup_command = 1u;
            menu->transition_phase = 0u;
            return 1u;
        }
        break;
    default:
        break;
    }
    return 0u;
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


/* Source: ofs_CB77_01 ($CB77-$CBA3), phase-1 timing/state slice only.
 * The seven-byte copy in the original feeds a dynamic PPU upload. Its semantic
 * meaning is unresolved, so this native function deliberately does not invent
 * a tile patch or renderer event. It is not integrated into the live menu.
 */
void nativeMenuTransitionPhase1(NativeMenuState *menu)
{
    if (menu == 0 || menu->transition_phase != 1u) {
        return;
    }

    /* The original handler waits for ram_0033_timer to reach zero. */
    if (menu->timer_values[18] != 0u) {
        return;
    }

    /* Source chooses $20 when ram_004B != 0, otherwise $31. */
    menu->transition_duration = menu->transition_flag_004B != 0u ? 0x20u : 0x31u;
    menu->timer_values[18] = (uint8_t)(menu->transition_duration + 3u);
    menu->transition_phase = 2u;
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

    if (menu->transition_flag_004B == 0u) {
        menu->source_state = menu->source_state >= 0x09u ? 0x08u : 0x00u;
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

