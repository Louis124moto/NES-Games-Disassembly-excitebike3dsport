#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"
#include "nes_chr_assets.h"

/*
 * libctru's top-screen framebuffer is BGR8 in rotated memory order.
 * This is a diagnostic tile atlas only, not the original NES menu layout.
 */
static int s_runtimeAtlasWasShown;

static void rendererDrawChrAtlas(void)
{
    static const uint8_t shades[4] = {0u, 85u, 170u, 255u};
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer;
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    unsigned int tile, row, col;

    if (assetExtractorIsActive()) {
        pixels = assetExtractorGetPixels(&tileCount);
    } else {
#if NES_CHR_TILE_COUNT > 0
        pixels = &nesChrPixels[0][0];
        tileCount = NES_CHR_TILE_COUNT;
#endif
    }

    framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width < 240u || height < 400u) return;
    if (assetExtractorIsActive() && (pixels == NULL || tileCount == 0u)) {
        memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
        s_runtimeAtlasWasShown = 0;
        return;
    }
    if (!assetExtractorIsActive() && s_runtimeAtlasWasShown) {
        memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
        s_runtimeAtlasWasShown = 0;
    }
    if (pixels == NULL || tileCount == 0u) return;
    if (assetExtractorIsActive()) s_runtimeAtlasWasShown = 1;
    if (tileCount > 960u) tileCount = 960u;

    for (tile = 0u; tile < tileCount; ++tile) {
        const unsigned int tileX = (tile % 30u) * 8u;
        const unsigned int tileY = (tile / 30u) * 8u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const unsigned int x = tileX + col;
                const unsigned int y = tileY + row;
                const uint8_t shade = shades[pixels[tile * 64u + row * 8u + col] & 3u];
                const unsigned int offset = 3u * (x * 240u + (239u - y));
                framebuffer[offset + 0u] = shade;
                framebuffer[offset + 1u] = shade;
                framebuffer[offset + 2u] = shade;
            }
        }
    }
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
    if (assetExtractorIsActive()) {
        printf("BUILT-IN NES CHR EXTRACTOR\n");
        printf("==========================\n");
        printf("%s\n", assetExtractorGetStatus());
        printf("ROM path: sdmc:/3ds/excitebike3ds/rom.nes\n");
        printf("Y: close/return   X: reload   B: close\n");
        printf("Displays raw CHR tiles, not the original menu layout.\n");
        return;
    }
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
