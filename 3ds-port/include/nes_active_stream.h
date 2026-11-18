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
/* Last valid selector in tbl_C0F6_lo / tbl_C111_hi ($00-$1A). */
#define NES_EXCITEBIKE_RAM0045_MAX_TABLE_INDEX 0x1Au

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

typedef enum {
    NES_NMI_UPLOAD_NONE = 0,    /* ram_0045 == 0: only the dynamic buffer would run; not modelled, state untouched */
    NES_NMI_UPLOAD_APPLIED,     /* static list executed on the model and ram_0045 cleared */
    NES_NMI_UPLOAD_UNSUPPORTED  /* $1A, >= $1B, bad arguments or a malformed list: state untouched */
} NesNmiUploadResult;

#define NES_MENU_NAMETABLE_BYTES 2048u
#define NES_MENU_PALETTE_BYTES 32u
#define NES_MENU_DYNAMIC_BUFFER_BYTES 32u /* RAM $0300-$031F */

#define NES_MENU_ZERO_PAGE_LIST_BYTES 16u /* RAM $00A0-$00AF */

/*
 * sub_C27F with ram_0045 == $1A (con_0045_1A): tbl_C0F6_lo/tbl_C111_hi point
 * sub_D3C1 at $00A1, so the record stream lives in ramA0[1..] ($00A0 is a length
 * byte that the list itself does not read). Only ram_0045 == $1A runs; any other
 * value returns NONE. On success ram_0045 is cleared (the NMI acknowledgement).
 */
NesNmiUploadResult nesExcitebikeNmiUploadZeroPage(uint8_t *ram0045,
                                                  uint8_t ramA0[NES_MENU_ZERO_PAGE_LIST_BYTES],
                                                  uint8_t nametables[NES_MENU_NAMETABLE_BYTES],
                                                  uint8_t palette[NES_MENU_PALETTE_BYTES]);

/*
 * sub_C27F with ram_0045 == 0: tbl_C0F6_lo/tbl_C111_hi entry $00 points sub_D3C1
 * at $0301, so the dynamic buffer is executed (ram0300[1..]; ram0300[0] is the
 * buffer length byte). A first byte of zero is the end token (nothing happens).
 * On success $0300 and $0301 are cleared exactly like the ASM epilogue. A
 * malformed buffer returns UNSUPPORTED and leaves everything untouched.
 */
NesNmiUploadResult nesExcitebikeNmiUploadDynamic(uint8_t ram0300[NES_MENU_DYNAMIC_BUFFER_BYTES],
                                                 uint8_t nametables[NES_MENU_NAMETABLE_BYTES],
                                                 uint8_t palette[NES_MENU_PALETTE_BYTES]);

/*
 * Native model of sub_C27F for a STATIC list ($01-$19): run the list that
 * tbl_C0F6_lo/tbl_C111_hi select for *ram0045 with sub_D3C1 semantics on top of
 * the CURRENT PPU model, then clear ram_0045 (the NMI acknowledgement). Lists
 * are cumulative on the real PPU, so existing nametable and palette bytes that
 * the list does not touch are preserved.
 *   nametables: CPU-side mirror of PPU $2000-$27FF (2 KiB, attributes included)
 *   palette   : PPU palette RAM $3F00-$3F1F; entries $10/$14/$18/$1C mirror
 *               $00/$04/$08/$0C exactly as on the PPU
 * The dynamic buffer ($00) and the zero-page list ($1A) are not modelled; the
 * state is left untouched for them instead of being fabricated.
 */
NesNmiUploadResult nesExcitebikeNmiUploadStatic(uint8_t *ram0045,
                                                uint8_t nametables[NES_MENU_NAMETABLE_BYTES],
                                                uint8_t palette[NES_MENU_PALETTE_BYTES]);

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
