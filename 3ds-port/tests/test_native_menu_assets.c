#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_menu_assets.h"

#define SNAPSHOT_BYTES (NATIVE_MENU_SCENE_BYTES * 2u + 64u)
#define BASE "/tmp/excitebike-native-assets-test"
#define DIR BASE "/extracted"

static int writeBytes(const char *path, const void *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int writeSnapshot(const char *path, uint8_t tile)
{
    uint8_t snapshot[SNAPSHOT_BYTES];
    memset(snapshot, 0, sizeof(snapshot));
    snapshot[0] = tile;
    snapshot[NATIVE_MENU_SCENE_BYTES] = 1u;
    snapshot[960u] = 0x05u;
    snapshot[NATIVE_MENU_SCENE_BYTES + 960u] = 1u;
    return writeBytes(path, snapshot, sizeof(snapshot));
}

int main(void)
{
    static const char executable[] = BASE "/excitebike3ds.3dsx";
    static const char *const snapshotNames[] = {
        "ppu_snapshot_02.bin", "ppu_snapshot_06.bin", "ppu_snapshot_03.bin",
        "ppu_snapshot_15.bin", "ppu_snapshot_05.bin", "ppu_snapshot_16.bin",
        "ppu_snapshot_17.bin"
    };
    uint8_t pack[16u + 64u], palette[32], mask[32];
    NativeMenuAssets assets = {0};
    unsigned i;
    int ok = 0;

    (void)mkdir(BASE, 0777);
    (void)mkdir(DIR, 0777);
    memset(pack, 0, sizeof(pack));
    memcpy(pack, "EBPX", 4u);
    pack[4] = 1u; pack[6] = 16u; pack[8] = 1u;
    pack[12] = 8u; pack[13] = 8u; pack[14] = 1u;
    for (i = 0u; i < 64u; ++i) pack[16u + i] = (uint8_t)(i & 3u);
    memset(mask, 1, sizeof(mask));

    if (!writeBytes(DIR "/chr_pixels.ebp", pack, sizeof(pack))) goto done;
    for (i = 0u; i < sizeof(snapshotNames) / sizeof(snapshotNames[0]); ++i) {
        char path[256];
        (void)snprintf(path, sizeof(path), DIR "/%s", snapshotNames[i]);
        if (!writeSnapshot(path, (uint8_t)(0x20u + i))) goto done;
    }
    for (i = 0u; i < NATIVE_MENU_PALETTE_COUNT; ++i) {
        char path[256], maskPath[256];
        (void)snprintf(path, sizeof(path), DIR "/ppu_palette_%u.bin", i + 12u);
        (void)snprintf(maskPath, sizeof(maskPath), DIR "/ppu_palette_%u_mask.bin", i + 12u);
        memset(palette, (int)i, sizeof(palette));
        if (!writeBytes(path, palette, sizeof(palette)) ||
            !writeBytes(maskPath, mask, sizeof(mask))) goto done;
    }

    if (!nativeMenuAssetsLoad(&assets, executable)) {
        printf("FAIL: loader rejected source asset set: %s\n", assets.error);
        goto done;
    }
    if (assets.chr_tile_count != 1u || assets.chr_pixels[17u] != 1u ||
        assets.scenes[0].nametable[0] != 0x20u ||
        assets.scenes[1].nametable[0] != 0x22u ||
        assets.scenes[2].nametable[0] != 0x23u ||
        assets.scenes[3].nametable[0] != 0x24u ||
        assets.scenes[4].nametable[0] != 0x25u ||
        assets.scenes[5].nametable[0] != 0x26u ||
        assets.palettes[6][18u] != 6u ||
        assets.palette_written[6][18u] != 1u) {
        puts("FAIL: native scene merge, CHR decode or palette selection mismatch");
        goto done;
    }
    puts("PASS: six source menu scenes and seven palette presets load without a ROM.");
    ok = 1;

done:
    nativeMenuAssetsUnload(&assets);
    (void)remove(DIR "/chr_pixels.ebp");
    for (i = 0u; i < sizeof(snapshotNames) / sizeof(snapshotNames[0]); ++i) {
        char path[256];
        (void)snprintf(path, sizeof(path), DIR "/%s", snapshotNames[i]);
        (void)remove(path);
    }
    for (i = 0u; i < NATIVE_MENU_PALETTE_COUNT; ++i) {
        char path[256], maskPath[256];
        (void)snprintf(path, sizeof(path), DIR "/ppu_palette_%u.bin", i + 12u);
        (void)snprintf(maskPath, sizeof(maskPath), DIR "/ppu_palette_%u_mask.bin", i + 12u);
        (void)remove(path);
        (void)remove(maskPath);
    }
    (void)rmdir(DIR);
    (void)rmdir(BASE);
    return ok ? 0 : 1;
}
