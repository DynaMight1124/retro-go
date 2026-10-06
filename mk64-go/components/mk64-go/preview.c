#include "preview.h"

#include <stddef.h>
#include <rg_display.h>
#include <rg_system.h>

#include <stdint.h>

#include "recipe_index.h"
#include "resident.h"

#define PREVIEW_WIDTH 320
#define PREVIEW_HEIGHT 240

typedef struct {
    int16_t x, y, z, s, t;
    uint8_t color[4];
} course_vertex_t;

_Static_assert(sizeof(course_vertex_t) == 14, "Unexpected course vertex layout");

extern const course_vertex_t d_course_mario_raceway_vertex[];

static size_t vertex_count(void)
{
    uint32_t asset_offset = (uintptr_t)d_course_mario_raceway_vertex -
                            (uintptr_t)gMk64AssetRegion;
    for (size_t i = 0; i < mk64_recipe_count; ++i) {
        if (mk64_recipes[i].dst == asset_offset && mk64_recipes[i].kind == 2 &&
            (mk64_recipes[i].size + 1) % sizeof(course_vertex_t) == 0)
            return (mk64_recipes[i].size + 1) / sizeof(course_vertex_t);
    }
    return 0;
}

bool mk64_preview_show(void)
{
    size_t count = vertex_count();
    if (!count)
        return false;

    int min_x = INT16_MAX, max_x = INT16_MIN;
    int min_z = INT16_MAX, max_z = INT16_MIN;
    for (size_t i = 0; i < count; ++i) {
        const course_vertex_t *v = &d_course_mario_raceway_vertex[i];
        if (v->x < min_x) min_x = v->x;
        if (v->x > max_x) max_x = v->x;
        if (v->z < min_z) min_z = v->z;
        if (v->z > max_z) max_z = v->z;
    }
    RG_LOGI("MK64: Mario Raceway vertex data: %u vertices, x %d..%d z %d..%d",
            (unsigned)count, min_x, max_x, min_z, max_z);
    if (count != 5757 || min_x != -1479 || max_x != 4325 ||
        min_z != -1949 || max_z != 2721)
        return false;

    /* Display submission is asynchronous; retain this surface until app exit. */
    static rg_surface_t *surface;
    surface = rg_surface_create(PREVIEW_WIDTH, PREVIEW_HEIGHT,
                                RG_PIXEL_565_LE, MEM_SLOW);
    if (!surface)
        return false;
    rg_surface_fill(surface, NULL, 0x0841);
    uint16_t *pixels = surface->data;
    size_t plotted = 0;
    for (size_t i = 0; i < count; ++i) {
        const course_vertex_t *v = &d_course_mario_raceway_vertex[i];
        int px = 10 + (int32_t)(v->x - min_x) * (PREVIEW_WIDTH - 21) /
                      (max_x - min_x);
        int py = PREVIEW_HEIGHT - 11 -
                 (int32_t)(v->z - min_z) * (PREVIEW_HEIGHT - 21) /
                 (max_z - min_z);
        if (px >= 0 && px < PREVIEW_WIDTH && py >= 0 && py < PREVIEW_HEIGHT) {
            pixels[py * PREVIEW_WIDTH + px] = 0xffff;
            ++plotted;
        }
    }
    rg_display_submit(surface, 0);
    RG_LOGI("MK64: Mario Raceway vertex preview submitted (%u points)",
            (unsigned)plotted);
    return true;
}
