#include <stdio.h>
#include <string.h>
#include "nes_race_setup.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

int main(void)
{
    static NesRaceRam r;
    unsigned int i;

    memset(&r, 0, sizeof(r));
    r.zp[NES_ZP_0041] = 2u; r.zp[NES_ZP_0043] = 3u;
    nesRaceSetupC551(&r); CHECK(r.zp[NES_ZP_0045] == 0x15u, 1);
    r.zp[NES_ZP_0041] = 8u;
    nesRaceSetupC551(&r); CHECK(r.zp[NES_ZP_0045] == 0x12u, 2);
    r.zp[NES_ZP_0041] = 7u; r.zp[NES_ZP_0043] = 0u;
    nesRaceSetupC551(&r); CHECK(r.zp[NES_ZP_0045] == 0x12u, 3);

    memset(r.zp, 0xAA, sizeof(r.zp));
    nesRaceSetupC869(&r);
    for (i = 0; i < 256u; ++i) {
        const uint8_t e = (i >= 0x4Du && i <= 0xEFu) ? 0u : 0xAAu;
        CHECK(r.zp[i] == e, 10);
    }

    memset(&r, 0xAA, sizeof(r));
    nesRaceSetupC820AfterD13F(&r);
    CHECK(r.zp[1] == 3u && r.zp[0] == 0u && r.zp[NES_ZP_0045] == 1u && r.zp[NES_ZP_004B] == 0u, 20);
    for (i = 0; i < 256u; ++i) CHECK(r.pg03[i] == ((i >= 0x40u && i <= 0xEFu) ? 0u : 0xAAu), 21);
    for (i = 0; i < 0x80u; ++i) {
        CHECK(r.pg04[i] == 0x3Bu && r.pg04[0x80u + i] == 0x3Du && r.pg05[i] == 0x3Du, 22);
    }
    CHECK(r.pg05[0x80] == 0xAAu && r.pg04[0x80 - 1] == 0x3Bu, 23);
    CHECK(r.oam[0] == 0xBFu && r.oam[1] == 0xFFu && r.oam[3] == 0xF7u && r.oam[2] == 0xAAu, 24);
    CHECK(r.zp[0xE8] == 0x3Fu && r.zp[0xE9] == 0x1Fu && r.zp[0xEB] == 0x0Au && r.zp[0xEA] == 0x25u, 25);

    nesRaceSetupC5A3(&r); CHECK(r.zp[NES_ZP_0045] == 0x17u, 30);
    nesRaceSetupC566(&r); CHECK(r.zp[NES_ZP_0045] == 0x0Eu, 31);

    /* Command 6 head, two-player-capable path (ram_03F2 != 0 -> objects 3..0). */
    memset(&r, 0, sizeof(r));
    r.pg03[0xF2] = 1u;
    r.zp[0x19] = 0x10u; r.zp[0x1A] = 0x21u; r.zp[0x1B] = 0x32u; r.zp[0x1C] = 0x43u;
    r.zp[0x18] = 0x01u;                       /* read through the X=$FF wrap */
    r.zp[NES_ZP_0047] = 5u; r.zp[NES_ZP_0040] = 9u;
    nesRaceSetupC875Head(&r);
    CHECK(r.zp[NES_ZP_0047] == 6u && r.zp[NES_ZP_0040] == 10u && r.zp[NES_ZP_0034] == 5u, 40);
    CHECK(r.zp[0xE8] == 0x34u && r.zp[0xE9] == 0x14u && r.pg03[0xF1] == 0xA0u, 41);
    CHECK(r.pg03[0xB4] == 0x80u && r.pg03[0xB0] == 0x80u && r.pg03[0xBD] == 0xFFu, 42);
    CHECK(r.pg03[0x70] == 1u && r.pg03[0xBE] == 2u, 43);
    CHECK(r.zp[0xB8] == 0x0Eu && r.zp[0xB9] == 0x1Au && r.zp[0xBA] == 0x26u && r.zp[0xBB] == 0x32u, 44);
    CHECK(r.zp[0x64] == 8u && r.zp[0x67] == 8u && r.zp[0xAC] == 6u && r.zp[0xAF] == 6u, 45);
    CHECK(r.zp[0xE0] == 0x09u && r.zp[0xE3] == 0x0Fu && r.zp[0x80] == 0x38u && r.zp[0x83] == 0x68u, 46);
    CHECK(r.zp[0x84] == 1u && r.zp[0xA8] == 1u && r.zp[0x87] == 1u && r.zp[0xAB] == 1u, 47);
    CHECK(r.zp[0x74] == 0x08u && r.zp[0x75] == 0x10u && r.zp[0x76] == 0x19u && r.zp[0x77] == 0x21u, 48);
    /* ram_0019+$FF = $0018 = 1 -> &3 = 1 -> LSR ram_0079: 6 >> 1 = 3; others stay 6. */
    CHECK(r.zp[0x78] == 6u && r.zp[0x79] == 3u && r.zp[0x7A] == 6u && r.zp[0x7B] == 6u, 49);

    /* ram_03F2 == 0: only object 0 initialised; timer 0034 kept when non-zero. */
    memset(&r, 0, sizeof(r));
    r.zp[NES_ZP_0034] = 9u; r.zp[0x18] = 0x00u;   /* &3 = 0 -> index 2 */
    nesRaceSetupC875Head(&r);
    CHECK(r.zp[NES_ZP_0034] == 9u, 50);
    CHECK(r.zp[0xB8] == 0x0Eu && r.zp[0xB9] == 0u && r.zp[0x64] == 8u && r.zp[0x65] == 0u, 51);
    CHECK(r.zp[0x78] == 6u && r.zp[0x7A] == 0u, 52);  /* 0078+2 was 0 -> stays 0 */

    puts("PASS: race setup list 2 commands 1,2,3(tail),4,5,6(head).");
    return 0;
}
