#ifndef EXCITEBIKE_NES_MENU_CPU_H
#define EXCITEBIKE_NES_MENU_CPU_H
#include <stdint.h>
int nesMenuCpuInit(const char *executablePath);
int nesMenuCpuAvailable(void);
int nesMenuCpuSetupCommand(uint16_t routineAddress, uint8_t asmState,
                           uint8_t course, uint8_t *ram0045,
                           uint8_t ram004D_EF[0xA3]);
int nesMenuCpuRunRamRoutine(uint16_t routineAddress, uint8_t cpuRam[0x800]);
int nesMenuCpuEditorInit(uint8_t cpuRam[0x800], uint8_t nametables[2048],
                         uint8_t palette[32], uint8_t *ppuCtrl,
                         uint8_t *ppuMask, uint8_t *scrollX,
                         uint8_t *scrollY, uint8_t *paletteWritten);
int nesMenuCpuInput(uint8_t buttonPress, uint8_t count,
                    uint8_t *selection, uint8_t *repeatGuard,
                    uint8_t *timer30, uint8_t *timer31,
                    uint8_t *ram00FF, uint8_t *actionX);
#endif
