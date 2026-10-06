#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <ultra64.h>

void mk64_rg_gfx_begin(void);
void mk64_rg_gfx_walk(Gfx *list, size_t count, unsigned frame);
bool mk64_rg_gfx_present(unsigned frame);
bool mk64_rg_gfx_screenshot(const char *filename, int width, int height);
void mk64_rg_gfx_redraw(void);
void mk64_rg_gfx_capture(int x, int y, int width, int height, void *target);
typedef struct { unsigned copies, us, max_us; } Mk64CaptureStats;
Mk64CaptureStats mk64_rg_gfx_capture_stats(void);

/* Adapter-only RDP no-op tag: switch the current frame to native UI output. */
#define MK64_RG_UI_TAG 0x52475549u /* RGUI */

void mk64_rg_set_render_enabled(bool enabled);
