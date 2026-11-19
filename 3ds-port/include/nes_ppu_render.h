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
#define NES_PPU_NAMETABLE_PAGE_BYTES 1024u
#define NES_PPU_VERTICAL_MIRROR_BYTES (2u * NES_PPU_NAMETABLE_PAGE_BYTES)
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
    NES_PPU_RENDER_INVALID_PIXEL,
    NES_PPU_RENDER_UNSUPPORTED /* behind-background priority requires bgOpaque */
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

/*
 * Composite the 64 OAM sprites over an image produced by nesPpuRenderNametable.
 *   chrPixels/tileCount: the FULL decoded CHR.
 *                        8x8 sprites use PPUCTRL bit 3 (tiles 0..255 or 256..511).
 *                        8x16 sprites use tile bit 0 for the pattern table and the
 *                        even tile pair beginning at tile & $FE (PPUCTRL bit 3 ignored).
 *   oam                : 256 bytes, Y/tile/attribute/X per sprite
 *   palette            : the 32 palette RAM bytes (sprite palettes at 16..31, sprite
 *                        subpalette = attribute & 3, colour 0 transparent)
 * PPU rules applied: sprite Y is drawn one line lower (Y+1); sprites with Y >= $EF
 * are off-screen; lower OAM index wins; at most 8 sprites per scanline (the 9th and
 * later are dropped); PPUMASK bit 4 enables sprites and bit 2 shows them in the
 * leftmost 8 pixels; horizontal/vertical flip; 8x8 and 8x16 sprite heights. Attribute
 * bit 5 (behind the background) needs bgOpaque, a 256x240 map that is nonzero where
 * the background pixel is not colour 0 (see nesPpuRenderNametableOpaque); with
 * bgOpaque == NULL such a sprite makes the function return UNSUPPORTED. No pixels
 * are drawn on invalid or unsupported input. *drawn (optional) receives the number
 * of sprite pixels written.
 */
int nesPpuRenderSprites(const uint8_t *chrPixels, size_t tileCount,
                        const uint8_t *oam, const uint8_t *palette,
                        uint8_t ppuCtrl, uint8_t ppuMask,
                        const uint8_t *bgOpaque,
                        uint8_t *rgb, size_t rgbCapacity, size_t *drawn);

int nesPpuRenderNametableOpaque(const uint8_t *chrPixels, size_t tileCount,
                                const uint8_t *nametable, const uint8_t *attributes,
                                const uint8_t *palette, uint8_t *rgb,
                                size_t rgbCapacity, uint8_t *opaque,
                                size_t opaqueCapacity);

/* Compose two vertical-mirrored physical nametables with PPUCTRL base bits and $2005 scroll. */
int nesPpuRenderMirroredNametablesOpaque(
    const uint8_t *chrPixels, size_t tileCount,
    const uint8_t nametables[NES_PPU_VERTICAL_MIRROR_BYTES],
    const uint8_t *palette, uint8_t ppuCtrl, uint8_t scrollX, uint8_t scrollY,
    uint8_t *rgb, size_t rgbCapacity, uint8_t *opaque, size_t opaqueCapacity);

int nesPpuRenderNametable(const uint8_t *chrPixels, size_t tileCount,
                          const uint8_t *nametable,
                          const uint8_t *attributes,
                          const uint8_t *palette,
                          uint8_t *rgb, size_t rgbCapacity);

#endif
