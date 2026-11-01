#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"
#include "menus.h"
#include "nes_active_stream.h"
#include "nes_ppu_render.h"
#include "nes_ppu_commands.h"

/*
 * libctru's default framebuffer is BGR8 and stored sideways: physical
 * (x,y) maps to byte offset ((x * 240) + (239 - y)) * 3.
 * This is linear rotated storage, NOT an 8x8-tiled framebuffer.
 */
static size_t topFramebufferOffset(unsigned int screenX, unsigned int screenY)
{
    return (((size_t)screenX * 240u) + (239u - screenY)) * 3u;
}

/*
 * libctru's gfx framebuffers are double buffered by default: gfxSwapBuffers()
 * alternates the buffer returned by gfxGetFramebuffer(). Hardware observation
 * on 2026-11-01 showed the LCD scanout did not follow that alternation on the
 * test console: console text rendered only once stayed perfectly stable, while
 * top-screen content redrawn every frame showed a ~30 Hz mix of fresh and
 * stale pixels (mush with a flickering bottom-left corner; single-frame
 * photographs look clean). rendererInit() therefore disables double
 * buffering so the drawn buffer and the scanned buffer always agree.
 * Consequences for this renderer:
 *  - console text is re-rendered every frame (cheap and keeps it current);
 *  - a full-buffer memset before drawing would flash a black sweep on the
 *    scanned buffer, so every diagnostic view repaints its full extent with
 *    identical content each frame instead of clearing first.
 */
static PrintConsole s_bottomConsole;
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

static const char *viewName(unsigned int mode)
{
    switch (mode) {
    case 0u: return "CHR atlas";
    case 1u: return "palette diagnostics";
    case 2u: return "PPU snapshots";
    case 3u: return "framebuffer pattern";
    case 4u: return "active stream/OAM";
    default: return "unknown";
    }
}

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
            framebuffer[dst] = s_ppuRgb[src + 2u];
            framebuffer[dst + 1u] = s_ppuRgb[src + 1u];
            framebuffer[dst + 2u] = s_ppuRgb[src];
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
    /*
     * Single-buffered mode (see rendererInit): the LCD scans the buffer we
     * draw into. Repainting identical content every frame is invisible, but
     * a full-buffer memset flashes a black sweep, so each view repaints its
     * full extent instead of clearing first. Undrawn areas keep their
     * previous content; the consoleInit clear provides the initial black
     * background.
     */
    if (!assetExtractorIsActive() || s_gameStarted) return;
    pixels = assetExtractorGetPixels(&tileCount);

    /* Diagnostic mode: known RGB/BGR blocks and 8-pixel grid, independent of CHR decode. */
    if (assetExtractorGetViewMode() == 3u) {
        unsigned int x, y;
        for (y = 0u; y < 240u; ++y) {
            for (x = 0u; x < 400u; ++x) {
                uint8_t red, green, blue;
                if (x < 80u) { red = 255u; green = 0u; blue = 0u; }
                else if (x < 160u) { red = 0u; green = 255u; blue = 0u; }
                else if (x < 240u) { red = 0u; green = 0u; blue = 255u; }
                else if (x < 320u) { red = 255u; green = 255u; blue = 255u; }
                else { red = 0u; green = 0u; blue = 0u; }
                /* Grid every 8 pixels, with stronger axes every 40 pixels. */
                if ((x % 40u) == 0u || (y % 40u) == 0u) {
                    red = 255u; green = 255u; blue = 0u;
                } else if ((x % 8u) == 0u || (y % 8u) == 0u) {
                    red = (uint8_t)(red / 2u); green = (uint8_t)(green / 2u); blue = (uint8_t)(blue / 2u);
                }
                {
                    const size_t offset = topFramebufferOffset(x, y);
                    framebuffer[offset] = blue;
                    framebuffer[offset + 1u] = green;
                    framebuffer[offset + 2u] = red;
                }
            }
        }
        return;
    }

    if (assetExtractorGetViewMode() == 2u) {
        size_t writes = 0u, tile, pixelX, pixelY;
        unsigned int bandX;
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
        /* Repaint the side bands so a view switch leaves no stale pixels. */
        for (pixelY = 0u; pixelY < 240u; ++pixelY) {
            for (bandX = 0u; bandX < 72u; ++bandX) {
                const size_t bandOffset = topFramebufferOffset(bandX, (unsigned int)pixelY);
                framebuffer[bandOffset] = 0u;
                framebuffer[bandOffset + 1u] = 0u;
                framebuffer[bandOffset + 2u] = 0u;
            }
            for (bandX = 328u; bandX < 400u; ++bandX) {
                const size_t bandOffset = topFramebufferOffset(bandX, (unsigned int)pixelY);
                framebuffer[bandOffset] = 0u;
                framebuffer[bandOffset + 1u] = 0u;
                framebuffer[bandOffset + 2u] = 0u;
            }
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
                    framebuffer[destOffset] = s_sourcePpuRgb[sourceOffset + 2u];
                    framebuffer[destOffset + 1u] = s_sourcePpuRgb[sourceOffset + 1u];
                    framebuffer[destOffset + 2u] = s_sourcePpuRgb[sourceOffset];
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
            for (y = 0u; y < 60u; ++y) {
                for (x = 0u; x < 50u; ++x) {
                    const size_t offset = topFramebufferOffset(cellX + x, cellY + y);
                    framebuffer[offset] = rgb[2];
                    framebuffer[offset + 1u] = rgb[1];
                    framebuffer[offset + 2u] = rgb[0];
                }
            }
        }
        return;
    }

    /*
     * Atlas view: decode the current page on demand (up to 375 tiles) so every
     * source tile is reachable even when the total exceeds the eager 960 cap.
     * A standalone CHR tile has no assigned palette. Use the selected source
     * palette preset's first background subpalette for this diagnostic view;
     * the SD export also writes all seven presets x four subpalette variants.
     */
    {
        uint8_t atlasPalette[32], atlasPaletteMask[32];
        size_t paletteWrites = 0u;
        const uint8_t *pagePixels;
        size_t pageTileCount = 0u;
        if (nesExcitebikePaletteSnapshot(assetExtractorGetPaletteIndex(),
                atlasPalette, atlasPaletteMask, &paletteWrites) != NES_PPU_COMMAND_OK) {
            memset(atlasPalette, 0, sizeof(atlasPalette));
            memset(atlasPaletteMask, 0, sizeof(atlasPaletteMask));
        }
        pagePixels = assetExtractorGetPagePixels(&pageTileCount, &firstTile);
        if (pagePixels == NULL || pageTileCount == 0u) return;
        tileCount = pageTileCount;

    /* Repaint the unused tail of the page so paging leaves no stale pixels. */
    for (tile = pageTileCount; tile < 375u; ++tile) {
        const unsigned int clearX = (unsigned int)(tile % 25u) * 16u;
        const unsigned int clearY = (unsigned int)(tile / 25u) * 16u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                unsigned int dx, dy;
                for (dy = 0u; dy < 2u; ++dy) {
                    for (dx = 0u; dx < 2u; ++dx) {
                        const size_t offset = topFramebufferOffset(clearX + col * 2u + dx,
                                                                  clearY + row * 2u + dy);
                        framebuffer[offset] = 0u;
                        framebuffer[offset + 1u] = 0u;
                        framebuffer[offset + 2u] = 0u;
                    }
                }
            }
        }
    }

    /* 2x nearest-neighbour zoom: 375 tiles per page, with D-pad paging. */
    for (tile = 0u; tile < tileCount; ++tile) {
        const unsigned int tileX = (tile % 25u) * 16u;
        const unsigned int tileY = (tile / 25u) * 16u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const uint8_t pixel = pagePixels[tile * 64u + row * 8u + col] & 3u;
                const size_t paletteEntry = pixel == 0u ? 0u : pixel;
                uint8_t rgb[3];
                const int known = atlasPaletteMask[paletteEntry] != 0u &&
                                  nesPpuColorToRgb(atlasPalette[paletteEntry], rgb);
                unsigned int dy, dx;
                if (!known) {
                    const uint8_t shade = ((col / 2u + row / 2u + tile) & 1u) != 0u ? 192u : 64u;
                    rgb[0] = shade; rgb[1] = 0u; rgb[2] = shade;
                }
                for (dy = 0u; dy < 2u; ++dy) {
                    for (dx = 0u; dx < 2u; ++dx) {
                        const unsigned int x = tileX + col * 2u + dx;
                        const unsigned int y = tileY + row * 2u + dy;
                        const size_t offset = topFramebufferOffset(x, y);
                        framebuffer[offset + 0u] = rgb[2];
                        framebuffer[offset + 1u] = rgb[1];
                        framebuffer[offset + 2u] = rgb[0];
                    }
                }
            }
        }
    }
    }
}

void rendererInit(void)
{
    gfxInitDefault();
    /*
     * Hardware observation 2026-11-01: with libctru's default double buffering
     * the LCD scanout did not follow the gfxSwapBuffers() alternation on the
     * test console (once-rendered console text stayed perfectly stable while
     * every-frame-redrawn top-screen content alternated between fresh and
     * stale pixels: mush plus a flickering bottom-left corner, clean in
     * single-frame photographs). Disable double buffering so the buffer drawn
     * into is always the buffer being scanned; per-frame repaints of
     * identical content are then invisible.
     */
    gfxSetDoubleBuffering(GFX_TOP, false);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, &s_bottomConsole);
    consoleSelect(&s_bottomConsole);
    s_gameStarted = 0;
    s_menuActive = 0;
}

void rendererBeginFrame(void)
{
    consoleSelect(&s_bottomConsole);
}

void rendererDraw(const GameState *state)
{
    char logText[512];
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
        return;
    }
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
        consoleSelect(&s_bottomConsole);
        consoleClear();
        fputs(logText, stdout);
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
                "D-pad left/right: palette; up/down: change view\n"
                "A: export; B: back\n");
        }
        consoleSelect(&s_bottomConsole);
        consoleClear();
        fputs(logText, stdout);
        return;
    }
    if (!s_gameStarted && assetExtractorIsActive() && assetExtractorGetViewMode() == 4u) {
        size_t i, writeIndex, used;
        size_t hiddenCount = 0u;
        const unsigned int menuState = state != NULL ? (unsigned int)state->menu_asm_state : 0u;
        const uint8_t ram0045 = state != NULL ? state->menu_ram0045 : 0u;
        const int headerMatch = assetExtractorRomMatchesExcitebikeHeader();
        NesActiveStreamInfo info;
        char target[80];
        const int ram0045Valid = nesExcitebikeActiveStream(ram0045, &info);
        if (ram0045Valid) {
            (void)snprintf(target, sizeof(target), "%s %s%s%s",
                info.isPalette ? "PAL" : "PPU",
                info.sourceLabel != NULL ? info.sourceLabel : "?",
                info.hint != NULL ? " [" : "",
                info.hint != NULL ? info.hint : "");
        } else {
            (void)snprintf(target, sizeof(target), "uncaptured");
        }
        used = (size_t)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / ACTIVE STREAM + OAM\n"
            "------------------------------\n"
            "ROM: %.44s\n"
            "Header matches Excitebike: %s\n"
            "Menu state $0041: %u (locked; Stage 1 pending)\n"
            "Live $0045: $%02X -> %s\n",
            assetExtractorGetRomIdentitySummary(),
            headerMatch == 1 ? "yes" : (headerMatch == 0 ? "no" : "unknown"),
            menuState, (unsigned int)ram0045, target);
        for (writeIndex = 0u;
             writeIndex < nesExcitebikeRam0045SetupWriteCount() && used < sizeof(logText);
             ++writeIndex) {
            const NesRam0045SetupWrite *write = nesExcitebikeRam0045SetupWrite(writeIndex);
            char resolved[80];
            NesActiveStreamInfo winfo;
            int n;
            if (write == NULL) continue;
            if (state != NULL && write->asmState != state->menu_asm_state) continue;
            if (nesExcitebikeActiveStream(write->ram0045, &winfo)) {
                (void)snprintf(resolved, sizeof(resolved), "%s %s%s%s",
                    winfo.isPalette ? "PAL" : "PPU",
                    winfo.sourceLabel != NULL ? winfo.sourceLabel : "?",
                    winfo.hint != NULL ? " [" : "",
                    winfo.hint != NULL ? winfo.hint : "");
            } else {
                (void)snprintf(resolved, sizeof(resolved), "uncaptured");
            }
            n = snprintf(logText + used, sizeof(logText) - used,
                "cmd%u: $0045=$%02X -> %s\n",
                (unsigned int)write->commandIndex,
                (unsigned int)write->ram0045, resolved);
            if (n < 0 || (size_t)n >= sizeof(logText) - used) break;
            used += (size_t)n;
        }
        if (state != NULL) {
            for (i = 0u; i < 64u; ++i) {
                if (state->menu_oam[i * 4u] == 0xF8u) ++hiddenCount;
            }
        }
        if (used < sizeof(logText)) {
            (void)snprintf(logText + used, sizeof(logText) - used,
                "OAM: %lu visible / %lu hidden (Y=$F8 via sub_D255)\n"
                "Live NMI consumer $D55A/$CC2D: not traced\n"
                "D-pad up/down: change view; A: export; B: back\n",
                state != NULL ? (unsigned long)(64u - hiddenCount) : 0u,
                (unsigned long)hiddenCount);
        }
        consoleSelect(&s_bottomConsole);
        consoleClear();
        fputs(logText, stdout);
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
            (unsigned long)assetExtractorGetTotalTiles());
    } else {
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / ASSET LOG\n"
            "------------------------------\n"
            "ROM: %.44s\n"
            "CHR: %.36s\n"
            "Tiles: %lu\n"
            "Source: rom.nes or CHR_ROM.chr\n"
            "Exports: CHR, PNG, 7 palettes + 18 PPU snapshots\n"
            "Atlas: %s\n"
            "View: %s; page %lu/%lu\n"
            "D-pad left/right: page/palette\n"
            "D-pad up/down: change view\n"
            "A: open/export; B: close/re-extract\n"
            "START: menu locked until Stage 1 passes\n"
            "Atlas uses grayscale indices, not game colors.\n",
            assetExtractorGetRomIdentitySummary(),
            assetExtractorGetStatus(),
            (unsigned long)assetExtractorGetTotalTiles(),
            assetExtractorIsActive() ? "OPEN" : "closed",
            viewName(assetExtractorGetViewMode()),
            (unsigned long)(assetExtractorGetPage() + 1u),
            (unsigned long)((assetExtractorGetTotalTiles() + 374u) / 375u));
    }
    consoleSelect(&s_bottomConsole);
    consoleClear();
    fputs(logText, stdout);
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
}

void rendererExit(void)
{
    gfxExit();
}

void rendererSetMenuActive(int active)
{
    s_menuActive = active != 0;
}
