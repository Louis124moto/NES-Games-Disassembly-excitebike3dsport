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


static int pixelIs(const uint8_t *rgb, unsigned int x, unsigned int y, uint8_t nesColor)
{
    uint8_t want[3];
    const size_t at = ((size_t)y * NES_PPU_WIDTH + x) * 3u;
    if (nesPpuColorToRgb(nesColor, want) == 0) return 0; /* returns 1 on success */
    return rgb[at] == want[0] && rgb[at + 1u] == want[1] && rgb[at + 2u] == want[2];
}

static int pixelUntouched(const uint8_t *rgb, unsigned int x, unsigned int y)
{
    const size_t at = ((size_t)y * NES_PPU_WIDTH + x) * 3u;
    return rgb[at] == 7u && rgb[at + 1u] == 7u && rgb[at + 2u] == 7u;
}

/*
 * Sprite compositor. Expected pixels were derived by hand from the PPU rules
 * (Y+1 offset, flips, colour 0 transparent, lower OAM index wins, 8 per line,
 * left-8 clip, mask/ctrl bits), not by running the function under test.
 */
static int testSprites(void)
{
    enum { TILES = 300 };
    uint8_t *chr = (uint8_t *)calloc(TILES, 64u);
    uint8_t *rgb = (uint8_t *)malloc(NES_PPU_RGB_BYTES);
    uint8_t oam[256], pal[32];
    size_t drawn = 99u, i;
    unsigned int r, c;
    int rc = 0;
    if (chr == NULL || rgb == NULL) { free(chr); free(rgb); return 100; }
    /* tile 5: row 0 = all colour 3; rows 1-7 = col0 -> 1, col7 -> 2, others transparent. */
    for (r = 0u; r < 8u; ++r)
        for (c = 0u; c < 8u; ++c)
            chr[5u * 64u + r * 8u + c] = (uint8_t)(r == 0u ? 3u : (c == 0u ? 1u : (c == 7u ? 2u : 0u)));
    for (i = 0u; i < 64u; ++i) chr[261u * 64u + i] = 3u; /* tile 5 of the second pattern table */
    memset(pal, 0, sizeof(pal));
    pal[17] = 0x16u; pal[18] = 0x21u; pal[19] = 0x30u; pal[21] = 0x27u;
    memset(oam, 0xF8, sizeof(oam));
    oam[0] = 10u; oam[1] = 5u; oam[2] = 0u; oam[3] = 20u;

    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, &drawn) != NES_PPU_RENDER_OK) { rc = 101; goto done; }
    /* sprite at Y=10 starts on line 11; row 0 = 8 pixels of colour 3 (0x30) */
    for (c = 0u; c < 8u; ++c) if (!pixelIs(rgb, 20u + c, 11u, 0x30u)) { rc = 102; goto done; }
    if (!pixelUntouched(rgb, 20u, 10u)) { rc = 103; goto done; }               /* Y+1: line 10 is empty */
    if (!pixelIs(rgb, 20u, 12u, 0x16u) || !pixelIs(rgb, 27u, 12u, 0x21u)) { rc = 104; goto done; }
    if (!pixelUntouched(rgb, 21u, 12u) || !pixelUntouched(rgb, 26u, 12u)) { rc = 105; goto done; } /* transparent */
    if (!pixelIs(rgb, 20u, 18u, 0x16u)) { rc = 106; goto done; }              /* last row = line 18 */
    if (!pixelUntouched(rgb, 20u, 19u)) { rc = 107; goto done; }
    if (drawn != 8u + 7u * 2u) { rc = 108; goto done; }                       /* 8 + 7 rows x 2 pixels */

    /* horizontal flip swaps the two edge colours */
    oam[2] = 0x40u; memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 109; goto done; }
    if (!pixelIs(rgb, 20u, 12u, 0x21u) || !pixelIs(rgb, 27u, 12u, 0x16u)) { rc = 110; goto done; }
    /* vertical flip puts the all-colour-3 row on the last line */
    oam[2] = 0x80u; memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 111; goto done; }
    if (!pixelIs(rgb, 22u, 18u, 0x30u) || !pixelUntouched(rgb, 22u, 11u)) { rc = 112; goto done; }
    /* attribute bits 0-1 choose the sprite subpalette (palette RAM 16 + 4n) */
    oam[2] = 0x01u; memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 113; goto done; }
    if (!pixelIs(rgb, 20u, 12u, 0x27u)) { rc = 114; goto done; }
    /* PPUCTRL bit 3 selects the second sprite pattern table (tile 261 = all colour 3) */
    oam[2] = 0u; memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x18u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 115; goto done; }
    if (!pixelIs(rgb, 23u, 15u, 0x30u)) { rc = 116; goto done; }

    /* PPUMASK: bit 4 off -> nothing; bit 2 off -> leftmost 8 pixels clipped */
    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x0Au, NULL, rgb, NES_PPU_RGB_BYTES, &drawn) != NES_PPU_RENDER_OK || drawn != 0u) { rc = 117; goto done; }
    oam[3] = 4u;
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 118; goto done; }
    if (!pixelUntouched(rgb, 4u, 11u) || !pixelUntouched(rgb, 7u, 11u) || !pixelIs(rgb, 8u, 11u, 0x30u)) { rc = 119; goto done; }
    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Eu, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 120; goto done; }
    if (!pixelIs(rgb, 4u, 11u, 0x30u)) { rc = 121; goto done; }

    /* lower OAM index wins: sprite 1 (subpalette 1) sits under sprite 0 at the same spot */
    oam[3] = 20u; oam[4] = 10u; oam[5] = 5u; oam[6] = 1u; oam[7] = 20u;
    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 122; goto done; }
    if (!pixelIs(rgb, 20u, 12u, 0x16u)) { rc = 123; goto done; }
    memset(&oam[4], 0xF8, 4u);

    /* 8 sprites per scanline: the ninth is dropped */
    for (i = 0u; i < 9u; ++i) { oam[i * 4u] = 100u; oam[i * 4u + 1u] = 5u; oam[i * 4u + 2u] = 0u; oam[i * 4u + 3u] = (uint8_t)(i * 16u); }
    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Eu, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 124; goto done; }
    if (!pixelIs(rgb, 112u, 101u, 0x30u) || !pixelUntouched(rgb, 128u, 101u)) { rc = 125; goto done; }
    memset(&oam[0], 0xF8, 36u);
    oam[0] = 10u; oam[1] = 5u; oam[2] = 0u; oam[3] = 20u;

    /* unmodelled features refuse and draw nothing; off-screen sprites do not count */
    memset(rgb, 7, NES_PPU_RGB_BYTES);
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x30u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_UNSUPPORTED) { rc = 126; goto done; }
    oam[2] = 0x20u;
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_UNSUPPORTED) { rc = 127; goto done; }
    if (!pixelUntouched(rgb, 20u, 11u)) { rc = 128; goto done; }
    oam[0] = 0xF8u; /* hidden: its behind-background bit is irrelevant */
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_OK) { rc = 129; goto done; }
    oam[0] = 10u; oam[2] = 0u; oam[1] = 255u; /* tile beyond the loaded CHR */
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x18u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_MISSING_TILE) { rc = 130; goto done; }
    /* behind-background priority (attribute bit 5) with a background-opacity map: drawn only over colour 0 */
    {
        uint8_t *opaque = (uint8_t *)calloc((size_t)NES_PPU_WIDTH * NES_PPU_HEIGHT, 1u);
        if (opaque == NULL) { rc = 140; goto done; }
        oam[0] = 10u; oam[1] = 5u; oam[2] = 0x20u; oam[3] = 20u;
        opaque[11u * NES_PPU_WIDTH + 22u] = 1u;    /* background covers line 11, x 22 */
        memset(rgb, 7, NES_PPU_RGB_BYTES);
        if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, opaque, rgb, NES_PPU_RGB_BYTES, &drawn) != NES_PPU_RENDER_OK) { free(opaque); rc = 141; goto done; }
        if (pixelUntouched(rgb, 21u, 11u) || !pixelUntouched(rgb, 22u, 11u) || pixelUntouched(rgb, 23u, 11u)) { free(opaque); rc = 142; goto done; }
        if (drawn != 7u + 7u * 2u) { free(opaque); rc = 143; goto done; } /* row 0 loses one pixel to the background */
        free(opaque);
        oam[2] = 0u;
    }
    pal[17] = 0x40u;
    if (nesPpuRenderSprites(chr, TILES, oam, pal, 0x10u, 0x1Au, NULL, rgb, NES_PPU_RGB_BYTES, NULL) != NES_PPU_RENDER_INVALID_PALETTE) { rc = 131; goto done; }
done:
    free(chr); free(rgb);
    return rc;
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
    { const int sp = testSprites(); if (sp != 0) { fprintf(stderr, "sprite compositor failed: %d\n", sp); return 50 + sp; } }
    puts("PASS: native NES PPU nametable renderer, palette attributes and sprite compositor.");
    return 0;
}
