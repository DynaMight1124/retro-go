#pragma once
#include <stdbool.h>
#define Q2_LIGHTING_BYTES (64u * 256u)
typedef struct {
    unsigned char *source, *lighting, *alpha;
} q2_colormap_t;
// Takes ownership of the loaded PCX pixels. Alpha always stays in source.
bool q2_colormap_init(q2_colormap_t *map, unsigned char *source);
void q2_colormap_free(q2_colormap_t *map);
