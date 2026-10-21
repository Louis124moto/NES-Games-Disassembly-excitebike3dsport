#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"
#include "menus.h"
#include "nes_ppu_render.h"
#include "nes_ppu_commands.h"

/*
 * gfxGetFramebuffer reports the top screen as 240x400 memory coordinates,
 * rotated relative to its physical 400x240 presentation. Convert physical
 * screen (x,y) to framebuffer coordinates before applying the 8x8 tile layout.
 */
static size_t topFramebufferOffset(unsigned int screenX, unsigned int screenY)
{
    /* libctru exposes the rotated top framebuffer as 240x400.
       Physical screen (x,y) maps to framebuffer (y, 399-x). */
    const unsigned int fbX = screenY;
    const unsigned int fbY = 399u - screenX;
    const unsigned int tileY = 49u - (fbY >> 3u);
    return ((((fbX >> 3u) * 50u) + tileY) * 64u
            + (fbX & 7u) * 8u + (fbY & 7u)) * 3u;
}

/* Four-level grayscale intentionally exposes the decoded 2bpp pixel indices.
   The true per-tile game palette is not knowable without nametable attributes. */
static const uint8_t s_pixelGray[4] = {0u, 85u, 170u, 255u};

static PrintConsole s_bottomConsole;
static char s_lastLog[512];
static int s_logInitialized;
static int s_gameStarted;
static int s_menuActive;
/* Static storage avoids putting a 184 KiB NES RGB frame on the 3DS stack. */
static uint8_t s_ppuRgb[NES_PPU_RGB_BYTES];

/*
 * Draw a source-derived NES background centered on the 400x240 top screen.
 * This path remains gated until the ASM translation has populated a complete
 * PPU snapshot; it must not render zero-initialized placeholder memory as art.
 */
static int rendererDrawPpuBackground(const GameState *state)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *pixels;
    size_t tileCount = 0u;
    unsigned int x, y;

    if (state == NULL || state->menu_ppu_background_ready == 0u) return 0;
    if (framebuffer == NULL || width != 240u || height != 400u) return 0;
    pixels = assetExtractorGetPixels(&tileCount);
    if (pixels == NULL || nesPpuRenderNametable(
            pixels, tileCount,
            state->menu_ppu_nametables,
            &state->menu_ppu_nametables[NES_PPU_NAMETABLE_BYTES],
            state->menu_ppu_palette, s_ppuRgb, sizeof(s_ppuRgb)) != NES_PPU_RENDER_OK) {
        return 0;
    }

    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    /* NES background is 256 pixels wide; center it in the 400-pixel 3DS display. */
    for (y = 0u; y < NES_PPU_HEIGHT; ++y) {
        for (x = 0u; x < NES_PPU_WIDTH; ++x) {
            const size_t src = ((size_t)y * NES_PPU_WIDTH + x) * 3u;
            const size_t dst = topFramebufferOffset(x + 72u, y);
            framebuffer[dst] = s_ppuRgb[src];
            framebuffer[dst + 1u] = s_ppuRgb[src + 1u];
            framebuffer[dst + 2u] = s_ppuRgb[src + 2u];
        }
    }
    return 1;
}

static void rendererDrawChrAtlas(void)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    size_t firstTile;
    unsigned int tile, row, col;

    /* On libctru top-screen framebuffer dimensions are 240x400. */
    if (s_menuActive) return;
    if (framebuffer == NULL || width != 240u || height != 400u) return;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (!assetExtractorIsActive() || s_gameStarted) return;

    if (assetExtractorGetViewMode() == 1u) {
        uint8_t palette[32], written[32];
        size_t writes = 0u, index;
        if (nesExcitebikePaletteSnapshot(assetExtractorGetPaletteIndex(), palette, written, &writes) != NES_PPU_COMMAND_OK) return;
        for (index = 0u; index < 32u; ++index) {
            const unsigned int cellX = (unsigned int)(index % 8u) * 50u;
            const unsigned int cellY = (unsigned int)(index / 8u) * 60u;
            uint8_t rgb[3] = {64u,64u,64u};
            unsigned int x, y;
            if (written[index] != 0u) (void)nesPpuColorToRgb(palette[index], rgb);
            for (y = 1u; y < 59u; ++y) {
                for (x = 1u; x < 49u; ++x) {
                    const size_t offset = topFramebufferOffset(cellX + x, cellY + y);
                    framebuffer[offset] = rgb[0];
                    framebuffer[offset + 1u] = rgb[1];
                    framebuffer[offset + 2u] = rgb[2];
                }
            }
        }
        return;
    }

    pixels = assetExtractorGetPixels(&tileCount);
    if (pixels == NULL || tileCount == 0u) return;
    /* 2x nearest-neighbour zoom: 375 tiles per page, with D-pad paging. */
    firstTile = assetExtractorGetPage() * 375u;
    if (firstTile >= tileCount) return;
    tileCount -= firstTile;
    if (tileCount > 375u) tileCount = 375u;

    for (tile = 0u; tile < tileCount; ++tile) {
        const size_t sourceTile = firstTile + tile;
        const unsigned int tileX = (tile % 25u) * 16u;
        const unsigned int tileY = (tile / 25u) * 16u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const uint8_t pixel = pixels[sourceTile * 64u + row * 8u + col] & 3u;
                const uint8_t gray = s_pixelGray[pixel];
                unsigned int dy, dx;
                for (dy = 0u; dy < 2u; ++dy) {
                    for (dx = 0u; dx < 2u; ++dx) {
                        const unsigned int x = tileX + col * 2u + dx;
                        const unsigned int y = tileY + row * 2u + dy;
                        const size_t offset = topFramebufferOffset(x, y);
                        framebuffer[offset + 0u] = gray;
                        framebuffer[offset + 1u] = gray;
                        framebuffer[offset + 2u] = gray;
                    }
                }
            }
        }
    }
}

void rendererInit(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, &s_bottomConsole);
    consoleSelect(&s_bottomConsole);
    s_logInitialized = 0;
    s_gameStarted = 0;
    s_menuActive = 0;
    s_lastLog[0] = '\0';
}

void rendererBeginFrame(void)
{
    consoleSelect(&s_bottomConsole);
}

void rendererDraw(const GameState *state)
{
    char logText[512];
    size_t tileCount = 0u;
    (void)assetExtractorGetPixels(&tileCount);
    if (s_menuActive && state != NULL) {
        if (!rendererDrawPpuBackground(state)) {
            consoleSelect(&s_bottomConsole);
            consoleClear();
            menusRender(state);
        } else {
            consoleSelect(&s_bottomConsole);
            consoleClear();
            fputs("EXCITEBIKE 3DS / PPU BACKGROUND\n"
                  "Source-derived nametable rendered.\n"
                  "Sprite/OAM composition is not yet enabled.\n", stdout);
        }
        s_logInitialized = 0;
        return;
    }
    (void)state;
    if (!s_gameStarted && assetExtractorIsActive() && assetExtractorGetViewMode() == 1u) {
        uint8_t palette[32], written[32];
        size_t writes = 0u, index, used;
        const char *sourceLabel = NULL;
        size_t streamSize = 0u;
        (void)nesExcitebikePaletteStream(assetExtractorGetPaletteIndex(), &streamSize, &sourceLabel);
        (void)nesExcitebikePaletteSnapshot(assetExtractorGetPaletteIndex(), palette, written, &writes);
        used = (size_t)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / SOURCE PPU PALETTE\n"
            "------------------------------\n"
            "Source: %s\nWrites replayed: %lu\n"
            "Values: ", sourceLabel != NULL ? sourceLabel : "unknown", (unsigned long)writes);
        for (index = 0u; index < 32u && used < sizeof(logText); ++index) {
            const int writtenCount = snprintf(logText + used, sizeof(logText) - used,
                "%02X%s", palette[index], (index % 8u == 7u) ? "\n       " : " ");
            if (writtenCount < 0 || (size_t)writtenCount >= sizeof(logText) - used) break;
            used += (size_t)writtenCount;
        }
        if (used < sizeof(logText)) {
            const int maskHeader = snprintf(logText + used, sizeof(logText) - used, "\nMask: ");
            if (maskHeader > 0) used += (size_t)maskHeader;
        }
        for (index = 0u; index < 32u && used < sizeof(logText); ++index) {
            const int maskCount = snprintf(logText + used, sizeof(logText) - used,
                "%c%s", written[index] != 0u ? '#' : '-', (index % 8u == 7u) ? "\n      " : " ");
            if (maskCount < 0 || (size_t)maskCount >= sizeof(logText) - used) break;
            used += (size_t)maskCount;
        }
        if (used < sizeof(logText)) {
            (void)snprintf(logText + used, sizeof(logText) - used,
                "\n# = written by this source list; - = untouched.\n"
                "Preset does not identify the active live-game palette.\n"
                "D-pad left/right: palette; up/down: simple/advanced\n"
                "A: export; B: back\n");
        }
        if (!s_logInitialized || strcmp(logText, s_lastLog) != 0) {
            consoleSelect(&s_bottomConsole);
            consoleClear();
            fputs(logText, stdout);
            (void)snprintf(s_lastLog, sizeof(s_lastLog), "%s", logText);
            s_logInitialized = 1;
        }
        return;
    }
    if (s_gameStarted) {
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS\n"
            "------------------------------\n"
            "START received.\n"
            "Native game mode entered.\n"
            "Faithful NES menu/race runtime\n"
            "is not integrated yet.\n"
            "CHR tiles extracted: %lu\n"
            "B: return to asset viewer\n",
            (unsigned long)tileCount);
    } else {
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / ASSET LOG\n"
            "------------------------------\n"
            "CHR: %.36s\n"
            "Tiles: %lu / 960\n"
            "Source: rom.nes or CHR_ROM.chr\n"
            "Exports: CHR, pixels, PNG + 7 source palettes\n"
            "Atlas: %s\n"
            "View: %s; page %lu/%lu\n"
            "D-pad left/right: page/palette\n"
            "D-pad up/down: simple/advanced\n"
            "A: open/export; B: close/re-extract\n"
            "START: menu locked until Stage 1 passes\n"
            "Atlas uses grayscale indices, not game colors.\n",
            assetExtractorGetStatus(), (unsigned long)tileCount,
            assetExtractorIsActive() ? "OPEN" : "closed",
            assetExtractorGetViewMode() == 0u ? "CHR atlas" : "palette diagnostics",
            (unsigned long)(assetExtractorGetPage() + 1u),
            (unsigned long)((tileCount + 374u) / 375u));
    }
    if (!s_logInitialized || strcmp(logText, s_lastLog) != 0) {
        consoleClear();
        fputs(logText, stdout);
        (void)snprintf(s_lastLog, sizeof(s_lastLog), "%s", logText);
        s_logInitialized = 1;
    }
}

void rendererEndFrame(void)
{
    rendererDrawChrAtlas();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void rendererSetGameStarted(int started)
{
    s_gameStarted = started != 0;
    s_logInitialized = 0;
}

void rendererExit(void)
{
    gfxExit();
}

void rendererSetMenuActive(int active)
{
    s_menuActive = active != 0;
    s_logInitialized = 0;
}
