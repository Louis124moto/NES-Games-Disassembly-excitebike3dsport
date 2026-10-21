#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "asset_extractor.h"
#include "asset_export.h"
#include "nes_chr_decode.h"

/*
 * Runtime CHR extraction/cache for the 3DS homebrew.
 * Put a legally obtained classic iNES ROM named rom.nes beside the launched
 * .3dsx. The path is derived from argv[0], not from a fixed SD-card folder.
 *
 * Decoded pixel indices are persisted beside the .3dsx at:
 *   extracted/chr_pixels.bin
 *
 * The cache stores decoded 2bpp pixel indices, not a reconstructed title
 * screen. Nametable/attribute layout, palette selection, compressed PRG
 * graphics, and PPU timing still require source tracing.
 */
#define EXTRACTOR_PATH_CAPACITY 512u
static char s_romPath[EXTRACTOR_PATH_CAPACITY] = "rom.nes";
static char s_cachePath[EXTRACTOR_PATH_CAPACITY] = "extracted/chr_cache.bin";
static char s_exportDirectory[EXTRACTOR_PATH_CAPACITY] = "extracted";
#define EXTRACTOR_MAX_TILES 960u
#define EXTRACTOR_MAX_ROM_BYTES (8u * 1024u * 1024u)
#define EXTRACTOR_CACHE_HEADER_SIZE 14u

static const uint8_t s_cacheMagic[8] = {'E', 'B', 'C', 'H', 'R', '0', '3', 0};
static uint32_t s_romFingerprint;
static uint8_t s_pixels[EXTRACTOR_MAX_TILES][64];
static size_t s_tileCount;
static int s_active;
static int s_attempted;
static size_t s_page;
static char s_status[160] = "Starting native CHR extraction...";

static void setStatus(const char *message)
{
    (void)snprintf(s_status, sizeof(s_status), "%s", message);
}

static void ensureExtractorDirectories(void)
{
    char directory[EXTRACTOR_PATH_CAPACITY];
    size_t length = strlen(s_cachePath);
    if (length >= sizeof(directory)) return;
    memcpy(directory, s_cachePath, length + 1u);
    while (length > 0u && directory[length - 1u] != '/' && directory[length - 1u] != '\\') --length;
    if (length == 0u) return;
    directory[length - 1u] = '\0';
    (void)mkdir(directory, 0777);
}

/* FNV-1a hash of the complete ROM, so a cache from another ROM is rejected. */
static int computeRomFingerprint(uint32_t *fingerprint)
{
    FILE *file = fopen(s_romPath, "rb");
    uint8_t chunk[1024];
    uint32_t hash = 2166136261u;
    size_t got;
    if (file == NULL) return 0;
    while ((got = fread(chunk, 1u, sizeof(chunk), file)) > 0u) {
        size_t i;
        for (i = 0u; i < got; ++i) {
            hash ^= chunk[i];
            hash *= 16777619u;
        }
    }
    if (ferror(file)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    *fingerprint = hash;
    return 1;
}

/* File format v3: magic, uint16 tile count, uint32 ROM fingerprint, decoded pixels. */
static int loadExtractedCache(void)
{
    FILE *file;
    uint8_t header[EXTRACTOR_CACHE_HEADER_SIZE];
    size_t expectedBytes;
    size_t readBytes;

    file = fopen(s_cachePath, "rb");
    if (file == NULL) return 0;

    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, s_cacheMagic, sizeof(s_cacheMagic)) != 0) {
        fclose(file);
        return 0;
    }

    s_tileCount = (size_t)header[8] | ((size_t)header[9] << 8);
    {
        const uint32_t cachedFingerprint = (uint32_t)header[10] |
            ((uint32_t)header[11] << 8u) | ((uint32_t)header[12] << 16u) |
            ((uint32_t)header[13] << 24u);
        if (cachedFingerprint != s_romFingerprint) {
            s_tileCount = 0u;
            fclose(file);
            return 0;
        }
    }

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
    header[10] = (uint8_t)(s_romFingerprint & 0xFFu);
    header[11] = (uint8_t)((s_romFingerprint >> 8u) & 0xFFu);
    header[12] = (uint8_t)((s_romFingerprint >> 16u) & 0xFFu);
    header[13] = (uint8_t)((s_romFingerprint >> 24u) & 0xFFu);

    file = fopen(s_cachePath, "wb");
    if (file == NULL) return 0;
    ok = fwrite(header, 1u, sizeof(header), file) == sizeof(header) &&
         fwrite(&s_pixels[0][0], 1u, pixelBytes, file) == pixelBytes;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int exportCurrentRomAssets(void)
{
    FILE *file;
    uint8_t *rom;
    long fileSize;
    size_t romSize;
    const uint8_t *rawChr = NULL;
    size_t rawChrBytes = 0u;
    char exportStatus[160];
    NesChrDecodeResult result;

    file = fopen(s_romPath, "rb");
    if (file == NULL) { setStatus("Cannot export: ROM not found beside the 3DSX."); return 0; }
    if (fseek(file, 0, SEEK_END) != 0 || (fileSize = ftell(file)) < 16L ||
        fseek(file, 0, SEEK_SET) != 0 || (unsigned long)fileSize > (unsigned long)EXTRACTOR_MAX_ROM_BYTES) {
        fclose(file); setStatus("Cannot export: ROM size is invalid or exceeds the 8 MiB limit."); return 0;
    }
    romSize = (size_t)fileSize;
    rom = (uint8_t *)malloc(romSize);
    if (rom == NULL) { fclose(file); setStatus("Cannot export: not enough memory for ROM."); return 0; }
    if (fread(rom, 1u, romSize, file) != romSize) {
        free(rom); fclose(file); setStatus("Cannot export: could not read complete ROM."); return 0;
    }
    fclose(file);
    result = nesChrGetRawData(rom, romSize, &rawChr, &rawChrBytes);
    if (result != NES_CHR_DECODE_OK) {
        free(rom); setStatus(nesChrDecodeError(result)); return 0;
    }
    if (!assetExportWrite(s_exportDirectory, rawChr, rawChrBytes, &s_pixels[0][0],
                          s_tileCount, exportStatus, sizeof(exportStatus))) {
        free(rom); setStatus(exportStatus); return 0;
    }
    free(rom);
    setStatus(exportStatus);
    return 1;
}

static void loadChrRom(void)
{
    FILE *file;
    uint8_t *rom;
    long fileSize;
    size_t romSize;
    NesChrDecodeResult result;
    const uint8_t *rawChr = NULL;
    size_t rawChrBytes = 0u;
    char exportStatus[160];
    int exportOk = 0;

    s_attempted = 1;
    s_tileCount = 0u;
    memset(s_pixels, 0, sizeof(s_pixels));

    file = fopen(s_romPath, "rb");
    if (file == NULL) {
        (void)snprintf(s_status, sizeof(s_status), "ROM not found beside the 3DSX: %s", s_romPath);
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

    /* Hash only after the ROM buffer has been allocated and read successfully. */
    {
        uint32_t hash = 2166136261u;
        size_t i;
        for (i = 0u; i < romSize; ++i) {
            hash ^= rom[i];
            hash *= 16777619u;
        }
        s_romFingerprint = hash;
    }

    result = nesChrDecodeInes(rom, romSize, &s_pixels[0][0],
                              EXTRACTOR_MAX_TILES, &s_tileCount);
    if (result == NES_CHR_DECODE_OK) {
        const NesChrDecodeResult rawResult = nesChrGetRawData(rom, romSize, &rawChr, &rawChrBytes);
        if (rawResult != NES_CHR_DECODE_OK) result = rawResult;
        else exportOk = assetExportWrite(s_exportDirectory, rawChr, rawChrBytes, &s_pixels[0][0],
                                         s_tileCount, exportStatus, sizeof(exportStatus));
    }
    free(rom);
    if (result != NES_CHR_DECODE_OK) {
        s_tileCount = 0u;
        setStatus(nesChrDecodeError(result));
        return;
    }

    {
        const int cacheOk = saveExtractedCache();
        if (cacheOk && exportOk) {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache and SD exports saved.",
                           (unsigned long)s_tileCount);
        } else if (cacheOk) {
        (void)snprintf(s_status, sizeof(s_status),
                       "Extracted %lu CHR tiles; cache saved, SD export failed (A to retry).",
                       (unsigned long)s_tileCount);
    } else {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache saved, SD export failed (A to retry).",
                           (unsigned long)s_tileCount);
        } else {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache save failed.",
                           (unsigned long)s_tileCount);
        }
    }
}

void assetExtractorInit(const char *executablePath)
{
    const char *slash = NULL;
    size_t directoryLength = 0u;
    size_t i;
    int pathValid = 1;

    /* Keep rom.nes beside the launched .3dsx; never depend on a fixed folder. */
    if (executablePath != NULL) {
        for (i = 0u; executablePath[i] != '\0'; ++i) {
            if (executablePath[i] == '/' || executablePath[i] == '\\') slash = &executablePath[i];
        }
    }
    if (slash != NULL) directoryLength = (size_t)(slash - executablePath) + 1u;
    if (directoryLength + sizeof("rom.nes") <= sizeof(s_romPath) && directoryLength + sizeof("extracted/chr_cache.bin") <= sizeof(s_cachePath) && directoryLength + sizeof("extracted") <= sizeof(s_exportDirectory)) {
        if (directoryLength > 0u) {
            memcpy(s_romPath, executablePath, directoryLength);
            memcpy(s_cachePath, executablePath, directoryLength);
            memcpy(s_exportDirectory, executablePath, directoryLength);
        }
        memcpy(s_romPath + directoryLength, "rom.nes", sizeof("rom.nes"));
        memcpy(s_cachePath + directoryLength, "extracted/chr_cache.bin", sizeof("extracted/chr_cache.bin"));
        memcpy(s_exportDirectory + directoryLength, "extracted", sizeof("extracted"));
    } else {
        (void)snprintf(s_status, sizeof(s_status), "Executable path too long; place rom.nes beside the 3DSX.");
        pathValid = 0;
    }

    s_active = 0;
    s_attempted = 0;
    s_page = 0u;
    s_tileCount = 0u;
    if (pathValid) {
        /*
         * Load the cache or attempt extraction immediately so the bottom-screen
         * log reports the result on startup without requiring the user to open
         * the viewer first. A opens/exports; B closes/retries; D-pad pages.
         */
        s_attempted = 1;
        setStatus("Checking CHR cache and ROM...");
        if (computeRomFingerprint(&s_romFingerprint)) {
            if (!loadExtractedCache()) loadChrRom();
            else (void)exportCurrentRomAssets();
        } else {
            loadChrRom();
        }
    }
}

void assetExtractorUpdate(unsigned int pressedKeys)
{
    if ((pressedKeys & KEY_A) != 0u) {
        if (!s_active) {
            s_active = 1;
            if (!s_attempted) {
                s_attempted = 1;
                if (!loadExtractedCache()) loadChrRom();
                else (void)exportCurrentRomAssets();
            }
        } else {
            (void)exportCurrentRomAssets();
        }
    }
    if ((pressedKeys & KEY_RIGHT) != 0u && s_tileCount > 0u) {
        const size_t pages = (s_tileCount + 374u) / 375u;
        s_page = (s_page + 1u) % pages;
    }
    if ((pressedKeys & KEY_LEFT) != 0u && s_tileCount > 0u) {
        const size_t pages = (s_tileCount + 374u) / 375u;
        s_page = (s_page + pages - 1u) % pages;
    }
    if ((pressedKeys & KEY_B) != 0u) {
        if (s_active) s_active = 0;
        else loadChrRom(); /* B retries extraction while the atlas is closed. */
    }
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

size_t assetExtractorGetPage(void)
{
    return s_page;
}

void assetExtractorSetPage(size_t page)
{
    const size_t pages = (s_tileCount + 374u) / 375u;
    s_page = pages == 0u ? 0u : page % pages;
}
