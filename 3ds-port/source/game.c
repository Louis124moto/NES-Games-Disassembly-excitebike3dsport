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

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    rendererInit();
    assetExtractorInit(executablePath);
}

void gameUpdate(void)
{
    inputUpdate(&s_input);
    assetExtractorUpdate(s_input.pressed);
}

void gameRender(void)
{
    rendererBeginFrame();
    if (assetExtractorIsActive()) rendererDraw(NULL);
    rendererEndFrame();
}

void gameExit(void)
{
    rendererExit();
}
