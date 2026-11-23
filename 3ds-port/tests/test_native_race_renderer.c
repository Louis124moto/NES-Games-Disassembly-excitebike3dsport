#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "native_menu_renderer.h"

static uint8_t framebuffer[400u * 240u * 3u];

uint8_t *gfxGetFramebuffer(int screen, int side, u16 *width, u16 *height)
{
    (void)screen;
    (void)side;
    if (width != NULL) *width = 240u;
    if (height != NULL) *height = 400u;
    return framebuffer;
}

static size_t pixelOffset(unsigned x, unsigned y)
{
    return (((size_t)x * 240u) + (239u - y)) * 3u;
}

int main(void)
{
    NativeMenuAssets assets;
    NativeMenuState menu;
    NativeRaceSession session;
    size_t i, offset;
    uint8_t before;

    memset(&assets, 0, sizeof(assets));
    memset(&menu, 0, sizeof(menu));
    memset(&session, 0, sizeof(session));

    assets.chr_pixels = (uint8_t *)calloc(
        NATIVE_MENU_CHR_TILE_LIMIT, 64u);
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

    nativeRaceRenderReset();
    if (!nativeRaceRenderFrame(&assets, &menu, &session)) {
        puts("FAIL render initial native course scene");
        free(assets.chr_pixels);
        return 1;
    }

    offset = pixelOffset(72u, 0u);
    before = framebuffer[offset];
    if (before != 116u) {
        printf("FAIL initial source tile pixel B=%u expected=116\n", before);
        free(assets.chr_pixels);
        return 1;
    }

    /* A source $8E packet must update one native nametable column (14 rows). */
    session.course_column_serial = 1u;
    session.course_column_buffer[1] = 0x20u;
    session.course_column_buffer[2] = 0x00u;
    session.course_column_buffer[3] = 0x8Eu;
    for (i = 4u; i < 18u; ++i)
        session.course_column_buffer[i] = 1u;

    if (!nativeRaceRenderFrame(&assets, &menu, &session)) {
        puts("FAIL apply native F4FF column packet");
        free(assets.chr_pixels);
        return 1;
    }
    if (framebuffer[offset] != 144u) {
        printf("FAIL dynamic course tile pixel B=%u expected=144\n",
               framebuffer[offset]);
        free(assets.chr_pixels);
        return 1;
    }

    /* CBA4/CBD4/CBE3/CCCA: exact three podium 2x2 OAM blocks. */
    nativeRaceRenderBuildResultsPodium(&session);
    if (session.result_podium_active == 0u ||
        session.oam.bytes[0xC4u] != 0x37u ||
        session.oam.bytes[0xC5u] != 0xB7u ||
        session.oam.bytes[0xC7u] != 0x78u ||
        session.oam.bytes[0xC8u] != 0x3Fu ||
        session.oam.bytes[0xC9u] != 0xB8u ||
        session.oam.bytes[0xCBu] != 0x80u ||
        session.oam.bytes[0xCCu] != 0x3Fu ||
        session.oam.bytes[0xCDu] != 0xB9u ||
        session.oam.bytes[0xCFu] != 0x70u ||
        session.oam.bytes[0xD4u] != 0x3Fu ||
        session.oam.bytes[0xD5u] != 0xB9u ||
        session.oam.bytes[0xD6u] != 0x41u ||
        session.oam.bytes[0xD7u] != 0x80u) {
        puts("FAIL source podium OAM blocks were not built correctly");
        free(assets.chr_pixels);
        return 1;
    }
    memset(session.oam.bytes + 0xC0u, 0xF8, 0x20u);
    session.result_podium_active = 0u;

    /* C5A7/CB77 result-time glyphs are drawn at the source PPU rows. */
    session.result_active = 1u;
    session.result_record_glyphs[0] = 0u;
    session.result_current_glyphs[0] = 1u;
    if (!nativeRaceRenderFrame(&assets, &menu, &session) ||
        framebuffer[pixelOffset(72u + 18u * 8u, 15u * 8u)] != 116u ||
        framebuffer[pixelOffset(72u + 18u * 8u, 17u * 8u)] != 144u) {
        puts("FAIL source result times were not rendered at $21F2/$2232");
        free(assets.chr_pixels);
        return 1;
    }
    session.result_active = 0u;

    /* Invalid PPU addresses are rejected instead of corrupting the map. */
    session.course_column_serial = 2u;
    session.course_column_buffer[1] = 0x30u;
    session.course_column_buffer[2] = 0x00u;
    if (nativeRaceRenderFrame(&assets, &menu, &session)) {
        puts("FAIL renderer accepted out-of-range course packet");
        free(assets.chr_pixels);
        return 1;
    }

    free(assets.chr_pixels);
    puts("PASS: native race framebuffer draws the source scene, applies F4FF columns and rejects invalid addresses.");
    return 0;
}
