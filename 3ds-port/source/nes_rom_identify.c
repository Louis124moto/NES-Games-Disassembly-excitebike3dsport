#include <stdio.h>
#include <string.h>
#include "nes_rom_identify.h"

NesRomIdentifyResult nesRomIdentifyHeader(const uint8_t *rom, size_t romSize,
                                          NesRomIdentity *identity)
{
    int isNes2;
    if (identity != NULL) {
        memset(identity, 0, sizeof(*identity));
        identity->headerValid = 0;
    }
    if (rom == NULL || identity == NULL) return NES_ROM_IDENTIFY_INVALID_ARGUMENT;
    if (romSize < 16u) return NES_ROM_IDENTIFY_SHORT_HEADER;
    if (memcmp(rom, "NES\x1A", 4u) != 0) return NES_ROM_IDENTIFY_BAD_MAGIC;

    isNes2 = (rom[7] & 0x0Cu) == 0x08u;
    identity->isNes2 = isNes2;
    identity->verticalMirroring = (rom[6] & 0x01u) != 0;
    identity->trainer = (rom[6] & 0x04u) != 0;
    if (!isNes2) {
        /* iNES: mapper low nibble from flags 6, high nibble from flags 7 low. */
        identity->mapper = (unsigned int)(rom[6] >> 4) |
                           ((unsigned int)(rom[7] & 0x0Fu) << 4);
    } else {
        /*
         * NES 2.0: flags 7 low nibble is the console/format field, NOT mapper
         * bits. Mapper = flags 6 high nibble | flags 7 HIGH nibble | flags 8
         * low nibble << 8. Reading flags 7 low nibble as mapper bits made a
         * canonical NES 2.0 mapper-0 header (flags 7 = $08) report mapper 128
         * on hardware (reported 2026-11-01).
         */
        identity->mapper = (unsigned int)(rom[6] >> 4) |
                           ((unsigned int)(rom[7] & 0xF0u)) |
                           (((unsigned int)(rom[8] & 0x0Fu)) << 8);
    }
    identity->usesChrRam = rom[5] == 0u;

    if (!isNes2) {
        identity->prgRomKib = (unsigned int)rom[4] * 16u;
        identity->chrRomKib = (unsigned int)rom[5] * 8u;
    } else {
        const unsigned int prgHigh = (unsigned int)(rom[9] & 0x0Fu);
        const unsigned int chrHigh = (unsigned int)((rom[9] >> 4u) & 0x0Fu);
        size_t prgBytes, chrBytes;
        if (prgHigh == 0x0Fu) {
            const unsigned int exponent = (unsigned int)rom[4] >> 2;
            const unsigned int multiplier = (unsigned int)rom[4] & 3u;
            if (exponent > 23u) return NES_ROM_IDENTIFY_UNSUPPORTED_NES2;
            prgBytes = ((size_t)1u << exponent) * (size_t)(2u * multiplier + 1u);
        } else {
            prgBytes = (((size_t)prgHigh << 8) | (size_t)rom[4]) * 16384u;
        }
        if (chrHigh == 0x0Fu) {
            const unsigned int exponent = (unsigned int)rom[5] >> 2;
            const unsigned int multiplier = (unsigned int)rom[5] & 3u;
            if (exponent > 23u) return NES_ROM_IDENTIFY_UNSUPPORTED_NES2;
            chrBytes = ((size_t)1u << exponent) * (size_t)(2u * multiplier + 1u);
        } else if (chrHigh == 0x0Eu) {
            chrBytes = 0u;
            identity->usesChrRam = 1;
        } else {
            chrBytes = (((size_t)chrHigh << 8) | (size_t)rom[5]) * 8192u;
        }
        if ((prgBytes % 1024u) != 0u || (chrBytes % 1024u) != 0u) {
            return NES_ROM_IDENTIFY_UNSUPPORTED_NES2;
        }
        identity->prgRomKib = (unsigned int)(prgBytes / 1024u);
        identity->chrRomKib = (unsigned int)(chrBytes / 1024u);
    }

    identity->headerValid = 1;
    return NES_ROM_IDENTIFY_OK;
}

int nesRomIdentityMatchesExcitebike(const NesRomIdentity *identity)
{
    if (identity == NULL || identity->headerValid == 0) return 0;
    return identity->mapper == 0u &&
           identity->prgRomKib == 16u &&
           identity->chrRomKib == 8u &&
           identity->usesChrRam == 0;
}

const char *nesRomIdentitySummary(const NesRomIdentity *identity,
                                  char *buffer, size_t capacity)
{
    if (identity == NULL || buffer == NULL || capacity == 0u) return NULL;
    if (identity->headerValid == 0) {
        (void)snprintf(buffer, capacity, "ROM header not identified.");
        return buffer;
    }
    (void)snprintf(buffer, capacity, "%s mapper %u, PRG %u KiB, CHR-%s %u KiB",
                   identity->isNes2 ? "NES 2.0" : "iNES",
                   identity->mapper,
                   identity->prgRomKib,
                   identity->usesChrRam ? "RAM" : "ROM",
                   identity->chrRomKib);
    return buffer;
}

const char *nesRomIdentifyError(NesRomIdentifyResult result)
{
    switch (result) {
    case NES_ROM_IDENTIFY_OK: return "ROM header parsed.";
    case NES_ROM_IDENTIFY_INVALID_ARGUMENT: return "Invalid identity argument.";
    case NES_ROM_IDENTIFY_SHORT_HEADER: return "ROM smaller than the iNES header.";
    case NES_ROM_IDENTIFY_BAD_MAGIC: return "Invalid iNES magic.";
    case NES_ROM_IDENTIFY_UNSUPPORTED_NES2: return "Unsupported NES 2.0 size encoding.";
    default: return "Unknown identity error.";
    }
}
