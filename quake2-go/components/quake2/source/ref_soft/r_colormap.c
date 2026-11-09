#include "r_colormap.h"
#include <rg_utils.h>
#include "esp_memory_utils.h"
#include <stdlib.h>
#include <string.h>

bool q2_colormap_init(q2_colormap_t *map, unsigned char *source)
{
    map->source = map->lighting = source;
    map->alpha = source + Q2_LIGHTING_BYTES;
    unsigned char *copy = rg_alloc(Q2_LIGHTING_BYTES, MEM_FAST | MEM_NOPANIC);
    // MEM_FAST is a preference: reject a PSRAM fallback copy.
    if (copy && esp_ptr_internal(copy)) {
        memcpy(copy, source, Q2_LIGHTING_BYTES);
        map->lighting = copy;
        return true;
    }
    free(copy);
    return false;
}

void q2_colormap_free(q2_colormap_t *map)
{
    if (map->lighting != map->source) free(map->lighting);
    free(map->source);
    memset(map, 0, sizeof(*map));
}
