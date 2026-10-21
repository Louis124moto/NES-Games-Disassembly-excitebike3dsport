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
    printf("EXCITEBIKE 3DS - PORT DEVELOPMENT\n");
    printf("==================================\n");
    printf("Menu input: ASM-derived sub_C7DE model\n");
    printf("Cursor: %u | action X: %u\n",
           (unsigned int)state->menu_selection,
           (unsigned int)state->menu_action_x);
    printf("This screen is a trace prototype, not NES artwork.\n");
    printf("Original title graphics and menu labels remain pending.\n");
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
