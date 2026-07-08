#ifndef GFX_PC_H
#define GFX_PC_H

#include <stdbool.h>
#include <stdint.h>

struct GfxRenderingAPI;
struct GfxWindowManagerAPI;

struct GfxDimensions {
    uint32_t width, height;
#ifdef GBI_FLOATS
    float aspect_ratio;
#else
    int32_t aspect_ratio;
#endif
};

extern struct GfxDimensions gfx_current_dimensions;

#ifdef __cplusplus
extern "C" {
#endif

void gfx_init(struct GfxWindowManagerAPI *wapi, struct GfxRenderingAPI *rapi, const char *game_name, bool start_in_fullscreen);
void sm64_init_gfx_pc(void);
void gfx_shutdown(void);
struct GfxRenderingAPI *gfx_get_current_rendering_api(void);
void gfx_start_frame(void);
void gfx_run(Gfx *commands);
void gfx_end_frame(void);
void gfx_update_dimensions(void);

// Per-frame debug counters (read & reset)
int gfx_get_debug_flush_count(void);
int gfx_get_debug_tri_count(void);
int gfx_get_debug_tex_count(void);
void gfx_debug_arm_menu_model(int shared_type, int pos_x, int pos_y, int pos_z,
                              int scale_x, int scale_y, int scale_z);
void gfx_debug_arm_character_model(int shared_type, int row_x, int row_y, int row_z);

#ifdef __cplusplus
}
#endif

#endif
