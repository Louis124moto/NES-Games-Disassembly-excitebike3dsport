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
    if (state == 0) {
        return;
    }

    printf("EXCITEBIKE 3DS - PORT SCAFFOLD\n");
    printf("==============================\n");
    printf("Architecture / input test\n\n");
    printf("Speed: %d\n", state->bike.speed);
    printf("Heat:  %d\n", state->bike.temperature);
    printf("Track position: %d\n\n", state->bike.track_position);
    printf("A: throttle   B: boost (temporary test)\n");
    printf("D-pad: position test\n");
    printf("START: quit\n");
    printf("\nThis is NOT authentic gameplay yet.\n");
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
