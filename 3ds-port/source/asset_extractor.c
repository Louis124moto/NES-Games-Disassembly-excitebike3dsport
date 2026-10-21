#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "asset_extractor.h"
#include "nes_chr_decode.h"

/*
 * Runtime CHR extraction/cache for the 3DS homebrew.
 * Put a legally obtained classic iNES ROM at:
 *   sdmc:/3ds/excitebike3ds/rom.nes
 *
 * Extracted decoded pixel indices are persisted at:
 *   sdmc:/3ds/excitebike3ds/extracted/chr_pixels.bin
 *
 * The cache stores decoded 2bpp pixel indices, not a reconstructed title
 * screen. Nametable/attribute layout, palette selection, compressed PRG
 * graphics, and PPU timing still require source tracing.
 */
#define EXTRACTOR_ROM_PATH "sdmc:/3ds/excitebike3ds/rom.nes"
#define EXTRACTOR_CACHE_PATH "sdmc:/3ds/excitebike3ds/extracted/chr_pixels.bin"
#define EXTRACTOR_MAX_TILES 960u
#define EXTRACTOR_MAX_ROM_BYTES (8u * 1024u * 1024u)
#define EXTRACTOR_CACHE_HEADER_SIZE 10u

static const uint8_t s_cacheMagic[8] = {'E', 'B', 'C', 'H', 'R', '0', '1', 0};
static uint8_t s_pixels[EXTRACTOR_MAX_TILES][64];
static size_t s_tileCount;
static int s_active;
static int s_attempted;
static char s_status[160] = "Press Y to open CHR extractor.";

static void setStatus(const char *message)
{
    (void)snprintf(s_status, sizeof(s_status), "%s", message);
}

static void ensureExtractorDirectories(void)
{
    (void)mkdir("sdmc:/3ds", 0777);
    (void)mkdir("sdmc:/3ds/excitebike3ds", 0777);
    (void)mkdir("sdmc:/3ds/excitebike3ds/extracted", 0777);
}

/* File format: 8-byte magic, uint16 little-endian tile count, 64 bytes/tile. */
static int loadExtractedCache(void)
{
    FILE *file;
    uint8_t header[EXTRACTOR_CACHE_HEADER_SIZE];
    size_t expectedBytes;
    size_t readBytes;

    file = fopen(EXTRACTOR_CACHE_PATH, "rb");
    if (file == NULL) return 0;

    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, s_cacheMagic, sizeof(s_cacheMagic)) != 0) {
        fclose(file);
        return 0;
    }

    s_tileCount = (size_t)header[8] | ((size_t)header[9] << 8);
    if (s_tileCount == 0u || s_tileCount > EXTRACTOR_MAX_TILES) {
        s_tileCount = 0u;
        fclose(file);
        return 0;
    }

    expectedBytes = s_tileCount * 64u;
    readBytes = fread(&s_pixels[0][0], 1u, expectedBytes, file);
    if (readBytes != expectedBytes || fgetc(file) != EOF) {
        s_tileCount = 0u;
        fclose(file);
        return 0;
    }
    fclose(file);

    {
        size_t i;
        for (i = 0u; i < expectedBytes; ++i) {
            if (((const uint8_t *)s_pixels)[i] > 3u) {
                s_tileCount = 0u;
                return 0;
            }
        }
    }
    (void)snprintf(s_status, sizeof(s_status),
                   "Loaded %lu CHR tiles from extracted cache.",
                   (unsigned long)s_tileCount);
    return 1;
}

static int saveExtractedCache(void)
{
    FILE *file;
    uint8_t header[EXTRACTOR_CACHE_HEADER_SIZE];
    size_t pixelBytes = s_tileCount * 64u;
    int ok;

    if (s_tileCount == 0u || s_tileCount > EXTRACTOR_MAX_TILES) return 0;
    ensureExtractorDirectories();

    memcpy(header, s_cacheMagic, sizeof(s_cacheMagic));
    header[8] = (uint8_t)(s_tileCount & 0xFFu);
    header[9] = (uint8_t)((s_tileCount >> 8) & 0xFFu);

    file = fopen(EXTRACTOR_CACHE_PATH, "wb");
    if (file == NULL) return 0;
    ok = fwrite(header, 1u, sizeof(header), file) == sizeof(header) &&
         fwrite(&s_pixels[0][0], 1u, pixelBytes, file) == pixelBytes;
    if (fclose(file) != 0) ok = 0;
    return ok;
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

    if (saveExtractedCache()) {
        (void)snprintf(s_status, sizeof(s_status),
                       "Extracted %lu CHR tiles; cache saved.",
                       (unsigned long)s_tileCount);
    } else {
        (void)snprintf(s_status, sizeof(s_status),
                       "Extracted %lu CHR tiles; cache save failed.",
                       (unsigned long)s_tileCount);
    }
}

void assetExtractorInit(void)
{
    s_active = 0;
    s_attempted = 0;
    s_tileCount = 0u;
    setStatus("Press Y to load/extract CHR texture data.");
}

void assetExtractorUpdate(unsigned int pressedKeys)
{
    if ((pressedKeys & KEY_Y) != 0u) {
        s_active = !s_active;
        if (s_active && !s_attempted) {
            s_attempted = 1;
            if (!loadExtractedCache()) loadChrRom();
        }
    }
    if (!s_active) return;
    /* X explicitly re-reads the ROM and refreshes the persisted cache. */
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
