#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "asset_export.h"
#include "nes_chr_decode.h"
#include "nes_ppu_commands.h"

static int fileStartsWith(const char *path, const unsigned char *magic, size_t count)
{
    unsigned char data[8];
    FILE *file = fopen(path, "rb");
    int ok;
    if (file == NULL) return 0;
    ok = fread(data, 1u, count, file) == count && memcmp(data, magic, count) == 0;
    fclose(file);
    return ok;
}

static int pngHasDimensions(const char *path, unsigned int width, unsigned int height)
{
    unsigned char header[24];
    FILE *file = fopen(path, "rb");
    int ok;
    if (file == NULL) return 0;
    ok = fread(header, 1u, sizeof(header), file) == sizeof(header) &&
         memcmp(header, "\x89PNG\r\n\x1a\n", 8u) == 0 &&
         memcmp(header + 12u, "IHDR", 4u) == 0 &&
         (((unsigned int)header[16] << 24u) | ((unsigned int)header[17] << 16u) |
          ((unsigned int)header[18] << 8u) | header[19]) == width &&
         (((unsigned int)header[20] << 24u) | ((unsigned int)header[21] << 16u) |
          ((unsigned int)header[22] << 8u) | header[23]) == height;
    fclose(file);
    return ok;
}

/* Decode one 16-byte CHR tile into 64 one-byte pixel indices. */
static void decodeTile(const unsigned char *chr, unsigned char out[64])
{
    unsigned int row, col;
    for (row = 0u; row < 8u; ++row) {
        const unsigned char low = chr[row];
        const unsigned char high = chr[row + 8u];
        for (col = 0u; col < 8u; ++col) {
            const unsigned int bit = 7u - col;
            out[row * 8u + col] = (unsigned char)(((low >> bit) & 1u) |
                                                  (((high >> bit) & 1u) << 1u));
        }
    }
}

int main(void)
{
    const char *dir = "test_asset_export_output";
    const char *bigDir = "test_asset_export_output_big";
    const unsigned char pngMagic[8] = {137u,80u,78u,71u,13u,10u,26u,10u};
    static unsigned char raw[960u * 16u];
    static unsigned char pixels[960u * 64u];
    static unsigned char bigRaw[1000u * 16u];
    static unsigned char actual[1000u * 64u];
    char status[160];
    FILE *file;
    size_t i, decoded = 0u;
    (void)mkdir(dir, 0777);
    (void)mkdir(bigDir, 0777);
    for (i = 0u; i < sizeof(raw); ++i) raw[i] = (unsigned char)(i * 7u + 3u);
    for (i = 0u; i < sizeof(bigRaw); ++i) bigRaw[i] = (unsigned char)(i * 11u + 5u);
    if (nesChrDecodeRaw(raw, sizeof(raw), pixels, 960u, &decoded) != NES_CHR_DECODE_OK ||
        decoded != 960u) return 21;
    if (!assetExportWrite(dir, raw, sizeof(raw), pixels, 960u, NULL, status, sizeof(status))) {
        fprintf(stderr, "Export failed: %s\n", status);
        return 1;
    }
    if (!fileStartsWith("test_asset_export_output/chr_atlas.png", pngMagic, sizeof(pngMagic))) return 2;
    if (!pngHasDimensions("test_asset_export_output/chr_atlas.png", 256u, 240u)) return 18;
    if (!pngHasDimensions("test_asset_export_output/chr_atlas_palette_12_subpalette_0.png", 256u, 240u)) return 20;
    if (!fileStartsWith("test_asset_export_output/ppu_preview_01.png", pngMagic, sizeof(pngMagic))) return 14;
    if (!pngHasDimensions("test_asset_export_output/ppu_preview_01.png", 256u, 240u)) return 19;
    file = fopen("test_asset_export_output/chr_raw.bin", "rb");
    if (file == NULL) return 3;
    {
        static unsigned char actualRaw[sizeof(raw)];
        if (fread(actualRaw, 1u, sizeof(actualRaw), file) != sizeof(actualRaw) ||
            memcmp(actualRaw, raw, sizeof(raw)) != 0) { fclose(file); return 4; }
    }
    fclose(file);
    file = fopen("test_asset_export_output/chr_pixels.bin", "rb");
    if (file == NULL) return 5;
    {
        if (fread(actual, 1u, sizeof(pixels), file) != sizeof(pixels) ||
            memcmp(actual, pixels, sizeof(pixels)) != 0) { fclose(file); return 6; }
    }
    fclose(file);
    if (!fileStartsWith("test_asset_export_output/chr_pixels.ebp", (const unsigned char *)"EBPX", 4u)) return 15;
    file = fopen("test_asset_export_output/chr_pixels.ebp", "rb");
    if (file == NULL) return 16;
    {
        unsigned char header[16], firstPixels[64];
        if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
            memcmp(header, "EBPX", 4u) != 0 || header[4] != 1u || header[5] != 0u ||
            header[6] != 16u || header[7] != 0u || header[8] != 0xC0u ||
            header[9] != 0x03u || header[10] != 0u || header[11] != 0u ||
            header[12] != 8u || header[13] != 8u || header[14] != 1u ||
            fread(firstPixels, 1u, sizeof(firstPixels), file) != sizeof(firstPixels) ||
            memcmp(firstPixels, pixels, sizeof(firstPixels)) != 0) {
            fclose(file); return 17;
        }
    }
    fclose(file);
    if (!fileStartsWith("test_asset_export_output/ppu_palette_12.bin", (const unsigned char *)"\x29\x27\x22\x30", 4u)) return 9;
    if (!fileStartsWith("test_asset_export_output/ppu_palette_12_stream.bin", (const unsigned char *)"\x3F\x00\x08", 3u)) return 10;
    {
        const unsigned char streamMagic[3] = {0x23u, 0xD0u, 0x60u};
        if (!fileStartsWith("test_asset_export_output/ppu_stream_01.bin", streamMagic, sizeof(streamMagic))) return 11;
    }
    file = fopen("test_asset_export_output/ppu_snapshot_01.bin", "rb");
    if (file == NULL) return 12;
    {
        unsigned char value = 0u, mask = 0u;
        if (fseek(file, 0x3D0L, SEEK_SET) != 0 || fread(&value, 1u, 1u, file) != 1u ||
            fseek(file, (long)(NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES + 0x3D0u), SEEK_SET) != 0 ||
            fread(&mask, 1u, 1u, file) != 1u || value != 0x55u || mask != 1u) {
            fclose(file); return 13;
        }
    }
    fclose(file);
    file = fopen("test_asset_export_output/manifest.txt", "rb");
    if (file == NULL) return 7;
    {
        char manifest[2048];
        const size_t n = fread(manifest, 1u, sizeof(manifest)-1u, file);
        manifest[n] = '\0';
        if (strstr(manifest, "Palette presets:") == NULL ||
            strstr(manifest, "PPU command streams:") == NULL ||
            strstr(manifest, "chr_atlas.png") == NULL ||
            strstr(manifest, "chr_pixels.ebp") == NULL ||
            strstr(manifest, "Color variants: chr_atlas_palette_12..18_subpalette_0..3.png") == NULL ||
            strstr(manifest, "Color previews: ppu_preview_01..18.png") == NULL ||
            strstr(manifest, "ALL 960 tiles") == NULL) { fclose(file); return 8; }
    }
    fclose(file);

    /* Sources beyond 960 tiles: exports must contain ALL tiles. */
    if (nesChrDecodeRaw(bigRaw, sizeof(bigRaw), pixels, 960u, &decoded) != NES_CHR_DECODE_OK ||
        decoded != 960u) return 22;
    if (!assetExportWrite(bigDir, bigRaw, sizeof(bigRaw), pixels, 960u, NULL, status, sizeof(status))) {
        fprintf(stderr, "Big export failed: %s\n", status);
        return 23;
    }
    file = fopen("test_asset_export_output_big/chr_pixels.bin", "rb");
    if (file == NULL) return 24;
    {
        unsigned char tile999[64], expected[64];
        if (fseek(file, (long)(999u * 64u), SEEK_SET) != 0 ||
            fread(tile999, 1u, sizeof(tile999), file) != sizeof(tile999)) { fclose(file); return 25; }
        decodeTile(bigRaw + 999u * 16u, expected);
        if (memcmp(tile999, expected, sizeof(tile999)) != 0) { fclose(file); return 26; }
        if (fseek(file, 0L, SEEK_END) != 0 || ftell(file) != (long)(1000u * 64u)) { fclose(file); return 27; }
    }
    fclose(file);
    file = fopen("test_asset_export_output_big/chr_pixels.ebp", "rb");
    if (file == NULL) return 28;
    {
        unsigned char header[16];
        if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
            memcmp(header, "EBPX", 4u) != 0 || header[8] != 0xE8u ||
            header[9] != 0x03u || header[10] != 0u || header[11] != 0u) {
            fclose(file); return 29;
        }
    }
    fclose(file);
    file = fopen("test_asset_export_output_big/manifest.txt", "rb");
    if (file == NULL) return 30;
    {
        char manifest[2048];
        const size_t n = fread(manifest, 1u, sizeof(manifest)-1u, file);
        manifest[n] = '\0';
        if (strstr(manifest, "ALL 1000 tiles") == NULL) { fclose(file); return 31; }
    }
    fclose(file);

    (void)unlink("test_asset_export_output/chr_atlas.png");
    for (i = 12u; i <= 18u; ++i) { unsigned int sub; for (sub = 0u; sub < 4u; ++sub) { char path[160]; (void)snprintf(path, sizeof(path), "test_asset_export_output/chr_atlas_palette_%lu_subpalette_%u.png", (unsigned long)i, sub); (void)unlink(path); } }
    (void)unlink("test_asset_export_output/chr_raw.bin");
    (void)unlink("test_asset_export_output/chr_pixels.bin");
    (void)unlink("test_asset_export_output/chr_pixels.ebp");
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_preview_%02lu.png", (unsigned long)i);
        (void)unlink(path);
    }
    (void)unlink("test_asset_export_output/manifest.txt");
    for (i = 12u; i <= 18u; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_palette_%lu.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_palette_%lu_mask.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_palette_%lu_stream.bin", (unsigned long)i);
        (void)unlink(path);
    }
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_stream_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_snapshot_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
    }
    (void)rmdir(dir);

    (void)unlink("test_asset_export_output_big/chr_atlas.png");
    for (i = 12u; i <= 18u; ++i) { unsigned int sub; for (sub = 0u; sub < 4u; ++sub) { char path[160]; (void)snprintf(path, sizeof(path), "test_asset_export_output_big/chr_atlas_palette_%lu_subpalette_%u.png", (unsigned long)i, sub); (void)unlink(path); } }
    (void)unlink("test_asset_export_output_big/chr_raw.bin");
    (void)unlink("test_asset_export_output_big/chr_pixels.bin");
    (void)unlink("test_asset_export_output_big/chr_pixels.ebp");
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_preview_%02lu.png", (unsigned long)i);
        (void)unlink(path);
    }
    (void)unlink("test_asset_export_output_big/manifest.txt");
    for (i = 12u; i <= 18u; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_palette_%lu.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_palette_%lu_mask.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_palette_%lu_stream.bin", (unsigned long)i);
        (void)unlink(path);
    }
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_stream_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output_big/ppu_snapshot_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
    }
    (void)rmdir(bigDir);
    puts("PASS: native CHR raw/pixel/PNG/manifest export streams all tiles beyond the 960 cap.");
    return 0;
}
