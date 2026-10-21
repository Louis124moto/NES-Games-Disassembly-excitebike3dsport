#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_chr_decode.h"

static int testDecodePlanesAndOffset(void)
{
    uint8_t rom[16u + 16384u + 8192u];
    uint8_t pixels[2u * 64u];
    size_t count = 0u;
    const uint8_t *raw = NULL;
    size_t rawBytes = 0u;
    unsigned int i;
    memset(rom, 0, sizeof(rom));
    memcpy(rom, "NES\x1A", 4u);
    rom[4] = 1u; /* one 16 KiB PRG bank */
    rom[5] = 1u; /* one 8 KiB CHR bank */
    memset(&rom[16u + 16384u], 0, 16u);
    memset(&rom[16u + 16384u + 16u], 0xFF, 8u); /* plane 0 tile 1 */
    memset(&rom[16u + 16384u + 24u], 0, 8u);
    if (nesChrDecodeInes(rom, sizeof(rom), pixels, 2u, &count) != NES_CHR_DECODE_OK || count != 2u)
        return 1;
    if (nesChrGetRawData(rom, sizeof(rom), &raw, &rawBytes) != NES_CHR_DECODE_OK ||
        rawBytes != 8192u || raw != &rom[16u + 16384u]) return 4;
    for (i = 0u; i < 64u; ++i) if (pixels[i] != 0u) return 2;
    for (i = 0u; i < 64u; ++i) if (pixels[64u + i] != 1u) return 3;
    return 0;
}

static int testRawChrDecode(void)
{
    uint8_t raw[32] = {0};
    uint8_t pixels[2u * 64u];
    size_t count = 0u;
    memset(&raw[16u], 0xFF, 8u);
    if (nesChrDecodeRaw(raw, sizeof(raw), pixels, 2u, &count) != NES_CHR_DECODE_OK || count != 2u) return 1;
    if (pixels[0] != 0u || pixels[64u] != 1u || pixels[71u] != 1u || pixels[72u] != 0u) return 2;
    if (nesChrDecodeRaw(raw, sizeof(raw) - 1u, pixels, 2u, &count) != NES_CHR_DECODE_INVALID_RAW_SIZE) return 3;
    return 0;
}

static int testTrainerAndInvalidHeaders(void)
{
    uint8_t *rom = (uint8_t *)calloc(1u, 16u + 512u + 16384u + 8192u);
    uint8_t pixels[64];
    size_t count = 0u;
    size_t size = 16u + 512u + 16384u + 8192u;
    if (rom == NULL) return 1;
    memcpy(rom, "NES\x1A", 4u); rom[4] = 1u; rom[5] = 1u; rom[6] = 0x04u;
    rom[16u + 512u + 16384u] = 0xFFu; /* first plane row after trainer and PRG */
    /* The nonzero row must be read after the optional trainer, not from it. */
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_OK) { free(rom); return 2; }
    {
        unsigned int x;
        for (x = 0u; x < 8u; ++x) if (pixels[x] != 1u) { free(rom); return 7; }
        if (pixels[8u] != 0u) { free(rom); return 8; }
    }
    /* NES 2.0 with ordinary 8 KiB CHR size fields is supported. */
    rom[7] = 0x08u;
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_OK || count != 1u) { free(rom); return 3; }
    rom[7] = 0u; rom[5] = 0u;
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_CHR_RAM) { free(rom); return 4; }
    rom[5] = 1u;
    if (nesChrDecodeInes(rom, size - 1u, pixels, 1u, &count) != NES_CHR_DECODE_TRUNCATED_ROM) { free(rom); return 5; }
    rom[0] = 'X';
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_BAD_MAGIC) { free(rom); return 6; }
    free(rom);
    return 0;
}

int main(void)
{
    int result = testDecodePlanesAndOffset();
    if (result != 0) { fprintf(stderr, "CHR plane/offset test failed: %d\n", result); return 1; }
    result = testRawChrDecode();
    if (result != 0) { fprintf(stderr, "Raw CHR test failed: %d\\n", result); return 1; }
    result = testTrainerAndInvalidHeaders();
    if (result != 0) { fprintf(stderr, "CHR header/bounds test failed: %d\n", result); return 1; }
    puts("PASS: iNES CHR parser, trainer offset, 2bpp decode and invalid-input bounds.");
    return 0;
}
