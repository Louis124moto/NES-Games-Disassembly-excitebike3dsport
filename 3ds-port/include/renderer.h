#ifndef EXCITEBIKE_RENDERER_H
#define EXCITEBIKE_RENDERER_H

#include "game_state.h"

void rendererInit(void);
void rendererBeginFrame(void);
void rendererDraw(const GameState *state);
void rendererEndFrame(void);
void rendererSetGameStarted(int started);
void rendererSetMenuActive(int active);
void rendererExit(void);

#endif
