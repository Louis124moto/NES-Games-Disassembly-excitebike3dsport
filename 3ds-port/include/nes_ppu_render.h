#ifndef EXCITEBIKE_NES_PPU_RENDER_H
#define EXCITEBIKE_NES_PPU_RENDER_H

#include <stddef.h>
#include <stdint.h>

#define NES_PPU_WIDTH 256u
#define NES_PPU_HEIGHT 240u
#define NES_PPU_TILES_WIDE 32u
#define NES_PPU_TILES_HIGH 30u
#define NES_PPU_NAMETABLE_BYTES 960u
#define NES_PPU_ATTRIBUTE_BYTES 64u
#define NES_PPU_PALETTE_BYTES 16u
#define NES_PPU_RGB_BYTES (NES_PPU_WIDTH * NES_PPU_HEIGHT * 3u)
#define NES_PPU_MAX_TILES 960u

typedef enum {
    NES_PPU_RENDER_OK = 0,
    NES_PPU_RENDER_INVALID_ARGUMENT,
    NES_PPU_RENDER_INVALID_TILE_COUNT,
    NES_PPU_RENDER_OUTPUT_TOO_SMALL,
    NES_PPU_RENDER_INVALID_PALETTE,
    NES_PPU_RENDER_MISSING_TILE,
    NES_PPU_RENDER_INVALID_PIXEL
} NesPpuRenderResult;

/*
 * Compose an NES background from decoded 2bpp CHR pixels and source-derived
 * PPU nametable/attribute/palette snapshots. No data is inferred here.
 * palette must contain the 16 background palette RAM values (0..63).
 */
int nesPpuColorToRgb(uint8_t nesColor, uint8_t rgb[3]);

/*
 * CHR atlas palette modes, modelled on the CHR viewers of existing NES
 * debuggers (Mesen "Palette Selection": the four background palettes, the four
 * sprite palettes, and a grayscale option). A bare CHR tile has no palette of
 * its own, so the atlas lets the viewer pick one instead of guessing.
 *   modes 0-3: background subpalettes 0-3 (palette RAM $3F00 + 4*n)
 *   modes 4-7: sprite subpalettes 0-3 (palette RAM $3F10 + 4*(n-4))
 *   mode 8   : grayscale (default; no palette claimed)
 */
#define NES_ATLAS_MODE_COUNT 9u
#define NES_ATLAS_MODE_GRAYSCALE 8u

/*
 * Palette RAM index (0..31) that the PPU uses for CHR pixel value 0..3 under
 * atlas mode 0..7. Pixel 0 always resolves to the universal background entry
 * (index 0), exactly like the PPU does for every subpalette. Returns -1 for
 * the grayscale mode or for out-of-range arguments.
 */
int nesAtlasPaletteIndex(unsigned int mode, unsigned int pixel);

/* Gray shade for pixel value 0..3 (0, 85, 170, 255); 0 for out-of-range input. */
uint8_t nesAtlasGrayShade(unsigned int pixel);

int nesPpuRenderNametable(const uint8_t *chrPixels, size_t tileCount,
                          const uint8_t *nametable,
                          const uint8_t *attributes,
                          const uint8_t *palette,
                          uint8_t *rgb, size_t rgbCapacity);

#endif
