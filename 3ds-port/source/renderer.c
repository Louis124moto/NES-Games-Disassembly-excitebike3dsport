#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"
#include "native_menu.h"
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
 *
 * Second hardware observation, same day: disabling double buffering removed
 * the corner blink, but every large per-frame rewrite of the scanned buffer
 * remained visible on this console. The top screen still rendered as mush,
 * and the bottom screen - the only surface still rewritten every frame
 * (consoleClear() plus a full log reprint) - developed the same offset-0
 * corner flicker the top screen used to show. The only configuration
 * observed to hold a perfectly stable image on this console is ZERO
 * framebuffer writes per frame (console text rendered once). This renderer
 * is therefore change-driven:
 *  - rendererWriteConsole() rebuilds the log text every frame (CPU-only) and
 *    reprints the console only when the text actually changed;
 *  - rendererTopNeedsRepaint() redraws the top screen only when the
 *    view/page/palette/stream/state inputs changed;
 *  - a transition repaint may be caught once by the scanout; the settled
 *    image is bit-stable. If the settled top screen still shows artifacts,
 *    they are the drawn content itself, not a timing defect.
 */
/*
 * Top-screen pixel format (root-cause finding, 2026-11-16, from libctru source
 * and the official devkitPro 3ds-examples/graphics/bitmap/24bit-color example):
 * consoleInit(screen, ...) in libctru calls
 * gfxSetScreenFormat(screen, GSP_RGB565_OES), i.e. 2 bytes per pixel. This
 * renderer writes 3-byte BGR8 pixels directly into the top framebuffer using
 * topFramebufferOffset(), so the top screen must NEVER be passed to
 * consoleInit(): the LCD would scan out the 3-byte data as 2-byte RGB565 pixels
 * (garbled "mush" while the 16-bit bottom console text stays sharp). The
 * official example keeps the console and the raw 24-bit image on different
 * screens for exactly this reason. Only the bottom screen is a console here.
 * Hardware confirmation of this fix is pending (see 3ds-port/TODO.md).
 */
static PrintConsole s_bottomConsole;
static int s_gameStarted;
static int s_menuActive;
/* Static storage avoids putting a 184 KiB NES RGB frame on the 3DS stack. */
static uint8_t s_ppuRgb[NES_PPU_RGB_BYTES];
static uint8_t s_splitBgOpaque[NES_PPU_WIDTH * NES_PPU_HEIGHT];
static uint8_t s_sprite0HitFound;
static uint32_t s_sprite0HitCycle = UINT32_MAX;
static uint8_t s_sourcePpuRgb[NES_PPU_RGB_BYTES];
static uint8_t s_sourceNametable[NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES];
static uint8_t s_sourceNametableMask[NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES];
static uint8_t s_sourcePalette[32];
static uint8_t s_sourcePaletteMask[32];
static uint8_t s_sourceNametableView[NES_PPU_NAMETABLE_BYTES];
static uint8_t s_sourceAttributes[NES_PPU_ATTRIBUTE_BYTES];
static uint8_t s_sourcePaletteView[NES_PPU_PALETTE_BYTES];
static uint8_t s_unknownTiles[NES_PPU_NAMETABLE_BYTES];
/* Last text written to the bottom console (see rendererWriteConsole). */
static char s_lastConsoleText[512];

/*
 * Store the last text written to the console with an explicit byte loop so
 * the 3DS build stays free of copy/truncation warnings under -Wall -Wextra.
 */
static void rendererStoreConsoleText(const char *text)
{
    size_t i;
    for (i = 0u; i + 1u < sizeof(s_lastConsoleText) && text[i] != '\0'; ++i) {
        s_lastConsoleText[i] = text[i];
    }
    s_lastConsoleText[i] = '\0';
}

/*
 * Change-driven console: the log text is rebuilt every frame, but the bottom
 * framebuffer is touched only when the text differs from the last written
 * copy. consoleClear() plus a full reprint happens only on state changes; in
 * steady state this performs zero framebuffer writes, which is the only
 * configuration observed stable on the test console.
 */
static void rendererWriteConsole(const char *text)
{
    consoleSelect(&s_bottomConsole);
    if (strcmp(text, s_lastConsoleText) == 0) return;
    consoleClear();
    fputs(text, stdout);
    rendererStoreConsoleText(text);
}

/*
 * Change-driven top screen: every top-screen view is a pure function of the
 * inputs compared below, so the framebuffer is repainted only when one of
 * them changes. This is the documented fallback after single buffering alone
 * did not stabilize the top screen, and it makes the failure mode
 * deterministic: a settled screen that still shows artifacts is a content
 * defect (photograph it), and a screen that repaints without input means an
 * input below is oscillating (the bottom log View/page lines would show it).
 */
static int rendererTopNeedsRepaint(void)
{
    static int s_active = -1;
    static int s_game = -1;
    static int s_menu = -1;
    static unsigned int s_view = 0xFFFFFFFFu;
    static size_t s_page = (size_t)-1;
    static size_t s_palette = (size_t)-1;
    static unsigned int s_atlasMode = 0xFFFFFFFFu;
    static size_t s_stream = (size_t)-1;
    static size_t s_tiles = (size_t)-1;
    const int active = assetExtractorIsActive() != 0;
    const int game = s_gameStarted;
    const int menu = s_menuActive;
    const unsigned int view = assetExtractorGetViewMode();
    const size_t page = (size_t)assetExtractorGetPage();
    const size_t palette = active ? (size_t)assetExtractorGetPaletteIndex() : 0u;
    const unsigned int atlasMode = active ? assetExtractorGetAtlasMode() : 0u;
    const size_t stream = active ? (size_t)assetExtractorGetPpuStreamIndex() : 0u;
    const size_t tiles = (size_t)assetExtractorGetTotalTiles();
    const int dirty = (active != s_active) || (game != s_game) || (menu != s_menu) ||
                      (view != s_view) || (page != s_page) || (palette != s_palette) ||
                      (atlasMode != s_atlasMode) ||
                      (stream != s_stream) || (tiles != s_tiles);

    s_active = active;
    s_game = game;
    s_menu = menu;
    s_view = view;
    s_page = page;
    s_palette = palette;
    s_atlasMode = atlasMode;
    s_stream = stream;
    s_tiles = tiles;
    return dirty;
}

/*
 * Fill the whole top framebuffer with black (BGR8, 240x400 sideways storage).
 * Used once at startup, because the top screen is no longer a console that
 * clears itself on init, and whenever the viewer closes so no stale atlas
 * pixels stay on screen.
 */
static void rendererClearTop(void)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width != 240u || height != 400u) return;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
}

static const char *atlasModeName(unsigned int mode)
{
    static const char *const names[NES_ATLAS_MODE_COUNT] = {
        "BG 0", "BG 1", "BG 2", "BG 3", "SPR 0", "SPR 1", "SPR 2", "SPR 3", "grayscale"
    };
    return mode < NES_ATLAS_MODE_COUNT ? names[mode] : "?";
}

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
/* Non-zero where the background pixel is not colour 0 (sprite behind-BG priority). */
static uint8_t s_bgOpaque[NES_PPU_WIDTH * NES_PPU_HEIGHT];

/* Result of the last sprite composition (shown in the bottom log). */
static size_t s_bgChrTileCount;
static int s_bgRenderResult;
static int s_bgRenderReason;

/*
 * Rendering disabled ($2001 bits 3/4 clear): the PPU shows the universal backdrop
 * colour (palette RAM $3F00) over the whole picture. Only drawn once a palette list
 * has really been executed; before that there is no source colour to claim.
 */
static int rendererDrawBackdrop(const GameState *state)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    uint8_t rgb[3];
    unsigned int x, y;
    if (state == NULL || state->menu_ppu_palette_loaded == 0u) return 0;
    if (framebuffer == NULL || width != 240u || height != 400u) return 0;
    if (nesPpuColorToRgb((uint8_t)(state->menu_ppu_palette[0] & 0x3Fu), rgb) == 0) return 0;
    for (y = 0u; y < 240u; ++y) {
        for (x = 0u; x < 400u; ++x) {
            const size_t dst = topFramebufferOffset(x, y);
            framebuffer[dst] = rgb[2];
            framebuffer[dst + 1u] = rgb[1];
            framebuffer[dst + 2u] = rgb[0];
        }
    }
    return 1;
}

static int rendererDrawPpuBackground(const GameState *state)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *chrAll;
    size_t chrAllCount = 0u;
    unsigned int y;

    s_sprite0HitFound = 0u;
    s_sprite0HitCycle = UINT32_MAX;
    s_bgRenderReason = 0;
    s_bgRenderResult = NES_PPU_RENDER_OK;
    s_bgChrTileCount = 0u;
    if (state == NULL || state->menu_ppu_background_ready == 0u) {
        s_bgRenderReason = 1;
        return 0;
    }
    if (framebuffer == NULL || width != 240u || height != 400u) {
        s_bgRenderReason = 2;
        return 0;
    }
    chrAll = assetExtractorGetPixels(&chrAllCount);
    s_bgChrTileCount = chrAllCount;
    if (chrAll == NULL || chrAllCount == 0u) {
        s_bgRenderReason = 4;
        return 0;
    }

    if (state->menu_ppu_split_valid) {
        const uint8_t *topPixels = chrAll;
        const uint8_t *bottomPixels = chrAll;
        size_t topTileCount = chrAllCount;
        size_t bottomTileCount = chrAllCount;
        /* Each half may select its own BG pattern table and nametable base. */
        if ((state->menu_ppu_top_ctrl & 0x10u) != 0u) {
            if (topTileCount <= 256u) { s_bgRenderReason = 3; return 0; }
            topPixels += 256u * 64u;
            topTileCount -= 256u;
        }
        if ((state->menu_ppu_ctrl & 0x10u) != 0u) {
            if (bottomTileCount <= 256u) { s_bgRenderReason = 3; return 0; }
            bottomPixels += 256u * 64u;
            bottomTileCount -= 256u;
        }
        s_bgRenderResult = nesPpuRenderMirroredNametablesOpaque(
                topPixels, topTileCount, state->menu_ppu_nametables,
                state->menu_ppu_palette, state->menu_ppu_top_ctrl,
                state->menu_ppu_top_scroll_x, state->menu_ppu_top_scroll_y,
                s_ppuRgb, sizeof(s_ppuRgb), s_bgOpaque, sizeof(s_bgOpaque));
        if (s_bgRenderResult != NES_PPU_RENDER_OK) { s_bgRenderReason = 5; return 0; }
        s_bgRenderResult = nesPpuRenderMirroredNametablesOpaque(
                bottomPixels, bottomTileCount, state->menu_ppu_nametables,
                state->menu_ppu_palette, state->menu_ppu_ctrl,
                state->menu_scroll_x, state->menu_scroll_y,
                s_sourcePpuRgb, sizeof(s_sourcePpuRgb),
                s_splitBgOpaque, sizeof(s_splitBgOpaque));
        if (s_bgRenderResult != NES_PPU_RENDER_OK) { s_bgRenderReason = 5; return 0; }

        if (!nesPpuCompositeRasterSplit(s_ppuRgb, s_bgOpaque,
                                        s_sourcePpuRgb, s_splitBgOpaque,
                                        state->menu_ppu_split_scanline,
                                        state->menu_ppu_split_dot,
                                        s_ppuRgb, sizeof(s_ppuRgb),
                                        s_bgOpaque, sizeof(s_bgOpaque))) {
            s_bgRenderReason = 5;
            s_bgRenderResult = NES_PPU_RENDER_UNSUPPORTED;
            return 0;
        }
    } else {
        const uint8_t *pixels = chrAll;
        size_t tileCount = chrAllCount;
        if ((state->menu_ppu_ctrl & 0x10u) != 0u) {
            if (tileCount <= 256u) { s_bgRenderReason = 3; return 0; }
            pixels += 256u * 64u;
            tileCount -= 256u;
        }
        s_bgRenderResult = nesPpuRenderMirroredNametablesOpaque(
                pixels, tileCount, state->menu_ppu_nametables,
                state->menu_ppu_palette, state->menu_ppu_ctrl,
                state->menu_scroll_x, state->menu_scroll_y,
                s_ppuRgb, sizeof(s_ppuRgb), s_bgOpaque, sizeof(s_bgOpaque));
        if (s_bgRenderResult != NES_PPU_RENDER_OK) { s_bgRenderReason = 5; return 0; }
    }

    {
        unsigned long hitCycle = ~0UL;
        if (nesPpuFindSprite0HitCycle(chrAll, chrAllCount, state->menu_oam,
                                      state->menu_ppu_ctrl, state->menu_ppu_mask,
                                      s_bgOpaque, &hitCycle)) {
            s_sprite0HitFound = 1u;
            s_sprite0HitCycle = hitCycle > UINT32_MAX ? UINT32_MAX : (uint32_t)hitCycle;
        }
    }

    /* Source OAM sprites (8x8/8x16, flips and priority) over the split-composited PPU background. */
    (void)nesPpuRenderSprites(chrAll, chrAllCount, state->menu_oam,
                              state->menu_ppu_palette, state->menu_ppu_ctrl,
                              state->menu_ppu_mask, s_bgOpaque, s_ppuRgb,
                              sizeof(s_ppuRgb), NULL);

    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    for (y = 0u; y < NES_PPU_HEIGHT; ++y) {
        unsigned int dstX;
        for (dstX = 0u; dstX < 320u; ++dstX) {
            const unsigned int srcX = (dstX * NES_PPU_WIDTH) / 320u;
            const size_t src = ((size_t)y * NES_PPU_WIDTH + srcX) * 3u;
            const size_t dst = topFramebufferOffset(dstX + 40u, y);
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
     * Change-driven mode (see rendererInit and rendererTopNeedsRepaint): this
     * function runs only when view inputs changed, so no view may rely on a
     * previous-frame clear. Each view repaints its full 400x240 extent;
     * undrawn areas keep their previous content; the consoleInit clear
     * provides the initial black background.
     */
    if (!assetExtractorIsActive() || s_gameStarted) {
        /* Viewer closed: blank the top screen instead of leaving the last atlas page. */
        rendererClearTop();
        return;
    }
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
        /* Repaint the 4:3 viewport; the 40-pixel side bands stay black. */
        for (pixelY = 0u; pixelY < 240u; ++pixelY) {
            for (bandX = 0u; bandX < 40u; ++bandX) {
                const size_t bandOffset = topFramebufferOffset(bandX, (unsigned int)pixelY);
                framebuffer[bandOffset] = 0u;
                framebuffer[bandOffset + 1u] = 0u;
                framebuffer[bandOffset + 2u] = 0u;
            }
            for (bandX = 360u; bandX < 400u; ++bandX) {
                const size_t bandOffset = topFramebufferOffset(bandX, (unsigned int)pixelY);
                framebuffer[bandOffset] = 0u;
                framebuffer[bandOffset + 1u] = 0u;
                framebuffer[bandOffset + 2u] = 0u;
            }
        }
        for (pixelY = 0u; pixelY < 240u; ++pixelY) {
            for (pixelX = 0u; pixelX < 320u; ++pixelX) {
                const unsigned int sourceX = ((unsigned int)pixelX * 256u) / 320u;
                const size_t tileIndex = (pixelY / 8u) * 32u + sourceX / 8u;
                const size_t sourceOffset = (pixelY * 256u + sourceX) * 3u;
                const size_t destOffset = topFramebufferOffset((unsigned int)pixelX + 40u, (unsigned int)pixelY);
                if (s_unknownTiles[tileIndex] != 0u) {
                    const uint8_t shade = ((sourceX / 2u + pixelY / 2u) & 1u) != 0u ? 192u : 64u;
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
     * A standalone CHR tile has no assigned palette, and colouring every tile
     * with one fixed subpalette is wrong for most of them (reported on
     * hardware 2026-11-17: tiles came out red/green/light blue). Like the CHR
     * viewers of existing NES debuggers, the view is grayscale by default and
     * X/Y pick a background or sprite subpalette of the selected source
     * palette preset (L/R). A pixel whose palette entry the preset never
     * writes stays gray instead of being hatched; the SD export still writes
     * all seven presets x four subpalette variants.
     */
    {
        uint8_t atlasPalette[32], atlasPaletteMask[32];
        const unsigned int atlasMode = assetExtractorGetAtlasMode();
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
                const int paletteEntry = nesAtlasPaletteIndex(atlasMode, pixel);
                uint8_t rgb[3];
                unsigned int dy, dx;
                if (paletteEntry < 0 || atlasPaletteMask[paletteEntry] == 0u ||
                    !nesPpuColorToRgb(atlasPalette[paletteEntry], rgb)) {
                    const uint8_t shade = nesAtlasGrayShade(pixel);
                    rgb[0] = shade; rgb[1] = shade; rgb[2] = shade;
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
     * into is always the buffer being scanned. A same-day retest showed this
     * removes the corner blink but does NOT make large per-frame repaints
     * invisible on this console; the renderer is therefore also change-driven
     * (see rendererWriteConsole and rendererTopNeedsRepaint).
     */
    gfxSetDoubleBuffering(GFX_TOP, false);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    /*
     * Top screen = raw 24-bit BGR8 pixels (gfxInitDefault's format). Do NOT call
     * consoleInit(GFX_TOP, ...): it would switch the top screen to RGB565 (see
     * the note above s_bottomConsole). Setting the format explicitly is a no-op
     * guard (gfxSetScreenFormat only records the format when the framebuffers
     * are already large enough) and then the screen is cleared to black once.
     */
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    rendererClearTop();
    consoleInit(GFX_BOTTOM, &s_bottomConsole);
    consoleSelect(&s_bottomConsole);
    s_gameStarted = 0;
    s_menuActive = 0;
}

void rendererBeginFrame(void)
{
    consoleSelect(&s_bottomConsole);
}

/*
 * One-line description of what the NMI upload (vec_C23B_NMI -> sub_C27F) does
 * for a ram_0045 value, from the source trace documented in nes_active_stream.h.
 */
static void rendererDescribeRam0045(uint8_t ram0045, char *out, size_t outSize)
{
    NesActiveStreamInfo info;
    const NesRam0045Target target = nesExcitebikeRam0045Target(ram0045);
    if (nesExcitebikeActiveStream(ram0045, &info)) {
        (void)snprintf(out, outSize, "%s %s%s%s%s",
            info.isPalette ? "PAL" : "PPU",
            info.sourceLabel != NULL ? info.sourceLabel : "?",
            info.hint != NULL ? " [" : "",
            info.hint != NULL ? info.hint : "",
            info.hint != NULL ? "]" : "");
    } else if (target.kind == NES_RAM0045_TARGET_DYNAMIC_BUFFER) {
        (void)snprintf(out, outSize, "dynamic buffer $%04X", (unsigned int)target.cpuAddress);
    } else if (target.kind == NES_RAM0045_TARGET_ZERO_PAGE_LIST) {
        (void)snprintf(out, outSize, "list $%04X (not in CDL)", (unsigned int)target.cpuAddress);
    } else {
        (void)snprintf(out, outSize, "outside table");
    }
}

void rendererGetSprite0Hit(uint8_t *found, uint32_t *cycleFromPrerender)
{
    if (found != NULL) *found = s_sprite0HitFound;
    if (cycleFromPrerender != NULL) *cycleFromPrerender = s_sprite0HitCycle;
}

void rendererDraw(const GameState *state)
{
    char logText[768]; /* default view text is ~510 bytes with worst-case arguments */
    if (!s_menuActive || state == NULL || state->menu_ppu_background_ready == 0u) {
        s_sprite0HitFound = 0u;
        s_sprite0HitCycle = UINT32_MAX;
    }
    if (s_menuActive && state != NULL) {
        /*
         * Menu mode: write both screens only when the menu-state inputs
         * changed (change-driven rendering; see the notes at the top of this
         * file). Revisit this gate when an animated menu is integrated
         * (Stage 2/3).
         */
        static int s_menuDrawn = 0;
        static uint8_t s_lastAsmState = 0u;
        static uint8_t s_lastSelection = 0u;
        static uint8_t s_lastRam0045 = 0u;
        static uint8_t s_lastPpuReady = 0u;
        static uint8_t s_lastRevision = 0u;
        static uint8_t s_lastMask = 0u;
        static uint8_t s_lastCtrl = 0u;
        static uint8_t s_lastScrollX = 0u;
        static uint8_t s_lastScrollY = 0u;
        static uint8_t s_lastSplitValid = 0u;
        static uint8_t s_lastSplitScanline = 0u;
        static uint16_t s_lastSplitDot = 0u;
        static uint8_t s_lastTopCtrl = 0u;
        static uint8_t s_lastTopScrollX = 0u;
        static uint8_t s_lastTopScrollY = 0u;
        static uint8_t s_lastOam[256];
        if (s_menuDrawn == 0 ||
            state->menu_ppu_mask != s_lastMask || state->menu_ppu_ctrl != s_lastCtrl ||
            state->menu_scroll_x != s_lastScrollX || state->menu_scroll_y != s_lastScrollY ||
            state->menu_ppu_split_valid != s_lastSplitValid ||
            state->menu_ppu_split_scanline != s_lastSplitScanline ||
            state->menu_ppu_split_dot != s_lastSplitDot ||
            state->menu_ppu_top_ctrl != s_lastTopCtrl ||
            state->menu_ppu_top_scroll_x != s_lastTopScrollX ||
            state->menu_ppu_top_scroll_y != s_lastTopScrollY ||
            memcmp(s_lastOam, state->menu_oam, sizeof(s_lastOam)) != 0 || state->menu_asm_state != s_lastAsmState ||
            state->menu_selection != s_lastSelection ||
            state->menu_ram0045 != s_lastRam0045 ||
            state->menu_ppu_background_ready != s_lastPpuReady ||
            state->menu_ppu_revision != s_lastRevision) {
            s_menuDrawn = 1;
            s_lastAsmState = state->menu_asm_state;
            s_lastSelection = state->menu_selection;
            s_lastRam0045 = state->menu_ram0045;
            s_lastPpuReady = state->menu_ppu_background_ready;
            s_lastRevision = state->menu_ppu_revision;
            s_lastMask = state->menu_ppu_mask;
            s_lastCtrl = state->menu_ppu_ctrl;
            s_lastScrollX = state->menu_scroll_x;
            s_lastScrollY = state->menu_scroll_y;
            s_lastSplitValid = state->menu_ppu_split_valid;
            s_lastSplitScanline = state->menu_ppu_split_scanline;
            s_lastSplitDot = state->menu_ppu_split_dot;
            s_lastTopCtrl = state->menu_ppu_top_ctrl;
            s_lastTopScrollX = state->menu_ppu_top_scroll_x;
            s_lastTopScrollY = state->menu_ppu_top_scroll_y;
            memcpy(s_lastOam, state->menu_oam, sizeof(s_lastOam));
            if (state->menu_ppu_background_ready == 0u && rendererDrawBackdrop(state)) {
                /* $2001 is off during setup: show the source PPU backdrop, not debug text. */
                consoleSelect(&s_bottomConsole);
                consoleClear();
                rendererStoreConsoleText("");
            } else if (!rendererDrawPpuBackground(state)) {
                consoleSelect(&s_bottomConsole);
                consoleClear();
                /* menusRender writes the console directly; invalidate the cache. */
                rendererStoreConsoleText("");
                nativeMenuRenderStatus(state);
                if (s_bgRenderReason == 3) {
                    printf("PPU BG unavailable: PPUCTRL selects $1000 but CHR has only %lu tiles.\n",
                           (unsigned long)s_bgChrTileCount);
                } else if (s_bgRenderReason == 5) {
                    printf("PPU BG renderer error: %d (CHR tiles %lu, PPUCTRL %02X).\n",
                           s_bgRenderResult, (unsigned long)s_bgChrTileCount,
                           (unsigned int)state->menu_ppu_ctrl);
                } else if (s_bgRenderReason == 4) {
                    printf("PPU BG unavailable: no decoded CHR data.\n");
                }
            } else {
                /* Normal play shows only the source-derived NES frame; diagnostics stay in error paths. */
                consoleSelect(&s_bottomConsole);
                consoleClear();
                rendererStoreConsoleText("");
            }
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
        rendererWriteConsole(logText);
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
        rendererWriteConsole(logText);
        return;
    }
    if (!s_gameStarted && assetExtractorIsActive() && assetExtractorGetViewMode() == 4u) {
        size_t i, writeIndex, used;
        size_t hiddenCount = 0u;
        const unsigned int menuState = state != NULL ? (unsigned int)state->menu_asm_state : 0u;
        const uint8_t ram0045 = state != NULL ? state->menu_ram0045 : 0u;
        const int headerMatch = assetExtractorRomMatchesExcitebikeHeader();
        char target[80];
        rendererDescribeRam0045(ram0045, target, sizeof(target));
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
            int n;
            if (write == NULL) continue;
            if (state != NULL && write->asmState != state->menu_asm_state) continue;
            rendererDescribeRam0045(write->ram0045, resolved, sizeof(resolved));
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
                "NMI $C23B->$C27F reads $0045, clears it\n"
                "D-pad up/down: change view; A: export; B: back\n",
                state != NULL ? (unsigned long)(64u - hiddenCount) : 0u,
                (unsigned long)hiddenCount);
        }
        rendererWriteConsole(logText);
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
            "Atlas palette: %s (preset %lu/7)\n"
            "D-pad left/right: page; up/down: view\n"
            "X/Y: atlas palette; L/R: preset\n"
            "A: open/export; B: close/re-extract\n"
            "START: menu locked until Stage 1 passes\n"
            "Gray = no palette; colours are per subpalette.\n",
            assetExtractorGetRomIdentitySummary(),
            assetExtractorGetStatus(),
            (unsigned long)assetExtractorGetTotalTiles(),
            assetExtractorIsActive() ? "OPEN" : "closed",
            viewName(assetExtractorGetViewMode()),
            (unsigned long)(assetExtractorGetPage() + 1u),
            (unsigned long)((assetExtractorGetTotalTiles() + 374u) / 375u),
            atlasModeName(assetExtractorGetAtlasMode()),
            (unsigned long)(assetExtractorGetPaletteIndex() + 1u));
    }
    rendererWriteConsole(logText);
}

void rendererEndFrame(void)
{
    /*
     * Change-driven top screen: repaint only when view inputs changed (see
     * rendererTopNeedsRepaint). gfxFlushBuffers() still runs every frame so
     * console writes are published even on frames without a top repaint.
     * gfxSwapBuffers() is retained: double buffering is disabled for both
     * screens, so it alternates nothing (hardware-verified: once-drawn
     * console text neither flickers nor blanks at 30 Hz with this call in
     * place), and removing it would change an untested code path.
     */
    if (rendererTopNeedsRepaint()) {
        rendererDrawChrAtlas();
    }
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

/*
 * Faithful game path: present one complete 256x240 RGB NES frame (row-major) on the top screen,
 * centred in the 400x240 panel, then publish and wait for vblank. The bottom screen console is left alone.
 */
void rendererPresentNesFrame(const uint8_t *rgb)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    unsigned int x, y;
    if (framebuffer != NULL && rgb != NULL && width == 240u && height == 400u) {
        for (x = 0u; x < 400u; ++x) {
            const int nx = (int)x - 72;
            for (y = 0u; y < 240u; ++y) {
                const size_t dst = topFramebufferOffset(x, y);
                if (nx < 0 || nx >= 256) {
                    framebuffer[dst] = framebuffer[dst + 1u] = framebuffer[dst + 2u] = 0u;
                } else {
                    const uint8_t *p = &rgb[((size_t)y * 256u + (unsigned int)nx) * 3u];
                    framebuffer[dst] = p[2];
                    framebuffer[dst + 1u] = p[1];
                    framebuffer[dst + 2u] = p[0];
                }
            }
        }
    }
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void rendererPrintBottom(const char *text)
{
    consoleSelect(&s_bottomConsole);
    consoleClear();
    printf("%s", text);
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
