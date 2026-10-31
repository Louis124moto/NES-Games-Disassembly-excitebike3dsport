#ifndef EXCITEBIKE_NES_ACTIVE_STREAM_H
#define EXCITEBIKE_NES_ACTIVE_STREAM_H

#include <stddef.h>
#include <stdint.h>

/*
 * Active command-stream selection trace for ram_0045 (CPU $0045).
 *
 * Evidence (Excitebike/bank_val.inc, con_0045_* block): ram_0045 is the
 * index used with the PPU command-stream table at CPU $C0F6
 * (ROM offset 0x0106), with named values con_0045_02 = "blue background"
 * and con_0045_18 = "title screen". The embedded stream arrays in
 * nes_ppu_commands.c map table indices $01-$11 and $19 to static PPU
 * update lists and $12-$18 to the seven palette command lists (D3D6-D450).
 * Menu setup commands write ram_0045 with exactly these table indices
 * (translated in menus.c and the state-2 helpers in nes_routines.c), so
 * the write side of the active palette/stream selection per game state is
 * tabulated here.
 *
 * NOT yet proven: the exact instruction-level reader of ram_0045
 * (consumers noted in bank_val.inc at ROM offsets 0x00056A / 0x000C3D)
 * and dynamic indices $00 / $1A, which are not captured statically.
 */
#define NES_EXCITEBIKE_RAM0045_MIN_PPU 0x01u
#define NES_EXCITEBIKE_RAM0045_MAX_PPU 0x11u
#define NES_EXCITEBIKE_RAM0045_MIN_PALETTE 0x12u
#define NES_EXCITEBIKE_RAM0045_MAX_PALETTE 0x18u
#define NES_EXCITEBIKE_RAM0045_D631 0x19u
#define NES_EXCITEBIKE_RAM0045_UNCAPTURED_HIGH 0x1Au

typedef struct {
    int valid;              /* 1 when ram0045 maps to a captured static stream */
    int isPalette;          /* 1: palette command list, 0: PPU update list */
    size_t arrayIndex;      /* index into the palette or PPU stream array */
    const char *sourceLabel;
    const char *hint;       /* source-named value (e.g. "title screen") or NULL */
} NesActiveStreamInfo;

int nesExcitebikeActiveStream(uint8_t ram0045, NesActiveStreamInfo *info);

/* Source-derived ram_0045 writes performed by each menu setup command list. */
typedef struct {
    uint8_t asmState;        /* ram_0041 state whose setup list writes ram_0045 */
    uint8_t commandIndex;    /* $0044-1 at write time */
    uint8_t ram0045;         /* value written */
    const char *commandLabel;
} NesRam0045SetupWrite;

size_t nesExcitebikeRam0045SetupWriteCount(void);
const NesRam0045SetupWrite *nesExcitebikeRam0045SetupWrite(size_t index);

#endif
