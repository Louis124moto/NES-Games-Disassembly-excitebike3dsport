#ifndef EXCITEBIKE_MENUS_H
#define EXCITEBIKE_MENUS_H

#include "game_state.h"

void menusUpdate(GameState *state, unsigned int heldKeys, unsigned int pressedKeys);
void menusRender(const GameState *state);

#endif
