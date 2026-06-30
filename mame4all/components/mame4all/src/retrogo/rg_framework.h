#ifdef __cplusplus
extern "C" {
#endif

#ifndef RG_FRAMEWORK_H
#define RG_FRAMEWORK_H

#include "retrogo/rg_psram.h"
#include <stdint.h>

struct rg_framework_psram_struct {
    uint32_t L_screen_line_checksum[1024];
    uint16_t L_map_viewport_to_source_x[1024];
    uint16_t L_map_viewport_to_source_y[1024];
    uint8_t L_area_dirty[1024];
};

#define rg_screen_line_checksum     (rg_psram->ptr_framework->L_screen_line_checksum)
#define rg_map_viewport_to_source_x (rg_psram->ptr_framework->L_map_viewport_to_source_x)
#define rg_map_viewport_to_source_y (rg_psram->ptr_framework->L_map_viewport_to_source_y)
#define rg_area_dirty               (rg_psram->ptr_framework->L_area_dirty)

static void check_framework_psram(void)
{
    if (!rg_psram->ptr_framework)
    {
        rg_psram->ptr_framework = (struct rg_framework_psram_struct *)mame_alloc_slow(sizeof(struct rg_framework_psram_struct));
        memset(rg_psram->ptr_framework, 0, sizeof(struct rg_framework_psram_struct));
    }
}

#endif

#ifdef __cplusplus
}
#endif
