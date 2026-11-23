#ifndef EXCITEBIKE_RENDERER_H
#define EXCITEBIKE_RENDERER_H

struct NativeMenuAssets;
struct SceneDrawList;

/* Initialises libctru gfx, the bottom-screen text console and the citro3d
 * top-screen renderer. Returns 1 on success. */
int rendererInit(void);

/* Draws one top-screen frame from a scene description (black when empty). */
void rendererDrawScene(const struct NativeMenuAssets *assets, const struct SceneDrawList *list);

/* Call after the extracted assets were (re)loaded so the tile atlas is rebuilt. */
void rendererInvalidateAssets(void);

/* Replaces the text on the bottom screen; identical text is not redrawn. */
void rendererPrintBottom(const char *text);

void rendererExit(void);

#endif
