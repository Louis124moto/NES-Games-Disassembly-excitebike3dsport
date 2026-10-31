#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "asset_extractor.h"
#include "asset_export.h"
#include "nes_chr_cache.h"
#include "nes_chr_decode.h"
#include "nes_ppu_commands.h"
#include "nes_rom_identify.h"

/*
 * Runtime CHR extraction/cache for the 3DS homebrew.
 * Prefer rom.nes beside the launched .3dsx. If it is absent, accept a verified
 * raw CHR dump named CHR_ROM.chr. The path derives from argv[0], not a fixed folder.
 *
 * Decoded pixel indices are persisted beside the .3dsx at:
 *   extracted/chr_pixels.bin
 *
 * The cache stores decoded 2bpp pixel indices, not a reconstructed title
 * screen. Raw CHR fallback cannot provide palettes, nametables, sprites/OAM,
 * compressed PRG graphics, or PPU timing.
 *
 * The eager decoded buffer holds at most EXTRACTOR_MAX_TILES tiles; the
 * complete raw CHR is retained on the heap so exports contain ALL tiles and
 * the atlas view decodes pages on demand. The decoded-pixel cache is only
 * written when the whole source fits the eager cap.
 */
#define EXTRACTOR_PATH_CAPACITY 512u
static char s_romPath[EXTRACTOR_PATH_CAPACITY] = "rom.nes";
static char s_rawChrPath[EXTRACTOR_PATH_CAPACITY] = "CHR_ROM.chr";
static char s_sourcePath[EXTRACTOR_PATH_CAPACITY] = "rom.nes";
static int s_sourceIsRaw;
static char s_cachePath[EXTRACTOR_PATH_CAPACITY] = "extracted/chr_cache.bin";
static char s_exportDirectory[EXTRACTOR_PATH_CAPACITY] = "extracted";
#define EXTRACTOR_MAX_TILES 960u
#define EXTRACTOR_PAGE_TILES 375u
#define EXTRACTOR_MAX_ROM_BYTES (8u * 1024u * 1024u)

static uint32_t s_romFingerprint;
static uint8_t s_pixels[EXTRACTOR_MAX_TILES][64];
static uint8_t s_pagePixels[EXTRACTOR_PAGE_TILES][64];
static uint8_t *s_rawChr;
static size_t s_rawChrBytes;
static size_t s_totalTiles; /* complete source tile count (rawChrBytes/16) */
static size_t s_tileCount;  /* eager decoded count = min(s_totalTiles, 960) */
static NesRomIdentity s_identity;
static char s_identitySummary[160] = "Pending ROM header check.";
static int s_active;
static int s_attempted;
static size_t s_page;
static unsigned int s_viewMode;
static size_t s_paletteIndex;
static size_t s_ppuStreamIndex;
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

static void selectSourcePath(void)
{
    FILE *file = fopen(s_romPath, "rb");
    if (file != NULL) {
        fclose(file);
        (void)snprintf(s_sourcePath, sizeof(s_sourcePath), "%s", s_romPath);
        s_sourceIsRaw = 0;
        return;
    }
    file = fopen(s_rawChrPath, "rb");
    if (file != NULL) {
        fclose(file);
        (void)snprintf(s_sourcePath, sizeof(s_sourcePath), "%s", s_rawChrPath);
        s_sourceIsRaw = 1;
        return;
    }
    (void)snprintf(s_sourcePath, sizeof(s_sourcePath), "%s", s_romPath);
    s_sourceIsRaw = 0;
}

/* FNV-1a hash of the complete ROM, so a cache from another ROM is rejected. */
static int computeRomFingerprint(uint32_t *fingerprint)
{
    FILE *file = fopen(s_sourcePath, "rb");
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

/*
 * Retain the complete raw CHR on the heap (bounded by the 8 MiB ROM limit) so
 * every tile stays available for streamed exports and on-demand page decode.
 */
static void retainRawChr(const uint8_t *rawChr, size_t rawChrBytes)
{
    uint8_t *copy;
    if (rawChr == NULL || rawChrBytes == 0u ||
        rawChrBytes > EXTRACTOR_MAX_ROM_BYTES) return;
    copy = (uint8_t *)malloc(rawChrBytes);
    if (copy == NULL) return;
    memcpy(copy, rawChr, rawChrBytes);
    free(s_rawChr);
    s_rawChr = copy;
    s_rawChrBytes = rawChrBytes;
}

/* Header-level ROM identity for the loaded source; honest when raw/unreadable. */
static void refreshRomIdentity(const uint8_t *rom, size_t romSize)
{
    memset(&s_identity, 0, sizeof(s_identity));
    if (s_sourceIsRaw || rom == NULL) {
        (void)snprintf(s_identitySummary, sizeof(s_identitySummary),
                       "Raw CHR source; ROM identity unavailable.");
        return;
    }
    if (nesRomIdentifyHeader(rom, romSize, &s_identity) == NES_ROM_IDENTIFY_OK) {
        (void)nesRomIdentitySummary(&s_identity, s_identitySummary, sizeof(s_identitySummary));
    } else {
        (void)snprintf(s_identitySummary, sizeof(s_identitySummary),
                       "ROM header not identified.");
    }
}

/* File format v3 via the host-testable nes_chr_cache module. */
static int loadExtractedCache(void)
{
    size_t cachedTiles = 0u;
    if (nesChrCacheLoad(s_cachePath, s_romFingerprint, &s_pixels[0][0],
                       EXTRACTOR_MAX_TILES, &cachedTiles) != NES_CHR_CACHE_OK) {
        s_tileCount = 0u;
        return 0;
    }
    s_tileCount = cachedTiles;
    s_totalTiles = s_tileCount;
    (void)snprintf(s_status, sizeof(s_status),
                   "Loaded %lu CHR tiles from extracted cache.",
                   (unsigned long)s_tileCount);
    return 1;
}

/* 1 = saved, 0 = failed, -1 = skipped because the source exceeds 960 tiles. */
static int saveExtractedCache(void)
{
    if (s_tileCount == 0u) return 0;
    if (s_totalTiles > EXTRACTOR_MAX_TILES) return -1;
    ensureExtractorDirectories();
    return nesChrCacheSave(s_cachePath, &s_pixels[0][0], s_tileCount,
                           s_romFingerprint) == NES_CHR_CACHE_OK ? 1 : 0;
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

    selectSourcePath();
    file = fopen(s_sourcePath, "rb");
    if (file == NULL) { setStatus("No rom.nes or CHR_ROM.chr found beside the 3DSX."); return 0; }
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
    if (s_sourceIsRaw) {
        if (romSize == 0u || (romSize % 16u) != 0u) {
            free(rom); setStatus("Raw CHR file size must be a non-empty multiple of 16 bytes."); return 0;
        }
        rawChr = rom;
        rawChrBytes = romSize;
    } else {
        result = nesChrGetRawData(rom, romSize, &rawChr, &rawChrBytes);
        if (result != NES_CHR_DECODE_OK) {
            free(rom); setStatus(nesChrDecodeError(result)); return 0;
        }
    }
    refreshRomIdentity(rom, romSize);
    s_totalTiles = rawChrBytes / 16u;
    if (nesChrDecodeRaw(rawChr, rawChrBytes, &s_pixels[0][0],
                        EXTRACTOR_MAX_TILES, &s_tileCount) != NES_CHR_DECODE_OK) {
        s_tileCount = 0u;
    }
    retainRawChr(rawChr, rawChrBytes);
    if (!assetExportWrite(s_exportDirectory, rawChr, rawChrBytes, &s_pixels[0][0],
                          s_tileCount, &s_identity, exportStatus, sizeof(exportStatus))) {
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
    s_totalTiles = 0u;
    memset(s_pixels, 0, sizeof(s_pixels));

    selectSourcePath();
    file = fopen(s_sourcePath, "rb");
    if (file == NULL) {
        (void)snprintf(s_status, sizeof(s_status), "No rom.nes or CHR_ROM.chr beside the 3DSX.");
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

    if (s_sourceIsRaw) {
        rawChr = rom;
        rawChrBytes = romSize;
        result = nesChrDecodeRaw(rom, romSize, &s_pixels[0][0],
                                 EXTRACTOR_MAX_TILES, &s_tileCount);
    } else {
        result = nesChrDecodeInes(rom, romSize, &s_pixels[0][0],
                                  EXTRACTOR_MAX_TILES, &s_tileCount);
        if (result == NES_CHR_DECODE_OK) {
            const NesChrDecodeResult rawResult = nesChrGetRawData(rom, romSize, &rawChr, &rawChrBytes);
            if (rawResult != NES_CHR_DECODE_OK) result = rawResult;
        }
    }
    if (result == NES_CHR_DECODE_OK) {
        s_totalTiles = rawChrBytes / 16u;
        refreshRomIdentity(rom, romSize);
        exportOk = assetExportWrite(s_exportDirectory, rawChr, rawChrBytes, &s_pixels[0][0],
                                   s_tileCount, &s_identity, exportStatus, sizeof(exportStatus));
        retainRawChr(rawChr, rawChrBytes);
    }
    free(rom);
    if (result != NES_CHR_DECODE_OK) {
        s_tileCount = 0u;
        s_totalTiles = 0u;
        setStatus(nesChrDecodeError(result));
        return;
    }

    {
        const int cacheOk = saveExtractedCache();
        if (cacheOk == 1 && exportOk) {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache and SD exports saved.",
                           (unsigned long)s_totalTiles);
        } else if (cacheOk == 1) {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache saved, SD export failed (A to retry).",
                           (unsigned long)s_totalTiles);
        } else if (cacheOk == -1) {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; pixel cache skipped (>960).",
                           (unsigned long)s_totalTiles);
        } else {
            (void)snprintf(s_status, sizeof(s_status),
                           "Extracted %lu CHR tiles; cache save failed.",
                           (unsigned long)s_totalTiles);
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
    if (directoryLength + sizeof("rom.nes") <= sizeof(s_romPath) && directoryLength + sizeof("CHR_ROM.chr") <= sizeof(s_rawChrPath) && directoryLength + sizeof("extracted/chr_cache.bin") <= sizeof(s_cachePath) && directoryLength + sizeof("extracted") <= sizeof(s_exportDirectory)) {
        if (directoryLength > 0u) {
            memcpy(s_romPath, executablePath, directoryLength);
            memcpy(s_rawChrPath, executablePath, directoryLength);
            memcpy(s_cachePath, executablePath, directoryLength);
            memcpy(s_exportDirectory, executablePath, directoryLength);
        }
        memcpy(s_romPath + directoryLength, "rom.nes", sizeof("rom.nes"));
        memcpy(s_rawChrPath + directoryLength, "CHR_ROM.chr", sizeof("CHR_ROM.chr"));
        memcpy(s_cachePath + directoryLength, "extracted/chr_cache.bin", sizeof("extracted/chr_cache.bin"));
        memcpy(s_exportDirectory + directoryLength, "extracted", sizeof("extracted"));
    } else {
        (void)snprintf(s_status, sizeof(s_status), "Executable path too long; place rom.nes beside the 3DSX.");
        pathValid = 0;
    }

    s_active = 0;
    s_attempted = 0;
    s_page = 0u;
    s_viewMode = 0u;
    s_paletteIndex = 0u;
    s_ppuStreamIndex = 0u;
    s_tileCount = 0u;
    s_totalTiles = 0u;
    free(s_rawChr);
    s_rawChr = NULL;
    s_rawChrBytes = 0u;
    memset(&s_identity, 0, sizeof(s_identity));
    (void)snprintf(s_identitySummary, sizeof(s_identitySummary), "Pending ROM header check.");
    if (pathValid) {
        /*
         * Load the cache or attempt extraction immediately so the bottom-screen
         * log reports the result on startup without requiring the user to open
         * the viewer first. A opens/exports; B closes/retries; D-pad pages.
         */
        s_attempted = 1;
        selectSourcePath();
        setStatus("Checking CHR cache and source data...");
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
    if (s_active && (pressedKeys & KEY_UP) != 0u) {
        s_viewMode = (s_viewMode + 1u) % 5u;
    } else if (s_active && (pressedKeys & KEY_DOWN) != 0u) {
        s_viewMode = (s_viewMode + 4u) % 5u;
    }
    if (s_active && s_viewMode == 1u) {
        if ((pressedKeys & KEY_RIGHT) != 0u) s_paletteIndex = (s_paletteIndex + 1u) % 7u;
        if ((pressedKeys & KEY_LEFT) != 0u) s_paletteIndex = (s_paletteIndex + 6u) % 7u;
    } else if (s_active && s_viewMode == 2u) {
        if ((pressedKeys & KEY_RIGHT) != 0u) s_ppuStreamIndex = (s_ppuStreamIndex + 1u) % NES_EXCITEBIKE_PPU_STREAM_COUNT;
        if ((pressedKeys & KEY_LEFT) != 0u) s_ppuStreamIndex = (s_ppuStreamIndex + NES_EXCITEBIKE_PPU_STREAM_COUNT - 1u) % NES_EXCITEBIKE_PPU_STREAM_COUNT;
    } else if (s_active && s_viewMode == 0u) {
        if ((pressedKeys & KEY_RIGHT) != 0u && s_totalTiles > 0u) {
            const size_t pages = (s_totalTiles + EXTRACTOR_PAGE_TILES - 1u) / EXTRACTOR_PAGE_TILES;
            s_page = (s_page + 1u) % pages;
        }
        if ((pressedKeys & KEY_LEFT) != 0u && s_totalTiles > 0u) {
            const size_t pages = (s_totalTiles + EXTRACTOR_PAGE_TILES - 1u) / EXTRACTOR_PAGE_TILES;
            s_page = (s_page + pages - 1u) % pages;
        }
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

const uint8_t *assetExtractorGetPagePixels(size_t *pageTileCount, size_t *pageFirstTile)
{
    const size_t firstTile = s_page * EXTRACTOR_PAGE_TILES;
    size_t count;
    if (pageTileCount != NULL) *pageTileCount = 0u;
    if (pageFirstTile != NULL) *pageFirstTile = 0u;
    if (s_totalTiles == 0u || firstTile >= s_totalTiles) return NULL;
    count = s_totalTiles - firstTile;
    if (count > EXTRACTOR_PAGE_TILES) count = EXTRACTOR_PAGE_TILES;
    if (s_rawChr != NULL) {
        size_t decoded = 0u;
        if (nesChrDecodeRaw(s_rawChr + firstTile * 16u, count * 16u,
                            &s_pagePixels[0][0], EXTRACTOR_PAGE_TILES,
                            &decoded) != NES_CHR_DECODE_OK || decoded != count) {
            return NULL;
        }
    } else if (firstTile < s_tileCount) {
        size_t eagerCount = s_tileCount - firstTile;
        if (eagerCount > count) eagerCount = count;
        memcpy(&s_pagePixels[0][0], &s_pixels[firstTile][0], eagerCount * 64u);
        count = eagerCount;
    } else {
        return NULL;
    }
    if (pageTileCount != NULL) *pageTileCount = count;
    if (pageFirstTile != NULL) *pageFirstTile = firstTile;
    return &s_pagePixels[0][0];
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
    const size_t pages = (s_totalTiles + EXTRACTOR_PAGE_TILES - 1u) / EXTRACTOR_PAGE_TILES;
    s_page = pages == 0u ? 0u : page % pages;
}

unsigned int assetExtractorGetViewMode(void)
{
    return s_viewMode;
}

size_t assetExtractorGetPaletteIndex(void)
{
    return s_paletteIndex;
}

size_t assetExtractorGetPpuStreamIndex(void)
{
    return s_ppuStreamIndex;
}

size_t assetExtractorGetTotalTiles(void)
{
    return s_totalTiles;
}

const char *assetExtractorGetRomIdentitySummary(void)
{
    return s_identitySummary;
}

int assetExtractorRomMatchesExcitebikeHeader(void)
{
    if (s_identity.headerValid == 0) return -1;
    return nesRomIdentityMatchesExcitebike(&s_identity) ? 1 : 0;
}
