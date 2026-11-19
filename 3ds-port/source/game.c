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
#include "renderer.h"

static InputState s_input;
static GameState s_diagnosticState;
static NativeMenuState s_menu;
static NativeMenuAssets s_menuAssets;
static char s_executablePath[512];
static int s_extractorInitialized;
static int s_menuDirty = 1;
static int s_assetsLoaded;

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
    if (menuVisualStateChanged(&before, &s_menu)) s_menuDirty = 1;
}

void gameRender(void)
{
    rendererBeginFrame();
    if (assetExtractorIsActive()) {
        rendererDraw(&s_diagnosticState);
    } else if (s_menuDirty) {
        (void)nativeMenuRenderFrame(s_assetsLoaded ? &s_menuAssets : NULL, &s_menu);
        s_menuDirty = 0;
    }
    rendererEndFrame();
}

void gameExit(void)
{
    nativeMenuAssetsUnload(&s_menuAssets);
    rendererExit();
}
