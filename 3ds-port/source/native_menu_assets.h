#ifndef EXCITEBIKE_NATIVE_MENU_ASSETS_H
#define EXCITEBIKE_NATIVE_MENU_ASSETS_H

#include <stddef.h>
#include <stdint.h>

#define NATIVE_MENU_SCENE_COUNT 6u
#define NATIVE_MENU_PALETTE_COUNT 7u
#define NATIVE_MENU_SCENE_BYTES 4096u
#define NATIVE_MENU_CHR_TILE_LIMIT 960u

typedef struct NativeMenuSceneAssets {
    uint8_t nametable[NATIVE_MENU_SCENE_BYTES];
    uint8_t written[NATIVE_MENU_SCENE_BYTES];
} NativeMenuSceneAssets;

typedef struct NativeMenuAssets {
    uint8_t *chr_pixels; /* EBPX decoded 8x8 pixel indices, 64 bytes per tile */
    size_t chr_tile_count;
    NativeMenuSceneAssets scenes[NATIVE_MENU_SCENE_COUNT];
    uint8_t palettes[NATIVE_MENU_PALETTE_COUNT][32];
    uint8_t palette_written[NATIVE_MENU_PALETTE_COUNT][32];
    uint8_t loaded;
    char error[128];
} NativeMenuAssets;

/* Loads only already-extracted assets beside the .3dsx; it never opens rom.nes. */
int nativeMenuAssetsLoad(NativeMenuAssets *assets, const char *executablePath);
void nativeMenuAssetsUnload(NativeMenuAssets *assets);

#endif
