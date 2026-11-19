#include <3ds.h>
#include "game.h"
#include "menu_tape_file.h"
#include "asset_extractor.h"
#include "game_state.h"
#include "input.h"
#include "menus.h"
#include "renderer.h"
#include "nes_menu_cpu.h"
#include "nes_game.h"
#include "nes_audio_out.h"

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
static int s_nesGameActive;      /* faithful path: original program running on the NES system layer */
static unsigned s_leaveHold;     /* port-only L+R hold counter to leave the faithful path */

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    s_menuActive = 0;
    s_nesGameActive = 0;
    s_leaveHold = 0u;
    gameStateReset(&s_menuState);
    rendererInit();
    assetExtractorInit(executablePath);
    (void)nesMenuCpuInit(executablePath); /* Source-recompiled sub_C7DE when rom.nes is present. */
    (void)nesGameInit(executablePath);    /* Faithful path: needs rom.nes with the disassembled PRG. */
    nesAudioOutInit();
}

void gameUpdate(void)
{
    inputUpdate(&s_input);

    if (s_nesGameActive) {
        /* Port-only escape (not in the original): hold L+R for about one second to return to the diagnostics. */
        if ((s_input.held & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) {
            if (++s_leaveHold >= 60u) { s_nesGameActive = 0; s_leaveHold = 0u; rendererPrintBottom("Faithful game stopped.\nSTART: diagnostics/menu path\n"); }
        } else {
            s_leaveHold = 0u;
        }
        if (s_nesGameActive && nesGameFrame(s_input.held)) {
            unsigned count = 0u;
            const int16_t *audio = nesGameAudio(&count);
            nesAudioOutPush(audio, count);
        } else if (s_nesGameActive) {
            s_nesGameActive = 0;
            rendererPrintBottom("Recompiled program reached an unknown address.\nFaithful game stopped.\n");
        }
        return;
    }

    if (!s_menuActive) {
        /*
         * Stage 1 was accepted by the owner on 2026-11-17 (see TODO.md). START
         * enters the Stage 2 original-menu path, but only once the CHR source
         * is loaded: the menu model needs real tiles and must not run on air.
         * The press is consumed here so the menu does not see it as a NES press.
         */
        if ((s_input.pressed & KEY_SELECT) && nesGameAvailable()) {
            /* Faithful game: the original program from its reset vector (see nes_game.h). */
            nesGameBoot();
            s_nesGameActive = 1;
            s_leaveHold = 0u;
            rendererPrintBottom("Excitebike (original program)\nD-pad/A/B/SELECT/START = NES pad\nHold L+R 1 s: leave (port only)\nAudio needs dspfirm.cdc; no tape yet\n");
            return;
        }
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
    if (s_nesGameActive) {
        rendererPresentNesFrame(nesGameFramebuffer());
        return;
    }
    rendererBeginFrame();
    rendererDraw(&s_menuState);
    rendererGetSprite0Hit(&s_menuState.menu_sprite0_hit_found,
                          &s_menuState.menu_sprite0_hit_cycle);
    s_menuState.menu_sprite0_hit_last_frame = s_menuState.menu_sprite0_hit_found;
    rendererEndFrame();
}

void gameExit(void)
{
    nesAudioOutExit();
    rendererExit();
}
