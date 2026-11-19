#ifndef EXCITEBIKE_NATIVE_MENU_H
#define EXCITEBIKE_NATIVE_MENU_H

#include "game_state.h"

/*
 * Native menu presentation hook.
 *
 * The old PPU/6502-driven menu implementation has been removed from the
 * executable. This hook intentionally does not emulate PPU state. The native
 * menu state machine and source-derived menu assets will be implemented here
 * in separately tested slices.
 */
void nativeMenuRenderStatus(const GameState *state);

#endif
