#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"

/* 3DS framebuffer is 8x8 tiled; coordinates passed here are screen coordinates. */
static size_t framebufferOffset(unsigned int x, unsigned int y, unsigned int height)
{
    const unsigned int flippedY = height - 1u - y;
    return ((((x >> 3u) * (height >> 3u)) + (flippedY >> 3u)) * 64u
            + (x & 7u) * 8u + (flippedY & 7u)) * 3u;
}

static void rendererDrawChrAtlas(void)
{
    static const uint8_t shades[4] = {0u, 85u, 170u, 255u};
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer;
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    unsigned int tile, row, col;

    framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width < 240u || height < 400u) return;

    /* Clear every frame, including after the viewer is closed. */
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (!assetExtractorIsActive()) return;

    pixels = assetExtractorGetPixels(&tileCount);
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
                const size_t offset = framebufferOffset(x, y, height);
                /* BGR8 framebuffer: equal channels produce neutral grayscale. */
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
    consoleSelect(NULL);
    consoleClear();
}

void rendererDraw(const GameState *state)
{
    size_t tileCount = 0u;
    (void)state;
    (void)assetExtractorGetPixels(&tileCount);

    /* Short lines fit the bottom-screen console without wrapping off-screen. */
    printf("EXCITEBIKE 3DS / ASSET LOG\n");
    printf("------------------------------\n");
    printf("CHR: %.42s\n", assetExtractorGetStatus());
    printf("Tiles: %lu / 960\n", (unsigned long)tileCount);
    printf("ROM: rom.nes beside 3DSX\n");
    printf("Cache: extracted/chr_pixels.bin\n");
    printf("Viewer: %s\n", assetExtractorIsActive() ? "OPEN" : "closed");
    printf("Y: toggle atlas\n");
    printf("X: retry extraction (atlas open)\n");
    printf("B: close atlas\n");
    printf("Atlas = raw CHR tiles, not the menu.\n");
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
