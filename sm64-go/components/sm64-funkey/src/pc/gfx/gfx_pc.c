#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <rg_system.h>
#include <assert.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include <PR/gbi.h>

#include "gfx_pc.h"
#include "config.h"
#include "gfx_cc.h"
#include "gfx_window_manager_api.h"
#include "gfx_rendering_api.h"
#include "gfx_screen_config.h"
#ifdef ENABLE_SOFTRAST
#include "gfx_soft.h"
#endif

#include "pc/configfile.h"

#define SUPPORT_CHECK(x) assert(x)
#define GFX_RENDER_DIAG 0
#define GFX_RENDER_COUNTERS 0
#define GFX_LOGI(...) do { if (GFX_RENDER_DIAG) { RG_LOGI(__VA_ARGS__); } } while (0)

// SCALE_M_N: upscale/downscale M-bit integer to N-bit
#define SCALE_5_8(VAL_) (((VAL_) * 0xFF) / 0x1F)
#define SCALE_8_5(VAL_) ((((VAL_) + 4) * 0x1F) / 0xFF)
#define SCALE_4_8(VAL_) ((VAL_) * 0x11)
#define SCALE_8_4(VAL_) ((VAL_) / 0x11)
#define SCALE_3_8(VAL_) ((VAL_) * 0x24)
#define SCALE_8_3(VAL_) ((VAL_) / 0x24)

#define HALF_SCREEN_WIDTH (SCREEN_WIDTH / 2)
#define HALF_SCREEN_HEIGHT (SCREEN_HEIGHT / 2)

#define MAX_BUFFERED 256
#define MAX_LIGHTS 2
#define MAX_VERTICES 64

// clip triangles for the software rasterizer in advance
#define GFX_MANUAL_CLIPPING 1

#ifdef ENABLE_SOFTRAST
// leave colors as 0-255 floats
#define GFX_DONT_SCALE_COLORS 1
// don't put in fog color
#define GFX_NO_FOG_COLOR 1
// premultiply by W
#define GFX_W_PREMULT 1
#endif

#ifdef GFX_DONT_SCALE_COLORS
#define GFX_COLOR_ONE 255.f
#define GFX_COLOR_CONVERT(x) (x)
#else
#define GFX_COLOR_ONE 1.f
#define GFX_COLOR_CONVERT(x) (x / 255.f)
#endif

#ifdef GFX_W_PREMULT
#define GFX_OUT_PROP(x) ((x) * w_inv)
#else
#define GFX_OUT_PROP(x) (x)
#endif

enum {
    CLIP_NONE   = 0,
    CLIP_NEAR   = 1,
    CLIP_FAR    = 2,
    CLIP_TOP    = 4,
    CLIP_BOTTOM = 8,
    CLIP_RIGHT  = 16,
    CLIP_LEFT   = 32,
    CLIP_ALL    = 63,
};

struct RGBA {
    uint8_t r, g, b, a;
};

struct XYWidthHeight {
    uint16_t x, y, width, height;
};

struct LoadedVertex {
    float x, y, z, w;
    float u, v;
    struct RGBA color;
    uint8_t clip_rej;
};

struct TextureHashmapNode {
    struct TextureHashmapNode *next;

    const uint8_t *texture_addr;
    uint8_t fmt, siz;

    uint32_t texture_id;
    uint8_t cms, cmt;
    bool linear_filter;
};
struct TextureCache {
    struct TextureHashmapNode *hashmap[2048];
    struct TextureHashmapNode pool[4096];
    uint32_t pool_pos;
};
static struct TextureCache *gfx_texture_cache;

struct ColorCombiner {
    uint32_t cc_id;
    uint32_t shader_id;
    struct ShaderProgram *prg;
    uint8_t shader_input_mapping[2][4];
};

static struct ColorCombiner color_combiner_pool[64];
static uint8_t color_combiner_pool_size;

#define RSP_MODELVIEW_STACK_CAPACITY 32

static struct RSP {
    float modelview_matrix_stack[RSP_MODELVIEW_STACK_CAPACITY][4][4];
    uint8_t modelview_matrix_stack_size;

    float MP_matrix[4][4];
    float P_matrix[4][4];

    Light_t current_lights[MAX_LIGHTS + 1];
    float current_lights_coeffs[MAX_LIGHTS][3];
    float current_lookat_coeffs[2][3]; // lookat_x, lookat_y
    uint8_t current_num_lights; // includes ambient light
    bool lights_changed;

    uint32_t geometry_mode;
    int16_t fog_mul, fog_offset;

    struct {
        // U0.16
        uint16_t s, t;
    } texture_scaling_factor;

    struct LoadedVertex loaded_vertices[MAX_VERTICES + 4];
} rsp;

static struct RDP {
    const uint8_t *palette;
    struct {
        const uint8_t *addr;
        uint8_t siz;
        uint8_t tile_number;
    } texture_to_load;
    struct {
        const uint8_t *addr;
        uint32_t size_bytes;
    } loaded_texture[2];
    struct {
        uint8_t fmt;
        uint8_t siz;
        uint8_t cms, cmt;
        uint16_t uls, ult, lrs, lrt; // U10.2
        uint32_t line_size_bytes;
    } texture_tile;
    bool textures_changed[2];

    uint32_t other_mode_l, other_mode_h;
    uint32_t combine_mode;

    struct RGBA env_color, prim_color, fog_color, fill_color;
    struct XYWidthHeight viewport, scissor;
    bool viewport_or_scissor_changed;
    void *z_buf_address;
    void *color_image_address;
} rdp;

static struct RenderingState {
    bool depth_test;
    bool depth_mask;
    bool decal_mode;
    bool alpha_blend;
    struct XYWidthHeight viewport, scissor;
    struct ShaderProgram *shader_program;
    struct TextureHashmapNode *textures[2];
} rendering_state;

struct GfxDimensions gfx_current_dimensions;
static float ratio_x = 1.f;
static float ratio_y = 1.f;

static bool dropped_frame;

static float *buf_vbo; // 3 vertices in a triangle and 26 floats per vtx
static size_t buf_vbo_len;
static size_t buf_vbo_num_tris;

void sm64_init_gfx_pc(void) {
    buf_vbo = rg_alloc(MAX_BUFFERED * (26 * 3) * sizeof(float), MEM_SLOW);
    gfx_texture_cache = rg_alloc(sizeof(*gfx_texture_cache), MEM_SLOW);
    if (!buf_vbo || !gfx_texture_cache) {
        RG_LOGE("gfx_pc: allocation failed vbo=%p texture_cache=%p",
                (void *)buf_vbo, (void *)gfx_texture_cache);
        abort();
    }
    memset(gfx_texture_cache, 0, sizeof(*gfx_texture_cache));
}

// Shared static buffer for texture import (avoids stack overflow on 40KB task stack)
static uint8_t gfx_texture_buf[32768];

static struct GfxWindowManagerAPI *gfx_wapi;
static struct GfxRenderingAPI *gfx_rapi;

// Per-frame debug counters (read & reset by main.c via getter functions)
static int gfx_dbg_flush_count;
static int gfx_dbg_tri_count;
static int gfx_dbg_tex_upload_count;
static int gfx_dbg_tex_upload_log_count;
static int gfx_dbg_texture_load_log_count;
static int gfx_dbg_textured_tri_log_count;
static int gfx_dbg_tex_rect_log_count;
static int gfx_dbg_texture_fail_log_count;
static int gfx_dbg_texture_state_log_count;
static int gfx_dbg_combiner_log_count;
static int gfx_dbg_null_texture_load_log_count;
static int gfx_dbg_null_texture_image_log_count;
static int gfx_dbg_texture_fallback_log_count;
static int gfx_dbg_texture_keep_log_count;
static int gfx_dbg_pool_guard_log_count;
static int gfx_dbg_vertex_guard_log_count;
static int gfx_dbg_light_guard_log_count;
static int gfx_dbg_dl_guard_log_count;
static int gfx_dbg_mtx_stack_log_count;
static int gfx_dbg_tile_log_count;
static int gfx_dbg_tile_size_log_count;
static int gfx_dbg_tex_rect_call_log_count;
static int gfx_dbg_title_model_log_count;
static int gfx_dbg_title_model_active;
static int gfx_dbg_title_model_loaded;
static int gfx_dbg_title_model_tex_index;
static int gfx_dbg_title_model_clip_drop;
static int gfx_dbg_title_model_cull_drop;
static int gfx_dbg_title_model_submit;
static int gfx_dbg_title_model_push;
static int gfx_dbg_title_mtx_log_count;
static int gfx_dbg_menu_model_arm_frames;
static int gfx_dbg_menu_model_active;
static int gfx_dbg_menu_model_log_count;
static int gfx_dbg_menu_model_flushes;
static int gfx_dbg_menu_model_submit;
static int gfx_dbg_menu_model_push;
static int gfx_dbg_menu_model_clip_drop;
static int gfx_dbg_menu_model_cull_drop;
static int gfx_dbg_menu_shader_log_count;
static int gfx_dbg_menu_preclip_log_count;
static int gfx_dbg_menu_mtx_log_count;
static int gfx_dbg_char_model_active;
static int gfx_dbg_char_model_shared_type;
static int gfx_dbg_char_model_target_count;
static int gfx_dbg_char_model_log_count;
static int gfx_dbg_char_model_arm_log_count;
static int gfx_dbg_char_model_matrix_log_count;
static int gfx_dbg_char_model_tri_log_count;

#define GFX_LOG_TEXTURE_TRIANGLES 0
#define GFX_LOG_TEXTURE_RECTS 0
#define GFX_TITLE_MODEL_DIAG 0
#define GFX_TITLE_MODEL_DIAG_START_TEX 40
#define GFX_TITLE_MODEL_DIAG_END_TEX 44
#define GFX_TITLE_MTX_LOG_LIMIT 40
#define GFX_MENU_MODEL_DIAG 0
#define GFX_MENU_MODEL_DIAG_FRAMES 3
#define GFX_MENU_MODEL_LOG_LIMIT 24
#define GFX_MENU_SHADER_LOG_LIMIT 0
#define GFX_MENU_PRECLIP_LOG_LIMIT 0
#define GFX_MENU_MTX_LOG_LIMIT 0
#define GFX_CHARACTER_MODEL_LOG_LIMIT 0
#define GFX_CHARACTER_MODEL_ARM_LOG_LIMIT 0
#define GFX_CHARACTER_MODEL_MTX_LOG_LIMIT 0
#define GFX_CHARACTER_MODEL_TRI_LOG_LIMIT 0
#define GFX_CHARACTER_MODEL_TARGET_LIMIT 16

#define GFX_TITLE_MODEL_DIAG_ENABLED (GFX_TITLE_MODEL_DIAG)
#define GFX_MENU_MODEL_DIAG_ENABLED (GFX_MENU_MODEL_DIAG || GFX_MENU_SHADER_LOG_LIMIT || GFX_MENU_PRECLIP_LOG_LIMIT || GFX_MENU_MTX_LOG_LIMIT)
#define GFX_CHARACTER_MODEL_DIAG_ENABLED (GFX_CHARACTER_MODEL_LOG_LIMIT || GFX_CHARACTER_MODEL_ARM_LOG_LIMIT || GFX_CHARACTER_MODEL_MTX_LOG_LIMIT || GFX_CHARACTER_MODEL_TRI_LOG_LIMIT)
#define GFX_RENDER_INFO_DIAG_ENABLED (GFX_RENDER_DIAG || GFX_LOG_TEXTURE_TRIANGLES || GFX_LOG_TEXTURE_RECTS)

static int gfx_dbg_char_model_target_type[GFX_CHARACTER_MODEL_TARGET_LIMIT];
static int gfx_dbg_char_model_target_row[GFX_CHARACTER_MODEL_TARGET_LIMIT][3];

static void gfx_dbg_title_model_note_stop(const char *reason);
static void gfx_dbg_menu_model_note_stop(const char *reason);
static bool gfx_dbg_menu_model_watched(void);

static bool gfx_dbg_is_menu_shader_id(uint32_t shader_id) {
    return shader_id == 0x01045045 ||
           shader_id == 0x05045045 ||
           shader_id == 0x01045A00;
}

static bool gfx_dbg_menu_preclip_is_target(void) {
    return gfx_dbg_menu_model_watched();
}

static bool gfx_matrix_is_finite(const float m[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (!isfinite(m[i][j])) {
                return false;
            }
        }
    }
    return true;
}

static int gfx_dbg_float_to_int(float value) {
    if (!isfinite(value)) {
        return 0;
    }
    if (value > (float)INT_MAX) {
        return INT_MAX;
    }
    if (value < (float)INT_MIN) {
        return INT_MIN;
    }
    return (int)value;
}

static bool gfx_dbg_int_close(int a, int b, int tolerance) {
    const int diff = a - b;
    return diff >= -tolerance && diff <= tolerance;
}

static void gfx_dbg_menu_mtx_log(const char *phase, uint8_t parameters,
                                 const float matrix[4][4], bool matrix_finite) {
    const float basis =
        matrix[0][0] * matrix[0][0] + matrix[0][1] * matrix[0][1] + matrix[0][2] * matrix[0][2] +
        matrix[1][0] * matrix[1][0] + matrix[1][1] * matrix[1][1] + matrix[1][2] * matrix[1][2] +
        matrix[2][0] * matrix[2][0] + matrix[2][1] * matrix[2][1] + matrix[2][2] * matrix[2][2];

    if (!gfx_dbg_menu_model_watched() || gfx_dbg_menu_mtx_log_count >= GFX_MENU_MTX_LOG_LIMIT ||
        (matrix_finite && isfinite(basis) && basis < 1000000000000.0f &&
         matrix[3][3] > 0.99f && matrix[3][3] < 1.01f)) {
        return;
    }

    GFX_LOGI("gfx_menu_mtx[%d]: %s param=%02x type=%s op=%s push=%d sp=%u finite=%d basis=%.3e in3=%.3f,%.3f,%.3f,%.3f mv3=%.3f,%.3f,%.3f,%.3f p3=%.3f,%.3f,%.3f,%.3f mp3=%.3f,%.3f,%.3f,%.3f",
            gfx_dbg_menu_mtx_log_count,
            phase,
            parameters,
            (parameters & G_MTX_PROJECTION) ? "P" : "MV",
            (parameters & G_MTX_LOAD) ? "load" : "mul",
            (parameters & G_MTX_PUSH) ? 1 : 0,
            rsp.modelview_matrix_stack_size,
            matrix_finite ? 1 : 0,
            (double)basis,
            (double)matrix[3][0], (double)matrix[3][1], (double)matrix[3][2], (double)matrix[3][3],
            (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][0],
            (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][1],
            (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][2],
            (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][3],
            (double)rsp.P_matrix[3][0],
            (double)rsp.P_matrix[3][1],
            (double)rsp.P_matrix[3][2],
            (double)rsp.P_matrix[3][3],
            (double)rsp.MP_matrix[3][0],
            (double)rsp.MP_matrix[3][1],
            (double)rsp.MP_matrix[3][2],
            (double)rsp.MP_matrix[3][3]);
    gfx_dbg_menu_mtx_log_count++;
}

static void gfx_dbg_menu_preclip_log(const char *result, uint8_t i1, uint8_t i2, uint8_t i3,
                                     const struct LoadedVertex *v1,
                                     const struct LoadedVertex *v2,
                                     const struct LoadedVertex *v3,
                                     float cull_cross, size_t emitted) {
    if (gfx_dbg_menu_preclip_log_count >= GFX_MENU_PRECLIP_LOG_LIMIT) {
        return;
    }

    const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
    const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;
    GFX_LOGI("gfx_menu_preclip[%d]: result=%s emitted=%lu tri=%u/%u/%u cc=%08lx geom=%08lx tex=%lux%lu src=%p/%lu tile=(%u,%u)-(%u,%u) uv=%ld,%ld/%ld,%ld/%ld,%ld rgba=%u,%u,%u,%u clip=%02x/%02x/%02x xyzw=%.1f,%.1f,%.1f,%.2f/%.1f,%.1f,%.1f,%.2f/%.1f,%.1f,%.1f,%.2f cross=%.3f",
            gfx_dbg_menu_preclip_log_count,
            result,
            (unsigned long)emitted,
            i1, i2, i3,
            (unsigned long)rdp.combine_mode,
            (unsigned long)rsp.geometry_mode,
            (unsigned long)tex_width, (unsigned long)tex_height,
            rdp.loaded_texture[0].addr,
            (unsigned long)rdp.loaded_texture[0].size_bytes,
            rdp.texture_tile.uls, rdp.texture_tile.ult,
            rdp.texture_tile.lrs, rdp.texture_tile.lrt,
            (long)v1->u, (long)v1->v,
            (long)v2->u, (long)v2->v,
            (long)v3->u, (long)v3->v,
            v1->color.r, v1->color.g, v1->color.b, v1->color.a,
            v1->clip_rej, v2->clip_rej, v3->clip_rej,
            (double)v1->x, (double)v1->y, (double)v1->z, (double)v1->w,
            (double)v2->x, (double)v2->y, (double)v2->z, (double)v2->w,
            (double)v3->x, (double)v3->y, (double)v3->z, (double)v3->w,
            (double)cull_cross);
    gfx_dbg_menu_preclip_log_count++;
}

static void gfx_dbg_char_tri_log(const char *result, uint8_t i1, uint8_t i2, uint8_t i3,
                                 const struct LoadedVertex *v1,
                                 const struct LoadedVertex *v2,
                                 const struct LoadedVertex *v3,
                                 float cull_cross, size_t emitted) {
    if (!gfx_dbg_char_model_active ||
        gfx_dbg_char_model_tri_log_count >= GFX_CHARACTER_MODEL_TRI_LOG_LIMIT) {
        return;
    }

    const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
    const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;
    GFX_LOGI("gfx_char_tri[%d]: result=%s emit=%lu type=%d tri=%u/%u/%u geom=%08lx tex=%lux%lu/%lu clip=%02x/%02x/%02x xw=%d/%d,%d/%d,%d/%d yw=%d/%d,%d/%d,%d/%d cross=%d",
            gfx_dbg_char_model_tri_log_count,
            result,
            (unsigned long)emitted,
            gfx_dbg_char_model_shared_type,
            i1, i2, i3,
            (unsigned long)rsp.geometry_mode,
            (unsigned long)tex_width,
            (unsigned long)tex_height,
            (unsigned long)rdp.loaded_texture[0].size_bytes,
            v1->clip_rej, v2->clip_rej, v3->clip_rej,
            gfx_dbg_float_to_int(v1->x), gfx_dbg_float_to_int(v1->w),
            gfx_dbg_float_to_int(v2->x), gfx_dbg_float_to_int(v2->w),
            gfx_dbg_float_to_int(v3->x), gfx_dbg_float_to_int(v3->w),
            gfx_dbg_float_to_int(v1->y), gfx_dbg_float_to_int(v1->w),
            gfx_dbg_float_to_int(v2->y), gfx_dbg_float_to_int(v2->w),
            gfx_dbg_float_to_int(v3->y), gfx_dbg_float_to_int(v3->w),
            gfx_dbg_float_to_int(cull_cross * 1000000.0f));
    gfx_dbg_char_model_tri_log_count++;
}

int gfx_get_debug_flush_count(void) { int n = gfx_dbg_flush_count; gfx_dbg_flush_count = 0; return n; }
int gfx_get_debug_tri_count(void)   { int n = gfx_dbg_tri_count;   gfx_dbg_tri_count = 0;   return n; }
int gfx_get_debug_tex_count(void)   { int n = gfx_dbg_tex_upload_count; gfx_dbg_tex_upload_count = 0; return n; }

void gfx_debug_arm_menu_model(int shared_type, int pos_x, int pos_y, int pos_z,
                              int scale_x, int scale_y, int scale_z) {
#if GFX_MENU_MODEL_DIAG
    if (gfx_dbg_menu_model_arm_frames <= 0) {
        gfx_dbg_menu_model_arm_frames = GFX_MENU_MODEL_DIAG_FRAMES;
        if (gfx_dbg_menu_model_log_count < GFX_MENU_MODEL_LOG_LIMIT) {
            GFX_LOGI("gfx_menu_arm[%d]: type=%d pos=%d,%d,%d scale=%d,%d,%d",
                    gfx_dbg_menu_model_log_count, shared_type, pos_x, pos_y, pos_z,
                    scale_x, scale_y, scale_z);
            gfx_dbg_menu_model_log_count++;
        }
    }
#else
    (void) shared_type;
    (void) pos_x;
    (void) pos_y;
    (void) pos_z;
    (void) scale_x;
    (void) scale_y;
    (void) scale_z;
#endif
}

void gfx_debug_arm_character_model(int shared_type, int row_x, int row_y, int row_z) {
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
    if (gfx_dbg_char_model_log_count >= GFX_CHARACTER_MODEL_LOG_LIMIT) {
        return;
    }
    if (gfx_dbg_char_model_target_count >= GFX_CHARACTER_MODEL_TARGET_LIMIT) {
        return;
    }
    if (gfx_dbg_char_model_arm_log_count < GFX_CHARACTER_MODEL_ARM_LOG_LIMIT) {
        GFX_LOGI("gfx_char_arm[%d]: type=%d row=%d,%d,%d",
                gfx_dbg_char_model_arm_log_count,
                shared_type,
                row_x,
                row_y,
                row_z);
        gfx_dbg_char_model_arm_log_count++;
    }
    gfx_dbg_char_model_target_type[gfx_dbg_char_model_target_count] = shared_type;
    gfx_dbg_char_model_target_row[gfx_dbg_char_model_target_count][0] = row_x;
    gfx_dbg_char_model_target_row[gfx_dbg_char_model_target_count][1] = row_y;
    gfx_dbg_char_model_target_row[gfx_dbg_char_model_target_count][2] = row_z;
    gfx_dbg_char_model_target_count++;
#else
    (void) shared_type;
    (void) row_x;
    (void) row_y;
    (void) row_z;
#endif
}

static void gfx_flush(void) {
    if (buf_vbo_len > 0) {
        if (GFX_RENDER_COUNTERS) {
            gfx_dbg_flush_count++;
            gfx_dbg_tri_count += buf_vbo_num_tris;
        }
        if (gfx_dbg_menu_model_active) {
            gfx_dbg_menu_model_flushes++;
        }
        gfx_rapi->draw_triangles(buf_vbo, buf_vbo_len, buf_vbo_num_tris);
        buf_vbo_len = 0;
        buf_vbo_num_tris = 0;
    }
}

static bool gfx_dbg_is_title_model_texture_load(uint32_t size_bytes) {
    return rdp.texture_to_load.siz == G_IM_SIZ_16b &&
           rdp.texture_tile.fmt == G_IM_FMT_RGBA &&
           rdp.texture_tile.line_size_bytes == 64 &&
           rdp.texture_tile.uls == 0 &&
           rdp.texture_tile.ult == 0 &&
           rdp.texture_tile.lrs == ((32 - 1) << G_TEXTURE_IMAGE_FRAC) &&
           rdp.texture_tile.lrt == ((32 - 1) << G_TEXTURE_IMAGE_FRAC) &&
           size_bytes == 2048;
}

static bool gfx_dbg_title_model_watched(void) {
    return gfx_dbg_title_model_active > 0 && gfx_dbg_title_model_loaded > 0;
}

static void gfx_dbg_title_model_note_load(const uint8_t *addr, uint32_t size_bytes) {
    if (!GFX_TITLE_MODEL_DIAG) {
        return;
    }

    if (!gfx_dbg_is_title_model_texture_load(size_bytes)) {
        return;
    }

    gfx_dbg_title_model_note_stop("next_load");

    gfx_dbg_title_model_tex_index++;
    gfx_dbg_title_model_loaded++;

    if (gfx_dbg_title_model_tex_index < GFX_TITLE_MODEL_DIAG_START_TEX ||
        gfx_dbg_title_model_tex_index > GFX_TITLE_MODEL_DIAG_END_TEX) {
        gfx_dbg_title_model_active = 0;
        return;
    }

    gfx_dbg_title_model_active = 1;
    gfx_dbg_title_model_clip_drop = 0;
    gfx_dbg_title_model_cull_drop = 0;
    gfx_dbg_title_model_submit = 0;
    gfx_dbg_title_model_push = 0;

    if (gfx_dbg_title_model_log_count < 16) {
        GFX_LOGI("gfx_title_model_load[%d]: tex=%d addr=%p bytes=%lu geom=%08lx line=%lu tile=(%u,%u)-(%u,%u) scale=%u/%u mv3=%.2f,%.2f,%.2f,%.2f p23=%.2f p32=%.2f p33=%.2f mp3=%.2f,%.2f,%.2f,%.2f",
                gfx_dbg_title_model_log_count,
                gfx_dbg_title_model_tex_index,
                addr,
                (unsigned long)size_bytes,
                (unsigned long)rsp.geometry_mode,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt,
                rsp.texture_scaling_factor.s,
                rsp.texture_scaling_factor.t,
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][0],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][1],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][2],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][3],
                (double)rsp.P_matrix[2][3],
                (double)rsp.P_matrix[3][2],
                (double)rsp.P_matrix[3][3],
                (double)rsp.MP_matrix[3][0],
                (double)rsp.MP_matrix[3][1],
                (double)rsp.MP_matrix[3][2],
                (double)rsp.MP_matrix[3][3]);
        gfx_dbg_title_model_log_count++;
    }
}

static void gfx_dbg_title_model_note_stop(const char *reason) {
    if (!gfx_dbg_title_model_watched()) {
        return;
    }

    if (gfx_dbg_title_model_log_count < 16) {
        GFX_LOGI("gfx_title_model_summary[%d]: reason=%s tex=%d loaded=%d submit=%d push=%d clip_drop=%d cull_drop=%d geom=%08lx next_addr=%p/%lu",
                gfx_dbg_title_model_log_count,
                reason,
                gfx_dbg_title_model_tex_index,
                gfx_dbg_title_model_loaded,
                gfx_dbg_title_model_submit,
                gfx_dbg_title_model_push,
                gfx_dbg_title_model_clip_drop,
                gfx_dbg_title_model_cull_drop,
                (unsigned long)rsp.geometry_mode,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes);
        gfx_dbg_title_model_log_count++;
    }

    gfx_dbg_title_model_active = 0;
}

static bool gfx_dbg_menu_model_watched(void) {
    return GFX_MENU_MODEL_DIAG && gfx_dbg_menu_model_arm_frames > 0;
}

static bool gfx_dbg_menu_model_is_target(uint32_t cc_id, const bool used_textures[2]) {
    if (!gfx_dbg_menu_model_watched() || !used_textures[0]) {
        return false;
    }

    const uint32_t shader_key = cc_id & 0x00ffffff;
    const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
    const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;
    return rdp.texture_tile.fmt == G_IM_FMT_RGBA
        && rdp.texture_tile.siz == G_IM_SIZ_16b
        && (rdp.loaded_texture[0].size_bytes == 2048 || rdp.loaded_texture[0].size_bytes == 4096)
        && (tex_width == 32 || tex_width == 64)
        && tex_height == 32
        && (shader_key == 0x00141141 || shader_key == 0x00141200);
}

static void gfx_dbg_menu_model_note_begin(const struct ColorCombiner *comb, const bool used_textures[2],
                                          bool use_alpha, const struct LoadedVertex *v1,
                                          const struct LoadedVertex *v2,
                                          const struct LoadedVertex *v3) {
    const uint32_t cc_id = comb->cc_id;
    const uint32_t shader_id = comb->shader_id;
    const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
    const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;

    if (gfx_dbg_is_menu_shader_id(shader_id) && used_textures[0] &&
        gfx_dbg_menu_shader_log_count < GFX_MENU_SHADER_LOG_LIMIT) {
        GFX_LOGI("gfx_menu_shader_tri[%d]: shader=%08lx cc=%08lx geom=%08lx alpha=%d tex=%lux%lu src=%p/%lu line=%lu tile=(%u,%u)-(%u,%u) uv=%ld,%ld/%ld,%ld/%ld,%ld rgba=%u,%u,%u,%u clip=%02x/%02x/%02x xyw=%.1f,%.1f,%.2f/%.1f,%.1f,%.2f/%.1f,%.1f,%.2f",
                gfx_dbg_menu_shader_log_count,
                (unsigned long)shader_id,
                (unsigned long)cc_id,
                (unsigned long)rsp.geometry_mode,
                use_alpha,
                (unsigned long)tex_width, (unsigned long)tex_height,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt,
                (long)v1->u, (long)v1->v,
                (long)v2->u, (long)v2->v,
                (long)v3->u, (long)v3->v,
                v1->color.r, v1->color.g, v1->color.b, v1->color.a,
                v1->clip_rej, v2->clip_rej, v3->clip_rej,
                (double)v1->x, (double)v1->y, (double)v1->w,
                (double)v2->x, (double)v2->y, (double)v2->w,
                (double)v3->x, (double)v3->y, (double)v3->w);
        gfx_dbg_menu_shader_log_count++;
    }

    if (!gfx_dbg_menu_model_is_target(cc_id, used_textures)) {
        return;
    }

    if (!gfx_dbg_menu_model_active) {
        gfx_dbg_menu_model_active = 1;
        gfx_dbg_menu_model_submit = 0;
        gfx_dbg_menu_model_push = 0;
        gfx_dbg_menu_model_clip_drop = 0;
        gfx_dbg_menu_model_cull_drop = 0;
    }

    if (gfx_dbg_menu_model_log_count < GFX_MENU_MODEL_LOG_LIMIT && gfx_dbg_menu_model_push < 6) {
        GFX_LOGI("gfx_menu_tri[%d]: cc=%08lx geom=%08lx alpha=%d tex=%lux%lu src=%p/%lu uv=%ld,%ld/%ld,%ld/%ld,%ld rgba=%u,%u,%u,%u clip=%02x/%02x/%02x w=%.2f/%.2f/%.2f",
                gfx_dbg_menu_model_log_count,
                (unsigned long)cc_id,
                (unsigned long)rsp.geometry_mode,
                use_alpha,
                (unsigned long)tex_width, (unsigned long)tex_height,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (long)v1->u, (long)v1->v,
                (long)v2->u, (long)v2->v,
                (long)v3->u, (long)v3->v,
                v1->color.r, v1->color.g, v1->color.b, v1->color.a,
                v1->clip_rej, v2->clip_rej, v3->clip_rej,
                (double)v1->w, (double)v2->w, (double)v3->w);
        gfx_dbg_menu_model_log_count++;
    }

    gfx_dbg_menu_model_push++;
}

static void gfx_dbg_menu_model_note_stop(const char *reason) {
    if (!gfx_dbg_menu_model_watched()) {
        return;
    }

    if (gfx_dbg_menu_model_log_count < GFX_MENU_MODEL_LOG_LIMIT) {
        GFX_LOGI("gfx_menu_summary[%d]: reason=%s active=%d submit=%d push=%d clip_drop=%d cull_drop=%d flushes=%d geom=%08lx tex0=%p/%lu tile=(%u,%u)-(%u,%u)",
                gfx_dbg_menu_model_log_count,
                reason,
                gfx_dbg_menu_model_active,
                gfx_dbg_menu_model_submit,
                gfx_dbg_menu_model_push,
                gfx_dbg_menu_model_clip_drop,
                gfx_dbg_menu_model_cull_drop,
                gfx_dbg_menu_model_flushes,
                (unsigned long)rsp.geometry_mode,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_menu_model_log_count++;
    }

    gfx_dbg_menu_model_active = 0;
}

static struct ShaderProgram *gfx_lookup_or_create_shader_program(uint32_t shader_id) {
    struct ShaderProgram *prg = gfx_rapi->lookup_shader(shader_id);
    if (prg == NULL) {
        gfx_rapi->unload_shader(rendering_state.shader_program);
        prg = gfx_rapi->create_and_load_new_shader(shader_id);
        rendering_state.shader_program = prg;
    }
    return prg;
}

static void gfx_generate_cc(struct ColorCombiner *comb, uint32_t cc_id) {
    uint8_t c[2][4];
    uint32_t shader_id = (cc_id >> 24) << 24;
    uint8_t shader_input_mapping[2][4] = {{0}};
    for (int i = 0; i < 4; i++) {
        c[0][i] = (cc_id >> (i * 3)) & 7;
        c[1][i] = (cc_id >> (12 + i * 3)) & 7;
    }
    for (int i = 0; i < 2; i++) {
        if (c[i][0] == c[i][1] || c[i][2] == CC_0) {
            c[i][0] = c[i][1] = c[i][2] = 0;
        }
        uint8_t input_number[8] = {0};
        int next_input_number = SHADER_INPUT_1;
        for (int j = 0; j < 4; j++) {
            int val = 0;
            switch (c[i][j]) {
                case CC_0:
                    break;
                case CC_TEXEL0:
                    val = SHADER_TEXEL0;
                    break;
                case CC_TEXEL1:
                    val = SHADER_TEXEL1;
                    break;
                case CC_TEXEL0A:
                    val = SHADER_TEXEL0A;
                    break;
                case CC_PRIM:
                case CC_SHADE:
                case CC_ENV:
                case CC_LOD:
                    if (input_number[c[i][j]] == 0) {
                        shader_input_mapping[i][next_input_number - 1] = c[i][j];
                        input_number[c[i][j]] = next_input_number++;
                    }
                    val = input_number[c[i][j]];
                    break;
            }
            shader_id |= val << (i * 12 + j * 3);
        }
    }
    comb->cc_id = cc_id;
    comb->shader_id = shader_id;
    comb->prg = gfx_lookup_or_create_shader_program(shader_id);
    memcpy(comb->shader_input_mapping, shader_input_mapping, sizeof(shader_input_mapping));

#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_combiner_log_count < 16) {
        struct CCFeatures features;
        gfx_cc_get_features(shader_id, &features);
        GFX_LOGI("gfx_cc_new[%d]: cc=%08lx shader=%08lx tex=%d/%d inputs=%d alpha=%d fog=%d c0=%u,%u,%u,%u c1=%u,%u,%u,%u",
                gfx_dbg_combiner_log_count,
                (unsigned long)cc_id, (unsigned long)shader_id,
                features.used_textures[0], features.used_textures[1],
                features.num_inputs, features.opt_alpha, features.opt_fog,
                features.c[0][0], features.c[0][1], features.c[0][2], features.c[0][3],
                features.c[1][0], features.c[1][1], features.c[1][2], features.c[1][3]);
        gfx_dbg_combiner_log_count++;
    }
#endif
}

static struct ColorCombiner *gfx_lookup_or_create_color_combiner(uint32_t cc_id) {
    static struct ColorCombiner *prev_combiner;
    if (prev_combiner != NULL && prev_combiner->cc_id == cc_id) {
        return prev_combiner;
    }

    for (size_t i = 0; i < color_combiner_pool_size; i++) {
        if (color_combiner_pool[i].cc_id == cc_id) {
            return prev_combiner = &color_combiner_pool[i];
        }
    }
    gfx_flush();
    if (color_combiner_pool_size >= sizeof(color_combiner_pool) / sizeof(color_combiner_pool[0])) {
        if (gfx_dbg_pool_guard_log_count < 8) {
            RG_LOGE("gfx_cc_pool_full[%d]: cc=%08lx size=%u",
                    gfx_dbg_pool_guard_log_count,
                    (unsigned long)cc_id, color_combiner_pool_size);
            gfx_dbg_pool_guard_log_count++;
        }
        return prev_combiner != NULL ? prev_combiner : &color_combiner_pool[0];
    }
    struct ColorCombiner *comb = &color_combiner_pool[color_combiner_pool_size++];
    gfx_generate_cc(comb, cc_id);
    return prev_combiner = comb;
}

static bool gfx_texture_cache_find(int tile, struct TextureHashmapNode **n, const uint8_t *orig_addr, uint32_t fmt, uint32_t siz) {
    if (!gfx_texture_cache) {
        return false;
    }
    size_t hash = (uintptr_t)orig_addr;
    hash = (hash >> 5) & 0x7ff;
    struct TextureHashmapNode **node = &gfx_texture_cache->hashmap[hash];
    while (*node != NULL && *node - gfx_texture_cache->pool < (int)gfx_texture_cache->pool_pos) {
        if ((*node)->texture_addr == orig_addr && (*node)->fmt == fmt && (*node)->siz == siz) {
            gfx_rapi->select_texture(tile, (*node)->texture_id);
            *n = *node;
            return true;
        }
        node = &(*node)->next;
    }

    return false;
}

static bool gfx_texture_cache_lookup(int tile, struct TextureHashmapNode **n, const uint8_t *orig_addr, uint32_t fmt, uint32_t siz) {
    if (gfx_texture_cache_find(tile, n, orig_addr, fmt, siz)) {
        return true;
    }
    if (!gfx_texture_cache) {
        return false;
    }

    size_t hash = (uintptr_t)orig_addr;
    hash = (hash >> 5) & 0x7ff;
    struct TextureHashmapNode **node = &gfx_texture_cache->hashmap[hash];
    while (*node != NULL && *node - gfx_texture_cache->pool < (int)gfx_texture_cache->pool_pos) {
        node = &(*node)->next;
    }

    if (gfx_texture_cache->pool_pos == sizeof(gfx_texture_cache->pool) / sizeof(struct TextureHashmapNode)) {
        // Pool is full. We just invalidate everything and start over.
        gfx_texture_cache->pool_pos = 0;
        memset(gfx_texture_cache->hashmap, 0, sizeof(gfx_texture_cache->hashmap));
        node = &gfx_texture_cache->hashmap[hash];
        //puts("Clearing texture cache");
    }
    *node = &gfx_texture_cache->pool[gfx_texture_cache->pool_pos++];
    if ((*node)->texture_addr == NULL) {
        (*node)->texture_id = gfx_rapi->new_texture();
    }
    gfx_rapi->select_texture(tile, (*node)->texture_id);
    gfx_rapi->set_sampler_parameters(tile, false, 0, 0);
    (*node)->cms = 0;
    (*node)->cmt = 0;
    (*node)->linear_filter = false;
    (*node)->next = NULL;
    (*node)->texture_addr = orig_addr;
    (*node)->fmt = fmt;
    (*node)->siz = siz;
    *n = *node;
    return false;
}

#if CONFIG_IDF_TARGET_ESP32S3
void gfx_pc_reset_texture_cache(void) {
    if (gfx_texture_cache) {
        gfx_texture_cache->pool_pos = 0;
        memset(gfx_texture_cache->hashmap, 0, sizeof(gfx_texture_cache->hashmap));
    }
}
#endif

struct TextureUploadInfo {
    uint32_t width;
    uint32_t height;
    size_t pixels_to_convert;
    size_t staging_bytes;
    bool needs_staging;
};

static bool texture_upload_info_set(struct TextureUploadInfo *info, size_t width, size_t height, bool needs_staging) {
    if (width == 0 || height == 0 || width > INT_MAX || height > INT_MAX) {
        return false;
    }
    if (width > ((size_t)-1) / height) {
        return false;
    }

    const size_t pixels = width * height;
    if (pixels == 0 || pixels > UINT32_MAX / 4U) {
        return false;
    }

    info->width = (uint32_t)width;
    info->height = (uint32_t)height;
    info->pixels_to_convert = pixels;
    info->needs_staging = needs_staging;
    info->staging_bytes = needs_staging ? pixels * 4 : 0;
    return true;
}

static bool texture_source_has_4b_pixels(size_t pixels, size_t size_bytes) {
    if (size_bytes > ((size_t)-1) / 2) {
        return true;
    }
    return pixels <= size_bytes * 2;
}

static bool calculate_texture_upload_info(int tile, uint8_t fmt, uint8_t siz, struct TextureUploadInfo *info) {
    if (tile < 0 || tile >= 2 || rdp.loaded_texture[tile].addr == NULL) {
        return false;
    }

    const size_t size_bytes = rdp.loaded_texture[tile].size_bytes;
    const size_t line_size = rdp.texture_tile.line_size_bytes;
    if (size_bytes == 0 || line_size == 0) {
        return false;
    }

    if (fmt == G_IM_FMT_CI && rdp.palette == NULL) {
        return false;
    }

    bool valid = false;
    switch (fmt) {
        case G_IM_FMT_RGBA:
            if (siz == G_IM_SIZ_16b) {
                valid = texture_upload_info_set(info, line_size / 2, size_bytes / line_size, true);
                return valid && info->pixels_to_convert <= size_bytes / 2;
            }
            if (siz == G_IM_SIZ_32b) {
                valid = texture_upload_info_set(info, line_size / 2, (size_bytes / 2) / line_size, false);
                return valid && info->pixels_to_convert <= size_bytes / 4;
            }
            return false;

        case G_IM_FMT_IA:
            if (siz == G_IM_SIZ_4b) {
                if (line_size > ((size_t)-1) / 2) return false;
                valid = texture_upload_info_set(info, line_size * 2, size_bytes / line_size, true);
                return valid && texture_source_has_4b_pixels(info->pixels_to_convert, size_bytes);
            }
            if (siz == G_IM_SIZ_8b) {
                valid = texture_upload_info_set(info, line_size, size_bytes / line_size, true);
                return valid && info->pixels_to_convert <= size_bytes;
            }
            if (siz == G_IM_SIZ_16b) {
                valid = texture_upload_info_set(info, line_size / 2, size_bytes / line_size, true);
                return valid && info->pixels_to_convert <= size_bytes / 2;
            }
            return false;

        case G_IM_FMT_CI:
            if (siz == G_IM_SIZ_4b) {
                if (line_size > ((size_t)-1) / 2) return false;
                valid = texture_upload_info_set(info, line_size * 2, size_bytes / line_size, true);
                return valid && texture_source_has_4b_pixels(info->pixels_to_convert, size_bytes);
            }
            if (siz == G_IM_SIZ_8b) {
                valid = texture_upload_info_set(info, line_size, size_bytes / line_size, true);
                return valid && info->pixels_to_convert <= size_bytes;
            }
            return false;

        case G_IM_FMT_I:
            if (siz == G_IM_SIZ_4b) {
                if (line_size > ((size_t)-1) / 2) return false;
                valid = texture_upload_info_set(info, line_size * 2, size_bytes / line_size, true);
                return valid && texture_source_has_4b_pixels(info->pixels_to_convert, size_bytes);
            }
            if (siz == G_IM_SIZ_8b) {
                valid = texture_upload_info_set(info, line_size, size_bytes / line_size, true);
                return valid && info->pixels_to_convert <= size_bytes;
            }
            return false;
    }

    return false;
}

static bool gfx_rdp_use_last_valid_texture(int tile) {
    if (tile != 0 || rdp.loaded_texture[0].addr != NULL || rdp.loaded_texture[1].addr == NULL) {
        return false;
    }

    rdp.loaded_texture[0] = rdp.loaded_texture[1];
    rdp.textures_changed[0] = true;

#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_texture_fallback_log_count < 8) {
        GFX_LOGI("gfx_tex_fallback[%d]: copied tile1 -> tile0 addr=%p bytes=%lu line=%lu tile=(%u,%u)-(%u,%u)",
                gfx_dbg_texture_fallback_log_count,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_texture_fallback_log_count++;
    }
#endif

    return true;
}

#if GFX_RENDER_INFO_DIAG_ENABLED
static void gfx_debug_log_texture_upload(int tile, uint8_t fmt, uint8_t siz, const struct TextureUploadInfo *info, const uint8_t *rgba32_buf) {
    if (gfx_dbg_tex_upload_log_count >= 24 || rgba32_buf == NULL || info->pixels_to_convert == 0) {
        return;
    }

    const size_t mid = info->pixels_to_convert / 2;
    const size_t last = info->pixels_to_convert - 1;
    const uint8_t *p0 = rgba32_buf;
    const uint8_t *pm = rgba32_buf + mid * 4;
    const uint8_t *pl = rgba32_buf + last * 4;

    GFX_LOGI("gfx_tex_upload[%d]: tile=%d addr=%p fmt=%u siz=%u wh=%lux%lu src=%lu line=%lu px0=%02x%02x%02x%02x mid=%02x%02x%02x%02x last=%02x%02x%02x%02x",
            gfx_dbg_tex_upload_log_count, tile, rdp.loaded_texture[tile].addr, fmt, siz,
            (unsigned long)info->width, (unsigned long)info->height,
            (unsigned long)rdp.loaded_texture[tile].size_bytes,
            (unsigned long)rdp.texture_tile.line_size_bytes,
            p0[0], p0[1], p0[2], p0[3],
            pm[0], pm[1], pm[2], pm[3],
            pl[0], pl[1], pl[2], pl[3]);
    gfx_dbg_tex_upload_log_count++;
}
#else
#define gfx_debug_log_texture_upload(...) ((void)0)
#endif

static void import_texture_rgba16(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint16_t col16 = (rdp.loaded_texture[tile].addr[2 * i] << 8) | rdp.loaded_texture[tile].addr[2 * i + 1];
        uint8_t a = col16 & 1;
        uint8_t r = col16 >> 11;
        uint8_t g = (col16 >> 6) & 0x1f;
        uint8_t b = (col16 >> 1) & 0x1f;
        rgba32_buf[4 * i + 0] = SCALE_5_8(r);
        rgba32_buf[4 * i + 1] = SCALE_5_8(g);
        rgba32_buf[4 * i + 2] = SCALE_5_8(b);
        rgba32_buf[4 * i + 3] = a ? 255 : 0;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_RGBA, G_IM_SIZ_16b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_rgba32(int tile, const struct TextureUploadInfo *info) {
    gfx_debug_log_texture_upload(tile, G_IM_FMT_RGBA, G_IM_SIZ_32b, info, rdp.loaded_texture[tile].addr);
    gfx_rapi->upload_texture(rdp.loaded_texture[tile].addr, (int)info->width, (int)info->height);
}

static void import_texture_ia4(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t byte = rdp.loaded_texture[tile].addr[i / 2];
        uint8_t part = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint8_t intensity = part >> 1;
        uint8_t alpha = part & 1;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        rgba32_buf[4 * i + 0] = SCALE_3_8(r);
        rgba32_buf[4 * i + 1] = SCALE_3_8(g);
        rgba32_buf[4 * i + 2] = SCALE_3_8(b);
        rgba32_buf[4 * i + 3] = alpha ? 255 : 0;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_IA, G_IM_SIZ_4b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_ia8(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t intensity = rdp.loaded_texture[tile].addr[i] >> 4;
        uint8_t alpha = rdp.loaded_texture[tile].addr[i] & 0xf;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        rgba32_buf[4 * i + 0] = SCALE_4_8(r);
        rgba32_buf[4 * i + 1] = SCALE_4_8(g);
        rgba32_buf[4 * i + 2] = SCALE_4_8(b);
        rgba32_buf[4 * i + 3] = SCALE_4_8(alpha);
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_IA, G_IM_SIZ_8b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_ia16(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t intensity = rdp.loaded_texture[tile].addr[2 * i];
        uint8_t alpha = rdp.loaded_texture[tile].addr[2 * i + 1];
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        rgba32_buf[4 * i + 0] = r;
        rgba32_buf[4 * i + 1] = g;
        rgba32_buf[4 * i + 2] = b;
        rgba32_buf[4 * i + 3] = alpha;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_IA, G_IM_SIZ_16b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_i4(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t byte = rdp.loaded_texture[tile].addr[i / 2];
        uint8_t part = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint8_t intensity = part;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        rgba32_buf[4 * i + 0] = SCALE_4_8(r);
        rgba32_buf[4 * i + 1] = SCALE_4_8(g);
        rgba32_buf[4 * i + 2] = SCALE_4_8(b);
        rgba32_buf[4 * i + 3] = 255;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_I, G_IM_SIZ_4b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_i8(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t intensity = rdp.loaded_texture[tile].addr[i];
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        rgba32_buf[4 * i + 0] = r;
        rgba32_buf[4 * i + 1] = g;
        rgba32_buf[4 * i + 2] = b;
        rgba32_buf[4 * i + 3] = 255;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_I, G_IM_SIZ_8b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_ci4(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t byte = rdp.loaded_texture[tile].addr[i / 2];
        uint8_t idx = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint16_t col16 = (rdp.palette[idx * 2] << 8) | rdp.palette[idx * 2 + 1]; // Big endian load
        uint8_t a = col16 & 1;
        uint8_t r = col16 >> 11;
        uint8_t g = (col16 >> 6) & 0x1f;
        uint8_t b = (col16 >> 1) & 0x1f;
        rgba32_buf[4 * i + 0] = SCALE_5_8(r);
        rgba32_buf[4 * i + 1] = SCALE_5_8(g);
        rgba32_buf[4 * i + 2] = SCALE_5_8(b);
        rgba32_buf[4 * i + 3] = a ? 255 : 0;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_CI, G_IM_SIZ_4b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static void import_texture_ci8(int tile, const struct TextureUploadInfo *info, uint8_t *rgba32_buf) {
    for (size_t i = 0; i < info->pixels_to_convert; i++) {
        uint8_t idx = rdp.loaded_texture[tile].addr[i];
        uint16_t col16 = (rdp.palette[idx * 2] << 8) | rdp.palette[idx * 2 + 1]; // Big endian load
        uint8_t a = col16 & 1;
        uint8_t r = col16 >> 11;
        uint8_t g = (col16 >> 6) & 0x1f;
        uint8_t b = (col16 >> 1) & 0x1f;
        rgba32_buf[4 * i + 0] = SCALE_5_8(r);
        rgba32_buf[4 * i + 1] = SCALE_5_8(g);
        rgba32_buf[4 * i + 2] = SCALE_5_8(b);
        rgba32_buf[4 * i + 3] = a ? 255 : 0;
    }

    gfx_debug_log_texture_upload(tile, G_IM_FMT_CI, G_IM_SIZ_8b, info, rgba32_buf);
    gfx_rapi->upload_texture(rgba32_buf, (int)info->width, (int)info->height);
}

static bool import_texture(int tile) {
    uint8_t fmt = rdp.texture_tile.fmt;
    uint8_t siz = rdp.texture_tile.siz;
    struct TextureUploadInfo info;

    gfx_rdp_use_last_valid_texture(tile);

    if (!calculate_texture_upload_info(tile, fmt, siz, &info)) {
        if (gfx_dbg_texture_fail_log_count < 16) {
            RG_LOGE("gfx_tex_import_fail[%d]: tile=%d addr=%p fmt=%u siz=%u src_bytes=%lu line=%lu palette=%p tile=(%u,%u)-(%u,%u)",
                    gfx_dbg_texture_fail_log_count, tile, rdp.loaded_texture[tile].addr, fmt, siz,
                    (unsigned long)rdp.loaded_texture[tile].size_bytes,
                    (unsigned long)rdp.texture_tile.line_size_bytes,
                    rdp.palette, rdp.texture_tile.uls, rdp.texture_tile.ult,
                    rdp.texture_tile.lrs, rdp.texture_tile.lrt);
            gfx_dbg_texture_fail_log_count++;
        }
        return false;
    }

    if (gfx_texture_cache_find(tile, &rendering_state.textures[tile], rdp.loaded_texture[tile].addr, fmt, siz)) {
        return true;
    }


    uint8_t *rgba32_buf = NULL;
    bool allocated_staging = false;
    if (info.needs_staging && info.staging_bytes > sizeof(gfx_texture_buf)) {
        rgba32_buf = rg_alloc(info.staging_bytes, MEM_SLOW);
        if (rgba32_buf == NULL) {
            return false;
        }
        allocated_staging = true;
    }

    gfx_texture_cache_lookup(tile, &rendering_state.textures[tile], rdp.loaded_texture[tile].addr, fmt, siz);

    if (info.needs_staging && rgba32_buf == NULL) {
        rgba32_buf = gfx_texture_buf;
    }

    if (GFX_RENDER_COUNTERS) {
        gfx_dbg_tex_upload_count++;
    }

    if (fmt == G_IM_FMT_RGBA) {
        if (siz == G_IM_SIZ_16b) {
            import_texture_rgba16(tile, &info, rgba32_buf);
        } else {
            import_texture_rgba32(tile, &info);
        }
    } else if (fmt == G_IM_FMT_IA) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_ia4(tile, &info, rgba32_buf);
        } else if (siz == G_IM_SIZ_8b) {
            import_texture_ia8(tile, &info, rgba32_buf);
        } else {
            import_texture_ia16(tile, &info, rgba32_buf);
        }
    } else if (fmt == G_IM_FMT_CI) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_ci4(tile, &info, rgba32_buf);
        } else {
            import_texture_ci8(tile, &info, rgba32_buf);
        }
    } else if (fmt == G_IM_FMT_I) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_i4(tile, &info, rgba32_buf);
        } else {
            import_texture_i8(tile, &info, rgba32_buf);
        }
    } else {
        if (allocated_staging) {
            free(rgba32_buf);
        }
        return false;
    }

    if (allocated_staging) {
        free(rgba32_buf);
    }
    return true;
}

static inline float rsqrtf(const float x) {
    const float x2 = x * 0.5f;
    union { float f; int32_t i; } u;
    u.f = x;
    u.i = 0x5f375a86 - (u.i >> 1);
    float y = u.f;
    y = y * (1.5f - (x2 * y * y));
    return y;
}

static inline void gfx_normalize_vector(float v[3]) {
    const float s = rsqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

static inline void gfx_transposed_matrix_mul(float *restrict res, const float *restrict a, const float (*restrict b)[4]) {
    res[0] = a[0] * b[0][0] + a[1] * b[0][1] + a[2] * b[0][2];
    res[1] = a[0] * b[1][0] + a[1] * b[1][1] + a[2] * b[1][2];
    res[2] = a[0] * b[2][0] + a[1] * b[2][1] + a[2] * b[2][2];
}

static inline void calculate_normal_dir(const Light_t *light, float coeffs[3]) {
    float light_dir[3] = {
        light->dir[0] / 127.0f,
        light->dir[1] / 127.0f,
        light->dir[2] / 127.0f
    };
    gfx_transposed_matrix_mul(coeffs, light_dir, rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1]);
    gfx_normalize_vector(coeffs);
}

static inline void gfx_matrix_mul_inplace(const float (*restrict a)[4], float (*restrict res)[4]) {
    float tmp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp[i][j] = a[i][0] * res[0][j] +
                        a[i][1] * res[1][j] +
                        a[i][2] * res[2][j] +
                        a[i][3] * res[3][j];
        }
    }
    memcpy(res, tmp, sizeof(tmp));
}

static inline void gfx_matrix_mul(float (*restrict res)[4], const float (*restrict a)[4], const float (*restrict b)[4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            res[i][j] = a[i][0] * b[0][j] +
                        a[i][1] * b[1][j] +
                        a[i][2] * b[2][j] +
                        a[i][3] * b[3][j];
        }
    }
}

static inline void gfx_matrix_identity(float (*restrict matrix)[4]) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0][0] = 1.0f;
    matrix[1][1] = 1.0f;
    matrix[2][2] = 1.0f;
    matrix[3][3] = 1.0f;
}

static void gfx_sp_matrix(uint8_t parameters, const int32_t *addr) {
    float matrix[4][4];
#ifndef GBI_FLOATS
    // Original GBI where fixed point matrices are used
    register int idx;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j += 2) {
            idx = (i << 1) + (j >> 1);
            const int32_t int_part = addr[idx];
            const uint32_t frac_part = addr[8 + idx];
            matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.f;
            matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.f;
        }
    }
#else
    // For a modified GBI where fixed point values are replaced with floats
    memcpy(matrix, addr, sizeof(matrix));
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
    const bool menu_mtx_finite = gfx_matrix_is_finite(matrix);
    if (gfx_dbg_menu_model_watched()) {
        gfx_dbg_menu_mtx_log("pre", parameters, matrix, menu_mtx_finite);
    }
#endif

#if GFX_TITLE_MODEL_DIAG_ENABLED
    const bool title_mtx_watch = GFX_TITLE_MODEL_DIAG &&
                                 gfx_dbg_title_model_tex_index >= GFX_TITLE_MODEL_DIAG_START_TEX - 2 &&
                                 gfx_dbg_title_model_tex_index <= GFX_TITLE_MODEL_DIAG_END_TEX + 1 &&
                                 gfx_dbg_title_mtx_log_count < GFX_TITLE_MTX_LOG_LIMIT;
    if (title_mtx_watch) {
        GFX_LOGI("gfx_title_mtx_pre[%d]: tex=%d addr=%p param=%02x type=%s op=%s push=%d sp=%u in3=%.2f,%.2f,%.2f,%.2f mv3=%.2f,%.2f,%.2f,%.2f p23=%.2f p32=%.2f p33=%.2f",
                gfx_dbg_title_mtx_log_count,
                gfx_dbg_title_model_tex_index,
                addr,
                parameters,
                (parameters & G_MTX_PROJECTION) ? "P" : "MV",
                (parameters & G_MTX_LOAD) ? "load" : "mul",
                (parameters & G_MTX_PUSH) ? 1 : 0,
                rsp.modelview_matrix_stack_size,
                (double)matrix[3][0],
                (double)matrix[3][1],
                (double)matrix[3][2],
                (double)matrix[3][3],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][0],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][1],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][2],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][3],
                (double)rsp.P_matrix[2][3],
                (double)rsp.P_matrix[3][2],
                (double)rsp.P_matrix[3][3]);
        gfx_dbg_title_mtx_log_count++;
    }
#endif

    if (parameters & G_MTX_PROJECTION) {
        if (parameters & G_MTX_LOAD) {
            memcpy(rsp.P_matrix, matrix, sizeof(matrix));
        } else {
            gfx_matrix_mul_inplace(matrix, rsp.P_matrix);
        }
    } else { // G_MTX_MODELVIEW
        if (parameters & G_MTX_PUSH) {
            if (rsp.modelview_matrix_stack_size < RSP_MODELVIEW_STACK_CAPACITY) {
                ++rsp.modelview_matrix_stack_size;
                memcpy(rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1], rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 2], sizeof(matrix));
            } else if (gfx_dbg_mtx_stack_log_count < 4) {
                RG_LOGE("gfx_mtx_stack_full[%d]: sp=%u param=%02x",
                        gfx_dbg_mtx_stack_log_count,
                        rsp.modelview_matrix_stack_size,
                        parameters);
                gfx_dbg_mtx_stack_log_count++;
            }
        }
        if (parameters & G_MTX_LOAD) {
            memcpy(rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1], matrix, sizeof(matrix));
        } else {
            gfx_matrix_mul_inplace(matrix, rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1]);
        }
        rsp.lights_changed = 1;
    }
    gfx_matrix_mul(rsp.MP_matrix, rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1], rsp.P_matrix);
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
    if (!(parameters & G_MTX_PROJECTION) && (parameters & G_MTX_LOAD) &&
        gfx_dbg_char_model_target_count > 0) {
        const int row_x = gfx_dbg_float_to_int(matrix[3][0]);
        const int row_y = gfx_dbg_float_to_int(matrix[3][1]);
        const int row_z = gfx_dbg_float_to_int(matrix[3][2]);
        int matched = -1;
        for (int i = 0; i < gfx_dbg_char_model_target_count; i++) {
            if (gfx_dbg_int_close(row_x, gfx_dbg_char_model_target_row[i][0], 1) &&
                gfx_dbg_int_close(row_y, gfx_dbg_char_model_target_row[i][1], 1) &&
                gfx_dbg_int_close(row_z, gfx_dbg_char_model_target_row[i][2], 1)) {
                matched = i;
                break;
            }
        }
        if (matched >= 0) {
            gfx_dbg_char_model_active = 1;
            gfx_dbg_char_model_shared_type = gfx_dbg_char_model_target_type[matched];
            for (int i = matched + 1; i < gfx_dbg_char_model_target_count; i++) {
                gfx_dbg_char_model_target_type[i - 1] = gfx_dbg_char_model_target_type[i];
                gfx_dbg_char_model_target_row[i - 1][0] = gfx_dbg_char_model_target_row[i][0];
                gfx_dbg_char_model_target_row[i - 1][1] = gfx_dbg_char_model_target_row[i][1];
                gfx_dbg_char_model_target_row[i - 1][2] = gfx_dbg_char_model_target_row[i][2];
            }
            gfx_dbg_char_model_target_count--;
        } else {
            gfx_dbg_char_model_active = 0;
        }
    } else if (!(parameters & G_MTX_PROJECTION) && (parameters & G_MTX_LOAD)) {
        gfx_dbg_char_model_active = 0;
    }
    if (gfx_dbg_char_model_active) {
        if (gfx_dbg_char_model_matrix_log_count < GFX_CHARACTER_MODEL_MTX_LOG_LIMIT) {
            GFX_LOGI("gfx_char_mtx[%d]: type=%d sp=%u row=%d,%d,%d mp3=%d,%d,%d,%d p23=%d p32=%d p33=%d geom=%08lx",
                    gfx_dbg_char_model_matrix_log_count,
                    gfx_dbg_char_model_shared_type,
                    rsp.modelview_matrix_stack_size,
                    gfx_dbg_float_to_int(matrix[3][0]),
                    gfx_dbg_float_to_int(matrix[3][1]),
                    gfx_dbg_float_to_int(matrix[3][2]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][0]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][1]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][2]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][3]),
                    gfx_dbg_float_to_int(rsp.P_matrix[2][3]),
                    gfx_dbg_float_to_int(rsp.P_matrix[3][2]),
                    gfx_dbg_float_to_int(rsp.P_matrix[3][3]),
                    (unsigned long)rsp.geometry_mode);
            gfx_dbg_char_model_matrix_log_count++;
        }
    }
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
    if (gfx_dbg_menu_model_watched()) {
        gfx_dbg_menu_mtx_log("post", parameters, matrix,
                             menu_mtx_finite &&
                             gfx_matrix_is_finite(rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1]) &&
                             gfx_matrix_is_finite(rsp.P_matrix) &&
                             gfx_matrix_is_finite(rsp.MP_matrix));
    }
#endif

#if GFX_TITLE_MODEL_DIAG_ENABLED
    if (title_mtx_watch && gfx_dbg_title_mtx_log_count < GFX_TITLE_MTX_LOG_LIMIT) {
        GFX_LOGI("gfx_title_mtx_post[%d]: tex=%d sp=%u mv3=%.2f,%.2f,%.2f,%.2f mp3=%.2f,%.2f,%.2f,%.2f",
                gfx_dbg_title_mtx_log_count,
                gfx_dbg_title_model_tex_index,
                rsp.modelview_matrix_stack_size,
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][0],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][1],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][2],
                (double)rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1][3][3],
                (double)rsp.MP_matrix[3][0],
                (double)rsp.MP_matrix[3][1],
                (double)rsp.MP_matrix[3][2],
                (double)rsp.MP_matrix[3][3]);
        gfx_dbg_title_mtx_log_count++;
    }
#endif
}

static void gfx_sp_pop_matrix(uint32_t count) {
    while (count--) {
        if (rsp.modelview_matrix_stack_size > 1) {
            --rsp.modelview_matrix_stack_size;
            if (rsp.modelview_matrix_stack_size > 0) {
                gfx_matrix_mul(rsp.MP_matrix, rsp.modelview_matrix_stack[rsp.modelview_matrix_stack_size - 1], rsp.P_matrix);
            }
        }
    }
}

static void gfx_sp_vertex(size_t n_vertices, size_t dest_index, const Vtx *vertices) {
    if (vertices == NULL || dest_index >= MAX_VERTICES || n_vertices > MAX_VERTICES - dest_index) {
        if (gfx_dbg_vertex_guard_log_count < 12) {
            RG_LOGE("gfx_vtx_oob[%d]: n=%lu dst=%lu ptr=%p max=%d",
                    gfx_dbg_vertex_guard_log_count,
                    (unsigned long)n_vertices, (unsigned long)dest_index,
                    (void *)vertices, MAX_VERTICES);
            gfx_dbg_vertex_guard_log_count++;
        }
        return;
    }

    if (rsp.current_num_lights == 0 || rsp.current_num_lights > MAX_LIGHTS + 1) {
        if (gfx_dbg_light_guard_log_count < 8) {
            RG_LOGE("gfx_light_clamp[%d]: num=%u",
                    gfx_dbg_light_guard_log_count, rsp.current_num_lights);
            gfx_dbg_light_guard_log_count++;
        }
        rsp.current_num_lights = MAX_LIGHTS + 1;
        rsp.lights_changed = true;
    }

    for (size_t i = 0; i < n_vertices; i++, dest_index++) {
        const Vtx_t *v = &vertices[i].v;
        const Vtx_tn *vn = &vertices[i].n;
        struct LoadedVertex *d = &rsp.loaded_vertices[dest_index];

        float x = v->ob[0] * rsp.MP_matrix[0][0] + v->ob[1] * rsp.MP_matrix[1][0] + v->ob[2] * rsp.MP_matrix[2][0] + rsp.MP_matrix[3][0];
        float y = v->ob[0] * rsp.MP_matrix[0][1] + v->ob[1] * rsp.MP_matrix[1][1] + v->ob[2] * rsp.MP_matrix[2][1] + rsp.MP_matrix[3][1];
        float z = v->ob[0] * rsp.MP_matrix[0][2] + v->ob[1] * rsp.MP_matrix[1][2] + v->ob[2] * rsp.MP_matrix[2][2] + rsp.MP_matrix[3][2];
        float w = v->ob[0] * rsp.MP_matrix[0][3] + v->ob[1] * rsp.MP_matrix[1][3] + v->ob[2] * rsp.MP_matrix[2][3] + rsp.MP_matrix[3][3];

        if (!isfinite(x) || !isfinite(y) || !isfinite(z) || !isfinite(w)) {
            x = y = z = 0.0f;
            w = -1.0f;
        }

        short U = v->tc[0] * rsp.texture_scaling_factor.s >> 16;
        short V = v->tc[1] * rsp.texture_scaling_factor.t >> 16;

        if (rsp.geometry_mode & G_LIGHTING) {
            if (rsp.lights_changed) {
                for (int i = 0; i < rsp.current_num_lights - 1; i++) {
                    calculate_normal_dir(&rsp.current_lights[i], rsp.current_lights_coeffs[i]);
                }
                static const Light_t lookat_x = {{0, 0, 0}, 0, {0, 0, 0}, 0, {127, 0, 0}, 0};
                static const Light_t lookat_y = {{0, 0, 0}, 0, {0, 0, 0}, 0, {0, 127, 0}, 0};
                calculate_normal_dir(&lookat_x, rsp.current_lookat_coeffs[0]);
                calculate_normal_dir(&lookat_y, rsp.current_lookat_coeffs[1]);
                rsp.lights_changed = false;
            }

            int r = rsp.current_lights[rsp.current_num_lights - 1].col[0];
            int g = rsp.current_lights[rsp.current_num_lights - 1].col[1];
            int b = rsp.current_lights[rsp.current_num_lights - 1].col[2];

            for (int i = 0; i < rsp.current_num_lights - 1; i++) {
                float intensity = 0;
                intensity += vn->n[0] * rsp.current_lights_coeffs[i][0];
                intensity += vn->n[1] * rsp.current_lights_coeffs[i][1];
                intensity += vn->n[2] * rsp.current_lights_coeffs[i][2];
                intensity /= 127.0f;
                if (intensity > 0.0f) {
                    r += intensity * rsp.current_lights[i].col[0];
                    g += intensity * rsp.current_lights[i].col[1];
                    b += intensity * rsp.current_lights[i].col[2];
                }
            }

            d->color.r = r > 255 ? 255 : r;
            d->color.g = g > 255 ? 255 : g;
            d->color.b = b > 255 ? 255 : b;

            if (rsp.geometry_mode & G_TEXTURE_GEN) {
                float dotx = 0, doty = 0;
                dotx += vn->n[0] * rsp.current_lookat_coeffs[0][0];
                dotx += vn->n[1] * rsp.current_lookat_coeffs[0][1];
                dotx += vn->n[2] * rsp.current_lookat_coeffs[0][2];
                doty += vn->n[0] * rsp.current_lookat_coeffs[1][0];
                doty += vn->n[1] * rsp.current_lookat_coeffs[1][1];
                doty += vn->n[2] * rsp.current_lookat_coeffs[1][2];

                U = (int32_t)((dotx / 127.0f + 1.0f) / 4.0f * rsp.texture_scaling_factor.s);
                V = (int32_t)((doty / 127.0f + 1.0f) / 4.0f * rsp.texture_scaling_factor.t);
            }
        } else {
            d->color.r = v->cn[0];
            d->color.g = v->cn[1];
            d->color.b = v->cn[2];
        }

        d->u = U;
        d->v = V;

        // trivial clip rejection
        d->clip_rej = 0;
        if (x < -w) d->clip_rej |= CLIP_LEFT;
        if (x >  w) d->clip_rej |= CLIP_RIGHT;
        if (y < -w) d->clip_rej |= CLIP_BOTTOM;
        if (y >  w) d->clip_rej |= CLIP_TOP;
        if (z < -w) d->clip_rej |= CLIP_FAR;
        if (z >  w) d->clip_rej |= CLIP_NEAR;

#if GFX_CHARACTER_MODEL_DIAG_ENABLED
        if (gfx_dbg_char_model_active &&
            gfx_dbg_char_model_log_count < GFX_CHARACTER_MODEL_LOG_LIMIT) {
            GFX_LOGI("gfx_char_vtx[%d]: type=%d sp=%u dst=%lu ob=%d,%d,%d xyzw=%d,%d,%d,%d mp3=%d,%d,%d,%d clip=%02x",
                    gfx_dbg_char_model_log_count,
                    gfx_dbg_char_model_shared_type,
                    rsp.modelview_matrix_stack_size,
                    (unsigned long)dest_index,
                    v->ob[0],
                    v->ob[1],
                    v->ob[2],
                    gfx_dbg_float_to_int(x),
                    gfx_dbg_float_to_int(y),
                    gfx_dbg_float_to_int(z),
                    gfx_dbg_float_to_int(w),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][0]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][1]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][2]),
                    gfx_dbg_float_to_int(rsp.MP_matrix[3][3]),
                    d->clip_rej);
            gfx_dbg_char_model_log_count++;
        }
#endif

        d->x = x;
        d->y = y;
        d->z = z;
        d->w = w;

        if (configEnableFog && (rsp.geometry_mode & G_FOG)) {
            w = (w == 0.f) ? 1.f / 0.001f : 1.f / w;
            const float winv = w < 0.0f ? 32767.0f : w;
            float fog_z = z * winv * rsp.fog_mul + rsp.fog_offset;
            if (fog_z < 0) fog_z = 0;
            if (fog_z > 255) fog_z = 255;
            d->color.a = fog_z; // Use alpha variable to store fog factor
        } else {
            d->color.a = v->cn[3];
        }
    }
}

static inline struct ColorCombiner *gfx_pick_combiner(bool *out_use_fog, bool *out_use_alpha) {
    uint32_t cc_id = rdp.combine_mode;

    bool use_alpha = (rdp.other_mode_l & (G_BL_A_MEM << 18)) == 0;
    const bool use_fog = configEnableFog && (rdp.other_mode_l >> 30) == G_BL_CLR_FOG;
    const bool texture_edge = (rdp.other_mode_l & CVG_X_ALPHA) == CVG_X_ALPHA;
    const bool use_noise = (rdp.other_mode_l & G_AC_DITHER) == G_AC_DITHER;

    if (texture_edge) {
        use_alpha = true;
    }

    if (use_alpha) cc_id |= SHADER_OPT_ALPHA;
    if (use_fog) cc_id |= SHADER_OPT_FOG;
    if (texture_edge) cc_id |= SHADER_OPT_TEXTURE_EDGE;
    if (use_noise) cc_id |= SHADER_OPT_NOISE;

    if (!use_alpha) {
        cc_id &= ~0xfff000;
    }

    struct ColorCombiner *comb = gfx_lookup_or_create_color_combiner(cc_id);
    struct ShaderProgram *prg = comb->prg;
    if (prg != rendering_state.shader_program) {
        gfx_flush();
        gfx_rapi->unload_shader(rendering_state.shader_program);
        gfx_rapi->load_shader(prg);
        rendering_state.shader_program = prg;
    }
    if (use_alpha != rendering_state.alpha_blend) {
        gfx_flush();
        gfx_rapi->set_use_alpha(use_alpha);
        rendering_state.alpha_blend = use_alpha;
    }

    if (out_use_fog) *out_use_fog = use_fog;
    if (out_use_alpha) *out_use_alpha = use_alpha;

    return comb;
}

static inline bool gfx_update_textures(const bool used_textures[2], const bool linear_filter) {
    for (int i = 0; i < 2; i++) {
        if (used_textures[i]) {
            if (rdp.textures_changed[i]) {
                gfx_flush();
                if (!import_texture(i)) {
                    if (rendering_state.textures[i] == NULL) {
                        return false;
                    }
#if GFX_RENDER_INFO_DIAG_ENABLED
                    if (gfx_dbg_texture_keep_log_count < 8) {
                        GFX_LOGI("gfx_tex_keep[%d]: tile=%d cached_id=%lu missing_addr=%p bytes=%lu line=%lu fmt=%u siz=%u",
                                gfx_dbg_texture_keep_log_count, i,
                                (unsigned long)rendering_state.textures[i]->texture_id,
                                rdp.loaded_texture[i].addr,
                                (unsigned long)rdp.loaded_texture[i].size_bytes,
                                (unsigned long)rdp.texture_tile.line_size_bytes,
                                rdp.texture_tile.fmt, rdp.texture_tile.siz);
                        gfx_dbg_texture_keep_log_count++;
                    }
#endif
                    rdp.textures_changed[i] = false;
                } else {
                    rdp.textures_changed[i] = false;
                }
            }
            if (rendering_state.textures[i] == NULL) {
                return false;
            }
            if (linear_filter != rendering_state.textures[i]->linear_filter || rdp.texture_tile.cms != rendering_state.textures[i]->cms || rdp.texture_tile.cmt != rendering_state.textures[i]->cmt) {
                gfx_flush();
                gfx_rapi->set_sampler_parameters(i, linear_filter, rdp.texture_tile.cms, rdp.texture_tile.cmt);
                rendering_state.textures[i]->linear_filter = linear_filter;
                rendering_state.textures[i]->cms = rdp.texture_tile.cms;
                rendering_state.textures[i]->cmt = rdp.texture_tile.cmt;
            }
        }
    }
    return true;
}

#ifdef ENABLE_SOFTRAST
static inline bool gfx_is_small_ia_text_triangle(const struct LoadedVertex *v1,
                                                 const struct LoadedVertex *v2,
                                                 const struct LoadedVertex *v3,
                                                 bool use_texture,
                                                 uint32_t tex_width,
                                                 uint32_t tex_height) {
    if (!gfx_overlay_output || !use_texture || rdp.texture_tile.fmt != G_IM_FMT_IA) {
        return false;
    }
    if ((rdp.other_mode_l & Z_UPD) != 0) {
        return false;
    }
    if (!((tex_width == 16 && tex_height == 8) ||
          (tex_width == 8 && tex_height == 16) ||
          (tex_width == 8 && tex_height == 8))) {
        return false;
    }
    if (v1->w <= 0.0f || v2->w <= 0.0f || v3->w <= 0.0f) {
        return false;
    }

    const float vp_x = ratio_x != 0.0f ? (float)rdp.viewport.x / ratio_x : 0.0f;
    const float vp_y = ratio_y != 0.0f ? (float)rdp.viewport.y / ratio_y : 0.0f;
    const float vp_hw = ratio_x != 0.0f ? (float)rdp.viewport.width / ratio_x * 0.5f : (float)HALF_SCREEN_WIDTH;
    const float vp_hh = ratio_y != 0.0f ? (float)rdp.viewport.height / ratio_y * 0.5f : (float)HALF_SCREEN_HEIGHT;
    const float x0 = (v1->x / v1->w) * vp_hw + vp_x + vp_hw;
    const float y0 = (v1->y / v1->w) * vp_hh + vp_y + vp_hh;
    const float x1 = (v2->x / v2->w) * vp_hw + vp_x + vp_hw;
    const float y1 = (v2->y / v2->w) * vp_hh + vp_y + vp_hh;
    const float x2 = (v3->x / v3->w) * vp_hw + vp_x + vp_hw;
    const float y2 = (v3->y / v3->w) * vp_hh + vp_y + vp_hh;
    const float min_x = fminf(x0, fminf(x1, x2));
    const float max_x = fmaxf(x0, fmaxf(x1, x2));
    const float min_y = fminf(y0, fminf(y1, y2));
    const float max_y = fmaxf(y0, fmaxf(y1, y2));

    return (max_x - min_x) <= 36.0f && (max_y - min_y) <= 36.0f;
}

static inline void gfx_overlay_submit_small_ia_text_triangle(const struct LoadedVertex *v1,
                                                            const struct LoadedVertex *v2,
                                                            const struct LoadedVertex *v3,
                                                            uint32_t tex_width,
                                                            uint32_t tex_height) {
    const float vp_x = ratio_x != 0.0f ? (float)rdp.viewport.x / ratio_x : 0.0f;
    const float vp_y = ratio_y != 0.0f ? (float)rdp.viewport.y / ratio_y : 0.0f;
    const float vp_hw = ratio_x != 0.0f ? (float)rdp.viewport.width / ratio_x * 0.5f : (float)HALF_SCREEN_WIDTH;
    const float vp_hh = ratio_y != 0.0f ? (float)rdp.viewport.height / ratio_y * 0.5f : (float)HALF_SCREEN_HEIGHT;
    const float x0 = (v1->x / v1->w) * vp_hw + vp_x + vp_hw;
    const float raw_y0 = (v1->y / v1->w) * vp_hh + vp_y + vp_hh;
    const float x1 = (v2->x / v2->w) * vp_hw + vp_x + vp_hw;
    const float raw_y1 = (v2->y / v2->w) * vp_hh + vp_y + vp_hh;
    const float x2 = (v3->x / v3->w) * vp_hw + vp_x + vp_hw;
    const float raw_y2 = (v3->y / v3->w) * vp_hh + vp_y + vp_hh;
    const float y0 = (float)(SCREEN_HEIGHT - 1) - raw_y0;
    const float y1 = (float)(SCREEN_HEIGHT - 1) - raw_y1;
    const float y2 = (float)(SCREEN_HEIGHT - 1) - raw_y2;
    const float u0 = ((float)v1->u - (float)rdp.texture_tile.uls * 8.0f) / 32.0f;
    const float vv0 = ((float)v1->v - (float)rdp.texture_tile.ult * 8.0f) / 32.0f;
    const float u1 = ((float)v2->u - (float)rdp.texture_tile.uls * 8.0f) / 32.0f;
    const float vv1 = ((float)v2->v - (float)rdp.texture_tile.ult * 8.0f) / 32.0f;
    const float u2 = ((float)v3->u - (float)rdp.texture_tile.uls * 8.0f) / 32.0f;
    const float vv2 = ((float)v3->v - (float)rdp.texture_tile.ult * 8.0f) / 32.0f;

    gfx_soft_overlay_textured_tri(x0, y0, u0, vv0,
                                  x1, y1, u1, vv1,
                                  x2, y2, u2, vv2,
                                  &rdp.env_color.r);
}
#endif

static inline void gfx_push_triangle(const struct LoadedVertex *restrict v1, const struct LoadedVertex *restrict v2, const struct LoadedVertex *restrict v3) {
    const struct LoadedVertex *v_arr[3] = {v1, v2, v3};

    const bool depth_test = (rsp.geometry_mode & G_ZBUFFER) == G_ZBUFFER;
    if (depth_test != rendering_state.depth_test) {
        gfx_flush();
        gfx_rapi->set_depth_test(depth_test);
        rendering_state.depth_test = depth_test;
    }

    const bool z_upd = (rdp.other_mode_l & Z_UPD) == Z_UPD;
    if (z_upd != rendering_state.depth_mask) {
        gfx_flush();
        gfx_rapi->set_depth_mask(z_upd);
        rendering_state.depth_mask = z_upd;
    }

    const bool zmode_decal = (rdp.other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    if (zmode_decal != rendering_state.decal_mode) {
        gfx_flush();
        gfx_rapi->set_zmode_decal(zmode_decal);
        rendering_state.decal_mode = zmode_decal;
    }

    if (rdp.viewport_or_scissor_changed) {
        if (memcmp(&rdp.viewport, &rendering_state.viewport, sizeof(rdp.viewport)) != 0) {
            gfx_flush();
            gfx_rapi->set_viewport(rdp.viewport.x, rdp.viewport.y, rdp.viewport.width, rdp.viewport.height);
            rendering_state.viewport = rdp.viewport;
        }
        if (memcmp(&rdp.scissor, &rendering_state.scissor, sizeof(rdp.scissor)) != 0) {
            gfx_flush();
            gfx_rapi->set_scissor(rdp.scissor.x, rdp.scissor.y, rdp.scissor.width, rdp.scissor.height);
            rendering_state.scissor = rdp.scissor;
        }
        rdp.viewport_or_scissor_changed = false;
    }

    uint8_t num_inputs;
    bool used_textures[2], use_fog, use_alpha;

    struct ColorCombiner *comb = gfx_pick_combiner(&use_fog, &use_alpha);
    gfx_rapi->shader_get_info(rendering_state.shader_program, &num_inputs, used_textures);

    const bool linear_filter = configFiltering && (rdp.other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT;
    const bool use_texture = used_textures[0] || used_textures[1];
    if (GFX_RENDER_DIAG && use_texture && gfx_dbg_texture_state_log_count < 16) {
        GFX_LOGI("gfx_tex_state[%d]: cc=%08lx geom=%08lx om=%08lx/%08lx used=%d/%d changed=%d/%d loaded0=%p/%lu loaded1=%p/%lu line=%lu tile=(%u,%u)-(%u,%u) cache0=%p",
                gfx_dbg_texture_state_log_count,
                (unsigned long)comb->cc_id,
                (unsigned long)rsp.geometry_mode,
                (unsigned long)rdp.other_mode_h,
                (unsigned long)rdp.other_mode_l,
                used_textures[0], used_textures[1],
                rdp.textures_changed[0], rdp.textures_changed[1],
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                rdp.loaded_texture[1].addr,
                (unsigned long)rdp.loaded_texture[1].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt,
                rendering_state.textures[0]);
        gfx_dbg_texture_state_log_count++;
    }
    if (!gfx_update_textures(used_textures, linear_filter)) {
        return;
    }
#if GFX_TITLE_MODEL_DIAG_ENABLED
    if (gfx_dbg_title_model_watched() && use_texture && gfx_dbg_title_model_push < 4 && gfx_dbg_title_model_log_count < 12) {
        GFX_LOGI("gfx_title_model_push[%d]: tex=%d cc=%08lx used=%d/%d addr=%p/%lu geom=%08lx uv=%ld,%ld/%ld,%ld/%ld,%ld clip=%02x/%02x/%02x w=%.2f/%.2f/%.2f",
                gfx_dbg_title_model_log_count,
                gfx_dbg_title_model_tex_index,
                (unsigned long)comb->cc_id,
                used_textures[0], used_textures[1],
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rsp.geometry_mode,
                (long)v1->u, (long)v1->v,
                (long)v2->u, (long)v2->v,
                (long)v3->u, (long)v3->v,
                v1->clip_rej, v2->clip_rej, v3->clip_rej,
                (double)v1->w, (double)v2->w, (double)v3->w);
        gfx_dbg_title_model_log_count++;
    }
    if (gfx_dbg_title_model_watched() && use_texture) {
        gfx_dbg_title_model_push++;
    }
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
    gfx_dbg_menu_model_note_begin(comb, used_textures, use_alpha, v1, v2, v3);
#endif
    const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
    const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;

#ifdef ENABLE_SOFTRAST
    if (gfx_is_small_ia_text_triangle(v1, v2, v3, use_texture, tex_width, tex_height)) {
        gfx_overlay_submit_small_ia_text_triangle(v1, v2, v3, tex_width, tex_height);
    }
#endif

    if (GFX_LOG_TEXTURE_TRIANGLES && use_texture && gfx_dbg_textured_tri_log_count < 16) {
        GFX_LOGI("gfx_tex_tri[%d]: cc=%08lx geom=%08lx inputs=%u used=%d/%d tex=%lux%lu uv=%ld,%ld/%ld,%ld/%ld,%ld rgba=%u,%u,%u clip=%02x/%02x/%02x w=%.2f/%.2f/%.2f",
                gfx_dbg_textured_tri_log_count,
                (unsigned long)comb->cc_id,
                (unsigned long)rsp.geometry_mode,
                num_inputs, used_textures[0], used_textures[1],
                (unsigned long)tex_width, (unsigned long)tex_height,
                (long)v1->u, (long)v1->v, (long)v2->u, (long)v2->v, (long)v3->u, (long)v3->v,
                v1->color.r, v1->color.g, v1->color.b,
                v1->clip_rej, v2->clip_rej, v3->clip_rej,
                (double)v1->w, (double)v2->w, (double)v3->w);
        gfx_dbg_textured_tri_log_count++;
    }

#ifndef GFX_W_PREMULT
    const bool z_is_from_0_to_1 = gfx_rapi->z_is_from_0_to_1();
#endif

    for (int i = 0; i < 3; i++) {
#ifdef GFX_W_PREMULT
        const float w = v_arr[i]->w;
        const float w_inv = 1.f / w;
        buf_vbo[buf_vbo_len++] = v_arr[i]->x * w_inv;
        buf_vbo[buf_vbo_len++] = v_arr[i]->y * w_inv;
        buf_vbo[buf_vbo_len++] = (v_arr[i]->z + w) * 0.5f * w_inv;
        buf_vbo[buf_vbo_len++] = w_inv; // store inverted W right away to save softrast the trouble
#else
        float z = v_arr[i]->z, w = v_arr[i]->w;
        if (z_is_from_0_to_1) {
            z = (z + w) / 2.0f;
        }
        buf_vbo[buf_vbo_len++] = v_arr[i]->x;
        buf_vbo[buf_vbo_len++] = v_arr[i]->y;
        buf_vbo[buf_vbo_len++] = z;
        buf_vbo[buf_vbo_len++] = w;
#endif

        if (use_texture) {
            float u = (v_arr[i]->u - rdp.texture_tile.uls * 8) / 32.0f;
            float v = (v_arr[i]->v - rdp.texture_tile.ult * 8) / 32.0f;
            if ((rdp.other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT) {
                // Linear filter adds 0.5f to the coordinates
                u += 0.5f;
                v += 0.5f;
            }
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(u / tex_width);
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(v / tex_height);
        }

        if (use_fog) {
#ifndef GFX_NO_FOG_COLOR
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(rdp.fog_color.r));
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(rdp.fog_color.g));
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(rdp.fog_color.b));
#endif
            buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(v_arr[i]->color.a)); // fog factor (not alpha)
        }

        for (int j = 0; j < num_inputs; j++) {
            const struct RGBA *color;
            struct RGBA tmp;
            for (int k = 0; k < 1 + (use_alpha ? 1 : 0); k++) {
                switch (comb->shader_input_mapping[k][j]) {
                    case CC_PRIM:
                        color = &rdp.prim_color;
                        break;
                    case CC_SHADE:
                        color = &v_arr[i]->color;
                        break;
                    case CC_ENV:
                        color = &rdp.env_color;
                        break;
                    case CC_LOD:
                    {
                        float distance_frac = (v1->w - 3000.0f) / 3000.0f;
                        if (distance_frac < 0.0f) distance_frac = 0.0f;
                        if (distance_frac > 1.0f) distance_frac = 1.0f;
                        tmp.r = tmp.g = tmp.b = tmp.a = distance_frac * 255.0f;
                        color = &tmp;
                        break;
                    }
                    default:
                        memset(&tmp, 0, sizeof(tmp));
                        color = &tmp;
                        break;
                }
                if (k == 0) {
                    buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(color->r));
                    buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(color->g));
                    buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(color->b));
                } else {
                    if (use_fog && color == &v_arr[i]->color) {
                        // Shade alpha is 100% for fog
                        buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_ONE);
                    } else {
                        buf_vbo[buf_vbo_len++] = GFX_OUT_PROP(GFX_COLOR_CONVERT(color->a));
                    }
                }
            }
        }
        /*struct RGBA *color = &v_arr[i]->color;
        buf_vbo[buf_vbo_len++] = color->r / 255.0f;
        buf_vbo[buf_vbo_len++] = color->g / 255.0f;
        buf_vbo[buf_vbo_len++] = color->b / 255.0f;
        buf_vbo[buf_vbo_len++] = color->a / 255.0f;*/
    }
    if (++buf_vbo_num_tris == MAX_BUFFERED) {
        gfx_flush();
    }
}

#ifdef GFX_MANUAL_CLIPPING
static inline float flerp(const float v0, const float v1, const float t) {
    return v0 + t * (v1 - v0);
}

static inline struct RGBA rgba_lerp(const struct RGBA c0, const struct RGBA c1, const float t) {
    return (struct RGBA){
        c0.r + (c1.r - c0.r) * t,
        c0.g + (c1.g - c0.g) * t,
        c0.b + (c1.b - c0.b) * t,
        c0.a + (c1.a - c0.a) * t,
    };
}

static inline bool gfx_clip_triangle(struct LoadedVertex *v1, struct LoadedVertex *v2, struct LoadedVertex *v3, const uint8_t clip_and) {
    static const float c_planes[][4] = {
        {  0.0f,  0.0f, -1.0f,  1.0f }, // near
        {  0.0f,  0.0f,  1.0f,  1.0f }, // far
        {  0.0f, -1.0f,  0.0f,  1.0f }, // top
        {  0.0f,  1.0f,  0.0f,  1.0f }, // bottom
        { -1.0f,  0.0f,  0.0f,  1.0f }, // left
        {  1.0f,  0.0f,  0.0f,  1.0f }, // right
    };

    const uint8_t clip_or = v1->clip_rej | v2->clip_rej | v3->clip_rej;

    if (!clip_or) return false; // triangle fully in frustum

    struct LoadedVertex v_buf[2][12] = { { *v1, *v2, *v3 } };
    int v_num[2] = { 3, 0 };
    int v_idx = 0;

    uint8_t plane_idx = 0;
    for (uint8_t clip_mask = 1; clip_mask < 64; clip_mask <<= 1, ++plane_idx) {
        if (!(clip_or & clip_mask)) continue;

        const int num_verts = v_num[v_idx];
        const int outidx = !v_idx;
        const struct LoadedVertex *v_in = v_buf[v_idx];
        struct LoadedVertex *v_out = v_buf[outidx];
        const float *plane = c_planes[plane_idx];

        for (int i = 0; i < num_verts; ++i) {
            const struct LoadedVertex *vthis = &v_in[i];
            const struct LoadedVertex *vnext = &v_in[(i + 1) % num_verts];
            const float d1 = plane[0] * vthis->x + plane[1] * vthis->y + plane[2] * vthis->z + vthis->w;
            const float d2 = plane[0] * vnext->x + plane[1] * vnext->y + plane[2] * vnext->z + vnext->w;
            const bool this_in = d1 > 0.0f;
            const bool next_in = d2 > 0.0f;
            // current is inside clipping plane, push it into output
            if (this_in) v_out[v_num[outidx]++] = *vthis;
            // one of the vertices is outside, clip the edge and push intersection
            if (this_in ^ next_in) {
                struct LoadedVertex *xv = &v_out[v_num[outidx]++];
                if (this_in) {
                    const float t = d1 / (d1 - d2);
                    xv->x = flerp(vthis->x, vnext->x, t);
                    xv->y = flerp(vthis->y, vnext->y, t);
                    xv->z = flerp(vthis->z, vnext->z, t);
                    xv->w = flerp(vthis->w, vnext->w, t);
                    xv->u = flerp(vthis->u, vnext->u, t);
                    xv->v = flerp(vthis->v, vnext->v, t);
                    xv->color = rgba_lerp(vthis->color, vnext->color, t);
                    xv->clip_rej = 0;
                } else {
                    const float t = d2 / (d2 - d1);
                    xv->x = flerp(vnext->x, vthis->x, t);
                    xv->y = flerp(vnext->y, vthis->y, t);
                    xv->z = flerp(vnext->z, vthis->z, t);
                    xv->w = flerp(vnext->w, vthis->w, t);
                    xv->u = flerp(vnext->u, vthis->u, t);
                    xv->v = flerp(vnext->v, vthis->v, t);
                    xv->color = rgba_lerp(vnext->color, vthis->color, t);
                }
            }
        }

        if (v_num[outidx] < 3) return true; // not enough for a triangle

        v_idx = outidx;
        v_num[!v_idx] = 0;
    }

    // make a triangle fan
    const int n = v_num[v_idx] - 1;
    const struct LoadedVertex *in = v_buf[v_idx];
    for (int i = 1; i < n; ++i)
        gfx_push_triangle(in + 0, in + i, in + i + 1);

    return true;
}
#endif

static void gfx_sp_tri1(uint8_t vtx1_idx, uint8_t vtx2_idx, uint8_t vtx3_idx) {
    struct LoadedVertex *v1 = &rsp.loaded_vertices[vtx1_idx];
    struct LoadedVertex *v2 = &rsp.loaded_vertices[vtx2_idx];
    struct LoadedVertex *v3 = &rsp.loaded_vertices[vtx3_idx];

    //if (rand()%2) return;

#if GFX_MENU_MODEL_DIAG_ENABLED
    const bool menu_preclip_target = gfx_dbg_menu_preclip_is_target();
#else
    const bool menu_preclip_target = false;
#endif
    const uint8_t clip_and = v1->clip_rej & v2->clip_rej & v3->clip_rej;
    if (clip_and) {
#if GFX_TITLE_MODEL_DIAG_ENABLED
        if (gfx_dbg_title_model_watched()) {
            gfx_dbg_title_model_clip_drop++;
            if (gfx_dbg_title_model_clip_drop <= 4 && gfx_dbg_title_model_log_count < 12) {
                GFX_LOGI("gfx_title_model_clip[%d]: tex=%d tri=%u/%u/%u and=%02x rej=%02x/%02x/%02x xyz=(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f) w=%.2f/%.2f/%.2f",
                        gfx_dbg_title_model_log_count,
                        gfx_dbg_title_model_tex_index,
                        vtx1_idx, vtx2_idx, vtx3_idx,
                        clip_and,
                        v1->clip_rej, v2->clip_rej, v3->clip_rej,
                        (double)v1->x, (double)v1->y, (double)v1->z,
                        (double)v2->x, (double)v2->y, (double)v2->z,
                        (double)v3->x, (double)v3->y, (double)v3->z,
                        (double)v1->w, (double)v2->w, (double)v3->w);
                gfx_dbg_title_model_log_count++;
            }
        }
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
        if (gfx_dbg_menu_model_watched()) {
            gfx_dbg_menu_model_clip_drop++;
        }
        if (menu_preclip_target) {
            gfx_dbg_menu_preclip_log("clip_and", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, 0.0f, 0);
        }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
        gfx_dbg_char_tri_log("clip_and", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, 0.0f, 0);
#endif
        // The whole triangle lies outside the visible area
        return;
    }

    float menu_cull_cross = 0.0f;
    const bool skip_menu_cull = menu_preclip_target &&
                                (rdp.loaded_texture[0].size_bytes == 2048 ||
                                 rdp.loaded_texture[0].size_bytes == 4096);
    if ((rsp.geometry_mode & G_CULL_BOTH) != 0 && !skip_menu_cull) {
        float dx1 = v1->x / (v1->w) - v2->x / (v2->w);
        float dy1 = v1->y / (v1->w) - v2->y / (v2->w);
        float dx2 = v3->x / (v3->w) - v2->x / (v2->w);
        float dy2 = v3->y / (v3->w) - v2->y / (v2->w);
        float cross = dx1 * dy2 - dy1 * dx2;

        if ((v1->w < 0) ^ (v2->w < 0) ^ (v3->w < 0)) {
            // If one vertex lies behind the eye, negating cross will give the correct result.
            // If all vertices lie behind the eye, the triangle will be rejected anyway.
            cross = -cross;
        }
        menu_cull_cross = cross;

        switch (rsp.geometry_mode & G_CULL_BOTH) {
            case G_CULL_FRONT:
                if (cross <= 0) {
#if GFX_TITLE_MODEL_DIAG_ENABLED
                    if (gfx_dbg_title_model_watched()) gfx_dbg_title_model_cull_drop++;
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
                    if (gfx_dbg_menu_model_watched()) gfx_dbg_menu_model_cull_drop++;
                    if (menu_preclip_target) {
                        gfx_dbg_menu_preclip_log("cull_front", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
                    }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
                    gfx_dbg_char_tri_log("cull_front", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
#endif
                    return;
                }
                break;
            case G_CULL_BACK:
                if (cross >= 0) {
#if GFX_TITLE_MODEL_DIAG_ENABLED
                    if (gfx_dbg_title_model_watched()) gfx_dbg_title_model_cull_drop++;
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
                    if (gfx_dbg_menu_model_watched()) gfx_dbg_menu_model_cull_drop++;
                    if (menu_preclip_target) {
                        gfx_dbg_menu_preclip_log("cull_back", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
                    }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
                    gfx_dbg_char_tri_log("cull_back", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
#endif
                    return;
                }
                break;
            case G_CULL_BOTH:
                // Why is this even an option?
#if GFX_TITLE_MODEL_DIAG_ENABLED
                if (gfx_dbg_title_model_watched()) gfx_dbg_title_model_cull_drop++;
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
                if (gfx_dbg_menu_model_watched()) gfx_dbg_menu_model_cull_drop++;
                if (menu_preclip_target) {
                    gfx_dbg_menu_preclip_log("cull_both", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
                }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
                gfx_dbg_char_tri_log("cull_both", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, cross, 0);
#endif
                return;
        }
    }

#if GFX_TITLE_MODEL_DIAG_ENABLED
    if (gfx_dbg_title_model_watched()) {
        gfx_dbg_title_model_submit++;
    }
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
    if (gfx_dbg_menu_model_watched()) {
        gfx_dbg_menu_model_submit++;
    }
#endif

#ifdef GFX_MANUAL_CLIPPING
    // clip the triangle and put the resulting triangles into the buffer
    // otherwise put the current triangle
    const size_t menu_tris_before_clip = buf_vbo_num_tris;
    const bool clipped = gfx_clip_triangle(v1, v2, v3, clip_and);
#if GFX_MENU_MODEL_DIAG_ENABLED
    if (menu_preclip_target && clipped) {
        const size_t emitted = buf_vbo_num_tris >= menu_tris_before_clip
                             ? buf_vbo_num_tris - menu_tris_before_clip
                             : 0;
        gfx_dbg_menu_preclip_log(emitted > 0 ? "clip_emit" : "clip_out",
                                 vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3,
                                 menu_cull_cross, emitted);
    }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
    if (gfx_dbg_char_model_active && clipped) {
        const size_t emitted = buf_vbo_num_tris >= menu_tris_before_clip
                             ? buf_vbo_num_tris - menu_tris_before_clip
                             : 0;
        gfx_dbg_char_tri_log(emitted > 0 ? "clip_emit" : "clip_out",
                             vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3,
                             menu_cull_cross, emitted);
    }
#endif
    if (!clipped)
#endif
    {
#if GFX_MENU_MODEL_DIAG_ENABLED
        if (menu_preclip_target) {
            gfx_dbg_menu_preclip_log("direct", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, menu_cull_cross, 1);
        }
#endif
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
        gfx_dbg_char_tri_log("direct", vtx1_idx, vtx2_idx, vtx3_idx, v1, v2, v3, menu_cull_cross, 1);
#endif
        gfx_push_triangle(v1, v2, v3);
    }
}

static void gfx_sp_geometry_mode(uint32_t clear, uint32_t set) {
    rsp.geometry_mode &= ~clear;
    rsp.geometry_mode |= set;
}

static void gfx_calc_and_set_viewport(const Vp_t *viewport) {
    // 2 bits fraction
    float width = 2.0f * viewport->vscale[0] / 4.0f;
    float height = 2.0f * viewport->vscale[1] / 4.0f;
    float x = (viewport->vtrans[0] / 4.0f) - width / 2.0f;
    float y = SCREEN_HEIGHT - ((viewport->vtrans[1] / 4.0f) + height / 2.0f);

    width *= ratio_x;
    height *= ratio_y;
    x *= ratio_x;
    y *= ratio_y;

    rdp.viewport.x = x;
    rdp.viewport.y = y;
    rdp.viewport.width = width;
    rdp.viewport.height = height;

    rdp.viewport_or_scissor_changed = true;
}

static void gfx_sp_movemem(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case G_MV_VIEWPORT:
            gfx_calc_and_set_viewport((const Vp_t *) data);
            break;
#if 0
        case G_MV_LOOKATY:
        case G_MV_LOOKATX:
            memcpy(rsp.current_lookat + (index - G_MV_LOOKATY) / 2, data, sizeof(Light_t));
            //rsp.lights_changed = 1;
            break;
#endif
#ifdef F3DEX_GBI_2
        case G_MV_LIGHT: {
            int lightidx = offset / 24 - 2;
            if (lightidx >= 0 && lightidx <= MAX_LIGHTS) { // skip lookat
                // NOTE: reads out of bounds if it is an ambient light
                memcpy(rsp.current_lights + lightidx, data, sizeof(Light_t));
            }
            break;
        }
#else
        case G_MV_L0:
        case G_MV_L1:
        case G_MV_L2:
            // NOTE: reads out of bounds if it is an ambient light
            memcpy(rsp.current_lights + (index - G_MV_L0) / 2, data, sizeof(Light_t));
            break;
#endif
    }
}

static void gfx_sp_moveword(uint8_t index, uint16_t offset, uint32_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
#ifdef F3DEX_GBI_2
            rsp.current_num_lights = data / 24 + 1; // add ambient light
#else
            // Ambient light is included
            // The 31th bit is a flag that lights should be recalculated
            rsp.current_num_lights = (data - 0x80000000U) / 32;
#endif
            if (rsp.current_num_lights == 0 || rsp.current_num_lights > MAX_LIGHTS + 1) {
                if (gfx_dbg_light_guard_log_count < 8) {
                    RG_LOGE("gfx_numlights_oob[%d]: data=%08lx num=%u max=%d",
                            gfx_dbg_light_guard_log_count,
                            (unsigned long)data, rsp.current_num_lights,
                            MAX_LIGHTS + 1);
                    gfx_dbg_light_guard_log_count++;
                }
                rsp.current_num_lights = MAX_LIGHTS + 1;
            }
            rsp.lights_changed = 1;
            break;
        case G_MW_FOG:
            rsp.fog_mul = (int16_t)(data >> 16);
            rsp.fog_offset = (int16_t)data;
            break;
    }
}

static void gfx_sp_texture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on) {
    rsp.texture_scaling_factor.s = sc;
    rsp.texture_scaling_factor.t = tc;
}

static void gfx_dp_set_scissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry) {
    float x = ulx / 4.0f * ratio_x;
    float y = (SCREEN_HEIGHT - lry / 4.0f) * ratio_y;
    float width = (lrx - ulx) / 4.0f * ratio_x;
    float height = (lry - uly) / 4.0f * ratio_y;

    rdp.scissor.x = x;
    rdp.scissor.y = y;
    rdp.scissor.width = width;
    rdp.scissor.height = height;

    rdp.viewport_or_scissor_changed = true;
}

static void gfx_dp_set_texture_image(uint32_t format, uint32_t size, uint32_t width, const void* addr) {
    rdp.texture_to_load.addr = addr;
    rdp.texture_to_load.siz = size;
    if (addr == NULL && gfx_dbg_null_texture_image_log_count < 8) {
        RG_LOGE("gfx_tex_null_settimg[%d]: fmt=%lu siz=%lu width=%lu prev_addr=%p prev_bytes=%lu line=%lu tile=(%u,%u)-(%u,%u)",
                gfx_dbg_null_texture_image_log_count,
                (unsigned long)format, (unsigned long)size, (unsigned long)width,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_null_texture_image_log_count++;
    }
}

static void gfx_rdp_commit_loaded_texture(uint8_t dst_tile, uint32_t size_bytes, const char *op) {
    if (dst_tile >= 2) {
        dst_tile = 0;
    }

    if (rdp.texture_to_load.addr == NULL || size_bytes == 0) {
        if (gfx_dbg_null_texture_load_log_count < 8) {
            RG_LOGE("gfx_tex_null_load[%d]: op=%s dst=%u load_tile=%u fmt=%u siz=%u bytes=%lu line=%lu keep0=%p/%lu tile=(%u,%u)-(%u,%u)",
                    gfx_dbg_null_texture_load_log_count, op, dst_tile,
                    rdp.texture_to_load.tile_number, rdp.texture_tile.fmt,
                    rdp.texture_to_load.siz, (unsigned long)size_bytes,
                    (unsigned long)rdp.texture_tile.line_size_bytes,
                    rdp.loaded_texture[0].addr,
                    (unsigned long)rdp.loaded_texture[0].size_bytes,
                    rdp.texture_tile.uls, rdp.texture_tile.ult,
                    rdp.texture_tile.lrs, rdp.texture_tile.lrt);
            gfx_dbg_null_texture_load_log_count++;
        }
        rdp.loaded_texture[0].addr = NULL;
        rdp.loaded_texture[0].size_bytes = 0;
        rdp.loaded_texture[1].addr = NULL;
        rdp.loaded_texture[1].size_bytes = 0;
        rdp.textures_changed[0] = true;
        rdp.textures_changed[1] = true;
        rendering_state.textures[0] = NULL;
        rendering_state.textures[1] = NULL;
        return;
    }

    rdp.loaded_texture[dst_tile].size_bytes = size_bytes;
    rdp.loaded_texture[dst_tile].addr = rdp.texture_to_load.addr;
    rdp.textures_changed[dst_tile] = true;

#if GFX_TITLE_MODEL_DIAG_ENABLED
    gfx_dbg_title_model_note_load(rdp.texture_to_load.addr, size_bytes);
#endif

#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_texture_load_log_count < 24) {
        GFX_LOGI("gfx_tex_load[%d]: op=%s dst=%u load_tile=%u addr=%p bytes=%lu fmt=%u siz=%u line=%lu tile=(%u,%u)-(%u,%u)",
                gfx_dbg_texture_load_log_count, op, dst_tile,
                rdp.texture_to_load.tile_number,
                rdp.texture_to_load.addr,
                (unsigned long)size_bytes,
                rdp.texture_tile.fmt,
                rdp.texture_to_load.siz,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_texture_load_log_count++;
    }
#endif

    if (dst_tile != 0) {
        rdp.loaded_texture[0] = rdp.loaded_texture[dst_tile];
        rdp.textures_changed[0] = true;
    }

}

static void gfx_dp_set_tile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette, uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks, uint32_t shifts) {
#if GFX_TITLE_MODEL_DIAG_ENABLED
    if (tile == G_TX_LOADTILE || (tile == G_TX_RENDERTILE && (line * 8) != 64)) {
        gfx_dbg_title_model_note_stop("set_tile");
    }
#endif

#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_tile_log_count < 32) {
        GFX_LOGI("gfx_set_tile[%d]: tile=%u fmt=%u siz=%lu line=%lu tmem=%lu load_tile=%u render=%d palette=%lu cms/cmt=%lu/%lu mask=%lu/%lu shift=%lu/%lu",
                gfx_dbg_tile_log_count, tile, fmt, (unsigned long)siz,
                (unsigned long)line, (unsigned long)tmem,
                rdp.texture_to_load.tile_number, tile == G_TX_RENDERTILE,
                (unsigned long)palette, (unsigned long)cms, (unsigned long)cmt,
                (unsigned long)masks, (unsigned long)maskt,
                (unsigned long)shifts, (unsigned long)shiftt);
        gfx_dbg_tile_log_count++;
    }
#endif

    if (tile == G_TX_RENDERTILE) {
        SUPPORT_CHECK(palette == 0); // palette should set upper 4 bits of color index in 4b mode
        rdp.texture_tile.fmt = fmt;
        rdp.texture_tile.siz = siz;
        rdp.texture_tile.cms = cms;
        rdp.texture_tile.cmt = cmt;
        rdp.texture_tile.line_size_bytes = line * 8;
        rdp.textures_changed[0] = true;
        rdp.textures_changed[1] = true;
    }

    if (tile == G_TX_LOADTILE) {
        rdp.texture_to_load.tile_number = (tmem / 256) & 1;
    }
}

static void gfx_dp_set_tile_size(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt) {
#if GFX_TITLE_MODEL_DIAG_ENABLED
    if (tile == G_TX_RENDERTILE &&
        (uls != 0 || ult != 0 ||
         lrs != ((32 - 1) << G_TEXTURE_IMAGE_FRAC) ||
         lrt != ((32 - 1) << G_TEXTURE_IMAGE_FRAC))) {
        gfx_dbg_title_model_note_stop("tile_size");
    }
#endif

#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_tile_size_log_count < 32) {
        GFX_LOGI("gfx_set_tile_size[%d]: tile=%u uls/ult=%u/%u lrs/lrt=%u/%u render=%d tex0=%p/%lu line=%lu",
                gfx_dbg_tile_size_log_count, tile, uls, ult, lrs, lrt,
                tile == G_TX_RENDERTILE, rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes);
        gfx_dbg_tile_size_log_count++;
    }
#endif

    if (tile == G_TX_RENDERTILE) {
        rdp.texture_tile.uls = uls;
        rdp.texture_tile.ult = ult;
        rdp.texture_tile.lrs = lrs;
        rdp.texture_tile.lrt = lrt;
        rdp.textures_changed[0] = true;
        rdp.textures_changed[1] = true;
    }
}

static void gfx_dp_load_tlut(uint8_t tile, uint32_t high_index) {
    SUPPORT_CHECK(tile == G_TX_LOADTILE);
    SUPPORT_CHECK(rdp.texture_to_load.siz == G_IM_SIZ_16b);
    rdp.palette = rdp.texture_to_load.addr;
}

static void gfx_dp_load_block(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt) {
    if (tile == 1) return;
    SUPPORT_CHECK(tile == G_TX_LOADTILE);
    SUPPORT_CHECK(uls == 0);
    SUPPORT_CHECK(ult == 0);

    // The lrs field rather seems to be number of pixels to load
    uint32_t word_size_shift;
    switch (rdp.texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = 0; // Or -1? It's unused in SM64 anyway.
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }
    uint32_t size_bytes = (lrs + 1) << word_size_shift;
    // assert(size_bytes <= 4096 && "bug: too big texture"); // ESP32-P4 can handle larger textures in PSRAM
#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_tile_log_count < 32) {
        GFX_LOGI("gfx_load_block_pre[%d]: tile=%u load_tile=%u addr=%p siz=%u lrs=%lu bytes=%lu dxt=%lu render_line=%lu render_tile=(%u,%u)-(%u,%u)",
                gfx_dbg_tile_log_count, tile, rdp.texture_to_load.tile_number,
                rdp.texture_to_load.addr, rdp.texture_to_load.siz,
                (unsigned long)lrs, (unsigned long)size_bytes, (unsigned long)dxt,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_tile_log_count++;
    }
#endif
    gfx_rdp_commit_loaded_texture(rdp.texture_to_load.tile_number, size_bytes, "load_block");
}

static void gfx_dp_load_tile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt) {
    if (tile == 1) return;
    SUPPORT_CHECK(tile == G_TX_LOADTILE);
    SUPPORT_CHECK(uls == 0);
    SUPPORT_CHECK(ult == 0);

    uint32_t word_size_shift;
    switch (rdp.texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }

    uint32_t size_bytes = (((lrs >> G_TEXTURE_IMAGE_FRAC) + 1) * ((lrt >> G_TEXTURE_IMAGE_FRAC) + 1)) << word_size_shift;
    // assert(size_bytes <= 4096 && "bug: too big texture"); // ESP32-P4 handles larger textures in PSRAM
    rdp.texture_tile.uls = uls;
    rdp.texture_tile.ult = ult;
    rdp.texture_tile.lrs = lrs;
    rdp.texture_tile.lrt = lrt;

    gfx_rdp_commit_loaded_texture(rdp.texture_to_load.tile_number, size_bytes, "load_tile");
}


static uint8_t color_comb_component(uint32_t v) {
    switch (v) {
        case G_CCMUX_TEXEL0:
            return CC_TEXEL0;
        case G_CCMUX_TEXEL1:
            return CC_TEXEL1;
        case G_CCMUX_PRIMITIVE:
            return CC_PRIM;
        case G_CCMUX_SHADE:
            return CC_SHADE;
        case G_CCMUX_ENVIRONMENT:
            return CC_ENV;
        case G_CCMUX_TEXEL0_ALPHA:
            return CC_TEXEL0A;
        case G_CCMUX_LOD_FRACTION:
            return CC_LOD;
        default:
            return CC_0;
    }
}

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return color_comb_component(a) |
           (color_comb_component(b) << 3) |
           (color_comb_component(c) << 6) |
           (color_comb_component(d) << 9);
}

static void gfx_dp_set_combine_mode(uint32_t rgb, uint32_t alpha) {
    rdp.combine_mode = rgb | (alpha << 12);
}

static void gfx_dp_set_env_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    rdp.env_color.r = r;
    rdp.env_color.g = g;
    rdp.env_color.b = b;
    rdp.env_color.a = a;
}

static void gfx_dp_set_prim_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    rdp.prim_color.r = r;
    rdp.prim_color.g = g;
    rdp.prim_color.b = b;
    rdp.prim_color.a = a;
}

static void gfx_dp_set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    rdp.fog_color.r = r;
    rdp.fog_color.g = g;
    rdp.fog_color.b = b;
    rdp.fog_color.a = a;
    if (gfx_rapi->set_fog_color) gfx_rapi->set_fog_color(&rdp.fog_color.r);
}

static void gfx_dp_set_fill_color(uint32_t packed_color) {
    uint16_t col16 = (uint16_t)packed_color;
    uint32_t r = col16 >> 11;
    uint32_t g = (col16 >> 6) & 0x1f;
    uint32_t b = (col16 >> 1) & 0x1f;
    uint32_t a = col16 & 1;
    rdp.fill_color.r = SCALE_5_8(r);
    rdp.fill_color.g = SCALE_5_8(g);
    rdp.fill_color.b = SCALE_5_8(b);
    rdp.fill_color.a = a * 255;
}

static void gfx_draw_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    uint32_t saved_other_mode_h = rdp.other_mode_h;
    uint32_t cycle_type = (rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    if (cycle_type == G_CYC_COPY) {
        rdp.other_mode_h = (rdp.other_mode_h & ~(3U << G_MDSFT_TEXTFILT)) | G_TF_POINT;
    }

    // U10.2 coordinates
    float ulxf = ulx;
    float ulyf = uly;
    float lrxf = lrx;
    float lryf = lry;

    ulxf = ulxf / (4.0f * HALF_SCREEN_WIDTH) - 1.0f;
    ulyf = -(ulyf / (4.0f * HALF_SCREEN_HEIGHT)) + 1.0f;
    lrxf = lrxf / (4.0f * HALF_SCREEN_WIDTH) - 1.0f;
    lryf = -(lryf / (4.0f * HALF_SCREEN_HEIGHT)) + 1.0f;

    struct LoadedVertex* ul = &rsp.loaded_vertices[MAX_VERTICES + 0];
    struct LoadedVertex* ll = &rsp.loaded_vertices[MAX_VERTICES + 1];
    struct LoadedVertex* lr = &rsp.loaded_vertices[MAX_VERTICES + 2];
    struct LoadedVertex* ur = &rsp.loaded_vertices[MAX_VERTICES + 3];

    ul->x = ulxf;
    ul->y = ulyf;
    ul->z = -1.0f;
    ul->w = 1.0f;

    ll->x = ulxf;
    ll->y = lryf;
    ll->z = -1.0f;
    ll->w = 1.0f;

    lr->x = lrxf;
    lr->y = lryf;
    lr->z = -1.0f;
    lr->w = 1.0f;

    ur->x = lrxf;
    ur->y = ulyf;
    ur->z = -1.0f;
    ur->w = 1.0f;

    // The coordinates for texture rectangle shall bypass the viewport setting
    struct XYWidthHeight default_viewport = {0, 0, gfx_current_dimensions.width, gfx_current_dimensions.height};
    struct XYWidthHeight viewport_saved = rdp.viewport;
    uint32_t geometry_mode_saved = rsp.geometry_mode;

    rdp.viewport = default_viewport;
    rdp.viewport_or_scissor_changed = true;
    rsp.geometry_mode = 0;

    gfx_sp_tri1(MAX_VERTICES + 0, MAX_VERTICES + 1, MAX_VERTICES + 3);
    gfx_sp_tri1(MAX_VERTICES + 1, MAX_VERTICES + 2, MAX_VERTICES + 3);

    rsp.geometry_mode = geometry_mode_saved;
    rdp.viewport = viewport_saved;
    rdp.viewport_or_scissor_changed = true;

    if (cycle_type == G_CYC_COPY) {
        rdp.other_mode_h = saved_other_mode_h;
    }
}

static void gfx_dp_texture_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls, int16_t ult, int16_t dsdx, int16_t dtdy, bool flip) {
#if GFX_RENDER_INFO_DIAG_ENABLED
    if (gfx_dbg_tex_rect_call_log_count < 24) {
        GFX_LOGI("gfx_tex_rect_call[%d]: tile=%u rect=%ld,%ld-%ld,%ld uv=%ld,%ld step=%ld,%ld flip=%d tex0=%p/%lu line=%lu tile_size=(%u,%u)-(%u,%u)",
                gfx_dbg_tex_rect_call_log_count, tile,
                (long)ulx, (long)uly, (long)lrx, (long)lry,
                (long)uls, (long)ult, (long)dsdx, (long)dtdy, flip,
                rdp.loaded_texture[0].addr,
                (unsigned long)rdp.loaded_texture[0].size_bytes,
                (unsigned long)rdp.texture_tile.line_size_bytes,
                rdp.texture_tile.uls, rdp.texture_tile.ult,
                rdp.texture_tile.lrs, rdp.texture_tile.lrt);
        gfx_dbg_tex_rect_call_log_count++;
    }
#endif

    uint32_t saved_combine_mode = rdp.combine_mode;
    if ((rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_COPY) {
        // Per RDP Command Summary Set Tile's shift s and this dsdx should be set to 4 texels
        // Divide by 4 to get 1 instead
        dsdx >>= 2;

        // Color combiner is turned off in copy mode
        gfx_dp_set_combine_mode(color_comb(0, 0, 0, G_CCMUX_TEXEL0), color_comb(0, 0, 0, G_ACMUX_TEXEL0));

        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    // uls and ult are S10.5
    // dsdx and dtdy are S5.10
    // lrx, lry, ulx, uly are U10.2
    // lrs, lrt are S10.5
    if (flip) {
        dsdx = -dsdx;
        dtdy = -dtdy;
    }
    int16_t width = !flip ? lrx - ulx : lry - uly;
    int16_t height = !flip ? lry - uly : lrx - ulx;
    float lrs = ((uls << 7) + dsdx * width) >> 7;
    float lrt = ((ult << 7) + dtdy * height) >> 7;

    if (gfx_rapi->tex_rect) {
        const float logical_x0 = ulx / 4.0f;
        const float logical_y0 = uly / 4.0f;
        const float logical_x1 = lrx / 4.0f;
        const float logical_y1 = lry / 4.0f;
        const float logical_w = logical_x1 - logical_x0;
        const float logical_h = logical_y1 - logical_y0;
        float ulxf = ulx * ratio_x;
        float ulyf = uly * ratio_y;
        float lrxf = lrx * ratio_x;
        float lryf = lry * ratio_y;
        const float dudx = ((lrs - (float)uls) / (lrxf - ulxf));
        const float dvdy = ((lrt - (float)ult) / (lryf - ulyf));
        const bool used_textures[2] = { true, false };
        gfx_pick_combiner(NULL, NULL);
        if (!gfx_update_textures(used_textures, false)) {
            rdp.combine_mode = saved_combine_mode;
            return;
        }
        gfx_flush();
        ulxf = HALF_SCREEN_WIDTH + (ulxf / 4.0f - HALF_SCREEN_WIDTH);
        lrxf = HALF_SCREEN_WIDTH + (lrxf / 4.0f - HALF_SCREEN_WIDTH);
        ulyf = ulyf / 4.0f;
        lryf = lryf / 4.0f;
        if (GFX_LOG_TEXTURE_RECTS && gfx_dbg_tex_rect_log_count < 12) {
            GFX_LOGI("gfx_tex_rect[%d]: shader=%p src=%p wh=%lux%lu rect=%ld,%ld-%ld,%ld uv=%ld,%ld rgba=%u,%u,%u,%u",
                    gfx_dbg_tex_rect_log_count,
                    rendering_state.shader_program,
                    rdp.loaded_texture[0].addr,
                    (unsigned long)((rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4),
                    (unsigned long)((rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4),
                    (long)ulxf, (long)ulyf, (long)lrxf, (long)lryf,
                    (long)(uls / 32), (long)(ult / 32),
                    rdp.env_color.r, rdp.env_color.g, rdp.env_color.b, rdp.env_color.a);
            gfx_dbg_tex_rect_log_count++;
        }
        gfx_rapi->tex_rect(ulxf, ulyf, lrxf, lryf, uls / 32.f, ult / 32.f, dudx / 8.f, dvdy / 8.f, &rdp.env_color.r);
#ifdef ENABLE_SOFTRAST
        const uint32_t tex_width = (rdp.texture_tile.lrs - rdp.texture_tile.uls + 4) / 4;
        const uint32_t tex_height = (rdp.texture_tile.lrt - rdp.texture_tile.ult + 4) / 4;
        if (gfx_overlay_output &&
            logical_w > 0.0f && logical_h > 0.0f &&
            logical_w <= 32.0f && logical_h <= 32.0f &&
            tex_width <= 32 && tex_height <= 32) {
            const float overlay_dudx = ((lrs - (float)uls) / logical_w) / 32.0f;
            const float overlay_dvdy = ((lrt - (float)ult) / logical_h) / 32.0f;
            gfx_soft_overlay_tex_rect((int)logical_x0, (int)logical_y0,
                                      (int)logical_x1, (int)logical_y1,
                                      uls / 32.f, ult / 32.f,
                                      overlay_dudx, overlay_dvdy,
                                      &rdp.env_color.r);
        }
#endif
    } else {
        struct LoadedVertex* ul = &rsp.loaded_vertices[MAX_VERTICES + 0];
        struct LoadedVertex* ll = &rsp.loaded_vertices[MAX_VERTICES + 1];
        struct LoadedVertex* lr = &rsp.loaded_vertices[MAX_VERTICES + 2];
        struct LoadedVertex* ur = &rsp.loaded_vertices[MAX_VERTICES + 3];
        ul->u = uls;
        ul->v = ult;
        lr->u = lrs;
        lr->v = lrt;
        if (!flip) {
            ll->u = uls;
            ll->v = lrt;
            ur->u = lrs;
            ur->v = ult;
        } else {
            ll->u = lrs;
            ll->v = ult;
            ur->u = uls;
            ur->v = lrt;
        }
        gfx_draw_rectangle(ulx, uly, lrx, lry);
    }

    rdp.combine_mode = saved_combine_mode;
}

static void gfx_dp_fill_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    if (rdp.color_image_address == rdp.z_buf_address) {
        // Don't clear Z buffer here since we already did it with glClear
        return;
    }
    uint32_t mode = (rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    if (mode == G_CYC_COPY || mode == G_CYC_FILL) {
        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    const uint32_t saved_combine_mode = rdp.combine_mode;
    gfx_dp_set_combine_mode(color_comb(0, 0, 0, G_CCMUX_SHADE), color_comb(0, 0, 0, G_ACMUX_SHADE));

    if (gfx_rapi->fill_rect) {
        float ulxf = ulx * ratio_x;
        float ulyf = uly * ratio_y;
        float lrxf = lrx * ratio_x;
        float lryf = lry * ratio_y;
        gfx_pick_combiner(NULL, NULL);
        gfx_flush();
        ulxf = HALF_SCREEN_WIDTH + (ulxf / 4.0f - HALF_SCREEN_WIDTH);
        lrxf = HALF_SCREEN_WIDTH + (lrxf / 4.0f - HALF_SCREEN_WIDTH);
        ulyf = ulyf / 4.0f;
        lryf = lryf / 4.0f;
        gfx_rapi->fill_rect(ulxf, ulyf, lrxf, lryf, &rdp.fill_color.r);
    } else {
        for (int i = MAX_VERTICES; i < MAX_VERTICES + 4; i++) {
            struct LoadedVertex* v = &rsp.loaded_vertices[i];
            v->color = rdp.fill_color;
        }
        gfx_draw_rectangle(ulx, uly, lrx, lry);
    }

    rdp.combine_mode = saved_combine_mode;
}

static void gfx_dp_set_z_image(void *z_buf_address) {
    rdp.z_buf_address = z_buf_address;
}

static void gfx_dp_set_color_image(uint32_t format, uint32_t size, uint32_t width, void* address) {
    rdp.color_image_address = address;
}

static void gfx_sp_set_other_mode(uint32_t shift, uint32_t num_bits, uint64_t mode) {
    uint64_t mask = (((uint64_t)1 << num_bits) - 1) << shift;
    uint64_t om = rdp.other_mode_l | ((uint64_t)rdp.other_mode_h << 32);
    om = (om & ~mask) | mode;
    rdp.other_mode_l = (uint32_t)om;
    rdp.other_mode_h = (uint32_t)(om >> 32);
}

static inline void *seg_addr(uintptr_t w1) {
    return (void *) w1;
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))

static void gfx_run_dl(Gfx* cmd) {
    if (cmd == NULL) {
        if (gfx_dbg_dl_guard_log_count < 8) {
            RG_LOGE("gfx_dl_null[%d]", gfx_dbg_dl_guard_log_count);
            gfx_dbg_dl_guard_log_count++;
        }
        return;
    }

    uint32_t command_count = 0;
    for (;;) {
        if (++command_count > 20000) {
            if (gfx_dbg_dl_guard_log_count < 8) {
                RG_LOGE("gfx_dl_limit[%d]: cmd=%p op=%02lx",
                        gfx_dbg_dl_guard_log_count, (void *)cmd,
                        (unsigned long)(cmd->words.w0 >> 24));
                gfx_dbg_dl_guard_log_count++;
            }
            return;
        }

        uint32_t opcode = cmd->words.w0 >> 24;

        switch (opcode) {
            // RSP commands:
            case G_MTX:
#ifdef F3DEX_GBI_2
                gfx_sp_matrix(C0(0, 8) ^ G_MTX_PUSH, (const int32_t *) seg_addr(cmd->words.w1));
#else
                gfx_sp_matrix(C0(16, 8), (const int32_t *) seg_addr(cmd->words.w1));
#endif
                break;
            case (uint8_t)G_POPMTX:
#ifdef F3DEX_GBI_2
                gfx_sp_pop_matrix(cmd->words.w1 / 64);
#else
                gfx_sp_pop_matrix(1);
#endif
                break;
            case G_MOVEMEM:
#ifdef F3DEX_GBI_2
                gfx_sp_movemem(C0(0, 8), C0(8, 8) * 8, seg_addr(cmd->words.w1));
#else
                gfx_sp_movemem(C0(16, 8), 0, seg_addr(cmd->words.w1));
#endif
                break;
            case (uint8_t)G_MOVEWORD:
#ifdef F3DEX_GBI_2
                gfx_sp_moveword(C0(16, 8), C0(0, 16), cmd->words.w1);
#else
                gfx_sp_moveword(C0(0, 8), C0(8, 16), cmd->words.w1);
#endif
                break;
            case (uint8_t)G_TEXTURE:
#ifdef F3DEX_GBI_2
                gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(1, 7));
#else
                gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(0, 8));
#endif
                break;
            case G_VTX:
#ifdef F3DEX_GBI_2
                gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), seg_addr(cmd->words.w1));
#elif defined(F3DEX_GBI) || defined(F3DLP_GBI)
                gfx_sp_vertex(C0(10, 6), C0(16, 8) / 2, seg_addr(cmd->words.w1));
#else
                gfx_sp_vertex((C0(0, 16)) / sizeof(Vtx), C0(16, 4), seg_addr(cmd->words.w1));
#endif
                break;
            case G_DL:
                if (C0(16, 1) == 0) {
                    // Push return address
                    gfx_run_dl((Gfx *)seg_addr(cmd->words.w1));
                } else {
                    cmd = (Gfx *)seg_addr(cmd->words.w1);
                    --cmd; // increase after break
                }
                break;
            case (uint8_t)G_ENDDL:
                return;
#ifdef F3DEX_GBI_2
            case G_GEOMETRYMODE:
                gfx_sp_geometry_mode(~C0(0, 24), cmd->words.w1);
                break;
#else
            case (uint8_t)G_SETGEOMETRYMODE:
                gfx_sp_geometry_mode(0, cmd->words.w1);
                break;
            case (uint8_t)G_CLEARGEOMETRYMODE:
                gfx_sp_geometry_mode(cmd->words.w1, 0);
                break;
#endif
            case (uint8_t)G_TRI1:
#ifdef F3DEX_GBI_2
                gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2);
#elif defined(F3DEX_GBI) || defined(F3DLP_GBI)
                gfx_sp_tri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2);
#else
                gfx_sp_tri1(C1(16, 8) / 10, C1(8, 8) / 10, C1(0, 8) / 10);
#endif
                break;
#if defined(F3DEX_GBI) || defined(F3DLP_GBI)
            case (uint8_t)G_TRI2:
                gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2);
                gfx_sp_tri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2);
                break;
#endif
            case (uint8_t)G_SETOTHERMODE_L:
#ifdef F3DEX_GBI_2
                gfx_sp_set_other_mode(31 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, cmd->words.w1);
#else
                gfx_sp_set_other_mode(C0(8, 8), C0(0, 8), cmd->words.w1);
#endif
                break;
            case (uint8_t)G_SETOTHERMODE_H:
#ifdef F3DEX_GBI_2
                gfx_sp_set_other_mode(63 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, (uint64_t) cmd->words.w1 << 32);
#else
                gfx_sp_set_other_mode(C0(8, 8) + 32, C0(0, 8), (uint64_t) cmd->words.w1 << 32);
#endif
                break;

            // RDP Commands:
            case G_SETTIMG:
                gfx_dp_set_texture_image(C0(21, 3), C0(19, 2), C0(0, 10), seg_addr(cmd->words.w1));
                break;
            case G_LOADBLOCK:
                gfx_dp_load_block(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
                break;
            case G_LOADTILE:
                gfx_dp_load_tile(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
                break;
            case G_SETTILE:
                gfx_dp_set_tile(C0(21, 3), C0(19, 2), C0(9, 9), C0(0, 9), C1(24, 3), C1(20, 4), C1(18, 2), C1(14, 4), C1(10, 4), C1(8, 2), C1(4, 4), C1(0, 4));
                break;
            case G_SETTILESIZE:
                gfx_dp_set_tile_size(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
                break;
            case G_LOADTLUT:
                gfx_dp_load_tlut(C1(24, 3), C1(14, 10));
                break;
            case G_SETENVCOLOR:
                gfx_dp_set_env_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
                break;
            case G_SETPRIMCOLOR:
                gfx_dp_set_prim_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
                break;
            case G_SETFOGCOLOR:
                gfx_dp_set_fog_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
                break;
            case G_SETFILLCOLOR:
                gfx_dp_set_fill_color(cmd->words.w1);
                break;
            case G_SETCOMBINE:
                gfx_dp_set_combine_mode(
                    color_comb(C0(20, 4), C1(28, 4), C0(15, 5), C1(15, 3)),
                    color_comb(C0(12, 3), C1(12, 3), C0(9, 3), C1(9, 3)));
                    /*color_comb(C0(5, 4), C1(24, 4), C0(0, 5), C1(6, 3)),
                    color_comb(C1(21, 3), C1(3, 3), C1(18, 3), C1(0, 3)));*/
                break;
            // G_SETPRIMCOLOR, G_CCMUX_PRIMITIVE, G_ACMUX_PRIMITIVE, is used by Goddard
            // G_CCMUX_TEXEL1, LOD_FRACTION is used in Bowser room 1
            case G_TEXRECT:
            case G_TEXRECTFLIP:
            {
                int32_t lrx, lry, tile, ulx, uly;
                uint32_t uls, ult, dsdx, dtdy;
#ifdef F3DEX_GBI_2E
                lrx = (int32_t)(C0(0, 24) << 8) >> 8;
                lry = (int32_t)(C1(0, 24) << 8) >> 8;
                ++cmd;
                ulx = (int32_t)(C0(0, 24) << 8) >> 8;
                uly = (int32_t)(C1(0, 24) << 8) >> 8;
                ++cmd;
                uls = C0(16, 16);
                ult = C0(0, 16);
                dsdx = C1(16, 16);
                dtdy = C1(0, 16);
#else
                lrx = C0(12, 12);
                lry = C0(0, 12);
                tile = C1(24, 3);
                ulx = C1(12, 12);
                uly = C1(0, 12);
                ++cmd;
                uls = C1(16, 16);
                ult = C1(0, 16);
                ++cmd;
                dsdx = C1(16, 16);
                dtdy = C1(0, 16);
#endif
                gfx_dp_texture_rectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == G_TEXRECTFLIP);
                break;
            }
            case G_FILLRECT:
#ifdef F3DEX_GBI_2E
            {
                int32_t lrx, lry, ulx, uly;
                lrx = (int32_t)(C0(0, 24) << 8) >> 8;
                lry = (int32_t)(C1(0, 24) << 8) >> 8;
                ++cmd;
                ulx = (int32_t)(C0(0, 24) << 8) >> 8;
                uly = (int32_t)(C1(0, 24) << 8) >> 8;
                gfx_dp_fill_rectangle(ulx, uly, lrx, lry);
                break;
            }
#else
                gfx_dp_fill_rectangle(C1(12, 12), C1(0, 12), C0(12, 12), C0(0, 12));
                break;
#endif
            case G_SETSCISSOR:
                gfx_dp_set_scissor(C1(24, 2), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
                break;
            case G_SETZIMG:
                gfx_dp_set_z_image(seg_addr(cmd->words.w1));
                break;
            case G_SETCIMG:
                gfx_dp_set_color_image(C0(21, 3), C0(19, 2), C0(0, 11), seg_addr(cmd->words.w1));
                break;
        }
        ++cmd;
    }
}

static void gfx_sp_reset() {
    rsp.modelview_matrix_stack_size = 1;
    gfx_matrix_identity(rsp.modelview_matrix_stack[0]);
    gfx_matrix_identity(rsp.P_matrix);
    gfx_matrix_identity(rsp.MP_matrix);
    rsp.current_num_lights = 2;
    rsp.lights_changed = true;
}

void gfx_get_dimensions(uint32_t *width, uint32_t *height) {
    gfx_wapi->get_dimensions(width, height);
}

void gfx_init(struct GfxWindowManagerAPI *wapi, struct GfxRenderingAPI *rapi, const char *game_name, bool start_in_fullscreen) {
    gfx_wapi = wapi;
    gfx_rapi = rapi;
    gfx_wapi->init(game_name, start_in_fullscreen);
    gfx_wapi->get_dimensions(&gfx_current_dimensions.width, &gfx_current_dimensions.height);
    gfx_rapi->init();

    // Used in the 120 star TAS
    static uint32_t precomp_shaders[] = {
        0x01200200,
        0x00000045,
        0x00000200,
        0x01200a00,
        0x00000a00,
        0x01a00045,
        0x00000551,
        0x01045045,
        0x05a00a00,
        0x01200045,
        0x05045045,
        0x01045a00,
        0x01a00a00,
        0x0000038d,
        0x01081081,
        0x0120038d,
        0x03200045,
        0x03200a00,
        0x01a00a6f,
        0x01141045,
        0x07a00a00,
        0x05200200,
        0x03200200,
        0x09200200,
        0x0920038d,
        0x09200045
    };
    for (size_t i = 0; i < sizeof(precomp_shaders) / sizeof(uint32_t); i++) {
        gfx_lookup_or_create_shader_program(precomp_shaders[i]);
    }
}

void gfx_shutdown(void) {
    if (gfx_rapi && gfx_rapi->shutdown) gfx_rapi->shutdown();
    if (gfx_wapi && gfx_wapi->shutdown) gfx_wapi->shutdown();
    gfx_rapi = NULL;
    gfx_wapi = NULL;
}

struct GfxRenderingAPI *gfx_get_current_rendering_api(void) {
    return gfx_rapi;
}

void gfx_start_frame(void) {
    gfx_wapi->handle_events();
    gfx_wapi->get_dimensions(&gfx_current_dimensions.width, &gfx_current_dimensions.height);
    if (gfx_current_dimensions.height == 0) {
        // Avoid division by zero
        gfx_current_dimensions.height = 1;
    }
    ratio_x = (float)gfx_current_dimensions.width / (float)SCREEN_WIDTH;
    ratio_y = (float)gfx_current_dimensions.height / (float)SCREEN_HEIGHT;
}

void gfx_run(Gfx *commands) {
    gfx_sp_reset();
#if GFX_MENU_MODEL_DIAG_ENABLED
    if (gfx_dbg_menu_model_arm_frames > 0) {
        gfx_dbg_menu_model_active = 0;
        gfx_dbg_menu_model_submit = 0;
        gfx_dbg_menu_model_push = 0;
        gfx_dbg_menu_model_clip_drop = 0;
        gfx_dbg_menu_model_cull_drop = 0;
        gfx_dbg_menu_model_flushes = 0;
    }
#endif

    //puts("New frame");

    if (!gfx_wapi->start_frame()) {
        dropped_frame = true;
        return;
    }
    dropped_frame = false;

    gfx_rapi->start_frame();
    gfx_run_dl(commands);
    gfx_flush();
#if GFX_CHARACTER_MODEL_DIAG_ENABLED
    gfx_dbg_char_model_active = 0;
    gfx_dbg_char_model_target_count = 0;
#endif
#if GFX_MENU_MODEL_DIAG_ENABLED
    if (gfx_dbg_menu_model_arm_frames > 0) {
        gfx_dbg_menu_model_note_stop("frame_end");
        gfx_dbg_menu_model_arm_frames--;
    }
#endif
    gfx_rapi->end_frame();
    gfx_wapi->swap_buffers_begin();
}

void gfx_end_frame(void) {
    if (!dropped_frame) {
        gfx_rapi->finish_render();
        gfx_wapi->swap_buffers_end();
    }
}
