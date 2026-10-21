#include <stdio.h>
#include <string.h>
#include "nes_ppu_commands.h"

static int testLiteralAndFillCommands(void)
{
    const uint8_t stream[] = {0x20,0x00,0x03,0x11,0x22,0x33,0x23,0xD0,0x42,0x55,0x00};
    uint8_t ram[NES_PPU_ADDRESS_SPACE];
    uint8_t mask[NES_PPU_ADDRESS_SPACE];
    size_t writes = 0u;
    memset(ram, 0, sizeof(ram));
    memset(mask, 0, sizeof(mask));
    if (nesPpuApplyCommandStream(stream, sizeof(stream), ram, mask, sizeof(ram), &writes) != NES_PPU_COMMAND_OK) return 1;
    if (writes != 5u || ram[0x2000] != 0x11u || ram[0x2001] != 0x22u || ram[0x2002] != 0x33u) return 2;
    if (ram[0x23D0] != 0x55u || ram[0x23D1] != 0x55u || ram[0x23D2] != 0x00u) return 3;
    return 0;
}

static int testSourcePaletteStreams(void)
{
    uint8_t palette[32], mask[32];
    size_t writes = 0u, size = 0u;
    const char *label = NULL;
    const uint8_t *stream = nesExcitebikePaletteStream(0u, &size, &label);
    if (stream == NULL || size != 19u || label == NULL || strstr(label, "D3D6") == NULL) return 1;
    if (nesExcitebikePaletteSnapshot(0u, palette, mask, &writes) != NES_PPU_COMMAND_OK) return 2;
    if (writes != 12u) return 30;
    if (palette[0] != 0x29u || palette[1] != 0x27u || palette[2] != 0x22u || palette[3] != 0x30u) return 31;
    if (palette[4] != 0x29u || palette[7] != 0x36u) return 32;
    if (palette[16] != palette[0]) return 33;
    if (mask[0] != 1u || mask[15] != 0u || mask[16] != 1u) return 34;
    if (nesExcitebikePaletteStream(7u, &size, &label) != NULL) return 4;
    return 0;
}

int main(void)
{
    int result = testLiteralAndFillCommands();
    if (result != 0) { fprintf(stderr, "PPU command interpreter test failed: %d\n", result); return 1; }
    result = testSourcePaletteStreams();
    if (result != 0) { fprintf(stderr, "Source palette stream test failed: %d\n", result); return 1; }
    puts("PASS: source-format PPU write-list interpreter and seven verified palette streams.");
    return 0;
}
