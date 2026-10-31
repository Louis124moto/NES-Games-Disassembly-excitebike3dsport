#include <string.h>
#include "nes_active_stream.h"

int main(void)
{
    NesActiveStreamInfo info;
    size_t i;

    /* Table indices $01-$11 map to static PPU update lists (array 0..16). */
    if (nesExcitebikeActiveStream(0x01u, &info) == 0 || !info.valid || info.isPalette ||
        info.arrayIndex != 0u || strstr(info.sourceLabel, "D470") == NULL) return 1;
    if (nesExcitebikeActiveStream(0x02u, &info) == 0 || info.isPalette ||
        info.arrayIndex != 1u || strstr(info.sourceLabel, "D64B") == NULL ||
        info.hint == NULL || strcmp(info.hint, "blue background") != 0) return 2;
    if (nesExcitebikeActiveStream(0x11u, &info) == 0 || info.isPalette ||
        info.arrayIndex != 16u || strstr(info.sourceLabel, "D614") == NULL) return 3;

    /* Table indices $12-$18 map to the seven palette command lists (array 0..6). */
    if (nesExcitebikeActiveStream(0x12u, &info) == 0 || !info.valid || !info.isPalette ||
        info.arrayIndex != 0u || strstr(info.sourceLabel, "D3D6") == NULL) return 4;
    if (nesExcitebikeActiveStream(0x17u, &info) == 0 || !info.isPalette ||
        info.arrayIndex != 5u || strstr(info.sourceLabel, "D435") == NULL) return 5;
    if (nesExcitebikeActiveStream(0x18u, &info) == 0 || !info.isPalette ||
        info.arrayIndex != 6u || strstr(info.sourceLabel, "D450") == NULL ||
        info.hint == NULL || strcmp(info.hint, "title screen") != 0) return 6;

    /* Index $19 is the D631 stream (PPU array slot 17). */
    if (nesExcitebikeActiveStream(0x19u, &info) == 0 || info.isPalette ||
        info.arrayIndex != 17u || strstr(info.sourceLabel, "D631") == NULL) return 7;

    /* Uncaptured indices do not resolve. */
    if (nesExcitebikeActiveStream(0x00u, &info) != 0) return 8;
    if (nesExcitebikeActiveStream(0x1Au, &info) != 0) return 9;
    if (nesExcitebikeActiveStream(0xFFu, &info) != 0) return 10;
    if (nesExcitebikeActiveStream(0x02u, NULL) != 0) return 11;

    /* Source-derived setup-write table: 18 rows, all resolving to captured streams. */
    if (nesExcitebikeRam0045SetupWriteCount() != 18u) return 12;
    for (i = 0u; i < nesExcitebikeRam0045SetupWriteCount(); ++i) {
        const NesRam0045SetupWrite *write = nesExcitebikeRam0045SetupWrite(i);
        if (write == NULL || nesExcitebikeActiveStream(write->ram0045, &info) == 0) return 13;
    }
    if (nesExcitebikeRam0045SetupWrite(nesExcitebikeRam0045SetupWriteCount()) != NULL) return 14;
    {
        const NesRam0045SetupWrite *write;
        write = nesExcitebikeRam0045SetupWrite(0u);
        if (write == NULL || write->asmState != 0u || write->commandIndex != 0u ||
            write->ram0045 != 0x02u) return 15;
        write = nesExcitebikeRam0045SetupWrite(1u);
        if (write == NULL || write->asmState != 0u || write->commandIndex != 1u ||
            write->ram0045 != 0x18u) return 16;
        write = nesExcitebikeRam0045SetupWrite(5u);
        if (write == NULL || write->asmState != 2u || write->commandIndex != 0u ||
            write->ram0045 != 0x12u) return 17;
        write = nesExcitebikeRam0045SetupWrite(6u);
        if (write == NULL || write->asmState != 2u || write->commandIndex != 3u ||
            write->ram0045 != 0x17u) return 18;
        write = nesExcitebikeRam0045SetupWrite(7u);
        if (write == NULL || write->asmState != 2u || write->commandIndex != 4u ||
            write->ram0045 != 0x0Eu) return 19;
        write = nesExcitebikeRam0045SetupWrite(16u);
        if (write == NULL || write->asmState != 8u || write->commandIndex != 0u ||
            write->ram0045 != 0x11u) return 20;
    }
    return 0;
}
