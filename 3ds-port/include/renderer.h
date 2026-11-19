#ifndef EXCITEBIKE_RENDERER_H
#define EXCITEBIKE_RENDERER_H

#include <stdint.h>
#include "game_state.h"

void rendererInit(void);
void rendererBeginFrame(void);
void rendererDraw(const GameState *state);
void rendererGetSprite0Hit(uint8_t *found, uint32_t *cycleFromPrerender);
void rendererEndFrame(void);
void rendererSetGameStarted(int started);
/* Faithful game path: show a complete 256x240 RGB frame (row-major) on the top screen and wait for vblank. */
void rendererPresentNesFrame(const uint8_t *rgb);
void rendererPrintBottom(const char *text);
void rendererSetMenuActive(int active);
void rendererExit(void);

#endif
