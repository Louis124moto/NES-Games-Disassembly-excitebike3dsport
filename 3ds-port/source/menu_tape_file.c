#include "menu_tape_file.h"

#include <stdio.h>
#include <string.h>

#define BLOCK0_BYTES 64u
#define BLOCK1_BYTES 256u

static char s_path[256] = MENU_TAPE_FILE_DEFAULT_PATH;

uint16_t menuTapeChecksum(const uint8_t *data, size_t size)
{
    uint16_t sum = 0u;
    size_t i;
    unsigned int bit;
    if (data == NULL) return 0u;
    for (i = 0u; i < size; ++i) {
        for (bit = 0u; bit < 8u; ++bit) {
            if ((data[i] >> bit) & 1u) ++sum; /* sub_C68A: 16-bit INC per 1 bit */
        }
    }
    return sum;
}

void menuTapeFileSetPath(const char *path)
{
    if (path == NULL) path = MENU_TAPE_FILE_DEFAULT_PATH;
    strncpy(s_path, path, sizeof(s_path) - 1u);
    s_path[sizeof(s_path) - 1u] = '\0';
}

static int tapeSave(void *ctx, const uint8_t *block0, const uint8_t *block1)
{
    FILE *file;
    uint8_t sum[2];
    uint16_t value;
    int ok = 1;
    (void)ctx;
    if (block0 == NULL || block1 == NULL) return 0;
    file = fopen(s_path, "wb");
    if (file == NULL) return 0;
    value = menuTapeChecksum(block0, BLOCK0_BYTES);
    sum[0] = (uint8_t)(value & 0xFFu); sum[1] = (uint8_t)(value >> 8);
    ok &= fwrite(block0, 1u, BLOCK0_BYTES, file) == BLOCK0_BYTES && fwrite(sum, 1u, 2u, file) == 2u;
    value = menuTapeChecksum(block1, BLOCK1_BYTES);
    sum[0] = (uint8_t)(value & 0xFFu); sum[1] = (uint8_t)(value >> 8);
    ok &= fwrite(block1, 1u, BLOCK1_BYTES, file) == BLOCK1_BYTES && fwrite(sum, 1u, 2u, file) == 2u;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int tapeReadBlock(void *ctx, unsigned int block, uint8_t *dst, size_t size)
{
    FILE *file;
    uint8_t sum[2];
    size_t payload = block == 0u ? BLOCK0_BYTES : BLOCK1_BYTES;
    long offset = block == 0u ? 0L : (long)(BLOCK0_BYTES + 2u);
    (void)ctx;
    if (dst == NULL || block > 1u || size != payload) return -1;
    file = fopen(s_path, "rb");
    if (file == NULL) return 0; /* no recorder signal: the original waits */
    if (fseek(file, offset, SEEK_SET) != 0 ||
        fread(dst, 1u, payload, file) != payload || fread(sum, 1u, 2u, file) != 2u) {
        fclose(file);
        return -1; /* truncated: framing error */
    }
    fclose(file);
    return menuTapeChecksum(dst, payload) == (uint16_t)(sum[0] | (sum[1] << 8)) ? 1 : -1;
}

const MenusTapeIo *menuTapeFileIo(void)
{
    static const MenusTapeIo io = { tapeSave, tapeReadBlock, NULL };
    return &io;
}
