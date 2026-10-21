#include "game.h"
#include "asset_extractor.h"
#include "input.h"
#include "renderer.h"

/*
 * Faithful-baseline runtime.
 *
 * The partial C menu trace and generic bike/track physics are intentionally
 * not executed as gameplay: they are not a complete translation of the NES
 * program. Until the source-driven menu/PPU path and gameplay dependencies
 * are complete, the only enabled UI is the explicitly diagnostic CHR viewer.
 */
static InputState s_input;
static int s_gameStarted;

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    s_gameStarted = 0;
    rendererInit();
    assetExtractorInit(executablePath);
}

void gameUpdate(void)
{
    inputUpdate(&s_input);
    if (!s_gameStarted && (s_input.pressed & KEY_START) != 0u) {
        s_gameStarted = 1;
        rendererSetGameStarted(1);
        return;
    }
    if (s_gameStarted && (s_input.pressed & KEY_B) != 0u) {
        s_gameStarted = 0;
        rendererSetGameStarted(0);
        return;
    }
    if (!s_gameStarted) assetExtractorUpdate(s_input.pressed);
}

void gameRender(void)
{
    rendererBeginFrame();
    rendererDraw(NULL);
    rendererEndFrame();
}

void gameExit(void)
{
    rendererExit();
}
