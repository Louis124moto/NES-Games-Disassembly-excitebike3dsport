#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"

/*
 * gfxGetFramebuffer reports the top screen as 240x400 memory coordinates,
 * rotated relative to its physical 400x240 presentation. Convert physical
 * screen (x,y) to framebuffer coordinates before applying the 8x8 tile layout.
 */
static size_t topFramebufferOffset(unsigned int screenX, unsigned int screenY)
{
    /* libctru exposes the rotated top framebuffer as 240x400.
       Physical screen (x,y) maps to framebuffer (y, 399-x). */
    const unsigned int fbX = screenY;
    const unsigned int fbY = 399u - screenX;
    const unsigned int tileY = 49u - (fbY >> 3u);
    return ((((fbX >> 3u) * 50u) + tileY) * 64u
            + (fbX & 7u) * 8u + (fbY & 7u)) * 3u;
}

/* Four-level grayscale intentionally exposes the decoded 2bpp pixel indices.
   The true per-tile game palette is not knowable without nametable attributes. */
static const uint8_t s_pixelGray[4] = {0u, 85u, 170u, 255u};

static PrintConsole s_bottomConsole;
static char s_lastLog[512];
static int s_logInitialized;
static int s_gameStarted;

static void rendererDrawChrAtlas(void)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    size_t firstTile;
    unsigned int tile, row, col;

    /* On libctru top-screen framebuffer dimensions are 240x400. */
    if (framebuffer == NULL || width != 240u || height != 400u) return;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (!assetExtractorIsActive() || s_gameStarted) return;

    pixels = assetExtractorGetPixels(&tileCount);
    if (pixels == NULL || tileCount == 0u) return;
    /* 2x nearest-neighbour zoom: 375 tiles per page, with D-pad paging. */
    firstTile = assetExtractorGetPage() * 375u;
    if (firstTile >= tileCount) return;
    tileCount -= firstTile;
    if (tileCount > 375u) tileCount = 375u;

    for (tile = 0u; tile < tileCount; ++tile) {
        const size_t sourceTile = firstTile + tile;
        const unsigned int tileX = (tile % 25u) * 16u;
        const unsigned int tileY = (tile / 25u) * 16u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const uint8_t pixel = pixels[sourceTile * 64u + row * 8u + col] & 3u;
                const uint8_t gray = s_pixelGray[pixel];
                unsigned int dy, dx;
                for (dy = 0u; dy < 2u; ++dy) {
                    for (dx = 0u; dx < 2u; ++dx) {
                        const unsigned int x = tileX + col * 2u + dx;
                        const unsigned int y = tileY + row * 2u + dy;
                        const size_t offset = topFramebufferOffset(x, y);
                        framebuffer[offset + 0u] = gray;
                        framebuffer[offset + 1u] = gray;
                        framebuffer[offset + 2u] = gray;
                    }
                }
            }
        }
    }
}

void rendererInit(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, &s_bottomConsole);
    consoleSelect(&s_bottomConsole);
    s_logInitialized = 0;
    s_gameStarted = 0;
    s_lastLog[0] = '\0';
}

void rendererBeginFrame(void)
{
    consoleSelect(&s_bottomConsole);
}

void rendererDraw(const GameState *state)
{
    char logText[512];
    size_t tileCount = 0u;
    (void)state;
    (void)assetExtractorGetPixels(&tileCount);
    if (s_gameStarted) {
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS\\n"
            "------------------------------\\n"
            "START received.\\n"
            "Native game mode entered.\\n"
            "Faithful NES menu/race runtime\\n"
            "is not integrated yet.\\n"
            "CHR tiles extracted: %lu\\n"
            "B: return to asset viewer\\n",
            (unsigned long)tileCount);
    } else {
        (void)snprintf(logText, sizeof(logText),
            "EXCITEBIKE 3DS / ASSET LOG\\n"
            "------------------------------\\n"
            "CHR: %.36s\\n"
            "Tiles: %lu / 960\\n"
            "ROM: rom.nes beside 3DSX\\n"
            "Cache: extracted/chr_pixels.bin\\n"
            "Atlas: %s\\n"
            "Page: %lu/%lu (D-pad left/right)\\n"
            "Y: toggle atlas; START: game mode\\n"
            "X: retry extraction; B: close atlas\\n"
            "Atlas uses grayscale 2bpp indices.\\n",
            assetExtractorGetStatus(), (unsigned long)tileCount,
            assetExtractorIsActive() ? "OPEN" : "closed",
            (unsigned long)(assetExtractorGetPage() + 1u),
            (unsigned long)((tileCount + 374u) / 375u));
    }
    if (!s_logInitialized || strcmp(logText, s_lastLog) != 0) {
        consoleClear();
        fputs(logText, stdout);
        (void)snprintf(s_lastLog, sizeof(s_lastLog), "%s", logText);
        s_logInitialized = 1;
    }
}

void rendererEndFrame(void)
{
    rendererDrawChrAtlas();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void rendererSetGameStarted(int started)
{
    s_gameStarted = started != 0;
    s_logInitialized = 0;
}

void rendererExit(void)
{
    gfxExit();
}
