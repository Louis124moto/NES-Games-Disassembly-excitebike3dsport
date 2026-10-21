#include <string.h>
#include "nes_ppu_commands.h"

/* Literal command bytes transcribed from Excitebike/bank_FF.asm. */
static const uint8_t s_palette12[] = {0x3F,0x00,0x08,0x29,0x27,0x22,0x30,0x29,0x27,0x18,0x36,0x3F,0x10,0x04,0x29,0x20,0x16,0x0F,0x00};
static const uint8_t s_palette13[] = {0x3F,0x00,0x08,0x01,0x1A,0x26,0x33,0x01,0x11,0x0C,0x2C,0x3F,0x10,0x04,0x01,0x20,0x16,0x0F,0x00};
static const uint8_t s_palette14[] = {0x3F,0x00,0x08,0x02,0x26,0x1C,0x30,0x02,0x29,0x19,0x39,0x3F,0x10,0x04,0x02,0x20,0x16,0x0F,0x00};
static const uint8_t s_palette15[] = {0x3F,0x00,0x08,0x00,0x26,0x22,0x30,0x00,0x27,0x18,0x37,0x3F,0x10,0x04,0x00,0x20,0x16,0x0F,0x00};
static const uint8_t s_palette16[] = {0x3F,0x00,0x08,0x02,0x19,0x22,0x30,0x02,0x00,0x2D,0x10,0x3F,0x10,0x04,0x02,0x20,0x16,0x0F,0x00};
static const uint8_t s_palette17[] = {0x3F,0x08,0x08,0x29,0x22,0x0F,0x20,0x29,0x22,0x0F,0x16,0x3F,0x14,0x0C,0x29,0x13,0x20,0x0F,0x29,0x31,0x1C,0x0F,0x29,0x20,0x19,0x0F,0x00};
static const uint8_t s_palette18[] = {0x3F,0x00,0x1C,0x02,0x0F,0x30,0x21,0x02,0x15,0x02,0x2A,0x02,0x3C,0x02,0x30,0x02,0x30,0x02,0x27,0x02,0x20,0x16,0x0F,0x02,0x27,0x13,0x3C,0x02,0x27,0x16,0x30,0x00};

typedef struct {
    const uint8_t *bytes;
    size_t size;
    const char *label;
} PaletteStream;

static const PaletteStream s_streams[NES_EXCITEBIKE_PALETTE_STREAM_COUNT] = {
    {s_palette12, sizeof(s_palette12), "_off_000_D3D6_12 (00:D3D6)"},
    {s_palette13, sizeof(s_palette13), "_off_000_D3E9_13 (00:D3E9)"},
    {s_palette14, sizeof(s_palette14), "_off_000_D3FC_14 (00:D3FC)"},
    {s_palette15, sizeof(s_palette15), "_off_000_D40F_15 (00:D40F)"},
    {s_palette16, sizeof(s_palette16), "_off_000_D422_16 (00:D422)"},
    {s_palette17, sizeof(s_palette17), "_off_000_D435_17 (00:D435)"},
    {s_palette18, sizeof(s_palette18), "_off_000_D450_18 (00:D450)"}
};

static size_t mirrorPaletteAddress(size_t address)
{
    size_t index;
    if (address < 0x3F00u || address > 0x3FFFu) return address;
    index = address & 0x1Fu;
    if (index >= 0x10u && (index & 0x03u) == 0u) index -= 0x10u;
    return 0x3F00u + index;
}

NesPpuCommandResult nesPpuApplyCommandStream(const uint8_t *stream, size_t streamSize,
                                              uint8_t *ppuRam, uint8_t *writtenMask,
                                              size_t ppuRamSize, size_t *writeCount)
{
    size_t offset = 0u;
    size_t writes = 0u;
    if (writeCount != NULL) *writeCount = 0u;
    if (stream == NULL || ppuRam == NULL || writtenMask == NULL || writeCount == NULL ||
        ppuRamSize < NES_PPU_ADDRESS_SPACE)
        return NES_PPU_COMMAND_INVALID_ARGUMENT;
    while (offset < streamSize) {
        size_t address, count, stride, i;
        uint8_t control;
        int repeat;
        uint8_t repeatedValue = 0u;
        if (streamSize - offset < 3u) return NES_PPU_COMMAND_TRUNCATED;
        address = ((size_t)stream[offset] << 8u) | stream[offset + 1u];
        control = stream[offset + 2u];
        offset += 3u;
        if (control == 0u) {
            *writeCount = writes;
            return NES_PPU_COMMAND_OK;
        }
        count = (size_t)(control & 0x3Fu);
        stride = (control & 0x80u) != 0u ? 32u : 1u;
        repeat = (control & 0x40u) != 0u;
        if (count == 0u) return NES_PPU_COMMAND_INVALID_COUNT;
        if (repeat) {
            if (offset >= streamSize) return NES_PPU_COMMAND_TRUNCATED;
            repeatedValue = stream[offset++];
        } else if (count > streamSize - offset) {
            return NES_PPU_COMMAND_TRUNCATED;
        }
        for (i = 0u; i < count; ++i) {
            size_t target;
            uint8_t value;
            if (address >= NES_PPU_ADDRESS_SPACE) return NES_PPU_COMMAND_ADDRESS_RANGE;
            target = mirrorPaletteAddress(address);
            if (target >= ppuRamSize) return NES_PPU_COMMAND_ADDRESS_RANGE;
            value = repeat ? repeatedValue : stream[offset + i];
            ppuRam[target] = value;
            writtenMask[target] = 1u;
            ++writes;
            address = (address + stride) & 0x3FFFu;
        }
        if (!repeat) offset += count;
    }
    return NES_PPU_COMMAND_TRUNCATED; /* No explicit end token. */
}

const uint8_t *nesExcitebikePaletteStream(size_t index, size_t *streamSize,
                                          const char **sourceLabel)
{
    if (streamSize != NULL) *streamSize = 0u;
    if (sourceLabel != NULL) *sourceLabel = NULL;
    if (index >= NES_EXCITEBIKE_PALETTE_STREAM_COUNT) return NULL;
    if (streamSize != NULL) *streamSize = s_streams[index].size;
    if (sourceLabel != NULL) *sourceLabel = s_streams[index].label;
    return s_streams[index].bytes;
}

NesPpuCommandResult nesExcitebikePaletteSnapshot(size_t index, uint8_t palette[32],
                                                  uint8_t writtenMask[32],
                                                  size_t *writeCount)
{
    static uint8_t ppuRam[NES_PPU_ADDRESS_SPACE];
    static uint8_t ppuMask[NES_PPU_ADDRESS_SPACE];
    const uint8_t *stream;
    size_t streamSize, writes, i;
    NesPpuCommandResult result;
    if (palette == NULL || writtenMask == NULL || writeCount == NULL)
        return NES_PPU_COMMAND_INVALID_ARGUMENT;
    stream = nesExcitebikePaletteStream(index, &streamSize, NULL);
    if (stream == NULL) return NES_PPU_COMMAND_INVALID_ARGUMENT;
    memset(ppuRam, 0, sizeof(ppuRam));
    memset(ppuMask, 0, sizeof(ppuMask));
    result = nesPpuApplyCommandStream(stream, streamSize, ppuRam, ppuMask,
                                      sizeof(ppuRam), &writes);
    if (result != NES_PPU_COMMAND_OK) return result;
    for (i = 0u; i < 32u; ++i) {
        palette[i] = ppuRam[0x3F00u + i];
        writtenMask[i] = ppuMask[0x3F00u + i];
    }
    *writeCount = writes;
    return NES_PPU_COMMAND_OK;
}
