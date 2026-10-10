#include "adapter.h"
#include "port_logic.h"
#include <string.h>

static rg_surface_t *surfaces[2], *last;
static rg_mutex_t *mutex;
static uint16_t palette[256];
static unsigned next;
static bool enabled = true;
static int wait_us;

void q2_video_init(void)
{
    mutex = rg_mutex_create();
    RG_ASSERT(mutex, "Video mutex allocation failed");
    for (int i = 0; i < 2; ++i)
        surfaces[i] = rg_surface_create(Q2_WIDTH, Q2_HEIGHT, RG_PIXEL_PAL565_BE, MEM_SLOW);
}

void QG_SetPalette(unsigned char rgb[768])
{
    rg_mutex_take(mutex, -1);
    for (int i = 0; i < 256; ++i)
        palette[i] = q2_rgb565_be(rgb[i*3], rgb[i*3+1], rgb[i*3+2]);
    rg_mutex_give(mutex);
}

void QG_DrawFrame(void *pixels)
{
    if (!enabled) return;
    rg_mutex_take(mutex, -1);
    rg_surface_t *frame = surfaces[next];
    // The preceding blocking submission released this alternate surface.
    // Keep the engine's incremental framebuffer separate from queued snapshots.
    memcpy(frame->data, pixels, Q2_WIDTH * Q2_HEIGHT);
    memcpy(frame->palette, palette, sizeof(palette));
    int64_t start = rg_system_timer();
    rg_display_submit(frame, 0);
    wait_us += (int)(rg_system_timer() - start);
    last = frame;
    next ^= 1;
    rg_mutex_give(mutex);
}

void q2_video_redraw(void)
{
    if (!mutex) return;
    rg_mutex_take(mutex, -1);
    if (last) rg_display_submit(last, 0);
    rg_mutex_give(mutex);
}

bool q2_video_screenshot(const char *filename, int width, int height)
{
    rg_mutex_take(mutex, -1);
    bool ok = last && rg_surface_save_image_file(last, filename, width, height);
    rg_mutex_give(mutex);
    return ok;
}

void q2_video_enable(bool value) { enabled = value; }
int QG_RenderEnabled(void) { return enabled; }
int q2_video_take_wait(void) { int result = wait_us; wait_us = 0; return result; }
