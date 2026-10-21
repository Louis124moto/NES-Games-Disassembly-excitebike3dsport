#include <string.h>
#include "nes_chr_decode.h"

NesChrDecodeResult nesChrDecodeInes(const uint8_t *rom, size_t romSize,
                                    uint8_t *pixels, size_t maxTiles,
                                    size_t *tileCount)
{
    size_t prgBytes, chrBytes, chrOffset, availableTiles, count, tile, row, col;
    const uint8_t *chr;

    if (tileCount != NULL) *tileCount = 0u;
    if (rom == NULL || pixels == NULL || tileCount == NULL || maxTiles == 0u)
        return NES_CHR_DECODE_INVALID_ARGUMENT;
    if (romSize < 16u) return NES_CHR_DECODE_SHORT_HEADER;
    if (memcmp(rom, "NES\x1A", 4u) != 0) return NES_CHR_DECODE_BAD_MAGIC;
    if ((rom[7] & 0x0Cu) == 0x08u) return NES_CHR_DECODE_UNSUPPORTED_NES2;
    if (rom[5] == 0u) return NES_CHR_DECODE_CHR_RAM;

    prgBytes = (size_t)rom[4] * 16384u;
    chrBytes = (size_t)rom[5] * 8192u;
    chrOffset = 16u + ((rom[6] & 0x04u) ? 512u : 0u) + prgBytes;
    if (chrOffset > romSize || chrBytes > romSize - chrOffset)
        return NES_CHR_DECODE_TRUNCATED_ROM;

    chr = &rom[chrOffset];
    availableTiles = chrBytes / 16u;
    count = availableTiles < maxTiles ? availableTiles : maxTiles;
    for (tile = 0u; tile < count; ++tile) {
        const uint8_t *source = &chr[tile * 16u];
        for (row = 0u; row < 8u; ++row) {
            const uint8_t plane0 = source[row];
            const uint8_t plane1 = source[row + 8u];
            for (col = 0u; col < 8u; ++col) {
                const unsigned int bit = 7u - (unsigned int)col;
                pixels[tile * 64u + row * 8u + col] =
                    (uint8_t)(((plane0 >> bit) & 1u) |
                              (((plane1 >> bit) & 1u) << 1u));
            }
        }
    }
    *tileCount = count;
    return count == 0u ? NES_CHR_DECODE_NO_TILES : NES_CHR_DECODE_OK;
}

const char *nesChrDecodeError(NesChrDecodeResult result)
{
    switch (result) {
    case NES_CHR_DECODE_OK: return "CHR-ROM decoded.";
    case NES_CHR_DECODE_INVALID_ARGUMENT: return "Invalid CHR decoder argument.";
    case NES_CHR_DECODE_BAD_MAGIC: return "Invalid iNES header magic.";
    case NES_CHR_DECODE_SHORT_HEADER: return "ROM file is smaller than an iNES header.";
    case NES_CHR_DECODE_UNSUPPORTED_NES2: return "NES 2.0 headers are not supported yet.";
    case NES_CHR_DECODE_CHR_RAM: return "ROM declares CHR-RAM, not stored CHR-ROM.";
    case NES_CHR_DECODE_TRUNCATED_ROM: return "ROM header sizes exceed the file length.";
    case NES_CHR_DECODE_NO_TILES: return "No complete CHR tiles were available.";
    default: return "Unknown CHR decoder error.";
    }
}
