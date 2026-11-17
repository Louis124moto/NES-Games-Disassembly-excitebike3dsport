#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_ppu_render.h"

static int testColorLookup(void)
{
    uint8_t rgb[3];
    if (!nesPpuColorToRgb(0x16u, rgb) || rgb[0] != 152u || rgb[1] != 34u || rgb[2] != 32u) return 1;
    if (nesPpuColorToRgb(0x40u, rgb)) return 2;
    return 0;
}

static int testAtlasPaletteModes(void)
{
    unsigned int mode, pixel;
    /* Pixel 0 is the universal background in every real-palette mode. */
    for (mode = 0u; mode < 8u; ++mode)
        if (nesAtlasPaletteIndex(mode, 0u) != 0) return 1;
    /* Background subpalettes: $3F00 + 4*n + pixel. */
    if (nesAtlasPaletteIndex(0u, 1u) != 1 || nesAtlasPaletteIndex(0u, 3u) != 3) return 2;
    if (nesAtlasPaletteIndex(1u, 1u) != 5 || nesAtlasPaletteIndex(3u, 3u) != 15) return 3;
    /* Sprite subpalettes: $3F10 + 4*n + pixel. */
    if (nesAtlasPaletteIndex(4u, 1u) != 17 || nesAtlasPaletteIndex(4u, 3u) != 19) return 4;
    if (nesAtlasPaletteIndex(7u, 3u) != 31) return 5;
    /* Grayscale and invalid input claim no palette entry. */
    if (nesAtlasPaletteIndex(NES_ATLAS_MODE_GRAYSCALE, 1u) != -1) return 6;
    if (nesAtlasPaletteIndex(9u, 1u) != -1 || nesAtlasPaletteIndex(0u, 4u) != -1) return 7;
    if (NES_ATLAS_MODE_COUNT != 9u) return 8;
    /* Every reachable index stays inside the 32-byte palette RAM. */
    for (mode = 0u; mode < 8u; ++mode)
        for (pixel = 0u; pixel < 4u; ++pixel) {
            const int index = nesAtlasPaletteIndex(mode, pixel);
            if (index < 0 || index > 31) return 9;
        }
    if (nesAtlasGrayShade(0u) != 0u || nesAtlasGrayShade(1u) != 85u ||
        nesAtlasGrayShade(2u) != 170u || nesAtlasGrayShade(3u) != 255u ||
        nesAtlasGrayShade(4u) != 0u) return 10;
    return 0;
}

int main(void)
{
    uint8_t *pixels = (uint8_t *)calloc(NES_PPU_MAX_TILES * 64u, 1u);
    uint8_t *rgb = (uint8_t *)calloc(NES_PPU_RGB_BYTES, 1u);
    uint8_t nametable[NES_PPU_NAMETABLE_BYTES];
    uint8_t attributes[NES_PPU_ATTRIBUTE_BYTES];
    uint8_t palette[NES_PPU_PALETTE_BYTES] = {
        0x0F,0x16,0x27,0x30, 0x0F,0x06,0x17,0x28,
        0x0F,0x21,0x12,0x23, 0x0F,0x09,0x19,0x29
    };
    size_t i;
    const int colorResult = testColorLookup();
    const int atlasResult = testAtlasPaletteModes();
    if (colorResult != 0) { fprintf(stderr, "NES color lookup failed: %d\\n", colorResult); return 1; }
    if (atlasResult != 0) { fprintf(stderr, "Atlas palette modes failed: %d\\n", atlasResult); free(pixels); free(rgb); return 1; }
    if (pixels == NULL || rgb == NULL) { free(pixels); free(rgb); return 1; }
    memset(nametable, 0, sizeof(nametable));
    memset(attributes, 0, sizeof(attributes));
    memset(pixels, 1, 64u); /* tile 0 pixels all use color index 1 */
    attributes[0] = (uint8_t)(2u << 2u);
    if (nesPpuRenderNametable(pixels, NES_PPU_MAX_TILES, nametable, attributes,
                              palette, rgb, NES_PPU_RGB_BYTES) != NES_PPU_RENDER_OK) return 2;
    /* NTSC color 0x16 for top-left, color 0x21 for top-right quadrant. */
    if (rgb[0] != 152u || rgb[1] != 34u || rgb[2] != 32u) {
        /* Top-left palette entry 1 is 0x16 (RGB 152,34,32). */
        free(pixels); free(rgb); return 3;
    }
    {
        const size_t p = (16u * 3u);
        if (rgb[p] != 76u || rgb[p + 1u] != 154u || rgb[p + 2u] != 236u) {
            /* palette[9] is 0x21 (RGB 76,154,236). */
            free(pixels); free(rgb); return 4;
        }
    }
    if (nesPpuRenderNametable(pixels, NES_PPU_MAX_TILES, nametable, attributes,
                              palette, rgb, NES_PPU_RGB_BYTES - 1u) != NES_PPU_RENDER_OUTPUT_TOO_SMALL) return 5;
    for (i = 0u; i < sizeof(palette); ++i) palette[i] = 0x40u;
    if (nesPpuRenderNametable(pixels, NES_PPU_MAX_TILES, nametable, attributes,
                              palette, rgb, NES_PPU_RGB_BYTES) != NES_PPU_RENDER_INVALID_PALETTE) return 6;
    free(pixels); free(rgb);
    puts("PASS: native NES PPU nametable renderer and palette attributes.");
    return 0;
}
