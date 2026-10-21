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

/* 64-colour NES master-palette RGB approximation, indexed by NES colour code. */
static const uint8_t s_nesMasterPalette[64][3] = {
    {84,84,84},{0,30,116},{8,16,144},{48,0,136},{68,0,100},{92,0,48},{84,4,0},{60,24,0},
    {32,42,0},{8,58,0},{0,64,0},{0,60,0},{0,50,60},{0,0,0},{0,0,0},{0,0,0},
    {152,150,152},{8,76,196},{48,50,236},{92,30,228},{136,20,176},{160,20,100},{152,34,32},{120,60,0},
    {84,90,0},{40,114,0},{8,124,0},{0,118,40},{0,102,120},{0,0,0},{0,0,0},{0,0,0},
    {236,238,236},{76,154,236},{120,124,236},{176,98,236},{228,84,236},{236,88,180},{236,106,100},{212,136,32},
    {160,170,0},{116,196,0},{76,208,32},{56,204,108},{56,180,204},{60,60,60},{0,0,0},{0,0,0},
    {236,238,236},{168,204,236},{188,188,236},{212,178,236},{236,174,236},{236,174,212},{236,180,176},{228,196,144},
    {204,210,120},{180,222,120},{168,226,144},{152,226,180},{160,214,228},{160,162,160},{0,0,0},{0,0,0}
};

/* Palette bytes recovered from the original bank_FF.asm palette streams. */
static const uint8_t s_gamePaletteSets[5][2][4] = {
    {{0x29,0x27,0x22,0x30},{0x29,0x27,0x18,0x36}},
    {{0x01,0x1A,0x26,0x33},{0x01,0x11,0x0C,0x2C}},
    {{0x02,0x26,0x1C,0x30},{0x02,0x29,0x19,0x39}},
    {{0x00,0x26,0x22,0x30},{0x00,0x27,0x18,0x37}},
    {{0x02,0x19,0x22,0x30},{0x02,0x00,0x2D,0x10}}
};

static PrintConsole s_bottomConsole;
static char s_lastLog[512];
static int s_logInitialized;

static void rendererDrawChrAtlas(void)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    unsigned int tile, row, col, bank;

    /* On libctru top-screen framebuffer dimensions are 240x400. */
    if (framebuffer == NULL || width != 240u || height != 400u) return;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    if (!assetExtractorIsActive()) return;

    pixels = assetExtractorGetPixels(&tileCount);
    if (pixels == NULL || tileCount == 0u) return;
    if (tileCount > 960u) tileCount = 960u;
    bank = assetExtractorGetPalette() % 10u;

    for (tile = 0u; tile < tileCount; ++tile) {
        const unsigned int tileX = (tile % 50u) * 8u;
        const unsigned int tileY = (tile / 50u) * 8u;
        for (row = 0u; row < 8u; ++row) {
            for (col = 0u; col < 8u; ++col) {
                const unsigned int x = tileX + col, y = tileY + row;
                const uint8_t pixel = pixels[tile * 64u + row * 8u + col] & 3u;
                const uint8_t nesIndex = s_gamePaletteSets[bank / 2u][bank % 2u][pixel] & 0x3Fu;
                const uint8_t *rgb = s_nesMasterPalette[nesIndex];
                const size_t offset = topFramebufferOffset(x, y);
                framebuffer[offset + 0u] = rgb[2];
                framebuffer[offset + 1u] = rgb[1];
                framebuffer[offset + 2u] = rgb[0];
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
    (void)snprintf(logText, sizeof(logText),
        "EXCITEBIKE 3DS / ASSET LOG\n"
        "------------------------------\n"
        "CHR: %.36s\n"
        "Tiles: %lu / 960\n"
        "ROM: rom.nes beside 3DSX\n"
        "Cache: extracted/chr_pixels.bin\n"
        "Atlas: %s\n"
        "L/R: palette %u/10\n"
        "Y: toggle atlas\n"
        "X: retry extraction\n"
        "B: close atlas\n"
        "CHR tiles are raw patterns.\n",
        assetExtractorGetStatus(), (unsigned long)tileCount,
        assetExtractorIsActive() ? "OPEN" : "closed",
        assetExtractorGetPalette() + 1u);
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

void rendererExit(void)
{
    gfxExit();
}
