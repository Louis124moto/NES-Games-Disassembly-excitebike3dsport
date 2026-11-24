/*
 * Regression test for the race scene builder: course tiles, $8E column
 * packets, podium OAM blocks, result-time glyphs and invalid packets.
 * The draw list is checked through the baked atlas texel each quad samples.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native_scene.h"

static uint32_t s_atlas[SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT];
static SceneDrawList s_list;

/* Blue channel of the first texel of the topmost (last drawn) quad at (x,y);
 * -1 when absent. */
static int quadBlue(unsigned x, unsigned y)
{
    unsigned n;
    for (n = s_list.count; n-- > 0u;) {
        const SceneQuad *q = &s_list.quads[n];
        if (q->x == (int)x && q->y == (int)y) {
            const uint32_t texel = s_atlas[(size_t)q->v0 * SCENE_ATLAS_WIDTH + q->u0];
            return (int)((texel >> 8) & 0xFFu);
        }
    }
    return -1;
}

static void bake(const NativeMenuAssets *assets)
{
    nativeSceneBakeAtlas(s_atlas, assets, s_list.palette_set);
}

#define FAIL(msg) do { puts("FAIL " msg); free(assets.chr_pixels); return 1; } while (0)

int main(void)
{
    NativeMenuAssets assets;
    NativeMenuState menu;
    NativeRaceSession session;
    size_t i;

    memset(&assets, 0, sizeof(assets));
    memset(&menu, 0, sizeof(menu));
    memset(&session, 0, sizeof(session));

    assets.chr_pixels = (uint8_t *)calloc(NATIVE_MENU_CHR_TILE_LIMIT, 64u);
    if (assets.chr_pixels == NULL) {
        puts("FAIL allocate host CHR image");
        return 1;
    }
    assets.chr_tile_count = NATIVE_MENU_CHR_TILE_LIMIT;
    assets.loaded = 1u;
    assets.race_scene_available = 1u;
    memset(assets.chr_pixels + 256u * 64u, 1, 64u);
    memset(assets.chr_pixels + 257u * 64u, 2, 64u);
    assets.palette_written[6][1] = 1u;
    assets.palette_written[6][2] = 1u;
    assets.palettes[6][1] = 0x01u;
    assets.palettes[6][2] = 0x02u;

    session.initialized = 1u;
    for (i = 0u; i < 64u; ++i)
        session.oam.bytes[i * 4u] = 0xF8u;

    nativeSceneRaceReset();
    if (!nativeSceneBuildRace(&s_list, &assets, &menu, &session))
        FAIL("build initial native course scene");
    bake(&assets);
    if (quadBlue(0u, 0u) != 116)
        FAIL("initial source tile colour (expected B=116)");

    /* A source $8E packet must update one native nametable column (14 rows). */
    session.course_column_serial = 1u;
    session.course_column_buffer[1] = 0x20u;
    session.course_column_buffer[2] = 0x00u;
    session.course_column_buffer[3] = 0x8Eu;
    for (i = 4u; i < 18u; ++i)
        session.course_column_buffer[i] = 1u;
    if (!nativeSceneBuildRace(&s_list, &assets, &menu, &session))
        FAIL("apply native F4FF column packet");
    bake(&assets);
    if (quadBlue(0u, 0u) != 144 || quadBlue(0u, 13u * 8u) != 144 ||
        quadBlue(0u, 14u * 8u) != 116)
        FAIL("dynamic course column (expected 14 rows of B=144 then B=116)");

    /* C5A7/CB77 result-time glyphs are drawn at the source PPU rows. */
    session.result_active = 1u;
    session.result_record_glyphs[0] = 0u;
    session.result_current_glyphs[0] = 1u;
    if (!nativeSceneBuildRace(&s_list, &assets, &menu, &session))
        FAIL("build result scene");
    bake(&assets);
    if (quadBlue(18u * 8u, 15u * 8u) != 116 || quadBlue(18u * 8u, 17u * 8u) != 144)
        FAIL("source result times were not placed at $21F2/$2232");
    session.result_active = 0u;

    /* Invalid PPU addresses are rejected instead of corrupting the map. */
    session.course_column_serial = 2u;
    session.course_column_buffer[1] = 0x30u;
    session.course_column_buffer[2] = 0x00u;
    if (nativeSceneBuildRace(&s_list, &assets, &menu, &session) || s_list.count != 0u)
        FAIL("scene builder accepted out-of-range course packet");

    free(assets.chr_pixels);
    puts("PASS: race scene draws the source course, applies F4FF columns and rejects invalid addresses.");
    return 0;
}
