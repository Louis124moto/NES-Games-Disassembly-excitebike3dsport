#include <string.h>
#include "nes_chr_decode.h"

#define NES_CHR_MAX_INPUT_BYTES (8u * 1024u * 1024u)

static int checkedChrSize(const uint8_t *rom, size_t *chrBytes)
{
    const unsigned int high = (unsigned int)(rom[9] & 0xF0u);
    const unsigned int low = (unsigned int)rom[5];
    if ((rom[7] & 0x0Cu) != 0x08u) {
        *chrBytes = (size_t)low * 8192u;
        return 1;
    }
    if (high != 0xF0u) {
        *chrBytes = (size_t)(((high >> 4u) << 8u) | low) * 8192u;
        return *chrBytes <= NES_CHR_MAX_INPUT_BYTES;
    }
    {
        const unsigned int exponent = low >> 2u;
        const unsigned int multiplier = low & 3u;
        size_t base;
        if (exponent >= sizeof(size_t) * 8u || exponent > 23u) return 0;
        base = (size_t)1u << exponent;
        if (base > NES_CHR_MAX_INPUT_BYTES / (2u * multiplier + 1u)) return 0;
        *chrBytes = base * (2u * multiplier + 1u);
    }
    return 1;
}

static int checkedPrgSize(const uint8_t *rom, size_t *prgBytes)
{
    if ((rom[7] & 0x0Cu) != 0x08u) {
        *prgBytes = (size_t)rom[4] * 16384u;
        return 1;
    }
    {
        const unsigned int high = (unsigned int)(rom[9] & 0x0Fu);
        const unsigned int low = (unsigned int)rom[4];
        if (high == 0x0Fu) {
            const unsigned int exponent = low >> 2u;
            const unsigned int multiplier = low & 3u;
            if (exponent >= sizeof(size_t) * 8u || exponent > 23u) return 0;
            *prgBytes = ((size_t)1u << exponent) * (2u * multiplier + 1u);
        } else {
            *prgBytes = (size_t)((high << 8u) | low) * 16384u;
        }
    }
    return *prgBytes <= NES_CHR_MAX_INPUT_BYTES;
}

NesChrDecodeResult nesChrGetRawData(const uint8_t *rom, size_t romSize,
                                    const uint8_t **chrData, size_t *chrBytes)
{
    size_t prgBytes, size, offset;
    if (chrData != NULL) *chrData = NULL;
    if (chrBytes != NULL) *chrBytes = 0u;
    if (rom == NULL || chrData == NULL || chrBytes == NULL)
        return NES_CHR_DECODE_INVALID_ARGUMENT;
    if (romSize < 16u) return NES_CHR_DECODE_SHORT_HEADER;
    if (memcmp(rom, "NES\x1A", 4u) != 0) return NES_CHR_DECODE_BAD_MAGIC;
    if (!checkedChrSize(rom, &size) || !checkedPrgSize(rom, &prgBytes))
        return NES_CHR_DECODE_UNSUPPORTED_NES2;
    if (size == 0u) return NES_CHR_DECODE_CHR_RAM;
    offset = 16u + ((rom[6] & 0x04u) ? 512u : 0u);
    if (offset > romSize || prgBytes > romSize - offset)
        return NES_CHR_DECODE_TRUNCATED_ROM;
    offset += prgBytes;
    if (offset > romSize || size > romSize - offset)
        return NES_CHR_DECODE_TRUNCATED_ROM;
    *chrData = rom + offset;
    *chrBytes = size;
    return NES_CHR_DECODE_OK;
}

static NesChrDecodeResult decodeChrBytes(const uint8_t *chr, size_t chrBytes,
                                                uint8_t *pixels, size_t maxTiles,
                                                size_t *tileCount)
{
    size_t availableTiles, count, tile, row, col;
    if (tileCount != NULL) *tileCount = 0u;
    if (chr == NULL || pixels == NULL || tileCount == NULL || maxTiles == 0u)
        return NES_CHR_DECODE_INVALID_ARGUMENT;
    if (chrBytes == 0u || (chrBytes % 16u) != 0u)
        return NES_CHR_DECODE_INVALID_RAW_SIZE;
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

NesChrDecodeResult nesChrDecodeRaw(const uint8_t *chr, size_t chrBytes,
                                   uint8_t *pixels, size_t maxTiles,
                                   size_t *tileCount)
{
    return decodeChrBytes(chr, chrBytes, pixels, maxTiles, tileCount);
}

NesChrDecodeResult nesChrDecodeInes(const uint8_t *rom, size_t romSize,
                                    uint8_t *pixels, size_t maxTiles,
                                    size_t *tileCount)
{
    const uint8_t *chr;
    size_t chrBytes;
    NesChrDecodeResult result;
    if (tileCount != NULL) *tileCount = 0u;
    if (rom == NULL || pixels == NULL || tileCount == NULL || maxTiles == 0u)
        return NES_CHR_DECODE_INVALID_ARGUMENT;
    result = nesChrGetRawData(rom, romSize, &chr, &chrBytes);
    if (result != NES_CHR_DECODE_OK) return result;
    return decodeChrBytes(chr, chrBytes, pixels, maxTiles, tileCount);
}

const char *nesChrDecodeError(NesChrDecodeResult result)
{
    switch (result) {
    case NES_CHR_DECODE_OK: return "CHR-ROM data located/decoded.";
    case NES_CHR_DECODE_INVALID_ARGUMENT: return "Invalid decoder argument.";
    case NES_CHR_DECODE_BAD_MAGIC: return "Invalid iNES magic.";
    case NES_CHR_DECODE_SHORT_HEADER: return "ROM smaller than header.";
    case NES_CHR_DECODE_UNSUPPORTED_NES2: return "Unsupported/oversize NES 2.0 size encoding.";
    case NES_CHR_DECODE_CHR_RAM: return "ROM uses CHR-RAM; runtime PPU capture is required.";
    case NES_CHR_DECODE_TRUNCATED_ROM: return "Header sizes exceed ROM file length.";
    case NES_CHR_DECODE_NO_TILES: return "No complete CHR tiles found.";
    case NES_CHR_DECODE_INVALID_RAW_SIZE: return "Raw CHR file must be a non-empty multiple of 16 bytes.";
    case NES_CHR_DECODE_OUTPUT_TOO_SMALL: return "Output buffer too small.";
    default: return "Unknown decoder error.";
    }
}
