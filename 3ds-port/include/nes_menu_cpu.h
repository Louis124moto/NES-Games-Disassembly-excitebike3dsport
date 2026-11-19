#ifndef EXCITEBIKE_NES_MENU_CPU_H
#define EXCITEBIKE_NES_MENU_CPU_H
#include <stdint.h>
int nesMenuCpuInit(const char *executablePath);
int nesMenuCpuAvailable(void);
int nesMenuCpuInput(uint8_t buttonPress, uint8_t count,
                    uint8_t *selection, uint8_t *repeatGuard,
                    uint8_t *timer30, uint8_t *timer31,
                    uint8_t *ram00FF, uint8_t *actionX);
#endif
