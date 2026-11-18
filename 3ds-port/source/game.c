#include <3ds.h>
#include "game.h"
#include "menu_tape_file.h"
#include "asset_extractor.h"
#include "game_state.h"
#include "input.h"
#include "menus.h"
#include "renderer.h"

/*
 * Runtime routing:
 * - cold boot remains the diagnostic/CHR-inspector screen;
 * - START (once CHR is loaded) leaves diagnostics and enters the source-derived
 *   menu state machine plus the native NMI/PPU upload model (menusNmiUpload);
 * - once in menu mode, controller edges are passed to the ASM-derived dispatcher;
 * - menu updates are now in the active 3DSX frame loop;
 * - the menu GameState is always passed to rendererDraw so the active
 *   stream/OAM trace view can report live $0041/$0045/OAM state even while
 *   the diagnostic viewer is on screen.
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
        /*
         * Stage 1 was accepted by the owner on 2026-11-17 (see TODO.md). START
         * enters the Stage 2 original-menu path, but only once the CHR source
         * is loaded: the menu model needs real tiles and must not run on air.
         * The press is consumed here so the menu does not see it as a NES press.
         */
        if ((s_input.pressed & KEY_START) && assetExtractorGetTotalTiles() > 0u) {
            s_menuActive = 1;
            gameStateReset(&s_menuState);
            menusSetTapeIo(menuTapeFileIo()); /* data-recorder stand-in for the save/load states */
            rendererSetMenuActive(1);
            return;
        }
        assetExtractorUpdate(s_input.pressed);
        return;
    }

    /*
     * Port-only escape, NOT in the original: with no recorder signal the original main thread
     * waits forever with NMI off (frozen picture). With no saved track file the load state
     * would trap the player the same way, so B leaves the menu path instead.
     */
    if (menusTapeWaiting(&s_menuState) && (s_input.pressed & KEY_B)) {
        s_menuActive = 0;
        rendererSetMenuActive(0);
        return;
    }

    menusNmiFrame(&s_menuState, s_input.held, s_input.pressed); /* one vec_C23B_NMI per frame */
}

void gameRender(void)
{
    rendererBeginFrame();
    rendererDraw(&s_menuState);
    rendererEndFrame();
}

void gameExit(void)
{
    rendererExit();
}
