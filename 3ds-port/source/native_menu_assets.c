#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native_menu_assets.h"

#define EBPX_HEADER_BYTES 16u
#define SNAPSHOT_BYTES (NATIVE_MENU_SCENE_BYTES * 2u + 64u)

static int makeAssetPath(char *out, size_t capacity, const char *executablePath,
                         const char *assetName)
{
    size_t directoryLength = 0u;
    size_t i;
    if (out == NULL || capacity == 0u || assetName == NULL) return 0;
    if (executablePath != NULL) {
        for (i = 0u; executablePath[i] != '\0'; ++i) {
            if (executablePath[i] == '/' || executablePath[i] == '\\') {
                directoryLength = i + 1u;
            }
        }
    }
    if (directoryLength + sizeof("extracted/") + strlen(assetName) > capacity) return 0;
    if (directoryLength != 0u) memcpy(out, executablePath, directoryLength);
    out[directoryLength] = '\0';
    (void)strcat(out, "extracted/");
    (void)strcat(out, assetName);
    return 1;
}

static int readExactFile(const char *path, uint8_t *destination, size_t expectedBytes)
{
    FILE *file;
    int trailing;
    size_t count;
    if (path == NULL || destination == NULL) return 0;
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    count = fread(destination, 1u, expectedBytes, file);
    trailing = fgetc(file);
    if (fclose(file) != 0) return 0;
    return count == expectedBytes && trailing == EOF;
}

static int loadChrPack(NativeMenuAssets *assets, const char *executablePath)
{
    char path[512];
    FILE *file;
    uint8_t header[EBPX_HEADER_BYTES];
    uint32_t tileCount;
    size_t pixelBytes;
    if (!makeAssetPath(path, sizeof(path), executablePath, "chr_pixels.ebp")) return 0;
    file = fopen(path, "rb");
    if (file == NULL) {
        (void)snprintf(assets->error, sizeof(assets->error), "Missing extracted/chr_pixels.ebp");
        return 0;
    }
    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, "EBPX", 4u) != 0 || header[4] != 1u || header[5] != 0u ||
        header[6] != EBPX_HEADER_BYTES || header[7] != 0u ||
        header[12] != 8u || header[13] != 8u || header[14] != 1u || header[15] != 0u) {
        fclose(file);
        (void)snprintf(assets->error, sizeof(assets->error), "Invalid EBPX version or format");
        return 0;
    }
    tileCount = (uint32_t)header[8] | ((uint32_t)header[9] << 8u) |
                ((uint32_t)header[10] << 16u) | ((uint32_t)header[11] << 24u);
    if (tileCount == 0u || tileCount > NATIVE_MENU_CHR_TILE_LIMIT) {
        fclose(file);
        (void)snprintf(assets->error, sizeof(assets->error), "Unsupported extracted CHR tile count");
        return 0;
    }
    pixelBytes = (size_t)tileCount * 64u;
    assets->chr_pixels = (uint8_t *)malloc(pixelBytes);
    if (assets->chr_pixels == NULL) {
        fclose(file);
        (void)snprintf(assets->error, sizeof(assets->error), "Not enough memory for CHR assets");
        return 0;
    }
    if (fread(assets->chr_pixels, 1u, pixelBytes, file) != pixelBytes || fgetc(file) != EOF) {
        fclose(file);
        free(assets->chr_pixels);
        assets->chr_pixels = NULL;
        (void)snprintf(assets->error, sizeof(assets->error), "Truncated or oversized CHR pack");
        return 0;
    }
    if (fclose(file) != 0) {
        free(assets->chr_pixels);
        assets->chr_pixels = NULL;
        (void)snprintf(assets->error, sizeof(assets->error), "Could not close CHR pack");
        return 0;
    }
    assets->chr_tile_count = (size_t)tileCount;
    return 1;
}

static int mergeSnapshot(NativeMenuSceneAssets *scene, const char *filename,
                         const char *executablePath, char *error, size_t errorCapacity)
{
    char path[512];
    uint8_t *snapshot;
    size_t i;
    if (!makeAssetPath(path, sizeof(path), executablePath, filename)) return 0;
    snapshot = (uint8_t *)malloc(SNAPSHOT_BYTES);
    if (snapshot == NULL) return 0;
    if (!readExactFile(path, snapshot, SNAPSHOT_BYTES)) {
        (void)snprintf(error, errorCapacity, "Missing or invalid extracted/%s", filename);
        free(snapshot);
        return 0;
    }
    /* Merge only source-authored writes. Unwritten cells remain the source clear value 0. */
    for (i = 0u; i < NATIVE_MENU_SCENE_BYTES; ++i) {
        if (snapshot[NATIVE_MENU_SCENE_BYTES + i] != 0u) {
            scene->nametable[i] = snapshot[i];
            scene->written[i] = 1u;
        }
    }
    free(snapshot);
    return 1;
}

int nativeMenuAssetsLoad(NativeMenuAssets *assets, const char *executablePath)
{
    char path[512];
    if (assets == NULL) return 0;
    nativeMenuAssetsUnload(assets);
    memset(assets, 0, sizeof(*assets));

    if (!loadChrPack(assets, executablePath)) goto fail;

    /* These are source PPU command snapshots, not screenshots. Preserve
     * source setup order and leave unsupported/dynamic content unpainted. */
    if (!mergeSnapshot(&assets->scenes[0], "ppu_snapshot_02.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[1], "ppu_snapshot_06.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[1], "ppu_snapshot_03.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[2], "ppu_snapshot_06.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[2], "ppu_snapshot_15.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[3], "ppu_snapshot_05.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[4], "ppu_snapshot_16.bin", executablePath,
                       assets->error, sizeof(assets->error)) ||
        !mergeSnapshot(&assets->scenes[5], "ppu_snapshot_17.bin", executablePath,
                       assets->error, sizeof(assets->error))) goto fail;

    /* First-course race background: merge the source setup-list-2 snapshots
     * in order. These are static source writes; the runtime draws them natively. */
    if (mergeSnapshot(&assets->scenes[6], "ppu_snapshot_18.bin", executablePath,
                      assets->error, sizeof(assets->error)) &&
        mergeSnapshot(&assets->scenes[6], "ppu_snapshot_01.bin", executablePath,
                      assets->error, sizeof(assets->error)) &&
        mergeSnapshot(&assets->scenes[6], "ppu_snapshot_17.bin", executablePath,
                      assets->error, sizeof(assets->error)) &&
        mergeSnapshot(&assets->scenes[6], "ppu_snapshot_14.bin", executablePath,
                      assets->error, sizeof(assets->error))) {
        assets->race_scene_available = 1u;
    } else {
        memset(&assets->scenes[6], 0, sizeof(assets->scenes[6]));
        assets->race_scene_available = 0u;
        assets->error[0] = '\\0';
    }

    /* The editor setup selects palette 12 + course. Other menu screens use
     * source palette preset 18, as in the existing front-end renderer. */
    {
        unsigned paletteIndex;
        for (paletteIndex = 0u; paletteIndex < NATIVE_MENU_PALETTE_COUNT; ++paletteIndex) {
            char filename[48];
            (void)snprintf(filename, sizeof(filename), "ppu_palette_%u.bin", paletteIndex + 12u);
            if (!makeAssetPath(path, sizeof(path), executablePath, filename) ||
                !readExactFile(path, assets->palettes[paletteIndex], 32u)) {
                (void)snprintf(assets->error, sizeof(assets->error),
                               "Missing or invalid palette preset %u", paletteIndex + 12u);
                goto fail;
            }
            (void)snprintf(filename, sizeof(filename), "ppu_palette_%u_mask.bin", paletteIndex + 12u);
            if (!makeAssetPath(path, sizeof(path), executablePath, filename) ||
                !readExactFile(path, assets->palette_written[paletteIndex], 32u)) {
                (void)snprintf(assets->error, sizeof(assets->error),
                               "Missing or invalid palette mask %u", paletteIndex + 12u);
                goto fail;
            }
        }
    }

    assets->loaded = 1u;
    assets->error[0] = '\0';
    return 1;

fail:
    free(assets->chr_pixels);
    assets->chr_pixels = NULL;
    assets->chr_tile_count = 0u;
    assets->loaded = 0u;
    return 0;
}

void nativeMenuAssetsUnload(NativeMenuAssets *assets)
{
    if (assets == NULL) return;
    free(assets->chr_pixels);
    assets->chr_pixels = NULL;
    assets->chr_tile_count = 0u;
    assets->loaded = 0u;
}
