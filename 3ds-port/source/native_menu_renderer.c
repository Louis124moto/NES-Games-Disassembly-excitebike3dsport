#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <3ds.h>
#include "native_menu_renderer.h"

/* Excitebike is NROM-128/CHR-ROM 8 KiB. The original writes $90 at C1D5
 * and $10 at C32B-C333 before the first race: PPUCTRL bit 4 selects the
 * $1000-$1FFF pattern table for background tiles (tile indices 256..511).
 * Bit 3 stays clear, so 8x8 OAM sprites continue using indices 0..255.
 */
#define EXCITEBIKE_BG_CHR_TILE_BASE 256u

/* NTSC RGB lookup for source palette indices; no PPU state or renderer is used. */
static const uint8_t s_nativeRgb[64][3] = {
 {84,84,84},{0,30,116},{8,16,144},{48,0,136},{68,0,100},{92,0,48},{84,4,0},{60,24,0},
 {32,42,0},{8,58,0},{0,64,0},{0,60,0},{0,50,60},{0,0,0},{0,0,0},{0,0,0},
 {152,150,152},{8,76,196},{48,50,236},{92,30,228},{136,20,176},{160,20,100},{152,34,32},{120,60,0},
 {84,90,0},{40,114,0},{8,124,0},{0,118,40},{0,102,120},{0,0,0},{0,0,0},{0,0,0},
 {236,238,236},{76,154,236},{120,124,236},{176,98,236},{228,84,236},{236,88,180},{236,106,100},{212,136,32},
 {160,170,0},{116,196,0},{76,208,32},{56,204,108},{56,180,204},{60,60,60},{0,0,0},{0,0,0},
 {236,238,236},{168,204,236},{188,188,236},{212,178,236},{236,174,236},{236,174,212},{236,180,176},{228,196,144},
 {204,210,120},{180,222,120},{168,226,144},{152,226,180},{160,214,228},{160,162,160},{0,0,0},{0,0,0}
};

static size_t framebufferOffset(unsigned int x, unsigned int y)
{
    return (((size_t)x * 240u) + (239u - y)) * 3u;
}

static const NativeMenuSceneAssets *sceneForState(const NativeMenuAssets *assets,
                                                   uint8_t state)
{
    if (state == 2u) return assets->race_scene_available ? &assets->scenes[6] : NULL;
    if (state == 0u) return &assets->scenes[0];
    if (state == 1u) return &assets->scenes[1];
    if (state == 8u) return &assets->scenes[2];
    if (state == 9u) return &assets->scenes[3];
    if (state == 12u) return &assets->scenes[4];
    if (state == 13u) return &assets->scenes[5];
    return NULL;
}

static void putPixel(uint8_t *framebuffer, unsigned int x, unsigned int y,
                     uint8_t color)
{
    uint8_t rgb[3];
    size_t offset;
    if (x >= 400u || y >= 240u || color > 0x3Fu) return;
    rgb[0] = s_nativeRgb[color][0];
    rgb[1] = s_nativeRgb[color][1];
    rgb[2] = s_nativeRgb[color][2];
    offset = framebufferOffset(x, y);
    framebuffer[offset] = rgb[2];
    framebuffer[offset + 1u] = rgb[1];
    framebuffer[offset + 2u] = rgb[0];
}

static void drawBackground(uint8_t *framebuffer, const NativeMenuAssets *assets,
                           const NativeMenuSceneAssets *scene, unsigned paletteSet)
{
    unsigned int tileY, tileX, pixelY, pixelX;
    for (tileY = 0u; tileY < 30u; ++tileY) {
        for (tileX = 0u; tileX < 32u; ++tileX) {
            const size_t mapIndex = (size_t)tileY * 32u + tileX;
            const uint8_t tile = scene->nametable[mapIndex];
            const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)tile;
            const size_t attributeIndex = 960u + (size_t)(tileY / 4u) * 8u + tileX / 4u;
            const uint8_t attribute = scene->nametable[attributeIndex];
            const unsigned int shift = (unsigned int)(((tileY & 2u) << 1u) | (tileX & 2u));
            const unsigned int subpalette = (attribute >> shift) & 3u;
            const uint8_t *tilePixels;
            if (chrTile >= assets->chr_tile_count) continue;
            tilePixels = assets->chr_pixels + chrTile * 64u;
            for (pixelY = 0u; pixelY < 8u; ++pixelY) {
                for (pixelX = 0u; pixelX < 8u; ++pixelX) {
                    const uint8_t value = (uint8_t)(tilePixels[pixelY * 8u + pixelX] & 3u);
                    const size_t paletteIndex = value == 0u ? 0u : (size_t)subpalette * 4u + value;
                    const unsigned int screenX = 72u + tileX * 8u + pixelX;
                    const unsigned int screenY = tileY * 8u + pixelY;
                    if (assets->palette_written[paletteSet][paletteIndex] != 0u) {
                        putPixel(framebuffer, screenX, screenY, assets->palettes[paletteSet][paletteIndex]);
                    }
                }
            }
        }
    }
}

static void drawCursor(uint8_t *framebuffer, const NativeMenuAssets *assets,
                       const NativeMenuState *menu, unsigned paletteSet)
{
    unsigned int x, y;
    const unsigned int left = menu->cursor_sprite_x;
    const unsigned int top = (unsigned int)menu->cursor_sprite_y + 1u;
    const uint8_t attributes = menu->cursor_sprite_attribute;
    const size_t paletteBase = 16u + (size_t)(attributes & 3u) * 4u;
    const uint8_t *pixels;
    if (menu->cursor_sprite_y == 0xF8u || menu->cursor_sprite_tile == 0xF8u ||
        menu->cursor_sprite_x == 0xF8u || menu->cursor_sprite_tile >= assets->chr_tile_count) return;
    pixels = assets->chr_pixels + (size_t)menu->cursor_sprite_tile * 64u;
    for (y = 0u; y < 8u; ++y) {
        const unsigned int sourceY = (attributes & 0x80u) != 0u ? 7u - y : y;
        for (x = 0u; x < 8u; ++x) {
            const unsigned int sourceX = (attributes & 0x40u) != 0u ? 7u - x : x;
            const uint8_t value = (uint8_t)(pixels[sourceY * 8u + sourceX] & 3u);
            const size_t paletteIndex = paletteBase + value;
            if (value == 0u || assets->palette_written[paletteSet][paletteIndex] == 0u) continue;
            putPixel(framebuffer, 72u + left + x, top + y, assets->palettes[paletteSet][paletteIndex]);
        }
    }
}

int nativeMenuRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer;
    const NativeMenuSceneAssets *scene;
    unsigned paletteSet;
    if (menu == NULL) return 0;
    framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width != 240u || height != 400u) return 0;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (assets == NULL || assets->loaded == 0u ||
        assets->chr_pixels == NULL || assets->chr_tile_count == 0u) return 0;
    scene = sceneForState(assets, menu->source_state);
    if (scene == NULL) return 0;
    /* C551 uses palette 12 for state 9 (the >= $08 branch); other
     * existing front-end/tape screens retain the established preset 18. */
    paletteSet = menu->source_state == 9u ? 0u : 6u;
    drawBackground(framebuffer, assets, scene, paletteSet);
    drawCursor(framebuffer, assets, menu, paletteSet);
    return 1;
}

static void drawRaceHud(uint8_t *framebuffer, const NativeMenuAssets *assets,
                        const NativeMenuSceneAssets *scene,
                        const NativeRaceSession *session, unsigned paletteSet)
{
    unsigned int i, x, y;
    const unsigned int tileY = 26u;

    for (i = 0u; i < 7u; ++i) {
        const unsigned int tileX = 3u + i;
        const uint8_t tile = session->setup.hud_glyphs[i];
        const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)tile;
        const size_t attributeIndex =
            960u + (size_t)(tileY / 4u) * 8u + tileX / 4u;
        const uint8_t attribute = scene->nametable[attributeIndex];
        const unsigned int shift =
            (unsigned int)(((tileY & 2u) << 1u) | (tileX & 2u));
        const unsigned int subpalette = (attribute >> shift) & 3u;
        const uint8_t *tilePixels;

        if (chrTile >= assets->chr_tile_count) continue;
        tilePixels = assets->chr_pixels + chrTile * 64u;
        for (y = 0u; y < 8u; ++y) {
            for (x = 0u; x < 8u; ++x) {
                const uint8_t value = (uint8_t)(tilePixels[y * 8u + x] & 3u);
                const size_t paletteIndex =
                    value == 0u ? 0u : (size_t)subpalette * 4u + value;
                if (assets->palette_written[paletteSet][paletteIndex] != 0u) {
                    putPixel(framebuffer, 72u + tileX * 8u + x,
                             tileY * 8u + y,
                             assets->palettes[paletteSet][paletteIndex]);
                }
            }
        }
    }
}


/* C5A7/CB77 score-time PPU writes: seven source CHR glyphs at $21F2
 * (stored course record) and $2232 (just-finished race time). */
static void drawResultGlyphRow(uint8_t *framebuffer,
                               const NativeMenuAssets *assets,
                               const NativeMenuSceneAssets *scene,
                               const uint8_t glyphs[7],
                               unsigned int tileY,
                               unsigned int tileX,
                               unsigned int paletteSet)
{
    unsigned int i, px, py;
    for (i = 0u; i < 7u; ++i) {
        const unsigned int column = tileX + i;
        const uint8_t attribute = scene->nametable[
            960u + (size_t)(tileY / 4u) * 8u + column / 4u];
        const unsigned int shift =
            (unsigned int)(((tileY & 2u) << 1u) | (column & 2u));
        const unsigned int subpalette = (attribute >> shift) & 3u;
        const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)glyphs[i];
        const uint8_t *pixels;
        if (chrTile >= assets->chr_tile_count) continue;
        pixels = assets->chr_pixels + chrTile * 64u;
        for (py = 0u; py < 8u; ++py) {
            for (px = 0u; px < 8u; ++px) {
                const uint8_t value = (uint8_t)(pixels[py * 8u + px] & 3u);
                const size_t paletteIndex =
                    value == 0u ? 0u : (size_t)subpalette * 4u + value;
                if (value != 0u &&
                    assets->palette_written[paletteSet][paletteIndex] != 0u) {
                    putPixel(framebuffer, 72u + column * 8u + px,
                             tileY * 8u + py,
                             assets->palettes[paletteSet][paletteIndex]);
                }
            }
        }
    }
}

static void drawRaceResultTimes(uint8_t *framebuffer,
                                const NativeMenuAssets *assets,
                                const NativeMenuSceneAssets *scene,
                                const NativeRaceSession *session)
{
    if (session->result_active == 0u) return;
    /* Source tbl_C0EC: index 0 -> $21F2, index 4 -> $2232. */
    drawResultGlyphRow(framebuffer, assets, scene,
                       session->result_record_glyphs, 15u, 18u, 6u);
    drawResultGlyphRow(framebuffer, assets, scene,
                       session->result_current_glyphs, 17u, 18u, 6u);
}

static void drawRaceOam(uint8_t *framebuffer, const NativeMenuAssets *assets,
                          const NativeRaceSession *session, unsigned paletteSet)
{
    int sprite;
    /* NES OAM priority: lower sprite indices are drawn last and appear in front. */
    for (sprite = 63; sprite >= 0; --sprite) {
        const size_t offset = (size_t)sprite * 4u;
        const uint8_t y = session->oam.bytes[offset + 0u];
        const uint8_t tile = session->oam.bytes[offset + 1u];
        const uint8_t attribute = session->oam.bytes[offset + 2u];
        const uint8_t x = session->oam.bytes[offset + 3u];
        const uint8_t *pixels;
        unsigned int py, px;

        if (y >= 0xEFu || tile >= assets->chr_tile_count) continue;
        pixels = assets->chr_pixels + (size_t)tile * 64u;
        for (py = 0u; py < 8u; ++py) {
            const unsigned int sourceY =
                (attribute & 0x80u) != 0u ? 7u - py : py;
            const unsigned int screenY = (unsigned int)y + 1u + py;
            if (screenY >= 240u) continue;
            for (px = 0u; px < 8u; ++px) {
                const unsigned int sourceX =
                    (attribute & 0x40u) != 0u ? 7u - px : px;
                const unsigned int screenX = (unsigned int)x + px;
                const uint8_t value = (uint8_t)(pixels[sourceY * 8u + sourceX] & 3u);
                const size_t paletteIndex =
                    16u + (size_t)(attribute & 3u) * 4u + value;

                if (screenX >= 256u || value == 0u) continue;
                if (assets->palette_written[paletteSet][paletteIndex] == 0u) continue;
                putPixel(framebuffer, 72u + screenX, screenY,
                         assets->palettes[paletteSet][paletteIndex]);
            }
        }
    }
}

/* Native two-page course map. The source stream's $8E packets are decoded
 * once into tile/attribute cells; no NES PPU, register or timing model exists
 * in the game runtime. The renderer rasterizes these native scene cells. */
static uint8_t s_raceCourseMap[2u * 1024u];
static uint32_t s_raceLastColumnSerial;
static int s_raceCourseMapReady;


/* Source CBA4/CBD4/CBE3 + CCCA/D1C7: each podium is two diagonal
 * 8x8 sprites, placed in OAM $02C4, $02CC and $02D4. */
void nativeRaceRenderBuildResultsPodium(NativeRaceSession *session)
{
    static const uint8_t x_position[3] = { 0x78u, 0x70u, 0x80u };
    static const uint8_t y_position[3] = { 0x37u, 0x3Fu, 0x3Fu };
    static const uint8_t tile_base[3] = { 0xB7u, 0xB9u, 0xB9u };
    static const uint8_t attribute[3] = { 0x01u, 0x01u, 0x41u };
    static const uint8_t oam_offset[3] = { 0x04u, 0x0Cu, 0x14u };
    unsigned int i;
    if (session == NULL) return;
    for (i = 0u; i < 3u; ++i) {
        const unsigned int base = 0xC0u + oam_offset[i];
        session->oam.bytes[base] = y_position[i];
        session->oam.bytes[base + 1u] = tile_base[i];
        session->oam.bytes[base + 2u] = attribute[i];
        session->oam.bytes[base + 3u] = x_position[i];
        session->oam.bytes[base + 4u] = (uint8_t)(y_position[i] + 8u);
        session->oam.bytes[base + 5u] = (uint8_t)(tile_base[i] + 1u);
        session->oam.bytes[base + 6u] = attribute[i];
        session->oam.bytes[base + 7u] = (uint8_t)(x_position[i] + 8u);
    }
    session->result_podium_active = 1u;
}

void nativeRaceRenderReset(void)
{
    memset(s_raceCourseMap, 0, sizeof(s_raceCourseMap));
    s_raceLastColumnSerial = 0u;
    s_raceCourseMapReady = 0;
}

static int applyRaceCourseColumn(const uint8_t command[0x12])
{
    unsigned int page, offset, i;
    unsigned int address;
    if (command == NULL || command[3] != 0x8Eu) return 0;
    address = ((unsigned int)command[1] << 8u) | command[2];
    if (address < 0x2000u || address >= 0x3000u) return 0;

    offset = address - 0x2000u;
    page = (offset >> 10u) & 1u; /* Excitebike's vertical two-page layout. */
    offset &= 0x03FFu;
    for (i = 0u; i < 14u; ++i) {
        if (offset >= 1024u) return 0;
        s_raceCourseMap[page * 1024u + offset] = command[4u + i];
        offset += 32u; /* $8E: next tile is one 32-byte nametable row below. */
        if (offset >= 1024u) {
            offset -= 1024u;
            page ^= 1u;
        }
    }
    return 1;
}

static int drawRaceCourse(uint8_t *framebuffer, const NativeMenuAssets *assets,
                          const NativeMenuSceneAssets *scene,
                          const NativeRaceSession *session)
{
    unsigned int x, y;
    if (!s_raceCourseMapReady) {
        /* Scene 6 is four source nametables; vertical mirroring uses pages 0/1. */
        memcpy(s_raceCourseMap, scene->nametable, sizeof(s_raceCourseMap));
        s_raceLastColumnSerial = 0u;
        s_raceCourseMapReady = 1;
    }

    if (session->course_column_serial != s_raceLastColumnSerial) {
        if (!applyRaceCourseColumn(session->course_column_buffer)) return 0;
        s_raceLastColumnSerial = session->course_column_serial;
    }

    for (y = 0u; y < 240u; ++y) {
        const unsigned int tileY = y >> 3u;
        for (x = 0u; x < 256u; ++x) {
            const unsigned int worldX =
                ((unsigned int)(session->movement.base_nametable & 1u) << 8u) +
                (unsigned int)session->movement.player_scroll_x + x;
            const unsigned int page = (worldX >> 8u) & 1u;
            const unsigned int sourceX = worldX & 0xFFu;
            const unsigned int tileX = sourceX >> 3u;
            const size_t pageBase = (size_t)page * 1024u;
            const size_t tileIndex = pageBase + (size_t)tileY * 32u + tileX;
            const size_t attributeIndex = pageBase + 960u +
                (size_t)(tileY >> 2u) * 8u + (tileX >> 2u);
            const uint8_t tile = s_raceCourseMap[tileIndex];
            const uint8_t attribute = s_raceCourseMap[attributeIndex];
            const unsigned int shift = ((tileY & 2u) << 1u) | (tileX & 2u);
            const unsigned int subpalette = (attribute >> shift) & 3u;
            const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)tile;
            uint8_t pixel, paletteIndex;
            if (chrTile >= assets->chr_tile_count) continue;
            pixel = (uint8_t)(assets->chr_pixels[
                chrTile * 64u + (size_t)(y & 7u) * 8u + (sourceX & 7u)] & 3u);
            paletteIndex = pixel == 0u ? 0u : (uint8_t)(subpalette * 4u + pixel);
            if (assets->palette_written[6u][paletteIndex] != 0u) {
                putPixel(framebuffer, 72u + x, y, assets->palettes[6u][paletteIndex]);
            }
        }
    }
    return 1;
}

int nativeRaceRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu,
                          const NativeRaceSession *session)
{
    uint8_t *framebuffer;
    const NativeMenuSceneAssets *scene;
    if (assets == NULL || menu == NULL || session == NULL ||
        assets->loaded == 0u || assets->race_scene_available == 0u ||
        session->initialized == 0u) return 0;

    framebuffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (framebuffer == NULL) return 0;

    scene = &assets->scenes[6];
    memset(framebuffer, 0, 400u * 240u * 3u);
    if (!drawRaceCourse(framebuffer, assets, scene, session)) return 0;
    drawRaceHud(framebuffer, assets, scene, session, 6u);
    drawRaceResultTimes(framebuffer, assets, scene, session);
    drawRaceOam(framebuffer, assets, session, 6u);
    drawCursor(framebuffer, assets, menu, 6u); /* OAM sprite zero has highest priority. */
    return 1;
}
