#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asset_extractor.h"
#include "nes_chr_decode.h"

/*
 * Runtime CHR inspection for the 3DS homebrew.
 * Put a legally obtained classic iNES ROM at:
 *   sdmc:/3ds/excitebike3ds/rom.nes
 *
 * This reads only uncompressed CHR-ROM patterns. It does not reconstruct
 * nametable/attribute layout, compressed PRG graphics, or PPU timing.
 */
#define EXTRACTOR_ROM_PATH "sdmc:/3ds/excitebike3ds/rom.nes"
#define EXTRACTOR_MAX_TILES 960u
#define EXTRACTOR_MAX_ROM_BYTES (8u * 1024u * 1024u)

static uint8_t s_pixels[EXTRACTOR_MAX_TILES][64];
static size_t s_tileCount;
static int s_active;
static int s_attempted;
static char s_status[160] = "Press Y to open CHR extractor.";

static void setStatus(const char *message)
{
    (void)snprintf(s_status, sizeof(s_status), "%s", message);
}

static void loadChrRom(void)
{
    FILE *file;
    uint8_t *rom;
    long fileSize;
    size_t romSize;
    NesChrDecodeResult result;

    s_attempted = 1;
    s_tileCount = 0u;
    memset(s_pixels, 0, sizeof(s_pixels));

    file = fopen(EXTRACTOR_ROM_PATH, "rb");
    if (file == NULL) {
        setStatus("ROM not found: sdmc:/3ds/excitebike3ds/rom.nes");
        return;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (fileSize = ftell(file)) < 16L ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        setStatus("ROM file is too small or cannot be read.");
        return;
    }
    if ((unsigned long)fileSize > (unsigned long)EXTRACTOR_MAX_ROM_BYTES) {
        fclose(file);
        setStatus("ROM exceeds the runtime extractor's 8 MiB safety limit.");
        return;
    }

    romSize = (size_t)fileSize;
    rom = (uint8_t *)malloc(romSize);
    if (rom == NULL) {
        fclose(file);
        setStatus("Not enough memory to read the ROM.");
        return;
    }
    if (fread(rom, 1u, romSize, file) != romSize) {
        free(rom);
        fclose(file);
        setStatus("Could not read the complete ROM file.");
        return;
    }
    fclose(file);

    result = nesChrDecodeInes(rom, romSize, &s_pixels[0][0],
                              EXTRACTOR_MAX_TILES, &s_tileCount);
    free(rom);
    if (result != NES_CHR_DECODE_OK) {
        s_tileCount = 0u;
        setStatus(nesChrDecodeError(result));
        return;
    }
    (void)snprintf(s_status, sizeof(s_status),
                   "Loaded %lu CHR tiles (showing up to %u).",
                   (unsigned long)s_tileCount, (unsigned int)EXTRACTOR_MAX_TILES);
}

void assetExtractorInit(void)
{
    s_active = 0;
    s_attempted = 0;
    s_tileCount = 0u;
    setStatus("Press Y to open the built-in CHR extractor.");
}

void assetExtractorUpdate(unsigned int pressedKeys)
{
    if ((pressedKeys & KEY_Y) != 0u) {
        s_active = !s_active;
        if (s_active && !s_attempted) loadChrRom();
    }
    if (!s_active) return;
    if ((pressedKeys & KEY_X) != 0u) loadChrRom();
    if ((pressedKeys & KEY_B) != 0u) s_active = 0;
}

int assetExtractorIsActive(void)
{
    return s_active;
}

const uint8_t *assetExtractorGetPixels(size_t *tileCount)
{
    if (tileCount != NULL) *tileCount = s_tileCount;
    return &s_pixels[0][0];
}

const char *assetExtractorGetStatus(void)
{
    return s_status;
}
