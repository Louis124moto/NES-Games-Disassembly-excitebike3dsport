#ifndef NES_SYSTEM_H
#define NES_SYSTEM_H
/*
 * NES system layer around the statically recompiled original program (source/nes_gen_code.c):
 * CPU context (cooperative main thread + NMI), PPU registers/VRAM/OAM with a scanline renderer
 * (scroll splits, sprites, sprite-0 hit), controller port, OAM DMA. Audio is not modelled yet.
 * Only mapper 0 / NROM-128 (Excitebike) is supported; the PRG image must match the ASM-derived CRC.
 */
#include <stddef.h>
#include <stdint.h>
#include "nes_machine.h"
#include "nes_apu.h"

#define NES_SYS_WIDTH 256u
#define NES_SYS_HEIGHT 240u
#define NES_SYS_FRAME_CYCLES 29781ul
#define NES_SYS_VBLANK_CYCLE 27394ul     /* scanline 241 * 341 / 3 */

typedef enum {
    NES_SYS_OK = 0,
    NES_SYS_BAD_ROM,          /* not an iNES NROM-128 image with 8 KiB CHR */
    NES_SYS_PRG_MISMATCH,     /* PRG CRC differs from the ROM the recompiled code was generated from */
    NES_SYS_CODE_ERROR        /* recompiled code reached an unknown address (m.pc) */
} NesSystemResult;

typedef struct NesSystem {
    NesMachine m;
    uint8_t prg[0x4000];
    uint8_t chr[0x2000];
    int verticalMirroring;
    /* PPU */
    uint8_t ctrl, mask, status, oamAddr, readBuf, fineX, w;
    uint16_t v, t;
    uint8_t vram[0x800];
    uint8_t pal[32];
    uint8_t oam[256];
    /* controller */
    uint8_t pad, padShift, strobe;
    /* timing / frame state */
    unsigned long frameStart;
    unsigned renderLine;          /* next scanline to render (0..240) */
    int s0Line, s0X;              /* sprite-0 hit position this frame, -1 when none */
    uint16_t mainPc;              /* resume address of the single CPU context */
    int vblankDone;               /* 1 between the vblank event and the end-of-frame event */
    unsigned long frames;
    uint8_t rgbPalette[64][3];
    uint8_t fb[NES_SYS_WIDTH * NES_SYS_HEIGHT * 3u];
    NesApu apu;
    unsigned long audioStart;     /* CPU cycle at which the current audio chunk started */
    int16_t audio[1024];          /* 32 kHz mono samples of the last completed frame */
    unsigned audioCount;
    int lastError;                /* NesSystemResult of the last frame */
} NesSystem;

NesSystemResult nesSystemInit(NesSystem *s, const uint8_t *rom, size_t romSize);
/* pad bits: 0 A, 1 B, 2 Select, 3 Start, 4 Up, 5 Down, 6 Left, 7 Right (hardware shift order). */
void nesSystemSetPad(NesSystem *s, uint8_t pad);
/* Run one video frame; the framebuffer (RGB, 256x240) is complete afterwards. */
NesSystemResult nesSystemRunFrame(NesSystem *s);
static inline const uint8_t *nesSystemFramebuffer(const NesSystem *s) { return s->fb; }

#endif
