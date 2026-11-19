#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_menu_assets.h"

#define SNAPSHOT_BYTES (NATIVE_MENU_SCENE_BYTES * 2u + 64u)

static int writeBytes(const char *path, const void *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int writeSnapshot(const char *path, uint8_t tile0, uint8_t tile1)
{
    uint8_t snapshot[SNAPSHOT_BYTES];
    memset(snapshot, 0, sizeof(snapshot));
    snapshot[0] = tile0;
    snapshot[1] = tile1;
    snapshot[NATIVE_MENU_SCENE_BYTES] = 1u;
    snapshot[NATIVE_MENU_SCENE_BYTES + 1u] = tile1 != 0u ? 1u : 0u;
    snapshot[960u] = 0x05u;
    snapshot[NATIVE_MENU_SCENE_BYTES + 960u] = 1u;
    return writeBytes(path, snapshot, sizeof(snapshot));
}

int main(void)
{
    static const char base[] = "/tmp/excitebike-native-assets-test";
    static const char dir[] = "/tmp/excitebike-native-assets-test/extracted";
    static const char executable[] = "/tmp/excitebike-native-assets-test/excitebike3ds.3dsx";
    static const char packPath[] = "/tmp/excitebike-native-assets-test/extracted/chr_pixels.ebp";
    static const char snapshot02[] = "/tmp/excitebike-native-assets-test/extracted/ppu_snapshot_02.bin";
    static const char snapshot03[] = "/tmp/excitebike-native-assets-test/extracted/ppu_snapshot_03.bin";
    static const char snapshot06[] = "/tmp/excitebike-native-assets-test/extracted/ppu_snapshot_06.bin";
    static const char snapshot15[] = "/tmp/excitebike-native-assets-test/extracted/ppu_snapshot_15.bin";
    static const char palettePath[] = "/tmp/excitebike-native-assets-test/extracted/ppu_palette_18.bin";
    static const char paletteMaskPath[] = "/tmp/excitebike-native-assets-test/extracted/ppu_palette_18_mask.bin";
    uint8_t pack[16u + 64u];
    uint8_t palette[32], paletteMask[32];
    NativeMenuAssets assets = {0};
    unsigned i;
    int ok = 0;

    (void)remove(packPath);
    (void)remove(snapshot02);
    (void)remove(snapshot03);
    (void)remove(snapshot06);
    (void)remove(snapshot15);
    (void)remove(palettePath);
    (void)remove(paletteMaskPath);
    (void)rmdir(dir);
    (void)rmdir(base);
    if (mkdir(base, 0777) != 0 || mkdir(dir, 0777) != 0) {
        puts("FAIL: could not create native asset test directory");
        return 1;
    }

    memset(pack, 0, sizeof(pack));
    memcpy(pack, "EBPX", 4u);
    pack[4] = 1u;
    pack[6] = 16u;
    pack[8] = 1u; /* one tile */
    pack[12] = 8u;
    pack[13] = 8u;
    pack[14] = 1u;
    for (i = 0u; i < 64u; ++i) pack[16u + i] = (uint8_t)(i & 3u);
    memset(palette, 0, sizeof(palette));
    memset(paletteMask, 1, sizeof(paletteMask));
    for (i = 0u; i < 32u; ++i) palette[i] = (uint8_t)(i & 0x3Fu);

    if (!writeBytes(packPath, pack, sizeof(pack)) ||
        !writeSnapshot(snapshot02, 0x22u, 0u) ||
        !writeSnapshot(snapshot06, 0x11u, 0x12u) ||
        !writeSnapshot(snapshot03, 0x33u, 0u) ||
        !writeSnapshot(snapshot15, 0x44u, 0u) ||
        !writeBytes(palettePath, palette, sizeof(palette)) ||
        !writeBytes(paletteMaskPath, paletteMask, sizeof(paletteMask))) {
        puts("FAIL: could not write synthetic exported asset pack");
        goto done;
    }

    if (!nativeMenuAssetsLoad(&assets, executable)) {
        printf("FAIL: native asset loader rejected valid EBPX/snapshots: %s\n", assets.error);
        goto done;
    }
    if (assets.chr_tile_count != 1u || assets.chr_pixels[17u] != 1u ||
        assets.scenes[0].nametable[0] != 0x22u ||
        assets.scenes[1].nametable[0] != 0x33u ||
        assets.scenes[1].nametable[1] != 0x12u ||
        assets.scenes[2].nametable[0] != 0x44u ||
        assets.palette[18u] != 18u || assets.palette_written[18u] != 1u) {
        puts("FAIL: EBPX tile data, ordered scene merge or palette load mismatch");
        goto done;
    }
    puts("PASS: native menu loads extracted EBPX, ordered title/selector/design snapshots and palette without ROM.");
    ok = 1;

done:
    nativeMenuAssetsUnload(&assets);
    (void)remove(packPath);
    (void)remove(snapshot02);
    (void)remove(snapshot03);
    (void)remove(snapshot06);
    (void)remove(snapshot15);
    (void)remove(palettePath);
    (void)remove(paletteMaskPath);
    (void)rmdir(dir);
    (void)rmdir(base);
    return ok ? 0 : 1;
}
