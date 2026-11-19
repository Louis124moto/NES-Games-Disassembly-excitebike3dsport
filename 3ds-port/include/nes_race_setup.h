#ifndef NES_RACE_SETUP_H
#define NES_RACE_SETUP_H
#include <stdint.h>

/*
 * Source-translated setup commands for race setup list 2 (state 2) / 5.
 * Each function is a line-by-line translation of the named routine in
 * Excitebike/bank_FF.asm; the RAM model is flat: zp = $0000-$00FF, pg03 = $0300-$03FF,
 * pg04 = $0400-$04FF, pg05 = $0500-$05FF, oam = $0200-$02FF.
 */
typedef struct NesRaceRam {
    uint8_t zp[256];
    uint8_t oam[256];
    uint8_t pg03[256];
    uint8_t pg04[256];
    uint8_t pg05[256];
} NesRaceRam;

#define NES_ZP_0040 0x40u
#define NES_ZP_0041 0x41u
#define NES_ZP_0043 0x43u
#define NES_ZP_0045 0x45u
#define NES_ZP_0046 0x46u
#define NES_ZP_0047 0x47u
#define NES_ZP_004B 0x4Bu
#define NES_ZP_0034 0x34u

/* ofs_001_C551_02_01: ram_0045 = $12 (+ ram_0043 when ram_0041 < 8). */
void nesRaceSetupC551(NesRaceRam *r);
/* ofs_001_C869_02_02: clear $004D-$00EF. */
void nesRaceSetupC869(NesRaceRam *r);
/* ofs_001_C820_02_03 after its JSR sub_D13F (sub_D13F itself is NOT ported). */
void nesRaceSetupC820AfterD13F(NesRaceRam *r);
/* ofs_001_C5A3_02_04: ram_0045 = $17. */
void nesRaceSetupC5A3(NesRaceRam *r);
/* ofs_001_C566_02_05: ram_0045 = $0E. */
void nesRaceSetupC566(NesRaceRam *r);
/*
 * ofs_001_C875_02_06 from its entry through the object-init loop and the
 * "LSR ram_0078,X" at bra_C8DB. The tail starting at bra_C8DE (ram_0046 test,
 * sub_C522, sub_C53A, sub_C935 ...) is NOT ported.
 */
void nesRaceSetupC875Head(NesRaceRam *r);
/* sub_ECE4: build the two dynamic PPU command buffers for X = 0, 4, or 8.
 * Returns 0 for an invalid table offset. PPU execution at loc_C289 is separate.
 */
int nesRaceSetupECE4(NesRaceRam *r, unsigned int tableOffset);

#endif
