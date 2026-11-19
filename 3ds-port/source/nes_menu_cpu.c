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
    unsigned long spriteHitCycle;
    unsigned long spriteHitClearCycle;
    uint8_t captureRasterSplit, timingOnly, ctrlWriteCount, scrollPairCount;
    uint8_t topCtrl, topScrollX, topScrollY;
    uint8_t bottomCtrl, bottomScrollX, bottomScrollY;
} NesMenuPpuBus;

#define NES_NMI_TO_PRERENDER_CPU_CYCLES 2273UL

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
        /* Bit 6 remains set from the previous frame until pre-render clears it,
         * then rises only at the computed opaque sprite-0/background overlap. */
        return (machine->cyc < bus->spriteHitClearCycle ||
                machine->cyc >= bus->spriteHitCycle) ? 0x40u : 0u;
    }
    return 0u;
}

static void nesMenuIoWrite(NesMachine *machine, uint16_t address, uint8_t value)
{
    NesMenuPpuBus *bus = (NesMenuPpuBus *)machine->user;
    if (address == 0x2000u) {
        *bus->ctrl = value;
        if (bus->captureRasterSplit) {
            if (bus->ctrlWriteCount == 0u) bus->topCtrl = value;
            else bus->bottomCtrl = value;
            if (bus->ctrlWriteCount < 2u) ++bus->ctrlWriteCount;
        }
    } else if (address == 0x2001u) *bus->mask = value;
    else if (address == 0x2005u) {
        if (bus->scrollLatch == 0u) {
            *bus->scrollX = value;
            if (bus->captureRasterSplit) {
                if (bus->scrollPairCount == 0u) bus->topScrollX = value;
                else bus->bottomScrollX = value;
            }
        } else {
            *bus->scrollY = value;
            if (bus->captureRasterSplit) {
                if (bus->scrollPairCount == 0u) bus->topScrollY = value;
                else bus->bottomScrollY = value;
                if (bus->scrollPairCount < 2u) ++bus->scrollPairCount;
            }
        }
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
        if (!bus->timingOnly) {
            if (target >= 0x3000u && target < 0x3F00u)
                target = (uint16_t)(target - 0x1000u);
            if (target >= 0x2000u && target < 0x3000u)
                bus->nametables[(target - 0x2000u) & 0x07FFu] = value;
            else if (target >= 0x3F00u) {
                bus->palette[nesMenuPaletteIndex(target)] = value;
                bus->paletteWritten = 1u;
            }
        }
        bus->address = (uint16_t)(bus->address +
            (((*bus->ctrl & 0x04u) != 0u) ? 32u : 1u));
    } else if (address == 0x4014u) {
        /* OAM DMA stalls the 6502 for 513 or 514 cycles depending on parity. */
        machine->cyc += 513UL + (machine->cyc & 1UL);
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

/*
 * vec_C23B_NMI calls sub_D14E when ram_0047 is nonzero. The bus preserves a
 * previous-frame sprite-0 hit until the modeled pre-render clear point, then
 * returns the elapsed CPU cycles so the caller can align the following C97C
 * wait to the current PPU phase. The delay loop is source-derived (6431 cycles
 * in state 9); the pre-render boundary and unmodeled NMI prelude remain timing
 * approximations.
 *
 * C97C receives a pixel-derived sprite-0 hit threshold computed from decoded
 * CHR, current OAM, the rendered background-opacity map, PPUMASK and PPUCTRL.
 * UINT32_MAX means no opaque overlap exists, so the original D19B wait times out
 * in the bounded host adapter rather than receiving a fabricated hit.
 */
int nesMenuCpuRasterSplit(uint8_t cpuRam[0x800], uint8_t nametables[2048],
                          uint8_t palette[32], uint8_t *ppuCtrl,
                          uint8_t *ppuMask, uint8_t *scrollX, uint8_t *scrollY,
                          uint8_t previousFrameHadSprite0Hit, NesMenuRasterSplit *split)
{
    NesMachine machine;
    NesMenuPpuBus bus;
    int result;
    if (!s_ready || cpuRam == NULL || nametables == NULL || palette == NULL ||
        ppuCtrl == NULL || ppuMask == NULL || scrollX == NULL || scrollY == NULL)
        return 0;
    memset(&machine, 0, sizeof(machine));
    memset(&bus, 0, sizeof(bus));
    memcpy(machine.ram, cpuRam, sizeof(machine.ram));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    bus.nametables = nametables; bus.palette = palette;
    bus.ctrl = ppuCtrl; bus.mask = ppuMask;
    bus.scrollX = scrollX; bus.scrollY = scrollY;
    bus.spriteHitClearCycle = previousFrameHadSprite0Hit
        ? NES_NMI_TO_PRERENDER_CPU_CYCLES : 0UL;
    bus.spriteHitCycle = ~0UL; /* no new-frame hit while the NMI upload/raster path executes */
    bus.timingOnly = 1u;       /* native PPU parser applies data; this pass measures source CPU timing */
    machine.user = &bus;
    machine.io_read = nesMenuIoRead;
    machine.io_write = nesMenuIoWrite;

    /*
     * Reproduce vec_C23B_NMI's source order for cycle accounting:
     * hardware NMI entry; JSR C32F; conditional $2001 update; OAM DMA;
     * JSR C27F; then D0AB/D14E. PPU data writes are ignored here because
     * menusNmiUpload's native parser applies the same stream to PPU memory.
     */
    machine.cyc += 7UL; /* 6502 NMI entry latency */
    machine.cyc += 6UL; /* JSR sub_C32F */
    result = nesCall(&machine, 0xC32Fu, 50000L);
    if (result != 0) return 0;
    if (machine.ram[0x0045u] == 0u) {
        const uint8_t newMask = (uint8_t)(machine.ram[0x0011u] | 0x1Au);
        machine.cyc += 3UL + 2UL + 3UL + 2UL + 4UL + 3UL; /* LDA/BNE/LDA/ORA/STA/STA */
        nesMenuIoWrite(&machine, 0x2001u, newMask);
        machine.ram[0x0011u] = newMask;
    } else {
        machine.cyc += 3UL + 3UL; /* LDA ram_0045 / taken BNE */
    }
    machine.cyc += 2UL + 4UL + 2UL + 4UL; /* LDA/STA $2003, LDA/STA $4014 */
    nesMenuIoWrite(&machine, 0x2003u, 0u);
    nesMenuIoWrite(&machine, 0x4014u, 2u); /* callback adds 513/514 DMA cycles */
    machine.cyc += 6UL; /* JSR sub_C27F */
    result = nesCall(&machine, 0xC27Fu, 100000L);
    if (result != 0) return 0;

    machine.cyc += 3UL + 2UL; /* LDA ram_0047 / BEQ not taken */
    if (machine.ram[0x0047u] != 0u) {
        bus.captureRasterSplit = 1u;
        machine.cyc += 6UL; /* JSR sub_D0AB */
        result = nesCall(&machine, 0xD0ABu, 100000L);
        if (result != 0) return 0;
        machine.cyc += 6UL; /* JSR sub_D14E */
        result = nesCall(&machine, 0xD14Eu, 100000L);
        if (result != 0) return 0;
    } else {
        return 0; /* this adapter is only used for the editor raster path */
    }
    if (split == NULL) return 0;
    split->topCtrl = bus.topCtrl;
    split->topScrollX = bus.topScrollX;
    split->topScrollY = bus.topScrollY;
    split->bottomCtrl = bus.bottomCtrl;
    split->bottomScrollX = bus.bottomScrollX;
    split->bottomScrollY = bus.bottomScrollY;
    split->ctrlWrites = bus.ctrlWriteCount;
    split->scrollPairs = bus.scrollPairCount;
    split->elapsedCycles = machine.cyc > UINT32_MAX ? UINT32_MAX : (uint32_t)machine.cyc;
    {
        const uint32_t phase = split->elapsedCycles > NES_NMI_TO_PRERENDER_CPU_CYCLES
            ? split->elapsedCycles - NES_NMI_TO_PRERENDER_CPU_CYCLES : 0u;
        const uint64_t dots = (uint64_t)phase * 3u;
        uint64_t scanline = dots / 341u;
        if (scanline > 239u) scanline = 239u;
        split->splitScanline = (uint8_t)scanline;
        split->splitDot = (uint16_t)(dots % 341u);
    }
    if (bus.ctrlWriteCount < 2u || bus.scrollPairCount < 2u) return 0;
    memcpy(cpuRam, machine.ram, sizeof(machine.ram));
    return 1;
}

int nesMenuCpuEditorFrame(uint8_t cpuRam[0x800], uint8_t heldButtons,
                          uint8_t pressedButtons, uint8_t nametables[2048],
                          uint8_t palette[32], uint8_t *ppuCtrl,
                          uint8_t *ppuMask, uint8_t *scrollX, uint8_t *scrollY,
                          uint32_t sprite0HitCycleFromCallStart)
{
    NesMachine machine;
    NesMenuPpuBus bus;
    int result;
    if (!s_ready || cpuRam == NULL || nametables == NULL || palette == NULL ||
        ppuCtrl == NULL || ppuMask == NULL || scrollX == NULL || scrollY == NULL)
        return 0;
    memset(&machine, 0, sizeof(machine));
    memset(&bus, 0, sizeof(bus));
    memcpy(machine.ram, cpuRam, sizeof(machine.ram));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    machine.ram[0x0014u] = heldButtons;
    machine.ram[0x0015u] = pressedButtons;
    bus.nametables = nametables; bus.palette = palette;
    bus.ctrl = ppuCtrl; bus.mask = ppuMask;
    bus.scrollX = scrollX; bus.scrollY = scrollY;
    bus.spriteHitClearCycle = 0UL;
    bus.spriteHitCycle = (unsigned long)sprite0HitCycleFromCallStart;
    machine.user = &bus;
    machine.io_read = nesMenuIoRead;
    machine.io_write = nesMenuIoWrite;
    result = nesCall(&machine, 0xC97Cu, 1000000L);
    if (result != 0) return 0;
    memcpy(cpuRam, machine.ram, sizeof(machine.ram));
    return 1;
}

/* Execute only audited menu routines whose transitive effects stay in CPU RAM/OAM/PPU command buffers. */
int nesMenuCpuRunRamRoutine(uint16_t routineAddress, uint8_t cpuRam[0x800])
{
    NesMachine machine;
    int result;
    if (!s_ready || cpuRam == NULL) return 0;
    if (routineAddress != 0xC5A3u && routineAddress != 0xC562u &&
        routineAddress != 0xC5C2u && routineAddress != 0xC465u &&
        routineAddress != 0xC55Eu && routineAddress != 0xC56Au &&
        routineAddress != 0xC455u && routineAddress != 0xC5A7u &&
        routineAddress != 0xC485u && routineAddress != 0xCB2Cu &&
        routineAddress != 0xD310u) return 0;
    memset(&machine, 0, sizeof(machine));
    memcpy(machine.ram, cpuRam, sizeof(machine.ram));
    machine.prg = s_prg;
    machine.sp = 0xFDu;
    result = nesCall(&machine, routineAddress, 100000L);
    if (result != 0) return 0;
    memcpy(cpuRam, machine.ram, sizeof(machine.ram));
    return 1;
}
