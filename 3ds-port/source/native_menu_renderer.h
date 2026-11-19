#ifndef EXCITEBIKE_NATIVE_MENU_RENDERER_H
#define EXCITEBIKE_NATIVE_MENU_RENDERER_H

#include "native_menu.h"
#include "native_menu_assets.h"

/* Draws source-derived native tile/sprite assets directly to the 3DS top framebuffer.
 * Returns 1 only when a complete supported front-end scene was drawn.
 */
int nativeMenuRenderFrame(const NativeMenuAssets *assets, const NativeMenuState *menu);

#endif
