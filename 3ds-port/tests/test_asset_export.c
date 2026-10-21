#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "asset_export.h"
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

int main(void)
{
    const char *dir = "test_asset_export_output";
    const unsigned char pngMagic[8] = {137u,80u,78u,71u,13u,10u,26u,10u};
    unsigned char raw[16] = {0u};
    unsigned char pixels[960u * 64u];
    char status[160];
    FILE *file;
    size_t i;
    (void)mkdir(dir, 0777);
    memset(pixels, 1, sizeof(pixels));
    for (i = 0u; i < sizeof(raw); ++i) raw[i] = (unsigned char)i;
    if (!assetExportWrite(dir, raw, sizeof(raw), pixels, 960u, status, sizeof(status))) {
        fprintf(stderr, "Export failed: %s\n", status);
        return 1;
    }
    if (!fileStartsWith("test_asset_export_output/chr_atlas.png", pngMagic, sizeof(pngMagic))) return 2;
    if (!fileStartsWith("test_asset_export_output/ppu_preview_01.png", pngMagic, sizeof(pngMagic))) return 14;
    file = fopen("test_asset_export_output/chr_raw.bin", "rb");
    if (file == NULL) return 3;
    {
        unsigned char actual[sizeof(raw)];
        if (fread(actual, 1u, sizeof(actual), file) != sizeof(actual) ||
            memcmp(actual, raw, sizeof(raw)) != 0) { fclose(file); return 4; }
    }
    fclose(file);
    file = fopen("test_asset_export_output/chr_pixels.bin", "rb");
    if (file == NULL) return 5;
    {
        unsigned char actual[sizeof(pixels)];
        if (fread(actual, 1u, sizeof(actual), file) != sizeof(actual) ||
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
            strstr(manifest, "Color previews: ppu_preview_01..18.png") == NULL) { fclose(file); return 8; }
    }
    fclose(file);
    (void)unlink("test_asset_export_output/chr_atlas.png");
    (void)unlink("test_asset_export_output/chr_raw.bin");
    (void)unlink("test_asset_export_output/chr_pixels.bin");
    (void)unlink("test_asset_export_output/chr_pixels.ebp");
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_preview_%02lu.png", (unsigned long)i);
        (void)unlink(path);
    }
    (void)unlink("test_asset_export_output/manifest.txt");
    (void)unlink("test_asset_export_output/ppu_palette_12.bin");
    (void)unlink("test_asset_export_output/ppu_palette_12_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_12_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_13.bin");
    (void)unlink("test_asset_export_output/ppu_palette_13_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_13_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_14.bin");
    (void)unlink("test_asset_export_output/ppu_palette_14_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_14_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_15.bin");
    (void)unlink("test_asset_export_output/ppu_palette_15_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_15_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_16.bin");
    (void)unlink("test_asset_export_output/ppu_palette_16_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_16_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_17.bin");
    (void)unlink("test_asset_export_output/ppu_palette_17_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_17_stream.bin");
    (void)unlink("test_asset_export_output/ppu_palette_18.bin");
    (void)unlink("test_asset_export_output/ppu_palette_18_mask.bin");
    (void)unlink("test_asset_export_output/ppu_palette_18_stream.bin");
    for (i = 1u; i <= NES_EXCITEBIKE_PPU_STREAM_COUNT; ++i) {
        char path[128];
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_stream_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
        (void)snprintf(path, sizeof(path), "test_asset_export_output/ppu_snapshot_%02lu.bin", (unsigned long)i);
        (void)unlink(path);
    }
    (void)rmdir(dir);
    puts("PASS: native CHR raw/pixel/PNG/manifest export and honest palette metadata.");
    return 0;
}
