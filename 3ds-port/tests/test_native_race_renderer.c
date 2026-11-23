#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
