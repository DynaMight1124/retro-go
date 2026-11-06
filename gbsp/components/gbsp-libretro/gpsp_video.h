#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern int gbsp_render_core1;
extern uint8_t gbsp_pal_dirty, gbsp_vram_dirty_any;
extern unsigned int gbsp_vram_dirty[3];

/* Configure before the ROM cache claims remaining PSRAM. False is a safe live
 * synchronous renderer, with no snapshot allocation or worker required. */
bool gbsp_render_configure(bool core1);
#if !RG_BUILD_RELEASE
int gbsp_render_stack_free(void);
#endif
void gbsp_render_sync(void);
void gbsp_render_wait(void);
void gbsp_render_invalidate(void);
void gbsp_vram_mark_all(void);
void gbsp_video_set_framebuffer(uint16_t *pixels);
void gbsp_display_poll(void);

static inline void gbsp_video_vram_write(uint32_t address)
{
    if (!gbsp_render_core1) return;
    uint32_t offset = address & 0x1FFFF;
    if (offset >= 0x18000) offset -= 0x8000;
    gbsp_vram_dirty[offset >> 15] |= 1u << ((offset >> 10) & 31);
    gbsp_vram_dirty_any = 1;
}

static inline void gbsp_video_vram_range(uint32_t a, uint32_t b)
{
    if (!gbsp_render_core1) return;
    if (b < a) { uint32_t t = a; a = b; b = t; }
    if (b - a >= 0x18000) { gbsp_vram_mark_all(); return; }
    for (uint32_t p = a & ~0x3FFu; p <= b; p += 0x400)
        gbsp_video_vram_write(p);
}
#ifdef __cplusplus
}
#endif
