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

    /*
     * sub_C27F model on the menu PPU: expectations below were hand-parsed from the
     * list bytes (record = addr-hi, addr-lo, control, data; bit 6 repeats one byte).
     */
    {
        uint8_t nt[NES_MENU_NAMETABLE_BYTES], pal[NES_MENU_PALETTE_BYTES];
        uint8_t nt0[NES_MENU_NAMETABLE_BYTES], pal0[NES_MENU_PALETTE_BYTES];
        uint8_t r45;
        size_t k;
        memset(nt, 0, sizeof(nt)); memset(pal, 0, sizeof(pal));
        nt[0x100] = 0x55u; pal[30] = 0x44u;

        /* $02 "blue background": 23 E3 4B AA / 23 F2 44 FF / 20 83 5A 27 ... */
        r45 = 0x02u;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_APPLIED) return 40;
        if (r45 != 0u) return 41;
        for (k = 0u; k < 0x0Bu; ++k) if (nt[0x3E3u + k] != 0xAAu) return 42;
        if (nt[0x3E2u] != 0u || nt[0x3EEu] != 0u) return 43;
        for (k = 0u; k < 4u; ++k) if (nt[0x3F2u + k] != 0xFFu) return 44;
        for (k = 0u; k < 0x1Au; ++k) if (nt[0x083u + k] != 0x27u) return 45;
        if (nt[0x082u] != 0u || nt[0x09Du] != 0u) return 46;
        if (nt[0x100] != 0x55u) return 47;          /* cumulative: untouched byte survives */
        if (pal[30] != 0x44u) return 48;

        /* $18 "title screen" palette list D450: 28 bytes from $3F00; $3F10.. mirror $3F00.. */
        r45 = 0x18u;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_APPLIED || r45 != 0u) return 49;
        if (pal[0] != 0x02u || pal[1] != 0x0Fu || pal[2] != 0x30u || pal[3] != 0x21u) return 50;
        if (pal[16] != pal[0] || pal[20] != pal[4] || pal[24] != pal[8]) return 51; /* mirrors */
        if (pal[17] != 0x20u || pal[18] != 0x16u || pal[19] != 0x0Fu) return 52;
        if (pal[21] != 0x27u || pal[22] != 0x13u || pal[23] != 0x3Cu) return 53;
        if (pal[30] != 0x44u) return 54;            /* beyond the 28 written bytes */
        if (nt[0x3E3u] != 0xAAu) return 55;         /* earlier nametable upload preserved */

        /* Not modelled: index 0, $1A and out-of-table values leave everything untouched. */
        memcpy(nt0, nt, sizeof(nt)); memcpy(pal0, pal, sizeof(pal));
        r45 = 0x00u;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_NONE || r45 != 0u) return 56;
        r45 = 0x1Au;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_UNSUPPORTED || r45 != 0x1Au) return 57;
        r45 = 0x1Bu;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_UNSUPPORTED || r45 != 0x1Bu) return 58;
        r45 = 0xFFu;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_UNSUPPORTED || r45 != 0xFFu) return 59;
        if (memcmp(nt, nt0, sizeof(nt)) != 0 || memcmp(pal, pal0, sizeof(pal)) != 0) return 60;
        if (nesExcitebikeNmiUploadStatic(NULL, nt, pal) != NES_NMI_UPLOAD_UNSUPPORTED) return 61;
        r45 = 0x02u;
        if (nesExcitebikeNmiUploadStatic(&r45, NULL, pal) != NES_NMI_UPLOAD_UNSUPPORTED || r45 != 0x02u) return 62;
        if (nesExcitebikeNmiUploadStatic(&r45, nt, NULL) != NES_NMI_UPLOAD_UNSUPPORTED || r45 != 0x02u) return 63;

        /* Every static index $01-$19 executes and is acknowledged. */
        for (k = 1u; k <= 0x19u; ++k) {
            r45 = (uint8_t)k;
            if (nesExcitebikeNmiUploadStatic(&r45, nt, pal) != NES_NMI_UPLOAD_APPLIED || r45 != 0u) return 64;
        }
    }
    return 0;
}
