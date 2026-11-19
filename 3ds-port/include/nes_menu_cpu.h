#ifndef EXCITEBIKE_NES_MENU_CPU_H
#define EXCITEBIKE_NES_MENU_CPU_H
#include <stdint.h>
int nesMenuCpuInit(const char *executablePath);
int nesMenuCpuAvailable(void);
int nesMenuCpuSetupCommand(uint16_t routineAddress, uint8_t asmState,
                           uint8_t course, uint8_t *ram0045,
                           uint8_t ram004D_EF[0xA3]);
/* Audited RAM/buffer-only routines: C465, C5A3, C562 and C5C2. */
int nesMenuCpuRunRamRoutine(uint16_t routineAddress, uint8_t cpuRam[0x800]);
typedef struct {
    uint8_t topCtrl, topScrollX, topScrollY;
    uint8_t bottomCtrl, bottomScrollX, bottomScrollY;
    uint8_t splitScanline;
    uint16_t splitDot;
    uint32_t elapsedCycles;
} NesMenuRasterSplit;

/* Execute D0AB/D14E, capture both PPUCTRL/scroll phases and return elapsed CPU cycles. */
/* C97C receives a pixel-derived sprite-0 hit threshold; UINT32_MAX means no hit. */
int nesMenuCpuRasterSplit(uint8_t cpuRam[0x800], uint8_t nametables[2048],
                          uint8_t palette[32], uint8_t *ppuCtrl,
                          uint8_t *ppuMask, uint8_t *scrollX, uint8_t *scrollY,
                          uint8_t previousFrameHadSprite0Hit, NesMenuRasterSplit *split);
int nesMenuCpuEditorFrame(uint8_t cpuRam[0x800], uint8_t heldButtons,
                          uint8_t pressedButtons, uint8_t nametables[2048],
                          uint8_t palette[32], uint8_t *ppuCtrl,
                          uint8_t *ppuMask, uint8_t *scrollX, uint8_t *scrollY,
                          uint32_t sprite0HitCycleFromCallStart);
int nesMenuCpuEditorInit(uint8_t cpuRam[0x800], uint8_t nametables[2048],
                         uint8_t palette[32], uint8_t *ppuCtrl,
                         uint8_t *ppuMask, uint8_t *scrollX,
                         uint8_t *scrollY, uint8_t *paletteWritten);
int nesMenuCpuInput(uint8_t buttonPress, uint8_t count,
                    uint8_t *selection, uint8_t *repeatGuard,
                    uint8_t *timer30, uint8_t *timer31,
                    uint8_t *ram00FF, uint8_t *actionX);
#endif
