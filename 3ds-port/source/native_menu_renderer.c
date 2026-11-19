#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <3ds.h>
#include "native_menu_renderer.h"
#include "nes_ppu_render.h"

static size_t framebufferOffset(unsigned int x, unsigned int y)
{
    return (((size_t)x * 240u) + (239u - y)) * 3u;
}

static const NativeMenuSceneAssets *sceneForState(const NativeMenuAssets *assets,
                                                   uint8_t state)
{
    if (state == 0u) return &assets->scenes[0];
    if (state == 1u) return &assets->scenes[1];
    if (state == 8u) return &assets->scenes[2];
    return NULL;
}

static void putPixel(uint8_t *framebuffer, unsigned int x, unsigned int y,
                     uint8_t color)
{
    uint8_t rgb[3];
    size_t offset;
    if (x >= 400u || y >= 240u || !nesPpuColorToRgb((uint8_t)(color & 0x3Fu), rgb)) return;
    offset = framebufferOffset(x, y);
    framebuffer[offset] = rgb[2];
    framebuffer[offset + 1u] = rgb[1];
    framebuffer[offset + 2u] = rgb[0];
}

static void drawBackground(uint8_t *framebuffer, const NativeMenuAssets *assets,
                           const NativeMenuSceneAssets *scene)
{
    unsigned int tileY, tileX, pixelY, pixelX;
    for (tileY = 0u; tileY < 30u; ++tileY) {
        for (tileX = 0u; tileX < 32u; ++tileX) {
            const size_t mapIndex = (size_t)tileY * 32u + tileX;
            const uint8_t tile = scene->nametable[mapIndex];
            const size_t attributeIndex = 960u + (size_t)(tileY / 4u) * 8u + tileX / 4u;
            const uint8_t attribute = scene->nametable[attributeIndex];
            const unsigned int shift = (unsigned int)(((tileY & 2u) << 1u) | (tileX & 2u));
            const unsigned int subpalette = (attribute >> shift) & 3u;
            const uint8_t *tilePixels;
            if (tile >= assets->chr_tile_count) continue;
            tilePixels = assets->chr_pixels + (size_t)tile * 64u;
            for (pixelY = 0u; pixelY < 8u; ++pixelY) {
                for (pixelX = 0u; pixelX < 8u; ++pixelX) {
                    const uint8_t value = (uint8_t)(tilePixels[pixelY * 8u + pixelX] & 3u);
                    const size_t paletteIndex = value == 0u ? 0u : (size_t)subpalette * 4u + value;
                    const unsigned int screenX = 72u + tileX * 8u + pixelX;
                    const unsigned int screenY = tileY * 8u + pixelY;
                    if (assets->palette_written[paletteIndex] != 0u) {
                        putPixel(framebuffer, screenX, screenY, assets->palette[paletteIndex]);
                    }
                }
            }
        }
    }
}

static void drawCursor(uint8_t *framebuffer, const NativeMenuAssets *assets,
                       const NativeMenuState *menu)
{
    unsigned int x, y;
    const unsigned int left = menu->cursor_sprite_x;
    const unsigned int top = (unsigned int)menu->cursor_sprite_y + 1u;
    const uint8_t attributes = menu->cursor_sprite_attribute;
    const size_t paletteBase = 16u + (size_t)(attributes & 3u) * 4u;
    const uint8_t *pixels;
    if (menu->cursor_sprite_y == 0xF8u || menu->cursor_sprite_tile == 0xF8u ||
        menu->cursor_sprite_x == 0xF8u || menu->cursor_sprite_tile >= assets->chr_tile_count) return;
    pixels = assets->chr_pixels + (size_t)menu->cursor_sprite_tile * 64u;
    for (y = 0u; y < 8u; ++y) {
        const unsigned int sourceY = (attributes & 0x80u) != 0u ? 7u - y : y;
        for (x = 0u; x < 8u; ++x) {
            const unsigned int sourceX = (attributes & 0x40u) != 0u ? 7u - x : x;
            const uint8_t value = (uint8_t)(pixels[sourceY * 8u + sourceX] & 3u);
            const size_t paletteIndex = paletteBase + value;
            if (value == 0u || assets->palette_written[paletteIndex] == 0u) continue;
            putPixel(framebuffer, 72u + left + x, top + y, assets->palette[paletteIndex]);
        }
    }
}

int nativeMenuRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu)
{
    u16 width = 0u, height = 0u;
    uint8_t *framebuffer;
    const NativeMenuSceneAssets *scene;
    if (assets == NULL || menu == NULL || assets->loaded == 0u ||
        assets->chr_pixels == NULL || assets->chr_tile_count == 0u) return 0;
    scene = sceneForState(assets, menu->source_state);
    if (scene == NULL) return 0;
    framebuffer = (uint8_t *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
    if (framebuffer == NULL || width != 240u || height != 400u) return 0;
    memset(framebuffer, 0, (size_t)width * (size_t)height * 3u);
    drawBackground(framebuffer, assets, scene);
    drawCursor(framebuffer, assets, menu);
    return 1;
}
