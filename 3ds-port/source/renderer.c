#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "renderer.h"
#include "gfx_c3d.h"

/*
 * Top screen: citro3d scene renderer (gfx_c3d.c).
 * Bottom screen: libctru text console, written only when its text changes.
 */
static PrintConsole s_bottomConsole;
static char s_lastBottomText[768];
static int s_initialized;

int rendererInit(void)
{
    if (s_initialized) return 1;
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleInit(GFX_BOTTOM, &s_bottomConsole);
    consoleSelect(&s_bottomConsole);
    s_lastBottomText[0] = '\0';
    if (!gfxC3dInit()) {
        gfxExit();
        return 0;
    }
    s_initialized = 1;
    return 1;
}

void rendererDrawScene(const NativeMenuAssets *assets, const SceneDrawList *list)
{
    if (!s_initialized) return;
    gfxC3dPresent(assets, list);
}

void rendererInvalidateAssets(void)
{
    gfxC3dInvalidateAtlas();
}

void rendererPrintBottom(const char *text)
{
    size_t i;
    if (!s_initialized || text == NULL) return;
    if (strncmp(text, s_lastBottomText, sizeof(s_lastBottomText) - 1u) == 0 &&
        strlen(text) < sizeof(s_lastBottomText)) return;
    consoleSelect(&s_bottomConsole);
    consoleClear();
    fputs(text, stdout);
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_BOTTOM, false);
    for (i = 0u; i + 1u < sizeof(s_lastBottomText) && text[i] != '\0'; ++i)
        s_lastBottomText[i] = text[i];
    s_lastBottomText[i] = '\0';
}

void rendererExit(void)
{
    if (!s_initialized) return;
    gfxC3dExit();
    gfxExit();
    s_initialized = 0;
}
