#ifndef EXCITEBIKE_NATIVE_MENU_H
#define EXCITEBIKE_NATIVE_MENU_H

#include <stdint.h>\n#include "game_state.h"

/*
 * Native menu presentation hook.
 *
 * The old PPU/6502-driven menu implementation has been removed from the
 * executable. This hook intentionally does not emulate PPU state. The native
 * menu state machine and source-derived menu assets will be implemented here
 * in separately tested slices.
 */
typedef struct NativeMenuSelectionState {
    uint8_t selection;       /* source ram_0042 */
    uint8_t debounce;        /* source ram_0049 */
    uint8_t selection_dirty;  /* source ram_00FF */
    uint8_t input_timer;      /* source ram_0031_timer */
    uint8_t start_timer;      /* source ram_0030_timer */
} NativeMenuSelectionState;

/* Exact native translation of sub_C7DE. pressed uses NES controller bit
 * positions; the 3DS-to-NES input mapping is a separate platform adapter. */
void nativeMenuUpdateSelection(NativeMenuSelectionState *menu,
                               uint8_t pressed,
                               uint8_t entry_count);

void nativeMenuRenderStatus(const GameState *state);

#endif
