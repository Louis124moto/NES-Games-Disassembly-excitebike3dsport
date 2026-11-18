#include <stddef.h>
#include <stdint.h>
#include "nes_ppu_render.h"
#include <string.h>

/* Common NTSC palette used by the original extraction reference. */
static const uint8_t s_nesRgb[64][3] = {
 {84,84,84},{0,30,116},{8,16,144},{48,0,136},{68,0,100},{92,0,48},{84,4,0},{60,24,0},
 {32,42,0},{8,58,0},{0,64,0},{0,60,0},{0,50,60},{0,0,0},{0,0,0},{0,0,0},
 {152,150,152},{8,76,196},{48,50,236},{92,30,228},{136,20,176},{160,20,100},{152,34,32},{120,60,0},
 {84,90,0},{40,114,0},{8,124,0},{0,118,40},{0,102,120},{0,0,0},{0,0,0},{0,0,0},
 {236,238,236},{76,154,236},{120,124,236},{176,98,236},{228,84,236},{236,88,180},{236,106,100},{212,136,32},
 {160,170,0},{116,196,0},{76,208,32},{56,204,108},{56,180,204},{60,60,60},{0,0,0},{0,0,0},
 {236,238,236},{168,204,236},{188,188,236},{212,178,236},{236,174,236},{236,174,212},{236,180,176},{228,196,144},
 {204,210,120},{180,222,120},{168,226,144},{152,226,180},{160,214,228},{160,162,160},{0,0,0},{0,0,0}
};

int nesPpuColorToRgb(uint8_t nesColor, uint8_t rgb[3])
{
    if (rgb == NULL || nesColor > 0x3Fu) return 0;
    rgb[0] = s_nesRgb[nesColor][0];
    rgb[1] = s_nesRgb[nesColor][1];
    rgb[2] = s_nesRgb[nesColor][2];
    return 1;
}

int nesAtlasPaletteIndex(unsigned int mode, unsigned int pixel)
{
    if (mode >= 8u || pixel > 3u) return -1;
    if (pixel == 0u) return 0; /* universal background colour */
    return (int)(mode * 4u + pixel); /* modes 0-3 -> $3F00.., 4-7 -> $3F10.. */
}

uint8_t nesAtlasGrayShade(unsigned int pixel)
{
    return pixel > 3u ? 0u : (uint8_t)(pixel * 85u);
}

int nesPpuRenderNametableOpaque(const uint8_t *chrPixels, size_t tileCount,
                                const uint8_t *nametable, const uint8_t *attributes,
                                const uint8_t *palette, uint8_t *rgb,
                                size_t rgbCapacity, uint8_t *opaque,
                                size_t opaqueCapacity)
{
    size_t y, x;
    if (opaque != NULL && opaqueCapacity < (size_t)NES_PPU_WIDTH * NES_PPU_HEIGHT)
        return NES_PPU_RENDER_OUTPUT_TOO_SMALL;
    if (chrPixels == NULL || nametable == NULL || attributes == NULL ||
        palette == NULL || rgb == NULL) return NES_PPU_RENDER_INVALID_ARGUMENT;
    if (tileCount == 0u || tileCount > NES_PPU_MAX_TILES)
        return NES_PPU_RENDER_INVALID_TILE_COUNT;
    if (rgbCapacity < NES_PPU_RGB_BYTES) return NES_PPU_RENDER_OUTPUT_TOO_SMALL;
    for (y = 0u; y < 16u; ++y) {
        if (palette[y] > 0x3Fu) return NES_PPU_RENDER_INVALID_PALETTE;
    }

    for (y = 0u; y < NES_PPU_HEIGHT; ++y) {
        for (x = 0u; x < NES_PPU_WIDTH; ++x) {
            const size_t tileX = x >> 3u;
            const size_t tileY = y >> 3u;
            const size_t tileIndex = nametable[tileY * NES_PPU_TILES_WIDE + tileX];
            const size_t attrIndex = (tileY >> 2u) * 8u + (tileX >> 2u);
            const unsigned int shift = (unsigned int)(((tileY & 2u) << 1u) | (tileX & 2u));
            const unsigned int subpalette = (attributes[attrIndex] >> shift) & 3u;
            size_t pixel;
            uint8_t colorIndex;
            uint8_t nesColor;
            size_t dst;
            if (tileIndex >= tileCount) return NES_PPU_RENDER_MISSING_TILE;
            pixel = (tileIndex * 64u) + ((y & 7u) * 8u) + (x & 7u);
            colorIndex = chrPixels[pixel];
            if (colorIndex > 3u) return NES_PPU_RENDER_INVALID_PIXEL;
            nesColor = colorIndex == 0u ? palette[0] : palette[subpalette * 4u + colorIndex];
            dst = (y * NES_PPU_WIDTH + x) * 3u;
            rgb[dst] = s_nesRgb[nesColor][0];
            rgb[dst + 1u] = s_nesRgb[nesColor][1];
            rgb[dst + 2u] = s_nesRgb[nesColor][2];
            if (opaque != NULL) opaque[y * NES_PPU_WIDTH + x] = colorIndex != 0u ? 1u : 0u;
        }
    }
    return NES_PPU_RENDER_OK;
}

int nesPpuRenderNametable(const uint8_t *chrPixels, size_t tileCount,
                          const uint8_t *nametable, const uint8_t *attributes,
                          const uint8_t *palette, uint8_t *rgb,
                          size_t rgbCapacity)
{
    return nesPpuRenderNametableOpaque(chrPixels, tileCount, nametable, attributes,
                                       palette, rgb, rgbCapacity, NULL, 0u);
}

int nesPpuRenderSprites(const uint8_t *chrPixels, size_t tileCount,
                        const uint8_t *oam, const uint8_t *palette,
                        uint8_t ppuCtrl, uint8_t ppuMask,
                        const uint8_t *bgOpaque,
                        uint8_t *rgb, size_t rgbCapacity, size_t *drawn)
{
    size_t i, line, written = 0u;
    const size_t tileBase = (ppuCtrl & 0x08u) != 0u ? 256u : 0u;
    uint8_t occupied[NES_PPU_WIDTH];

    if (drawn != NULL) *drawn = 0u;
    if (chrPixels == NULL || oam == NULL || palette == NULL || rgb == NULL)
        return NES_PPU_RENDER_INVALID_ARGUMENT;
    if (tileCount == 0u) return NES_PPU_RENDER_INVALID_TILE_COUNT;
    if (rgbCapacity < NES_PPU_RGB_BYTES) return NES_PPU_RENDER_OUTPUT_TOO_SMALL;
    for (i = 16u; i < 32u; ++i) {
        if (palette[i] > 0x3Fu) return NES_PPU_RENDER_INVALID_PALETTE;
    }
    if ((ppuMask & 0x10u) == 0u) return NES_PPU_RENDER_OK; /* sprites disabled */

    /* Pre-scan: refuse (draw nothing) when an on-screen sprite needs an unmodelled feature. */
    for (i = 0u; i < 64u; ++i) {
        const unsigned int top = (unsigned int)oam[i * 4u] + 1u;
        if (top >= NES_PPU_HEIGHT) continue; /* Y >= $EF: never visible */
        if ((ppuCtrl & 0x20u) != 0u) return NES_PPU_RENDER_UNSUPPORTED;
        if ((oam[i * 4u + 2u] & 0x20u) != 0u && bgOpaque == NULL) return NES_PPU_RENDER_UNSUPPORTED;
        if (tileBase + oam[i * 4u + 1u] >= tileCount) return NES_PPU_RENDER_MISSING_TILE;
    }

    for (line = 0u; line < NES_PPU_HEIGHT; ++line) {
        unsigned int onLine = 0u;
        memset(occupied, 0, sizeof(occupied));
        for (i = 0u; i < 64u; ++i) {
            const unsigned int top = (unsigned int)oam[i * 4u] + 1u;
            const uint8_t attr = oam[i * 4u + 2u];
            unsigned int row, col;
            size_t tile;
            if (top >= NES_PPU_HEIGHT || line < top || line >= top + 8u) continue;
            if (++onLine > 8u) break; /* sprite overflow: later sprites are not drawn */
            row = (unsigned int)(line - top);
            if ((attr & 0x80u) != 0u) row = 7u - row;
            tile = tileBase + oam[i * 4u + 1u];
            for (col = 0u; col < 8u; ++col) {
                const unsigned int sx = (unsigned int)oam[i * 4u + 3u] + col;
                const unsigned int srcCol = (attr & 0x40u) != 0u ? 7u - col : col;
                uint8_t pixel, nesColor;
                size_t dst;
                if (sx >= NES_PPU_WIDTH) continue;
                if (sx < 8u && (ppuMask & 0x04u) == 0u) continue; /* left 8 px clipped */
                pixel = chrPixels[tile * 64u + row * 8u + srcCol];
                if (pixel > 3u) return NES_PPU_RENDER_INVALID_PIXEL;
                if (pixel == 0u || occupied[sx] != 0u) continue; /* transparent / lower index wins */
                occupied[sx] = 1u; /* sprite-vs-sprite priority is resolved before sprite-vs-background */
                if ((attr & 0x20u) != 0u && bgOpaque[line * NES_PPU_WIDTH + sx] != 0u) continue;
                nesColor = palette[16u + (size_t)(attr & 3u) * 4u + pixel];
                dst = (line * NES_PPU_WIDTH + sx) * 3u;
                rgb[dst] = s_nesRgb[nesColor][0];
                rgb[dst + 1u] = s_nesRgb[nesColor][1];
                rgb[dst + 2u] = s_nesRgb[nesColor][2];
                ++written;
            }
        }
    }
    if (drawn != NULL) *drawn = written;
    return NES_PPU_RENDER_OK;
}
