/*
 * File: psp_texture_manager.c
 * Project: gfx
 * File Created: Friday, 7th August 2020 9:11:50 pm
 * Author: HaydenKow
 * -----
 * Copyright (c) 2020 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
#if defined(TARGET_PSP)
#include <PR/ultratypes.h>
#include "psp_texture_manager.h"
#include <string.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct PSP_Texture textures[512];
int texman_usage_percent(void);
extern int gfx_debug_frame;
extern void port_log(const char *fmt, ...);
static void *psp_tex_buffer = NULL;
static void *psp_tex_buffer_start = NULL;
static void *psp_tex_buffer_max = NULL;
/* Overflow arena in main RAM: the GE reads textures from RAM too (slower),
 * and menu frames need more than the VRAM arena holds. */
#define TEXMAN_RAM_SIZE (2 * 1024 * 1024) /* overflow arena (the shrunk N64 framebuffers and debug buffers pay for it on a PSP-1000) */
#define TEXMAN_NO_RAM 0
static unsigned char psp_tex_ram[TEXMAN_RAM_SIZE] __attribute__((aligned(64)));
static unsigned char *psp_tex_ram_ptr = psp_tex_ram;
static unsigned int psp_tex_number = 0;
unsigned int psp_tex_bound = 0;

static inline unsigned int getMemorySize(int width, int height, unsigned int psm) {
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

static inline unsigned int getTexWidthBytes(int width, unsigned int psm) {
    switch (psm) {
        case GU_PSM_T4:
            return (width >> 1);

        case GU_PSM_T8:
            return width;

        case GU_PSM_5650:
        case GU_PSM_5551:
        case GU_PSM_4444:
        case GU_PSM_T16:
            return 2 * width;

        case GU_PSM_8888:
        case GU_PSM_T32:
            return 4 * width;

        default:
            return 0;
    }
}

static void swizzle_fast(unsigned char *out, const unsigned char *in, unsigned int width,
                         unsigned int height) {
    unsigned int blockx, blocky;
    unsigned int j;

    unsigned int width_blocks = (width / 16);
    unsigned int height_blocks = (height / 8);

    unsigned int src_pitch = (width - 16) / 4;
    unsigned int src_row = width * 8;

    const unsigned char *ysrc = in;
    unsigned int *dst = (unsigned int *) out;

    for (blocky = 0; blocky < height_blocks; ++blocky) {
        const unsigned char *xsrc = ysrc;
        for (blockx = 0; blockx < width_blocks; ++blockx) {
            const unsigned int *src = (unsigned int *) xsrc;
            for (j = 0; j < 8; ++j) {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += src_pitch;
            }
            xsrc += 16;
        }
        ysrc += src_row;
    }
}

int texman_inited(void) {
    return psp_tex_buffer != 0;
}

void texman_reset(void *buf, unsigned int size) {
    memset(textures, 0, sizeof(textures));
    psp_tex_number = 0;
    psp_tex_buffer = psp_tex_buffer_start = buf;
    psp_tex_buffer_max = buf + size;
#ifdef DEBUG
    char msg[64];
    sprintf(msg, "TEXMAN reset @ %p size %d bytes\n", buf, size);
    sceIoWrite(1, msg, strlen(msg));
#endif
}

/* Counts arena resets: a texture id from before a reset means nothing after it
 * (the FPS overlay keeps one across frames). */
static unsigned int texman_gen;
unsigned int texman_generation(void) { return texman_gen; }

void texman_clear(void) {
    texman_gen++;
    memset(textures, 0, sizeof(textures));
    psp_tex_number = 0;
    psp_tex_bound = (unsigned int) -1; // nothing valid is bound any more
    psp_tex_buffer = psp_tex_buffer_start;
    psp_tex_ram_ptr = psp_tex_ram;
#ifdef DEBUG
    char msg[64];
    sprintf(msg, "TEXMAN clear %p size %d bytes!\n", psp_tex_buffer, TEXMAN_BUFFER_SIZE);
    sceIoWrite(1, msg, strlen(msg));
#endif
}

void texman_set_buffer(void *buf, unsigned int size) {
    psp_tex_buffer = buf;
    psp_tex_buffer_max = buf + size;
}

int gfx_vram_space_available(void) {
    return ((psp_tex_buffer_max - psp_tex_buffer) > (64 * 1024) || (!TEXMAN_NO_RAM && (psp_tex_ram + TEXMAN_RAM_SIZE - psp_tex_ram_ptr) > (64 * 1024))) &&
           texman_slots_available();
}

unsigned char *texman_get_tex_data(unsigned int num) {
    return textures[num].location;
}

unsigned char texman_get_tex_type(unsigned int num) {
    return textures[num].type;
}

/* Memory for the *bound* texture: uploads go to whichever texture is bound, so
 * a texture whose contents changed (the game rewrites its buffers in place)
 * can be re-uploaded into the space it already owns instead of eating more of
 * the arena. */
struct PSP_Texture *texman_reserve_memory(int width, int height, unsigned int type) {
    struct PSP_Texture *current = &textures[psp_tex_bound];
    unsigned int tex_size = (getMemorySize(width, height, type) + TEX_ALIGNMENT - 1) & ~(TEX_ALIGNMENT - 1);
    if (current->alloc_size >= tex_size && current->location != NULL) {
        return current;
    }
    if ((unsigned char *) psp_tex_buffer_max - (unsigned char *) psp_tex_buffer >= (int) tex_size) {
        current->location = psp_tex_buffer;
        psp_tex_buffer = (void *) ((unsigned char *) psp_tex_buffer + tex_size);
    } else if (!TEXMAN_NO_RAM && psp_tex_ram + TEXMAN_RAM_SIZE - psp_tex_ram_ptr >= (int) tex_size) {
        current->location = psp_tex_ram_ptr;
        psp_tex_ram_ptr += tex_size;
    } else {
        /* Out of memory.  The caller is expected to have checked
         * texman_can_hold() / gfx_vram_space_available() first; if it did not,
         * overwrite the START of the VRAM arena (a visible glitch on whatever
         * texture lived there) rather than write past the arena's end. */
        static int warned;
        if (!warned) { warned = 1; port_log("texman: arena overflow (%u bytes), reusing arena start\n", tex_size); }
        current->location = psp_tex_buffer_start;
    }
    current->alloc_size = tex_size;
    if (gfx_debug_frame) {
        port_log("  alloc tex %u (latest %u) loc %p size %u (arena %u%%)\n", psp_tex_bound, psp_tex_number, current->location, tex_size, (unsigned) texman_usage_percent());
    }
    return current;
}

/* Fraction of the arena (or of the texture slots) in use, in percent. */
int texman_usage_percent(void) {
    unsigned int used = ((unsigned char *) psp_tex_buffer - (unsigned char *) psp_tex_buffer_start) + (psp_tex_ram_ptr - psp_tex_ram);
    unsigned int total = ((unsigned char *) psp_tex_buffer_max - (unsigned char *) psp_tex_buffer_start) + (TEXMAN_NO_RAM ? 0 : TEXMAN_RAM_SIZE);
    unsigned int mem = total ? used * 100u / total : 100u;
    unsigned int slots = psp_tex_number * 100u / (sizeof(textures) / sizeof(textures[0]));
    return (int) (mem > slots ? mem : slots);
}

/* Can `bytes` be uploaded into texture `num` without growing past the arenas:
 * either its current allocation is big enough, or a fresh one fits. */
int texman_can_hold(unsigned int num, unsigned int bytes) {
    unsigned int tex_size = (bytes + TEX_ALIGNMENT - 1) & ~(TEX_ALIGNMENT - 1);
    if (num < sizeof(textures) / sizeof(textures[0]) && textures[num].location != NULL && textures[num].alloc_size >= tex_size) {
        return 1;
    }
    return ((unsigned char *) psp_tex_buffer_max - (unsigned char *) psp_tex_buffer) >= (int) tex_size ||
           (!TEXMAN_NO_RAM && psp_tex_ram + TEXMAN_RAM_SIZE - psp_tex_ram_ptr >= (int) tex_size);
}

int texman_slots_available(void) {
    return psp_tex_number < (sizeof(textures) / sizeof(textures[0])) - 2;
}

unsigned int texman_create(void) {
    psp_tex_number++;
    memset(&textures[psp_tex_number], 0, sizeof(textures[psp_tex_number]));
    psp_tex_bound = psp_tex_number;

#ifdef DEBUG
    printf("TEX_MAN new tex [%d] @ %x\n", psp_tex_number, psp_tex_buffer);
#endif
    return psp_tex_number;
}

void texman_upload_swizzle(int width, int height, unsigned int type, const void *buffer) {
    struct PSP_Texture *current = texman_reserve_memory(width, height, type);
    sceKernelDcacheWritebackRange(buffer, getMemorySize(width, height, type));
    current->width = width;
    current->height = height;
    current->pw = width;
    current->ph = height;
    current->tbw = width;
    current->type = type;
    /* 32bpp = 4 bytes, width is in bytes */
    swizzle_fast(current->location, buffer, getTexWidthBytes(width, type), height);
    current->swizzled = GU_TRUE;
#ifdef DEBUG
    printf("TEX_MAN upload swizzled [%d]\n", psp_tex_number);
#endif
    sceKernelDcacheWritebackRange(current->location, getMemorySize(width, height, type));
    sceKernelDcacheInvalidateRange(current->location, getMemorySize(width, height, type));
    texman_bind_tex(psp_tex_bound); // the texture we just wrote (may be a re-upload)
}

void texman_upload(int width, int height, unsigned int type, const void *buffer) {
    struct PSP_Texture *current = texman_reserve_memory(width, height, type);
    sceKernelDcacheWritebackRange(buffer, getMemorySize(width, height, type));
    current->width = width;
    current->height = height;
    current->pw = width;
    current->ph = height;
    current->tbw = width;
    current->type = type;
    current->swizzled = GU_FALSE;
    memcpy(current->location, buffer, getMemorySize(width, height, type));
#ifdef DEBUG
    // printf("TEX_MAN upload plain [%d]\n", psp_tex_number);
#endif
    sceKernelDcacheWritebackRange(current->location, getMemorySize(width, height, type));
    sceKernelDcacheInvalidateRange(current->location, getMemorySize(width, height, type));
    texman_bind_tex(psp_tex_bound); // the texture we just wrote (may be a re-upload)
}

static int next_pow2(int v) {
    int p = 1;
    while (p < v) {
        p <<= 1;
    }
    return p;
}

/* Textures the GE cannot take directly (non power-of-two, or too small for
 * 16x8 swizzle blocks - e.g. MK64's 320x3 background strips): copy the rows
 * into a power-of-two padded buffer and let sceGuTexScale map the real size. */
void texman_upload_pitched(int width, int height, unsigned int type, const void *buffer) {
    int pw = next_pow2(width);
    int ph = next_pow2(height);
    struct PSP_Texture *current = texman_reserve_memory(pw, ph, type);
    unsigned int src_row = getTexWidthBytes(width, type);
    unsigned int dst_row = getTexWidthBytes(pw, type);
    const unsigned char *src = buffer;
    unsigned char *dst = current->location;
    int y;

    current->width = width;
    current->height = height;
    current->pw = pw;
    current->ph = ph;
    current->tbw = pw;
    current->type = type;
    current->swizzled = GU_FALSE;
    for (y = 0; y < height; y++) {
        memcpy(dst + y * dst_row, src + y * src_row, src_row);
        /* replicate the last texel across the padding so edge sampling stays clean */
        if (dst_row > src_row) {
            unsigned int bpp = getTexWidthBytes(1, type);
            unsigned int x;
            if (bpp == 0) { /* T4: two texels per byte */
                memset(dst + y * dst_row + src_row, dst[y * dst_row + src_row - 1], dst_row - src_row);
            } else {
                for (x = src_row; x < dst_row; x += bpp) {
                    memcpy(dst + y * dst_row + x, dst + y * dst_row + src_row - bpp, bpp);
                }
            }
        }
    }
    for (y = height; y < ph; y++) {
        memcpy(dst + y * dst_row, dst + (height - 1) * dst_row, dst_row);
    }
    sceKernelDcacheWritebackRange(dst, getMemorySize(pw, ph, type));
    texman_bind_tex(psp_tex_bound); // the texture we just wrote (may be a re-upload)
}

void texman_bind_tex(unsigned int num) {
    const struct PSP_Texture *current;
    if (num >= sizeof(textures) / sizeof(textures[0])) {
        return;
    }
    current = &textures[num];
    psp_tex_bound = num;
    if (current->location == NULL) {
        return; // created but not uploaded yet
    }
    if (gfx_debug_frame) {
        port_log("  bind tex %u loc %p %dx%d (pw %d ph %d tbw %d) type %u swz %u alloc %u\n", num, current->location, current->width, current->height, current->pw, current->ph, current->tbw, current->type, current->swizzled, current->alloc_size);
    }
#ifdef DEBUG
    /* Note this will SPAM if you enable */
    // if (psp_tex_bound != num)
    //    printf("TEX_MAN bind tex [%d]\n", num);
#endif
    sceGuTexMode(current->type, 0, 0, current->swizzled);
    sceGuTexImage(0, current->pw, current->ph, current->tbw, current->location);
    // Normalised (3D) texture coordinates address the real image size.
    sceGuTexScale((float) current->width / (float) current->pw, (float) current->height / (float) current->ph);
    psp_tex_bound = num;
}

unsigned int texman_get_bound(void) {
    return psp_tex_bound;
}
#endif

/* Debug: copy a texture's VRAM contents back to RAM with the GE. */
void texman_debug_readback(unsigned int num, void *dst, unsigned int bytes) {
    extern unsigned int list[];
    const struct PSP_Texture *t = &textures[num];
    if (t->location == NULL) return;
    sceGuStart(GU_DIRECT, list);
    // copy as a 16-bit image, 8 pixels per 16 bytes: width = bytes / 2 pixels in rows of 512
    {
        unsigned int px = bytes / 2;
        unsigned int h = (px + 511) / 512;
        sceGuCopyImage(GU_PSM_5551, 0, 0, 512, h, 512, t->location, 0, 0, 512, dst);
    }
    sceGuFinish();
    sceGuSync(0, 0);
    sceKernelDcacheInvalidateRange(dst, bytes);
    port_log("readback tex %u loc %p %dx%d swz %u alloc %u\n", num, t->location, t->width, t->height, t->swizzled, t->alloc_size);
}
