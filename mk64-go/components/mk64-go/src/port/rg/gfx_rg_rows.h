#ifndef MK64_RG_ROWS_H
#define MK64_RG_ROWS_H

enum { PIX_DEPTH, PIX_COORD, PIX_FETCH, PIX_SHADE, PIX_STAGE_COUNT };
typedef struct { uint32_t cycles; unsigned samples; } RGPixelProbe;

enum { ATTR_Z, ATTR_ALPHA, ATTR_RED, ATTR_GREEN, ATTR_BLUE,
       ATTR_Q, ATTR_U, ATTR_V, ATTR_COUNT };

typedef struct {
    const float *x, *y, *slopes;
    const float (*attributes)[3];
    const float *attribute_dx;
    float area, inverse_area;
    int left, right, top, bottom;
    unsigned constant_attributes, red, green, blue, combiner_mode;
    bool depth_test, depth_write;
    uint16_t *pixels, *depth;
} RGRasterRows;

#ifdef __GNUC__
#define MK64_RASTER_INLINE static inline __attribute__((always_inline))
#else
#define MK64_RASTER_INLINE static inline
#endif

#endif
