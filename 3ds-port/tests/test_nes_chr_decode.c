#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes_chr_decode.h"

static int testDecodePlanesAndOffset(void)
{
    uint8_t rom[16u + 16384u + 8192u];
    uint8_t pixels[2u * 64u];
    size_t count = 0u;
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
    for (i = 0u; i < 64u; ++i) if (pixels[i] != 0u) return 2;
    for (i = 0u; i < 64u; ++i) if (pixels[64u + i] != 1u) return 3;
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
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_OK) { free(rom); return 2; }
    rom[7] = 0x08u;
    if (nesChrDecodeInes(rom, size, pixels, 1u, &count) != NES_CHR_DECODE_UNSUPPORTED_NES2) { free(rom); return 3; }
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
    result = testTrainerAndInvalidHeaders();
    if (result != 0) { fprintf(stderr, "CHR header/bounds test failed: %d\n", result); return 1; }
    puts("PASS: iNES CHR parser, trainer offset, 2bpp decode and invalid-input bounds.");
    return 0;
}
