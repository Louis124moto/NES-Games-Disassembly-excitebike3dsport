#ifndef EXCITEBIKE_NATIVE_SCENE_H
#define EXCITEBIKE_NATIVE_SCENE_H

#include <stddef.h>
#include <stdint.h>
#include "native_menu.h"
#include "native_menu_assets.h"
#include "native_race_session.h"

/*
 * Platform-independent scene description for the native renderer.
 *
 * The game never touches pixels. It turns semantic state (menu, race session,
 * extracted tile/palette assets) into a SceneDrawList: ordered textured quads
 * referencing one tile atlas. The citro3d backend (gfx_c3d.c) uploads the
 * atlas and draws the list; the host golden test rasterizes the same list in
 * software and compares it with the original software renderer.
 *
 * Quad coordinates live in the 256x240 game area. The backend centres that
 * area on the 400x240 top screen (SCENE_SCREEN_OFFSET_X).
 */
#define SCENE_GAME_WIDTH 256u
#define SCENE_GAME_HEIGHT 240u
#define SCENE_SCREEN_OFFSET_X 72u

/* Atlas: 12 variants of the 512 source tiles, RGBA8 (0xRRGGBBAA), 512x1024. */
#define SCENE_ATLAS_WIDTH 512u
#define SCENE_ATLAS_HEIGHT 1024u
#define SCENE_ATLAS_TILES 512u
#define SCENE_ATLAS_TILES_PER_ROW 32u
#define SCENE_ATLAS_VARIANT_WIDTH 256u
#define SCENE_ATLAS_VARIANT_HEIGHT 128u

/* Variant layout: background tiles keep colour 0 (backdrop), keyed tiles and
 * sprites treat colour 0 as transparent. */
#define SCENE_VARIANT_BACKGROUND 0u /* 0..3: background subpalette 0..3 */
#define SCENE_VARIANT_KEYED 4u      /* 4..7: background subpalette, colour 0 transparent */
#define SCENE_VARIANT_SPRITE 8u     /* 8..11: sprite palette 0..3 */
#define SCENE_VARIANT_COUNT 12u

#define SCENE_MAX_QUADS 1280u

typedef struct SceneQuad {
    int16_t x, y;      /* destination, game area pixels */
    uint8_t w, h;      /* destination size in pixels */
    /* Source edges in atlas texels. u0 > u1 (or v0 > v1) mirrors the quad. */
    uint16_t u0, v0, u1, v1;
} SceneQuad;

typedef struct SceneDrawList {
    uint8_t palette_set; /* palette preset the atlas must be baked with */
    uint16_t count;
    SceneQuad quads[SCENE_MAX_QUADS];
} SceneDrawList;

/* Builds the front-end scene. Returns 1 when a complete scene was produced;
 * on 0 the list is empty (black screen). */
int nativeSceneBuildMenu(SceneDrawList *list, const NativeMenuAssets *assets,
                         const NativeMenuState *menu);

/* Builds the race scene (scrolling course, HUD, result times, OAM sprites). */
int nativeSceneBuildRace(SceneDrawList *list, const NativeMenuAssets *assets,
                         const NativeMenuState *menu,
                         const NativeRaceSession *session);

void nativeSceneRaceReset(void);
void nativeSceneBuildResultsPodium(NativeRaceSession *session);

/* Bakes the 512x1024 RGBA8 atlas (SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT
 * uint32_t, row-major, top-down) for one extracted palette preset. */
void nativeSceneBakeAtlas(uint32_t *atlas, const NativeMenuAssets *assets,
                          unsigned paletteSet);

/* Converts the row-major atlas to the PICA200 texture layout: 8x8 tiles in
 * row-major order, Morton (Z-order, x in the low bit) inside each tile. */
void nativeSceneSwizzleAtlas(uint32_t *dst, const uint32_t *src);

/* Index of texel (x,y) inside the swizzled atlas. */
size_t nativeSceneSwizzledIndex(unsigned x, unsigned y);

#endif
