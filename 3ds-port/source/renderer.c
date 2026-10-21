#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "renderer.h"
#include "asset_extractor.h"

/* libctru framebuffers use 8x8 tiled pixels, stored B,G,R. */
static size_t framebufferOffset(unsigned int x, unsigned int y, unsigned int height)
{
    const unsigned int flippedY = height - 1u - y;
    return ((((x >> 3u) * (height >> 3u)) + (flippedY >> 3u)) * 64u
            + (x & 7u) * 8u + (flippedY & 7u)) * 3u;
}

/*
 * Common 2C02/2C07-style 64-colour NES master-palette approximation.
 * RGB values represent hardware palette colours, not game-specific palette
 * RAM. The ROM's PPU palette writes still need to be traced for exact menus.
 */
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

/* Exact background sub-palettes from bank_FF.asm palette command streams:
 * $D3D6, $D3E9, $D3FC and $D40F each write 8 bytes at $3F00.
 * $D422 is another original background palette set. */
static const uint8_t s_gamePaletteSets[5][2][4] = {
    {{0x29,0x27,0x22,0x30},{0x29,0x27,0x18,0x36}}, /* _off_000_D3D6_12 */
    {{0x01,0x1A,0x26,0x33},{0x01,0x11,0x0C,0x2C}}, /* _off_000_D3E9_13 */
    {{0x02,0x26,0x1C,0x30},{0x02,0x29,0x19,0x39}}, /* _off_000_D3FC_14 */
    {{0x00,0x26,0x22,0x30},{0x00,0x27,0x18,0x37}}, /* _off_000_D40F_15 */
    {{0x02,0x19,0x22,0x30},{0x02,0x00,0x2D,0x10}}  /* _off_000_D422_16 */
};

static void rendererDrawChrAtlas(void)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    const uint8_t *pixels = NULL;
    size_t tileCount = 0u;
    unsigned int tile, row, col, bank;
    if (framebuffer == NULL || width < 400u || height < 240u) return;
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
                const size_t offset = framebufferOffset(x, y, height);
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
    printf("EXCITEBIKE 3DS / ASSET LOG\n");
    printf("------------------------------\n");
    printf("CHR: %.42s\n", assetExtractorGetStatus());
    printf("Tiles: %lu / 960\n", (unsigned long)tileCount);
    printf("ROM: rom.nes beside 3DSX\n");
    printf("Cache: extracted/chr_pixels.bin\n");
    printf("Viewer: %s\n", assetExtractorIsActive() ? "OPEN" : "closed");
    printf("L/R: NES palette preview %u\n", assetExtractorGetPalette() + 1u);
    printf("Y: toggle atlas\n");
    printf("X: retry extraction (atlas open)\n");
    printf("B: close atlas\n");
    printf("Preview palette; game palette pending.\n");
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
