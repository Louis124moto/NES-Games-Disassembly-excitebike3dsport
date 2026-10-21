#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asset_extractor.h"

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
#define EXTRACTOR_TILE_BYTES 16u
#define EXTRACTOR_CHR_BYTES (EXTRACTOR_MAX_TILES * EXTRACTOR_TILE_BYTES)

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
    uint8_t header[16];
    long fileSize;
    size_t prgBytes, chrBytes, chrOffset, bytesToRead, tile, row, col;
    uint8_t tileBytes[EXTRACTOR_TILE_BYTES];

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
    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, "NES\x1A", 4u) != 0) {
        fclose(file);
        setStatus("Invalid iNES header. Expected NES\\x1A magic.");
        return;
    }
    if ((header[7] & 0x0Cu) == 0x08u) {
        fclose(file);
        setStatus("NES 2.0 ROM parsing is not supported by this runtime extractor yet.");
        return;
    }
    if (header[5] == 0u) {
        fclose(file);
        setStatus("ROM declares CHR-RAM, not stored CHR-ROM; no tiles to extract.");
        return;
    }

    prgBytes = (size_t)header[4] * 16384u;
    chrBytes = (size_t)header[5] * 8192u;
    chrOffset = 16u + ((header[6] & 0x04u) ? 512u : 0u) + prgBytes;
    if (chrOffset > (size_t)fileSize || chrBytes > (size_t)fileSize - chrOffset) {
        fclose(file);
        setStatus("ROM header sizes exceed file length; extraction aborted.");
        return;
    }
    if (fseek(file, (long)chrOffset, SEEK_SET) != 0) {
        fclose(file);
        setStatus("Cannot seek to CHR-ROM data.");
        return;
    }

    bytesToRead = chrBytes < EXTRACTOR_CHR_BYTES ? chrBytes : EXTRACTOR_CHR_BYTES;
    for (tile = 0u; tile < bytesToRead / EXTRACTOR_TILE_BYTES; ++tile) {
        if (fread(tileBytes, 1u, sizeof(tileBytes), file) != sizeof(tileBytes)) {
            fclose(file);
            s_tileCount = 0u;
            setStatus("Unexpected end of CHR-ROM data.");
            return;
        }
        for (row = 0u; row < 8u; ++row) {
            uint8_t plane0 = tileBytes[row];
            uint8_t plane1 = tileBytes[row + 8u];
            for (col = 0u; col < 8u; ++col) {
                const unsigned int bit = 7u - (unsigned int)col;
                s_pixels[tile][row * 8u + col] =
                    (uint8_t)(((plane0 >> bit) & 1u) |
                              (((plane1 >> bit) & 1u) << 1u));
            }
        }
    }
    fclose(file);
    s_tileCount = bytesToRead / EXTRACTOR_TILE_BYTES;
    (void)snprintf(s_status, sizeof(s_status),
                   "Loaded %lu CHR tiles from ROM (showing up to %u).",
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
    return s_pixels[0];
}

const char *assetExtractorGetStatus(void)
{
    return s_status;
}
