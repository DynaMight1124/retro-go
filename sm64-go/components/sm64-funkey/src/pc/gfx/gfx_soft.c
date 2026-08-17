#ifdef ENABLE_SOFTRAST

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>
#include <math.h>
#include <assert.h>

#ifndef _LANGUAGE_C
# define _LANGUAGE_C
#endif
#include <PR/gbi.h>

#include "gfx_pc.h"
#include "config.h"
#include "gfx_soft.h"
#include "gfx_cc.h"
#include "macros.h"

#define ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))

#define MAX_TEXTURES 8192
#define TEXCACHE_STEP 0x10000

enum WrapType {
    WRAP_REPEAT = 0,
    WRAP_CLAMP  = 1,
    WRAP_MIRROR = 2,
};

enum DrawFlags {
    DRAW_ZWRITE = 1,
    DRAW_BLEND = 2,
    DRAW_BLEND_EDGE = 4,
};

enum MixType {
    SH_MT_NONE            = 0,
    SH_MT_COLOR           = 1 << 0,
    SH_MT_COLOR_COLOR     = 1 << 1,
    SH_MT_TEXTURE         = 1 << 2,
    SH_MT_TEXTURE_COLOR   = 1 << 3,
    SH_MT_TEXTURE_TEXTURE = 1 << 4,
};

typedef union Vector2 { 
    struct { float x, y; };
    struct { float u, v; };
} Vector2;

typedef union Vector3 {
    struct { float x, y, z; };
    Vector2 xy;
    float v[3];
} Vector3;

typedef union Vector4 {
    struct { float x, y, z, w; };
    Vector2 xy;
    Vector3 xyz;
    float v[4];
} Vector4;

typedef union Color4 {
    struct { uint8_t r, g, b, a; };
    uint32_t c;
} Color4;

struct Tri {
    float *v0;
    float *v1;
    float *v2;
};

struct Texture;

// texture sampling function: takes integer u,v and wraps/clamps it, samples texture, returns color
typedef Color4 (*sample_fn_t)(const struct Texture * const, const int, const int);
// pixel drawing function: does blending, zwriting, alpha edge checking or whatever else, then plots pixel
typedef void (*draw_fn_t)(const int idx, uint16_t uz, const Color4 src);
// color combiner: takes float vertex properties and obtains final fragment color from them
#define COMBINE_ARGS const float z, const float *props, \
                     UNUSED const struct Texture *tex0, UNUSED const struct Texture *tex1
typedef Color4 (*combine_fn_t)(COMBINE_ARGS);
// rasterizer: walks the triangle and interpolates a fixed amount of vertex properties
typedef void (*rast_fn_t)(const struct Tri tri);

struct ShaderProgram {
    uint32_t shader_id;
    struct CCFeatures cc;
    enum MixType mix;
    uint32_t draw_flags;
    int num_props;
    combine_fn_t combine;
    rast_fn_t rast;
};

struct Texture {
    int w, h;           // size
    float fw, fh;       // float size because float conversion bad
    int wrap_w, wrap_h; // size - 1 for wrapping
    bool filter;        // linear filter
    uint32_t addr;      // offset into texcache
    sample_fn_t sample; // sampling function (does wrapping/clamping)
};

struct Viewport {
    int x, y, w, h; // rect
    float cx, cy;   // center
    float hw, hh;   // half size
    // float zn, zf, cz, hz; // FIXME: ztrick
};

struct ClipRect {
    int x0, y0; // top left
    int x1, y1; // bottom right
};

uint32_t *gfx_output;
uint32_t *gfx_overlay_output;
bool gfx_overlay_active;
int gfx_overlay_min_x;
int gfx_overlay_min_y;
int gfx_overlay_max_x;
int gfx_overlay_max_y;
int16_t gfx_overlay_row_min_x[SCREEN_HEIGHT];
int16_t gfx_overlay_row_max_x[SCREEN_HEIGHT];

// this is set in the drawing functions
static draw_fn_t draw_fn;

static struct ShaderProgram shader_program_pool[64];
static uint8_t shader_program_pool_size;
static int gfx_soft_dbg_shader_pool_log_count;
static struct ShaderProgram *cur_shader = NULL;

static struct Texture *cur_tex[2]; // currently selected textures for both tiles
static struct Texture *tex_hdr = NULL;
static uint32_t tex_num = 0; // amount of textures in cache
static int cur_tmu = 0; // select tile (used only for uploading)

// texture cache: linearly stores RGBA data of every cached texture
static uint8_t *texcache;
static uint32_t texcache_addr; // current offset into cache
static uint32_t texcache_size; // cache capacity

static bool do_blend; // fragment blending toggle
static bool do_clip;  // scissor toggle

static struct ClipRect r_clip;
static struct Viewport r_view;

static Color4 fog_color; // this is set by set_fog_color() calls from gfx_pc

static bool z_test;        // whether to perform depth testing
static bool z_write;       // whether to write into the Z buffer
static float z_offset;     // offset for decal mode
static uint16_t *z_buffer;

static bool gfx_soft_drawing_triangles;
static int gfx_soft_dbg_tri_pixels;
static int gfx_soft_dbg_rect_pixels;
static int gfx_soft_dbg_fill_pixels;
static int gfx_soft_dbg_oob_log_count;

#define GFX_SOFT_TITLE_SHADER_TEX   0x00000A00
#define GFX_SOFT_FORCE_TITLE_SOLID 0
#define GFX_SOFT_PIXEL_COUNTERS 0
#define GFX_SOFT_LOGO_DIAG 0
#define GFX_SOFT_MENU_DIAG 0
#define GFX_SOFT_MENU_SHADER_MODULATE 0x01045045
#define GFX_SOFT_MENU_SHADER_MODULATE_EDGE 0x05045045
#define GFX_SOFT_MENU_SHADER_DECAL    0x01045A00

static bool gfx_soft_dbg_logo_active;
static int gfx_soft_dbg_logo_summary_count;
static int gfx_soft_dbg_logo_tris;
static int gfx_soft_dbg_logo_pixels;
static int gfx_soft_dbg_logo_drawn;
static int gfx_soft_dbg_logo_zfail;
static int gfx_soft_dbg_logo_oob;
static int gfx_soft_dbg_logo_alpha0;
static int gfx_soft_dbg_logo_nonblack;
static int gfx_soft_dbg_logo_min_x;
static int gfx_soft_dbg_logo_max_x;
static int gfx_soft_dbg_logo_min_y;
static int gfx_soft_dbg_logo_max_y;
static bool gfx_soft_dbg_menu_active;
static int gfx_soft_dbg_menu_summary_count;
static int gfx_soft_dbg_menu_tris;
static int gfx_soft_dbg_menu_pixels;
static int gfx_soft_dbg_menu_drawn;
static int gfx_soft_dbg_menu_zfail;
static int gfx_soft_dbg_menu_oob;
static int gfx_soft_dbg_menu_alpha0;
static int gfx_soft_dbg_menu_nonblack;
static int gfx_soft_dbg_menu_min_x;
static int gfx_soft_dbg_menu_max_x;
static int gfx_soft_dbg_menu_min_y;
static int gfx_soft_dbg_menu_max_y;

#define scr_width SM64_RENDER_WIDTH
#define scr_height SM64_RENDER_HEIGHT
#define scr_size (SM64_RENDER_WIDTH * SM64_RENDER_HEIGHT)
#define overlay_width SCREEN_WIDTH
#define overlay_height SCREEN_HEIGHT
#define overlay_size (SCREEN_WIDTH * SCREEN_HEIGHT)

/* math shit */

static inline uint16_t u16clamp(const int v) {
    return (v < 0) ? (uint16_t)0 : (v > 0xFFFF) ? (uint16_t)0xFFFF : (uint16_t)v;
}

static inline int iwrap0w(const int x, const int wrap) {
    return x & wrap;
}

static inline int iclamp0w(const int x, const int wrap) {
    return (x < 0) ? 0 : (x > wrap) ? wrap : x;
}

static inline int imirror0w(const int x, const int wrap) {
    return iclamp0w(abs(x), wrap); // NOTE: this is not a universal solution
}

static inline uint8_t rgba_mul_component(const uint8_t c1, const uint8_t c2) {
    return (uint8_t)(((uint16_t)c1 * c2) >> 8);
}

static inline uint8_t rgba_blend_component(const uint8_t src, const uint8_t dst,
                                           const uint8_t a, const uint8_t ia) {
    return (uint8_t)(rgba_mul_component(src, a) + rgba_mul_component(dst, ia));
}

static inline uint8_t rgba_lerp_component(const uint8_t c1, const uint8_t c2,
                                          const uint8_t t) {
    const int delta = (int)c2 - (int)c1;
    return (uint8_t)(c1 + (uint8_t)(((int)t * delta) >> 8));
}

static inline Color4 rgba_modulate(const Color4 c1, const Color4 c2) {
    return (Color4) {{
        .r = rgba_mul_component(c1.r, c2.r),
        .g = rgba_mul_component(c1.g, c2.g),
        .b = rgba_mul_component(c1.b, c2.b),
        .a = rgba_mul_component(c1.a, c2.a),
    }};
}

static inline Color4 rgba_blend(const Color4 src, const Color4 dst, const uint8_t a) {
    const uint8_t ia = 0xFF - a;
    return (Color4) {{
        .r = rgba_blend_component(src.r, dst.r, a, ia),
        .g = rgba_blend_component(src.g, dst.g, a, ia),
        .b = rgba_blend_component(src.b, dst.b, a, ia),
        .a = dst.a,
    }};
}

static inline Color4 rgba_lerp(const Color4 c1, const Color4 c2, const uint8_t t) {
    return (Color4) {{
        .r = rgba_lerp_component(c1.r, c2.r, t),
        .g = rgba_lerp_component(c1.g, c2.g, t),
        .b = rgba_lerp_component(c1.b, c2.b, t),
        .a = rgba_lerp_component(c1.a, c2.a, t),
    }};
}

static inline int imin(const int a, const int b) {
    return (a < b) ? a : b;
}

static inline int imax(const int a, const int b) {
    return (a > b) ? a : b;
}

static inline void viewport_transform(Vector4 *v) {
    // gfx_pc.c with ENABLE_SOFTRAST defined will feed us with everything already pre-multiplied by inverse of w
    v->x = v->x * r_view.hw + r_view.cx + 0.5f;
    v->y = v->y * r_view.hh + r_view.cy + 0.5f;
    // v->w is also already 1.f / v->w
}

/* texture sampling functions */

static inline Color4 tex_get(const struct Texture * const tex, const int x, const int y) {
#if CONFIG_IDF_TARGET_ESP32S3
    return (Color4) { .c = ((const uint32_t *)tex->addr)[y * tex->w + x] };
#else
    return (Color4) { .c = ((const uint32_t *)(texcache + tex->addr))[y * tex->w + x] };
#endif
}

static Color4 tex_sample_nearest_rr(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iwrap0w(x, tex->wrap_w), iwrap0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_rc(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iwrap0w(x, tex->wrap_w), iclamp0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_rm(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iwrap0w(x, tex->wrap_w), imirror0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_cc(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iclamp0w(x, tex->wrap_w), iclamp0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_cr(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iclamp0w(x, tex->wrap_w), iwrap0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_cm(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, iclamp0w(x, tex->wrap_w), imirror0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_mm(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, imirror0w(x, tex->wrap_w), imirror0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_mc(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, imirror0w(x, tex->wrap_w), iclamp0w(y, tex->wrap_h));
}

static Color4 tex_sample_nearest_mr(const struct Texture * const tex, const int x, const int y) {
    return tex_get(tex, imirror0w(x, tex->wrap_w), iwrap0w(y, tex->wrap_h));
}

static inline Color4 tex_sample_nearest(const struct Texture * const tex, const float u, const float v) {
    const int x = u * tex->fw;
    const int y = v * tex->fh;
    return tex->sample(tex, x, y);
}

/* color combiners */

#define tex_sample tex_sample_nearest

static Color4 combine_rgb(COMBINE_ARGS) {
    return (Color4) {{ .r = props[0] * z, .g = props[1] * z, .b = props[2] * z, .a = 0xFF }};
}

static Color4 combine_rgba(COMBINE_ARGS) {
    return (Color4) {{ .r = props[0] * z, .g = props[1] * z, .b = props[2] * z, .a = props[3] * z }};
}

static Color4 combine_fog_rgb(COMBINE_ARGS) {
    const uint8_t fog = props[0] * z;
    const Color4 c = (Color4) {{ .r = props[1] * z, .g = props[2] * z, .b = props[3] * z, .a = 0xFF }};
    return rgba_blend(fog_color, c, fog);
}

static Color4 combine_fog_rgba(COMBINE_ARGS) {
    const uint8_t fog = props[0] * z;
    const Color4 c = (Color4) {{ .r = props[1] * z, .g = props[2] * z, .b = props[3] * z, .a = props[4] * z }};
    return rgba_blend(fog_color, c, fog);
}

static Color4 combine_rgba_rgba(COMBINE_ARGS) {
    const Color4 ca = (Color4) {{ .r = props[0] * z, .g = props[1] * z, .b = props[2] * z, .a = props[3] * z }};
    const Color4 cb = (Color4) {{ .r = props[4] * z, .g = props[5] * z, .b = props[6] * z, .a = props[7] * z }};
    return rgba_modulate(ca, cb);
}

static Color4 combine_tex(COMBINE_ARGS) {
    return tex_sample(tex0, props[0] * z, props[1] * z);
}

static Color4 combine_tex_fog(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const uint8_t fog = props[2] * z;
    return rgba_blend(fog_color, tc, fog);
}

static Color4 combine_tex_rgb(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, .a = 0xFF }};
    return rgba_modulate(tc, cc);
}

static Color4 combine_tex_fog_rgb(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const uint8_t fog = props[2] * z;
    const Color4 cc = (Color4) {{ .r = props[3] * z, .g = props[4] * z, .b = props[5] * z, .a = 0xFF }};
    return rgba_blend(fog_color, rgba_modulate(tc, cc), fog);
}

static Color4 combine_tex_rgb_decal(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, .a = 0xFF }};
    return rgba_blend(tc, cc, tc.a);
}

static Color4 combine_tex_rgba(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, .a = props[5] * z }};
    return rgba_modulate(tc, cc);
}

static Color4 combine_tex_rgba_texa(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, .a = 0xFF }};
    return rgba_modulate(tc, cc);
}

static inline bool shader_preserves_texture_alpha(uint32_t shader_id) {
    return shader_id == 0x01A00045 ||
           shader_id == GFX_SOFT_MENU_SHADER_MODULATE ||
           shader_id == GFX_SOFT_MENU_SHADER_MODULATE_EDGE;
}

static Color4 combine_tex_fog_rgba(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const uint8_t fog = props[2] * z;
    const Color4 cc = (Color4) {{ .r = props[3] * z, .g = props[4] * z, .b = props[5] * z, .a = props[6] * z }};
    return rgba_blend(fog_color, rgba_modulate(tc, cc), fog);
}

static Color4 combine_tex_rgba_decal(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, .a = props[5] * z }};
    Color4 out = rgba_blend(tc, cc, tc.a);
    out.a = tc.a;
    return out;
}

static Color4 combine_tex_rgb_rgb(COMBINE_ARGS) {
    const Color4 tc = tex_sample(tex0, props[0] * z, props[1] * z);
    const Color4 cc1 = (Color4) {{ .r = props[2] * z, .g = props[3] * z, .b = props[4] * z, 0xFF }};
    const Color4 cc2 = (Color4) {{ .r = props[5] * z, .g = props[6] * z, .b = props[7] * z, 0xFF }};
    return rgba_lerp(cc2, cc1, tc.r);
}

static Color4 combine_tex_tex_rgba(COMBINE_ARGS) {
    const float u = props[0] * z;
    const float v = props[1] * z;
    const Color4 tc1 = tex_sample(tex0, u, v);
    const Color4 tc2 = tex_sample(tex1, u, v);
    const uint8_t r = props[2] * z;
    return rgba_lerp(tc1, tc2, r);
}

/* fragment plotters */

static inline void gfx_soft_count_drawn_pixel(void) {
#if GFX_SOFT_PIXEL_COUNTERS
    if (gfx_soft_drawing_triangles) {
        gfx_soft_dbg_tri_pixels++;
    } else {
        gfx_soft_dbg_rect_pixels++;
    }
#endif
}

static inline void gfx_soft_dbg_logo_bounds(const int x, const int y) {
#if GFX_SOFT_LOGO_DIAG
    if (!gfx_soft_dbg_logo_active) return;
    if (x < gfx_soft_dbg_logo_min_x) gfx_soft_dbg_logo_min_x = x;
    if (x > gfx_soft_dbg_logo_max_x) gfx_soft_dbg_logo_max_x = x;
    if (y < gfx_soft_dbg_logo_min_y) gfx_soft_dbg_logo_min_y = y;
    if (y > gfx_soft_dbg_logo_max_y) gfx_soft_dbg_logo_max_y = y;
#endif
}

static inline void gfx_soft_dbg_menu_bounds(const int x, const int y) {
#if GFX_SOFT_MENU_DIAG
    if (!gfx_soft_dbg_menu_active) return;
    if (x < gfx_soft_dbg_menu_min_x) gfx_soft_dbg_menu_min_x = x;
    if (x > gfx_soft_dbg_menu_max_x) gfx_soft_dbg_menu_max_x = x;
    if (y < gfx_soft_dbg_menu_min_y) gfx_soft_dbg_menu_min_y = y;
    if (y > gfx_soft_dbg_menu_max_y) gfx_soft_dbg_menu_max_y = y;
#endif
}

static inline Color4 gfx_soft_dbg_logo_color(Color4 src) {
#if GFX_SOFT_LOGO_DIAG
    if (!gfx_soft_dbg_logo_active) return src;
    if (src.a == 0) gfx_soft_dbg_logo_alpha0++;
    if (src.r || src.g || src.b) gfx_soft_dbg_logo_nonblack++;
#if GFX_SOFT_FORCE_TITLE_SOLID
    return (Color4) {{ .r = 255, .g = 0, .b = 255, .a = 255 }};
#else
    return src;
#endif
#else
    return src;
#endif
}

static inline Color4 gfx_soft_dbg_menu_color(Color4 src) {
#if GFX_SOFT_MENU_DIAG
    if (!gfx_soft_dbg_menu_active) return src;
    if (src.a == 0) gfx_soft_dbg_menu_alpha0++;
    if (src.r || src.g || src.b) gfx_soft_dbg_menu_nonblack++;
#endif
    return src;
}

static inline bool gfx_soft_dbg_is_title_shader(void) {
    return cur_shader && cur_shader->shader_id == GFX_SOFT_TITLE_SHADER_TEX;
}

static inline bool gfx_soft_dbg_is_menu_shader(void) {
    return cur_shader && (cur_shader->shader_id == GFX_SOFT_MENU_SHADER_MODULATE ||
                          cur_shader->shader_id == GFX_SOFT_MENU_SHADER_MODULATE_EDGE ||
                          cur_shader->shader_id == GFX_SOFT_MENU_SHADER_DECAL);
}

static void gfx_soft_dbg_logo_begin(size_t tris) {
#if GFX_SOFT_LOGO_DIAG
    if (!GFX_SOFT_LOGO_DIAG || gfx_soft_dbg_logo_summary_count >= 8 || !gfx_soft_dbg_is_title_shader()) {
        gfx_soft_dbg_logo_active = false;
        return;
    }

    gfx_soft_dbg_logo_active = true;
    gfx_soft_dbg_logo_tris = (int)tris;
    gfx_soft_dbg_logo_pixels = 0;
    gfx_soft_dbg_logo_drawn = 0;
    gfx_soft_dbg_logo_zfail = 0;
    gfx_soft_dbg_logo_oob = 0;
    gfx_soft_dbg_logo_alpha0 = 0;
    gfx_soft_dbg_logo_nonblack = 0;
    gfx_soft_dbg_logo_min_x = scr_width;
    gfx_soft_dbg_logo_max_x = -1;
    gfx_soft_dbg_logo_min_y = scr_height;
    gfx_soft_dbg_logo_max_y = -1;
#else
    gfx_soft_dbg_logo_active = false;
#endif
}

static void gfx_soft_dbg_logo_end(void) {
#if GFX_SOFT_LOGO_DIAG
    if (!gfx_soft_dbg_logo_active) return;

    RG_LOGI("gfx_logo_canary: shader=%08lx tris=%d pixels=%d drawn=%d zfail=%d oob=%d alpha0=%d nonblack=%d bounds=(%d,%d)-(%d,%d) force=%d ztest=%d zwrite=%d scissor=(%d,%d)-(%d,%d)",
            (unsigned long)cur_shader->shader_id,
            gfx_soft_dbg_logo_tris,
            gfx_soft_dbg_logo_pixels,
            gfx_soft_dbg_logo_drawn,
            gfx_soft_dbg_logo_zfail,
            gfx_soft_dbg_logo_oob,
            gfx_soft_dbg_logo_alpha0,
            gfx_soft_dbg_logo_nonblack,
            gfx_soft_dbg_logo_min_x,
            gfx_soft_dbg_logo_min_y,
            gfx_soft_dbg_logo_max_x,
            gfx_soft_dbg_logo_max_y,
            GFX_SOFT_FORCE_TITLE_SOLID,
            z_test,
            z_write,
            r_clip.x0,
            r_clip.y0,
            r_clip.x1,
            r_clip.y1);

    gfx_soft_dbg_logo_active = false;
    gfx_soft_dbg_logo_summary_count++;
#endif
}

static void gfx_soft_dbg_menu_begin(size_t tris) {
#if GFX_SOFT_MENU_DIAG
    if (!GFX_SOFT_MENU_DIAG || gfx_soft_dbg_menu_summary_count >= 12 || !gfx_soft_dbg_is_menu_shader()) {
        gfx_soft_dbg_menu_active = false;
        return;
    }

    gfx_soft_dbg_menu_active = true;
    gfx_soft_dbg_menu_tris = (int)tris;
    gfx_soft_dbg_menu_pixels = 0;
    gfx_soft_dbg_menu_drawn = 0;
    gfx_soft_dbg_menu_zfail = 0;
    gfx_soft_dbg_menu_oob = 0;
    gfx_soft_dbg_menu_alpha0 = 0;
    gfx_soft_dbg_menu_nonblack = 0;
    gfx_soft_dbg_menu_min_x = scr_width;
    gfx_soft_dbg_menu_max_x = -1;
    gfx_soft_dbg_menu_min_y = scr_height;
    gfx_soft_dbg_menu_max_y = -1;
#else
    gfx_soft_dbg_menu_active = false;
#endif
}

static void gfx_soft_dbg_menu_end(void) {
#if GFX_SOFT_MENU_DIAG
    if (!gfx_soft_dbg_menu_active) return;

    RG_LOGI("gfx_menu_canary: shader=%08lx tris=%d pixels=%d drawn=%d zfail=%d oob=%d alpha0=%d nonblack=%d bounds=(%d,%d)-(%d,%d) ztest=%d zwrite=%d scissor=(%d,%d)-(%d,%d)",
            (unsigned long)cur_shader->shader_id,
            gfx_soft_dbg_menu_tris,
            gfx_soft_dbg_menu_pixels,
            gfx_soft_dbg_menu_drawn,
            gfx_soft_dbg_menu_zfail,
            gfx_soft_dbg_menu_oob,
            gfx_soft_dbg_menu_alpha0,
            gfx_soft_dbg_menu_nonblack,
            gfx_soft_dbg_menu_min_x,
            gfx_soft_dbg_menu_min_y,
            gfx_soft_dbg_menu_max_x,
            gfx_soft_dbg_menu_max_y,
            z_test,
            z_write,
            r_clip.x0,
            r_clip.y0,
            r_clip.x1,
            r_clip.y1);

    gfx_soft_dbg_menu_active = false;
    gfx_soft_dbg_menu_summary_count++;
#endif
}

int gfx_soft_get_debug_tri_pixels(void) {
    int n = gfx_soft_dbg_tri_pixels;
    gfx_soft_dbg_tri_pixels = 0;
    return n;
}

int gfx_soft_get_debug_rect_pixels(void) {
    int n = gfx_soft_dbg_rect_pixels;
    gfx_soft_dbg_rect_pixels = 0;
    return n;
}

int gfx_soft_get_debug_fill_pixels(void) {
    int n = gfx_soft_dbg_fill_pixels;
    gfx_soft_dbg_fill_pixels = 0;
    return n;
}

static void draw_pixel(const int idx, UNUSED const uint16_t z, Color4 src) {
    gfx_soft_count_drawn_pixel();
    gfx_output[idx] = src.c;
}

static void draw_pixel_zwrite(const int idx, const uint16_t z, Color4 src) {
    gfx_soft_count_drawn_pixel();
    gfx_output[idx] = src.c;
    z_buffer[idx] = z;
}

static void draw_pixel_blend(const int idx, UNUSED const uint16_t z, Color4 src) {
    gfx_soft_count_drawn_pixel();
    const uint8_t a = src.a;
    const uint8_t ia = 255 - a;
    const Color4 dst = (Color4) { .c = gfx_output[idx] };
    src.r = rgba_blend_component(src.r, dst.r, a, ia);
    src.g = rgba_blend_component(src.g, dst.g, a, ia);
    src.b = rgba_blend_component(src.b, dst.b, a, ia);
    gfx_output[idx] = src.c;
}

static void draw_pixel_blend_zwrite(const int idx, const uint16_t z, Color4 src) {
    gfx_soft_count_drawn_pixel();
    const uint8_t a = src.a;
    const uint8_t ia = 255 - a;
    const Color4 dst = (Color4) { .c = gfx_output[idx] };
    src.r = rgba_blend_component(src.r, dst.r, a, ia);
    src.g = rgba_blend_component(src.g, dst.g, a, ia);
    src.b = rgba_blend_component(src.b, dst.b, a, ia);
    gfx_output[idx] = src.c;
    z_buffer[idx] = z;
}

static void draw_pixel_blend_edge(const int idx, UNUSED const uint16_t z, Color4 src) {
    if (src.a > 0x80) {
        gfx_soft_count_drawn_pixel();
        const uint8_t a = src.a;
        const uint8_t ia = 255 - a;
        const Color4 dst = (Color4) { .c = gfx_output[idx] };
        src.r = rgba_blend_component(src.r, dst.r, a, ia);
        src.g = rgba_blend_component(src.g, dst.g, a, ia);
        src.b = rgba_blend_component(src.b, dst.b, a, ia);
        gfx_output[idx] = src.c;
    }
}

static void draw_pixel_blend_edge_zwrite(const int idx, const uint16_t z, Color4 src) {
    if (src.a > 0x80) {
        gfx_soft_count_drawn_pixel();
        const uint8_t a = src.a;
        const uint8_t ia = 255 - a;
        const Color4 dst = (Color4) { .c = gfx_output[idx] };
        src.r = rgba_blend_component(src.r, dst.r, a, ia);
        src.g = rgba_blend_component(src.g, dst.g, a, ia);
        src.b = rgba_blend_component(src.b, dst.b, a, ia);
        gfx_output[idx] = src.c;
        z_buffer[idx] = z;
    }
}

/* rasterizers */

#if GFX_SOFT_LOGO_DIAG
#define GFX_SOFT_LOGO_SCAN_PIXEL(draw_x_, draw_y_) do { \
    if (gfx_soft_dbg_logo_active) { \
        gfx_soft_dbg_logo_pixels++; \
        gfx_soft_dbg_logo_bounds((draw_x_), (draw_y_)); \
    } \
} while (0)
#define GFX_SOFT_LOGO_OOB_PIXEL() do { \
    if (gfx_soft_dbg_logo_active) gfx_soft_dbg_logo_oob++; \
} while (0)
#define GFX_SOFT_LOGO_COLOR(src_) do { \
    (src_) = gfx_soft_dbg_logo_color(src_); \
} while (0)
#define GFX_SOFT_LOGO_DRAWN_PIXEL() do { \
    if (gfx_soft_dbg_logo_active) gfx_soft_dbg_logo_drawn++; \
} while (0)
#define GFX_SOFT_LOGO_ZFAIL_PIXEL() do { \
    if (gfx_soft_dbg_logo_active) gfx_soft_dbg_logo_zfail++; \
} while (0)
#else
#define GFX_SOFT_LOGO_SCAN_PIXEL(draw_x_, draw_y_) ((void)0)
#define GFX_SOFT_LOGO_OOB_PIXEL() ((void)0)
#define GFX_SOFT_LOGO_COLOR(src_) ((void)0)
#define GFX_SOFT_LOGO_DRAWN_PIXEL() ((void)0)
#define GFX_SOFT_LOGO_ZFAIL_PIXEL() ((void)0)
#endif

#if GFX_SOFT_MENU_DIAG
#define GFX_SOFT_MENU_SCAN_PIXEL(draw_x_, draw_y_) do { \
    if (gfx_soft_dbg_menu_active) { \
        gfx_soft_dbg_menu_pixels++; \
        gfx_soft_dbg_menu_bounds((draw_x_), (draw_y_)); \
    } \
} while (0)
#define GFX_SOFT_MENU_OOB_PIXEL() do { \
    if (gfx_soft_dbg_menu_active) gfx_soft_dbg_menu_oob++; \
} while (0)
#define GFX_SOFT_MENU_COLOR(src_) do { \
    (src_) = gfx_soft_dbg_menu_color(src_); \
} while (0)
#define GFX_SOFT_MENU_DRAWN_PIXEL() do { \
    if (gfx_soft_dbg_menu_active) gfx_soft_dbg_menu_drawn++; \
} while (0)
#define GFX_SOFT_MENU_ZFAIL_PIXEL() do { \
    if (gfx_soft_dbg_menu_active) gfx_soft_dbg_menu_zfail++; \
} while (0)
#else
#define GFX_SOFT_MENU_SCAN_PIXEL(draw_x_, draw_y_) ((void)0)
#define GFX_SOFT_MENU_OOB_PIXEL() ((void)0)
#define GFX_SOFT_MENU_COLOR(src_) ((void)0)
#define GFX_SOFT_MENU_DRAWN_PIXEL() ((void)0)
#define GFX_SOFT_MENU_ZFAIL_PIXEL() ((void)0)
#endif

#define R_RASTERIZE_TRI_SEG(y_a, y_b, nprops, combine_fn) \
    register int y = y_a; \
    register int y_end = y_b; \
    register int x, x_end; \
    register int idx; \
    register float dx, w; \
    uint16_t uz; \
    Color4 src; \
    /* draw triangle segment from y_a to y_b */ \
    while (y < y_end) { \
        /* do scissor clipping */ \
        /* Clamp once per scanline so every generated framebuffer index is \
         * valid without repeating bounds branches for every pixel. */ \
        x = imax(0, imax(r_clip.x0, (int)x_a)); \
        x_end = imin(scr_width, imin(r_clip.x1, (int)x_b)); \
        /* do X subpixel prestepping */ \
        dx = 1.f - (x_a - x); \
        for (i = 2; i < nprops; ++i) p[i] = p_a[i] + dx * dp[i].x; \
        idx = scr_width * (scr_height - y - 1) + x; \
        /* draw scanline from current x_a to current x_b */ \
        while (x++ < x_end) { \
            const int draw_x = x - 1; \
            const int draw_y = y; \
            GFX_SOFT_LOGO_SCAN_PIXEL(draw_x, draw_y); \
            GFX_SOFT_MENU_SCAN_PIXEL(draw_x, draw_y); \
            uz = u16clamp(p[2] * 65535.f + raster_z_offset); \
            if (!raster_z_test || uz <= raster_z_buffer[idx]) { \
                w = 1.f / p[3]; /* the combiner will multiply by w any props it needs to persp correct */ \
                src = combine_fn(w, p + 4, raster_tex0, raster_tex1); \
                GFX_SOFT_LOGO_COLOR(src); \
                GFX_SOFT_MENU_COLOR(src); \
                GFX_SOFT_LOGO_DRAWN_PIXEL(); \
                GFX_SOFT_MENU_DRAWN_PIXEL(); \
                if (direct_opaque_draw) { \
                    raster_output[idx] = src.c; \
                    if (direct_opaque_zwrite) raster_z_buffer[idx] = uz; \
                } else { \
                    raster_draw_fn(idx, uz, src); \
                } \
            } else { \
                GFX_SOFT_LOGO_ZFAIL_PIXEL(); \
                GFX_SOFT_MENU_ZFAIL_PIXEL(); \
            } \
            for (i = 2; i < nprops; ++i) p[i] += dp[i].x; \
            ++idx; \
        } \
        /* advance scanline start and end and prop starts */ \
        x_a += dxdy_a; \
        x_b += dxdy_b; \
        for (i = 2; i < nprops; ++i) p_a[i] += dpdy_a[i]; \
        ++y; \
    }

#define R_RASTERIZE(tri, nprops, combine_fn) \
    const float *v0 = (float *)tri.v0; \
    const float *v1 = (float *)tri.v1; \
    const float *v2 = (float *)tri.v2; \
    const bool direct_opaque_draw = cur_shader->draw_flags == 0; \
    const bool direct_opaque_zwrite = z_write; \
    const bool raster_z_test = z_test; \
    const float raster_z_offset = z_offset; \
    uint16_t *const raster_z_buffer = z_buffer; \
    uint32_t *const raster_output = gfx_output; \
    const draw_fn_t raster_draw_fn = draw_fn; \
    const struct Texture *const raster_tex0 = cur_tex[0]; \
    const struct Texture *const raster_tex1 = cur_tex[1]; \
    const int clip_y0 = imax(0, r_clip.y0); \
    const int clip_y1 = imin(scr_height, r_clip.y1); \
    const int y0i = imax(clip_y0, (int)v0[1]); \
    const int y2i = imin(clip_y1, (int)v2[1]); \
    const int y1i = imin(y2i, imax(y0i, (int)v1[1])); \
    if ((y0i == y1i && y0i == y2i) || ((int)v0[0] == (int)v1[0] && (int)v0[0] == (int)v2[0])) \
        return; /* triangle has zero area */ \
    const Vector4 ab = (Vector4) {{ v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2], v1[3] - v0[3] }}; \
    const Vector4 ac = (Vector4) {{ v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2], v2[3] - v0[3] }}; \
    const Vector2 bc = (Vector2) {{ v2[0] - v1[0], v2[1] - v1[1] }}; \
    const float cross = ac.x * ab.y - ab.x * ac.y; \
    if (cross == 0.0f) return; /* zero-area triangle - would cause NaN/Inf and infinite loop */ \
    const float denom = 1.f / cross; \
    const float dxdy_ab = ab.x / ab.y; /* x increment along ab */ \
    const float dxdy_ac = ac.x / ac.y; /* x increment along ac */ \
    const float dxdy_bc = bc.x / bc.y; /* x increment along bc */ \
    const bool side = dxdy_ac > dxdy_ab; /* which side the longer edge (AC) is on */ \
    const float y_pre0 = 1.f - (v0[1] - y0i); /* subpixel pre-step */ \
    float dpdy_a[nprops]; /* vertex prop increments along left edge */ \
    float p_a[nprops]; /* vertex leftmost points */ \
    float p[nprops]; /* current vertex prop values */ \
    Vector2 dp[nprops]; /* X and Y increments for vertex props */ \
    register int i; \
    /* we'll interpolate z/w (p[2]), 1/w (p[3]) and the other properties (also divided by w) */ \
    for (i = 2; i < nprops; ++i) { \
        dp[i].x = ((v2[i] - v0[i]) * ab.y - (v1[i] - v0[i]) * ac.y) * denom; \
        dp[i].y = ((v1[i] - v0[i]) * ac.x - (v2[i] - v0[i]) * ab.x) * denom; \
    } \
    if (!side) { \
        /* longer edge is on the left */ \
        const float dxdy_a = dxdy_ac; \
        /* first column of this scanline is on AC */ \
        float x_a = v0[0] + y_pre0 * dxdy_a; \
        for (i = 2; i < nprops; ++i) { \
            dpdy_a[i] = dxdy_ac * dp[i].x + dp[i].y; \
            p_a[i] = v0[i] + y_pre0 * dpdy_a[i]; \
        } \
        if (y0i < y1i) { \
            /* left is AC, right is AB */ \
            const float dxdy_b = dxdy_ab; \
            /* last column of this scanline */ \
            float x_b = v0[0] + y_pre0 * dxdy_ab; \
            R_RASTERIZE_TRI_SEG(y0i, y1i, nprops, combine_fn); \
        } \
        if (y1i < y2i) { \
            /* left is AC, right is BC */ \
            const float dxdy_b = dxdy_bc; \
            /* calculate prestep for vertex B */ \
            const float y_pre1 = 1.f - (v1[1] - y1i); \
            float x_b = v1[0] + y_pre1 * dxdy_bc; \
            R_RASTERIZE_TRI_SEG(y1i, y2i, nprops, combine_fn); \
        } \
    } else { \
        /* longer edge is on the right */ \
        const float dxdy_b = dxdy_ac; \
        /* last column of this scanline is on AC */ \
        float x_b = v0[0] + y_pre0 * dxdy_ac; \
        if (y0i < y1i) { \
            /* right is AC, left is AB */ \
            const float dxdy_a = dxdy_ab; \
            float x_a = v0[0] + y_pre0 * dxdy_a; \
            for (i = 2; i < nprops; ++i) { \
                dpdy_a[i] = dxdy_ab * dp[i].x + dp[i].y; \
                p_a[i] = v0[i] + y_pre0 * dpdy_a[i]; \
            } \
            R_RASTERIZE_TRI_SEG(y0i, y1i, nprops, combine_fn); \
        } \
        if (y1i < y2i) { \
            /* right is AC, left is BC */ \
            const float y_pre1 = 1.f - (v1[1] - y1i); \
            const float dxdy_a = dxdy_bc; \
            float x_a = v1[0] + y_pre1 * dxdy_a; \
            for (i = 2; i < nprops; ++i) { \
                dpdy_a[i] = dxdy_bc * dp[i].x + dp[i].y; \
                p_a[i] = v1[i] + y_pre1 * dpdy_a[i]; \
            } \
            R_RASTERIZE_TRI_SEG(y1i, y2i, nprops, combine_fn); \
        } \
    }

// define a bunch of rasterizers/interpolators for known property counts
// nprops includes XYZW

#define DEFINE_RAST_FUNC(name, nprops, combine_fn) \
    static void rast_fn_ ## name (const struct Tri tri) { R_RASTERIZE(tri, nprops, combine_fn); }

static inline Color4 combine_indirect(COMBINE_ARGS) {
    return cur_shader->combine(z, props, tex0, tex1);
}

#define GET_GENERIC_RAST_FUNC(nprops) rast_fn_generic_ ## nprops

DEFINE_RAST_FUNC(generic_6, 6, combine_indirect)
DEFINE_RAST_FUNC(generic_7, 7, combine_indirect)
DEFINE_RAST_FUNC(generic_8, 8, combine_indirect)
DEFINE_RAST_FUNC(generic_9, 9, combine_indirect)
DEFINE_RAST_FUNC(generic_10, 10, combine_indirect)
DEFINE_RAST_FUNC(generic_11, 11, combine_indirect)
DEFINE_RAST_FUNC(generic_12, 12, combine_indirect)
DEFINE_RAST_FUNC(generic_13, 13, combine_indirect)
DEFINE_RAST_FUNC(generic_14, 14, combine_indirect)

/* Each software combiner has a fixed interpolant layout. Direct calls preserve
 * the existing math while allowing the compiler to inline the per-pixel work. */
DEFINE_RAST_FUNC(rgb, 7, combine_rgb)
DEFINE_RAST_FUNC(rgba, 8, combine_rgba)
DEFINE_RAST_FUNC(fog_rgb, 8, combine_fog_rgb)
DEFINE_RAST_FUNC(fog_rgba, 9, combine_fog_rgba)
DEFINE_RAST_FUNC(rgba_rgba, 12, combine_rgba_rgba)
DEFINE_RAST_FUNC(tex, 6, combine_tex)
DEFINE_RAST_FUNC(tex_fog, 7, combine_tex_fog)
DEFINE_RAST_FUNC(tex_rgb, 9, combine_tex_rgb)
DEFINE_RAST_FUNC(tex_fog_rgb, 10, combine_tex_fog_rgb)
DEFINE_RAST_FUNC(tex_rgb_decal, 9, combine_tex_rgb_decal)
DEFINE_RAST_FUNC(tex_rgba, 10, combine_tex_rgba)
DEFINE_RAST_FUNC(tex_rgba_texa, 10, combine_tex_rgba_texa)
DEFINE_RAST_FUNC(tex_fog_rgba, 11, combine_tex_fog_rgba)
DEFINE_RAST_FUNC(tex_rgba_decal, 10, combine_tex_rgba_decal)
DEFINE_RAST_FUNC(tex_rgb_rgb, 12, combine_tex_rgb_rgb)
DEFINE_RAST_FUNC(tex_tex_rgba, 9, combine_tex_tex_rgba)

static rast_fn_t gfx_soft_get_specialized_rast(combine_fn_t combine, int num_props,
                                                rast_fn_t fallback) {
#define MATCH_RAST(combine_name, props) \
    if (combine == combine_ ## combine_name && num_props == props) return rast_fn_ ## combine_name
    MATCH_RAST(rgb, 3);
    MATCH_RAST(rgba, 4);
    MATCH_RAST(fog_rgb, 4);
    MATCH_RAST(fog_rgba, 5);
    MATCH_RAST(rgba_rgba, 8);
    MATCH_RAST(tex, 2);
    MATCH_RAST(tex_fog, 3);
    MATCH_RAST(tex_rgb, 5);
    MATCH_RAST(tex_fog_rgb, 6);
    MATCH_RAST(tex_rgb_decal, 5);
    MATCH_RAST(tex_rgba, 6);
    MATCH_RAST(tex_rgba_texa, 6);
    MATCH_RAST(tex_fog_rgba, 7);
    MATCH_RAST(tex_rgba_decal, 6);
    MATCH_RAST(tex_rgb_rgb, 8);
    MATCH_RAST(tex_tex_rgba, 5);
#undef MATCH_RAST
    return fallback;
}

static inline void pop_triangle(const float *buf, const int stride) {
    Vector4 *v0 = (Vector4 *)buf;
    Vector4 *v1 = (Vector4 *)(buf + stride);
    Vector4 *v2 = (Vector4 *)(buf + (stride << 1));
    Vector4 *vt;

    // the vertices come to us in clip space, but already divided by w, still gotta transform
    viewport_transform(v0);
    viewport_transform(v1);
    viewport_transform(v2);

    // sort in Y order
    if (v0->y > v1->y) { vt = v0; v0 = v1; v1 = vt; }
    if (v0->y > v2->y) { vt = v0; v0 = v2; v2 = vt; }
    if (v1->y > v2->y) { vt = v1; v1 = v2; v2 = vt; }

    const struct Tri out = (struct Tri) { (float *)v0, (float *)v1, (float *)v2 };
    cur_shader->rast(out);
}

static inline void depth_clear(void) {
    memset(z_buffer, 0xFF, scr_size << 1);
}

static inline void color_clear(void) {
    memset(gfx_output, 0x00, scr_size << 2);
}

/* FIXME: ztrick fucks with sky blending
static inline void depth_swap(void) {
    ++z_frame;
    if (z_frame & 1) {
        r_view.zn = 0.f;
        r_view.zf = 0.4999f;
        z_reverse = false;
    } else {
        r_view.zn = 1.f;
        r_view.zf = 0.5f;
        z_reverse = true;
    }
    r_view.hz = (r_view.zf - r_view.zn) * 0.5f;
    r_view.cz = (r_view.zn + r_view.zf) * 0.5f;
}
*/

/* interface */

static bool gfx_soft_z_is_from_0_to_1(void) {
    return true;
}

static void gfx_soft_unload_shader(struct ShaderProgram *old_prg) {
    if (cur_shader && (cur_shader == old_prg || !old_prg))
        cur_shader = NULL;
}

static void gfx_soft_load_shader(struct ShaderProgram *new_prg) {
    cur_shader = new_prg;
}

static struct ShaderProgram *gfx_soft_create_and_load_new_shader(uint32_t shader_id) {
    static const rast_fn_t rast_funcs[] = {
        NULL,
        NULL,
        GET_GENERIC_RAST_FUNC(6),
        GET_GENERIC_RAST_FUNC(7),
        GET_GENERIC_RAST_FUNC(8),
        GET_GENERIC_RAST_FUNC(9),
        GET_GENERIC_RAST_FUNC(10),
        GET_GENERIC_RAST_FUNC(11),
        GET_GENERIC_RAST_FUNC(12),
        GET_GENERIC_RAST_FUNC(13),
        GET_GENERIC_RAST_FUNC(14),
    };

    struct CCFeatures ccf;
    gfx_cc_get_features(shader_id, &ccf);

    if (shader_program_pool_size >= sizeof(shader_program_pool) / sizeof(shader_program_pool[0])) {
        if (gfx_soft_dbg_shader_pool_log_count < 8) {
            RG_LOGE("gfx_soft_shader_pool_full[%d]: shader=%08lx size=%u",
                    gfx_soft_dbg_shader_pool_log_count,
                    (unsigned long)shader_id, shader_program_pool_size);
            gfx_soft_dbg_shader_pool_log_count++;
        }
        return cur_shader != NULL ? cur_shader : &shader_program_pool[0];
    }

    struct ShaderProgram *prg = &shader_program_pool[shader_program_pool_size++];

    prg->shader_id = shader_id;
    prg->cc = ccf;

    int num_props = 0;

    if (ccf.opt_fog) num_props++; // software renderer only gets fog intensity

    num_props += ccf.num_inputs * (ccf.opt_alpha ? 4 : 3);
    num_props += ccf.used_textures[0] * 2;

    if (ccf.used_textures[0] && ccf.used_textures[1]) {
        prg->mix = SH_MT_TEXTURE_TEXTURE;
        prg->combine = combine_tex_tex_rgba; // only one such known shader
    } else if (ccf.used_textures[0] && ccf.num_inputs) {
        prg->mix = SH_MT_TEXTURE_COLOR;
        if (ccf.num_inputs > 1)
            prg->combine = combine_tex_rgb_rgb; // only one such known shader
        else if (shader_id == 0x0000038D || shader_id == 0x01200A00 || shader_id == 0x01045A00 || shader_id == 0x0120038D)
            prg->combine = ccf.opt_alpha ? combine_tex_rgba_decal : combine_tex_rgb_decal;
        else if (ccf.opt_fog)
            prg->combine = ccf.opt_alpha ? combine_tex_fog_rgba : combine_tex_fog_rgb;
        else if (ccf.opt_alpha)
            prg->combine = shader_preserves_texture_alpha(shader_id) ? combine_tex_rgba_texa : combine_tex_rgba;
        else
            prg->combine = combine_tex_rgb;
    } else if (ccf.used_textures[0]) {
        prg->mix = SH_MT_TEXTURE;
        prg->combine = ccf.opt_fog ? combine_tex_fog : combine_tex;
    } else if (ccf.num_inputs > 1) {
        prg->mix = SH_MT_COLOR_COLOR;
        prg->combine = combine_rgba_rgba; // only one such known shader
    } else if (ccf.num_inputs) {
        prg->mix = SH_MT_COLOR;
        if (ccf.opt_fog)
            prg->combine = ccf.opt_alpha ? combine_fog_rgba : combine_fog_rgb;
        else
            prg->combine = ccf.opt_alpha ? combine_rgba : combine_rgb;
    }

    if (ccf.opt_alpha) {
        if (ccf.opt_texture_edge)
            prg->draw_flags = DRAW_BLEND_EDGE;
        else
            prg->draw_flags = DRAW_BLEND;
    } else {
        prg->draw_flags = 0;
    }

    prg->num_props = num_props;
    // pick rasterizer that interps the amount of float properties this shader requires
    prg->rast = gfx_soft_get_specialized_rast(prg->combine, num_props,
                                              rast_funcs[num_props]);

    gfx_soft_load_shader(prg);

    return prg;
}

static struct ShaderProgram *gfx_soft_lookup_shader(uint32_t shader_id) {
    for (size_t i = 0; i < shader_program_pool_size; i++)
        if (shader_program_pool[i].shader_id == shader_id)
            return &shader_program_pool[i];
    return NULL;
}

static void gfx_soft_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2]) {
    *num_inputs = prg->cc.num_inputs;
    used_textures[0] = prg->cc.used_textures[0];
    used_textures[1] = prg->cc.used_textures[1];
}

static uint32_t gfx_soft_new_texture(void) {
    const uint32_t id = tex_num;

    if (id >= MAX_TEXTURES) {
        RG_LOGE("gfx_soft: ran out of texture slots (tex_num=%lu >= MAX=%d)", (unsigned long)id, MAX_TEXTURES);
        abort();
    }

    tex_num++;
    tex_hdr[id].sample = tex_sample_nearest_rr;

    return id;
}

static void gfx_soft_select_texture(int tile, uint32_t texture_id) {
    cur_tex[tile] = tex_hdr + texture_id;
    cur_tmu = tile;
}

#if CONFIG_IDF_TARGET_ESP32S3
static void gfx_soft_upload_texture(const uint8_t *rgba32_buf, int width, int height) {
    uint32_t size = width * height * 4;
    uint8_t *addr = rg_alloc(size, MEM_SLOW);
    if (!addr) {
        printf("gfx_soft: could not alloc %u bytes for texture\n", size);
        abort();
    }
    memcpy(addr, rgba32_buf, size);
    struct Texture *tex = cur_tex[cur_tmu];
    if (tex->addr) {
        texcache_addr -= (tex->w * tex->h * 4);
        free((void *)tex->addr);
    }
    tex->addr = (uint32_t)addr;
    tex->w = width;
    tex->h = height;
    tex->wrap_w = width - 1;
    tex->wrap_h = height - 1;
    tex->fw = (float)tex->w;
    tex->fh = (float)tex->h;
    texcache_addr += size;
}
#else
static uint32_t tex_cache_alloc(const uint32_t w, const uint32_t h) {
    const uint32_t size = w * h * 4;

    if (texcache_addr + size > texcache_size) {
        texcache_size += TEXCACHE_STEP + size;
        texcache_size = ALIGN(texcache_size, TEXCACHE_STEP);
        uint8_t *new_cache = rg_alloc(texcache_size, MEM_SLOW);
        if (!new_cache) {
            printf("gfx_soft: could not alloc %u bytes for texture cache\n", texcache_size);
            abort();
        }
        memcpy(new_cache, texcache, texcache_addr);
        free(texcache);  // rg_alloc(MEM_SLOW) memory is safe to free() on ESP-IDF
        texcache = new_cache;
    }

    uint32_t ret = texcache_addr;
    texcache_addr += size;
    return ret;
}

static void gfx_soft_upload_texture(const uint8_t *rgba32_buf, int width, int height) {
    uint32_t addr = tex_cache_alloc(width, height);
    memcpy(texcache + addr, rgba32_buf, width * height * 4);
    struct Texture *tex = cur_tex[cur_tmu];
    tex->addr = addr;
    tex->w = width;
    tex->h = height;
    tex->wrap_w = width - 1;
    tex->wrap_h = height - 1;
    tex->fw = (float)tex->w;
    tex->fh = (float)tex->h;
}
#endif

static inline int gfx_cm_to_local(uint32_t val) {
    if (val & G_TX_CLAMP) return WRAP_CLAMP;
    return (val & G_TX_MIRROR) ? WRAP_MIRROR : WRAP_REPEAT;
}

static void gfx_soft_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    static const sample_fn_t samplers[] = {
        tex_sample_nearest_rr, // 0000
        tex_sample_nearest_rc, // 0001
        tex_sample_nearest_rm, // 0010
        NULL,
        tex_sample_nearest_cr, // 0100
        tex_sample_nearest_cc, // 0101
        tex_sample_nearest_cm, // 0110
        NULL,
        tex_sample_nearest_mr, // 1000
        tex_sample_nearest_mc, // 1001
        tex_sample_nearest_mm, // 1010
    };

    cms = gfx_cm_to_local(cms) << 2;
    cmt = gfx_cm_to_local(cmt);

    cur_tex[tile]->filter = linear_filter;
    cur_tex[tile]->sample = samplers[cms | cmt];
}

static void gfx_soft_set_depth_test(bool depth_test) {
    z_test = depth_test;
}

static void gfx_soft_set_depth_mask(bool z_upd) {
    z_write = z_upd;
}

static void gfx_soft_set_zmode_decal(bool zmode_decal) {
    z_offset = zmode_decal ? -32.f : 0.f;
}

static void gfx_soft_set_viewport(int x, int y, int width, int height) {
    r_view.x = x;
    r_view.y = y;
    r_view.w = width;
    r_view.h = height;
    r_view.hw = width >> 1;
    r_view.hh = height >> 1;
    r_view.cx = x + r_view.hw;
    r_view.cy = y + r_view.hh;
}

static void gfx_soft_set_scissor(int x, int y, int width, int height) {
    r_clip.x0 = imax(0, x);
    r_clip.y0 = imax(0, y);
    r_clip.x1 = imin(scr_width, x + width);
    r_clip.y1 = imin(scr_height, y + height);
}

static void gfx_soft_set_use_alpha(bool use_alpha) {
    do_blend = use_alpha;
}

static void gfx_soft_set_fog_color(const uint8_t *rgb) {
    fog_color.r = rgb[0];
    fog_color.g = rgb[1];
    fog_color.b = rgb[2];
    fog_color.a = 0xFF;
}

static inline void gfx_soft_pick_draw_func(void) {
    static const draw_fn_t draw_funcs[] = {
        draw_pixel,
        draw_pixel_zwrite,
        draw_pixel_blend,
        draw_pixel_blend_zwrite,
        draw_pixel_blend_edge,
        draw_pixel_blend_edge_zwrite,
    };
    draw_fn = draw_funcs[cur_shader->draw_flags | z_write];
}

static void gfx_soft_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    gfx_soft_pick_draw_func();
    const size_t num_verts = 3 * buf_vbo_num_tris;
    const size_t stride = buf_vbo_len / num_verts;
    const bool saved_drawing_triangles = gfx_soft_drawing_triangles;
    gfx_soft_dbg_logo_begin(buf_vbo_num_tris);
    gfx_soft_dbg_menu_begin(buf_vbo_num_tris);
    gfx_soft_drawing_triangles = true;
    for (size_t i = 0; i < num_verts * stride; i += 3 * stride)
        pop_triangle(buf_vbo + i, stride);
    gfx_soft_drawing_triangles = saved_drawing_triangles;
    gfx_soft_dbg_logo_end();
    gfx_soft_dbg_menu_end();
}

static void gfx_soft_fill_rect(int x0, int y0, int x1, int y1, const uint8_t *rgba) {
    // HACK: these are mainly used just to clear the screen and draw simple rects, so we ignore drawmode stuff and Z
    x0 = imax(0, x0);
    y0 = imax(0, y0);
    x1 = imin(scr_width, x1);
    y1 = imin(scr_height, y1);
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    if (GFX_SOFT_PIXEL_COUNTERS) {
        gfx_soft_dbg_fill_pixels += (x1 - x0) * (y1 - y0);
    }

    register const uint32_t color = *(uint32_t *)rgba;
    register uint32_t *base = gfx_output + y0 * scr_width + x0;
    register uint32_t *p;

    register int x, y;
    for (y = y0; y < y1; ++y, base += scr_width) {
        p = base;
        for (x = x0; x < x1; ++x, ++p)
            *p = color;
    }
}

static inline void gfx_soft_tex_rect_replace(int x0, int y0, int x1, int y1, const float u0, const float v0, const float dudx, const float dvdy) {
    register int base = y0 * scr_width + x0;
    register int idx;
    register int x, y;
    float u;
    float v = v0;
    for (y = y0; y < y1; ++y, base += scr_width, v += dvdy) {
        idx = base;
        u = u0;
        for (x = x0; x < x1; ++x, ++idx, u += dudx)
            draw_fn(idx, 0, cur_tex[0]->sample(cur_tex[0], u, v));
    }
}

static inline void gfx_soft_tex_rect_modulate(int x0, int y0, int x1, int y1, const float u0, const float v0, const float dudx, const float dvdy, const Color4 rgba) {
    register int base = y0 * scr_width + x0;
    register int idx;
    register int x, y;
    float u;
    float v = v0;
    for (y = y0; y < y1; ++y, base += scr_width, v += dvdy) {
        idx = base;
        u = u0;
        for (x = x0; x < x1; ++x, ++idx, u += dudx)
            draw_fn(idx, 0, rgba_modulate(cur_tex[0]->sample(cur_tex[0], u, v), rgba));
    }
}

static void gfx_soft_tex_rect(int x0, int y0, int x1, int y1, const float u0, const float v0, const float dudx, const float dvdy, const uint8_t *rgba) {
    x0 = imax(0, x0);
    y0 = imax(0, y0);
    x1 = imin(scr_width, x1);
    y1 = imin(scr_height, y1);
    const bool saved_z_write = z_write;
    const bool saved_z_test = z_test;
    z_write = false;
    z_test = false;
    gfx_soft_pick_draw_func();
    if (cur_shader->cc.num_inputs)
        gfx_soft_tex_rect_modulate(x0, y0, x1, y1, u0, v0, dudx, dvdy, *(Color4 *)rgba);
    else
        gfx_soft_tex_rect_replace(x0, y0, x1, y1, u0, v0, dudx, dvdy);
    z_write = saved_z_write;
    z_test = saved_z_test;
}

static inline void gfx_soft_overlay_write_pixel(const int idx, Color4 src) {
    if (src.a == 0) {
        return;
    }
    gfx_overlay_output[idx] = src.c;
    gfx_overlay_active = true;
}

static inline void gfx_soft_overlay_include_rect(int x0, int y0, int x1, int y1) {
    gfx_overlay_min_x = imin(gfx_overlay_min_x, x0);
    gfx_overlay_min_y = imin(gfx_overlay_min_y, y0);
    gfx_overlay_max_x = imax(gfx_overlay_max_x, x1);
    gfx_overlay_max_y = imax(gfx_overlay_max_y, y1);
    for (int y = y0; y < y1; y++) {
        gfx_overlay_row_min_x[y] = imin(gfx_overlay_row_min_x[y], x0);
        gfx_overlay_row_max_x[y] = imax(gfx_overlay_row_max_x[y], x1);
    }
}

static void gfx_soft_overlay_tex_rect_clipped(int x0, int y0, int x1, int y1,
                                              float u0, float v0, const float dudx, const float dvdy,
                                              const Color4 rgba, const bool modulate) {
    if (!gfx_overlay_output || !cur_tex[0]) {
        return;
    }

    if (x0 < 0) {
        u0 += (float)(-x0) * dudx;
        x0 = 0;
    }
    if (y0 < 0) {
        v0 += (float)(-y0) * dvdy;
        y0 = 0;
    }
    x1 = imin(overlay_width, x1);
    y1 = imin(overlay_height, y1);
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    gfx_soft_overlay_include_rect(x0, y0, x1, y1);

    int base = y0 * overlay_width + x0;
    float v = v0;
    for (int y = y0; y < y1; y++, base += overlay_width, v += dvdy) {
        int idx = base;
        float u = u0;
        for (int x = x0; x < x1; x++, idx++, u += dudx) {
            Color4 src = cur_tex[0]->sample(cur_tex[0], u, v);
            if (modulate) {
                src = rgba_modulate(src, rgba);
            }
            gfx_soft_overlay_write_pixel(idx, src);
        }
    }
}

void gfx_soft_overlay_tex_rect(int x0, int y0, int x1, int y1,
                               float u0, float v0, float dudx, float dvdy,
                               const uint8_t *rgba) {
    if (!cur_shader || !cur_tex[0]) {
        return;
    }

    const bool modulate = cur_shader->cc.num_inputs != 0;
    gfx_soft_overlay_tex_rect_clipped(x0, y0, x1, y1, u0, v0, dudx, dvdy,
                                      *(const Color4 *)rgba, modulate);
}

static inline float gfx_soft_overlay_edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void gfx_soft_overlay_textured_tri(float x0, float y0, float u0, float v0,
                                   float x1, float y1, float u1, float v1,
                                   float x2, float y2, float u2, float v2,
                                   const uint8_t *rgba) {
    if (!gfx_overlay_output || !cur_shader || !cur_tex[0]) {
        return;
    }

    const float area = gfx_soft_overlay_edge(x0, y0, x1, y1, x2, y2);
    if (area == 0.0f) {
        return;
    }

    int min_x = (int)floorf(fminf(x0, fminf(x1, x2)));
    int max_x = (int)ceilf(fmaxf(x0, fmaxf(x1, x2)));
    int min_y = (int)floorf(fminf(y0, fminf(y1, y2)));
    int max_y = (int)ceilf(fmaxf(y0, fmaxf(y1, y2)));

    min_x = imax(0, min_x);
    min_y = imax(0, min_y);
    max_x = imin(overlay_width, max_x);
    max_y = imin(overlay_height, max_y);
    if (max_x <= min_x || max_y <= min_y) {
        return;
    }
    gfx_soft_overlay_include_rect(min_x, min_y, max_x, max_y);

    const Color4 rgba_color = *(const Color4 *)rgba;
    const bool modulate = cur_shader->cc.num_inputs != 0;
    const bool area_positive = area > 0.0f;
    const float inv_area = 1.0f / area;

    for (int y = min_y; y < max_y; y++) {
        for (int x = min_x; x < max_x; x++) {
            const float px = (float)x + 0.5f;
            const float py = (float)y + 0.5f;
            const float w0 = gfx_soft_overlay_edge(x1, y1, x2, y2, px, py);
            const float w1 = gfx_soft_overlay_edge(x2, y2, x0, y0, px, py);
            const float w2 = gfx_soft_overlay_edge(x0, y0, x1, y1, px, py);
            if (area_positive ? (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                              : (w0 > 0.0f || w1 > 0.0f || w2 > 0.0f)) {
                continue;
            }

            const float b0 = w0 * inv_area;
            const float b1 = w1 * inv_area;
            const float b2 = w2 * inv_area;
            const float u = u0 * b0 + u1 * b1 + u2 * b2;
            const float v = v0 * b0 + v1 * b1 + v2 * b2;
            Color4 src = cur_tex[0]->sample(cur_tex[0], u, v);
            if (modulate) {
                src = rgba_modulate(src, rgba_color);
            }
            gfx_soft_overlay_write_pixel(y * overlay_width + x, src);
        }
    }
}

static void gfx_soft_set_resolution(UNUSED const int width, UNUSED const int height) {
    if (!z_buffer) {
        z_buffer = rg_alloc(scr_size * sizeof(int16_t), MEM_SLOW);
        if (!z_buffer) {
            printf("gfx_soft: could not alloc zbuffer for %dx%d\n", scr_width, scr_height);
            abort();
        }
    }
    if (!gfx_output) {
#if CONFIG_IDF_TARGET_ESP32P4
        /* This buffer is written by the rasterizer, read during blending, and
         * read again by the RGB565 display conversion. P4 has sufficient
         * internal RAM; rg_alloc falls back if the capability is unavailable. */
        gfx_output = rg_alloc(scr_size * sizeof(uint32_t), MEM_FAST);
#else
        gfx_output = rg_alloc(scr_size * sizeof(uint32_t), MEM_SLOW);
#endif
        if (!gfx_output) {
            printf("gfx_soft: could not alloc color buffer for %dx%d\n", scr_width, scr_height);
            abort();
        }
    }
    if (!gfx_overlay_output) {
        gfx_overlay_output = rg_alloc(overlay_size * sizeof(uint32_t), MEM_SLOW);
        if (!gfx_overlay_output) {
            printf("gfx_soft: could not alloc overlay buffer for %dx%d\n", overlay_width, overlay_height);
            abort();
        }
        memset(gfx_overlay_output, 0x00, overlay_size * sizeof(uint32_t));
    }
    gfx_overlay_min_x = overlay_width;
    gfx_overlay_min_y = overlay_height;
    gfx_overlay_max_x = 0;
    gfx_overlay_max_y = 0;
    for (int y = 0; y < overlay_height; y++) {
        gfx_overlay_row_min_x[y] = overlay_width;
        gfx_overlay_row_max_x[y] = 0;
    }

    depth_clear();
}

static void gfx_soft_init(void) {
#if CONFIG_IDF_TARGET_ESP32S3
    texcache = NULL;
    texcache_size = 0;
    texcache_addr = 0;
#else
    texcache = rg_alloc(TEXCACHE_STEP, MEM_SLOW); // this will be realloc'd as needed
    texcache_size = TEXCACHE_STEP;
    texcache_addr = 0;
    if (!texcache) {
        printf("gfx_soft: could not alloc %u bytes for texture cache\n", TEXCACHE_STEP);
        abort();
    }
#endif

    tex_hdr = rg_alloc(sizeof(struct Texture) * MAX_TEXTURES, MEM_SLOW);
    if (!tex_hdr) {
        RG_PANIC("Failed to allocate software-renderer texture headers!");
    }
    memset(tex_hdr, 0, sizeof(struct Texture) * MAX_TEXTURES);

    z_test = true;
    z_write = true;
    do_blend = false;
    do_clip = false;

    gfx_soft_set_resolution(gfx_current_dimensions.width, gfx_current_dimensions.height);
}

static void gfx_soft_start_frame(void) {
    // depth_swap(); // FIXME: ztrick
    depth_clear();
    if (gfx_overlay_output && gfx_overlay_active &&
        gfx_overlay_min_x < gfx_overlay_max_x && gfx_overlay_min_y < gfx_overlay_max_y) {
        for (int y = gfx_overlay_min_y; y < gfx_overlay_max_y; y++) {
            const int x0 = gfx_overlay_row_min_x[y];
            const int x1 = gfx_overlay_row_max_x[y];
            if (x0 < x1) {
                memset(gfx_overlay_output + y * overlay_width + x0, 0,
                       (size_t)(x1 - x0) * sizeof(uint32_t));
            }
        }
    }
    for (int y = gfx_overlay_min_y; y < gfx_overlay_max_y; y++) {
        gfx_overlay_row_min_x[y] = overlay_width;
        gfx_overlay_row_max_x[y] = 0;
    }
    gfx_overlay_active = false;
    gfx_overlay_min_x = overlay_width;
    gfx_overlay_min_y = overlay_height;
    gfx_overlay_max_x = 0;
    gfx_overlay_max_y = 0;
}

static void gfx_soft_shutdown(void) {
    free(z_buffer);
#if CONFIG_IDF_TARGET_ESP32S3
    gfx_soft_reset_texture_cache();
    free(tex_hdr);
#else
    free(texcache);
#endif
}

static void gfx_soft_on_resize(void) {
}

#if CONFIG_IDF_TARGET_ESP32S3
void gfx_soft_reset_texture_cache(void) {
    if (tex_hdr) {
        for (int i = 0; i < MAX_TEXTURES; i++) {
            if (tex_hdr[i].addr) {
                free((void *)tex_hdr[i].addr);
                tex_hdr[i].addr = 0;
            }
        }
    }
}
extern void gfx_pc_reset_texture_cache(void);
#endif

static void gfx_soft_end_frame(void) {
#if CONFIG_IDF_TARGET_ESP32S3
    if (texcache_addr > 384 * 1024) {
        gfx_pc_reset_texture_cache();
        gfx_soft_reset_texture_cache();
        texcache_addr = 0;
    }
#endif
}

static void gfx_soft_finish_render(void) {
}

struct GfxRenderingAPI gfx_soft_api = {
    gfx_soft_z_is_from_0_to_1,
    gfx_soft_unload_shader,
    gfx_soft_load_shader,
    gfx_soft_create_and_load_new_shader,
    gfx_soft_lookup_shader,
    gfx_soft_shader_get_info,
    gfx_soft_new_texture,
    gfx_soft_select_texture,
    gfx_soft_upload_texture,
    gfx_soft_set_sampler_parameters,
    gfx_soft_set_depth_test,
    gfx_soft_set_depth_mask,
    gfx_soft_set_zmode_decal,
    gfx_soft_set_viewport,
    gfx_soft_set_scissor,
    gfx_soft_set_use_alpha,
    gfx_soft_draw_triangles,
    gfx_soft_init,
    gfx_soft_on_resize,
    gfx_soft_start_frame,
    gfx_soft_end_frame,
    gfx_soft_finish_render,
    gfx_soft_fill_rect,
    gfx_soft_tex_rect,
    gfx_soft_set_fog_color,
    gfx_soft_shutdown,
};

#endif // ENABLE_OPENGL_LEGACY
