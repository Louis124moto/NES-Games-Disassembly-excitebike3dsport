/*
 * Golden test for the citro3d scene builder.
 *
 * The pre-citro3d software rasterizer (tests/legacy/legacy_native_renderer.c,
 * frozen) is the reference. For thousands of random menu and race states the
 * new scene builder's quad list, rasterized in software from the baked atlas,
 * must reproduce the legacy 400x240 framebuffer byte for byte, including
 * clipping, flips, sprite priority, keyed glyphs, palette gaps and the
 * persistent scrolling course map.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "native_scene.h"

int legacyMenuRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu);
void legacyRaceRenderReset(void);
int legacyRaceRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu,
                          const NativeRaceSession *session);

#define FB_BYTES (400u * 240u * 3u)
static uint8_t s_legacyFb[FB_BYTES];
static uint8_t s_newFb[FB_BYTES];
static uint32_t s_atlas[SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT];
static uint32_t s_swizzled[SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT];
static SceneDrawList s_list;
static NativeMenuAssets s_assets;
static NativeMenuState s_menu;
static NativeRaceSession s_session;

uint8_t *gfxGetFramebuffer(int screen, int side, u16 *width, u16 *height)
{
    (void)screen; (void)side;
    if (width != NULL) *width = 240u;
    if (height != NULL) *height = 400u;
    return s_legacyFb;
}

static uint32_t s_rng = 0x2468ACEu;
static uint32_t rnd(void)
{
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 17; s_rng ^= s_rng << 5;
    return s_rng;
}

static size_t fbOffset(unsigned x, unsigned y) { return ((size_t)x * 240u + (239u - y)) * 3u; }

/* Software rasterizer for a draw list: the reference model of what the GPU does. */
static void rasterize(uint8_t *fb, const SceneDrawList *list, const uint32_t *atlas)
{
    unsigned n, i, j;
    memset(fb, 0, FB_BYTES);
    for (n = 0u; n < list->count; ++n) {
        const SceneQuad *q = &list->quads[n];
        for (j = 0u; j < q->h; ++j) {
            const unsigned v = q->v0 <= q->v1 ? q->v0 + j : q->v0 - 1u - j;
            for (i = 0u; i < q->w; ++i) {
                const unsigned u = q->u0 <= q->u1 ? q->u0 + i : q->u0 - 1u - i;
                const uint32_t texel = atlas[(size_t)v * SCENE_ATLAS_WIDTH + u];
                const int sx = (int)SCENE_SCREEN_OFFSET_X + q->x + (int)i;
                const int sy = q->y + (int)j;
                if ((texel & 0xFFu) == 0u) continue;
                if (sx < 0 || sx >= 400 || sy < 0 || sy >= 240) continue;
                fb[fbOffset((unsigned)sx, (unsigned)sy)] = (uint8_t)(texel >> 8u);
                fb[fbOffset((unsigned)sx, (unsigned)sy) + 1u] = (uint8_t)(texel >> 16u);
                fb[fbOffset((unsigned)sx, (unsigned)sy) + 2u] = (uint8_t)(texel >> 24u);
            }
        }
    }
}

static void randomAssets(unsigned tileCount)
{
    unsigned s, i;
    free(s_assets.chr_pixels);
    memset(&s_assets, 0, sizeof(s_assets));
    s_assets.chr_pixels = (uint8_t *)malloc((size_t)tileCount * 64u);
    for (i = 0u; i < tileCount * 64u; ++i) s_assets.chr_pixels[i] = (uint8_t)(rnd() & 3u);
    s_assets.chr_tile_count = tileCount;
    for (s = 0u; s < NATIVE_MENU_SCENE_COUNT; ++s)
        for (i = 0u; i < NATIVE_MENU_SCENE_BYTES; ++i)
            s_assets.scenes[s].nametable[i] = (uint8_t)rnd();
    for (s = 0u; s < NATIVE_MENU_PALETTE_COUNT; ++s)
        for (i = 0u; i < 32u; ++i) {
            const uint32_t r = rnd();
            s_assets.palettes[s][i] = (r & 15u) == 0u ? (uint8_t)(0x40u + (r >> 8u) % 190u)
                                                      : (uint8_t)((r >> 8u) & 0x3Fu);
            s_assets.palette_written[s][i] = (r >> 20u) % 10u < 8u;
        }
    /* The legacy reference draws the race with preset 6; the runtime now uses
     * the per-course race palette set, so give both the same colours here. */
    for (s = 0u; s < NATIVE_RACE_COURSE_COUNT; ++s) {
        memcpy(s_assets.palettes[NATIVE_RACE_PALETTE_BASE + s], s_assets.palettes[6], 32u);
        memcpy(s_assets.palette_written[NATIVE_RACE_PALETTE_BASE + s], s_assets.palette_written[6], 32u);
    }
    s_assets.race_scene_available = 1u;
    s_assets.loaded = 1u;
}

static void bake(unsigned paletteSet)
{
    nativeSceneBakeAtlas(s_atlas, &s_assets, paletteSet);
}

static int compare(const char *what, unsigned iteration, int legacyResult, int newResult)
{
    size_t i;
    if (legacyResult != newResult) {
        printf("FAIL %s #%u: return legacy=%d new=%d\n", what, iteration, legacyResult, newResult);
        return 0;
    }
    for (i = 0u; i < FB_BYTES; ++i) {
        if (s_legacyFb[i] != s_newFb[i]) {
            const size_t px = i / 3u;
            printf("FAIL %s #%u: byte %zu (screen x=%zu y=%zu) legacy=%u new=%u\n",
                   what, iteration, i, px / 240u, 239u - px % 240u,
                   (unsigned)s_legacyFb[i], (unsigned)s_newFb[i]);
            return 0;
        }
    }
    return 1;
}

static int testMenu(unsigned iterations)
{
    static const uint8_t states[] = {0u, 1u, 2u, 8u, 9u, 12u, 13u, 3u, 5u, 40u};
    unsigned n;
    for (n = 0u; n < iterations; ++n) {
        int legacyResult, newResult;
        if (n % 25u == 0u) randomAssets(n % 75u == 0u ? 960u : (n % 50u == 0u ? 300u : 512u));
        memset(&s_menu, 0, sizeof(s_menu));
        s_menu.source_state = states[rnd() % sizeof(states)];
        s_menu.cursor_sprite_y = (rnd() % 5u == 0u) ? 0xF8u : (uint8_t)rnd();
        s_menu.cursor_sprite_x = (rnd() % 5u == 0u) ? 0xF8u : (uint8_t)rnd();
        s_menu.cursor_sprite_tile = (rnd() % 5u == 0u) ? 0xF8u : (uint8_t)rnd();
        s_menu.cursor_sprite_attribute = (uint8_t)rnd();
        legacyResult = legacyMenuRenderFrame(&s_assets, &s_menu);
        newResult = nativeSceneBuildMenu(&s_list, &s_assets, &s_menu);
        bake(s_list.palette_set);
        rasterize(s_newFb, &s_list, s_atlas);
        if (!compare("menu", n, legacyResult, newResult)) return 0;
    }
    return 1;
}

static void randomSession(int validPacket)
{
    unsigned i;
    for (i = 0u; i < 256u; ++i) {
        const uint32_t r = rnd();
        /* Bias towards on-screen and edge positions to exercise clipping. */
        s_session.oam.bytes[i] = (i & 3u) == 0u && (r % 4u) != 0u ? (uint8_t)(r % 0xF0u) : (uint8_t)r;
        if ((i & 3u) == 3u && (r % 3u) == 0u) s_session.oam.bytes[i] = (uint8_t)(0xF0u + (r >> 8u) % 16u);
    }
    for (i = 0u; i < 7u; ++i) {
        s_session.setup.hud_glyphs[i] = (uint8_t)rnd();
        s_session.result_record_glyphs[i] = (uint8_t)rnd();
        s_session.result_current_glyphs[i] = (uint8_t)rnd();
    }
    s_session.result_active = (uint8_t)(rnd() & 1u);
    s_session.movement.base_nametable = (uint8_t)rnd();
    s_session.movement.player_scroll_x = (uint8_t)rnd();
    if (rnd() % 3u != 0u) {
        const unsigned address = 0x2000u + rnd() % 0x1000u;
        s_session.course_column_buffer[0] = 0x12u;
        s_session.course_column_buffer[1] = (uint8_t)(address >> 8u);
        s_session.course_column_buffer[2] = (uint8_t)address;
        s_session.course_column_buffer[3] = validPacket ? 0x8Eu : 0x4Cu;
        for (i = 4u; i < 0x12u; ++i) s_session.course_column_buffer[i] = (uint8_t)rnd();
        s_session.course_column_serial += 1u + rnd() % 3u;
    }
}

static int testRace(unsigned iterations)
{
    unsigned n;
    for (n = 0u; n < iterations; ++n) {
        int legacyResult, newResult;
        if (n % 60u == 0u) {
            randomAssets(n % 120u == 0u ? 960u : (n % 180u == 60u ? 280u : 512u));
            memset(&s_session, 0, sizeof(s_session));
            s_session.initialized = 1u;
            legacyRaceRenderReset();
            nativeSceneRaceReset();
        }
        memset(&s_menu, 0, sizeof(s_menu));
        s_menu.cursor_sprite_y = (rnd() % 4u == 0u) ? 0xF8u : (uint8_t)rnd();
        s_menu.cursor_sprite_x = (uint8_t)rnd();
        s_menu.cursor_sprite_tile = (uint8_t)rnd();
        s_menu.cursor_sprite_attribute = (uint8_t)rnd();
        randomSession(rnd() % 12u != 0u);
        legacyResult = legacyRaceRenderFrame(&s_assets, &s_menu, &s_session);
        newResult = nativeSceneBuildRace(&s_list, &s_assets, &s_menu, &s_session);
        bake(s_list.palette_set);
        rasterize(s_newFb, &s_list, s_atlas);
        if (!compare("race", n, legacyResult, newResult)) return 0;
        if (s_list.count > SCENE_MAX_QUADS) { puts("FAIL quad overflow"); return 0; }
    }
    return 1;
}

static int testSwizzle(void)
{
    static unsigned char seen[SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT];
    unsigned x, y;
    if (nativeSceneSwizzledIndex(0u, 0u) != 0u || nativeSceneSwizzledIndex(1u, 0u) != 1u ||
        nativeSceneSwizzledIndex(0u, 1u) != 2u || nativeSceneSwizzledIndex(2u, 0u) != 4u ||
        nativeSceneSwizzledIndex(0u, 2u) != 8u || nativeSceneSwizzledIndex(4u, 0u) != 16u ||
        nativeSceneSwizzledIndex(0u, 4u) != 32u || nativeSceneSwizzledIndex(7u, 7u) != 63u ||
        nativeSceneSwizzledIndex(8u, 0u) != 64u ||
        nativeSceneSwizzledIndex(0u, 8u) != 64u * (SCENE_ATLAS_WIDTH / 8u)) {
        puts("FAIL Morton tile layout does not match the PICA200 texture format");
        return 0;
    }
    for (y = 0u; y < SCENE_ATLAS_HEIGHT; ++y)
        for (x = 0u; x < SCENE_ATLAS_WIDTH; ++x) {
            const size_t index = nativeSceneSwizzledIndex(x, y);
            if (index >= sizeof(seen) || seen[index]) { puts("FAIL swizzle is not a bijection"); return 0; }
            seen[index] = 1u;
        }
    for (y = 0u; y < SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT; ++y) s_atlas[y] = y * 2654435761u + 1u;
    nativeSceneSwizzleAtlas(s_swizzled, s_atlas);
    for (y = 0u; y < SCENE_ATLAS_HEIGHT; ++y)
        for (x = 0u; x < SCENE_ATLAS_WIDTH; ++x)
            if (s_swizzled[nativeSceneSwizzledIndex(x, y)] != s_atlas[(size_t)y * SCENE_ATLAS_WIDTH + x]) {
                puts("FAIL swizzled texel mismatch");
                return 0;
            }
    return 1;
}

static int testNullArguments(void)
{
    memset(&s_menu, 0, sizeof(s_menu));
    if (nativeSceneBuildMenu(NULL, &s_assets, &s_menu) != 0 ||
        nativeSceneBuildMenu(&s_list, NULL, &s_menu) != 0 || s_list.count != 0u ||
        nativeSceneBuildMenu(&s_list, &s_assets, NULL) != 0 ||
        nativeSceneBuildRace(&s_list, &s_assets, &s_menu, NULL) != 0 || s_list.count != 0u) {
        puts("FAIL scene builders accepted missing arguments");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!testSwizzle() || !testNullArguments() || !testMenu(600u) || !testRace(4000u)) return 1;
    free(s_assets.chr_pixels);
    puts("PASS: citro3d scene builder matches the legacy rasterizer byte for byte "
         "(600 menu + 4000 race frames), PICA200 swizzle verified.");
    return 0;
}
