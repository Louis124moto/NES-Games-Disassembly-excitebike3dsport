#include <stddef.h>
#include <stdint.h>
#include "nes_ppu_render.h"
#include "nes_palette.h"
#include <string.h>

int nesPpuColorToRgb(uint8_t nesColor, uint8_t rgb[3])
{
    return nesPaletteToRgb(nesColor, rgb);
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
            (void)nesPaletteToRgb(nesColor, &rgb[dst]);
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
