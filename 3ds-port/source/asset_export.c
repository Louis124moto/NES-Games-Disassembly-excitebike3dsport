#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "asset_export.h"
#include "nes_ppu_commands.h"

#define STBIW_ASSERT(x) ((void)0)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "third_party/stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define ATLAS_WIDTH 256
#define ATLAS_HEIGHT 240
#define ATLAS_RGB_BYTES ((size_t)ATLAS_WIDTH * (size_t)ATLAS_HEIGHT * 3u)

static uint8_t s_atlas[ATLAS_RGB_BYTES];
static const uint8_t s_gray[4] = {0u, 85u, 170u, 255u};

static int makePath(char *out, size_t capacity, const char *directory, const char *name)
{
    int n;
    if (out == NULL || directory == NULL || name == NULL) return 0;
    n = snprintf(out, capacity, "%s/%s", directory, name);
    return n > 0 && (size_t)n < capacity;
}

static int writeBytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static void buildAtlas(const uint8_t *pixels, size_t tileCount)
{
    size_t tile, y, x;
    memset(s_atlas, 0, sizeof(s_atlas));
    for (tile = 0u; tile < tileCount; ++tile) {
        const size_t tileX = (tile % 32u) * 8u;
        const size_t tileY = (tile / 32u) * 8u;
        for (y = 0u; y < 8u; ++y) {
            for (x = 0u; x < 8u; ++x) {
                const uint8_t index = pixels[tile * 64u + y * 8u + x];
                const size_t out = ((tileY + y) * ATLAS_WIDTH + tileX + x) * 3u;
                const uint8_t value = s_gray[index & 3u];
                s_atlas[out] = value;
                s_atlas[out + 1u] = value;
                s_atlas[out + 2u] = value;
            }
        }
    }
}

int assetExportWrite(const char *directory, const uint8_t *rawChr, size_t rawChrBytes,
                     const uint8_t *pixels, size_t tileCount,
                     char *status, size_t statusCapacity)
{
    char rawPath[512], pixelPath[512], pngPath[512], manifestPath[512];
    char manifest[1024];
    struct stat st;
    FILE *file;
    int manifestLength;
    size_t paletteIndex;
    if (status != NULL && statusCapacity > 0u) status[0] = '\0';
    if (directory == NULL || rawChr == NULL || rawChrBytes == 0u ||
        pixels == NULL || tileCount == 0u || tileCount > 960u) {
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Export rejected: invalid or unsupported CHR data.");
        return 0;
    }
    if (stat(directory, &st) != 0) {
        if (mkdir(directory, 0777) != 0 && errno != EEXIST) {
            if (status != NULL && statusCapacity > 0u)
                (void)snprintf(status, statusCapacity, "Cannot create export directory.");
            return 0;
        }
    }
    if (!makePath(rawPath, sizeof(rawPath), directory, "chr_raw.bin") ||
        !makePath(pixelPath, sizeof(pixelPath), directory, "chr_pixels.bin") ||
        !makePath(pngPath, sizeof(pngPath), directory, "chr_atlas.png") ||
        !makePath(manifestPath, sizeof(manifestPath), directory, "manifest.txt")) {
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Export path is too long.");
        return 0;
    }
    buildAtlas(pixels, tileCount);
    if (!writeBytes(rawPath, rawChr, rawChrBytes) ||
        !writeBytes(pixelPath, pixels, tileCount * 64u) ||
        !stbi_write_png(pngPath, ATLAS_WIDTH, ATLAS_HEIGHT, 3, s_atlas, ATLAS_WIDTH * 3)) {
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Export failed writing CHR/PNG files.");
        return 0;
    }
    for (paletteIndex = 0u; paletteIndex < NES_EXCITEBIKE_PALETTE_STREAM_COUNT; ++paletteIndex) {
        char palettePath[512], maskPath[512], streamPath[512];
        uint8_t palette[32], mask[32];
        const uint8_t *stream;
        const char *sourceLabel = NULL;
        size_t streamSize = 0u, writes = 0u;
        char fileName[48];
        (void)snprintf(fileName, sizeof(fileName), "ppu_palette_%lu.bin", (unsigned long)(paletteIndex + 12u));
        if (!makePath(palettePath, sizeof(palettePath), directory, fileName)) return 0;
        (void)snprintf(fileName, sizeof(fileName), "ppu_palette_%lu_mask.bin", (unsigned long)(paletteIndex + 12u));
        if (!makePath(maskPath, sizeof(maskPath), directory, fileName)) return 0;
        (void)snprintf(fileName, sizeof(fileName), "ppu_palette_%lu_stream.bin", (unsigned long)(paletteIndex + 12u));
        if (!makePath(streamPath, sizeof(streamPath), directory, fileName)) return 0;
        stream = nesExcitebikePaletteStream(paletteIndex, &streamSize, &sourceLabel);
        if (stream == NULL || nesExcitebikePaletteSnapshot(paletteIndex, palette, mask, &writes) != NES_PPU_COMMAND_OK ||
            !writeBytes(palettePath, palette, sizeof(palette)) ||
            !writeBytes(maskPath, mask, sizeof(mask)) || !writeBytes(streamPath, stream, streamSize)) {
            if (status != NULL && statusCapacity > 0u)
                (void)snprintf(status, statusCapacity, "Could not export source-derived palette command stream.");
            return 0;
        }
        (void)sourceLabel;
        (void)writes;
    }
    manifestLength = snprintf(manifest, sizeof(manifest),
        "Excitebike 3DS native CHR export\n"
        "Raw source: chr_raw.bin (%lu bytes of CHR-ROM data)\n"
        "Decoded pixels: chr_pixels.bin (%lu tiles, 64 2bpp indices per tile; runtime cap 960 tiles)\n"
        "Preview: chr_atlas.png (256x240 RGB; 32x30 tiles; grayscale index visualization)\n"
        "If source CHR contains more than the runtime cap, chr_raw.bin retains all bytes but decoded outputs include only the first 960 tiles.\n"
        "Palette presets: ppu_palette_12..18.bin replay source PPU write lists; mask and stream binaries are included\n"
        "Presets are not a claim about which palette is active in a live menu state\n"
        "Nametable/attributes/OAM: NOT CAPTURED by this export\n"
        "Unknown/compressed PRG graphics: NOT CLAIMED EXTRACTED\n",
        (unsigned long)rawChrBytes, (unsigned long)tileCount);
    if (manifestLength < 0 || (size_t)manifestLength >= sizeof(manifest)) {
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Manifest generation failed.");
        return 0;
    }
    file = fopen(manifestPath, "wb");
    if (file == NULL || fwrite(manifest, 1u, (size_t)manifestLength, file) != (size_t)manifestLength) {
        if (file != NULL) fclose(file);
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Could not write export manifest.");
        return 0;
    }
    if (fclose(file) != 0) {
        if (status != NULL && statusCapacity > 0u)
            (void)snprintf(status, statusCapacity, "Could not close export manifest.");
        return 0;
    }
    if (status != NULL && statusCapacity > 0u)
        (void)snprintf(status, statusCapacity, "SD exports saved: CHR binary, decoded pixels, PNG atlas, manifest.");
    return 1;
}
