#ifndef EXCITEBIKE_NES_PPU_COMMANDS_H
#define EXCITEBIKE_NES_PPU_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#define NES_PPU_ADDRESS_SPACE 0x4000u
#define NES_EXCITEBIKE_PALETTE_STREAM_COUNT 7u
#define NES_EXCITEBIKE_PPU_STREAM_COUNT 18u
#define NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES 0x1000u

typedef enum {
    NES_PPU_MIRROR_NONE = 0,
    NES_PPU_MIRROR_HORIZONTAL,
    NES_PPU_MIRROR_VERTICAL
} NesPpuNametableMirroring;

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
 * A zero FIRST byte of a record (address-hi) is the end token, exactly as in
 * sub_D3C1 (LDA (ptr),Y / BNE at $D3C6-$D3C8). A zero control byte inside a
 * record is rejected here as a conservative guard; the original would not stop
 * there but write 256 bytes (DEX wraps from 0), and no source list contains it.
 */
NesPpuCommandResult nesPpuApplyCommandStream(const uint8_t *stream, size_t streamSize,
                                              uint8_t *ppuRam, uint8_t *writtenMask,
                                              size_t ppuRamSize, size_t *writeCount);
/* Apply writes through a cartridge's physical nametable mirroring before storing them. */
NesPpuCommandResult nesPpuApplyCommandStreamMirrored(
    const uint8_t *stream, size_t streamSize, uint8_t *ppuRam, uint8_t *writtenMask,
    size_t ppuRamSize, size_t *writeCount, NesPpuNametableMirroring mirroring);

/* Palette command streams are source-derived from bank_FF.asm addresses D3D6-D46F. */
const uint8_t *nesExcitebikePaletteStream(size_t index, size_t *streamSize,
                                          const char **sourceLabel);
NesPpuCommandResult nesExcitebikePaletteSnapshot(size_t index, uint8_t palette[32],
                                                  uint8_t writtenMask[32],
                                                  size_t *writeCount);

/* Static PPU command lists at bank_FF.asm table C0F6 indices 1..17. */
const uint8_t *nesExcitebikePpuStream(size_t index, size_t *streamSize,
                                      const char **sourceLabel);
NesPpuCommandResult nesExcitebikePpuSnapshot(size_t index,
                                              uint8_t nametable[ NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES ],
                                              uint8_t nametableMask[ NES_EXCITEBIKE_NAMETABLE_SNAPSHOT_BYTES ],
                                              uint8_t palette[32], uint8_t paletteMask[32],
                                              size_t *writeCount);

#endif
