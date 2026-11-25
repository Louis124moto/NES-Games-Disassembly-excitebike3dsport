#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "native_scene.h"
#include "nes_palette.h"

/* Excitebike is NROM-128/CHR-ROM 8 KiB. The original writes $90 at C1D5
 * and $10 at C32B-C333 before the first race: PPUCTRL bit 4 selects the
 * $1000-$1FFF pattern table for background tiles (tile indices 256..511).
 * Bit 3 stays clear, so 8x8 OAM sprites continue using indices 0..255.
 */
#define EXCITEBIKE_BG_CHR_TILE_BASE 256u

#define MENU_PALETTE_SET 6u        /* source palette preset 18 */
#define MENU_PALETTE_SET_DESIGN 0u /* C551: state 9 uses preset 12 */


/* ------------------------------------------------------------------ atlas */

static size_t atlasTileU(unsigned variant, unsigned tile)
{
    return (size_t)((variant & 1u) * SCENE_ATLAS_VARIANT_WIDTH +
                    (tile % SCENE_ATLAS_TILES_PER_ROW) * 8u);
}

static size_t atlasTileV(unsigned variant, unsigned tile)
{
    return (size_t)((variant >> 1u) * SCENE_ATLAS_VARIANT_HEIGHT +
                    (tile / SCENE_ATLAS_TILES_PER_ROW) * 8u);
}

/* Palette entry used by one pixel value in one variant; -1 = transparent. */
static int variantPaletteIndex(unsigned variant, unsigned value)
{
    if (variant < SCENE_VARIANT_KEYED)
        return value == 0u ? 0 : (int)(variant * 4u + value);
    if (variant < SCENE_VARIANT_SPRITE)
        return value == 0u ? -1 : (int)((variant - SCENE_VARIANT_KEYED) * 4u + value);
    return value == 0u ? -1 : (int)(16u + (variant - SCENE_VARIANT_SPRITE) * 4u + value);
}

void nativeSceneBakeAtlas(uint32_t *atlas, const NativeMenuAssets *assets,
                          unsigned paletteSet)
{
    unsigned variant, tile, py, px;
    size_t tileCount;
    if (atlas == NULL) return;
    memset(atlas, 0, (size_t)SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT * sizeof(uint32_t));
    if (assets == NULL || assets->chr_pixels == NULL || paletteSet >= NATIVE_MENU_PALETTE_COUNT)
        return;
    tileCount = assets->chr_tile_count < SCENE_ATLAS_TILES ? assets->chr_tile_count
                                                            : SCENE_ATLAS_TILES;
    for (variant = 0u; variant < SCENE_VARIANT_COUNT; ++variant) {
        for (tile = 0u; tile < tileCount; ++tile) {
            const uint8_t *pixels = assets->chr_pixels + (size_t)tile * 64u;
            const size_t u0 = atlasTileU(variant, tile);
            const size_t v0 = atlasTileV(variant, tile);
            for (py = 0u; py < 8u; ++py) {
                for (px = 0u; px < 8u; ++px) {
                    const unsigned value = pixels[py * 8u + px] & 3u;
                    const int index = variantPaletteIndex(variant, value);
                    uint8_t color, rgb[3];
                    if (index < 0 || assets->palette_written[paletteSet][index] == 0u) continue;
                    color = assets->palettes[paletteSet][index];
                    if (!nesPaletteToRgb(color, rgb)) continue;
                    atlas[(v0 + py) * SCENE_ATLAS_WIDTH + u0 + px] =
                        ((uint32_t)rgb[0] << 24u) | ((uint32_t)rgb[1] << 16u) |
                        ((uint32_t)rgb[2] << 8u) | 0xFFu;
                }
            }
        }
    }
}

size_t nativeSceneSwizzledIndex(unsigned x, unsigned y)
{
    const unsigned fx = x & 7u, fy = y & 7u;
    const unsigned morton = (fx & 1u) | ((fy & 1u) << 1u) |
                            ((fx & 2u) << 1u) | ((fy & 2u) << 2u) |
                            ((fx & 4u) << 2u) | ((fy & 4u) << 3u);
    return ((size_t)(y >> 3u) * (SCENE_ATLAS_WIDTH >> 3u) + (x >> 3u)) * 64u + morton;
}

void nativeSceneSwizzleAtlas(uint32_t *dst, const uint32_t *src)
{
    unsigned x, y;
    if (dst == NULL || src == NULL) return;
    for (y = 0u; y < SCENE_ATLAS_HEIGHT; ++y)
        for (x = 0u; x < SCENE_ATLAS_WIDTH; ++x)
            dst[nativeSceneSwizzledIndex(x, y)] = src[(size_t)y * SCENE_ATLAS_WIDTH + x];
}

/* ------------------------------------------------------------ quad helpers */

static void listClear(SceneDrawList *list, unsigned paletteSet)
{
    list->count = 0u;
    list->palette_set = (uint8_t)paletteSet;
}

/*
 * Adds the visible part of one 8x8 tile. clipW/clipH are the visible columns
 * and rows counted from the quad's top-left on screen; srcLeft/srcTop skip
 * source columns/rows hidden on the left/top (course scrolling).
 * Mirroring is applied before cropping, exactly like the source sprite flips.
 */
static void addTile(SceneDrawList *list, int x, int y, unsigned tile, unsigned variant,
                    unsigned srcLeft, unsigned srcTop, unsigned clipW, unsigned clipH,
                    int flipX, int flipY)
{
    SceneQuad *quad;
    const unsigned u = (unsigned)atlasTileU(variant, tile);
    const unsigned v = (unsigned)atlasTileV(variant, tile);
    if (list->count >= SCENE_MAX_QUADS || clipW == 0u || clipH == 0u) return;
    quad = &list->quads[list->count++];
    quad->x = (int16_t)x;
    quad->y = (int16_t)y;
    quad->w = (uint8_t)clipW;
    quad->h = (uint8_t)clipH;
    if (flipX) {
        quad->u0 = (uint16_t)(u + 8u - srcLeft);
        quad->u1 = (uint16_t)(u + 8u - srcLeft - clipW);
    } else {
        quad->u0 = (uint16_t)(u + srcLeft);
        quad->u1 = (uint16_t)(u + srcLeft + clipW);
    }
    if (flipY) {
        quad->v0 = (uint16_t)(v + 8u - srcTop);
        quad->v1 = (uint16_t)(v + 8u - srcTop - clipH);
    } else {
        quad->v0 = (uint16_t)(v + srcTop);
        quad->v1 = (uint16_t)(v + srcTop + clipH);
    }
}

static unsigned subpaletteFor(const uint8_t *nametable, unsigned tileX, unsigned tileY)
{
    const uint8_t attribute = nametable[960u + (size_t)(tileY / 4u) * 8u + tileX / 4u];
    const unsigned shift = ((tileY & 2u) << 1u) | (tileX & 2u);
    return (attribute >> shift) & 3u;
}

/* A background/HUD tile taken from the second CHR pattern table. */
static int tileAvailable(const NativeMenuAssets *assets, size_t chrTile)
{
    return chrTile < assets->chr_tile_count && chrTile < SCENE_ATLAS_TILES;
}

static int assetsUsable(const NativeMenuAssets *assets)
{
    return assets != NULL && assets->loaded != 0u &&
           assets->chr_pixels != NULL && assets->chr_tile_count != 0u;
}

/* ------------------------------------------------------------------- menu */

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

static void addBackground(SceneDrawList *list, const NativeMenuAssets *assets,
                          const NativeMenuSceneAssets *scene)
{
    unsigned tileX, tileY;
    for (tileY = 0u; tileY < 30u; ++tileY) {
        for (tileX = 0u; tileX < 32u; ++tileX) {
            const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE +
                                   (size_t)scene->nametable[(size_t)tileY * 32u + tileX];
            if (!tileAvailable(assets, chrTile)) continue;
            addTile(list, (int)(tileX * 8u), (int)(tileY * 8u), (unsigned)chrTile,
                    SCENE_VARIANT_BACKGROUND + subpaletteFor(scene->nametable, tileX, tileY),
                    0u, 0u, 8u, 8u, 0, 0);
        }
    }
}

/* 8x8 sprite at game-area position (x, y); maxX/maxY bound what stays visible. */
static void addSprite(SceneDrawList *list, unsigned tile, unsigned attribute,
                      unsigned x, unsigned y, unsigned maxX, unsigned maxY)
{
    unsigned w, h;
    if (x >= maxX || y >= maxY) return;
    w = maxX - x < 8u ? maxX - x : 8u;
    h = maxY - y < 8u ? maxY - y : 8u;
    addTile(list, (int)x, (int)y, tile, SCENE_VARIANT_SPRITE + (attribute & 3u),
            0u, 0u, w, h, (attribute & 0x40u) != 0u, (attribute & 0x80u) != 0u);
}

static void addCursor(SceneDrawList *list, const NativeMenuAssets *assets,
                      const NativeMenuState *menu)
{
    if (menu->cursor_sprite_y == 0xF8u || menu->cursor_sprite_tile == 0xF8u ||
        menu->cursor_sprite_x == 0xF8u || menu->cursor_sprite_tile >= assets->chr_tile_count)
        return;
    /* The cursor may overhang the 256-pixel game area; only the 400-pixel
     * panel bounds it, as in the original native front-end. */
    addSprite(list, menu->cursor_sprite_tile, menu->cursor_sprite_attribute,
              menu->cursor_sprite_x, (unsigned)menu->cursor_sprite_y + 1u,
              400u - SCENE_SCREEN_OFFSET_X, SCENE_GAME_HEIGHT);
}

int nativeSceneBuildMenu(SceneDrawList *list, const NativeMenuAssets *assets,
                         const NativeMenuState *menu)
{
    const NativeMenuSceneAssets *scene;
    unsigned paletteSet;
    if (list == NULL) return 0;
    listClear(list, MENU_PALETTE_SET);
    if (menu == NULL || !assetsUsable(assets)) return 0;
    scene = sceneForState(assets, menu->source_state);
    if (scene == NULL) return 0;
    paletteSet = menu->source_state == 9u ? MENU_PALETTE_SET_DESIGN : MENU_PALETTE_SET;
    list->palette_set = (uint8_t)paletteSet;
    addBackground(list, assets, scene);
    addCursor(list, assets, menu);
    return 1;
}

/* ------------------------------------------------------------------- race */

/* Native two-page course map. The source stream's $8E packets are decoded
 * once into tile/attribute cells; the scene builder reads these cells. */
static uint8_t s_raceCourseMap[2u * 1024u];
static uint32_t s_raceLastColumnSerial;
static int s_raceCourseMapReady;

void nativeSceneRaceReset(void)
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

/* The scrolling course: 33 tile columns cover 256 pixels at any fine scroll. */
static int addRaceCourse(SceneDrawList *list, const NativeMenuAssets *assets,
                         const NativeMenuSceneAssets *scene,
                         const NativeRaceSession *session)
{
    unsigned int column, tileY;
    unsigned int worldX0, fine, firstTileColumn;
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

    worldX0 = ((unsigned int)(session->movement.base_nametable & 1u) << 8u) +
              (unsigned int)session->movement.player_scroll_x;
    fine = worldX0 & 7u;
    firstTileColumn = worldX0 >> 3u;
    for (column = 0u; column < 33u; ++column) {
        const int screenX = (int)(column * 8u) - (int)fine;
        const unsigned int visibleLeft = screenX < 0 ? (unsigned int)(-screenX) : 0u;
        const unsigned int visibleRight = screenX + 8 > (int)SCENE_GAME_WIDTH
                                              ? SCENE_GAME_WIDTH - (unsigned int)screenX : 8u;
        const unsigned int tileColumn = (firstTileColumn + column) & 63u;
        const size_t pageBase = (size_t)((tileColumn >> 5u) & 1u) * 1024u;
        const unsigned int tileX = tileColumn & 31u;
        if (visibleRight <= visibleLeft) continue;
        for (tileY = 0u; tileY < 30u; ++tileY) {
            const uint8_t tile = s_raceCourseMap[pageBase + (size_t)tileY * 32u + tileX];
            const uint8_t attribute = s_raceCourseMap[pageBase + 960u +
                (size_t)(tileY >> 2u) * 8u + (tileX >> 2u)];
            const unsigned int shift = ((tileY & 2u) << 1u) | (tileX & 2u);
            const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)tile;
            if (!tileAvailable(assets, chrTile)) continue;
            addTile(list, screenX + (int)visibleLeft, (int)(tileY * 8u), (unsigned)chrTile,
                    SCENE_VARIANT_BACKGROUND + ((attribute >> shift) & 3u),
                    visibleLeft, 0u, visibleRight - visibleLeft, 8u, 0, 0);
        }
    }
    (void)scene;
    return 1;
}

static void addRaceHud(SceneDrawList *list, const NativeMenuAssets *assets,
                       const NativeMenuSceneAssets *scene,
                       const NativeRaceSession *session)
{
    const unsigned int tileY = 26u;
    unsigned int i;
    for (i = 0u; i < 7u; ++i) {
        const unsigned int tileX = 3u + i;
        const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)session->setup.hud_glyphs[i];
        if (!tileAvailable(assets, chrTile)) continue;
        addTile(list, (int)(tileX * 8u), (int)(tileY * 8u), (unsigned)chrTile,
                SCENE_VARIANT_BACKGROUND + subpaletteFor(scene->nametable, tileX, tileY),
                0u, 0u, 8u, 8u, 0, 0);
    }
    if (session->setup.counter_visible != 0u) {
        /* C8E2: PPU $2378 = tile row 27, column 24. */
        for (i = 0u; i < 4u; ++i) {
            const unsigned int cx = 24u + i, cy = 27u;
            const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)session->setup.counter_tiles[i];
            if (!tileAvailable(assets, chrTile)) continue;
            addTile(list, (int)(cx * 8u), (int)(cy * 8u), (unsigned)chrTile,
                    SCENE_VARIANT_BACKGROUND + subpaletteFor(scene->nametable, cx, cy),
                    0u, 0u, 8u, 8u, 0, 0);
        }
    }
}

/* C5A7/CB77 score-time PPU writes: seven source CHR glyphs at $21F2
 * (stored course record) and $2232 (just-finished race time). Colour 0 is
 * transparent, so the course underneath stays visible. */
static void addResultGlyphRow(SceneDrawList *list, const NativeMenuAssets *assets,
                              const NativeMenuSceneAssets *scene,
                              const uint8_t glyphs[7], unsigned int tileY,
                              unsigned int tileX)
{
    unsigned int i;
    for (i = 0u; i < 7u; ++i) {
        const unsigned int column = tileX + i;
        const size_t chrTile = EXCITEBIKE_BG_CHR_TILE_BASE + (size_t)glyphs[i];
        if (!tileAvailable(assets, chrTile)) continue;
        addTile(list, (int)(column * 8u), (int)(tileY * 8u), (unsigned)chrTile,
                SCENE_VARIANT_KEYED + subpaletteFor(scene->nametable, column, tileY),
                0u, 0u, 8u, 8u, 0, 0);
    }
}

static void addRaceSprites(SceneDrawList *list, const NativeMenuAssets *assets,
                           const NativeRaceSession *session)
{
    int sprite;
    /* NES OAM priority: lower sprite indices are drawn last and appear in front. */
    for (sprite = 63; sprite >= 0; --sprite) {
        const size_t offset = (size_t)sprite * 4u;
        const uint8_t y = session->oam.bytes[offset + 0u];
        const uint8_t tile = session->oam.bytes[offset + 1u];
        const uint8_t attribute = session->oam.bytes[offset + 2u];
        const uint8_t x = session->oam.bytes[offset + 3u];
        if (y >= 0xEFu || tile >= assets->chr_tile_count) continue;
        addSprite(list, tile, attribute, x, (unsigned)y + 1u,
                  SCENE_GAME_WIDTH, SCENE_GAME_HEIGHT);
    }
}

int nativeSceneBuildRace(SceneDrawList *list, const NativeMenuAssets *assets,
                         const NativeMenuState *menu,
                         const NativeRaceSession *session)
{
    const NativeMenuSceneAssets *scene;
    if (list == NULL) return 0;
    listClear(list, NATIVE_RACE_PALETTE_BASE +
                     ((session != NULL && session->course < NATIVE_RACE_COURSE_COUNT) ? session->course : 0u));
    if (assets == NULL || menu == NULL || session == NULL ||
        assets->loaded == 0u || assets->race_scene_available == 0u ||
        session->initialized == 0u || !assetsUsable(assets)) return 0;

    scene = &assets->scenes[6];
    if (!addRaceCourse(list, assets, scene, session)) {
        list->count = 0u;
        return 0;
    }
    addRaceHud(list, assets, scene, session);
    if (session->result_active != 0u) {
        /* Source tbl_C0EC: index 0 -> $21F2, index 4 -> $2232. */
        addResultGlyphRow(list, assets, scene, session->result_record_glyphs, 15u, 18u);
        addResultGlyphRow(list, assets, scene, session->result_current_glyphs, 17u, 18u);
    }
    addRaceSprites(list, assets, session);
    addCursor(list, assets, menu); /* OAM sprite zero has highest priority. */
    return 1;
}
