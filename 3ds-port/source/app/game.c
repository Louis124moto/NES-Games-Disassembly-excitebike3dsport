#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "game.h"
#include "asset_extractor.h"
#include "input.h"
#include "native_menu.h"
#include "native_menu_assets.h"
#include "native_scene.h"
#include "native_race_session.h"
#include "race_session_world.h"
#include "race_results.h"
#include "race_loop.h"
#include "renderer.h"
#include "audio.h"

static InputState s_input;
static NativeMenuState s_menu;
static NativeMenuAssets s_menuAssets;
static char s_executablePath[512];
static int s_extractorInitialized;
static SceneDrawList s_scene;
static int s_assetsLoaded;
static NativeRaceSession s_raceSession;
static RaceWorld s_raceWorld;
static uint8_t s_raceMultiplayer;
static NativeRngState s_raceRng;
static int s_raceSessionReady;
static int s_raceFinishLatched;
static uint32_t s_raceUnsupportedMask;
static uint8_t s_raceRecords[0x40];


#define RACE_RECORD_PATH "sdmc:/3ds/excitebike/records.bin"

static void loadRaceRecords(void)
{
    FILE *file;
    memset(s_raceRecords, 0xFF, sizeof(s_raceRecords));
    file = fopen(RACE_RECORD_PATH, "rb");
    if (file == NULL) return;
    (void)fread(s_raceRecords, 1u, sizeof(s_raceRecords), file);
    fclose(file);
}

static void saveRaceRecords(void)
{
    FILE *file;
    (void)mkdir("sdmc:/3ds", 0777);
    (void)mkdir("sdmc:/3ds/excitebike", 0777);
    file = fopen(RACE_RECORD_PATH, "wb");
    if (file == NULL) return;
    (void)fwrite(s_raceRecords, 1u, sizeof(s_raceRecords), file);
    fclose(file);
}

static void applyRaceRecordsToWorld(void)
{
    memcpy(raceWorldByte(&s_raceWorld, 0x580u),
           s_raceRecords, sizeof(s_raceRecords));
}

/* C485/C53E/CB06: compare the race time with the saved record selected by
 * course, $0046 variant and multiplayer mode. The source's record window is
 * $0580-$05BF (64 bytes); one entry is three time bytes. */
static void buildRaceResultTimes(void)
{
    uint8_t course = *raceWorldByte(&s_raceWorld, 0x43u);
    uint8_t offset;
    uint8_t *record;
    uint8_t current[3];
    unsigned i;
    int comparison;

    if (course > 4u) course = 0u;
    offset = (uint8_t)(course * 3u);
    if (*raceWorldByte(&s_raceWorld, 0x46u) != 0u)
        offset = (uint8_t)(offset | 0x10u);
    if (*raceWorldByte(&s_raceWorld, 0x3F2u) != 0u)
        offset = (uint8_t)(offset + 0x23u);
    if ((unsigned)offset + 3u > sizeof(s_raceRecords)) offset = 0u;

    record = &s_raceRecords[offset];
    current[0] = *raceWorldByte(&s_raceWorld, 0x68u);
    current[1] = *raceWorldByte(&s_raceWorld, 0x69u);
    current[2] = *raceWorldByte(&s_raceWorld, 0x6Au);
    comparison = memcmp(record, current, sizeof(current));
    if (comparison > 0) {
        memcpy(record, current, sizeof(current));
        memcpy(raceWorldByte(&s_raceWorld, 0x580u),
               s_raceRecords, sizeof(s_raceRecords));
        saveRaceRecords();
        *raceWorldByte(&s_raceWorld, 0x53u) = 1u;
    } else if (comparison == 0) {
        *raceWorldByte(&s_raceWorld, 0x53u) = 1u;
    } else {
        /* C4AD/C522: first-course threshold from tbl_C091. sub_CB13
         * selects index 0, or index $10 when $0046 is set. Only the
         * <4 / >=4 distinction affects the native first-course transition. */
        const uint8_t threshold[3] = {
            *raceWorldByte(&s_raceWorld, 0x46u) != 0u ? 0x0Bu : 0x01u,
            *raceWorldByte(&s_raceWorld, 0x46u) != 0u ? 0x0Du : 0x10u,
            *raceWorldByte(&s_raceWorld, 0x46u) != 0u ? 0x0Fu : 0x00u
        };
        const int thresholdComparison = memcmp(current, threshold, sizeof(current));
        *raceWorldByte(&s_raceWorld, 0x53u) =
            thresholdComparison <= 0 ? 2u : 4u;
    }

    s_raceSession.result_record_glyphs[0] = (uint8_t)(record[0] % 10u);
    s_raceSession.result_record_glyphs[1] = 0xFBu;
    s_raceSession.result_record_glyphs[2] = (uint8_t)(record[1] / 10u);
    s_raceSession.result_record_glyphs[3] = (uint8_t)(record[1] % 10u);
    s_raceSession.result_record_glyphs[4] = 0xFBu;
    s_raceSession.result_record_glyphs[5] = (uint8_t)(record[2] / 10u);
    s_raceSession.result_record_glyphs[6] = (uint8_t)(record[2] % 10u);
    for (i = 0u; i < 7u; ++i)
        s_raceSession.result_current_glyphs[i] =
            *raceWorldByte(&s_raceWorld, 0x3D9u + i);
}

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

int gameInit(const char *executablePath)
{
    s_input.held = 0u;
    s_input.pressed = 0u;
    nativeMenuInit(&s_menu);
    loadRaceRecords();
    nativeRngReset(&s_raceRng); /* C1CE seed; startup wait-loop advances are still pending. */
    memset(&s_raceSession, 0, sizeof(s_raceSession));
    s_raceSessionReady = 0;
    s_raceFinishLatched = 0;
    s_raceUnsupportedMask = 0u;
    s_extractorInitialized = 0;
    memset(&s_menuAssets, 0, sizeof(s_menuAssets));
    rememberExecutablePath(executablePath);
    if (!rendererInit()) return 0;
    audioInit();

    /* Normal game startup consumes only the previously extracted asset pack.
     * It does not load or require rom.nes. X opens the separate extractor tool.
     */
    s_assetsLoaded = nativeMenuAssetsLoad(&s_menuAssets, s_executablePath);
    rendererInvalidateAssets();
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
    return 1;
}

void gameUpdate(void)
{
    inputUpdate(&s_input);
    audioUpdate();

    if (assetExtractorIsActive()) {
        assetExtractorUpdate((s_input.pressed & KEY_A) != 0u, (s_input.pressed & KEY_B) != 0u);
        if (!assetExtractorIsActive()) {
            s_assetsLoaded = nativeMenuAssetsLoad(&s_menuAssets, s_executablePath);
            rendererInvalidateAssets();
            if (s_assetsLoaded) {
                rendererPrintBottom("EXCITEBIKE — NATIVE MENU\n"
                                    "D-pad: move   A/Start: confirm\n"
                                    "X: asset extractor\n");
            } else {
                rendererPrintBottom("Native menu asset pack is incomplete.\n"
                                    "The extractor must export CHR + source snapshots.\n");
            }
        } else {
            char text[512];
            (void)snprintf(text, sizeof(text),
                           "EXCITEBIKE 3DS — ASSET EXTRACTOR\n"
                           "------------------------------\n"
                           "ROM: %.44s\n"
                           "%.100s\n"
                           "CHR tiles: %lu\n\n"
                           "A: extract / export again\n"
                           "B: close and reload the game assets\n",
                           assetExtractorGetRomIdentitySummary(),
                           assetExtractorGetStatus(),
                           (unsigned long)assetExtractorGetTotalTiles());
            rendererPrintBottom(text);
        }
        return;
    }

    /* Once the first-course session starts, the menu state machine must stop.
     * Otherwise menu timers/setup commands continue advancing underneath the race
     * and can consume gameplay inputs or mutate the selected mode/state. */
    if (s_raceSessionReady) {
        if (s_raceFinishLatched) {
            /* States 3/6/$0B: sub_CB2C on the race world (CB3B/CB77/CBA4/CC6B),
             * verified byte-for-byte against the original code. The NMI
             * prefix (timers, $003F) runs first; the PPU command buffer at
             * $0300 is drained by the renderer, so $0300 is cleared like NMI. */
            *raceWorldByte(&s_raceWorld, 0x14u) = inputNesHeld(&s_input);
            *raceWorldByte(&s_raceWorld, 0x15u) = inputNesPressed(&s_input);
            raceLoopNmiTick(&s_raceWorld);
            if (raceResultsPhaseStep(&s_raceWorld) != 0u)
                *raceWorldByte(&s_raceWorld, 0x41u) = 0u; /* out of model: leave */
            *raceWorldByte(&s_raceWorld, 0x300u) = 0u;
            memcpy(s_raceSession.oam.bytes, raceWorldByte(&s_raceWorld, 0x200u),
                   sizeof(s_raceSession.oam.bytes));
            audioSubmitSourceEvents(0u, *raceWorldByte(&s_raceWorld, 0xFBu), 0u, 0u, 0u);
            *raceWorldByte(&s_raceWorld, 0xFBu) = 0u;
            {
                const uint8_t st = *raceWorldByte(&s_raceWorld, 0x41u);
                s_menu.transition_phase = (st == 3u || st == 6u || st == 0x0Bu) ? 0u : 3u;
                s_menu.source_state = (st == 3u || st == 6u || st == 0x0Bu) ? 3u : st;
            }
            if (s_menu.transition_phase == 3u ||
                0) {
                /* The next course is not yet playable. Finish the original
                 * results interval, then expose the native selector. */
                const uint8_t completedState = s_menu.source_state;
                const uint8_t nextCourse = s_menu.selected_course;
                const uint8_t returnToTitle = (uint8_t)(completedState == 0u);
                s_raceSession.result_active = 0u;
                s_raceSession.result_podium_active = 0u;
                s_raceSessionReady = 0;
                s_raceFinishLatched = 0;
                s_raceUnsupportedMask = 0u;
                audioSubmitSourceEvents(0u, 0u, 0u, 0u, 0u);
                nativeMenuInit(&s_menu);
                s_menu.selected_mode = s_raceMultiplayer;
                s_menu.selected_course = nextCourse;
                if (returnToTitle == 0u) {
                    s_menu.source_state = 1u;
                    s_menu.input.selection = nextCourse;
                } else {
                    s_menu.source_state = 0u;
                }
                nativeMenuUpdate(&s_menu, 0u);
                rendererPrintBottom(returnToTitle != 0u
                    ? "RACE RESULTS COMPLETE — SOURCE TITLE RETURN\n"
                    : "RACE RESULTS COMPLETE — NEXT COURSE SELECTOR\n"
                      "D-pad: choose course   A/Start: confirm\n"
                      "X: asset extractor\n");
            }
            return;
        }

        const uint32_t previousUnsupported = s_raceUnsupportedMask;
        s_raceUnsupportedMask |= nativeRaceSessionStepFrame(
            &s_raceSession, &s_raceWorld,
            inputNesHeld(&s_input), inputNesPressed(&s_input));
        audioSubmitSourceEvents(*raceWorldByte(&s_raceWorld, 0xFCu),
                                *raceWorldByte(&s_raceWorld, 0xFBu),
                                *raceWorldByte(&s_raceWorld, 0xFDu),
                                *raceWorldByte(&s_raceWorld, 0xFEu),
                                *raceWorldByte(&s_raceWorld, 0xFFu));
        audioSubmitApuEnable(*raceWorldByte(&s_raceWorld, 0x4015u));
        *raceWorldByte(&s_raceWorld, 0xFBu) = 0u;
        *raceWorldByte(&s_raceWorld, 0xFDu) = 0u;
        *raceWorldByte(&s_raceWorld, 0xFEu) = 0u;
        *raceWorldByte(&s_raceWorld, 0xFFu) = 0u;
        /* C3AF increments $0041 when the source finish transition completes.
         * Latch that state exactly once, as the original state dispatcher does. */
        if (*raceWorldByte(&s_raceWorld, 0x41u) != 2u &&
            *raceWorldByte(&s_raceWorld, 0x52u) == 1u) {
            s_raceFinishLatched = 1;
            buildRaceResultTimes();
            s_raceSession.result_active = 1u;
            s_menu.source_state = 3u;
            s_menu.transition_phase = 0u;
            s_menu.transition_gate_0053 = *raceWorldByte(&s_raceWorld, 0x53u);
            s_menu.transition_flag_0046 = *raceWorldByte(&s_raceWorld, 0x46u);
            s_menu.transition_flag_004B = *raceWorldByte(&s_raceWorld, 0x4Bu);
            s_menu.selected_mode = s_raceMultiplayer;
            audioSubmitSourceEvents(0u, 0u, 0u, 0u, 0u);
            rendererPrintBottom("RACE RESULTS — SOURCE TIME DISPLAY\n"
                                "The stored time and finished-race time are shown.\n"
                                "Press a button to continue.\n");
        }
        /* Keep out-of-domain source states observable instead of silently
         * presenting a partially simulated frame as a verified result. */
        if (s_raceUnsupportedMask != previousUnsupported) {
            char warning[160];
            (void)snprintf(warning, sizeof(warning),
                           "NATIVE RACE: unsupported stage mask %08lX\n"
                           "Simulation continues for diagnosis; frame is not fully verified.",
                           (unsigned long)s_raceUnsupportedMask);
            rendererPrintBottom(warning);
        }
        return;
    }

    if ((s_input.pressed & KEY_X) != 0u) {
        if (!s_extractorInitialized) {
            assetExtractorInit(s_executablePath);
            s_extractorInitialized = 1;
        }
        assetExtractorUpdate(1, 0);
        return;
    }

    nativeMenuTickTimers(&s_menu);
    nativeMenuUpdate(&s_menu, inputNesPressed(&s_input));
    (void)nativeMenuStepSetup(&s_menu);

    /* Source $00FB music requests raised by the title/menu setup code (C2D1,
     * C514...) go to the sound driver, which consumes them once. */
    audioSubmitApuEnable(0x0Fu);     /* menu/title code (C2E3) keeps all APU channels on */
    if (s_menu.title_animation_flag != 0u) {
        audioSubmitSourceEvents(0u, s_menu.title_animation_flag, 0u, 0u, 0u);
        s_menu.title_animation_flag = 0u;
    }

    /* The source setup list 2 completes C551/C869/C820/C5A3/C566/C875.
     * Compose the semantic native world once. Subsequent gameUpdate calls
     * advance raceLoopStep through nativeRaceSessionStepFrame; no NES CPU/PPU
     * or emulator is used by the shipped runtime. */
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
            s_raceFinishLatched = 0;
            s_raceUnsupportedMask = 0u;
            s_raceMultiplayer = s_menu.selected_mode;
            nativeRaceSessionBuildWorld(&s_raceSession, s_raceMultiplayer, &s_raceWorld);
            applyRaceRecordsToWorld();
            nativeSceneRaceReset();
            rendererPrintBottom("FIRST COURSE — NATIVE SIMULATION\n"
                                "Native bike movement, opponent, collision and course-stream stages run.\n"
                                "F4FF columns feed the native renderer; source time results, podium and timed transition are implemented.\n");
        } else {
            rendererPrintBottom("FIRST COURSE — NATIVE SETUP FAILED\n"
                                "Check the first-course setup input state.\n");
        }
    }

}

void gameRender(void)
{
    const NativeMenuAssets *assets = s_assetsLoaded ? &s_menuAssets : NULL;
    s_scene.count = 0u;
    if (!assetExtractorIsActive()) {
        if (s_raceSessionReady) {
            if (!nativeSceneBuildRace(&s_scene, assets, &s_menu, &s_raceSession)) {
                rendererPrintBottom("FIRST COURSE — SCENE ASSETS MISSING\n"
                                    "Re-run the asset extractor for snapshots 01, 14, 17 and 18.\n");
            }
        } else {
            (void)nativeSceneBuildMenu(&s_scene, assets, &s_menu);
        }
    }
    rendererDrawScene(assets, &s_scene);
}

void gameExit(void)
{
    s_raceSessionReady = 0;
    memset(&s_raceSession, 0, sizeof(s_raceSession));
    nativeMenuAssetsUnload(&s_menuAssets);
    rendererExit();
    audioExit();
}
