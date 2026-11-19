#include <stdio.h>
#include "native_menu.h"


/* Source: sub_C7DE ($C7DE-$C81F). */
void nativeMenuUpdateSelection(NativeMenuSelectionState *menu,
                               uint8_t pressed,
                               uint8_t entry_count)
{
    uint8_t selection;

    if (menu == 0 || entry_count == 0u) {
        return;
    }

    selection = menu->selection;
    if ((pressed & 0x3Cu) == 0u) {
        menu->debounce = 0u;
        return;
    }

    menu->input_timer = 0x5Du;
    if (menu->debounce != 0u) {
        return;
    }

    if ((pressed & 0x20u) != 0u) {
        selection = (uint8_t)(selection + 1u);
        if (selection >= entry_count) {
            selection = 1u;
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
        if (selection == 0u) {
            selection = (uint8_t)(entry_count - 1u);
        } else {
            selection = (uint8_t)(selection - 1u);
        }
        menu->selection_dirty = 1u;
    } else {
        /* Start is a timed action, not a selection movement. */
        menu->start_timer = 4u;
    }

    menu->selection = selection;
    menu->debounce = (uint8_t)(menu->debounce + 1u);
}

void nativeMenuRenderStatus(const GameState *state)
{
    (void)state;
    puts("EXCITEBIKE — NATIVE PORT");
    puts("------------------------");
    puts("The legacy NES runtime has been disconnected.");
    puts("Native menu reconstruction is the next implementation stage.");
}
