#include "nes_active_stream.h"
#include "nes_ppu_commands.h"

/* Named values from Excitebike/bank_val.inc (con_0045_* comments). */
static const char *ram0045Hint(uint8_t value)
{
    switch (value) {
    case 0x02u: return "blue background";
    case 0x18u: return "title screen";
    default: return NULL;
    }
}

int nesExcitebikeActiveStream(uint8_t ram0045, NesActiveStreamInfo *info)
{
    const uint8_t *stream = NULL;
    size_t streamSize = 0u;

    if (info == NULL) return 0;
    info->valid = 0;
    info->isPalette = 0;
    info->arrayIndex = 0u;
    info->sourceLabel = NULL;
    info->hint = ram0045Hint(ram0045);

    if (ram0045 >= NES_EXCITEBIKE_RAM0045_MIN_PPU &&
        ram0045 <= NES_EXCITEBIKE_RAM0045_MAX_PPU) {
        info->isPalette = 0;
        info->arrayIndex = (size_t)ram0045 - NES_EXCITEBIKE_RAM0045_MIN_PPU;
        stream = nesExcitebikePpuStream(info->arrayIndex, &streamSize, &info->sourceLabel);
    } else if (ram0045 >= NES_EXCITEBIKE_RAM0045_MIN_PALETTE &&
               ram0045 <= NES_EXCITEBIKE_RAM0045_MAX_PALETTE) {
        info->isPalette = 1;
        info->arrayIndex = (size_t)ram0045 - NES_EXCITEBIKE_RAM0045_MIN_PALETTE;
        stream = nesExcitebikePaletteStream(info->arrayIndex, &streamSize, &info->sourceLabel);
    } else if (ram0045 == NES_EXCITEBIKE_RAM0045_D631) {
        info->isPalette = 0;
        info->arrayIndex = 17u; /* slot 18 in nesExcitebikePpuStream indexing */
        stream = nesExcitebikePpuStream(info->arrayIndex, &streamSize, &info->sourceLabel);
    } else {
        /* $00 (cleared at reset) and $1A (26) are not captured statically. */
        return 0;
    }
    if (stream == NULL) return 0;
    info->valid = 1;
    return 1;
}

/*
 * Source-derived ram_0045 writes per menu setup command, from the translated
 * setup lists (menus.c states 0/1/4/6/8, the state-2 helpers, and the shared
 * entries documented in ASM_FUNCTION_INVENTORY.md). States whose setup lists
 * are not yet translated (5, 9, 10, 11, 12) intentionally have no rows here.
 */
static const NesRam0045SetupWrite s_setupWrites[] = {
    {0x00u, 0u, 0x02u, "ofs_001_C2BD_00_01"},
    {0x00u, 1u, 0x18u, "ofs_001_C514_00_02"},
    {0x01u, 0u, 0x06u, "ofs_001_C55E_01_01"},
    {0x01u, 1u, 0x03u, "ofs_001_C3F3_01_02"},
    {0x01u, 2u, 0x18u, "ofs_001_C514_01_03"},
    {0x02u, 0u, 0x12u, "ofs_001_C551_02_01"},
    {0x02u, 3u, 0x17u, "ofs_001_C5A3_02_04"},
    {0x02u, 4u, 0x0Eu, "ofs_001_C566_02_05"},
    {0x03u, 0u, 0x06u, "ofs_001_C55E_03_01"},
    {0x03u, 1u, 0x04u, "ofs_001_C56A_03_02"},
    {0x04u, 0u, 0x06u, "ofs_001_C55E_04_01"},
    {0x04u, 1u, 0x09u, "ofs_001_C571_04_02"},
    {0x04u, 3u, 0x18u, "ofs_001_C514_04_04"},
    {0x06u, 0u, 0x06u, "ofs_001_C55E_06_01"},
    {0x06u, 1u, 0x0Fu, "ofs_001_C575_06_02"},
    {0x06u, 2u, 0x18u, "ofs_001_C514_06_03"},
    {0x08u, 0u, 0x11u, "ofs_001_C582_08_01"},
    {0x0Du, 0u, 0x11u, "ofs_001_C582_08_01 (state 13 shares list 8)"}
};

size_t nesExcitebikeRam0045SetupWriteCount(void)
{
    return sizeof(s_setupWrites) / sizeof(s_setupWrites[0]);
}

const NesRam0045SetupWrite *nesExcitebikeRam0045SetupWrite(size_t index)
{
    if (index >= nesExcitebikeRam0045SetupWriteCount()) return NULL;
    return &s_setupWrites[index];
}
