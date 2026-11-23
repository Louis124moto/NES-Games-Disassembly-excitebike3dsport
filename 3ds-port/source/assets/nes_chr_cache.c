#include <stdio.h>
#include <string.h>
#include "nes_chr_cache.h"

static const uint8_t s_cacheMagic[8] = {'E', 'B', 'C', 'H', 'R', '0', '3', 0};

NesChrCacheResult nesChrCacheSave(const char *path, const uint8_t *pixels,
                                  size_t tileCount, uint32_t romFingerprint)
{
    FILE *file;
    uint8_t header[NES_CHR_CACHE_HEADER_SIZE];
    size_t pixelBytes;
    int ok;

    if (path == NULL || pixels == NULL || tileCount == 0u ||
        tileCount > NES_CHR_CACHE_MAX_TILES) {
        return NES_CHR_CACHE_INVALID_ARGUMENT;
    }
    pixelBytes = tileCount * 64u;

    memcpy(header, s_cacheMagic, sizeof(s_cacheMagic));
    header[8] = (uint8_t)(tileCount & 0xFFu);
    header[9] = (uint8_t)((tileCount >> 8) & 0xFFu);
    header[10] = (uint8_t)(romFingerprint & 0xFFu);
    header[11] = (uint8_t)((romFingerprint >> 8) & 0xFFu);
    header[12] = (uint8_t)((romFingerprint >> 16) & 0xFFu);
    header[13] = (uint8_t)((romFingerprint >> 24) & 0xFFu);

    file = fopen(path, "wb");
    if (file == NULL) return NES_CHR_CACHE_WRITE_FAILED;
    ok = fwrite(header, 1u, sizeof(header), file) == sizeof(header) &&
         fwrite(pixels, 1u, pixelBytes, file) == pixelBytes;
    if (fclose(file) != 0) ok = 0;
    return ok ? NES_CHR_CACHE_OK : NES_CHR_CACHE_WRITE_FAILED;
}

NesChrCacheResult nesChrCacheLoad(const char *path, uint32_t expectedRomFingerprint,
                                  uint8_t *pixels, size_t maxTiles, size_t *tileCount)
{
    FILE *file;
    uint8_t header[NES_CHR_CACHE_HEADER_SIZE];
    size_t expectedBytes, readBytes, cachedCount, i;

    if (tileCount != NULL) *tileCount = 0u;
    if (path == NULL || pixels == NULL || tileCount == NULL || maxTiles == 0u) {
        return NES_CHR_CACHE_INVALID_ARGUMENT;
    }
    file = fopen(path, "rb");
    if (file == NULL) return NES_CHR_CACHE_NO_FILE;

    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, s_cacheMagic, sizeof(s_cacheMagic)) != 0) {
        fclose(file);
        return NES_CHR_CACHE_BAD_MAGIC;
    }
    cachedCount = (size_t)header[8] | ((size_t)header[9] << 8);
    {
        const uint32_t cachedFingerprint = (uint32_t)header[10] |
            ((uint32_t)header[11] << 8) | ((uint32_t)header[12] << 16) |
            ((uint32_t)header[13] << 24);
        if (cachedFingerprint != expectedRomFingerprint) {
            fclose(file);
            return NES_CHR_CACHE_STALE_FINGERPRINT;
        }
    }
    if (cachedCount == 0u || cachedCount > NES_CHR_CACHE_MAX_TILES ||
        cachedCount > maxTiles) {
        fclose(file);
        return NES_CHR_CACHE_BAD_TILE_COUNT;
    }

    expectedBytes = cachedCount * 64u;
    readBytes = fread(pixels, 1u, expectedBytes, file);
    if (readBytes != expectedBytes) {
        fclose(file);
        return NES_CHR_CACHE_TRUNCATED_PAYLOAD;
    }
    if (fgetc(file) != EOF) {
        fclose(file);
        return NES_CHR_CACHE_TRAILING_DATA;
    }
    fclose(file);

    for (i = 0u; i < expectedBytes; ++i) {
        if (pixels[i] > 3u) return NES_CHR_CACHE_INVALID_PIXEL;
    }
    *tileCount = cachedCount;
    return NES_CHR_CACHE_OK;
}

const char *nesChrCacheError(NesChrCacheResult result)
{
    switch (result) {
    case NES_CHR_CACHE_OK: return "Cache loaded.";
    case NES_CHR_CACHE_INVALID_ARGUMENT: return "Invalid cache argument.";
    case NES_CHR_CACHE_NO_FILE: return "No decoded-pixel cache present.";
    case NES_CHR_CACHE_BAD_MAGIC: return "Cache magic invalid; regenerating.";
    case NES_CHR_CACHE_STALE_FINGERPRINT: return "Cache is from a different ROM; regenerating.";
    case NES_CHR_CACHE_BAD_TILE_COUNT: return "Cache tile count invalid; regenerating.";
    case NES_CHR_CACHE_TRUNCATED_PAYLOAD: return "Cache payload truncated; regenerating.";
    case NES_CHR_CACHE_TRAILING_DATA: return "Cache has trailing data; regenerating.";
    case NES_CHR_CACHE_INVALID_PIXEL: return "Cache contains invalid pixel indices; regenerating.";
    case NES_CHR_CACHE_WRITE_FAILED: return "Cache write failed.";
    default: return "Unknown cache error.";
    }
}
