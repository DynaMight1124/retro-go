#pragma once
#include "gpsp_video.h"
#include "gbsp_video_settings.h"
#include "gbsp_frame_pool.h"

static rg_surface_t *updates[3], *currentUpdate, *shownUpdate;
static gp_frame_pool_t gp_frames;

static void gp_display_bind(void)
{
    currentUpdate = updates[gp_frames.draw];
    gbsp_video_set_framebuffer((uint16_t *)currentUpdate->data);
}

static void gp_display_init(void)
{
    int count = 0;
    for (int i = 0; i < gp_video_buffers; i++) {
#if defined(XTENSA_ARCH)
        uint32_t caps = MEM_SLOW;
#else
        uint32_t caps = i < 2 ? MEM_FAST : MEM_SLOW;
#endif
        updates[i] = rg_surface_create(240, 161, RG_PIXEL_565_LE, caps | MEM_NOPANIC);
        if (!updates[i]) break;
        updates[i]->height = 160;
        count++;
    }
    RG_ASSERT(count, "Could not allocate a GBA video buffer");
    if (count != gp_video_buffers)
        RG_LOGW("video: requested %d buffers, using %d", gp_video_buffers, count);
    gp_video_buffers_active = count;
    gp_frame_pool_init(&gp_frames, count);
    shownUpdate = updates[0];
    gp_display_bind();
}

static void gp_display_configure_renderer(void)
{
    gp_video_renderer_active = gbsp_render_configure(gp_video_renderer != 0);
    RG_LOGI("video: renderer %s, buffers %d (requested %s/%d)",
            gp_video_renderer_active ? "Core 1" : "Sync", gp_frames.count,
            gp_video_renderer ? "Core 1" : "Sync", gp_video_buffers);
}

/* Runs only on the emulation/UI thread, including scanline/audio wait polls. */
void gbsp_display_poll(void)
{
    if (!gp_frames.count) return;
    if (gp_frames.pending >= 0 && !rg_display_is_busy()) {
        rg_display_submit(updates[gp_frames.pending], 0);
        gp_frame_pool_submit(&gp_frames);
        shownUpdate = updates[gp_frames.shown];
    }
}

static void gp_display_begin(void)
{
    gbsp_display_poll();
    if (gp_frames.count > 1 && gp_frames.draw == gp_frames.submitted)
        gp_frames.draw = -1; /* startup/menu redraw may have submitted buffer 0 */
    for (;;) {
        bool busy = rg_display_is_busy();
        if (!(gp_frames.count == 1 && busy) && gp_frame_pool_next(&gp_frames, busy) >= 0) break;
        rg_task_yield();
        gbsp_display_poll();
    }
    gp_display_bind();
}

static bool gp_display_finish(void)
{
    bool was_busy = rg_display_is_busy();
    /* Worker completion is a separate lifetime from display completion. */
    gbsp_render_wait();
    gp_frame_pool_publish(&gp_frames);
    for (;;) {
        gbsp_display_poll();
        if (gp_frame_pool_next(&gp_frames, rg_display_is_busy()) >= 0) break;
        rg_task_yield();
    }
    gp_display_bind();
    return was_busy;
}

static void gp_display_quiesce(void)
{
    if (!gp_frames.count) return; /* system callbacks can precede video init */
    gbsp_render_wait();
    do {
        gbsp_display_poll();
        if (gp_frames.pending < 0 && !rg_display_is_busy()) break;
        rg_task_yield();
    } while (true);
    rg_display_sync();
}

static void gp_display_redraw(void)
{
    gp_display_quiesce();
    if (shownUpdate) {
        rg_display_submit(shownUpdate, 0);
        gp_frames.submitted = gp_frames.shown;
    }
}

static bool gp_display_screenshot(const char *filename, int width, int height)
{
    gp_display_quiesce();
    if (!shownUpdate) return false;
    return gp_screenshot_save(shownUpdate, filename, width, height);
}

static void gp_display_apply_pending(void)
{
    if (!gp_video_restart_requested) return;
    gp_video_restart_requested = false;
    if (gp_video_settings_pending() &&
        rg_gui_confirm(_("Apply video settings?"), _("Restart game to apply renderer and buffering changes?"), true)) {
        gp_display_quiesce();
        gp_save_flush_sync();
        rg_system_restart();
    }
}
