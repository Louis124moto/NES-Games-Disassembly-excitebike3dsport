#ifndef EXCITEBIKE_NES_CHR_CACHE_H
#define EXCITEBIKE_NES_CHR_CACHE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Versioned decoded-pixel cache (format v3), extracted from the runtime
 * extractor so header/fingerprint/payload validation is host-testable.
 *
 * File layout: magic "EBCHR03" + NUL, u16 tile count, u32 ROM fingerprint
 * (FNV-1a over the complete source file), then tileCount * 64 decoded
 * one-byte 2bpp pixel indices. A cache written from a different ROM is
 * rejected. Only decoded pixels are cached; raw CHR and identity come from
 * the source file itself.
 */
#define NES_CHR_CACHE_MAX_TILES 960u
#define NES_CHR_CACHE_HEADER_SIZE 14u

typedef enum {
    NES_CHR_CACHE_OK = 0,
    NES_CHR_CACHE_INVALID_ARGUMENT,
    NES_CHR_CACHE_NO_FILE,
    NES_CHR_CACHE_BAD_MAGIC,
    NES_CHR_CACHE_STALE_FINGERPRINT,
    NES_CHR_CACHE_BAD_TILE_COUNT,
    NES_CHR_CACHE_TRUNCATED_PAYLOAD,
    NES_CHR_CACHE_TRAILING_DATA,
    NES_CHR_CACHE_INVALID_PIXEL,
    NES_CHR_CACHE_WRITE_FAILED
} NesChrCacheResult;

NesChrCacheResult nesChrCacheSave(const char *path, const uint8_t *pixels,
                                  size_t tileCount, uint32_t romFingerprint);

NesChrCacheResult nesChrCacheLoad(const char *path, uint32_t expectedRomFingerprint,
                                  uint8_t *pixels, size_t maxTiles, size_t *tileCount);

const char *nesChrCacheError(NesChrCacheResult result);

#endif
