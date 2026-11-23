#ifndef EXCITEBIKE_GFX_C3D_H
#define EXCITEBIKE_GFX_C3D_H

#include <stdint.h>
#include "native_scene.h"

/*
 * citro3d backend: draws a SceneDrawList on the top screen.
 * libctru's gfx and the bottom-screen console must already be initialised.
 */
int gfxC3dInit(void);
void gfxC3dExit(void);

/* Drops the cached atlas (call when the extracted assets were reloaded). */
void gfxC3dInvalidateAtlas(void);

/*
 * Draws one frame. When the list needs a palette preset the atlas does not
 * hold yet, the atlas is baked, swizzled and uploaded first. A NULL assets
 * pointer or an empty list shows a black frame.
 */
void gfxC3dPresent(const NativeMenuAssets *assets, const SceneDrawList *list);

#endif
