#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nes_chr_cache.h"

int main(void)
{
    const char *path = "test_chr_cache.bin";
    static unsigned char pixels[512u * 64u];
    static unsigned char loaded[512u * 64u];
    static unsigned char fileBytes[14u + 512u * 64u];
    size_t i, count = 0u;

    for (i = 0u; i < sizeof(pixels); ++i) pixels[i] = (unsigned char)(i % 4u);

    /* Round-trip save/load. */
    if (nesChrCacheSave(path, pixels, 512u, 0x12345678u) != NES_CHR_CACHE_OK) return 1;
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 512u, &count) != NES_CHR_CACHE_OK ||
        count != 512u) return 2;
    if (memcmp(pixels, loaded, sizeof(pixels)) != 0) return 3;

    /* Stale fingerprint from a different ROM is rejected. */
    if (nesChrCacheLoad(path, 0xDEADBEEFu, loaded, 512u, &count) != NES_CHR_CACHE_STALE_FINGERPRINT) return 4;

    /* Buffer too small for the cached count. */
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 100u, &count) != NES_CHR_CACHE_BAD_TILE_COUNT) return 5;

    /* Bad magic. */
    {
        FILE *file = fopen(path, "r+b");
        int ok;
        if (file == NULL) return 6;
        ok = fseek(file, 0L, SEEK_SET) == 0 && fputc('X', file) != EOF;
        fclose(file);
        if (!ok) return 7;
    }
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 512u, &count) != NES_CHR_CACHE_BAD_MAGIC) return 8;

    /* Truncated payload, then trailing data. */
    {
        FILE *file = fopen(path, "rb");
        size_t got;
        if (file == NULL) return 9;
        got = fread(fileBytes, 1u, sizeof(fileBytes), file);
        fclose(file);
        if (got != 14u + 512u * 64u) return 10;
        file = fopen(path, "wb");
        if (file == NULL) return 11;
        (void)fwrite(fileBytes, 1u, 14u + 100u * 64u, file);
        fclose(file);
    }
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 512u, &count) != NES_CHR_CACHE_TRUNCATED_PAYLOAD) return 12;
    {
        FILE *file = fopen(path, "ab");
        if (file == NULL) return 13;
        (void)fputc(0, file);
        fclose(file);
    }
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 512u, &count) != NES_CHR_CACHE_TRAILING_DATA) return 14;

    /* Invalid pixel index (>3) is rejected. */
    if (nesChrCacheSave(path, pixels, 512u, 0x12345678u) != NES_CHR_CACHE_OK) return 15;
    {
        FILE *file = fopen(path, "r+b");
        if (file == NULL) return 16;
        if (fseek(file, (long)(14u + 10u), SEEK_SET) != 0 || fputc(4, file) == EOF) {
            fclose(file);
            return 17;
        }
        fclose(file);
    }
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 512u, &count) != NES_CHR_CACHE_INVALID_PIXEL) return 18;

    /* Counts above the 960-tile cap are rejected at save and load time. */
    if (nesChrCacheSave(path, pixels, 1000u, 0x12345678u) != NES_CHR_CACHE_INVALID_ARGUMENT) return 19;
    {
        FILE *file = fopen(path, "wb");
        if (file == NULL) return 20;
        (void)fwrite("EBCHR03", 1u, 8u, file);
        (void)fputc(0xE8, file); /* 1000 & 0xFF */
        (void)fputc(0x03, file); /* 1000 >> 8 */
        (void)fputc(0x78, file); (void)fputc(0x56, file);
        (void)fputc(0x34, file); (void)fputc(0x12, file);
        fclose(file);
    }
    if (nesChrCacheLoad(path, 0x12345678u, loaded, 960u, &count) != NES_CHR_CACHE_BAD_TILE_COUNT) return 21;

    /* NULL arguments. */
    if (nesChrCacheSave(NULL, pixels, 512u, 1u) != NES_CHR_CACHE_INVALID_ARGUMENT) return 22;
    if (nesChrCacheSave(path, NULL, 512u, 1u) != NES_CHR_CACHE_INVALID_ARGUMENT) return 23;
    if (nesChrCacheLoad(path, 1u, NULL, 512u, &count) != NES_CHR_CACHE_INVALID_ARGUMENT) return 24;
    if (nesChrCacheLoad(path, 1u, loaded, 512u, NULL) != NES_CHR_CACHE_INVALID_ARGUMENT) return 25;

    (void)unlink(path);
    return 0;
}
