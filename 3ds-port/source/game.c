#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "asset_extractor.h"
#include "game_state.h"
#include "input.h"
#include "native_menu.h"
#include "native_menu_assets.h"
#include "native_menu_renderer.h"
#include "native_race_session.h"
#include "renderer.h"

static InputState s_input;
static GameState s_diagnosticState;
static NativeMenuState s_menu;
static NativeMenuAssets s_menuAssets;
static char s_executablePath[512];
static int s_extractorInitialized;
static int s_menuDirty = 1;
static int s_assetsLoaded;
static NativeRaceSession s_raceSession;
static NativeRngState s_raceRng;
static int s_raceSessionReady;

static void rememberExecutablePath(const char *path)
{
    size_t i = 0u;
    if (path == NULL) {
        s_executablePath[0] = '\0';
        return;
    }
    while (i + 1u < sizeof(s_executablePath) && path[i] != '\0') {
        s_executablePath[i] = path[i];
        ++i;
    }
    s_executablePath[i] = '\0';
}

static int menuVisualStateChanged(const NativeMenuState *before,
                                  const NativeMenuState *after)
{
    return before->source_state != after->source_state ||
           before->input.selection != after->input.selection ||
           before->source_visual_asset != after->source_visual_asset ||
           before->setup_command != after->setup_command ||
           before->transition_phase != after->transition_phase ||
           before->race_setup_complete != after->race_setup_complete ||
           before->cursor_sprite_y != after->cursor_sprite_y ||
           before->cursor_sprite_tile != after->cursor_sprite_tile ||
           before->cursor_sprite_attribute != after->cursor_sprite_attribute ||
           before->cursor_sprite_x != after->cursor_sprite_x;
}

void gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    gameStateReset(&s_diagnosticState);
    nativeMenuInit(&s_menu);
    nativeRngReset(&s_raceRng); /* C1CE seed; startup wait-loop advances are still pending. */
    memset(&s_raceSession, 0, sizeof(s_raceSession));
    s_raceSessionReady = 0;
    s_extractorInitialized = 0;
    s_menuDirty = 1;
    memset(&s_menuAssets, 0, sizeof(s_menuAssets));
    rememberExecutablePath(executablePath);
    rendererInit();

    /* Normal game startup consumes only the previously extracted asset pack.
     * It does not load or require rom.nes. X opens the separate extractor tool.
     */
    s_assetsLoaded = nativeMenuAssetsLoad(&s_menuAssets, s_executablePath);
    rendererSetMenuActive(1);
    if (s_assetsLoaded) {
        rendererPrintBottom("EXCITEBIKE — NATIVE MENU\n"
                            "D-pad: move   A/Start: confirm\n"
                            "X: asset extractor\n");
    } else {
        char message[256];
        (void)snprintf(message, sizeof(message),
                       "NATIVE MENU ASSETS NOT READY\n%s\n"
                       "Press X to open the existing asset extractor.\n"
                       "The game itself does not load rom.nes.",
                       s_menuAssets.error[0] != '\0' ? s_menuAssets.error :
                       "Run extraction to create the EBPX and source snapshots.");
        rendererPrintBottom(message);
    }
}

void gameUpdate(void)
{
    NativeMenuState before;
    inputUpdate(&s_input);

    if (assetExtractorIsActive()) {
        assetExtractorUpdate(s_input.pressed);
        if (!assetExtractorIsActive()) {
            s_assetsLoaded = nativeMenuAssetsLoad(&s_menuAssets, s_executablePath);
            rendererSetMenuActive(1);
            s_menuDirty = 1;
            if (s_assetsLoaded) {
                rendererPrintBottom("EXCITEBIKE — NATIVE MENU\n"
                                    "D-pad: move   A/Start: confirm\n"
                                    "X: asset extractor\n");
            } else {
                rendererPrintBottom("Native menu asset pack is incomplete.\n"
                                    "The extractor must export CHR + source snapshots.\n");
            }
        }
        return;
    }

    if ((s_input.pressed & KEY_X) != 0u) {
        if (!s_extractorInitialized) {
            assetExtractorInit(s_executablePath);
            s_extractorInitialized = 1;
        }
        assetExtractorUpdate(KEY_A);
        rendererSetMenuActive(0);
        return;
    }

    before = s_menu;
    nativeMenuTickTimers(&s_menu);
    nativeMenuUpdate(&s_menu, inputNesPressed(&s_input));
    (void)nativeMenuStepSetup(&s_menu);

    /* The source setup list 2 completes C551/C869/C820/C5A3/C566/C875.
     * Compose its native gameplay state once, without starting the still
     * incomplete per-frame race loop or invoking the NES CPU/PPU. */
    if (s_menu.race_setup_complete && !s_raceSessionReady) {
        NativeRaceSetupInput setupInput;
        memset(&setupInput, 0, sizeof(setupInput));
        setupInput.multiplayer_flag = s_menu.selected_mode;
        setupInput.setup_index_0041 = s_menu.source_state;
        setupInput.transition_flag_0046 = s_menu.transition_flag_0046;
        setupInput.timer_0034 = s_menu.transition_timer_0034;
        setupInput.countdown_timer_003C = 0u;
        setupInput.transition_counter_03F8 = s_menu.transition_counter_03F8;
        setupInput.setup_counter_0040 = 0u;
        setupInput.setup_counter_0047 = 0u;
        if (nativeRaceSessionInitFirstCourse(&s_raceSession, &setupInput, &s_raceRng)) {
            s_raceSessionReady = 1;
            rendererPrintBottom("FIRST COURSE — NATIVE SCENE READY\n"
                                "Original background and initial HUD are rendered.\n"
                                "Bike sprites and the race frame loop are next.\n");
        } else {
            rendererPrintBottom("FIRST COURSE — NATIVE SETUP FAILED\n"
                                "Check the first-course setup input state.\n");
        }
    }

    if (menuVisualStateChanged(&before, &s_menu)) s_menuDirty = 1;
}

void gameRender(void)
{
    rendererBeginFrame();
    if (assetExtractorIsActive()) {
        rendererDraw(&s_diagnosticState);
    } else if (s_menuDirty) {
        if (s_raceSessionReady) {
            if (!nativeRaceRenderFrame(s_assetsLoaded ? &s_menuAssets : NULL,
                                       &s_menu, &s_raceSession)) {
                rendererPrintBottom("FIRST COURSE — SCENE ASSETS MISSING\n"
                                    "Re-run the asset extractor for snapshots 01, 14, 17 and 18.\n");
            }
        } else {
            (void)nativeMenuRenderFrame(s_assetsLoaded ? &s_menuAssets : NULL, &s_menu);
        }
        s_menuDirty = 0;
    }
    rendererEndFrame();
}

void gameExit(void)
{
    s_raceSessionReady = 0;
    memset(&s_raceSession, 0, sizeof(s_raceSession));
    nativeMenuAssetsUnload(&s_menuAssets);
    rendererExit();
}
