#include "game.h"
#include "asset_extractor.h"
#include "game_state.h"
#include "input.h"
#include "menus.h"
#include "renderer.h"

/*
 * Runtime routing:
 * - cold boot remains the diagnostic/CHR-inspector screen;
 * - START leaves diagnostics and enters the source-derived menu state machine;
 * - once in menu mode, controller edges are passed to the ASM-derived dispatcher;
 * - menu updates are now in the active 3DSX frame loop.
 *
 * No extra menu controls are invented: B is not a diagnostic escape in menu mode.
 * The native menu model is still incomplete and menusRender currently reports
 * its state rather than reproducing the original PPU image. Do not describe
 * this integration point as a faithful title-screen implementation.
 */
static InputState s_input;
static GameState s_menuState;
static int s_menuActive;

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    s_menuActive = 0;
    gameStateReset(&s_menuState);
    rendererInit();
    assetExtractorInit(executablePath);
}

void gameUpdate(void)
{
    inputUpdate(&s_input);

    if (!s_menuActive) {
        if ((s_input.pressed & KEY_START) != 0u) {
            gameStateReset(&s_menuState);
            s_menuActive = 1;
            rendererSetMenuActive(1);
            return;
        }
        assetExtractorUpdate(s_input.pressed);
        return;
    }

    menusUpdate(&s_menuState, s_input.held, s_input.pressed);
}

void gameRender(void)
{
    rendererBeginFrame();
    rendererDraw(s_menuActive ? &s_menuState : NULL);
    rendererEndFrame();
}

void gameExit(void)
{
    rendererExit();
}
