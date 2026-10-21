#ifndef EXCITEBIKE_NES_PPU_COMMANDS_H
#define EXCITEBIKE_NES_PPU_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#define NES_PPU_ADDRESS_SPACE 0x4000u
#define NES_EXCITEBIKE_PALETTE_STREAM_COUNT 7u

typedef enum {
    NES_PPU_COMMAND_OK = 0,
    NES_PPU_COMMAND_INVALID_ARGUMENT,
    NES_PPU_COMMAND_TRUNCATED,
    NES_PPU_COMMAND_INVALID_COUNT,
    NES_PPU_COMMAND_ADDRESS_RANGE
} NesPpuCommandResult;

/*
 * Interpret the original Excitebike PPU update-list format used by sub_D3C1.
 * Bytes are address-hi, address-lo, control, then either control&0x3F literal
 * bytes or one repeated byte when bit 6 is set. Bit 7 selects +32 PPU stride.
 * A zero control byte terminates the list.
 */
NesPpuCommandResult nesPpuApplyCommandStream(const uint8_t *stream, size_t streamSize,
                                              uint8_t *ppuRam, uint8_t *writtenMask,
                                              size_t ppuRamSize, size_t *writeCount);

/* Palette command streams are source-derived from bank_FF.asm addresses D3D6-D46F. */
const uint8_t *nesExcitebikePaletteStream(size_t index, size_t *streamSize,
                                          const char **sourceLabel);
NesPpuCommandResult nesExcitebikePaletteSnapshot(size_t index, uint8_t palette[32],
                                                  uint8_t writtenMask[32],
                                                  size_t *writeCount);

#endif
