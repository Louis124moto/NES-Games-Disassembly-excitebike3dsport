#include <stdio.h>
#include <string.h>
#include "nes_machine.h"
#include "nes_gen_code.h"
#include "nes_menu_cpu.h"
#define NES_PRG_BYTES 0x4000u
#define PATH_CAPACITY 512u
static uint8_t s_prg[NES_PRG_BYTES];
static int s_ready;
int nesMenuCpuInit(const char *executablePath)
{
    char path[PATH_CAPACITY];
    const char *slash = NULL;
    size_t i, directoryLength = 0u;
    FILE *file;
    uint8_t header[16];
    long offset;
    s_ready = 0;
    if (executablePath != NULL) {
        for (i = 0u; executablePath[i] != '\0'; ++i)
            if (executablePath[i] == '/' || executablePath[i] == '\\') slash = &executablePath[i];
    }
    if (slash != NULL) directoryLength = (size_t)(slash - executablePath) + 1u;
    if (directoryLength + sizeof("rom.nes") > sizeof(path)) return 0;
    if (directoryLength != 0u) memcpy(path, executablePath, directoryLength);
    memcpy(path + directoryLength, "rom.nes", sizeof("rom.nes"));
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        memcmp(header, "NES\x1A", 4u) != 0 ||
        (header[7] & 0x0Cu) == 0x08u ||
        ((header[6] >> 4u) | (header[7] & 0xF0u)) != 0u ||
        header[4] != 1u) {
        fclose(file);
        return 0;
    }
    offset = 16L + ((header[6] & 0x04u) != 0u ? 512L : 0L);
    if (fseek(file, offset, SEEK_SET) != 0 ||
        fread(s_prg, 1u, sizeof(s_prg), file) != sizeof(s_prg)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    s_ready = 1;
    return 1;
}
int nesMenuCpuAvailable(void) { return s_ready; }
int nesMenuCpuInput(uint8_t buttonPress, uint8_t count,
                    uint8_t *selection, uint8_t *repeatGuard,
                    uint8_t *timer30, uint8_t *timer31,
                    uint8_t *ram00FF, uint8_t *actionX)
{
    NesMachine machine;
    int result;
    if (!s_ready || selection == NULL || repeatGuard == NULL ||
        timer30 == NULL || timer31 == NULL || ram00FF == NULL || actionX == NULL) return 0;
    memset(&machine, 0, sizeof(machine));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    machine.a = count;
    machine.ram[0x0015u] = buttonPress;
    machine.ram[0x0030u] = *timer30;
    machine.ram[0x0031u] = *timer31;
    machine.ram[0x0042u] = *selection;
    machine.ram[0x0049u] = *repeatGuard;
    machine.ram[0x00FFu] = *ram00FF;
    result = nesCall(&machine, 0xC7DEu, 4096L);
    if (result != 0) return 0;
    *selection = machine.ram[0x0042u];
    *repeatGuard = machine.ram[0x0049u];
    *timer30 = machine.ram[0x0030u];
    *timer31 = machine.ram[0x0031u];
    *ram00FF = machine.ram[0x00FFu];
    *actionX = machine.x;
    return 1;
}

/*
 * Execute a source setup-list routine against the original 6502 PRG.
 * This deliberately exposes only the RAM bytes currently represented by
 * GameState; callers must not use it for routines with unmodelled side effects.
 */
int nesMenuCpuSetupCommand(uint16_t routineAddress, uint8_t asmState,
                           uint8_t course, uint8_t *ram0045,
                           uint8_t ram004D_EF[0xA3])
{
    NesMachine machine;
    unsigned int i;
    int result;
    if (!s_ready || ram0045 == NULL || ram004D_EF == NULL) return 0;
    if (routineAddress != 0xC551u && routineAddress != 0xC869u) return 0;

    memset(&machine, 0, sizeof(machine));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    machine.ram[0x0041u] = asmState;
    machine.ram[0x0043u] = course;
    machine.ram[0x0045u] = *ram0045;
    for (i = 0u; i < 0xA3u; ++i)
        machine.ram[0x004Du + i] = ram004D_EF[i];

    result = nesCall(&machine, routineAddress, 8192L);
    if (result != 0) return 0;

    *ram0045 = machine.ram[0x0045u];
    for (i = 0u; i < 0xA3u; ++i)
        ram004D_EF[i] = machine.ram[0x004Du + i];
    return 1;
}

typedef struct {
    uint8_t *nametables, *palette, *ctrl, *mask, *scrollX, *scrollY;
    uint16_t address;
    uint8_t addressHigh, addressLatch, scrollLatch, paletteWritten;
} NesMenuPpuBus;

static uint8_t nesMenuPaletteIndex(uint16_t address)
{
    uint8_t index = (uint8_t)(address & 0x1Fu);
    if (index == 0x10u || index == 0x14u || index == 0x18u || index == 0x1Cu)
        index = (uint8_t)(index - 0x10u);
    return index;
}

static uint8_t nesMenuIoRead(NesMachine *machine, uint16_t address)
{
    NesMenuPpuBus *bus = (NesMenuPpuBus *)machine->user;
    if (address == 0x2002u) {
        bus->addressLatch = 0u;
        bus->scrollLatch = 0u;
    }
    return 0u;
}

static void nesMenuIoWrite(NesMachine *machine, uint16_t address, uint8_t value)
{
    NesMenuPpuBus *bus = (NesMenuPpuBus *)machine->user;
    if (address == 0x2000u) *bus->ctrl = value;
    else if (address == 0x2001u) *bus->mask = value;
    else if (address == 0x2005u) {
        if (bus->scrollLatch == 0u) *bus->scrollX = value;
        else *bus->scrollY = value;
        bus->scrollLatch ^= 1u;
    } else if (address == 0x2006u) {
        if (bus->addressLatch == 0u) {
            bus->addressHigh = (uint8_t)(value & 0x3Fu);
            bus->addressLatch = 1u;
        } else {
            bus->address = (uint16_t)(((uint16_t)bus->addressHigh << 8) | value);
            bus->addressLatch = 0u;
        }
    } else if (address == 0x2007u) {
        uint16_t target = (uint16_t)(bus->address & 0x3FFFu);
        if (target >= 0x3000u && target < 0x3F00u)
            target = (uint16_t)(target - 0x1000u);
        if (target >= 0x2000u && target < 0x3000u)
            bus->nametables[(target - 0x2000u) & 0x07FFu] = value;
        else if (target >= 0x3F00u) {
            bus->palette[nesMenuPaletteIndex(target)] = value;
            bus->paletteWritten = 1u;
        }
        bus->address = (uint16_t)(bus->address +
            (((*bus->ctrl & 0x04u) != 0u) ? 32u : 1u));
    }
}

int nesMenuCpuEditorInit(uint8_t cpuRam[0x800], uint8_t nametables[2048],
                         uint8_t palette[32], uint8_t *ppuCtrl,
                         uint8_t *ppuMask, uint8_t *scrollX,
                         uint8_t *scrollY, uint8_t *paletteWritten)
{
    NesMachine machine;
    NesMenuPpuBus bus;
    int result;
    if (!s_ready || cpuRam == NULL || nametables == NULL || palette == NULL ||
        ppuCtrl == NULL || ppuMask == NULL || scrollX == NULL || scrollY == NULL ||
        paletteWritten == NULL)
        return 0;
    memset(&machine, 0, sizeof(machine));
    memset(&bus, 0, sizeof(bus));
    memcpy(machine.ram, cpuRam, sizeof(machine.ram));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    bus.nametables = nametables; bus.palette = palette;
    bus.ctrl = ppuCtrl; bus.mask = ppuMask;
    bus.scrollX = scrollX; bus.scrollY = scrollY;
    machine.user = &bus;
    machine.io_read = nesMenuIoRead;
    machine.io_write = nesMenuIoWrite;
    result = nesCall(&machine, 0xC820u, 50000L);
    if (result != 0) return 0;
    memcpy(cpuRam, machine.ram, sizeof(machine.ram));
    *paletteWritten = bus.paletteWritten;
    return 1;
}

/* Execute only audited menu routines whose transitive effects stay in CPU RAM/OAM/PPU command buffers. */
int nesMenuCpuRunRamRoutine(uint16_t routineAddress, uint8_t cpuRam[0x800])
{
    NesMachine machine;
    int result;
    if (!s_ready || cpuRam == NULL) return 0;
    if (routineAddress != 0xC5A3u && routineAddress != 0xC562u &&
        routineAddress != 0xC5C2u && routineAddress != 0xC465u) return 0;
    memset(&machine, 0, sizeof(machine));
    memcpy(machine.ram, cpuRam, sizeof(machine.ram));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    result = nesCall(&machine, routineAddress, 100000L);
    if (result != 0) return 0;
    memcpy(cpuRam, machine.ram, sizeof(machine.ram));
    return 1;
}
