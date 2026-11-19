#ifndef EXCITEBIKE_NES_GAME_H
#define EXCITEBIKE_NES_GAME_H
#include <stdint.h>
/*
 * Faithful game path: the ORIGINAL program (statically recompiled from bank_FF.asm, see source/nes_gen_code.c)
 * runs from the reset vector on the NES system layer (source/nes_system.c) using the player's own rom.nes
 * (PRG must match the disassembly's CRC; CHR comes from the ROM).  Title, menus, races, results... are all the
 * original code; nothing is re-implemented here.  Audio and the data-recorder (tape) are not emulated yet.
 */
typedef enum {
    NES_GAME_OK = 0,
    NES_GAME_NO_ROM,         /* rom.nes not found beside the 3DSX */
    NES_GAME_BAD_ROM,        /* not a mapper-0 16 KiB PRG + 8 KiB CHR iNES file */
    NES_GAME_PRG_MISMATCH    /* PRG differs from the disassembled program */
} NesGameStatus;

NesGameStatus nesGameInit(const char *executablePath);
int nesGameAvailable(void);
/* Start (cold boot) / restart the original program. */
void nesGameBoot(void);
/* hidKeysHeld() bits (libctru KEY_*): A,B,SELECT,START,UP,DOWN,LEFT,RIGHT map to the NES pad. Runs one frame. */
int nesGameFrame(uint32_t held);
const uint8_t *nesGameFramebuffer(void);   /* 256x240 RGB, row-major */
const char *nesGameStatusText(void);
uint32_t nesGameFrames(void);
#endif
