#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"

/* Diagnostic CHR atlas only; no placeholder game/menu is rendered. */
static void rendererDrawChrAtlas(void)
{
    static const uint8_t shades[4] = {0u, 85u, 170u, 255u};
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer;
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    unsigned int tile, row, col;

    if (!assetExtractorIsActive()) return;
    pixels = assetExtractorGetPixels(&tileCount);
    framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width < 240u || height < 400u) return;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (pixels == NULL || tileCount == 0u) return;
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
    size_t tileCount = 0u;
    (void)state;
    (void)assetExtractorGetPixels(&tileCount);

    /*
     * This console is the bottom-screen log. Keep it visible even while the
     * atlas viewer is closed, so startup/cache/ROM errors are immediately
     * inspectable instead of hidden behind the Y toggle.
     */
    printf("EXCITEBIKE 3DS - ASSET LOG\n");
    printf("CHR status: %s\n", assetExtractorGetStatus());
    printf("Decoded tiles: %lu / 960\n", (unsigned long)tileCount);
    printf("ROM path: rom.nes beside the launched 3DSX\n");
    printf("Cache path: extracted/chr_pixels.bin beside the 3DSX\n");
    printf("Viewer: %s\n", assetExtractorIsActive() ? "OPEN" : "closed");
    printf("Y: toggle atlas   X: retry extraction (viewer open)\n");
    printf("B: close atlas   A: no action\n");
    printf("NOTE: raw CHR patterns only; menu layout/palettes not yet emulated.\n");
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
