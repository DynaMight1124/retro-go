#if defined(TARGET_PSP)

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include <PR/gbi.h>

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <string.h>

#include "psp_texture_manager.h"

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

void *gu_zbp; // z-buffer (VRAM offset), for debugging
float identity_matrix[4][4] __attribute__((aligned(16))) = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };

/* Shader IDs
id        alp fog edg nse ut0 ut1 num sin0 sin1 mul0 mul1 mix0 mix1 cas
-----------------------------------------------------------------------
69        0   0   0   0   1   0   1   0    1    1    1    1    1    0
512       0   0   0   0   0   0   1   1    1    0    1    0    1    0
909       0   0   0   0   1   0   1   0    1    0    1    1    1    0
1361      0   0   0   0   1   0   2   0    1    0    1    1    1    0
2560      0   0   0   0   1   0   0   1    1    0    1    0    1    0
17059909  1   0   0   0   1   0   1   0    0    1    1    1    1    1
17062400  1   0   0   0   1   0   1   1    0    0    1    0    1    0
17305729  1   0   0   0   0   0   2   0    0    1    1    1    1    1
18092101  1   0   0   0   1   0   1   0    0    1    1    1    1    0
18874437  1   0   0   0   1   0   1   0    1    1    0    1    0    0
18874880  1   0   0   0   0   0   1   1    1    0    0    0    0    1
18875277  1   0   0   0   1   0   1   0    1    0    0    1    0    0
18876928  1   0   0   0   1   0   1   1    1    0    0    0    0    0
27263045  1   0   0   0   1   0   1   0    1    1    0    1    0    0
27265536  1   0   0   0   1   0   0   1    1    0    0    0    0    1
27265647  1   0   0   0   1   1   1   0    1    0    0    1    0    0
52428869  1   1   0   0   1   0   1   0    1    1    0    1    0    0
52429312  1   1   0   0   0   0   1   1    1    0    0    0    0    1
52431360  1   1   0   0   1   0   1   1    1    0    0    0    0    0
84168773  1   0   1   0   1   0   1   0    0    1    1    1    1    1
85983744  1   0   1   0   0   0   1   1    1    0    0    0    0    1
94374400  1   0   1   0   1   0   0   1    1    0    0    0    0    1
127928832 1   1   1   0   1   0   0   1    1    0    0    0    0    1
153092165 1   0   0   1   1   0   1   0    1    1    0    1    0    0
153092608 1   0   0   1   0   0   1   1    1    0    0    0    0    1
153093005 1   0   0   1   1   0   1   0    1    0    0    1    0    0
153094656 1   0   0   1   1   0   1   1    1    0    0    0    0    0

printf("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", shader_id,
    cc_features.opt_alpha,
    cc_features.opt_fog,
    cc_features.opt_texture_edge,
    cc_features.opt_noise,
    cc_features.used_textures[0],
    cc_features.used_textures[1],
    cc_features.num_inputs,
    cc_features.do_single[0],
    cc_features.do_single[1],
    cc_features.do_multiply[0],
    cc_features.do_multiply[1],
    cc_features.do_mix[0],
    cc_features.do_mix[1],
    cc_features.color_alpha_same
);
*/

/* Shader Working List:
84168773    - Menu Overlays
*/

/* Shader Broken List:
153092165   - Noise
153092608   - Noise
153093005   - Noise
153094656   - Noise
*/

// clang-format off
static uint32_t shader_ids[27] =
{
69       ,
512      ,
909      ,
1361     ,
2560     ,
17059909 ,
17062400 ,
17305729 ,
18092101 ,
18874437 ,
18874880 ,
18875277 ,
18876928 ,
27263045 ,
27265536 ,
27265647 ,
52428869 ,
52429312 ,
52431360 ,
84168773 ,
85983744 ,
94374400 ,
127928832,
153092165,
153092608,
153093005,
153094656
};

static uint32_t shader_remap[27*2] = {
69       ,69       ,
512      ,512      ,
909      ,909      ,
1361     ,1361     ,
2560     ,2560     ,
17059909 ,17059909 ,
17062400 ,17062400 ,
17305729 ,17305729 ,
18092101 ,18092101 ,
18874437 ,18874437 ,
18874880 ,18874880 ,
18875277 ,18875277 ,
18876928 ,18876928 ,
27263045 ,27263045 ,
27265536 ,27265536 ,
27265647 ,27265647 ,
52428869 ,52428869 ,
52429312 ,52429312 ,
52431360 ,52431360 ,
84168773 ,84168773 ,
85983744 ,85983744 ,
94374400 ,94374400 ,
127928832,127928832,
153092165,69,
153092608,69,
153093005,69,
153094656,69
};
// clang-format on

static uint32_t shader_broken[27] = {
    153092165, // Noise
    153092608, // Noise
    153093005, // Noise
    153094656  // Noise
};

/* GE display list for one frame; vertex data is allocated inside it too. */
#define GU_LIST_BYTES (512 * 1024) /* self-recycling via the GE_TL overflow guard; small to free RAM for heavy course loads */
unsigned int __attribute__((aligned(64))) list[GU_LIST_BYTES / 4];
extern unsigned int psp_tex_bound;
extern void port_log(const char*, ...); /* 512 KB GE list */

static unsigned int staticOffset = 0;
unsigned int scegu_fog_color = 0;

static unsigned int getMemorySize(unsigned int width, unsigned int height, unsigned int psm) {
    switch (psm) {
        case GU_PSM_T4:
            return (width * height) >> 1;

        case GU_PSM_T8:
            return width * height;

        case GU_PSM_5650:
        case GU_PSM_5551:
        case GU_PSM_4444:
        case GU_PSM_T16:
            return 2 * width * height;

        case GU_PSM_8888:
        case GU_PSM_T32:
            return 4 * width * height;

        default:
            return 0;
    }
}

#define TEX_ALIGNMENT (16)
void *getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm) {
    unsigned int memSize = getMemorySize(width, height, psm);
    void *result = (void *) (staticOffset | 0x40000000);
    staticOffset += memSize;

    return result;
}

void *getStaticVramBufferBytes(size_t bytes) {
    unsigned int memSize = bytes;
    void *result = (void *) (staticOffset | 0x40000000);
    staticOffset += memSize;

    return (void *) (((unsigned int) result) + ((unsigned int) sceGeEdramGetAddr()));
}

#include "gfx_cc.h"
#include "gfx_rendering_api.h"
#include "macros.h"
extern int gfx_debug_frame;
extern void port_log(const char *fmt, ...);
#define GULOG(...) do { if (gfx_debug_frame) port_log(__VA_ARGS__); } while (0)

enum MixType {
    SH_MT_NONE,
    SH_MT_TEXTURE,
    SH_MT_COLOR,
    SH_MT_TEXTURE_TEXTURE,
    SH_MT_TEXTURE_COLOR,
    SH_MT_COLOR_COLOR,
};

struct ShaderProgram {
    bool enabled;
    uint32_t shader_id;
    struct CCFeatures cc;
    enum MixType mix;
    bool texture_used[2];
    int texture_ord[2];
    int num_inputs;
};

struct SamplerState {
    int min_filter;
    int mag_filter;
    int wrap_s;
    int wrap_t;
    uint32_t tex;
};

typedef struct Vertex {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

typedef struct VertexColor {
    unsigned short a, b;
    unsigned long color;
    unsigned short x, y, z;
} VertexColor;

static struct ShaderProgram shader_program_pool[64];
static uint8_t shader_program_pool_size;
static struct ShaderProgram *cur_shader = NULL;
static struct SamplerState tmu_state[2];
static bool gl_blend = false;
static int dbg_texfunc = -1, dbg_alphatest = -1; /* debug: last GE state sent */

static inline uint32_t get_shader_index(uint32_t id) {
    size_t i;
    for (i = 0; i < 27; i++) {
        if (shader_ids[i] == id) {
            return i;
        }
    }
    char msg[32];
    sprintf(msg, "ERROR! Shader not known %u\n", id);
    sceIoWrite(2, msg, strlen(msg));
    return 0;
}

static inline uint32_t get_shader_remap(uint32_t id) {
    size_t index = get_shader_index(id);
    return shader_remap[index * 2 + 1];
}

static inline bool is_shader_enabled(uint32_t id) {
    size_t i;
    for (i = 0; i < 27; i++) {
        if (shader_broken[i] == id) {
            return false;
        }
    }
    return true;
}

static struct ShaderProgram *get_shader_from_id(uint32_t id) {
    size_t i;
    for (i = 0; i < shader_program_pool_size; i++) {
        if (shader_program_pool[i].shader_id == id) {
            return &shader_program_pool[i];
        }
    }
    return NULL;
}

static bool gfx_scegu_z_is_from_0_to_1(void) {
    return true;
}

static inline int texenv_set_color(UNUSED struct ShaderProgram *prg) {
    return GU_TFX_MODULATE;
}

static inline int texenv_set_texture(UNUSED struct ShaderProgram *prg) {
    return GU_TFX_MODULATE;
}

static inline int texenv_set_texture_color(struct ShaderProgram *prg) {
    /* rgb = (TEXEL0 - input) * TEXEL0.a + input is a lerp by texel alpha, which
     * is exactly the GE's DECAL function (shader 0x38D: kart shadows / decals
     * over the shade).  Everything else is texel * vertex colour. */
    const struct CCFeatures *cc = &prg->cc;
    if (cc->c[0][0] == SHADER_TEXEL0 && cc->c[0][2] == SHADER_TEXEL0A && cc->do_mix[0]) {
        return GU_TFX_DECAL;
    }
    return GU_TFX_MODULATE;
}

static inline int texenv_set_texture_texture(UNUSED struct ShaderProgram *prg) {
    /*@Note: hack shader 0x1A00A6F for Bowser/Peach Paintings (still broken, but just fixed on peach)*/
    return GU_TFX_DECAL;
}

static bool gu_zmode_decal;
static bool gu_tex_add; /* texture function ADD: texel + vertex colour (the kart tint, gfx_pc.c) */
static void gfx_scegu_update_depth_offset(void);
static void gfx_scegu_apply_shader(struct ShaderProgram *prg) {
    // If we have textures, Enable otherwise Disable
    if (prg->texture_used[0] || prg->texture_used[1]) {
        sceGuEnable(GU_TEXTURE_2D);
        GULOG("  gu: texture on (shader %08X mix %d)\n", prg->shader_id, prg->mix);
    } else {
        sceGuDisable(GU_TEXTURE_2D);
        GULOG("  gu: texture off (shader %08X)\n", prg->shader_id);
        return;
    }
/*@Note: Revisit one day! */
#if 0
    if (prg->shader_id & SHADER_OPT_FOG) {
        // Yea this doesnt work at all */
        //sceGuFog(scegu_fog_near, scegu_fog_far, 0x00FF0000);//scegu_fog_color); // color is the same for all verts, only intensity is different
        //sceGuEnable(GU_FOG);
        sceGuEnable(GU_BLEND);
    }
#endif

    if (prg->num_inputs) {
        // have colors
        // TODO: more than one color (maybe glSecondaryColorPointer?)
        // HACK: if there's a texture and two colors, one of them is likely for speculars or some shit
        // (see mario head)
        //       if there's two colors but no texture, the real color is likely the second one
        /*
        const int hack = (prg->num_inputs > 1) * (4 - (int)prg->texture_used[0]);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, cur_buf_stride, ofs + hack);
        ofs += 4 * prg->num_inputs;
        */
    }

    if (prg->shader_id & SHADER_OPT_TEXTURE_EDGE) {
        // (horrible) alpha discard
        sceGuEnable(GU_ALPHA_TEST);
        sceGuAlphaFunc(GU_GREATER, 0x55, 0xff); /* 0.3f  */
        GULOG("  gu: alphatest on\n");
        dbg_alphatest = 1;
    } else {
        sceGuDisable(GU_ALPHA_TEST);
        GULOG("  gu: alphatest off\n");
        dbg_alphatest = 0;
    }

    if (!prg->enabled) {
        // configure formulae, we only need to do this once
        prg->enabled = true;

        int mode;
        switch (prg->mix) {
            case SH_MT_TEXTURE:
                mode = texenv_set_texture(prg);
                break;
            case SH_MT_TEXTURE_TEXTURE:
                mode = texenv_set_texture_texture(prg);
                break;
            case SH_MT_TEXTURE_COLOR:
                mode = texenv_set_texture_color(prg);
                break;
            default:
                mode = texenv_set_color(prg);
                break;
        }

        /* (The sm64 PSP port forced GU_TFX_REPLACE for shader 0x01A00045 --
         * texel * shade rgb, texel alpha -- for its transition screens.  MK64
         * shades real geometry with it: Rainbow Road's star guardrail is that
         * texture over yellow vertex colours and came out white, issue #10.) */
        if (gu_tex_add) {
            mode = GU_TFX_ADD; // Cv = Cf + Ct, Av = Af * At
        }
        sceGuTexFunc(mode, GU_TCC_RGBA);
        dbg_texfunc = mode;
        GULOG("  gu: texfunc %d\n", mode);
    }
}

/* Switch the texture function between the shader's own and ADD (issue #15:
 * the kart tint combiner).  Called between batches, inside the GE list. */
void gfx_scegu_set_texfunc_add(bool on) {
    if (gu_tex_add == on) return;
    gu_tex_add = on;
    if (cur_shader) {
        cur_shader->enabled = false;
        gfx_scegu_apply_shader(cur_shader);
        cur_shader->enabled = false; // keep load_shader's always-re-send behaviour
    }
}
/* State reset (gfx_overlay_state_dirty): forget ADD without touching the GE;
 * the forced shader reload re-sends the texture function. */
void gfx_scegu_reset_texfunc_add(void) {
    gu_tex_add = false;
}

static void gfx_scegu_unload_shader(struct ShaderProgram *old_prg) {
    if (cur_shader && (cur_shader == old_prg || !old_prg)) {
        cur_shader->enabled = false;
        cur_shader = NULL;
    }
}

static void gfx_scegu_load_shader(struct ShaderProgram *new_prg) {
    cur_shader = new_prg;
    gfx_scegu_apply_shader(cur_shader);
    if (cur_shader)
        cur_shader->enabled = false;
}

static struct ShaderProgram *gfx_scegu_create_and_load_new_shader(uint32_t shader_id) {
    struct CCFeatures ccf;
    gfx_cc_get_features(shader_id, &ccf);

    if (shader_program_pool_size == sizeof(shader_program_pool) / sizeof(shader_program_pool[0])) {
        extern int gfx_shader_pool_recycled;
        port_log("gfx: shader pool full, recycling\n");
        shader_program_pool_size = 0;
        cur_shader = NULL;
        gfx_shader_pool_recycled = 1; // gfx_pc must drop every cached ShaderProgram pointer
    }
    struct ShaderProgram *prg = &shader_program_pool[shader_program_pool_size++];

    prg->shader_id = shader_id;
    prg->cc = ccf;
    prg->num_inputs = ccf.num_inputs;
    prg->texture_used[0] = ccf.used_textures[0];
    prg->texture_used[1] = ccf.used_textures[1];

    if (ccf.used_textures[0] && ccf.used_textures[1]) {
        prg->mix = SH_MT_TEXTURE_TEXTURE;
        if (ccf.do_single[1]) {
            prg->texture_ord[0] = 1;
            prg->texture_ord[1] = 0;
        } else {
            prg->texture_ord[0] = 0;
            prg->texture_ord[1] = 1;
        }
    } else if (ccf.used_textures[0] && ccf.num_inputs) {
        prg->mix = SH_MT_TEXTURE_COLOR;
    } else if (ccf.used_textures[0]) {
        prg->mix = SH_MT_TEXTURE;
    } else if (ccf.num_inputs > 1) {
        prg->mix = SH_MT_COLOR_COLOR;
    } else if (ccf.num_inputs) {
        prg->mix = SH_MT_COLOR;
    }

    prg->enabled = false;

    gfx_scegu_load_shader(prg);

    return prg;
}

static struct ShaderProgram *gfx_scegu_lookup_shader(uint32_t shader_id) {
    for (size_t i = 0; i < shader_program_pool_size; i++) {
        if (shader_program_pool[i].shader_id == shader_id) {
            return &shader_program_pool[i];
        }
    }
    return NULL;
}

static void gfx_scegu_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2]) {
    *num_inputs = prg->num_inputs;
    used_textures[0] = prg->texture_used[0];
    used_textures[1] = prg->texture_used[1];
}

static unsigned int gfx_scegu_new_texture(void) {
    return texman_create();
}

static uint32_t gfx_cm_to_opengl(uint32_t val) {
    if (val & G_TX_CLAMP)
        return GU_CLAMP;
    return GU_REPEAT;
}

static inline int ispow2(uint32_t x) {
    return (x & (x - 1)) == 0;
}

// compute the next highest power of 2 of 32-bit v
static inline int nextpow2(int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v++;
    return v;
}

static inline void gfx_scegu_apply_tmu_state(const int tile) {
    sceGuTexFilter(tmu_state[tile].min_filter, tmu_state[tile].mag_filter);
    sceGuTexWrap(tmu_state[tile].wrap_s, tmu_state[tile].wrap_t);
    GULOG("  gu: sampler filter %d wrap %d %d\n", tmu_state[tile].min_filter, tmu_state[tile].wrap_s, tmu_state[tile].wrap_t);
}

static void gfx_scegu_set_sampler_parameters(const int tile, const bool linear_filter, const uint32_t cms, const uint32_t cmt) {
    const int filter = linear_filter ? GU_LINEAR : GU_NEAREST;

    const int wrap_s = gfx_cm_to_opengl(cms);
    const int wrap_t = gfx_cm_to_opengl(cmt);

    tmu_state[tile].min_filter = filter;
    tmu_state[tile].mag_filter = filter;
    tmu_state[tile].wrap_s = wrap_s;
    tmu_state[tile].wrap_t = wrap_t;

    // set state for the first texture right away
    if (!tile)
        gfx_scegu_apply_tmu_state(tile);
}

static void gfx_scegu_select_texture(int tile, unsigned int texture_id) {
    // Compare with what the GE actually has bound (uploads rebind behind our
    // back), not with this tile's last selection.
    tmu_state[tile].tex = texture_id;
    if (psp_tex_bound != texture_id) {
        texman_bind_tex(texture_id);
        gfx_scegu_set_sampler_parameters(tile, false, 0, 0);
    }
}

/* Used for rescaling textures ROUGHLY into pow2 dims */
static unsigned int __attribute__((aligned(16))) scaled[256 * 256]; /* 256 KB: one 256x256 RGBA texture */
static void gfx_scegu_resample_32bit(const unsigned int *in, int inwidth, int inheight, unsigned int *out, int outwidth, int outheight) {
    int i, j;
    const unsigned int *inrow;
    unsigned int frac, fracstep;

    fracstep = inwidth * 0x10000 / outwidth;
    for (i = 0; i < outheight; i++, out += outwidth) {
        inrow = in + inwidth * (i * inheight / outheight);
        frac = fracstep >> 1;
        for (j = 0; j < outwidth; j += 4) {
            out[j] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 1] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 2] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 3] = inrow[frac >> 16];
            frac += fracstep;
        }
    }
}

static void gfx_scegu_resample_16bit(const unsigned short *in, int inwidth, int inheight, unsigned short *out, int outwidth, int outheight) {
    int i, j;
    const unsigned short *inrow;
    unsigned int frac, fracstep;

    fracstep = inwidth * 0x10000 / outwidth;
    for (i = 0; i < outheight; i++, out += outwidth) {
        inrow = in + inwidth * (i * inheight / outheight);
        frac = fracstep >> 1;
        for (j = 0; j < outwidth; j += 4) {
            out[j] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 1] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 2] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 3] = inrow[frac >> 16];
            frac += fracstep;
        }
    }
}

static void gfx_scegu_resample_8bit(const unsigned char *in, int inwidth, int inheight, unsigned char *out, int outwidth, int outheight) {
    int i, j;
    const unsigned char *inrow;
    unsigned int frac, fracstep;

    fracstep = inwidth * 0x10000 / outwidth;
    for (i = 0; i < outheight; i++, out += outwidth) {
        inrow = in + inwidth * (i * inheight / outheight);
        frac = fracstep >> 1;
        for (j = 0; j < outwidth; j += 4) {
            out[j] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 1] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 2] = inrow[frac >> 16];
            frac += fracstep;
            out[j + 3] = inrow[frac >> 16];
            frac += fracstep;
        }
    }
}

static void gfx_scegu_upload_texture(const uint8_t *rgba32_buf, int width, int height, unsigned int type) {
    int row_bytes = (type == GU_PSM_8888) ? width * 4 : (type == GU_PSM_T4) ? width / 2 : (type == GU_PSM_T8) ? width : width * 2;
    if (gfx_debug_frame) {
        unsigned int sum = 0, i, n = row_bytes * height;
        for (i = 0; i < n; i++) sum += rgba32_buf[i];
        port_log("  upload %dx%d type %d bytes %u sum %u first %02X%02X %02X%02X\n", width, height, type, n, sum, rgba32_buf[0], rgba32_buf[1], rgba32_buf[2], rgba32_buf[3]);
    }
    if (ispow2(width) && ispow2(height) && (row_bytes % 16) == 0 && (height % 8) == 0) {
        texman_upload_swizzle(width, height, type, (void *) rgba32_buf);
    } else {
        texman_upload_pitched(width, height, type, rgba32_buf);
    }
}

/* N64 back-face culling -> GE hardware culling.  `cull` is the masked
 * geometry-mode bits (G_CULL_FRONT/BACK/BOTH).  Winding: vertices reach the GE
 * in view space and the projection flips handedness, so a front face is CW
 * on screen; front-face is set to CW and we cull the requested side. */
#include <PR/gbi.h>
void gfx_scegu_set_cull_mode(uint32_t cull) {
    static uint32_t last = 0xFFFFFFFF;
    if (cull == last) return;
    last = cull;
    if ((cull & G_CULL_BOTH) == G_CULL_BOTH || cull == 0) {
        sceGuDisable(GU_CULL_FACE); // both handled CPU-side (skip); none = no cull
        return;
    }
    sceGuEnable(GU_CULL_FACE);
    // Cull the requested side.  GU_CW = the side treated as front (kept).
    sceGuFrontFace((cull & G_CULL_FRONT) ? GU_CCW : GU_CW);
}

static int dbg_depth_test = -1;
static void gfx_scegu_set_depth_test(bool depth_test) {
    dbg_depth_test = depth_test;
    if (depth_test) {
        sceGuEnable(GU_DEPTH_TEST);
        GULOG("  gu: depth on\n");
    } else {
        sceGuDisable(GU_DEPTH_TEST);
        GULOG("  gu: depth off\n");
    }
}

/* Depth writes as last requested (the GE starts with them enabled).  The
 * frame-start clear has to turn them on and must put them back afterwards:
 * the interpreter only sends a change, so a frame whose first z-tested draw
 * expects writes off (Rainbow Road: the kart shadow, before the course is
 * drawn) otherwise wrote the shadow's depth and the road under it then
 * failed the depth test -- the black "box" under the kart (issue #10). */
static bool gu_zupd = true;
static void gfx_scegu_set_depth_mask(bool z_upd) {
    gu_zupd = z_upd;
    sceGuDepthMask(!z_upd);
    GULOG("  gu: depthmask zupd %d\n", z_upd);
}

/* Depth bias: decals must win over the coplanar surface they sit on. */
static void gfx_scegu_update_depth_offset(void) {
    sceGuDepthOffset(gu_zmode_decal ? 32 : 0);
}

static void gfx_scegu_set_zmode_decal(bool zmode_decal) {
    gu_zmode_decal = zmode_decal;
    gfx_scegu_update_depth_offset();
}

/* Each setter sets the GE scissor to its own rectangle.  (Making it the overlap
 * of the two was tried on max_fps_experiments: on the PSP, never in PPSSPP, the
 * results screen's replays then drew with garbled textures.  Views smaller than
 * the screen are kept inside their viewport by CPU clipping instead: gfx_pc.c
 * ge_guard_ndc.) */
static void gfx_scegu_set_viewport(int x, int y, int width, int height) {
    sceGuViewport(2048 - (SCR_WIDTH / 2) + x + (width / 2), 2048 + (SCR_HEIGHT / 2) - y - (height / 2), width, height);
    sceGuScissor(x, SCR_HEIGHT - y - height, x + width, SCR_HEIGHT - y);
}

static void gfx_scegu_set_scissor(int x, int y, int width, int height) {
    sceGuScissor(x, SCR_HEIGHT - y - height, x + width, SCR_HEIGHT - y);
}

static void gfx_scegu_set_use_alpha(bool use_alpha) {
    gl_blend = use_alpha;
    if (use_alpha) {
        sceGuEnable(GU_BLEND);
        GULOG("  gu: blend on\n");
    } else {
        sceGuDisable(GU_BLEND);
        GULOG("  gu: blend off\n");
    }
}

// draws the same triangles as plain fog color + fog intensity as alpha
// on top of the normal tris and blends them to achieve sort of the same effect
// as fog would
static inline void gfx_scegu_blend_fog_tris(void) {
    /*@Todo: figure this out! */
    return;
#if 0
    // if a texture was used, replace it with fog color instead, but still keep the alpha
    if (cur_shader->texture_used[0]) {
        glActiveTexture(GL_TEXTURE0);
        TEXENV_COMBINE_ON();
        // out.rgb = input0.rgb
        TEXENV_COMBINE_SET1(RGB, GL_REPLACE, GL_PRIMARY_COLOR);
        // out.a = texel0.a * input0.a
        TEXENV_COMBINE_SET2(ALPHA, GL_MODULATE, GL_TEXTURE, GL_PRIMARY_COLOR);
    }

    glEnableClientState(GL_COLOR_ARRAY); // enable color array temporarily
    glColorPointer(4, GL_FLOAT, cur_buf_stride, cur_fog_ofs); // set fog colors as primary colors
    if (!gl_blend) glEnable(GL_BLEND); // enable blending temporarily
    glDepthFunc(GL_LEQUAL); // Z is the same as the base triangles

    glDrawArrays(GL_TRIANGLES, 0, 3 * cur_buf_num_tris);

    glDepthFunc(GL_LESS); // set back to default
    if (!gl_blend) glDisable(GL_BLEND); // disable blending if it was disabled
    glDisableClientState(GL_COLOR_ARRAY); // will get reenabled later anyway
#endif
}

extern void memcpy_vfpu(void *dst, const void *src, size_t size);
/* Direct vertex emit (gfx_pc.c buf_vbo): a batch is written straight into the
 * list, BATCH_HEADROOM bytes past the list's write position -- room for the
 * commands the flush issues before the draw (the projection matrix) -- and
 * aligned to a cache line, so the block the CPU writes back holds nothing the
 * list code writes through the uncached alias.  Nothing else writes GE
 * commands while a batch is open: every state change flushes first. */
#define BATCH_HEADROOM 512u
static void *batch_ptr;
static unsigned int batch_misses;
void *gfx_scegu_batch_begin(unsigned int max_bytes) {
    unsigned int cur = ((unsigned int) sceGuGetMemory(0)) & ~0x40000000u;
    unsigned int start = (cur + 8u + BATCH_HEADROOM + 63u) & ~63u;
    if (start + max_bytes + 4096u > (unsigned int) list + GU_LIST_BYTES) {
        batch_ptr = NULL; /* too near the end of the list: the staging path, and gfx_flush recycles */
        return NULL;
    }
    batch_ptr = (void *) start;
    return batch_ptr;
}

static void gfx_scegu_draw_triangles(float buf_vbo[], UNUSED size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (gfx_debug_frame) {
        const Vertex* v = (const Vertex*) buf_vbo;
        port_log("  tris %d: v0 (%.2f,%.2f,%.2f) uv (%.2f,%.2f) col %08X tex %d [blend %d alphatest %d texfunc %d depth %d]\n", (int) buf_vbo_num_tris, v[0].x, v[0].y, v[0].z, v[0].u, v[0].v, v[0].color, (cur_shader && cur_shader->texture_used[0]) ? (int) psp_tex_bound : -1,
                 gl_blend, dbg_alphatest, dbg_texfunc, dbg_depth_test);
    }
    if (!is_shader_enabled(cur_shader->shader_id)) {
        gfx_scegu_apply_shader(get_shader_from_id(get_shader_remap(cur_shader->shader_id)));
    }

#ifdef PORT_GFX_BISECT
    sceGuDisable(GU_SCISSOR_TEST); sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_ALPHA_TEST); sceGuDisable(GU_BLEND);
    sceGuDisable(GU_CULL_FACE);
#endif
#ifdef PORT_DEBUG_GUARD
    { extern int gfx_trace_frames;
    if (gfx_trace_frames > 0) {
        const Vertex* vv = (const Vertex*) buf_vbo;
        size_t nv = 3 * buf_vbo_num_tris, k;
        static uint32_t batch;
        for (k = 0; k < nv; k++) {
            float x=vv[k].x,y=vv[k].y,z=vv[k].z,u=vv[k].u,vt=vv[k].v;
            int bad = !(x==x)||!(y==y)||!(z==z)||!(u==u)||!(vt==vt)
                    || x>1e9f||x<-1e9f||y>1e9f||y<-1e9f||z>1e9f||z<-1e9f
                    || u>1e7f||u<-1e7f||vt>1e7f||vt<-1e7f;
            if (bad) {
                port_log("BADVTX batch %u vtx %u: xyz(%g,%g,%g) uv(%g,%g) tex %d shader %08X\n",
                    batch, (unsigned)k, x,y,z,u,vt,
                    (cur_shader && cur_shader->texture_used[0])?(int)psp_tex_bound:-1,
                    cur_shader?cur_shader->shader_id:0);
                break;
            }
        }
        batch++;
    }}
#endif
    {
        unsigned int nbytes = sizeof(Vertex) * 3 * buf_vbo_num_tris;
        void *buf = NULL;
        if ((void *) buf_vbo == batch_ptr && batch_ptr != NULL) {
            /* The vertices are already in list memory (gfx_scegu_batch_begin),
             * in whole cache lines of their own.  Put them in RAM, then move
             * the list's write pointer past them: sceGuGetMemory(n) returns
             * its current position + 8 and continues n bytes after that. */
            unsigned int end = ((unsigned int) batch_ptr + nbytes + 63u) & ~63u;
            unsigned int cur = ((unsigned int) sceGuGetMemory(0)) & ~0x40000000u; /* where the list is now */
            if (cur + 8u <= (unsigned int) batch_ptr && (unsigned int) batch_ptr - cur <= BATCH_HEADROOM + 128u) {
                sceKernelDcacheWritebackInvalidateRange(batch_ptr, end - (unsigned int) batch_ptr);
                sceGuGetMemory((int) (end - (cur + 8u)));
                buf = batch_ptr;
            } else {
                batch_misses++; /* the list moved (recycled, or more commands than the headroom): copy */
            }
        }
        if (buf == NULL) {
            buf = sceGuGetMemory(nbytes);
            memcpy(buf, buf_vbo, nbytes);
        }
        batch_ptr = NULL;
        sceGuDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 3 * buf_vbo_num_tris, 0, buf);
    }

    // cur_fog_ofs is only set if GL_EXT_fog_coord isn't used
    // if (cur_fog_ofs) gfx_scegu_blend_fog_tris();
}

void gfx_scegu_draw_triangles_2d(float buf_vbo[], UNUSED size_t buf_vbo_len, UNUSED size_t buf_vbo_num_tris) {
    if (!is_shader_enabled(cur_shader->shader_id)) {
        gfx_scegu_apply_shader(get_shader_from_id(get_shader_remap(cur_shader->shader_id)));
    }

#ifdef PORT_GFX_BISECT
    sceGuDisable(GU_SCISSOR_TEST); sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_ALPHA_TEST); sceGuDisable(GU_BLEND);
#endif
    void *quad_buf = sceGuGetMemory(sizeof(VertexColor) * 2);
    memcpy(quad_buf, buf_vbo, sizeof(VertexColor) * 2);
    if (gfx_debug_frame) {
        const VertexColor* v = (const VertexColor*) buf_vbo;
        port_log("  sprite (%d,%d,%d)-(%d,%d,%d) uv (%d,%d)-(%d,%d) col %08lX tex %d\n", v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[0].a, v[0].b, v[1].a, v[1].b, v[0].color, (cur_shader && (cur_shader->texture_used[0] || cur_shader->texture_used[1])) ? psp_tex_bound : -1);
    }
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, quad_buf);
}

/*============================================================================*/
/* Stadium TV screens: capture a patch of the finished frame into N64 tiles   */
/*============================================================================*/

/* The game (copy_framebuffer in skybox_and_splitscreen.c) asks for up to six
 * 64x32 RGBA16 tiles per frame, each a patch of the previous frame in N64
 * 320x240 coordinates.  Requests are collected while the game builds its
 * display list; once the frame has been rendered (gfx_scegu_end_frame, after
 * the sync) the GE scales every requested patch of the frame into a small
 * 5551 render target, sceGuCopyImage brings it back to RAM (PPSSPP reads a
 * framebuffer back only through a block transfer), and the CPU rewrites it as
 * big-endian N64 RGBA16 in the tile the interpreter will import next frame.
 * Issue #11. */
#define CAP_MAX      8
#define CAP_SLOT_W   64
#define CAP_SLOT_H   32
#define CAP_W        (CAP_SLOT_W * 2)               /* 2 x 4 slots */
#define CAP_H        (CAP_SLOT_H * (CAP_MAX / 2))
typedef struct { int x, y, w, h; uint16_t *target; } CaptureReq;
static CaptureReq cap_req[CAP_MAX];
static int cap_count;
static void *cap_vram;                                /* offset in VRAM (getStaticVramBuffer style) */
static void *cur_draw_fb;                             /* draw buffer being rendered this frame (same style) */
static uint16_t cap_ram[CAP_W * CAP_H] __attribute__((aligned(64)));
extern void gfx_overlay_state_dirty(void);

void port_fb_copy_request(int x, int y, int w, int h, uint16_t *target) {
    int i;
    if (cap_vram == NULL || target == NULL || (uintptr_t) target < 0x08800000u || w <= 0 || h <= 0) {
        return;
    }
    if (w > CAP_SLOT_W) w = CAP_SLOT_W;
    if (h > CAP_SLOT_H) h = CAP_SLOT_H;
    for (i = 0; i < cap_count; i++) {
        if (cap_req[i].target == target) break; // same tile asked twice: keep the latest
    }
    if (i == cap_count) {
        if (cap_count == CAP_MAX) return;
        cap_count++;
    }
    cap_req[i].x = x; cap_req[i].y = y; cap_req[i].w = w; cap_req[i].h = h; cap_req[i].target = target;
}

/* N64 320x240 frame coordinates -> PSP frame pixels.  The 3D view keeps the
 * N64's vertical field of view (skybox_and_splitscreen.c), so the N64 picture
 * is the centred 4:3 part of the 480x272 frame, scaled by 272/240. */
static inline float cap_map_x(int x) {
#ifdef PORT_NO_WIDE_FOV
    return x * (SCR_WIDTH / 320.0f);
#else
    return (SCR_WIDTH - 320.0f * SCR_HEIGHT / 240.0f) * 0.5f + x * (SCR_HEIGHT / 240.0f);
#endif
}
static inline float cap_map_y(int y) {
    return y * (SCR_HEIGHT / 240.0f);
}

static void gfx_scegu_capture_screens(void) {
    const unsigned char *edram = (const unsigned char *) sceGeEdramGetAddr();
    const void *frame_abs = edram + ((uintptr_t) cur_draw_fb & 0x00FFFFFFu);
    void *cap_abs = (void *) (edram + ((uintptr_t) cap_vram & 0x00FFFFFFu));
    const volatile uint16_t *src = (const volatile uint16_t *) ((uintptr_t) cap_ram | 0x40000000u); // uncached: the GE wrote it
    int i, x, y;

    if (cap_count == 0) {
        return;
    }
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBufferList(GU_PSM_5551, cap_vram, CAP_W);
    sceGuOffset(2048 - (CAP_W / 2), 2048 - (CAP_H / 2));
    sceGuViewport(2048, 2048, CAP_W, CAP_H);
    sceGuScissor(0, 0, CAP_W, CAP_H);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDepthMask(GU_TRUE);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_5650, 0, 0, 0);
    sceGuTexImage(0, 512, 512, BUF_WIDTH, frame_abs);
    sceGuTexScale(1.0f, 1.0f);
    sceGuTexOffset(0.0f, 0.0f);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFlush();
    for (i = 0; i < cap_count; i++) {
        const CaptureReq *r = &cap_req[i];
        VertexColor *v = (VertexColor *) sceGuGetMemory(sizeof(VertexColor) * 2);
        int sx = (i & 1) * CAP_SLOT_W, sy = (i >> 1) * CAP_SLOT_H;
        v[0].a = (unsigned short) (cap_map_x(r->x) + 0.5f);
        v[0].b = (unsigned short) (cap_map_y(r->y) + 0.5f);
        v[0].color = 0xFFFFFFFF;
        v[0].x = (unsigned short) sx; v[0].y = (unsigned short) sy; v[0].z = 0;
        v[1].a = (unsigned short) (cap_map_x(r->x + r->w) + 0.5f);
        v[1].b = (unsigned short) (cap_map_y(r->y + r->h) + 0.5f);
        v[1].color = 0xFFFFFFFF;
        v[1].x = (unsigned short) (sx + r->w); v[1].y = (unsigned short) (sy + r->h); v[1].z = 0;
        sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, v);
        GULOG("  capture %d: n64 (%d,%d) %dx%d -> psp (%u,%u)-(%u,%u) tile %p\n", i, r->x, r->y, r->w, r->h, v[0].a, v[0].b, v[1].a, v[1].b, r->target);
    }
    sceGuCopyImage(GU_PSM_5551, 0, 0, CAP_W, CAP_H, CAP_W, cap_abs, 0, 0, CAP_W, cap_ram);
    sceGuTexSync();
    // Back to the frame.  gfx_overlay_state_dirty() (below) makes the
    // interpreter re-send its state, but it only re-sends what DIFFERS from
    // the state it declares (blend on, depth test off, depth writes off, no
    // decal), so the GE must be left in exactly that state -- through the
    // backend's own setters so their caches agree.  Leaving blend disabled
    // here drew the Luigi Raceway clouds on black boxes (issue #11 follow-up).
    sceGuDrawBufferList(GU_PSM_5650, cur_draw_fb, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2), SCR_WIDTH, SCR_HEIGHT);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    gfx_scegu_set_use_alpha(true);
    gfx_scegu_set_depth_test(false);
    gfx_scegu_set_depth_mask(false);
    gfx_scegu_set_zmode_decal(false);
    sceGuFinish();
    sceGuSync(0, 0);

    for (i = 0; i < cap_count; i++) {
        const CaptureReq *r = &cap_req[i];
        int sx = (i & 1) * CAP_SLOT_W, sy = (i >> 1) * CAP_SLOT_H;
        uint16_t *dst = r->target;
        uint32_t sum = 0;
        static int logged;
        for (y = 0; y < r->h; y++) {
            const volatile uint16_t *row = src + (sy + y) * CAP_W + sx;
            for (x = 0; x < r->w; x++) {
                uint16_t p = row[x]; // PSP 5551: a b g r (low bits = red)
                uint16_t n64 = (uint16_t) (((p & 0x1F) << 11) | (((p >> 5) & 0x1F) << 6) | (((p >> 10) & 0x1F) << 1) | 1);
                sum += n64;
                *dst++ = (uint16_t) ((n64 << 8) | (n64 >> 8)); // big-endian texel, read back with be16()
            }
        }
        if (logged < 12) { // the first tiles, once: proves the readback is live on the device
            logged++;
            port_log("gfx: screen tile %p <- frame (%d,%d) %dx%d, texel sum %08X\n", r->target, r->x, r->y, r->w, r->h, (unsigned) sum);
        }
    }
    cap_count = 0;
    psp_tex_bound = (unsigned int) -1; // the GE has our frame bound, not a texman texture
    gfx_overlay_state_dirty();
}

static void gfx_scegu_init(void) {
    sceGuInit();

    void *fbp0 = getStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_5650);
    void *fbp1 = getStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_5650);
    void *zbp = getStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_4444);
    gu_zbp = zbp;

    cur_draw_fb = fbp0;
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_5650, fbp0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, fbp1, BUF_WIDTH);
    sceGuDepthBuffer(zbp, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2), SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(0xffff, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0x55, 0xff); /* 0.3f  */
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_CULL_FACE);
    sceGuFrontFace(GU_CCW);
    sceGuDepthMask(GU_FALSE);
    sceGuTexEnvColor(0xffffffff);
    sceGuTexOffset(0.0f, 0.0f);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    cap_vram = getStaticVramBuffer(CAP_W, CAP_H, GU_PSM_5551); // stadium screen capture target (issue #11)
    void *texman_buffer = getStaticVramBufferBytes(TEXMAN_BUFFER_SIZE);
    void *texman_aligned = (void *) ((((unsigned int) texman_buffer + TEX_ALIGNMENT - 1) / TEX_ALIGNMENT) * TEX_ALIGNMENT);
    texman_reset(texman_aligned, TEXMAN_BUFFER_SIZE);
    if (!texman_buffer) {
        char msg[32];
        sprintf(msg, "OUT OF MEMORY!\n");
        sceIoWrite(1, msg, strlen(msg));

        sceKernelExitGame();
    }
}

/* Run everything queued so far and start a fresh list: called before the
 * texture arena is recycled mid-frame, so no pending draw reads freed VRAM. */
void gfx_scegu_sync_pending(void) {
    sceGuFinish();
    sceGuSync(0, 0);
    sceGuStart(GU_DIRECT, list);
}

static void gfx_scegu_start_frame(void) {
    sceGuStart(GU_DIRECT, list);
    sceGuDisable(GU_SCISSOR_TEST);
    sceGuDepthMask(GU_TRUE); // Must be set to clear Z-buffer
    sceGuClearColor(0xFF000000);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthMask(!gu_zupd); // back to what the interpreter last asked for (see gfx_scegu_set_depth_mask)

    // Identity every frame? unsure.
    //sceGuSetMatrix(GU_PROJECTION, (const ScePspFMatrix4 *) identity_matrix);
    sceGuSetMatrix(GU_VIEW, (const ScePspFMatrix4 *) identity_matrix);
    sceGuSetMatrix(GU_MODEL, (const ScePspFMatrix4 *) identity_matrix); // vertices arrive in view space

#if 0
    const int DitherMatrix[2][16] = { { 0, 8, 0, 8,
                         8, 0, 8, 0,
                         0, 8, 0, 8,
                         8, 0, 8, 0 },
                        { 8, 8, 8, 8,
                          0, 8, 0, 8,
                          8, 8, 8, 8,
                          0, 8, 0, 8 } };

    extern int gDoDither;
    extern int gFrame;

    sceGuDisable(GU_DITHER);
    if(gDoDither){
        // every frame
        sceGuSetDither((const ScePspIMatrix4 *)DitherMatrix[(gFrame&1)]);
        sceGuEnable(GU_DITHER);
    }
#endif
}

void gfx_scegu_on_resize(void) {
}

extern int gPortVblanksPerFrame, gPortLastFrameVblanks; /* port.h (s32): the 60 fps split frames */
extern unsigned int gPortLastFrameBusyUs;
static void gfx_scegu_end_frame(void) {
    static unsigned int max_used, frames;
    unsigned int used = (unsigned int) sceGuFinish();
    if (used > max_used) {
        max_used = used;
    }
    if ((++frames % 300) == 0 || used > GU_LIST_BYTES - 65536) {
        port_log("gfx: display list %u bytes (max %u of %u)\n", used, max_used, (unsigned) GU_LIST_BYTES);
    }
    sceGuSync(0, 0);
    gfx_scegu_capture_screens(); // stadium TV tiles from the finished frame (before the vblank wait absorbs it)
    // MK64 runs one game frame per two VI retraces: lock to 30 fps by waiting
    // for the second vblank since the last swap (no wait if we are already late).
    {
        static int last_vcount = -1;
        static unsigned int last_swap_us;
        int target = last_vcount + gPortVblanksPerFrame; // 2, or 1 for a split (60 fps) frame
        gPortLastFrameBusyUs = sceKernelGetSystemTimeLow() - last_swap_us;
        while (last_vcount >= 0 && (int) sceDisplayGetVcount() < target) {
            sceDisplayWaitVblankStart();
        }
        gPortLastFrameVblanks = last_vcount >= 0 ? (int) sceDisplayGetVcount() - last_vcount : gPortVblanksPerFrame;
        last_vcount = (int) sceDisplayGetVcount();
        last_swap_us = sceKernelGetSystemTimeLow();
    }
    cur_draw_fb = sceGuSwapBuffers(); // the buffer the next frame renders into
}

static void gfx_scegu_finish_render(void) {
    /* There should be something here! */
}

// clang-format off
struct GfxRenderingAPI gfx_opengl_api = {
    gfx_scegu_z_is_from_0_to_1,
    gfx_scegu_unload_shader,
    gfx_scegu_load_shader,
    gfx_scegu_create_and_load_new_shader,
    gfx_scegu_lookup_shader,
    gfx_scegu_shader_get_info,
    gfx_scegu_new_texture,
    gfx_scegu_select_texture,
    gfx_scegu_upload_texture,
    gfx_scegu_set_sampler_parameters,
    gfx_scegu_set_depth_test,
    gfx_scegu_set_depth_mask,
    gfx_scegu_set_zmode_decal,
    gfx_scegu_set_viewport,
    gfx_scegu_set_scissor,
    gfx_scegu_set_use_alpha,
    gfx_scegu_draw_triangles,
    gfx_scegu_init,
    gfx_scegu_on_resize,
    gfx_scegu_start_frame,
    gfx_scegu_end_frame,
    gfx_scegu_finish_render
};

#endif // RAPI_GL_LEGACY
