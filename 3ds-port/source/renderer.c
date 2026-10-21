#include <stdio.h>
#include <3ds.h>
#include "renderer.h"
#include "nes_chr_assets.h"

/*
 * libctru's top-screen framebuffer is BGR8 in rotated memory order.
 * This is a diagnostic tile atlas only, not the original NES menu layout.
 */
static void rendererDrawChrAtlas(void)
{
#if NES_CHR_TILE_COUNT > 0
    static const uint8_t shades[4] = {0u, 85u, 170u, 255u};
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(
        GFX_TOP, GFX_LEFT, &width, &height);
    unsigned int tile, row, col;

    if (framebuffer == NULL || width < 240u || height < 400u) return;
    for (tile = 0u; tile < NES_CHR_TILE_COUNT && tile < 960u; ++tile) {
        const unsigned int tileX = (tile % 32u) * 8u;
        const unsigned int tileY = (tile / 32u) * 8u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const unsigned int x = tileX + col;
                const unsigned int y = tileY + row;
                const uint8_t shade = shades[nesChrPixels[tile][row * 8u + col] & 3u];
                const unsigned int offset = 3u * (x * 240u + (239u - y));
                framebuffer[offset + 0u] = shade; /* B */
                framebuffer[offset + 1u] = shade; /* G */
                framebuffer[offset + 2u] = shade; /* R */
            }
        }
    }
#endif
}

void rendererInit(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, NULL);
}

void rendererBeginFrame(void)
{
    consoleClear();
}

void rendererDraw(const GameState *state)
{
    (void)state;
    printf("EXCITEBIKE 3DS - FAITHFUL PORT WORKSPACE\n");
    printf("========================================\n");
}

void rendererEndFrame(void)
{
    rendererDrawChrAtlas();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void rendererExit(void)
{
    gfxExit();
}
