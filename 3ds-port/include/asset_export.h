#ifndef EXCITEBIKE_ASSET_EXPORT_H
#define EXCITEBIKE_ASSET_EXPORT_H

#include <stddef.h>
#include <stdint.h>

#include "nes_rom_identify.h"

/*
 * Export source CHR, decoded 2bpp indices, grayscale atlas, source-derived
 * color PPU previews and metadata.
 *
 * rawChr/rawChrBytes carry the COMPLETE source CHR (16 bytes per tile, no
 * 960-tile cap): chr_pixels.bin and chr_pixels.ebp are streamed from it in
 * chunks and contain ALL tiles. pixels/decodedTileCount provide the eagerly
 * decoded leading tiles (exactly min(rawChrBytes/16, 960)) used to compose
 * chr_atlas.png and the ppu_preview_*.png images. identity, when non-NULL and
 * header-valid, adds honest header-level ROM identity lines to the manifest.
 */
int assetExportWrite(const char *directory, const uint8_t *rawChr, size_t rawChrBytes,
                     const uint8_t *pixels, size_t decodedTileCount,
                     const NesRomIdentity *identity,
                     char *status, size_t statusCapacity);

#endif
