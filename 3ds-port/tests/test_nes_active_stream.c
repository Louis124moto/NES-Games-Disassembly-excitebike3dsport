#include <stdio.h>
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

    /*
     * tbl_C0F6_lo / tbl_C111_hi transcription: for every static index $01-$19 the
     * combined CPU address must appear in the independently named source label.
     */
    for (i = 1u; i <= 0x19u; ++i) {
        char hex[8];
        const NesRam0045Target target = nesExcitebikeRam0045Target((uint8_t)i);
        if (target.kind != NES_RAM0045_TARGET_STATIC_LIST) return 21;
        if (nesExcitebikeActiveStream((uint8_t)i, &info) == 0) return 22;
        (void)snprintf(hex, sizeof(hex), "%04X", (unsigned int)target.cpuAddress);
        if (strstr(info.sourceLabel, hex) == NULL) return 23;
    }
    {
        NesRam0045Target target = nesExcitebikeRam0045Target(0x00u);
        if (target.kind != NES_RAM0045_TARGET_DYNAMIC_BUFFER || target.cpuAddress != 0x0301u) return 24;
        target = nesExcitebikeRam0045Target(0x1Au);
        if (target.kind != NES_RAM0045_TARGET_ZERO_PAGE_LIST || target.cpuAddress != 0x00A1u) return 25;
        target = nesExcitebikeRam0045Target(0x1Bu);
        if (target.kind != NES_RAM0045_TARGET_OUT_OF_TABLE || target.cpuAddress != 0u) return 26;
        target = nesExcitebikeRam0045Target(0xFFu);
        if (target.kind != NES_RAM0045_TARGET_OUT_OF_TABLE) return 27;
        /* Spot checks against the table bytes: $18 = "title screen" palette list D450. */
        target = nesExcitebikeRam0045Target(0x18u);
        if (target.cpuAddress != 0xD450u) return 28;
    }

    /* vec_C23B_NMI: only ram_0045 == 0 enables rendering through $2001. */
    {
        int wrote = -1;
        if (nesExcitebikeNmiRamFor2001(0x00u, 0x00u, &wrote) != 0x1Au || wrote != 1) return 29;
        if (nesExcitebikeNmiRamFor2001(0x00u, 0x01u, &wrote) != 0x1Bu || wrote != 1) return 30;
        if (nesExcitebikeNmiRamFor2001(0x00u, 0x1Au, &wrote) != 0x1Au || wrote != 1) return 31;
        if (nesExcitebikeNmiRamFor2001(0x02u, 0x01u, &wrote) != 0x01u || wrote != 0) return 32;
        if (nesExcitebikeNmiRamFor2001(0x18u, 0x00u, &wrote) != 0x00u || wrote != 0) return 33;
        if (nesExcitebikeNmiRamFor2001(0x00u, 0x00u, NULL) != 0x1Au) return 34;
    }

    /* sub_C27F tail: the upload clears ram_0300, ram_0301 and ram_0045. */
    {
        uint8_t r45 = 0x1Au, r300 = 0x44u, r301 = 0x55u;
        nesExcitebikeNmiUploadAck(&r45, &r300, &r301);
        if (r45 != 0u || r300 != 0u || r301 != 0u) return 35;
        r45 = 0x02u;
        nesExcitebikeNmiUploadAck(&r45, NULL, NULL);
        if (r45 != 0u) return 36;
        nesExcitebikeNmiUploadAck(NULL, NULL, NULL);
    }
    return 0;
}
