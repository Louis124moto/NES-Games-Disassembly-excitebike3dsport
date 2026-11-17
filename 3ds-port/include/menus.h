#ifndef EXCITEBIKE_MENUS_H
#define EXCITEBIKE_MENUS_H

#include "game_state.h"

void menusUpdate(GameState *state, unsigned int heldKeys, unsigned int pressedKeys);
void menusRender(const GameState *state);
/*
 * Native model of vec_C23B_NMI's PPU work for one frame (call after menusUpdate,
 * i.e. at the end of the frame, like the NMI at vblank): enable rendering when
 * ram_0045 is zero, otherwise execute the static list it selects and clear it.
 * Dynamic buffer ($00) and list $1A are not modelled and leave the PPU untouched.
 */
void menusNmiUpload(GameState *state);

#endif
