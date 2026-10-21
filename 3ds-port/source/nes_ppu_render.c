#include <stddef.h>
#include <stdint.h>
#include "nes_ppu_render.h"

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

int nesPpuRenderNametable(const uint8_t *chrPixels, size_t tileCount,
                          const uint8_t *nametable, const uint8_t *attributes,
                          const uint8_t *palette, uint8_t *rgb,
                          size_t rgbCapacity)
{
    size_t y, x;
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
            const size_t pixel = (tileIndex * 64u) + ((y & 7u) * 8u) + (x & 7u);
            const uint8_t colorIndex = chrPixels[pixel];
            const uint8_t nesColor = colorIndex == 0u ? palette[0] : palette[subpalette * 4u + colorIndex];
            const size_t dst = (y * NES_PPU_WIDTH + x) * 3u;
            if (tileIndex >= tileCount) return NES_PPU_RENDER_MISSING_TILE;
            if (colorIndex > 3u) return NES_PPU_RENDER_INVALID_PIXEL;
            rgb[dst] = s_nesRgb[nesColor][0];
            rgb[dst + 1u] = s_nesRgb[nesColor][1];
            rgb[dst + 2u] = s_nesRgb[nesColor][2];
        }
    }
    return NES_PPU_RENDER_OK;
}
