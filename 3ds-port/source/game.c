#include <3ds.h>
#include "game.h"
#include "asset_extractor.h"
#include "game_state.h"
#include "input.h"
#include "renderer.h"

/*
 * The executable currently contains the source-asset inspector only.
 *
 * IMPORTANT: no NES ROM is executed here. The previous nesGame/nesSystem
 * boot path, generated 6502 code, software APU output and PRG-backed menu
 * recompiler have been disconnected from the live application. The native
 * game loop will be integrated subsystem-by-subsystem after source tracing
 * and differential tests are in place.
 */
static InputState s_input;
static GameState s_diagnosticState;

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    gameStateReset(&s_diagnosticState);
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
    rendererDraw(&s_diagnosticState);
    rendererEndFrame();
}

void gameExit(void)
{
    rendererExit();
}
