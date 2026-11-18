#include "nes_race_setup.h"

static const uint8_t tbl_C0B0[4] = {0x09u, 0x0Bu, 0x0Du, 0x0Fu};
static const uint8_t tbl_C0B4_default_position[4] = {0x0Eu, 0x1Au, 0x26u, 0x32u};
static const uint8_t tbl_C0B8[4] = {0x38u, 0x48u, 0x58u, 0x68u};

void nesRaceSetupC551(NesRaceRam *r)
{
    uint8_t a = 0x12u;
    if (r->zp[NES_ZP_0041] < 0x08u) a = (uint8_t)(a + r->zp[NES_ZP_0043]);
    r->zp[NES_ZP_0045] = a;
}

void nesRaceSetupC869(NesRaceRam *r)
{
    unsigned int x;
    for (x = 0xEFu; x != 0x4Cu; --x) r->zp[x] = 0u;
}

void nesRaceSetupC820AfterD13F(NesRaceRam *r)
{
    unsigned int i;
    r->zp[1] = 0x03u;
    r->zp[NES_ZP_0045] = 0x01u;
    r->zp[0] = 0x00u;
    r->zp[NES_ZP_004B] = 0x00u;
    /* STA (ram_0000),Y with pointer $0300, Y = $EF down to $40. */
    for (i = 0xEFu; i >= 0x40u; --i) r->pg03[i] = 0u;
    for (i = 0u; i < 0x80u; ++i) {
        r->pg04[i] = 0x3Bu;
        r->pg04[0x80u + i] = 0x3Du;
        r->pg05[i] = 0x3Du;
    }
    r->oam[0] = 0xBFu;
    r->oam[3] = 0xF7u;
    r->oam[1] = 0xFFu;
    r->zp[0xE8] = 0x3Fu;
    r->zp[0xE9] = 0x1Fu;
    r->zp[0xEB] = 0x0Au;
    r->zp[0xEA] = 0x25u;
}

void nesRaceSetupC5A3(NesRaceRam *r) { r->zp[NES_ZP_0045] = 0x17u; }
void nesRaceSetupC566(NesRaceRam *r) { r->zp[NES_ZP_0045] = 0x0Eu; }

void nesRaceSetupC875Head(NesRaceRam *r)
{
    uint8_t x;
    r->zp[NES_ZP_0047]++;
    r->zp[NES_ZP_0040]++;
    if (r->zp[NES_ZP_0034] == 0u) r->zp[NES_ZP_0034] = 0x05u;
    r->zp[0xE8] = 0x34u;
    r->zp[0xE9] = 0x14u;
    r->pg03[0xF1] = 0xA0u;
    r->pg03[0xB4] = 0x80u;
    r->pg03[0xB0] = 0x80u;
    r->pg03[0xBD] = 0xFFu;
    r->pg03[0x70] = 0x01u;
    r->pg03[0xBE] = 0x02u;
    x = (r->pg03[0xF2] != 0u) ? 3u : 0u;
    for (;;) {
        r->zp[(uint8_t)(0xB8u + x)] = tbl_C0B4_default_position[x];
        r->zp[(uint8_t)(0x64u + x)] = 0x08u;
        r->zp[(uint8_t)(0xE0u + x)] = tbl_C0B0[x];
        r->zp[(uint8_t)(0x80u + x)] = tbl_C0B8[x];
        r->zp[(uint8_t)(0xACu + x)] = 0x06u;
        r->zp[(uint8_t)(0x78u + x)] = 0x06u;
        r->zp[(uint8_t)(0x84u + x)] = 0x01u;
        r->zp[(uint8_t)(0xA8u + x)] = 0x01u;
        r->zp[(uint8_t)(0x74u + x)] = (uint8_t)(r->zp[(uint8_t)(0x19u + x)] >> 1);
        if (x == 0u) { x = 0xFFu; break; }   /* DEX; BPL falls through when X wraps to $FF */
        --x;
    }
    /* LDA ram_0019,X with X = $FF: zero-page indexing wraps to $0018. */
    x = (uint8_t)(r->zp[(uint8_t)(0x19u + x)] & 0x03u);
    if (x == 0u) x = 0x02u;
    r->zp[(uint8_t)(0x78u + x)] >>= 1;
}
