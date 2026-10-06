/* Bounded F3D renderer stage: clears and flat shaded menu geometry. */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rg_display.h>
#include <rg_system.h>
#include <rg_utils.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_cpu.h>

#include "gfx_rg.h"
#include "gfx_rg_rows.h"
#include "port.h"
#include "diagnostic.h"
#include "main.h"
#include "port.h"
#include "recipe_index.h"
#include "resident.h"
#include "buffers.h"
#include "racing/skybox_and_splitscreen.h"
#include "assets/common_data.h"
#include "code_80057C60.h"

/* Performance experiment: retain the game's 320x240 logical space.
 * Scale only raster output; use 1 to restore full-resolution rendering. */
#ifndef MK64_RENDER_SCALE
#define MK64_RENDER_SCALE 2
#endif
_Static_assert(MK64_RENDER_SCALE == 1 || MK64_RENDER_SCALE == 2,
               "Supported raster scales are 1 and 2");
#define MK64_WORLD_WIDTH (320 / MK64_RENDER_SCALE)
#define MK64_WORLD_HEIGHT (240 / MK64_RENDER_SCALE)
#define MK64_OUTPUT_WIDTH 320
#define MK64_OUTPUT_HEIGHT 240
/* Runtime dimensions change only at the explicit world/HUD boundary. */
#define MK64_SCREEN_WIDTH sRasterWidth
#define MK64_SCREEN_HEIGHT sRasterHeight
#define MK64_DL_BUDGET 8192
#define MK64_DL_DEPTH 12
#define MK64_VERTEX_SLOTS 64
#define MK64_MATRIX_STACK 16

/* Enable only when investigating an individual graphics command. */
#ifndef MK64_RG_VERBOSE_GFX
#define MK64_RG_VERBOSE_GFX 0
#endif

/* F3D_OLD declares immediate opcodes as negative signed values. */
_Static_assert((uint8_t)G_ENDDL == 0xb8, "Unexpected F3D end opcode");

extern struct GfxPool gGfxPools[2];
extern uintptr_t gPhysicalZBuffer;
extern int8_t gTLUTRedShell[512];
extern void *port_seg_to_ptr(uintptr_t address);

static rg_surface_t *sSurface;
static rg_surface_t *sBackSurface;
static rg_surface_t *sCompleteSurface;
static unsigned sCompleteFrame;
#define MK64_CAPTURE_SLOTS 8
typedef struct { void *target; unsigned frame; int x, y, width, height; bool valid; } RGCapture;
static RGCapture sCaptures[MK64_CAPTURE_SLOTS];
static unsigned sCaptureSlot;
static Mk64CaptureStats sCaptureStats;
static bool sBackSurfaceTried;
static uint16_t *sDepth;
static rg_surface_t *sOutputSurface, *sOutputBackSurface;
static bool sOutputTried, sHighResolution, sUiDepthCleared;
static uint16_t *sUiDepth, *sRasterDepth, *sRasterPixels;
static unsigned sRasterWidth = MK64_WORLD_WIDTH;
static unsigned sRasterHeight = MK64_WORLD_HEIGHT;
static unsigned sRasterScale = MK64_RENDER_SCALE;
static uint16_t sFillColor;
static uint32_t sPrimitiveColor;
static uint32_t sEnvironmentColor;
static unsigned sCycleType;
static uint32_t sOtherModeL;
static uint32_t sGeometryMode;
static const uint16_t *sTextureImage;
static unsigned sTextureWidth, sTextureHeight;
static unsigned sTextureModeS, sTextureModeT;
static unsigned sTextureFormat, sTextureSize;
static uint16_t sTexturePalette[256];
static unsigned sTexturePaletteBank, sPaletteEntries, sPaletteLoadOffset;
static unsigned sImageWidth;
static const uint8_t *sLoadedImage;
static unsigned sLoadedStride, sLoadedWidth, sLoadedHeight;
static unsigned sLoadedOriginS, sLoadedOriginT;
static unsigned sLoadedNibbleOffset;
static unsigned sLoadedSize;
static bool sLoadedTile;
static unsigned sTextureRects, sSkippedTextureRects, sTexturePixels;
static unsigned sRectColorSource, sRectAlphaSource;
static struct { unsigned rgb[4], alpha[4]; } sTriangleCombiner[2];
static bool sTextureEnabled;
static float sTextureScaleS, sTextureScaleT;
static Light_t sLights[8];
static unsigned sNumLights;
static unsigned sLightLoads;
static bool sColorTarget;
static unsigned sCommands;
static unsigned sInvalidLists;
static unsigned sUnclosedLists;
static unsigned sFillRects;
static unsigned sVertexLoads;
static unsigned sTriangles;
static unsigned sVisibleTriangles;
static unsigned sBadGeometry;
static bool sTraceGeometry;
static bool sTraceAssets;
static bool sTraceBorders;
static unsigned sFrame;
static unsigned sRasterFrames;
/* Sample periodically. Buckets exclude
 * display-list calls so nested execution is never counted twice. */
enum { PERF_TRIANGLE, PERF_RECTANGLE, PERF_VERTEX, PERF_MATRIX,
       PERF_TEXTURE, PERF_FILL, PERF_BUCKETS };
static bool sProfileFrame;
static unsigned sPerfUs[PERF_BUCKETS];
static unsigned sBeginUs, sDisplayWaitUs, sWalkUs, sPixelUs;
static unsigned sCompositionUs, sWorldTriangleUs, sWorldRectangleUs;
static unsigned sBoxPixels, sWrittenPixels, sEarlyDepthRejects, sFastPixels;
static unsigned sSpanPixels;
static unsigned sWhitePixels;
static unsigned sLookupPixels;
static unsigned sSkippedAttributeSteps;
static unsigned sUniformShaderReuse;
static unsigned sDecalPixels;
/* One in 64 covered fragments, only on diagnostic frames excluded by the
 * quiet CPU profiler. Raw cycles avoid timer resolution loss; interrupt time
 * and counter-read overhead remain included. Never extrapolate these samples
 * as exact full-frame totals. */
static unsigned sPixelProbeCursor;
static RGPixelProbe sPixelProbe[PIX_STAGE_COUNT];

static void record_pixel_probe(unsigned stage, uint32_t started)
{
    uint32_t elapsed = esp_cpu_get_cycle_count() - started;
    sPixelProbe[stage].cycles += elapsed;
    ++sPixelProbe[stage].samples;
}
static uint8_t *sModulateTable;
static bool sModulateTableTried;
static uint32_t *sTextureCache;
static unsigned sTextureCacheCount, sTextureCacheHits, sTextureCacheMisses;
static bool sTextureCacheTried;
/* Sample visibility failures without changing coverage/depth decisions. */
static unsigned sClipRejects[7], sAlphaRejects, sTextureSampleFailures;
enum { COMBINE_GENERAL, COMBINE_DIRECT, COMBINE_MODULATE, COMBINE_MODULATE_WHITE,
       COMBINE_MODULATE_DECAL, COMBINE_MODULATE_DECAL_WHITE };

static int profile_bucket(unsigned opcode)
{
    switch (opcode) {
        case (uint8_t)G_TRI1: case (uint8_t)G_TRI2: case (uint8_t)G_QUAD:
            return PERF_TRIANGLE;
        case G_TEXRECT: case G_TEXRECTFLIP: return PERF_RECTANGLE;
        case G_VTX: return PERF_VERTEX;
        case G_MTX: case (uint8_t)G_POPMTX: return PERF_MATRIX;
        case G_LOADTLUT: case G_LOADTILE: case G_LOADBLOCK: return PERF_TEXTURE;
        case G_FILLRECT: return PERF_FILL;
        default: return -1;
    }
}

static unsigned sSpriteTraces;
static unsigned sSpriteSetupTraces;
static bool sTraceSegmentB;
static unsigned sTextureImages;
static unsigned sTexturedTriangles;
static unsigned sDepthTriangles;
static unsigned sCulledTriangles;
static unsigned sClippedTriangles;
static unsigned sBlendedTriangles;
static unsigned sSegmentBLists;
static unsigned sLightColorCommands;
static unsigned sSkippedUnmappedLists;
static unsigned sOpcodeCounts[256];
static float sProjection[4][4];
static float sModelview[MK64_MATRIX_STACK][4][4];
static unsigned sMatrixTop;
typedef struct {
    float x, y, z, w;
    float u, v;
    uint8_t red, green, blue, alpha;
    bool valid;
} RGVertex;
static RGVertex sVertices[MK64_VERTEX_SLOTS];

static bool in_span(uintptr_t address, uintptr_t base, size_t size)
{
    return address >= base && size >= sizeof(Gfx) &&
           address - base <= size - sizeof(Gfx);
}

static bool in_byte_span(uintptr_t address, size_t length, uintptr_t base, size_t size)
{
    return address >= base && length <= size && address - base <= size - length;
}

static bool readable_data(const void *pointer, size_t length)
{
    uintptr_t address = (uintptr_t)pointer;
    if (port_skybox_readable(pointer, length))
        return true;
    if (in_byte_span(address, length, (uintptr_t)gMk64AssetRegion,
                     MK64_ASSET_REGION_SIZE) ||
        in_byte_span(address, length, (uintptr_t)gPortMemoryPool,
                     PORT_MEMORY_POOL_SIZE) ||
        in_byte_span(address, length, (uintptr_t)&D_802BFB80, sizeof(D_802BFB80)) ||
        in_byte_span(address, length, (uintptr_t)D_80183FA8, sizeof(D_80183FA8)) ||
        in_byte_span(address, length, (uintptr_t)gPlayerPalettesList,
                     sizeof(gPlayerPalettesList[0]) * 2) ||
        in_byte_span(address, length, (uintptr_t)gTLUTRedShell, sizeof(gTLUTRedShell)))
        return true;
    for (unsigned i = 0; i < 2; ++i)
        if (in_byte_span(address, length, (uintptr_t)&gGfxPools[i],
                         sizeof(gGfxPools[i])))
            return true;
    return false;
}

static void matrix_identity(float matrix[4][4])
{
    memset(matrix, 0, 16 * sizeof(float));
    for (unsigned i = 0; i < 4; ++i)
        matrix[i][i] = 1.0f;
}

static void matrix_multiply(float result[4][4], const float left[4][4],
                            const float right[4][4])
{
    float product[4][4];
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col) {
            product[row][col] = 0.0f;
            for (unsigned k = 0; k < 4; ++k)
                product[row][col] += left[row][k] * right[k][col];
        }
    memcpy(result, product, sizeof(product));
}

static void load_matrix(uint32_t w0, uint32_t w1)
{
    const uint32_t *source = port_seg_to_ptr(w1);
    if (!source || !readable_data(source, sizeof(Mtx))) {
        ++sBadGeometry;
        return;
    }
    float input[4][4];
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; col += 2) {
            unsigned index = row * 2 + col / 2;
            uint32_t high = source[index];
            uint32_t low = source[8 + index];
            input[row][col] = (int32_t)((high & 0xffff0000u) |
                                         (low >> 16)) / 65536.0f;
            input[row][col + 1] = (int32_t)((high << 16) |
                                             (low & 0xffffu)) / 65536.0f;
        }
    unsigned flags = (w0 >> 16) & 0xff;
    float (*target)[4] = sProjection;
    if (!(flags & G_MTX_PROJECTION)) {
        if (flags & G_MTX_PUSH) {
            if (sMatrixTop + 1 >= MK64_MATRIX_STACK) {
                ++sBadGeometry;
                return;
            }
            memcpy(sModelview[sMatrixTop + 1], sModelview[sMatrixTop],
                   sizeof(sModelview[0]));
            ++sMatrixTop;
        }
        target = sModelview[sMatrixTop];
    }
    if (flags & G_MTX_LOAD)
        memcpy(target, input, sizeof(input));
    else
        matrix_multiply(target, input, target);
}

static void load_vertices(uint32_t w0, uint32_t w1)
{
    unsigned count = (w0 >> 10) & 0x3f;
    unsigned first = ((w0 >> 16) & 0xff) / 2;
    const Vtx *source = port_seg_to_ptr(w1);
    bool valid_range = first < MK64_VERTEX_SLOTS &&
                       count <= MK64_VERTEX_SLOTS - first;
    bool readable = count && source &&
                    readable_data(source, count * sizeof(Vtx));
    if (sTraceGeometry)
        do { RG_LOGD("MK64 vertex cmd %08x %08x: first %u count %u ptr %p "
                "range %u readable %u", (unsigned)w0, (unsigned)w1,
                first, count, source, valid_range, readable); } while (0);
    if ((!count || !valid_range || !readable) && sTraceAssets &&
        sBadGeometry < 8)
        RG_LOGW("MK64 vertex reject: %08x %08x first %u count %u "
                "ptr %p range %u readable %u", (unsigned)w0, (unsigned)w1,
                first, count, source, valid_range, readable);
    if (!count || !valid_range || !readable) {
        if (valid_range)
            for (unsigned i = 0; i < count; ++i)
                sVertices[first + i].valid = false;
        ++sBadGeometry;
        return;
    }
    float combined[4][4];
    matrix_multiply(combined, sModelview[sMatrixTop], sProjection);
    float directions[7][3];
    if (sGeometryMode & G_LIGHTING)
        for (unsigned light = 0; light + 1 < sNumLights; ++light) {
            const Light_t *source_light = &sLights[light];
            float vector[3] = {source_light->dir[0] / 127.0f,
                               source_light->dir[1] / 127.0f,
                               source_light->dir[2] / 127.0f};
            float length = 0.0f;
            for (unsigned axis = 0; axis < 3; ++axis) {
                directions[light][axis] =
                    vector[0] * sModelview[sMatrixTop][axis][0] +
                    vector[1] * sModelview[sMatrixTop][axis][1] +
                    vector[2] * sModelview[sMatrixTop][axis][2];
                length += directions[light][axis] * directions[light][axis];
            }
            if (length > 0.00001f) {
                float scale = 1.0f / sqrtf(length);
                for (unsigned axis = 0; axis < 3; ++axis)
                    directions[light][axis] *= scale;
            }
        }
    for (unsigned i = 0; i < count; ++i) {
        const Vtx_t *vertex = &source[i].v;
        float x = vertex->ob[0], y = vertex->ob[1], z = vertex->ob[2];
        unsigned slot = first + i;
        sVertices[slot].x = x * combined[0][0] + y * combined[1][0] +
                            z * combined[2][0] + combined[3][0];
        sVertices[slot].y = x * combined[0][1] + y * combined[1][1] +
                            z * combined[2][1] + combined[3][1];
        sVertices[slot].z = x * combined[0][2] + y * combined[1][2] +
                            z * combined[2][2] + combined[3][2];
        sVertices[slot].w = x * combined[0][3] + y * combined[1][3] +
                            z * combined[2][3] + combined[3][3];
        sVertices[slot].valid = isfinite(sVertices[slot].x) &&
                                isfinite(sVertices[slot].y) &&
                                isfinite(sVertices[slot].z) &&
                                isfinite(sVertices[slot].w);
        sVertices[slot].alpha = vertex->cn[3];
        if (sGeometryMode & G_LIGHTING) {
            const Vtx_tn *normal = &source[i].n;
            float channels[3] = {sLights[sNumLights - 1].col[0],
                                 sLights[sNumLights - 1].col[1],
                                 sLights[sNumLights - 1].col[2]};
            for (unsigned light = 0; light + 1 < sNumLights; ++light) {
                float dot = normal->n[0] * directions[light][0] +
                            normal->n[1] * directions[light][1] +
                            normal->n[2] * directions[light][2];
                if (dot <= 0.0f) continue;
                dot /= 127.0f;
                for (unsigned channel = 0; channel < 3; ++channel)
                    channels[channel] += dot * sLights[light].col[channel];
            }
            sVertices[slot].red = (uint8_t)fminf(channels[0], 255.0f);
            sVertices[slot].green = (uint8_t)fminf(channels[1], 255.0f);
            sVertices[slot].blue = (uint8_t)fminf(channels[2], 255.0f);
            if (sGeometryMode & G_TEXTURE_GEN) {
                /* One tile cycle until tile shift and masking are modeled. */
                sVertices[slot].u = (normal->n[0] / 127.0f + 1.0f) * 16.0f;
                sVertices[slot].v = (normal->n[1] / 127.0f + 1.0f) * 16.0f;
            }
        } else {
            sVertices[slot].red = vertex->cn[0];
            sVertices[slot].green = vertex->cn[1];
            sVertices[slot].blue = vertex->cn[2];
        }
        if (!(sGeometryMode & G_TEXTURE_GEN)) {
            sVertices[slot].u = vertex->tc[0] * sTextureScaleS / 32.0f;
            sVertices[slot].v = vertex->tc[1] * sTextureScaleT / 32.0f;
        }
    }
    ++sVertexLoads;
}

static Gfx *list_end(Gfx *command)
{
    uintptr_t address = (uintptr_t)command;
    if (address % _Alignof(Gfx))
        return NULL;
    uintptr_t base = (uintptr_t)gMk64AssetRegion;
    if (in_span(address, base, MK64_ASSET_REGION_SIZE)) {
        uint32_t offset = (uint32_t)(address - base);
        size_t low = 0, high = mk64_recipe_count;
        while (low < high) {
            size_t middle = low + (high - low) / 2;
            if (mk64_recipes[mk64_recipe_dst_order[middle]].dst <= offset)
                low = middle + 1;
            else
                high = middle;
        }
        if (!low)
            return NULL;
        const mk64_recipe_t *recipe = &mk64_recipes[mk64_recipe_dst_order[low - 1]];
        if (offset - recipe->dst >= recipe->size)
            return NULL;
        uint32_t next = low < mk64_recipe_count
                            ? mk64_recipes[mk64_recipe_dst_order[low]].dst
                            : MK64_ASSET_REGION_SIZE;
        uint32_t end_offset = recipe->dst + recipe->size;
        /* Several generated DL sizes omit the last four padding bytes. */
        end_offset = (end_offset + sizeof(Gfx) - 1) & ~(sizeof(Gfx) - 1);
        if (end_offset > next || end_offset > MK64_ASSET_REGION_SIZE)
            end_offset = (recipe->dst + recipe->size) & ~(sizeof(Gfx) - 1);
        uintptr_t end = base + end_offset;
        return end > address ? (Gfx *)end : NULL;
    }
    for (unsigned i = 0; i < 2; ++i) {
        Gfx *start = gGfxPools[i].gfxPool;
        if (in_span(address, (uintptr_t)start, sizeof(gGfxPools[i].gfxPool)))
            return start + GFX_POOL_SIZE;
    }
    base = (uintptr_t)gPortMemoryPool;
    if (in_span(address, base, PORT_MEMORY_POOL_SIZE)) {
        uintptr_t end = base + PORT_MEMORY_POOL_SIZE;
        uintptr_t bounded = address + 512 * sizeof(Gfx);
        return (Gfx *)(bounded < end ? bounded : end);
    }
    return NULL;
}

static uint16_t rgba5551_to_565(uint16_t value)
{
    unsigned red = (value >> 11) & 31;
    unsigned green = (value >> 6) & 31;
    unsigned blue = (value >> 1) & 31;
    return (uint16_t)((red << 11) | (green << 6) | (green << 5) | blue);
}

static uint16_t rgba8888_to_565(uint32_t value)
{
    return (uint16_t)((((value >> 24) & 0xff) >> 3) << 11 |
                      (((value >> 16) & 0xff) >> 2) << 5 |
                      (((value >> 8) & 0xff) >> 3));
}

static uint16_t blend_565(uint16_t foreground, uint16_t background,
                          unsigned alpha)
{
    unsigned inverse = 255 - alpha;
    /* Alpha is 0..255. Put R/B into separate 16-bit lanes: each weighted
     * sum including rounding is at most 31*255+127, so no carry crosses
     * lanes. One multiply then computes both channel products. */
    uint32_t rb_foreground = ((uint32_t)(foreground & 0xf800) << 5) | (foreground & 31);
    uint32_t rb_background = ((uint32_t)(background & 0xf800) << 5) | (background & 31);
    uint32_t rb = rb_foreground * alpha + rb_background * inverse + 0x007f007fu;
    /* For these bounded nonnegative sums, (n+1+(n>>8))>>8 is exactly n/255.
     * Mask the shifted packed value to keep each lane's correction separate. */
    rb = ((rb + 0x00010001u + ((rb >> 8) & 0x00ff00ffu)) >> 8) & 0x001f001fu;
    unsigned green = (((foreground >> 5) & 63) * alpha +
                      ((background >> 5) & 63) * inverse + 127);
    green = (green + 1 + (green >> 8)) >> 8;
    return (uint16_t)(((rb >> 5) & 0xf800) | (green << 5) | (rb & 31));
}

/* Promotion is sticky for the remainder of this frame: subsequent fades,
 * borders and menu overlays must blend over the native-resolution HUD. */
static void promote_ui_resolution(void)
{
    if (sHighResolution || !sOutputSurface) return;
    const uint16_t *source = sSurface->data;
    uint16_t *target = sOutputSurface->data;
    if (MK64_RENDER_SCALE == 2) {
        /* Expand in internal stack memory and bulk-copy each row. Avoid
         * halfword stores to PSRAM and reading it back to duplicate rows. */
        uint32_t expanded[MK64_WORLD_WIDTH];
        for (unsigned y = 0; y < MK64_WORLD_HEIGHT; ++y) {
            for (unsigned x = 0; x < MK64_WORLD_WIDTH; ++x)
                expanded[x] = (uint32_t)source[y * MK64_WORLD_WIDTH + x] * 0x00010001u;
            memcpy(target + y * 2 * MK64_OUTPUT_WIDTH, expanded, sizeof(expanded));
            memcpy(target + (y * 2 + 1) * MK64_OUTPUT_WIDTH, expanded, sizeof(expanded));
        }
    } else {
        memcpy(target, source, MK64_OUTPUT_WIDTH * MK64_OUTPUT_HEIGHT * sizeof(uint16_t));
    }
    sRasterWidth = MK64_OUTPUT_WIDTH;
    sRasterHeight = MK64_OUTPUT_HEIGHT;
    sRasterScale = 1;
    sRasterPixels = target;
    sRasterDepth = sUiDepth;
    sHighResolution = true;
}

/* Only sampled frames read the clock. Keep composition separate from the
 * world/UI raster buckets so the device log can identify the next bottleneck. */
static void promote_profiled_ui(void)
{
    if (sHighResolution || !sOutputSurface) return;
    int64_t started = sProfileFrame ? rg_system_timer() : 0;
    if (sProfileFrame) {
        sWorldTriangleUs = sPerfUs[PERF_TRIANGLE];
        sWorldRectangleUs = sPerfUs[PERF_RECTANGLE];
    }
    promote_ui_resolution();
    if (sProfileFrame) sCompositionUs += (uint32_t)(rg_system_timer() - started);
}

static void fill_rect(uint32_t w0, uint32_t w1)
{
    if (!sSurface || !sColorTarget)
        return;
    int left = (int)((w1 >> 12) & 0xfff) / 4;
    int top = (int)(w1 & 0xfff) / 4;
    int right = (int)((w0 >> 12) & 0xfff) / 4;
    int bottom = (int)(w0 & 0xfff) / 4;
    left /= sRasterScale;
    top /= sRasterScale;
    right /= sRasterScale;
    bottom /= sRasterScale;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= MK64_SCREEN_WIDTH) right = MK64_SCREEN_WIDTH - 1;
    if (bottom >= MK64_SCREEN_HEIGHT) bottom = MK64_SCREEN_HEIGHT - 1;
    if (right < left || bottom < top)
        return;
    if (sTraceGeometry)
        do { RG_LOGD("MK64 fill %u after %u triangles: %d,%d..%d,%d cycle %u "
                "fill %04x prim %08x",
                sFillRects + 1, sTriangles, left, top, right, bottom,
                sCycleType, sFillColor, (unsigned)sPrimitiveColor); } while (0);
    uint16_t *pixels = sRasterPixels;
    bool fill_cycle = sCycleType == (G_CYC_FILL >> G_MDSFT_CYCLETYPE);
    uint16_t color = fill_cycle ? rgba5551_to_565(sFillColor)
                                : rgba8888_to_565(sPrimitiveColor);
    unsigned alpha = fill_cycle ? 255 : (sPrimitiveColor & 0xff);
    if (sTraceBorders)
        do { RG_LOGD("MK64 border frame %u: rect %d,%d..%d,%d color %04x alpha %u "
                "cycle %u fill %04x prim %08x triangles %u",
                sFrame, left, top, right, bottom, color, alpha, sCycleType,
                sFillColor, (unsigned)sPrimitiveColor, sTriangles); } while (0);
    if (alpha == 255) {
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x)
                pixels[y * MK64_SCREEN_WIDTH + x] = color;
    } else if (alpha) {
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x) {
                uint16_t *pixel = &pixels[y * MK64_SCREEN_WIDTH + x];
                *pixel = blend_565(color, *pixel, alpha);
            }
    }
    ++sFillRects;
}

/* Match the rectangle shader's exact 5->8->6 green quantisation. The
 * triangle conversion uses different rounding; do not change its colors. */
static uint16_t rectangle_rgba5551_to_565(uint16_t texel)
{
    unsigned green = (texel >> 6) & 31;
    unsigned green6 = green * 2 + (green >= 18);
    return (uint16_t)((texel & 0xf800u) | (green6 << 5) | ((texel >> 1) & 31));
}

/* Menu artwork is drawn through RDP texture rectangles, usually as 32-pixel
 * strips of a larger image. Keep the source stride and tile origin from the
 * load command instead of treating each strip as a tightly packed texture. */
static void texture_rect(uint32_t w0, uint32_t w1, uint32_t st,
                         uint32_t derivatives, bool flip)
{
    if (!sSurface || !sColorTarget || !sLoadedImage ||
        !sLoadedStride || !sLoadedWidth || !sLoadedHeight ||
        sTextureSize > G_IM_SIZ_32b ||
        (sTextureFormat != G_IM_FMT_RGBA &&
         sTextureFormat != G_IM_FMT_IA && sTextureFormat != G_IM_FMT_I &&
         sTextureFormat != G_IM_FMT_CI)) {
        ++sSkippedTextureRects;
        return;
    }
    unsigned bytes_per_texel = sTextureSize == G_IM_SIZ_32b ? 4 :
                               sTextureSize == G_IM_SIZ_16b ? 2 : 1;
    unsigned bits_per_texel = 4u << sTextureSize;
    size_t required = (((size_t)(sLoadedHeight - 1) * sLoadedStride +
                       sLoadedWidth + sLoadedNibbleOffset) * bits_per_texel + 7) / 8;
    if (!readable_data(sLoadedImage, required)) {
        ++sSkippedTextureRects;
        return;
    }
    int x0 = (w1 >> 12) & 0xfff, y0 = w1 & 0xfff;
    int x1 = (w0 >> 12) & 0xfff, y1 = w0 & 0xfff;
    int left = x0 / 4, top = y0 / 4;
    int right = (x1 + 3) / 4, bottom = (y1 + 3) / 4;
    if (sCycleType == (G_CYC_COPY >> G_MDSFT_CYCLETYPE)) {
        ++right;
        ++bottom;
    }
    left /= sRasterScale;
    top /= sRasterScale;
    right = (right + sRasterScale - 1) / sRasterScale;
    bottom = (bottom + sRasterScale - 1) / sRasterScale;
    if (right > MK64_SCREEN_WIDTH) right = MK64_SCREEN_WIDTH;
    if (bottom > MK64_SCREEN_HEIGHT) bottom = MK64_SCREEN_HEIGHT;
    int16_t start_s = (int16_t)(st >> 16), start_t = (int16_t)st;
    int16_t ds = (int16_t)(derivatives >> 16);
    int16_t dt = (int16_t)derivatives;
    if (sCycleType == (G_CYC_COPY >> G_MDSFT_CYCLETYPE)) ds >>= 2;
    bool copy_cycle = sCycleType == (G_CYC_COPY >> G_MDSFT_CYCLETYPE);
    uint32_t rgb_factor = sRectColorSource == G_CCMUX_PRIMITIVE ? sPrimitiveColor :
                          sRectColorSource == G_CCMUX_ENVIRONMENT ? sEnvironmentColor : 0xffffffffu;
    unsigned alpha_factor = sRectAlphaSource == G_ACMUX_PRIMITIVE ? sPrimitiveColor & 255 :
                            sRectAlphaSource == G_ACMUX_ENVIRONMENT ? sEnvironmentColor & 255 : 255;
    bool direct_16 = (copy_cycle || ((rgb_factor >> 8) == 0xffffff && alpha_factor == 255)) &&
                    ((sTextureFormat == G_IM_FMT_RGBA && sTextureSize == G_IM_SIZ_16b) ||
                     (sTextureFormat == G_IM_FMT_CI && sTextureSize <= G_IM_SIZ_8b));
    const uint8_t *texels = sLoadedImage;
    uint16_t *pixels = sRasterPixels;
    for (int y = top < 0 ? 0 : top; y < bottom; ++y) {
        for (int x = left < 0 ? 0 : left; x < right; ++x) {
            /* Starts are S10.5, derivatives S5.10, screen positions U10.2.
             * Convert starts to ten fractional bits before stepping. */
            int logical_x = x * sRasterScale * 4;
            int logical_y = y * sRasterScale * 4;
            int sx = start_s * 32 + ((flip ? logical_y - y0 : logical_x - x0) * ds) / 4;
            int sy = start_t * 32 + ((flip ? logical_x - x0 : logical_y - y0) * dt) / 4;
            int tx = (sx >> 10) - (int)sLoadedOriginS;
            int ty = (sy >> 10) - (int)sLoadedOriginT;
            if (tx < 0 || ty < 0 || tx >= (int)sLoadedWidth ||
                ty >= (int)sLoadedHeight) continue;
            size_t offset = ((size_t)ty * sLoadedStride + tx) * bytes_per_texel;
            unsigned red, green, blue, alpha;
            unsigned pixel_index = ty * sLoadedStride + tx + sLoadedNibbleOffset;
            unsigned packed = sTextureSize == G_IM_SIZ_4b
                ? (texels[pixel_index / 2] >> ((pixel_index & 1) ? 0 : 4)) & 15 : 0;
            if (direct_16) {
                uint16_t texel;
                if (sTextureFormat == G_IM_FMT_CI) {
                    unsigned entry = sTextureSize == G_IM_SIZ_4b
                        ? packed + sTexturePaletteBank * 16 : texels[offset];
                    if (entry >= sPaletteEntries) continue;
                    texel = sTexturePalette[entry];
                } else {
                    texel = (uint16_t)(texels[offset] << 8 | texels[offset + 1]);
                }
                if (!(texel & 1)) continue;
                pixels[y * MK64_SCREEN_WIDTH + x] = rectangle_rgba5551_to_565(texel);
                ++sTexturePixels;
                continue;
            }
            if (sTextureFormat == G_IM_FMT_CI && sTextureSize <= G_IM_SIZ_8b) {
                unsigned entry = sTextureSize == G_IM_SIZ_4b
                    ? packed + sTexturePaletteBank * 16 : texels[offset];
                if (entry >= sPaletteEntries) continue;
                uint16_t texel = sTexturePalette[entry];
                red = ((texel >> 11) & 31) * 255 / 31;
                green = ((texel >> 6) & 31) * 255 / 31;
                blue = ((texel >> 1) & 31) * 255 / 31;
                alpha = (texel & 1) ? 255 : 0;
            } else if (sTextureSize == G_IM_SIZ_4b && sTextureFormat == G_IM_FMT_I) {
                red = green = blue = alpha = packed * 17;
            } else if (sTextureSize == G_IM_SIZ_4b && sTextureFormat == G_IM_FMT_IA) {
                red = green = blue = (packed >> 1) * 255 / 7;
                alpha = (packed & 1) ? 255 : 0;
            } else if (sTextureFormat == G_IM_FMT_RGBA && bytes_per_texel == 4) {
                red = texels[offset];
                green = texels[offset + 1];
                blue = texels[offset + 2];
                alpha = texels[offset + 3];
            } else if (sTextureFormat == G_IM_FMT_RGBA && bytes_per_texel == 2) {
                uint16_t texel = (uint16_t)(texels[offset] << 8 | texels[offset + 1]);
                red = ((texel >> 11) & 31) * 255 / 31;
                green = ((texel >> 6) & 31) * 255 / 31;
                blue = ((texel >> 1) & 31) * 255 / 31;
                alpha = (texel & 1) ? 255 : 0;
            } else if (sTextureFormat == G_IM_FMT_IA) {
                unsigned intensity = bytes_per_texel == 2 ? texels[offset] :
                                     (texels[offset] >> 4) * 17;
                red = green = blue = intensity;
                alpha = bytes_per_texel == 2 ? texels[offset + 1] :
                        (texels[offset] & 15) * 17;
            } else if (sTextureFormat == G_IM_FMT_I && bytes_per_texel == 1) {
                red = green = blue = alpha = texels[offset];
            } else {
                continue;
            }
            if (!alpha) continue;
            if (sCycleType != (G_CYC_COPY >> G_MDSFT_CYCLETYPE)) {
                /* Menu decal modes use the texture directly. Only multiply
                 * for their explicit primitive/environment modulation modes. */
                if (sRectColorSource == G_CCMUX_PRIMITIVE ||
                    sRectColorSource == G_CCMUX_ENVIRONMENT) {
                    uint32_t factor = sRectColorSource == G_CCMUX_PRIMITIVE
                                      ? sPrimitiveColor : sEnvironmentColor;
                    red = red * ((factor >> 24) & 255) / 255;
                    green = green * ((factor >> 16) & 255) / 255;
                    blue = blue * ((factor >> 8) & 255) / 255;
                }
                if (sRectAlphaSource == G_ACMUX_PRIMITIVE)
                    alpha = alpha * (sPrimitiveColor & 255) / 255;
                else if (sRectAlphaSource == G_ACMUX_ENVIRONMENT)
                    alpha = alpha * (sEnvironmentColor & 255) / 255;
            }
            uint16_t color = (uint16_t)(((red >> 3) << 11) |
                                        ((green >> 2) << 5) | (blue >> 3));
            uint16_t *pixel = &pixels[y * MK64_SCREEN_WIDTH + x];
            *pixel = alpha == 255 ? color : blend_565(color, *pixel, alpha);
            ++sTexturePixels;
        }
    }
}

static void load_texture_palette(uint32_t word) {
    unsigned count = ((word >> 14) & 0x3ff) + 1;
    if (sPaletteLoadOffset >= 256 || count > 256 - sPaletteLoadOffset ||
        !sTextureImage || !readable_data(sTextureImage, count * 2)) {
        sPaletteEntries = 0;
        return;
    }
    const uint8_t *bytes = (const uint8_t *)sTextureImage;
            (void)bytes;
    for (unsigned i = 0; i < count; ++i)
        sTexturePalette[sPaletteLoadOffset + i] =
            (uint16_t)((bytes[i * 2] << 8) | bytes[i * 2 + 1]);
    sPaletteEntries = sPaletteLoadOffset + count;
}

static bool sample_triangle_texture_uncached(unsigned index, uint16_t *color,
                                    unsigned *alpha) {
    const uint8_t *bytes = sLoadedTile ? sLoadedImage : (const uint8_t *)sTextureImage;
    if (sLoadedTile) {
        /* LOADTILE can read an I4 image as bytes, then render it as nibbles.
         * Tile bounds include an edge texel and do not define source stride. */
        unsigned stride = (sLoadedStride << sLoadedSize) >> sTextureSize;
        index = (index / sTextureWidth) * stride + index % sTextureWidth +
                sLoadedNibbleOffset;
    }
    unsigned value = sTextureSize == G_IM_SIZ_4b
        ? (bytes[index / 2] >> ((index & 1) ? 0 : 4)) & 15
        : bytes[index];
    *alpha = 255;
    if (sTextureFormat == G_IM_FMT_CI && sTextureSize <= G_IM_SIZ_8b) {
        unsigned entry = value + (sTextureSize == G_IM_SIZ_4b ? sTexturePaletteBank * 16 : 0);
        if (entry >= sPaletteEntries) return false;
        uint16_t rgba = sTexturePalette[entry];
        *color = rgba5551_to_565(rgba);
        *alpha = (rgba & 1) ? 255 : 0;
        return true;
    }
    if (sTextureFormat == G_IM_FMT_RGBA && sTextureSize == G_IM_SIZ_16b) {
        uint16_t rgba = (uint16_t)((bytes[index * 2] << 8) | bytes[index * 2 + 1]);
        *color = rgba5551_to_565(rgba);
        *alpha = (rgba & 1) ? 255 : 0;
        return true;
    }
    unsigned red, green, blue;
    if (sTextureFormat == G_IM_FMT_RGBA && sTextureSize == G_IM_SIZ_32b) {
        red = bytes[index * 4]; green = bytes[index * 4 + 1]; blue = bytes[index * 4 + 2];
        *alpha = bytes[index * 4 + 3];
    } else if (sTextureFormat == G_IM_FMT_IA) {
        if (sTextureSize == G_IM_SIZ_16b) {
            red = bytes[index * 2]; *alpha = bytes[index * 2 + 1];
        } else if (sTextureSize == G_IM_SIZ_8b) {
            red = (value >> 4) * 17; *alpha = (value & 15) * 17;
        } else if (sTextureSize == G_IM_SIZ_4b) {
            red = (value >> 1) * 255 / 7; *alpha = (value & 1) ? 255 : 0;
        } else return false;
        green = blue = red;
    } else if (sTextureFormat == G_IM_FMT_I && sTextureSize <= G_IM_SIZ_8b) {
        red = green = blue = sTextureSize == G_IM_SIZ_4b ? value * 17 : value;
        *alpha = red; /* RDP intensity supplies both RGB and alpha. */
    } else return false;
    *color = (uint16_t)(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
    return true;
}

static void invalidate_triangle_texture_cache(void)
{
    sTextureCacheCount = 0;
}

static void prepare_triangle_texture_cache(unsigned draw_pixels)
{
    if (!sTextureCache || sTextureCacheCount || !sTextureWidth || !sTextureHeight ||
        sTextureWidth > 4096 || sTextureHeight > 4096) return;
    unsigned texels = sTextureWidth * sTextureHeight;
    /* A single current tile, at most 16KiB. Avoid clearing a large table for
     * a tiny triangle. Decode lazily so unused texels never read PSRAM. */
    if (texels > 4096 || draw_pixels < texels / 2) return;
    memset(sTextureCache, 0, texels * sizeof(*sTextureCache));
    sTextureCacheCount = texels;
}

static bool sample_triangle_texture(unsigned index, uint16_t *color, unsigned *alpha)
{
    if (index >= sTextureCacheCount)
        return sample_triangle_texture_uncached(index, color, alpha);
    uint32_t texel = sTextureCache[index];
    if (!(texel & 0x80000000u)) {
        unsigned decoded_alpha;
        uint16_t decoded_color;
        bool valid = sample_triangle_texture_uncached(index, &decoded_color, &decoded_alpha);
        texel = 0x80000000u;
        if (valid) texel |= 0x01000000u | decoded_alpha << 16 | decoded_color;
        sTextureCache[index] = texel;
        if (sProfileFrame) ++sTextureCacheMisses;
    } else if (sProfileFrame) ++sTextureCacheHits;
    if (!(texel & 0x01000000u)) return false;
    *color = (uint16_t)texel;
    *alpha = (texel >> 16) & 255;
    return true;
}

static void set_triangle_combiner(uint32_t w0, uint32_t w1) {
    unsigned rgb[2][4] = {{(w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7},
                          {(w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7}};
    unsigned alpha[2][4] = {{(w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7},
                            {(w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7}};
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        memcpy(sTriangleCombiner[cycle].rgb, rgb[cycle], sizeof(rgb[cycle]));
        memcpy(sTriangleCombiner[cycle].alpha, alpha[cycle], sizeof(alpha[cycle]));
    }
}

static int triangle_rgb_source(unsigned mux, unsigned term, unsigned channel,
                               const int texture[4], const int shade[4], const int combined[4]) {
    switch (mux) {
        case 0: return combined[channel];
        case 1: case 2: return texture[channel];
        case 3: return (sPrimitiveColor >> (24 - channel * 8)) & 255;
        case 4: return shade[channel];
        case 5: return (sEnvironmentColor >> (24 - channel * 8)) & 255;
        case 6: return term == 0 || term == 3 ? 255 : 0;
        case 7: return term == 2 ? combined[3] : 0;
        case 8: case 9: return term == 2 ? texture[3] : 0;
        case 10: return term == 2 ? sPrimitiveColor & 255 : 0;
        case 11: return term == 2 ? shade[3] : 0;
        case 12: return term == 2 ? sEnvironmentColor & 255 : 0;
        default: return 0;
    }
}

static int triangle_alpha_source(unsigned mux, const int texture[4],
                                 const int shade[4], const int combined[4]) {
    switch (mux) {
        case 0: return combined[3];
        case 1: case 2: return texture[3];
        case 3: return sPrimitiveColor & 255;
        case 4: return shade[3];
        case 5: return sEnvironmentColor & 255;
        case 6: return 255;
        default: return 0;
    }
}

static uint16_t combine_triangle_pixel(uint16_t texel, unsigned texel_alpha,
                                      unsigned red, unsigned green, unsigned blue,
                                      unsigned shade_alpha, unsigned *alpha) {
    int texture[4] = {((texel >> 11) & 31) * 255 / 31,
                      ((texel >> 5) & 63) * 255 / 63, (texel & 31) * 255 / 31,
                      (int)texel_alpha};
    int shade[4] = {(int)red, (int)green, (int)blue, (int)shade_alpha};
    int combined[4] = {0};
    /* One-cycle RDP uses the second mux; two-cycle feeds COMBINED forward. */
    for (unsigned cycle = sCycleType == 1 ? 0 : 1; cycle < 2; ++cycle) {
        int result[4];
        for (unsigned channel = 0; channel < 4; ++channel) {
            int values[4];
            for (unsigned term = 0; term < 4; ++term)
                values[term] = channel == 3
                    ? triangle_alpha_source(sTriangleCombiner[cycle].alpha[term], texture, shade, combined)
                    : triangle_rgb_source(sTriangleCombiner[cycle].rgb[term], term, channel, texture, shade, combined);
            int value = (values[0] - values[1]) * values[2] / 255 + values[3];
            result[channel] = value < 0 ? 0 : value > 255 ? 255 : value;
        }
        memcpy(combined, result, sizeof(combined));
    }
    *alpha = (unsigned)combined[3];
    return (uint16_t)(((combined[0] >> 3) << 11) | ((combined[1] >> 2) << 5) | (combined[2] >> 3));
}

/* Select once per triangle. Keep two-cycle and unusual muxes on the
 * general evaluator, including the kart and logo equations. */
static unsigned triangle_combiner_fast_mode(void)
{
    if (sCycleType != 0) return COMBINE_GENERAL;
    const unsigned *rgb = sTriangleCombiner[1].rgb;
    const unsigned *alpha = sTriangleCombiner[1].alpha;
    if (rgb[2] == 31 && alpha[2] == 7) return COMBINE_DIRECT;
    if (rgb[0] == 1 && rgb[1] == 15 && rgb[2] == 4 && rgb[3] == 7) {
        if (alpha[0] == 1 && alpha[1] == 7 && alpha[2] == 4 && alpha[3] == 7)
            return COMBINE_MODULATE;
        /* MODULATEIDECALA: same RGB product, independent texture alpha.
         * A/B cannot affect alpha when its multiplier C is ZERO. */
        if (alpha[2] == 7 && (alpha[3] == 1 || alpha[3] == 2))
            return COMBINE_MODULATE_DECAL;
    }
    return COMBINE_GENERAL;
}

static void build_modulate_table(uint8_t *table)
{
    /* Store quantized channel results, preserving expand/multiply/truncate.
     * R/B share the 5-bit table; G uses the following 6-bit table. */
    for (unsigned shade = 0; shade < 256; ++shade) {
        for (unsigned texel = 0; texel < 32; ++texel)
            table[shade * 32 + texel] = ((texel * 255 / 31) * shade / 255) >> 3;
        for (unsigned texel = 0; texel < 64; ++texel)
            table[256 * 32 + shade * 64 + texel] = ((texel * 255 / 63) * shade / 255) >> 2;
    }
}

static uint16_t combine_triangle_pixel_fast(unsigned mode, uint16_t texel,
    unsigned texel_alpha, unsigned red, unsigned green, unsigned blue,
    unsigned shade_alpha, unsigned *alpha)
{
    if (mode == COMBINE_MODULATE_WHITE || mode == COMBINE_MODULATE_DECAL_WHITE) {
        /* Expanding RGB565, multiplying by 255/255 and truncating back
         * preserves every texel exactly. Alpha can still vary across the row. */
        *alpha = mode == COMBINE_MODULATE_DECAL_WHITE ? texel_alpha : texel_alpha * shade_alpha / 255;
        if (*alpha > 255) *alpha = 255;
        return texel;
    }
    if (mode == COMBINE_DIRECT) {
        uint16_t color;
        switch (sTriangleCombiner[1].rgb[3]) {
            case 1: case 2: color = texel; break;
            case 3: color = rgba8888_to_565(sPrimitiveColor); break;
            case 4:
                if (red > 255) red = 255;
                if (green > 255) green = 255;
                if (blue > 255) blue = 255;
                color = (uint16_t)((red >> 3) << 11 | (green >> 2) << 5 | (blue >> 3));
                break;
            case 5: color = rgba8888_to_565(sEnvironmentColor); break;
            case 6: color = 0xffff; break;
            default: color = 0; break;
        }
        switch (sTriangleCombiner[1].alpha[3]) {
            case 1: case 2: *alpha = texel_alpha; break;
            case 3: *alpha = sPrimitiveColor & 255; break;
            case 4: *alpha = shade_alpha; break;
            case 5: *alpha = sEnvironmentColor & 255; break;
            case 6: *alpha = 255; break;
            default: *alpha = 0; break;
        }
        if (*alpha > 255) *alpha = 255;
        return color;
    }
    if (mode == COMBINE_MODULATE || mode == COMBINE_MODULATE_DECAL) {
        if (sModulateTable && (red | green | blue) <= 255) {
            unsigned r = sModulateTable[red * 32 + (texel >> 11)];
            unsigned g = sModulateTable[256 * 32 + green * 64 + ((texel >> 5) & 63)];
            unsigned b = sModulateTable[blue * 32 + (texel & 31)];
            *alpha = mode == COMBINE_MODULATE_DECAL ? texel_alpha : texel_alpha * shade_alpha / 255;
            if (*alpha > 255) *alpha = 255;
            return (uint16_t)(r << 11 | g << 5 | b);
        }
        /* Preserve the general evaluator's expand, multiply, truncate order. */
        red = (((texel >> 11) & 31) * 255 / 31) * red / 255;
        green = (((texel >> 5) & 63) * 255 / 63) * green / 255;
        blue = ((texel & 31) * 255 / 31) * blue / 255;
        *alpha = mode == COMBINE_MODULATE_DECAL ? texel_alpha : texel_alpha * shade_alpha / 255;
        if (red > 255) red = 255;
        if (green > 255) green = 255;
        if (blue > 255) blue = 255;
        if (*alpha > 255) *alpha = 255;
        return (uint16_t)((red >> 3) << 11 | (green >> 2) << 5 | (blue >> 3));
    }
    return combine_triangle_pixel(texel, texel_alpha, red, green, blue, shade_alpha, alpha);
}

static float triangle_edge(float ax, float ay, float bx, float by,
                           float px, float py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static int triangle_texel_floor(float coordinate)
{
    /* UVs have the same finite int range required by (int)floorf(). A C cast
     * truncates toward zero; correcting a negative fraction gives floor
     * without a libm call for every S/T sample. Keep exact integer boundaries. */
    int truncated = (int)coordinate;
    return truncated - (coordinate < (float)truncated);
}

static unsigned triangle_texture_coordinate(int coordinate, unsigned extent,
                                            unsigned mode)
{
    if (mode & G_TX_CLAMP) {
        if (coordinate < 0) return 0;
        if ((unsigned)coordinate >= extent) return extent - 1;
        return coordinate;
    }
    unsigned period = mode & G_TX_MIRROR ? extent * 2 : extent;
    unsigned wrapped = (period & (period - 1)) == 0
        ? (unsigned)coordinate & (period - 1)
        : (coordinate % (int)period + (int)period) % (int)period;
    return wrapped < extent ? wrapped : period - 1 - wrapped;
}

static bool triangle_row_span(const float x[3], const float y[3],
                              const float slopes[3], float area, int row,
                              int left, int right, int *first, int *last)
{
    float sample_y = row + 0.5f;
    float low = left + 0.5f, high = right + 0.5f;
    for (unsigned edge = 0; edge < 3; ++edge) {
        unsigned next = (edge + 1) % 3;
        float dy = y[next] - y[edge];
        if (dy == 0.0f) {
            float value = triangle_edge(x[edge], y[edge], x[next], y[next], 0, sample_y);
            if ((area > 0 && value < 0) || (area < 0 && value > 0)) return false;
        } else {
            float crossing = x[edge] + (sample_y - y[edge]) * slopes[edge];
            if ((dy > 0) == (area > 0)) low = fmaxf(low, crossing);
            else high = fminf(high, crossing);
        }
    }
    if (low > high + 0.0001f) return false;
    /* Conservative bounds absorb intersection rounding. Verify only the
     * endpoints with the original edge equations; convex interior is covered. */
    int start = (int)ceilf(low - 0.5f) - 1;
    int finish = (int)floorf(high - 0.5f) + 1;
    if (start < left) start = left;
    if (finish > right) finish = right;
    for (unsigned side = 0; side < 2; ++side) {
        while (start <= finish) {
            float sample_x = (side ? finish : start) + 0.5f;
            bool inside = true;
            for (unsigned edge = 0; edge < 3; ++edge) {
                unsigned next = (edge + 1) % 3;
                float value = triangle_edge(x[edge], y[edge], x[next], y[next], sample_x, sample_y);
                if ((area > 0 && value < 0) || (area < 0 && value > 0)) { inside = false; break; }
            }
            if (inside) break;
            if (side) --finish; else ++start;
        }
    }
    *first = start; *last = finish;
    return start <= finish;
}

/* Four constant flag combinations let GCC remove unused interpolation and
 * sampling branches, following SM64's specialised property-count rasterizers.
 * The maths for every consumed attribute and every pixel remains unchanged. */
MK64_RASTER_INLINE unsigned raster_triangle_rows(const RGRasterRows *row,
                                                bool textured, bool gradient, unsigned raster_width)
{
    const float *x = row->x, *y = row->y, *slopes = row->slopes;
    const float (*attributes)[3] = row->attributes;
    const float *attribute_dx = row->attribute_dx;
    float area = row->area, inverse_area = row->inverse_area;
    int left = row->left, right = row->right, top = row->top, bottom = row->bottom;
    unsigned constant_attributes = row->constant_attributes;
    unsigned red = row->red, green = row->green, blue = row->blue;
    unsigned combiner_mode = row->combiner_mode;
    bool depth_test = row->depth_test, depth_write = row->depth_write;
    uint16_t *pixels = row->pixels;
    uint16_t *depths = row->depth;
    unsigned needed_attributes = (1u << ATTR_Z) | (1u << ATTR_ALPHA);
    if (gradient) needed_attributes |= (1u << ATTR_RED) | (1u << ATTR_GREEN) | (1u << ATTR_BLUE);
    if (textured) needed_attributes |= (1u << ATTR_Q) | (1u << ATTR_U) | (1u << ATTR_V);
    /* Lazy evaluation preserves the early-depth path: a wholly occluded
     * triangle never invokes the shader. Cache the final alpha too; alpha
     * holes still leave depth untouched and blending still reads each pixel. */
    bool uniform_shader = !textured && !gradient &&
                          (constant_attributes & (1u << ATTR_ALPHA));
    bool shader_ready = false;
    uint16_t uniform_color = 0;
    unsigned uniform_alpha = 0;
    unsigned written = 0;
    for (int py = top; py <= bottom; ++py) {
        int first, last;
        if (!triangle_row_span(x, y, slopes, area, py, left, right, &first, &last)) continue;
        if (sProfileFrame) {
            unsigned span = last - first + 1;
            sSpanPixels += span;
            sSkippedAttributeSteps += span * ((gradient ? 0 : 3) + (textured ? 0 : 3));
        }
        float sample_x = first + 0.5f, sample_y = py + 0.5f;
        float bary[3] = {
            triangle_edge(x[1], y[1], x[2], y[2], sample_x, sample_y) * inverse_area,
            triangle_edge(x[2], y[2], x[0], y[0], sample_x, sample_y) * inverse_area,
            triangle_edge(x[0], y[0], x[1], y[1], sample_x, sample_y) * inverse_area,
        };
        float value[ATTR_COUNT];
        for (unsigned i = 0; i < ATTR_COUNT; ++i) {
            if (!(needed_attributes & (1u << i))) continue;
            value[i] = constant_attributes & (1u << i) ? attributes[i][0] :
                bary[0] * attributes[i][0] + bary[1] * attributes[i][1] + bary[2] * attributes[i][2];
        }
        /* Loop increments run even on alpha/depth rejection. Rebase every row
         * to keep floating-point drift bounded by the screen width. */
        for (int px = first; px <= last; ++px,
             value[ATTR_Z] += attribute_dx[ATTR_Z], value[ATTR_ALPHA] += attribute_dx[ATTR_ALPHA],
             gradient ? (value[ATTR_RED] += attribute_dx[ATTR_RED],
                         value[ATTR_GREEN] += attribute_dx[ATTR_GREEN],
                         value[ATTR_BLUE] += attribute_dx[ATTR_BLUE], 0) : 0,
             textured ? (value[ATTR_Q] += attribute_dx[ATTR_Q],
                         value[ATTR_U] += attribute_dx[ATTR_U],
                         value[ATTR_V] += attribute_dx[ATTR_V], 0) : 0) {
            unsigned pixel_index = py * raster_width + px;
            bool probe = MK64_RG_PIXEL_PROBE && sProfileFrame && ((sPixelProbeCursor++ & 63u) == 0);
            uint32_t probe_started = probe ? esp_cpu_get_cycle_count() : 0;
            /* Reject occluded fragments before texture fetch and combiner.
             * Delay the write until final alpha is known: sprite holes
             * must not hide geometry drawn later. */
            uint16_t depth = 0;
            if (depth_test) {
                float ndc_z = value[ATTR_Z];
                if (ndc_z < -1.0f) ndc_z = -1.0f;
                if (ndc_z > 1.0f) ndc_z = 1.0f;
                depth = (uint16_t)((ndc_z + 1.0f) * 32767.0f);
                if (depth >= depths[pixel_index]) {
                    if (sProfileFrame) ++sEarlyDepthRejects;
                    if (probe) record_pixel_probe(PIX_DEPTH, probe_started);
                    continue;
                }
            }
            if (probe) {
                record_pixel_probe(PIX_DEPTH, probe_started);
                probe_started = esp_cpu_get_cycle_count();
            }
            unsigned rr = red, gg = green, bb = blue;
            unsigned shade_alpha = value[ATTR_ALPHA] <= 0 ? 0 : value[ATTR_ALPHA] >= 255 ? 255 : (unsigned)value[ATTR_ALPHA];
            uint16_t tex_color = 0xffff;
            unsigned alpha = 255;
            if (gradient) {
                rr = value[ATTR_RED] <= 0 ? 0 : value[ATTR_RED] >= 255 ? 255 : (unsigned)value[ATTR_RED];
                gg = value[ATTR_GREEN] <= 0 ? 0 : value[ATTR_GREEN] >= 255 ? 255 : (unsigned)value[ATTR_GREEN];
                bb = value[ATTR_BLUE] <= 0 ? 0 : value[ATTR_BLUE] >= 255 ? 255 : (unsigned)value[ATTR_BLUE];
            }
            if (textured) {
                float reciprocal = value[ATTR_Q];
                if (reciprocal <= 0.0f) {
                    if (probe) record_pixel_probe(PIX_COORD, probe_started);
                    continue;
                }
                float perspective = 1.0f / reciprocal;
                int tx = triangle_texel_floor(value[ATTR_U] * perspective);
                int ty = triangle_texel_floor(value[ATTR_V] * perspective);
                tx = triangle_texture_coordinate(tx, sTextureWidth, sTextureModeS);
                ty = triangle_texture_coordinate(ty, sTextureHeight, sTextureModeT);
                if (probe) {
                    record_pixel_probe(PIX_COORD, probe_started);
                    probe_started = esp_cpu_get_cycle_count();
                }
                bool sampled = sample_triangle_texture(ty * sTextureWidth + tx, &tex_color, &alpha);
                if (probe) record_pixel_probe(PIX_FETCH, probe_started);
                if (!sampled) {
                    if (sProfileFrame) ++sTextureSampleFailures;
                    continue;
                }
            }
            if (probe) {
                if (!textured) record_pixel_probe(PIX_COORD, probe_started);
                probe_started = esp_cpu_get_cycle_count();
            }
            uint16_t shade;
            if (uniform_shader && shader_ready) {
                shade = uniform_color;
                alpha = uniform_alpha;
                if (sProfileFrame) ++sUniformShaderReuse;
            } else {
                shade = combine_triangle_pixel_fast(combiner_mode, tex_color, alpha, rr, gg, bb, shade_alpha, &alpha);
                if (uniform_shader) {
                    uniform_color = shade;
                    uniform_alpha = alpha;
                    shader_ready = true;
                }
            }
            if (sProfileFrame && combiner_mode != COMBINE_GENERAL) ++sFastPixels;
            if (sProfileFrame && (combiner_mode == COMBINE_MODULATE_WHITE ||
                                  combiner_mode == COMBINE_MODULATE_DECAL_WHITE)) ++sWhitePixels;
            if (sProfileFrame && (combiner_mode == COMBINE_MODULATE ||
                                  combiner_mode == COMBINE_MODULATE_DECAL) && sModulateTable) ++sLookupPixels;
            if (sProfileFrame && (combiner_mode == COMBINE_MODULATE_DECAL ||
                                  combiner_mode == COMBINE_MODULATE_DECAL_WHITE)) ++sDecalPixels;
            if (!alpha) {
                if (sProfileFrame) ++sAlphaRejects;
                if (probe) record_pixel_probe(PIX_SHADE, probe_started);
                continue;
            }
            if (depth_write) depths[pixel_index] = depth;
            pixels[pixel_index] = alpha < 255 ? blend_565(shade, pixels[pixel_index], alpha) : shade;
            ++written;
            if (probe) record_pixel_probe(PIX_SHADE, probe_started);
        }
    }
    return written;
}

static void raster_triangle(const RGVertex vertices[3])
{
    if (!sSurface || !sColorTarget)
        return;
    float x[3], y[3];
    for (unsigned i = 0; i < 3; ++i) {
        const float w = vertices[i].w;
        x[i] = (vertices[i].x / w + 1.0f) *
               (MK64_SCREEN_WIDTH * 0.5f);
        y[i] = (1.0f - vertices[i].y / w) *
               (MK64_SCREEN_HEIGHT * 0.5f);
        if (!isfinite(x[i]) || !isfinite(y[i]) ||
            x[i] < -10000.0f || x[i] > 10000.0f ||
            y[i] < -10000.0f || y[i] > 10000.0f) {
            ++sBadGeometry;
            return;
        }
    }
    if (sTraceGeometry && sTriangles == 1)
        do { RG_LOGD("MK64 first triangle screen: (%.1f,%.1f) (%.1f,%.1f) "
                "(%.1f,%.1f)", (double)x[0], (double)y[0],
                (double)x[1], (double)y[1], (double)x[2], (double)y[2]); } while (0);
    float area = triangle_edge(x[0], y[0], x[1], y[1], x[2], y[2]);
    if (area > -0.01f && area < 0.01f)
        return;
    unsigned cull = sGeometryMode & G_CULL_BOTH;
    /* triangle_edge has the opposite sign to the N64 winding cross. */
    if (cull == G_CULL_BOTH ||
        (cull == G_CULL_BACK && area <= 0.0f) ||
        (cull == G_CULL_FRONT && area >= 0.0f)) {
        ++sCulledTriangles;
        return;
    }
    float min_x = fminf(x[0], fminf(x[1], x[2]));
    float max_x = fmaxf(x[0], fmaxf(x[1], x[2]));
    float min_y = fminf(y[0], fminf(y[1], y[2]));
    float max_y = fmaxf(y[0], fmaxf(y[1], y[2]));
    if (min_x >= MK64_SCREEN_WIDTH || max_x < 0 ||
        min_y >= MK64_SCREEN_HEIGHT || max_y < 0)
        return;
    int left = (int)fmaxf(0.0f, floorf(min_x));
    int right = (int)fminf(MK64_SCREEN_WIDTH - 1, ceilf(max_x));
    int top = (int)fmaxf(0.0f, floorf(min_y));
    int bottom = (int)fminf(MK64_SCREEN_HEIGHT - 1, ceilf(max_y));
    unsigned red = (vertices[0].red + vertices[1].red + vertices[2].red) / 3;
    unsigned green = (vertices[0].green + vertices[1].green + vertices[2].green) / 3;
    unsigned blue = (vertices[0].blue + vertices[1].blue + vertices[2].blue) / 3;
    unsigned texture_stride = sLoadedTile
        ? (sLoadedStride << sLoadedSize) >> sTextureSize : sTextureWidth;
    unsigned texture_pixels = sTextureHeight
        ? (sTextureHeight - 1) * texture_stride + sTextureWidth : 0;
    if (sLoadedTile) texture_pixels += sLoadedNibbleOffset;
    unsigned texture_bytes = (texture_pixels * (4u << sTextureSize) + 7) / 8;
    uint16_t first_texel;
    unsigned first_alpha;
    unsigned combiner_mode = triangle_combiner_fast_mode();
    /* Direct shade/primitive/environment equations cannot depend on a stale
     * enabled texture. Keep sampling whenever RGB or alpha needs TEXEL0/1,
     * and conservatively for every general or modulate equation. */
    unsigned rgb_source = sTriangleCombiner[1].rgb[3];
    unsigned alpha_source = sTriangleCombiner[1].alpha[3];
    bool needs_texture = combiner_mode != COMBINE_DIRECT ||
                         rgb_source == 1 || rgb_source == 2 ||
                         alpha_source == 1 || alpha_source == 2;
    bool textured = needs_texture && sTextureEnabled && sTextureImage &&
                    sTextureWidth && sTextureHeight &&
                    sTextureWidth <= 512 && sTextureHeight <= 512 &&
                    readable_data(sLoadedTile ? sLoadedImage : (const uint8_t *)sTextureImage, texture_bytes) &&
                    sample_triangle_texture_uncached(0, &first_texel, &first_alpha);
    if (textured)
        prepare_triangle_texture_cache((unsigned)(right - left + 1) * (unsigned)(bottom - top + 1));
    bool gradient = vertices[0].red != vertices[1].red || vertices[0].red != vertices[2].red ||
                    vertices[0].green != vertices[1].green || vertices[0].green != vertices[2].green ||
                      vertices[0].blue != vertices[1].blue || vertices[0].blue != vertices[2].blue;
    if (!gradient && red == 255 && green == 255 && blue == 255) {
        if (combiner_mode == COMBINE_MODULATE) combiner_mode = COMBINE_MODULATE_WHITE;
        else if (combiner_mode == COMBINE_MODULATE_DECAL) combiner_mode = COMBINE_MODULATE_DECAL_WHITE;
    }
    if (sTraceAssets && sSpriteTraces < 40 &&
        (sTextureFormat == G_IM_FMT_CI || sTextureFormat == G_IM_FMT_I ||
         sTextureFormat == G_IM_FMT_IA)) {
        ++sSpriteTraces;
        do { RG_LOGD("MK64 sprite %u: image %p %ux%u fmt %u size %u enable %u sample %u "
                "palette %u bank %u scale %.5f,%.5f UV %.2f,%.2f %.2f,%.2f %.2f,%.2f "
                "prim %08x env %08x cycle %u",
                sSpriteTraces, sTextureImage, sTextureWidth, sTextureHeight,
                sTextureFormat, sTextureSize, sTextureEnabled, textured,
                sPaletteEntries, sTexturePaletteBank, sTextureScaleS, sTextureScaleT,
                vertices[0].u, vertices[0].v, vertices[1].u, vertices[1].v,
                vertices[2].u, vertices[2].v, (unsigned)sPrimitiveColor,
                (unsigned)sEnvironmentColor, sCycleType); } while (0);
        do { RG_LOGD("MK64 sprite mux: RGB %u,%u,%u,%u / %u,%u,%u,%u alpha %u,%u,%u,%u / %u,%u,%u,%u",
                sTriangleCombiner[0].rgb[0], sTriangleCombiner[0].rgb[1],
                sTriangleCombiner[0].rgb[2], sTriangleCombiner[0].rgb[3],
                sTriangleCombiner[1].rgb[0], sTriangleCombiner[1].rgb[1],
                sTriangleCombiner[1].rgb[2], sTriangleCombiner[1].rgb[3],
                sTriangleCombiner[0].alpha[0], sTriangleCombiner[0].alpha[1],
                sTriangleCombiner[0].alpha[2], sTriangleCombiner[0].alpha[3],
                sTriangleCombiner[1].alpha[0], sTriangleCombiner[1].alpha[1],
                sTriangleCombiner[1].alpha[2], sTriangleCombiner[1].alpha[3]); } while (0);
        if (sTextureImage && readable_data(sTextureImage, 4)) {
            const uint8_t *bytes = (const uint8_t *)sTextureImage;
            (void)bytes;
            do { RG_LOGD("MK64 sprite data: %02x %02x %02x %02x palette[0,1] %04x,%04x",
                    bytes[0], bytes[1], bytes[2], bytes[3],
                    sTexturePalette[0], sTexturePalette[1]); } while (0);
        }
    }
    bool depth_test = sRasterDepth && (sGeometryMode & G_ZBUFFER) &&
                      (sOtherModeL & Z_CMP);
    if (depth_test && sHighResolution && !sUiDepthCleared) {
        /* Most race HUD frames do not use depth. Clear the PSRAM menu table
         * only when a native-resolution 3D menu element actually needs it. */
        memset(sUiDepth, 0xff, MK64_OUTPUT_WIDTH * MK64_OUTPUT_HEIGHT * sizeof(uint16_t));
        sUiDepthCleared = true;
    }
    bool depth_write = depth_test && (sOtherModeL & Z_UPD);
    bool translucent = (sOtherModeL & FORCE_BL) && !depth_write;
    if (textured) ++sTexturedTriangles;
    if (depth_test) ++sDepthTriangles;
    if (translucent) ++sBlendedTriangles;
    float inverse_w[3], z[3], u[3], v[3];
    for (unsigned i = 0; i < 3; ++i) {
        inverse_w[i] = 1.0f / vertices[i].w;
        z[i] = vertices[i].z * inverse_w[i];
        u[i] = vertices[i].u * inverse_w[i];
        v[i] = vertices[i].v * inverse_w[i];
    }
    float inverse_area = 1.0f / area;
    float slopes[3], bary_dx[3];
    for (unsigned i = 0; i < 3; ++i) {
        unsigned next = (i + 1) % 3;
        float dy = y[next] - y[i];
        slopes[i] = dy != 0 ? (x[next] - x[i]) / dy : 0;
        bary_dx[(i + 2) % 3] = dy * inverse_area;
    }
    float attributes[ATTR_COUNT][3], attribute_dx[ATTR_COUNT];
    unsigned constant_attributes = 0;
    for (unsigned i = 0; i < 3; ++i) {
        attributes[ATTR_Z][i] = z[i];
        attributes[ATTR_ALPHA][i] = vertices[i].alpha;
        attributes[ATTR_RED][i] = vertices[i].red;
        attributes[ATTR_GREEN][i] = vertices[i].green;
        attributes[ATTR_BLUE][i] = vertices[i].blue;
        attributes[ATTR_Q][i] = inverse_w[i];
        attributes[ATTR_U][i] = u[i];
        attributes[ATTR_V][i] = v[i];
    }
    for (unsigned i = 0; i < ATTR_COUNT; ++i) {
        bool constant = attributes[i][0] == attributes[i][1] && attributes[i][0] == attributes[i][2];
        if (constant) constant_attributes |= 1u << i;
        attribute_dx[i] = constant ? 0 : attributes[i][0] * bary_dx[0] +
                          attributes[i][1] * bary_dx[1] + attributes[i][2] * bary_dx[2];
    }
    uint16_t *pixels = sRasterPixels;
    unsigned written = 0;
    int64_t pixel_started = sProfileFrame ? rg_system_timer() : 0;
    if (sProfileFrame)
        sBoxPixels += (unsigned)(right - left + 1) * (unsigned)(bottom - top + 1);
    const RGRasterRows rows = {
        .x = x, .y = y, .slopes = slopes,
        .attributes = attributes, .attribute_dx = attribute_dx,
        .area = area, .inverse_area = inverse_area,
        .left = left, .right = right, .top = top, .bottom = bottom,
        .constant_attributes = constant_attributes,
        .red = red, .green = green, .blue = blue, .combiner_mode = combiner_mode,
        .depth_test = depth_test, .depth_write = depth_write, .pixels = pixels, .depth = sRasterDepth,
    };
    /* Keep the world row stride a compile-time constant, as before native
     * UI was added. The hot loop is specialised for each target and shader. */
    if (sHighResolution) {
        if (textured) {
            if (gradient) written = raster_triangle_rows(&rows, true, true, MK64_OUTPUT_WIDTH);
            else written = raster_triangle_rows(&rows, true, false, MK64_OUTPUT_WIDTH);
        } else {
            if (gradient) written = raster_triangle_rows(&rows, false, true, MK64_OUTPUT_WIDTH);
            else written = raster_triangle_rows(&rows, false, false, MK64_OUTPUT_WIDTH);
        }
    } else {
        if (textured) {
            if (gradient) written = raster_triangle_rows(&rows, true, true, MK64_WORLD_WIDTH);
            else written = raster_triangle_rows(&rows, true, false, MK64_WORLD_WIDTH);
        } else {
            if (gradient) written = raster_triangle_rows(&rows, false, true, MK64_WORLD_WIDTH);
            else written = raster_triangle_rows(&rows, false, false, MK64_WORLD_WIDTH);
        }
    }
    if (sProfileFrame) {
        sPixelUs += (uint32_t)(rg_system_timer() - pixel_started);
        sWrittenPixels += written;
    }
    if (written)
        ++sVisibleTriangles;
}

/* Clip before perspective division. Dropping an entire triangle when one
 * vertex is behind the eye leaves holes in the nearby road during camera pans.
 * Seven half-spaces also bound screen coordinates before raster conversion. */
static float clip_distance(const RGVertex *vertex, unsigned plane)
{
    switch (plane) {
        case 0: return vertex->w - 0.001f;
        case 1: return vertex->z + vertex->w;
        case 2: return vertex->w - vertex->z;
        case 3: return vertex->x + vertex->w;
        case 4: return vertex->w - vertex->x;
        case 5: return vertex->y + vertex->w;
        default: return vertex->w - vertex->y;
    }
}

static RGVertex clip_interpolate(const RGVertex *a, const RGVertex *b, float t)
{
    RGVertex out;
#define INTERPOLATE(field) out.field = a->field + (b->field - a->field) * t
    INTERPOLATE(x); INTERPOLATE(y); INTERPOLATE(z); INTERPOLATE(w);
    INTERPOLATE(u); INTERPOLATE(v);
    INTERPOLATE(red); INTERPOLATE(green); INTERPOLATE(blue); INTERPOLATE(alpha);
#undef INTERPOLATE
    out.valid = true;
    return out;
}

static void draw_triangle(unsigned a, unsigned b, unsigned c)
{
    ++sTriangles;
    if (a >= MK64_VERTEX_SLOTS || b >= MK64_VERTEX_SLOTS ||
        c >= MK64_VERTEX_SLOTS || !sVertices[a].valid ||
        !sVertices[b].valid || !sVertices[c].valid) {
        ++sBadGeometry;
        return;
    }
    RGVertex polygon[2][12];
    polygon[0][0] = sVertices[a];
    polygon[0][1] = sVertices[b];
    polygon[0][2] = sVertices[c];
    unsigned outside = 0, common = 0x7f;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned mask = 0;
        for (unsigned plane = 0; plane < 7; ++plane)
            if (clip_distance(&polygon[0][i], plane) < 0) mask |= 1u << plane;
        outside |= mask;
        common &= mask;
    }
    if (common) {
        if (sProfileFrame)
            for (unsigned plane = 0; plane < 7; ++plane)
                if (common & (1u << plane)) ++sClipRejects[plane];
        return;
    }
    if (!outside) {
        raster_triangle(polygon[0]);
        return;
    }
    ++sClippedTriangles;
    unsigned count = 3, buffer = 0;
    for (unsigned plane = 0; plane < 7 && count; ++plane) {
        if (!(outside & (1u << plane))) continue;
        RGVertex *input = polygon[buffer], *output = polygon[buffer ^ 1];
        unsigned written = 0;
        const RGVertex *previous = &input[count - 1];
        float previous_distance = clip_distance(previous, plane);
        for (unsigned i = 0; i < count; ++i) {
            const RGVertex *current = &input[i];
            float distance = clip_distance(current, plane);
            if ((distance >= 0) != (previous_distance >= 0)) {
                float t = previous_distance / (previous_distance - distance);
                output[written++] = clip_interpolate(previous, current, t);
            }
            if (distance >= 0) output[written++] = *current;
            previous = current;
            previous_distance = distance;
        }
        count = written;
        buffer ^= 1;
    }
    for (unsigned i = 1; i + 1 < count; ++i) {
        RGVertex triangle[3] = {polygon[buffer][0], polygon[buffer][i], polygon[buffer][i + 1]};
        raster_triangle(triangle);
    }
}

static void draw_packed_triangle(uint32_t word)
{
    draw_triangle(((word >> 16) & 0xff) / 2,
                  ((word >> 8) & 0xff) / 2,
                  (word & 0xff) / 2);
}

static void walk_list(Gfx *command, Gfx *end, unsigned depth)
{
    if (depth >= MK64_DL_DEPTH) {
        ++sInvalidLists;
        return;
    }
    Gfx *start = command;
    unsigned setup_bit = start == D_0D007D78 ? 1 : start == D_0D008DB8 ? 2 : 0;
    bool trace_setup = sTraceAssets && setup_bit && !(sSpriteSetupTraces & setup_bit);
    if (trace_setup) sSpriteSetupTraces |= setup_bit;
#if MK64_RG_COURSE_TRACE
    bool trace_shell = sProfileFrame && (start == D_0D005338 || start == D_0D005368);
    unsigned shell_written = sWrittenPixels, shell_culled = sCulledTriangles;
    unsigned shell_visible = sVisibleTriangles, shell_alpha = sAlphaRejects;
    unsigned shell_depth = sEarlyDepthRejects, shell_bad = sBadGeometry;
#endif
    while (command < end && sCommands < MK64_DL_BUDGET) {
        uint32_t w0 = command->words.w0;
        uint32_t w1 = command->words.w1;
        if (trace_setup)
            do { RG_LOGD("MK64 sprite setup %u: %08x %08x", setup_bit, (unsigned)w0, (unsigned)w1); } while (0);
        unsigned opcode = w0 >> 24;
        int perf_bucket = sProfileFrame ? profile_bucket(opcode) : -1;
        int64_t command_started = perf_bucket >= 0 ? rg_system_timer() : 0;
        ++sOpcodeCounts[opcode];
        ++sCommands;
        if (opcode == G_NOOP && w1 == MK64_RG_UI_TAG) {
            promote_profiled_ui();
        } else if (opcode == (uint8_t)G_ENDDL) {
#if MK64_RG_COURSE_TRACE
            if (trace_shell)
                do { RG_LOGD("MK64 shell draw frame %u: list %p visible %u cull %u bad %u write %u alpha %u earlyZ %u tex %p %ux%u palette %u geom %08x mode %08x",
                        sFrame + 1, start, sVisibleTriangles - shell_visible,
                        sCulledTriangles - shell_culled, sBadGeometry - shell_bad,
                        sWrittenPixels - shell_written, sAlphaRejects - shell_alpha,
                        sEarlyDepthRejects - shell_depth, sTextureImage,
                        sTextureWidth, sTextureHeight, sPaletteEntries,
                        (unsigned)sGeometryMode, (unsigned)sOtherModeL); } while (0);
#endif
            return;
        }
        if (opcode == G_DL) {
            Gfx *target = port_seg_to_ptr(w1);
            if (sTraceSegmentB &&
                (w1 >> 24) == 0x0b && sSegmentBLists < 4)
                do { RG_LOGD("MK64 segment B list %u: %08x -> %p",
                        ++sSegmentBLists, (unsigned)w1, target); } while (0);
            Gfx *target_end = list_end(target);
            if (!target_end && !target && (w1 >> 24) == 0x0b) {
                ++sSkippedUnmappedLists;
            } else if (!target_end) {
                ++sInvalidLists;
                if (sInvalidLists <= 8)
                    RG_LOGW("MK64 invalid DL target %08x at %p", (unsigned)w1, command);
            } else if (((w0 >> 16) & 1) == G_DL_NOPUSH) {
                command = target;
                end = target_end;
                continue;
            } else {
                walk_list(target, target_end, depth + 1);
            }
        } else if (opcode == G_MTX) {
            load_matrix(w0, w1);
        } else if (opcode == (uint8_t)G_POPMTX) {
            if (sMatrixTop) --sMatrixTop;
            else ++sBadGeometry;
        } else if (opcode == G_VTX) {
            load_vertices(w0, w1);
        } else if (opcode == (uint8_t)G_TEXTURE) {
            sTextureEnabled = (w0 & 0xff) != 0;
            sTextureScaleS = ((w1 >> 16) & 0xffff) / 65536.0f;
            sTextureScaleT = (w1 & 0xffff) / 65536.0f;
        } else if (opcode == (uint8_t)G_SETGEOMETRYMODE) {
            sGeometryMode |= w1;
        } else if (opcode == (uint8_t)G_CLEARGEOMETRYMODE) {
            sGeometryMode &= ~w1;
        } else if (opcode == (uint8_t)G_MOVEWORD) {
            if (((w0 >> 16) & 0xff) == G_MW_NUMLIGHT &&
                w1 >= 0x80000000u) {
                unsigned total = (w1 - 0x80000000u) / 32;
                if (total >= 1 && total <= 8)
                    sNumLights = total;
            }
            if (((w0 >> 16) & 0xff) == G_MW_LIGHTCOL) {
                ++sLightColorCommands;
                if (sTraceAssets && sLightColorCommands <= 4)
                    do { RG_LOGD("MK64 light color cmd %08x %08x",
                            (unsigned)w0, (unsigned)w1); } while (0);
            }
        } else if (opcode == G_MOVEMEM) {
            unsigned index = (w0 >> 16) & 0xff;
            if (sTraceAssets && sOpcodeCounts[G_MOVEMEM] <= 2)
                do { RG_LOGD("MK64 MOVEMEM index %02x source %08x",
                        index, (unsigned)w1); } while (0);
            if (index >= G_MV_L0 && index <= G_MV_L7 &&
                ((index - G_MV_L0) & 1) == 0) {
                unsigned light = (index - G_MV_L0) / 2;
                const Light_t *source = port_seg_to_ptr(w1);
                if (source && readable_data(source, sizeof(*source))) {
                    memcpy(&sLights[light], source, sizeof(*source));
                    ++sLightLoads;
                } else if (sTraceGeometry) {
                    RG_LOGW("MK64 light %u unreadable: %08x -> %p",
                            light, (unsigned)w1, source);
                }
            }
        } else if (opcode == (uint8_t)G_TRI2) {
            draw_packed_triangle(w0);
            draw_packed_triangle(w1);
        } else if (opcode == (uint8_t)G_TRI1) {
            draw_packed_triangle(w1);
        } else if (opcode == (uint8_t)G_QUAD) {
            unsigned v0 = ((w1 >> 16) & 0xff) / 2;
            unsigned v1 = ((w1 >> 8) & 0xff) / 2;
            unsigned v2 = (w1 & 0xff) / 2;
            unsigned v3 = (w1 >> 24) / 2;
            draw_triangle(v0, v1, v2);
            draw_triangle(v0, v2, v3);
        } else if (opcode == G_SETTIMG) {
            invalidate_triangle_texture_cache();
            sTextureImage = port_seg_to_ptr(w1);
            sImageWidth = (w0 & 0xfff) + 1;
            sTextureFormat = (w0 >> 21) & 7;
            sTextureSize = (w0 >> 19) & 3;
            if (sTraceAssets && sTextureImages < 4) {
                do { RG_LOGD("MK64 texture image %u: %08x %08x ptr %p readable %u",
                        sTextureImages + 1, (unsigned)w0, (unsigned)w1,
                        sTextureImage, sTextureImage &&
                        readable_data(sTextureImage, 2)); } while (0);
            }
            ++sTextureImages;
        } else if (opcode == G_LOADTILE) {
            invalidate_triangle_texture_cache();
            sLoadedTile = true;
            sLoadedSize = sTextureSize;
            unsigned left = ((w0 >> 12) & 0xfff) >> 2;
            unsigned top = (w0 & 0xfff) >> 2;
            unsigned right = ((w1 >> 12) & 0xfff) >> 2;
            unsigned bottom = (w1 & 0xfff) >> 2;
            unsigned bytes_per_texel = sTextureSize == G_IM_SIZ_32b ? 4 :
                                       sTextureSize == G_IM_SIZ_16b ? 2 : 1;
            sLoadedOriginS = left;
            sLoadedOriginT = top;
            sLoadedWidth = right >= left ? right - left + 1 : 0;
            sLoadedHeight = bottom >= top ? bottom - top + 1 : 0;
            sLoadedStride = sImageWidth;
            /* The game often requests one texel beyond the row's right edge.
             * The rectangle samples only the image's actual columns. */
            if (left < sImageWidth && sLoadedWidth > sImageWidth - left)
                sLoadedWidth = sImageWidth - left;
            size_t start_pixel = (size_t)top * sImageWidth + left;
            sLoadedNibbleOffset = sTextureSize == G_IM_SIZ_4b ? start_pixel & 1 : 0;
            sLoadedImage = sTextureImage && sImageWidth &&
                           left < sImageWidth && sLoadedWidth <= sImageWidth - left
                ? (const uint8_t *)sTextureImage +
                  (sTextureSize == G_IM_SIZ_4b ? start_pixel / 2 : start_pixel * bytes_per_texel)
                : NULL;
        } else if (opcode == G_LOADTLUT) {
            invalidate_triangle_texture_cache();
            load_texture_palette(w1);
        } else if (opcode == G_LOADBLOCK) {
            invalidate_triangle_texture_cache();
            sLoadedTile = false;
            sLoadedImage = (const uint8_t *)sTextureImage;
            sLoadedOriginS = sLoadedOriginT = 0;
            sLoadedNibbleOffset = 0;
            sLoadedWidth = sLoadedHeight = 0;
        } else if (opcode == G_SETTILE) {
            invalidate_triangle_texture_cache();
            if (((w1 >> 24) & 7) == G_TX_LOADTILE && (w0 & 0x1ff) >= 256)
                sPaletteLoadOffset = (w0 & 0x1ff) - 256;
            if (((w1 >> 24) & 7) == G_TX_RENDERTILE) {
                sTextureFormat = (w0 >> 21) & 7;
                sTextureSize = (w0 >> 19) & 3;
                sTexturePaletteBank = (w1 >> 20) & 15;
                sTextureModeS = (w1 >> 8) & 3;
                sTextureModeT = (w1 >> 18) & 3;
            }
        } else if (opcode == G_SETTILESIZE) {
            invalidate_triangle_texture_cache();
            if (((w1 >> 24) & 7) == G_TX_RENDERTILE) {
                unsigned left = (w0 >> 12) & 0xfff;
                unsigned top = w0 & 0xfff;
                unsigned right = (w1 >> 12) & 0xfff;
                unsigned bottom = w1 & 0xfff;
                sTextureWidth = right >= left ? ((right - left) >> 2) + 1 : 0;
                sTextureHeight = bottom >= top ? ((bottom - top) >> 2) + 1 : 0;
                if (sLoadedImage && !sLoadedWidth) {
                    sLoadedWidth = sTextureWidth;
                    sLoadedHeight = sTextureHeight;
                    sLoadedStride = sTextureWidth;
                }
            }
        } else if (opcode == (uint8_t)G_SETOTHERMODE_H) {
            if (((w0 >> 8) & 0xff) == G_MDSFT_CYCLETYPE &&
                (w0 & 0xff) == 2)
                sCycleType = (w1 >> G_MDSFT_CYCLETYPE) & 3;
        } else if (opcode == (uint8_t)G_SETOTHERMODE_L) {
            unsigned shift = (w0 >> 8) & 0xff;
            unsigned length = w0 & 0xff;
            if (shift < 32 && length && length <= 32 - shift) {
                uint32_t mask = (uint32_t)(((uint64_t)1 << length) - 1)
                                << shift;
                sOtherModeL = (sOtherModeL & ~mask) | (w1 & mask);
            }
        } else if (opcode == G_SETPRIMCOLOR) {
            sPrimitiveColor = w1;
        } else if (opcode == G_SETENVCOLOR) {
            sEnvironmentColor = w1;
        } else if (opcode == G_SETCOMBINE) {
            set_triangle_combiner(w0, w1);
            sRectColorSource = (w0 >> 15) & 31;
            sRectAlphaSource = (w0 >> 9) & 7;
        } else if (opcode == G_SETFILLCOLOR) {
            sFillColor = (uint16_t)w1;
        } else if (opcode == G_SETCIMG) {
            sColorTarget = w1 != (uint32_t)gPhysicalZBuffer;
        } else if (opcode == G_FILLRECT) {
            fill_rect(w0, w1);
        } else if (opcode == G_TEXRECT || opcode == G_TEXRECTFLIP) {
            ++sTextureRects;
#ifdef F3D_OLD
            const unsigned st_opcode = (uint8_t)G_RDPHALF_2;
            const unsigned derivatives_opcode = (uint8_t)G_RDPHALF_CONT;
#else
            const unsigned st_opcode = (uint8_t)G_RDPHALF_1;
            const unsigned derivatives_opcode = (uint8_t)G_RDPHALF_2;
#endif
            if (end - command >= 3 &&
                (command[1].words.w0 >> 24) == st_opcode &&
                (command[2].words.w0 >> 24) == derivatives_opcode) {
                texture_rect(w0, w1, command[1].words.w1,
                             command[2].words.w1, opcode == G_TEXRECTFLIP);
                command += 2;
                sCommands += 2;
                ++sOpcodeCounts[st_opcode];
                ++sOpcodeCounts[derivatives_opcode];
            } else {
                ++sSkippedTextureRects;
            }
        }
        if (perf_bucket >= 0)
            sPerfUs[perf_bucket] += (uint32_t)(rg_system_timer() - command_started);
        ++command;
    }
    if (command >= end) {
        ++sUnclosedLists;
        if (sUnclosedLists <= 8)
            RG_LOGW("MK64 DL without END: start %p, limit %p, depth %u, "
                    "first %08x %08x", start, end, depth,
                    (unsigned)start->words.w0, (unsigned)start->words.w1);
        if (sUnclosedLists <= 2)
            for (Gfx *sample = start; sample < end && sample < start + 8; ++sample)
                do { RG_LOGD("MK64 unclosed DL %u cmd %u: %08x %08x",
                        sUnclosedLists, (unsigned)(sample - start),
                        (unsigned)sample->words.w0, (unsigned)sample->words.w1); } while (0);
    }
}

/* Give the color target first claim on internal RAM for the framebuffer
 * experiment. The depth allocator below observes the remaining headroom. */
static void allocate_color_buffer(void)
{
    if (sSurface) return;
    const size_t bytes = sizeof(rg_surface_t) +
                         MK64_WORLD_WIDTH * MK64_WORLD_HEIGHT * sizeof(uint16_t);
    const size_t reserve = 128 * 1024;
    const unsigned caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    size_t available = heap_caps_get_free_size(caps);
    size_t largest = heap_caps_get_largest_free_block(caps);
    unsigned flags = available >= bytes + reserve && largest >= bytes ? MEM_FAST : MEM_SLOW;
    sSurface = rg_surface_create(MK64_WORLD_WIDTH, MK64_WORLD_HEIGHT, RG_PIXEL_565_LE, flags);
    RG_LOGI("MK64 raster output: %ux%u, logical 320x240, scale %u",
            MK64_WORLD_WIDTH, MK64_WORLD_HEIGHT, MK64_RENDER_SCALE);
    if (sSurface)
        RG_LOGI("MK64 color buffer: %p %u bytes in %s (requested %s)",
                sSurface->data, (unsigned)bytes,
                esp_ptr_internal(sSurface->data) ? "internal RAM" : "PSRAM",
                flags == MEM_FAST ? "internal RAM" : "PSRAM");
}

/* The depth image is a hot per-fragment table. Use internal RAM when it
 * fits without consuming the headroom needed by GUI, driver and task work.
 * MEM_FAST can fall back, so log actual placement as well as the request. */
static void allocate_depth_buffer(void)
{
    if (sDepth) return;
    const size_t bytes = MK64_WORLD_WIDTH * MK64_WORLD_HEIGHT * sizeof(uint16_t);
    const size_t reserve = 128 * 1024;
    const unsigned caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    size_t available = heap_caps_get_free_size(caps);
    size_t largest = heap_caps_get_largest_free_block(caps);
    unsigned flags = available >= bytes + reserve && largest >= bytes ? MEM_FAST : MEM_SLOW;
    sDepth = rg_alloc(bytes, flags);
    RG_LOGI("MK64 depth buffer: %p %u bytes in %s (requested %s)",
            sDepth, (unsigned)bytes, esp_ptr_internal(sDepth) ? "internal RAM" : "PSRAM",
            flags == MEM_FAST ? "internal RAM" : "PSRAM");
    RG_LOGI("MK64 internal RAM after depth: free %u largest %u bytes",
            (unsigned)heap_caps_get_free_size(caps),
            (unsigned)heap_caps_get_largest_free_block(caps));
}

static void allocate_back_buffer(void)
{
    if (sBackSurfaceTried || !sSurface) return;
    sBackSurfaceTried = true;
    const size_t bytes = sizeof(rg_surface_t) +
                         MK64_WORLD_WIDTH * MK64_WORLD_HEIGHT * sizeof(uint16_t);
    const unsigned caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (heap_caps_get_free_size(caps) >= bytes + 128 * 1024 &&
        heap_caps_get_largest_free_block(caps) >= bytes) {
        sBackSurface = rg_surface_create(MK64_WORLD_WIDTH, MK64_WORLD_HEIGHT,
                                         RG_PIXEL_565_LE, MEM_FAST);
        if (sBackSurface && !esp_ptr_internal(sBackSurface->data)) {
            rg_surface_free(sBackSurface);
            sBackSurface = NULL;
        }
    }
    RG_LOGI("MK64 display buffering: %s, extra %u bytes, internal free %u largest %u",
            sBackSurface ? "double, internal RAM" : "single fallback",
            sBackSurface ? (unsigned)bytes : 0,
            (unsigned)heap_caps_get_free_size(caps),
            (unsigned)heap_caps_get_largest_free_block(caps));
}

/* World color/depth retain their existing internal-RAM placement. The
 * display task reads a separate PSRAM pair containing native-resolution UI. */
static void allocate_output_buffers(void)
{
    if (sOutputTried || MK64_RENDER_SCALE == 1) return;
    sOutputTried = true;
    sOutputSurface = rg_surface_create(MK64_OUTPUT_WIDTH, MK64_OUTPUT_HEIGHT,
                                      RG_PIXEL_565_LE, MEM_SLOW);
    sOutputBackSurface = rg_surface_create(MK64_OUTPUT_WIDTH, MK64_OUTPUT_HEIGHT,
                                          RG_PIXEL_565_LE, MEM_SLOW);
    /* Menu flag/other 3D menu elements still need depth at their raster size.
     * This table is cold during world rendering and belongs in PSRAM too. */
    sUiDepth = rg_alloc(MK64_OUTPUT_WIDTH * MK64_OUTPUT_HEIGHT * sizeof(uint16_t), MEM_SLOW);
    if (!sOutputSurface || !sOutputBackSurface || !sUiDepth) {
        if (sOutputSurface) rg_surface_free(sOutputSurface);
        if (sOutputBackSurface) rg_surface_free(sOutputBackSurface);
        free(sUiDepth);
        sOutputSurface = sOutputBackSurface = NULL;
        sUiDepth = NULL;
    }
    RG_LOGI("MK64 UI composition: %s, PSRAM color/depth %u bytes",
            sOutputSurface ? "320x240 double buffered" : "low-resolution fallback",
            sOutputSurface ? MK64_OUTPUT_WIDTH * MK64_OUTPUT_HEIGHT * 6u : 0);
}

static void wait_for_color_buffer(void)
{
    /* With one surface, the display still owns these pixels until its queued
     * update is consumed. With two, submit's one-slot queue releases the
     * previous surface before returning, so the next draw buffer is free. */
    if (!sBackSurface && (!sOutputSurface || sCompleteSurface == sSurface))
        while (rg_display_is_busy()) rg_task_yield();
}

static void swap_color_buffer(void)
{
    if (sOutputBackSurface) {
        rg_surface_t *completed = sOutputSurface;
        sOutputSurface = sOutputBackSurface;
        sOutputBackSurface = completed;
    }
    if (!sBackSurface) return;
    rg_surface_t *completed = sSurface;
    sSurface = sBackSurface;
    sBackSurface = completed;
}

static void allocate_modulate_table(void)
{
    if (sModulateTableTried) return;
    sModulateTableTried = true;
    const size_t bytes = 256 * (32 + 64);
    const unsigned caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (heap_caps_get_free_size(caps) >= bytes + 128 * 1024 &&
        heap_caps_get_largest_free_block(caps) >= bytes) {
        sModulateTable = rg_alloc(bytes, MEM_FAST);
        /* Keep the scalar shader if MEM_FAST fell back to external RAM. */
        if (sModulateTable && !esp_ptr_internal(sModulateTable)) {
            free(sModulateTable);
            sModulateTable = NULL;
        }
        if (sModulateTable) build_modulate_table(sModulateTable);
    }
    RG_LOGI("MK64 modulation lookup: %s (%u bytes), internal free %u largest %u",
            sModulateTable ? "internal RAM" : "scalar fallback",
            sModulateTable ? (unsigned)bytes : 0,
            (unsigned)heap_caps_get_free_size(caps),
            (unsigned)heap_caps_get_largest_free_block(caps));
}

static void allocate_texture_cache(void)
{
    if (sTextureCacheTried) return;
    sTextureCacheTried = true;
    const size_t bytes = 4096 * sizeof(*sTextureCache);
    const unsigned caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (heap_caps_get_free_size(caps) >= bytes + 128 * 1024 &&
        heap_caps_get_largest_free_block(caps) >= bytes) {
        sTextureCache = rg_alloc(bytes, MEM_FAST);
        if (sTextureCache && !esp_ptr_internal(sTextureCache)) {
            free(sTextureCache);
            sTextureCache = NULL;
        }
    }
    RG_LOGI("MK64 texel cache: %s (%u bytes), internal free %u largest %u",
            sTextureCache ? "internal RAM" : "uncached fallback",
            sTextureCache ? (unsigned)bytes : 0,
            (unsigned)heap_caps_get_free_size(caps),
            (unsigned)heap_caps_get_largest_free_block(caps));
}

void mk64_rg_gfx_begin(void)
{
    int64_t started = rg_system_timer();
    sProfileFrame = MK64_RG_PROFILE && ++sRasterFrames % MK64_RG_REPORT_INTERVAL == 0;
    memset(sPerfUs, 0, sizeof(sPerfUs));
    sCompositionUs = sWorldTriangleUs = sWorldRectangleUs = 0;
    sPixelUs = sBoxPixels = sWrittenPixels = sEarlyDepthRejects = sFastPixels = 0;
    sSpanPixels = 0;
    sWhitePixels = 0;
    sLookupPixels = 0;
    sSkippedAttributeSteps = 0;
    sUniformShaderReuse = 0;
    sDecalPixels = 0;
    if (MK64_RG_PIXEL_PROBE) {
        sPixelProbeCursor = 0;
        memset(sPixelProbe, 0, sizeof(sPixelProbe));
    }
    sTextureCacheHits = sTextureCacheMisses = 0;
    invalidate_triangle_texture_cache();
    memset(sClipRejects, 0, sizeof(sClipRejects));
    sAlphaRejects = sTextureSampleFailures = 0;
    wait_for_color_buffer();
    sDisplayWaitUs = (uint32_t)(rg_system_timer() - started);
    allocate_color_buffer();
    allocate_depth_buffer();
    allocate_back_buffer();
    allocate_modulate_table();
    allocate_texture_cache();
    allocate_output_buffers();
    sHighResolution = false;
    sUiDepthCleared = false;
    sRasterWidth = MK64_WORLD_WIDTH;
    sRasterHeight = MK64_WORLD_HEIGHT;
    sRasterScale = MK64_RENDER_SCALE;
    sRasterPixels = sSurface ? sSurface->data : NULL;
    sRasterDepth = sDepth;
    if (sSurface)
        rg_surface_fill(sSurface, NULL, 0);
    if (sDepth)
        memset(sDepth, 0xff, MK64_WORLD_WIDTH * MK64_WORLD_HEIGHT *
                              sizeof(uint16_t));
    sFillColor = 0;
    sPrimitiveColor = 0xffffffffu;
    sEnvironmentColor = 0xffffffffu;
    sCycleType = G_CYC_1CYCLE >> G_MDSFT_CYCLETYPE;
    sOtherModeL = 0;
    sGeometryMode = 0;
    sTextureImage = NULL;
    sImageWidth = 0;
    sLoadedImage = NULL;
    sLoadedStride = sLoadedWidth = sLoadedHeight = 0;
    sLoadedOriginS = sLoadedOriginT = 0;
    sLoadedNibbleOffset = 0;
    sLoadedTile = false;
    sLoadedSize = 0;
    sTextureRects = sSkippedTextureRects = sTexturePixels = 0;
    sRectColorSource = sRectAlphaSource = 0;
    Gfx default_combiner = gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE);
    set_triangle_combiner(default_combiner.words.w0, default_combiner.words.w1);
    sTextureWidth = sTextureHeight = 0;
    sTextureModeS = sTextureModeT = 0;
    sTextureFormat = sTextureSize = 0;
    sTexturePaletteBank = sPaletteEntries = sPaletteLoadOffset = 0;
    sTextureEnabled = false;
    sTextureScaleS = sTextureScaleT = 1.0f;
    memset(sLights, 0, sizeof(sLights));
    sLights[0].col[0] = sLights[0].col[1] = sLights[0].col[2] = 255;
    sLights[0].dir[0] = sLights[0].dir[1] = 40;
    sLights[0].dir[2] = 20;
    sLights[1].col[0] = sLights[1].col[1] = sLights[1].col[2] = 31;
    sNumLights = 2;
    sLightLoads = 0;
    sColorTarget = true;
    matrix_identity(sProjection);
    matrix_identity(sModelview[0]);
    sMatrixTop = 0;
    memset(sVertices, 0, sizeof(sVertices));
    sBeginUs = (uint32_t)(rg_system_timer() - started);
}

void mk64_rg_gfx_walk(Gfx *list, size_t count, unsigned frame)
{
    int64_t started = rg_system_timer();
    memset(sOpcodeCounts, 0, sizeof(sOpcodeCounts));
    sCommands = sInvalidLists = sUnclosedLists = sFillRects = 0;
    sVertexLoads = sTriangles = sVisibleTriangles = sBadGeometry = 0;
    sTraceGeometry = MK64_RG_VERBOSE_GFX && frame == 24;
    sTraceAssets = MK64_RG_VERBOSE_GFX && (frame == 120 || frame == 300 || frame == 360 || frame == 420);
    sFrame = frame;
    sTraceBorders = MK64_RG_VERBOSE_GFX && frame >= 240 && frame <= 600 && frame % 8 == 0;
    sSpriteTraces = 0;
    sSpriteSetupTraces = 0;
    sTraceSegmentB = MK64_RG_VERBOSE_GFX && (frame == 2 || frame == 120);
    sTextureImages = 0;
    sTexturedTriangles = sDepthTriangles = 0;
    sCulledTriangles = sBlendedTriangles = sClippedTriangles = 0;
    sSegmentBLists = sLightColorCommands = sSkippedUnmappedLists = 0;
    if (count)
        walk_list(list, list + count, 0);
    sWalkUs = (uint32_t)(rg_system_timer() - started);
    if (MK64_RG_VERBOSE_GFX && (frame == 2 || frame == 4 || frame == 24 || frame % 60 == 0)) {
        do { RG_LOGD("MK64 gfx frame %u: %u commands incl. nested, %u fills, %u "
                "invalid and %u unclosed lists, walk %d us", frame,
                sCommands, sFillRects, sInvalidLists, sUnclosedLists,
                (int)(rg_system_timer() - started)); } while (0);
        do { RG_LOGD("MK64 gfx ops: DL %u END %u VTX %u TRI1 %u TRI2 %u "
                "QUAD %u TEXRECT %u FILL %u SETTIMG %u", sOpcodeCounts[G_DL],
                sOpcodeCounts[(uint8_t)G_ENDDL], sOpcodeCounts[(uint8_t)G_VTX],
                sOpcodeCounts[(uint8_t)G_TRI1], sOpcodeCounts[(uint8_t)G_TRI2],
                sOpcodeCounts[(uint8_t)G_QUAD],
                sOpcodeCounts[G_TEXRECT], sOpcodeCounts[G_FILLRECT],
                sOpcodeCounts[G_SETTIMG]); } while (0);
        do { RG_LOGD("MK64 geometry: %u vertex loads, %u triangles, %u visible, "
                "%u rejected, %u clipped", sVertexLoads, sTriangles, sVisibleTriangles,
                sBadGeometry, sClippedTriangles); } while (0);
        do { RG_LOGD("MK64 raster: %u textured, %u depth-tested, %u culled, "
                "%u blended triangles, texture %ux%u format %u size %u",
                sTexturedTriangles, sDepthTriangles,
                sCulledTriangles, sBlendedTriangles,
                sTextureWidth, sTextureHeight, sTextureFormat,
                sTextureSize); } while (0);
        do { RG_LOGD("MK64 texture rectangles: %u drawn, %u skipped, %u pixels; last load "
                "%ux%u stride %u origin %u,%u",
                sTextureRects - sSkippedTextureRects, sSkippedTextureRects,
                sTexturePixels, sLoadedWidth, sLoadedHeight, sLoadedStride,
                sLoadedOriginS, sLoadedOriginT); } while (0);
        do { RG_LOGD("MK64 shading: %u light loads, %u active lights, "
                "%u light-color commands, MOVEMEM %u MOVEWORD %u "
                "SETGEOM %u CLEARGEOM %u "
                "PRIMCOLOR %u SETCOMBINE %u",
                sLightLoads, sNumLights, sLightColorCommands,
                sOpcodeCounts[(uint8_t)G_MOVEMEM],
                sOpcodeCounts[(uint8_t)G_MOVEWORD],
                sOpcodeCounts[(uint8_t)G_SETGEOMETRYMODE],
                sOpcodeCounts[(uint8_t)G_CLEARGEOMETRYMODE],
                sOpcodeCounts[G_SETPRIMCOLOR], sOpcodeCounts[G_SETCOMBINE]); } while (0);
        do { RG_LOGD("MK64 unmapped segment-B lists skipped: %u",
                sSkippedUnmappedLists); } while (0);
    }
}

/* Requests arrive during game/display-list construction, before begin clears
 * the next draw buffer. Reading the completed surface is safe while Retro-Go
 * also reads it, and preserves previous-image feedback on skipped ticks. */
void mk64_rg_gfx_capture(int x, int y, int width, int height, void *target)
{
    if (!target || x < 0 || y < 0 || width <= 0 || height <= 0 ||
        width > 64 || height > 32 || x > 320 - width || y > 240 - height)
        return;
    uintptr_t address = (uintptr_t)target, base = (uintptr_t)gPortMemoryPool;
    unsigned bytes = (unsigned)(width * height * 2);
    if (address < base || address - base > PORT_MEMORY_POOL_SIZE - bytes)
        return; /* Destinations must be writable course textures in the pool. */
    rg_surface_t *surface = __atomic_load_n(&sCompleteSurface, __ATOMIC_ACQUIRE);
    if (!surface || !surface->data || surface->width <= 0 || surface->width > 320 ||
        surface->height <= 0 || surface->height > 240 || surface->stride < surface->width * 2)
        return;
    RGCapture *slot = NULL;
    for (unsigned i = 0; i < MK64_CAPTURE_SLOTS; ++i) {
        if (sCaptures[i].valid && sCaptures[i].target == target) {
            slot = &sCaptures[i];
            if (slot->frame == sCompleteFrame && slot->x == x && slot->y == y &&
                slot->width == width && slot->height == height)
                return;
            break;
        }
    }
    if (!slot) slot = &sCaptures[sCaptureSlot++ % MK64_CAPTURE_SLOTS];
    int64_t started = rg_system_timer();
    uint8_t *output = target;
    uint16_t columns[64];
    for (int col = 0; col < width; ++col)
        columns[col] = (unsigned)(x + col) * surface->width / 320;
    for (int row = 0; row < height; ++row) {
        unsigned sy = (unsigned)(y + row) * surface->height / 240;
        const uint16_t *pixels = (const uint16_t *)((const uint8_t *)surface->data + sy * surface->stride);
        for (int col = 0; col < width; ++col) {
            uint16_t p = pixels[columns[col]];
            /* Native little-endian RGB565 -> big-endian N64 RGBA5551. */
            uint16_t rgba = (p & 0xffc0) | ((p & 31) << 1) | 1;
            *output++ = rgba >> 8;
            *output++ = rgba;
        }
    }
    *slot = (RGCapture){target, sCompleteFrame, x, y, width, height, true};
    unsigned elapsed = (unsigned)(rg_system_timer() - started);
    ++sCaptureStats.copies;
    sCaptureStats.us += elapsed;
    if (elapsed > sCaptureStats.max_us) sCaptureStats.max_us = elapsed;
}

Mk64CaptureStats mk64_rg_gfx_capture_stats(void)
{
    Mk64CaptureStats result = sCaptureStats;
    sCaptureStats = (Mk64CaptureStats){0};
    return result;
}

bool mk64_rg_gfx_present(unsigned frame)
{
    if (!sSurface)
        return false;
    if (MK64_RG_VERBOSE_GFX && (frame == 24 || frame % 60 == 0)) {
        const uint16_t *pixels = sRasterPixels;
        unsigned nonblack = 0;
        int min_x = MK64_SCREEN_WIDTH, min_y = MK64_SCREEN_HEIGHT;
        int max_x = -1, max_y = -1;
        for (int y = 0; y < MK64_SCREEN_HEIGHT; ++y)
            for (int x = 0; x < MK64_SCREEN_WIDTH; ++x)
                if (pixels[y * MK64_SCREEN_WIDTH + x]) {
                    ++nonblack;
                    if (x < min_x) min_x = x;
                    if (y < min_y) min_y = y;
                    if (x > max_x) max_x = x;
                    if (y > max_y) max_y = y;
                }
        do { RG_LOGD("MK64 frame %u surface: %u nonblack pixels, bounds "
                "%d,%d..%d,%d", frame, nonblack, min_x, min_y, max_x, max_y); } while (0);
    }
    int64_t started = rg_system_timer();
    /* World-only frames (attract demo/intro/split-screen) need no native UI
     * composition. Retro-Go already scales their original internal surface. */
    rg_surface_t *completed = sHighResolution ? sOutputSurface : sSurface;
    sCompleteFrame = frame;
    __atomic_store_n(&sCompleteSurface, completed, __ATOMIC_RELEASE);
    rg_display_submit(completed, 0);
    unsigned submit_us = (uint32_t)(rg_system_timer() - started);
    (void)submit_us;
    if (sProfileFrame) {
        unsigned accounted = 0;
        for (unsigned i = 0; i < PERF_BUCKETS; ++i) accounted += sPerfUs[i];
        do { RG_LOGD("MK64 resolution frame %u us: worldtri %u uitri %u worldrect %u uirect %u upscale %u",
                frame,
                sHighResolution ? sWorldTriangleUs : sPerfUs[PERF_TRIANGLE],
                sHighResolution ? sPerfUs[PERF_TRIANGLE] - sWorldTriangleUs : 0,
                sHighResolution ? sWorldRectangleUs : sPerfUs[PERF_RECTANGLE],
                sHighResolution ? sPerfUs[PERF_RECTANGLE] - sWorldRectangleUs : 0,
                sCompositionUs); } while (0);
        /* Retro-Go's log buffer is 300 bytes including its prefix. */
        do { RG_LOGD("MK64 perf frame %u us: begin %u wait %u walk %u tri %u pix %u "
                "rect %u vtx %u mtx %u tex %u fill %u other %u submit %u",
                frame, sBeginUs, sDisplayWaitUs, sWalkUs,
                sPerfUs[PERF_TRIANGLE], sPixelUs, sPerfUs[PERF_RECTANGLE],
                sPerfUs[PERF_VERTEX], sPerfUs[PERF_MATRIX], sPerfUs[PERF_TEXTURE],
                sPerfUs[PERF_FILL], sWalkUs >= accounted ? sWalkUs - accounted : 0,
                submit_us); } while (0);
        do { RG_LOGD("MK64 work frame %u: box %u span %u write %u earlyZ %u fast %u white %u lut %u "
                "triangles %u visible %u textri %u clipped %u bad %u lists %u/%u",
                frame, sBoxPixels, sSpanPixels, sWrittenPixels, sEarlyDepthRejects, sFastPixels, sWhitePixels, sLookupPixels,
                sTriangles, sVisibleTriangles, sTexturedTriangles, sClippedTriangles,
                sBadGeometry, sInvalidLists, sUnclosedLists); } while (0);
        do { RG_LOGD("MK64 visibility frame %u: clip eye %u near %u far %u "
                "left %u right %u bottom %u top %u cull %u alpha %u texfail %u",
                frame, sClipRejects[0], sClipRejects[1], sClipRejects[2],
                sClipRejects[3], sClipRejects[4], sClipRejects[5], sClipRejects[6],
                sCulledTriangles, sAlphaRejects, sTextureSampleFailures); } while (0);
#if MK64_RG_PIXEL_PROBE
        do { RG_LOGD("MK64 pixelprobe frame %u cycles/sample: depth %u/%u coord %u/%u fetch %u/%u shade %u/%u",
                frame, (unsigned)sPixelProbe[PIX_DEPTH].cycles, sPixelProbe[PIX_DEPTH].samples,
                (unsigned)sPixelProbe[PIX_COORD].cycles, sPixelProbe[PIX_COORD].samples,
                (unsigned)sPixelProbe[PIX_FETCH].cycles, sPixelProbe[PIX_FETCH].samples,
                (unsigned)sPixelProbe[PIX_SHADE].cycles, sPixelProbe[PIX_SHADE].samples); } while (0);
#endif
        do { RG_LOGD("MK64 texcache frame %u: hits %u misses %u skipattr %u shadecache %u decal %u",
                frame, sTextureCacheHits, sTextureCacheMisses, sSkippedAttributeSteps, sUniformShaderReuse, sDecalPixels); } while (0);
    }
    swap_color_buffer();
    return true;
}

/* GUI callbacks run while rendering is paused. Use the submitted frame,
 * not sSurface, which already points at the next draw buffer. */
bool mk64_rg_gfx_screenshot(const char *filename, int width, int height)
{
    rg_surface_t *surface = __atomic_load_n(&sCompleteSurface, __ATOMIC_ACQUIRE);
    return surface && rg_surface_save_image_file(surface, filename, width, height);
}
void mk64_rg_gfx_redraw(void)
{
    rg_surface_t *surface = __atomic_load_n(&sCompleteSurface, __ATOMIC_ACQUIRE);
    if (surface) rg_display_submit(surface, 0);
}
