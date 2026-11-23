#include "race_tail.h"
#include "nes_routines.h"

static const uint8_t k_tblD8F3[4] = { 0xCFu, 0xD3u, 0xD7u, 0xCFu };
static const uint8_t k_tblD8E5[2] = { 0xE2u, 0xE8u };

static void drawGrid(RaceTailState *s, uint8_t x, uint8_t tile, uint8_t grid, uint8_t attribute, uint8_t marker)
{
    NesSubD1C7OamGridState g;
    g.ram0000_x = x;
    g.ram0001_y = 0x50u;
    g.ram0002_tile = tile;
    g.ram0003_grid = grid;
    g.ram0004_oamBase = 0xC0u;
    g.ram0008_maxIndex = 0u;
    g.ram0009_y = 0u;
    g.ram000A_markerBits = marker;
    g.ram000B_attribute = attribute;
    (void)nesSubD1C7BuildOamGrid(s->oam, &g);
}

/* ofs_004_E0E6_00 */
static void overheadBanner(RaceTailState *s)
{
    uint8_t x;
    if ((s->timer_003A | s->track_finished_0052 | s->timer_0024) != 0u) return;
    if (s->flag_03A0 == 0u) {
        if (s->timer_002E != 0u) return;
        s->tile_03A1 = k_tblD8F3[s->rng_0018 & 3u];
        s->x_03B9 = 0xF8u;
        s->flag_03A0 = 0xF8u;
    }
    s->x_03B9 = (uint8_t)(s->x_03B9 - s->scroll_0060);
    if (s->scroll_0060 != 0u && (s->frame_003F & 0x0Fu) == 0u) s->x_03B9 = (uint8_t)(s->x_03B9 - 1u);
    x = s->x_03B9;
    if (x >= 0xFCu) {
        s->timer_002E = (uint8_t)(s->rng_0018 & 0xA0u);
        s->flag_03A0 = 0u;
        return;
    }
    drawGrid(s, x, s->tile_03A1, 0x22u, 0x20u, 0x20u);
    if (s->x_03B9 >= 0x60u) return;
    nesSubE16CWriteSpriteStrip(s->oam, 0xC0u, (uint8_t)(s->tile_03A1 + 3u));
    nesSubE16CWriteSpriteStrip(s->oam, 0xC8u, (uint8_t)(s->tile_03A1 + 1u));
}

/* ofs_004_E2AF_01 */
static void finishSign(RaceTailState *s)
{
    const uint8_t previous = s->scroll_03BD;
    if (s->blink_03BB > 1u) { s->unsupported = 1u; return; }
    s->scroll_03BD = (uint8_t)(previous - s->scroll_0060);
    if (previous < s->scroll_0060) s->scroll_03BE = (uint8_t)(s->scroll_03BE - 1u);
    if (s->track_finished_0052 != 0u) {
        if (s->track_finished_0052 != 1u) return;
        if (s->timer_0020 == 0u) s->blink_03BB ^= 1u;
    }
    if ((uint8_t)(s->scroll_03BE - 1u) != 0u) return;
    drawGrid(s, s->scroll_03BD, k_tblD8E5[s->blink_03BB], 0x32u, 0x00u, 0x00u);
}

void raceFrameTail(RaceTailState *s)
{
    s->unsupported = 0u;
    if (s->timer_003C != 0u) return;
    if (s->state_03BC == 0u) overheadBanner(s);
    else if (s->state_03BC == 1u) finishSign(s);
    else s->unsupported = 1u;
}
