#include <stdio.h>
#include <3ds.h>
#include "renderer.h"

void rendererInit(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, NULL);
}

void rendererBeginFrame(void)
{
    consoleClear();
}

void rendererDraw(const GameState *state)
{
    if (state == 0) return;
    printf("EXCITEBIKE 3DS - ARCHITECTURE TEST\n");
    printf("==================================\n");
    printf("Speed: %d\nHeat: %d\nTrack position: %d\n\n",
           state->bike.speed, state->bike.temperature, state->bike.track_position);
    printf("A: throttle | B: temporary boost\n");
    printf("D-pad: position test | X: help toggle\n");
    printf("START: quit\n\n");
    printf("This is not authentic gameplay yet.\n");
}

void rendererEndFrame(void)
{
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void rendererExit(void)
{
    gfxExit();
}
