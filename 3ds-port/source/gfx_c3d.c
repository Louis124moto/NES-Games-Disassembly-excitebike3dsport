#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>
#include "gfx_c3d.h"
#include "scene_shbin.h"

/* Top screen: 400x240, stored rotated (240 wide, 400 tall) by the hardware. */
#define TOP_SCREEN_WIDTH 400.0f
#define TOP_SCREEN_HEIGHT 240.0f
#define VERTICES_PER_QUAD 6u

typedef struct SceneVertex {
    float x, y, z;
    float u, v;
} SceneVertex;

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static int s_initialized;
static C3D_RenderTarget *s_target;
static DVLB_s *s_shader;
static shaderProgram_s s_program;
static int s_projLocation;
static C3D_AttrInfo s_attrInfo;
static C3D_BufInfo s_bufInfo;
static C3D_Mtx s_projection;
static SceneVertex *s_vertices;
static C3D_Tex s_atlas;
static int s_atlasAllocated;
static int s_atlasPaletteSet = -1;

int gfxC3dInit(void)
{
    if (s_initialized) return 1;
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return 0;

    s_target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH16);
    if (s_target == NULL) { C3D_Fini(); return 0; }
    C3D_RenderTargetSetOutput(s_target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    s_shader = DVLB_ParseFile((u32 *)scene_shbin, scene_shbin_size);
    if (s_shader == NULL) { C3D_Fini(); return 0; }
    shaderProgramInit(&s_program);
    shaderProgramSetVsh(&s_program, &s_shader->DVLE[0]);
    s_projLocation = shaderInstanceGetUniformLocation(s_program.vertexShader, "projMtx");

    s_vertices = (SceneVertex *)linearAlloc(sizeof(SceneVertex) * VERTICES_PER_QUAD * SCENE_MAX_QUADS);
    if (s_vertices == NULL) {
        shaderProgramFree(&s_program);
        DVLB_Free(s_shader);
        C3D_Fini();
        return 0;
    }

    AttrInfo_Init(&s_attrInfo);
    AttrInfo_AddLoader(&s_attrInfo, 0, GPU_FLOAT, 3); /* v0 = position */
    AttrInfo_AddLoader(&s_attrInfo, 1, GPU_FLOAT, 2); /* v1 = texcoord */
    BufInfo_Init(&s_bufInfo);
    BufInfo_Add(&s_bufInfo, s_vertices, sizeof(SceneVertex), 2, 0x10);

    /* Pixel-exact 2D view of the 400x240 panel (same setup as citro2d). */
    Mtx_OrthoTilt(&s_projection, 0.0f, TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT, 0.0f, 1.0f, -1.0f, true);

    s_atlasPaletteSet = -1;
    s_initialized = 1;
    return 1;
}

void gfxC3dExit(void)
{
    if (!s_initialized) return;
    if (s_atlasAllocated) {
        C3D_TexDelete(&s_atlas);
        s_atlasAllocated = 0;
    }
    linearFree(s_vertices);
    s_vertices = NULL;
    shaderProgramFree(&s_program);
    DVLB_Free(s_shader);
    s_shader = NULL;
    C3D_Fini();
    s_initialized = 0;
}

void gfxC3dInvalidateAtlas(void)
{
    s_atlasPaletteSet = -1;
}

/* Bakes, swizzles and uploads the atlas for one palette preset. */
static int uploadAtlas(const NativeMenuAssets *assets, unsigned paletteSet)
{
    const size_t bytes = (size_t)SCENE_ATLAS_WIDTH * SCENE_ATLAS_HEIGHT * sizeof(uint32_t);
    uint32_t *linear = (uint32_t *)malloc(bytes);
    uint32_t *tiled = (uint32_t *)malloc(bytes);
    int ok = 0;
    if (linear != NULL && tiled != NULL) {
        if (!s_atlasAllocated) {
            if (C3D_TexInit(&s_atlas, SCENE_ATLAS_WIDTH, SCENE_ATLAS_HEIGHT, GPU_RGBA8)) {
                C3D_TexSetFilter(&s_atlas, GPU_NEAREST, GPU_NEAREST);
                C3D_TexSetWrap(&s_atlas, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
                s_atlasAllocated = 1;
            }
        }
        if (s_atlasAllocated) {
            nativeSceneBakeAtlas(linear, assets, paletteSet);
            nativeSceneSwizzleAtlas(tiled, linear);
            C3D_TexLoadImage(&s_atlas, tiled, GPU_TEXFACE_2D, 0);
            ok = 1;
        }
    }
    free(linear);
    free(tiled);
    return ok;
}

static void fillVertices(const SceneDrawList *list)
{
    const float uScale = 1.0f / (float)SCENE_ATLAS_WIDTH;
    const float vScale = 1.0f / (float)SCENE_ATLAS_HEIGHT;
    unsigned i;
    SceneVertex *out = s_vertices;
    for (i = 0u; i < list->count; ++i) {
        const SceneQuad *q = &list->quads[i];
        const float x0 = (float)((int)SCENE_SCREEN_OFFSET_X + q->x);
        const float y0 = (float)q->y;
        const float x1 = x0 + (float)q->w;
        const float y1 = y0 + (float)q->h;
        const float u0 = (float)q->u0 * uScale, u1 = (float)q->u1 * uScale;
        const float v0 = (float)q->v0 * vScale, v1 = (float)q->v1 * vScale;
        out[0] = (SceneVertex){ x0, y0, 0.0f, u0, v0 };
        out[1] = (SceneVertex){ x1, y0, 0.0f, u1, v0 };
        out[2] = (SceneVertex){ x0, y1, 0.0f, u0, v1 };
        out[3] = (SceneVertex){ x1, y0, 0.0f, u1, v0 };
        out[4] = (SceneVertex){ x1, y1, 0.0f, u1, v1 };
        out[5] = (SceneVertex){ x0, y1, 0.0f, u0, v1 };
        out += VERTICES_PER_QUAD;
    }
    GSPGPU_FlushDataCache(s_vertices, (size_t)list->count * VERTICES_PER_QUAD * sizeof(SceneVertex));
}

void gfxC3dPresent(const NativeMenuAssets *assets, const SceneDrawList *list)
{
    const int hasScene = assets != NULL && list != NULL && list->count != 0u;
    C3D_TexEnv *env;
    if (!s_initialized) return;

    if (hasScene && s_atlasPaletteSet != (int)list->palette_set) {
        s_atlasPaletteSet = uploadAtlas(assets, list->palette_set) ? (int)list->palette_set : -1;
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW); /* waits for the previous frame and vblank */
    C3D_RenderTargetClear(s_target, C3D_CLEAR_ALL, 0x000000FFu, 0u);
    C3D_FrameDrawOn(s_target);
    if (hasScene && s_atlasPaletteSet >= 0) {
        C3D_BindProgram(&s_program);
        C3D_SetAttrInfo(&s_attrInfo);
        C3D_SetBufInfo(&s_bufInfo);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s_projLocation, &s_projection);

        /* Texel colour straight to the framebuffer; transparent texels are discarded. */
        env = C3D_GetTexEnv(0);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
        C3D_AlphaTest(true, GPU_GREATER, 0);
        C3D_CullFace(GPU_CULL_NONE);
        C3D_TexBind(0, &s_atlas);

        fillVertices(list);
        C3D_DrawArrays(GPU_TRIANGLES, 0, (int)(list->count * VERTICES_PER_QUAD));
    }
    C3D_FrameEnd(0);
}
