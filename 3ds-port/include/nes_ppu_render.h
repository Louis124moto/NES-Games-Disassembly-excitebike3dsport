#ifndef EXCITEBIKE_NES_PPU_RENDER_H
#define EXCITEBIKE_NES_PPU_RENDER_H

#include <stddef.h>
#include <stdint.h>

/*
 * Export-only helpers for the asset extractor: turn a source nametable
 * snapshot into an RGB preview image. The game itself never uses this; it
 * renders through native_scene.c and the citro3d backend.
 */
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

/* NTSC RGB for a source colour index 0..63; 0 when out of range. */
int nesPpuColorToRgb(uint8_t nesColor, uint8_t rgb[3]);

/* RGB preview of one nametable. palette holds the 16 background colours. */
int nesPpuRenderNametable(const uint8_t *chrPixels, size_t tileCount,
                          const uint8_t *nametable, const uint8_t *attributes,
                          const uint8_t *palette, uint8_t *rgb, size_t rgbCapacity);

/* Same, also reporting which pixels are non-zero colours. */
int nesPpuRenderNametableOpaque(const uint8_t *chrPixels, size_t tileCount,
                                const uint8_t *nametable, const uint8_t *attributes,
                                const uint8_t *palette, uint8_t *rgb,
                                size_t rgbCapacity, uint8_t *opaque,
                                size_t opaqueCapacity);

#endif
