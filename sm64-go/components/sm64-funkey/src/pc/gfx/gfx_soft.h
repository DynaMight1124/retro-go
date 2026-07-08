#ifndef GFX_SOFT_H
#define GFX_SOFT_H

#include "gfx_rendering_api.h"
#include <stdbool.h>

// For some strange reasons, trying to use SDL to convert the surface
// on the Funkey results in a bus error and crash...

//#define SDL_SURFACE
//#define DIRECT_SDL

extern struct GfxRenderingAPI gfx_soft_api;

#include <stdint.h>
extern uint32_t *gfx_output;
extern uint32_t *gfx_overlay_output;
extern bool gfx_overlay_active;

void gfx_soft_overlay_tex_rect(int x0, int y0, int x1, int y1,
                               float u0, float v0, float dudx, float dvdy,
                               const uint8_t *rgba);
void gfx_soft_overlay_textured_tri(float x0, float y0, float u0, float v0,
                                   float x1, float y1, float u1, float v1,
                                   float x2, float y2, float u2, float v2,
                                   const uint8_t *rgba);

int gfx_soft_get_debug_tri_pixels(void);
int gfx_soft_get_debug_rect_pixels(void);
int gfx_soft_get_debug_fill_pixels(void);

#if CONFIG_IDF_TARGET_ESP32S3
void gfx_soft_reset_texture_cache(void);
#endif

#endif
