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
static uint8_t s_sourcePpuRgb[NES_PPU_RGB_BYTES];
static uint8_t s_sourceNametable[NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES];
static uint8_t s_sourceNametableMask[NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES];
static uint8_t s_sourcePalette[32];
static uint8_t s_sourcePaletteMask[32];
static uint8_t s_sourceNametableView[NES_PPU_NAMETABLE_BYTES];
static uint8_t s_sourceAttributes[NES_PPU_ATTRIBUTE_BYTES];
static uint8_t s_sourcePaletteView[NES_PPU_PALETTE_BYTES];
static uint8_t s_unknownTiles[NES_PPU_NAMETABLE_BYTES];

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
    pixels = assetExtractorGetPixels(&tileCount);

    if (assetExtractorGetViewMode() == 2u) {
        size_t writes = 0u, tile, pixelX, pixelY;
        const size_t streamIndex = assetExtractorGetPpuStreamIndex();
        if (nesExcitebikePpuSnapshot(streamIndex, s_sourceNametable, s_sourceNametableMask,
                                     s_sourcePalette, s_sourcePaletteMask, &writes) != NES_PPU_COMMAND_OK) return;
        memcpy(s_sourceNametableView, s_sourceNametable, sizeof(s_sourceNametableView));
        memcpy(s_sourceAttributes, s_sourceNametable + 0x3C0u, sizeof(s_sourceAttributes));
        memcpy(s_sourcePaletteView, s_sourcePalette, sizeof(s_sourcePaletteView));
        for (tile = 0u; tile < NES_PPU_NAMETABLE_BYTES; ++tile) {
            const size_t tileX = tile % 32u;
            const size_t tileY = tile / 32u;
            const size_t attributeIndex = 0x3C0u + (tileY / 4u) * 8u + (tileX / 4u);
            const unsigned int shift = (unsigned int)(((tileY & 2u) << 1u) | (tileX & 2u));
            const unsigned int paletteGroup = (s_sourceAttributes[(tileY / 4u) * 8u + (tileX / 4u)] >> shift) & 3u;
            size_t entry;
            s_unknownTiles[tile] = (uint8_t)(s_sourceNametableMask[tile] == 0u ||
                                               s_sourceNametableMask[attributeIndex] == 0u);
            if (pixels == NULL || tileCount == 0u || s_sourceNametableView[tile] >= tileCount) {
                s_sourceNametableView[tile] = 0u;
                s_unknownTiles[tile] = 1u;
            }
            for (entry = 0u; entry < 4u; ++entry) {
                if (s_sourcePaletteMask[paletteGroup * 4u + entry] == 0u)
                    s_unknownTiles[tile] = 1u;
            }
        }
        if (pixels == NULL || tileCount == 0u ||
            nesPpuRenderNametable(pixels, tileCount, s_sourceNametableView,
                                  s_sourceAttributes, s_sourcePaletteView,
                                  s_sourcePpuRgb, sizeof(s_sourcePpuRgb)) != NES_PPU_RENDER_OK) {
            memset(s_unknownTiles, 1, sizeof(s_unknownTiles));
        }
        for (pixelY = 0u; pixelY < 240u; ++pixelY) {
            for (pixelX = 0u; pixelX < 256u; ++pixelX) {
                const size_t tileIndex = (pixelY / 8u) * 32u + pixelX / 8u;
                const size_t sourceOffset = (pixelY * 256u + pixelX) * 3u;
                const size_t destOffset = topFramebufferOffset((unsigned int)pixelX + 72u, (unsigned int)pixelY);
                if (s_unknownTiles[tileIndex] != 0u) {
                    const uint8_t shade = ((pixelX / 2u + pixelY / 2u) & 1u) != 0u ? 192u : 64u;
                    framebuffer[destOffset] = shade;
                    framebuffer[destOffset + 1u] = shade;
                    framebuffer[destOffset + 2u] = shade;
                } else {
                    framebuffer[destOffset] = s_sourcePpuRgb[sourceOffset];
                    framebuffer[destOffset + 1u] = s_sourcePpuRgb[sourceOffset + 1u];
                    framebuffer[destOffset + 2u] = s_sourcePpuRgb[sourceOffset + 2u];
                }
            }
        }
        return;
    }

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
    if (!s_gameStarted && assetExtractorIsActive() && assetExtractorGetViewMode() == 2u) {
        size_t writes = 0u, i, nametableWritten = 0u, attributeWritten = 0u, paletteWritten = 0u;
        const char *sourceLabel = NULL;
        size_t streamSize = 0u;
        const size_t streamIndex = assetExtractorGetPpuStreamIndex();
        (void)nesExcitebikePpuStream(streamIndex, &streamSize, &sourceLabel);
        (void)nesExcitebikePpuSnapshot(streamIndex, s_sourceNametable, s_sourceNametableMask,
                                       s_sourcePalette, s_sourcePaletteMask, &writes);
        for (i = 0u; i < 960u; ++i) if (s_sourceNametableMask[i] != 0u) ++nametableWritten;
        for (i = 0u; i < 64u; ++i) if (s_sourceNametableMask[0x3C0u + i] != 0u) ++attributeWritten;
        for (i = 0u; i < 32u; ++i) if (s_sourcePaletteMask[i] != 0u) ++paletteWritten;
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / SOURCE PPU SNAPSHOT\n"
            "------------------------------\n"
            "Source: %s\nWrites replayed: %lu\n"
            "Nametable bytes written: %lu/960\n"
            "Attribute bytes written: %lu/64\n"
            "Palette addresses written: %lu/32\n"
            "Hatched tiles contain missing source data or unknown palette entries.\n"
            "This is a replayed command list, not a live game frame.\n"
            "D-pad left/right: PPU stream; up/down: change view\n"
            "A: export; B: back\n",
            sourceLabel != NULL ? sourceLabel : "unknown", (unsigned long)writes,
            (unsigned long)nametableWritten, (unsigned long)attributeWritten, (unsigned long)paletteWritten);
        if (!s_logInitialized || strcmp(logText, s_lastLog) != 0) {
            consoleSelect(&s_bottomConsole);
            consoleClear();
            fputs(logText, stdout);
            (void)snprintf(s_lastLog, sizeof(s_lastLog), "%s", logText);
            s_logInitialized = 1;
        }
        return;
    }
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
            assetExtractorGetViewMode() == 0u ? "CHR atlas" : (assetExtractorGetViewMode() == 1u ? "palette diagnostics" : "PPU snapshots"),
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
