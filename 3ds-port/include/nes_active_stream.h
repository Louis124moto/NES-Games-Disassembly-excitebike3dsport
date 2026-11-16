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
 * Read side (source trace, 2026-11-16, Excitebike/bank_FF.asm): the only
 * live reader is the NMI. The NMI vector at $FFFA is vec_C23B_NMI (CPU $C23B,
 * ROM offset 0x00024B). It calls sub_C27F (CPU $C27F, ROM offset 0x00028F),
 * which loads X = ram_0045, builds a pointer from tbl_C0F6_lo,X / tbl_C111_hi,X
 * in ram_0000/ram_0001 and runs the list with sub_D3C1, then clears ram_0300,
 * ram_0301 and ram_0045 and re-points the PPU address latch at $3F00 / $0000.
 * The other two loads of ram_0045 (C74B, C782) are busy-wait loops waiting for
 * that clear and are not covered by the CDL. $D55A and $CC2D, which earlier
 * revisions of this project named as the consumer, are NOT: $D55A is data inside
 * the list at D524 and $CC2D is the ADC #$0B inside sub_CC15 that WRITES ram_0045.
 * Index $00 resolves to the dynamic PPU buffer at $0301 ($0300 holds the write
 * offset); index $1A resolves to the zero-page list at $00A1 and is only reached
 * from code the CDL never executed.
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

/* Number of entries in tbl_C0F6_lo / tbl_C111_hi (indices $00-$1A). */
#define NES_EXCITEBIKE_RAM0045_TABLE_ENTRIES 27u
#define NES_EXCITEBIKE_RAM0045_DYNAMIC_BUFFER 0x00u
#define NES_EXCITEBIKE_RAM0045_ZERO_PAGE_LIST 0x1Au

typedef enum {
    NES_RAM0045_TARGET_STATIC_LIST = 0, /* $01-$19: fixed ROM list (PPU update or palette) */
    NES_RAM0045_TARGET_DYNAMIC_BUFFER,  /* $00: RAM buffer at $0301, write offset in $0300 */
    NES_RAM0045_TARGET_ZERO_PAGE_LIST,  /* $1A: list at $00A1 (code path not covered by the CDL) */
    NES_RAM0045_TARGET_OUT_OF_TABLE     /* >= $1B: no table entry; the original would read neighbouring bytes */
} NesRam0045TargetKind;

typedef struct {
    NesRam0045TargetKind kind;
    uint16_t cpuAddress; /* list start from tbl_C0F6_lo/tbl_C111_hi; 0 when out of table */
} NesRam0045Target;

/* Where sub_C27F points sub_D3C1 for a given ram_0045 (transcribed from the two tables). */
NesRam0045Target nesExcitebikeRam0045Target(uint8_t ram0045);

/*
 * vec_C23B_NMI, CPU $C23E-$C249: only when ram_0045 is zero does the NMI OR $1A
 * (background + sprites + left-8 background) into ram_for_2001 and write it to
 * $2001. Returns the new ram_for_2001; *wroteMask is 1 when $2001 was written.
 */
uint8_t nesExcitebikeNmiRamFor2001(uint8_t ram0045, uint8_t ramFor2001, int *wroteMask);

/*
 * sub_C27F, CPU $C28E-$C296: after the list has been executed the NMI clears
 * ram_0300, ram_0301 and ram_0045 (an acknowledgement for the setup code).
 */
void nesExcitebikeNmiUploadAck(uint8_t *ram0045, uint8_t *ram0300, uint8_t *ram0301);

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
