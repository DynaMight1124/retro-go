/**
 * Debug-only helpers for the PSP port: frame/depth dumps, the backend
 * self-test and the frame-numbered probes used with the scripted input run.
 *
 * Compiled to nothing in a normal build (the two 272 KB VRAM copy buffers
 * alone are worth keeping out of a PSP-1000's RAM).  Enable with
 *   EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT   (scripted run + probes)
 *   EXTRA_CFLAGS=-DPORT_GFX_DEBUG      (periodic screenshots)
 *   EXTRA_CFLAGS=-DPORT_GFX_SELFTEST   (draw straight through the backend at boot)
 */
#include <ultra64.h>
#include <macros.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port.h"
#include "gfx_rendering_api.h"

#if defined(PORT_INPUT_SCRIPT) || defined(PORT_GFX_DEBUG) || defined(PORT_GFX_SELFTEST)
#define PORT_DEBUG_DUMPS 1
#endif

#ifdef PORT_DEBUG_DUMPS
extern struct GfxRenderingAPI gfx_opengl_api; // gfx_scegu.c keeps the sm64-port name
extern unsigned int list[];
extern void gfx_scegu_draw_triangles_2d(float buf_vbo[], size_t len, size_t n);
extern float identity_matrix[4][4];

/* Read the frame currently on screen back out of VRAM and write a binary PPM. */
static void port_dump_buffer(int index, const void* topaddr, int bufferwidth, int pixelformat) {
    char name[64];
    FILE* fp;
    int x, y;
    static u8 row[480 * 3];

    snprintf(name, sizeof(name), "%s" "shot%03d.ppm", port_save_dir(), index);
    fp = fopen(name, "wb");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "P6\n480 272\n255\n");
    for (y = 0; y < 272; y++) {
        const u16* src = (const u16*) ((const u8*) topaddr + y * bufferwidth * 2);
        for (x = 0; x < 480; x++) {
            u16 p = src[x];
            if (pixelformat == PSP_DISPLAY_PIXEL_FORMAT_565) {
                row[x * 3 + 0] = (p & 0x1F) << 3;
                row[x * 3 + 1] = ((p >> 5) & 0x3F) << 2;
                row[x * 3 + 2] = ((p >> 11) & 0x1F) << 3;
            } else { // 5551
                row[x * 3 + 0] = (p & 0x1F) << 3;
                row[x * 3 + 1] = ((p >> 5) & 0x1F) << 3;
                row[x * 3 + 2] = ((p >> 10) & 0x1F) << 3;
            }
        }
        fwrite(row, 1, sizeof(row), fp);
    }
    fclose(fp);
    PORT_LOG("wrote %s\n", name);
}

/* One 272 KB copy buffer shared by the screenshot and the depth dump. */
static u16 sCopy[512 * 272] __attribute__((aligned(64)));

/* Copy the displayed frame out of VRAM with the GE (sceGuCopyImage): reading
 * VRAM from the CPU is unreliable in emulators. */
void port_screenshot(int index) {
    void* topaddr = NULL;
    int bufferwidth = 0;
    int pixelformat = 0;

    sceDisplayGetFrameBuf(&topaddr, &bufferwidth, &pixelformat, PSP_DISPLAY_SETBUF_IMMEDIATE);
    if (topaddr == NULL) {
        return;
    }
    sceGuStart(GU_DIRECT, list);
    sceGuCopyImage(GU_PSM_5650, 0, 0, 480, 272, bufferwidth, (void*) (((uintptr_t) topaddr & 0x1FFFFFFF) | 0x04000000), 0, 0, 512, sCopy);
    sceGuFinish();
    sceGuSync(0, 0);
    sceKernelDcacheInvalidateRange(sCopy, sizeof(sCopy));
    port_dump_buffer(index, sCopy, 512, PSP_DISPLAY_PIXEL_FORMAT_565);
}

/* Dump the 16-bit z-buffer as a grey PPM (near = bright). */
void port_depthshot(int index) {
    extern void* gu_zbp;
    char name[64];
    FILE* fp;
    int x, y;
    static u8 row[480 * 3];
    void* zaddr = (void*) (((uintptr_t) gu_zbp & 0x1FFFFF) | 0x04000000);
    sceGuStart(GU_DIRECT, list);
    sceGuCopyImage(GU_PSM_4444, 0, 0, 480, 272, 512, zaddr, 0, 0, 512, sCopy);
    sceGuFinish();
    sceGuSync(0, 0);
    sceKernelDcacheInvalidateRange(sCopy, sizeof(sCopy));
    snprintf(name, sizeof(name), "%s" "depth%03d.ppm", port_save_dir(), index);
    fp = fopen(name, "wb");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "P6\n480 272\n255\n");
    for (y = 0; y < 272; y++) {
        for (x = 0; x < 480; x++) {
            u8 v = sCopy[y * 512 + x] >> 8;
            row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = v;
        }
        fwrite(row, 1, sizeof(row), fp);
    }
    fclose(fp);
    PORT_LOG("wrote %s\n", name);
}

/* Draw a red textured sprite, a blue non-aligned strip and a green triangle
 * straight through the backend and dump the frame: separates texture/data
 * problems from GU state. */
void port_debug_selftest(void) {
#ifdef PORT_GFX_SELFTEST
    static u16 tex[16 * 16];
    static u16 tex2[20 * 3];
    struct { u16 u, v; u32 color; u16 x, y, z; } spr[2] = { { 0, 0, 0xFFFFFFFF, 100, 100, 0 }, { 16, 16, 0xFFFFFFFF, 200, 200, 0 } };
    struct { u16 u, v; u32 color; u16 x, y, z; } spr2[2] = { { 0, 0, 0xFFFFFFFF, 300, 100, 0 }, { 20, 3, 0xFFFFFFFF, 400, 130, 0 } };
    struct { float u, v; u32 color; float x, y, z; } tri[3] = { { 0, 0, 0xFF00FF00, -0.5f, -0.5f, 0.5f }, { 0, 0, 0xFF00FF00, 0.5f, -0.5f, 0.5f }, { 0, 0, 0xFF00FF00, 0.0f, 0.5f, 0.5f } };
    struct GfxRenderingAPI* r = &gfx_opengl_api;
    int i;
    u32 id;
    for (i = 0; i < 256; i++) tex[i] = 0x801F; // 5551: A=1, R=31
    for (i = 0; i < 60; i++) tex2[i] = 0xFC00;  // A=1, B=31
    r->start_frame();
    id = r->new_texture();
    r->select_texture(0, id);
    r->upload_texture((const u8*) tex, 16, 16, 1 /* GU_PSM_5551 */);
    r->set_sampler_parameters(0, false, 0, 0);
    r->load_shader(r->create_and_load_new_shader(0x45)); // texture * shade
    r->set_depth_test(false);
    r->set_use_alpha(false);
    r->set_viewport(0, 0, 480, 272);
    r->set_scissor(0, 0, 480, 272);
    gfx_scegu_draw_triangles_2d((float*) spr, 0, 1);
    id = r->new_texture(); // non-aligned 20x3 texture through the pitched path
    r->select_texture(0, id);
    r->upload_texture((const u8*) tex2, 20, 3, 1);
    r->set_sampler_parameters(0, false, 0, 0);
    gfx_scegu_draw_triangles_2d((float*) spr2, 0, 1);
    r->load_shader(r->create_and_load_new_shader(0x200)); // shade only
    sceGuSetMatrix(GU_PROJECTION, (const ScePspFMatrix4*) identity_matrix);
    sceGuSetMatrix(GU_VIEW, (const ScePspFMatrix4*) identity_matrix);
    sceGuSetMatrix(GU_MODEL, (const ScePspFMatrix4*) identity_matrix);
    r->draw_triangles((float*) tri, 0, 1);
    r->end_frame();
    r->finish_render();
    port_screenshot(999);
    PORT_LOG("selftest done\n");
#endif
}

/* Probes tied to the scripted input run (input_script.c): texture dumps
 * around the first race frames and a VRAM readback of one texture. */
void port_debug_frame_begin(u32 frame) {
#ifdef PORT_GFX_DEBUG
    extern int gfx_debug_frame;
    gfx_debug_frame = 0;
#endif
#ifdef PORT_INPUT_SCRIPT
    {
        extern int gfx_dump_textures;
        extern void gfx_debug_flush_texture_cache(void);
        gfx_dump_textures = (frame >= 955 && frame <= 975) || (frame >= 1300 && frame <= 1320);
        if (frame == 2039 && gPortHalfFrame == 0) { /* between frames only: never between the halves of a split frame */
            gfx_debug_flush_texture_cache();
        }
    }
#else
    (void) frame;
#endif
}

void port_debug_frame_end(u32 frame) {
#ifdef PORT_INPUT_SCRIPT
    if (frame == 1919) {
        extern void texman_debug_readback(unsigned int num, void* dst, unsigned int bytes);
        static u16 buf[512 * 2] __attribute__((aligned(64)));
        FILE* fp;
        texman_debug_readback(5, buf, 2048);
        fp = fopen(port_save_path("tex5_vram.bin"), "wb");
        if (fp) { fwrite(buf, 1, 2048, fp); fclose(fp); }
    }
    if (frame == 1321) {
        // Draw a captured 32x32 ground texture as a 2D sprite and as a tiled
        // 3D quad on top of the game frame.
        extern uint16_t gDebugTex32[32 * 32];
        extern int gDebugTex32Valid;
        struct GfxRenderingAPI* r = &gfx_opengl_api;
        if (gDebugTex32Valid) {
            struct { u16 u, v; u32 color; u16 x, y, z; } spr[2] = { { 0, 0, 0xFFFFFFFF, 300, 10, 0 }, { 32, 32, 0xFFFFFFFF, 364, 74, 0 } };
            struct { float u, v; u32 color; float x, y, z; } tri[6] = {
                { 0, 0, 0xFFFFFFFF, 0.2f, -0.9f, 0.5f }, { 4, 0, 0xFFFFFFFF, 0.9f, -0.9f, 0.5f }, { 4, 4, 0xFFFFFFFF, 0.9f, -0.2f, 0.5f },
                { 0, 0, 0xFFFFFFFF, 0.2f, -0.9f, 0.5f }, { 4, 4, 0xFFFFFFFF, 0.9f, -0.2f, 0.5f }, { 0, 4, 0xFFFFFFFF, 0.2f, -0.2f, 0.5f } };
            u32 id;
            r->start_frame();
            r->load_shader(r->create_and_load_new_shader(0x45));
            r->set_depth_test(false);
            r->set_use_alpha(false);
            r->set_viewport(0, 0, 480, 272);
            r->set_scissor(0, 0, 480, 272);
            id = r->new_texture();
            r->select_texture(0, id);
            r->upload_texture((const u8*) gDebugTex32, 32, 32, 1);
            r->set_sampler_parameters(0, false, 0, 0);
            gfx_scegu_draw_triangles_2d((float*) spr, 0, 1);
            sceGuSetMatrix(GU_PROJECTION, (const ScePspFMatrix4*) identity_matrix);
            sceGuSetMatrix(GU_VIEW, (const ScePspFMatrix4*) identity_matrix);
            sceGuSetMatrix(GU_MODEL, (const ScePspFMatrix4*) identity_matrix);
            r->draw_triangles((float*) tri, 0, 2);
            r->end_frame();
            r->finish_render();
            port_screenshot(1321);
            PORT_LOG("debug 32x32 blit done\n");
        }
    }
    if (frame == 2041) {
        // Blit the kart textures imported last frame as plain 2D sprites.
        extern uint16_t gDebugKartTex[2][64 * 32];
        extern int gDebugKartTexCount;
        struct GfxRenderingAPI* r = &gfx_opengl_api;
        int i;
        r->start_frame();
        r->load_shader(r->create_and_load_new_shader(0x45));
        r->set_depth_test(false);
        r->set_use_alpha(false);
        r->set_viewport(0, 0, 480, 272);
        r->set_scissor(0, 0, 480, 272);
        for (i = 0; i < gDebugKartTexCount; i++) {
            struct { u16 u, v; u32 color; u16 x, y, z; } spr[2] = { { 0, 0, 0xFFFFFFFF, 300, 10 + i * 70, 0 }, { 64, 32, 0xFFFFFFFF, 428, 74 + i * 70, 0 } };
            u32 id = r->new_texture();
            r->select_texture(0, id);
            r->upload_texture((const u8*) gDebugKartTex[i], 64, 32, 1 /* GU_PSM_5551 */);
            r->set_sampler_parameters(0, false, 0, 0);
            gfx_scegu_draw_triangles_2d((float*) spr, 0, 1);
        }
        r->end_frame();
        r->finish_render();
        port_screenshot(2041);
        PORT_LOG("kart blit done (%d textures)\n", gDebugKartTexCount);
    }
#endif
#ifdef PORT_GFX_DEBUG
    if (frame == 100 || (frame % 300) == 0) {
        port_screenshot((int) frame);
    }
#endif
#if !defined(PORT_INPUT_SCRIPT) && !defined(PORT_GFX_DEBUG)
    (void) frame;
#endif
}

#else /* !PORT_DEBUG_DUMPS: everything compiles to nothing */

void port_screenshot(UNUSED int index) {
}
void port_depthshot(UNUSED int index) {
}
void port_debug_selftest(void) {
}
void port_debug_frame_begin(UNUSED u32 frame) {
}
void port_debug_frame_end(UNUSED u32 frame) {
}

#endif
