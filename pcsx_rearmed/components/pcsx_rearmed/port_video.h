#ifndef PCSX_PORT_VIDEO_H
#define PCSX_PORT_VIDEO_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static inline uint16_t psx_rgb555_to_rgb565(uint16_t c) {
    /* PSX VRAM stores R, G and B in bits 0..4, 5..9 and 10..14. Expand
     * green from five to six bits; leaving RGB565 bit 10 clear gives every
     * frame a visible magenta cast. */
    uint16_t green = ((c & 0x03E0) << 1) | ((c & 0x0200) >> 4);
    return ((c & 0x001F) << 11) | green | ((c & 0x7C00) >> 10);
}

static inline uint32_t psx_rgb555x2_to_rgb565(uint32_t c) {
    /* Convert two little-endian pixels in parallel. Each mask keeps shifts
     * within its own 16-bit lane, so this is bit-identical to the scalar
     * conversion above. */
    return ((c & 0x001F001Fu) << 11) |
           ((c & 0x03E003E0u) << 1) |
           ((c & 0x02000200u) >> 4) |
           ((c & 0x7C007C00u) >> 10);
}

static inline uint16_t psx_bgr888_to_rgb565(const uint8_t *c) {
    return ((uint16_t)(c[0] & 0xF8) << 8) |
           ((uint16_t)(c[1] & 0xFC) << 3) | (c[2] >> 3);
}

/* Source VRAM rows are 2048 bytes; destination stride is in pixels. */
static inline void pcsx_port_blit(uint16_t *dst, int stride,
    int target_w, int target_h, const void *vram, int vram_offset, int bgr24,
    int x, int y, int w, int h, int output_width, int output_height)
{
    const uint8_t *src_base = (const uint8_t *)vram + vram_offset;

    // Stride is always 1024 for PS1 VRAM
    if (output_width != target_w || output_height != target_h ||
        x != 0 || y != 0 || w != target_w || h != target_h) {
        /* Map the full output mode, not just the active rectangle.
         * x/y are destination borders; vram_offset already selects
         * the source origin. Never use active width as surface stride. */
        int step_x = (output_width << 16) / target_w;
        int step_y = (output_height << 16) / target_h;

        for (int j = 0; j < target_h; j++) {
            int src_y = ((j * step_y) >> 16) - y;
            uint16_t *line_dst = dst + j * stride;
            if (src_y < 0 || src_y >= h) {
                memset(line_dst, 0, (size_t)target_w * 2);
                continue;
            }
            const uint8_t *line_src = src_base + src_y * 2048;
            for (int i = 0; i < target_w; i++) {
                int src_x = ((i * step_x) >> 16) - x;
                if (src_x < 0 || src_x >= w) {
                    line_dst[i] = 0;
                    continue;
                }
                line_dst[i] = bgr24
                    ? psx_bgr888_to_rgb565(line_src + src_x * 3)
                    : psx_rgb555_to_rgb565(
                        ((const uint16_t *)line_src)[src_x]);
            }
        }
    } else {
        for (int j = 0; j < h; j++) {
            const uint8_t *line_src = src_base + (j * 2048);
            uint16_t *line_dst = dst + j * stride;
            if (!bgr24 &&
                ((((uintptr_t)line_src | (uintptr_t)line_dst) & 3u) == 0)) {
                const uint32_t *src32 = (const uint32_t *)line_src;
                uint32_t *dst32 = (uint32_t *)line_dst;
                int pairs = w >> 1;

                for (int i = 0; i < pairs; i++)
                    dst32[i] = psx_rgb555x2_to_rgb565(src32[i]);
                if (w & 1)
                    line_dst[w - 1] = psx_rgb555_to_rgb565(
                        ((const uint16_t *)line_src)[w - 1]);
            } else {
                for (int i = 0; i < w; i++) {
                    line_dst[i] = bgr24
                        ? psx_bgr888_to_rgb565(line_src + i * 3)
                        : psx_rgb555_to_rgb565(
                            ((const uint16_t *)line_src)[i]);
                }
            }
        }
    }
}
#endif
