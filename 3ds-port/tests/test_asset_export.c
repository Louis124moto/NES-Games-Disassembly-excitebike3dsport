#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "asset_export.h"

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
    unsigned char pixels[64];
    char status[160];
    FILE *file;
    size_t i;
    (void)mkdir(dir, 0777);
    memset(pixels, 1, sizeof(pixels));
    for (i = 0u; i < sizeof(raw); ++i) raw[i] = (unsigned char)i;
    if (!assetExportWrite(dir, raw, sizeof(raw), pixels, 1u, status, sizeof(status))) {
        fprintf(stderr, "Export failed: %s\n", status);
        return 1;
    }
    if (!fileStartsWith("test_asset_export_output/chr_atlas.png", pngMagic, sizeof(pngMagic))) return 2;
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
    file = fopen("test_asset_export_output/manifest.txt", "rb");
    if (file == NULL) return 7;
    {
        char manifest[768];
        const size_t n = fread(manifest, 1u, sizeof(manifest)-1u, file);
        manifest[n] = '\0';
        if (strstr(manifest, "Palette: NOT APPLIED") == NULL ||
            strstr(manifest, "chr_atlas.png") == NULL) { fclose(file); return 8; }
    }
    fclose(file);
    (void)unlink("test_asset_export_output/chr_atlas.png");
    (void)unlink("test_asset_export_output/chr_raw.bin");
    (void)unlink("test_asset_export_output/chr_pixels.bin");
    (void)unlink("test_asset_export_output/manifest.txt");
    (void)rmdir(dir);
    puts("PASS: native CHR raw/pixel/PNG/manifest export and honest palette metadata.");
    return 0;
}
