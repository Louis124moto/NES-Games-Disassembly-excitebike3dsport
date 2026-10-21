#ifndef EXCITEBIKE_NES_CHR_DECODE_H
#define EXCITEBIKE_NES_CHR_DECODE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    NES_CHR_DECODE_OK = 0,
    NES_CHR_DECODE_INVALID_ARGUMENT,
    NES_CHR_DECODE_BAD_MAGIC,
    NES_CHR_DECODE_SHORT_HEADER,
    NES_CHR_DECODE_UNSUPPORTED_NES2,
    NES_CHR_DECODE_CHR_RAM,
    NES_CHR_DECODE_TRUNCATED_ROM,
    NES_CHR_DECODE_NO_TILES,
    NES_CHR_DECODE_OUTPUT_TOO_SMALL,
    NES_CHR_DECODE_INVALID_RAW_SIZE
} NesChrDecodeResult;

/* Return the original CHR-ROM byte range within a validated iNES/NES 2.0 image. */
NesChrDecodeResult nesChrGetRawData(const uint8_t *rom, size_t romSize,
                                    const uint8_t **chrData, size_t *chrBytes);

/* Decode a standalone raw CHR dump (exactly 16 bytes per tile). */
NesChrDecodeResult nesChrDecodeRaw(const uint8_t *chr, size_t chrBytes,
                                   uint8_t *pixels, size_t maxTiles,
                                   size_t *tileCount);

/* Decode raw 2bpp CHR-ROM patterns from an iNES/NES 2.0 image in memory. */
NesChrDecodeResult nesChrDecodeInes(const uint8_t *rom, size_t romSize,
                                    uint8_t *pixels, size_t maxTiles,
                                    size_t *tileCount);
const char *nesChrDecodeError(NesChrDecodeResult result);

#endif
