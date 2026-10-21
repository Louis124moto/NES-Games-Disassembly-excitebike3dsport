#include <string.h>
#include "nes_chr_decode.h"

static int checkedChrSize(const uint8_t *rom, size_t *chrBytes)
{
    const unsigned int high = (unsigned int)(rom[9] & 0xF0u);
    const unsigned int low = (unsigned int)rom[5];
    if ((rom[7] & 0x0Cu) != 0x08u) {
        *chrBytes = (size_t)low * 8192u;
        return 1;
    }

    /* NES 2.0: byte 9 high nibble normally extends byte 5. */
    if (high != 0xF0u) {
        *chrBytes = (size_t)(((high >> 4u) << 8u) | low) * 8192u;
        return 1;
    }

    /*
     * NES 2.0 exponent/multiplier form:
     * size = 2^E * (2*M + 1), with E=byte5>>2 and M=byte5&3.
     * Reject sizes that cannot be represented safely or exceed this runtime's
     * documented 8 MiB input limit before doing any shifts.
     */
    {
        const unsigned int exponent = low >> 2u;
        const unsigned int multiplier = low & 3u;
        size_t base;
        if (exponent >= sizeof(size_t) * 8u || exponent > 23u) return 0;
        base = (size_t)1u << exponent;
        if (base > (8u * 1024u * 1024u) / (2u * multiplier + 1u)) return 0;
        *chrBytes = base * (2u * multiplier + 1u);
    }
    return 1;
}

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
    if (memcmp(rom, "NES\\x1A", 4u) != 0) return NES_CHR_DECODE_BAD_MAGIC;

    if (!checkedChrSize(rom, &chrBytes))
        return NES_CHR_DECODE_UNSUPPORTED_NES2;
    if (chrBytes == 0u) return NES_CHR_DECODE_CHR_RAM;

    /* NES 2.0 PRG-ROM uses the same size encoding in bytes 4 and 9. */
    if ((rom[7] & 0x0Cu) == 0x08u) {
        const unsigned int prgHigh = (unsigned int)(rom[9] & 0x0Fu);
        const unsigned int prgLow = (unsigned int)rom[4];
        if (prgHigh == 0x0Fu) {
            const unsigned int exponent = prgLow >> 2u;
            const unsigned int multiplier = prgLow & 3u;
            if (exponent >= sizeof(size_t) * 8u || exponent > 23u)
                return NES_CHR_DECODE_TRUNCATED_ROM;
            prgBytes = ((size_t)1u << exponent) * (2u * multiplier + 1u);
        } else {
            prgBytes = (size_t)((prgHigh << 8u) | prgLow) * 16384u;
        }
    } else {
        prgBytes = (size_t)rom[4] * 16384u;
    }

    chrOffset = 16u + ((rom[6] & 0x04u) ? 512u : 0u);
    if (prgBytes > romSize - chrOffset) return NES_CHR_DECODE_TRUNCATED_ROM;
    chrOffset += prgBytes;
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
    case NES_CHR_DECODE_INVALID_ARGUMENT: return "Invalid decoder argument.";
    case NES_CHR_DECODE_BAD_MAGIC: return "Invalid iNES magic.";
    case NES_CHR_DECODE_SHORT_HEADER: return "ROM smaller than header.";
    case NES_CHR_DECODE_UNSUPPORTED_NES2: return "Unsupported/oversize NES 2.0 size encoding.";
    case NES_CHR_DECODE_CHR_RAM: return "ROM uses CHR-RAM, not CHR-ROM.";
    case NES_CHR_DECODE_TRUNCATED_ROM: return "Header sizes exceed ROM file length.";
    case NES_CHR_DECODE_NO_TILES: return "No complete CHR tiles found.";
    default: return "Unknown decoder error.";
    }
}
