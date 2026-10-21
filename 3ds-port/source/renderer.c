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
    (void)state;
    printf("EXCITEBIKE 3DS - FAITHFUL PORT WORKSPACE\n");
    printf("========================================\n");
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
