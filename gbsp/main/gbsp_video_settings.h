#pragma once
#include <rg_system.h>
#include <stdio.h>
#include <string.h>

static int gp_video_renderer, gp_video_buffers;
static int gp_video_renderer_boot, gp_video_buffers_boot;
static int gp_video_renderer_active, gp_video_buffers_active;
static bool gp_video_restart_requested;

static void gp_video_settings_load(void)
{
#if defined(XTENSA_ARCH)
    const int default_renderer = 1, default_buffers = 2;
#else
#if defined(RISCV_ARCH)
    const int default_renderer = 1;
#else
    const int default_renderer = 0;
#endif
    const int default_buffers = rg_settings_get_number(NS_APP, "gba_frame_double_buffering", true) ? 2 : 1;
#endif
    gp_video_renderer = rg_settings_get_number(NS_APP, "render", default_renderer);
    if (gp_video_renderer < 0 || gp_video_renderer > 1) gp_video_renderer = default_renderer;
    gp_video_buffers = rg_settings_get_number(NS_APP, "video_buffers", default_buffers);
    if (gp_video_buffers < 1 || gp_video_buffers > 3) gp_video_buffers = default_buffers;
    gp_video_renderer_boot = gp_video_renderer_active = gp_video_renderer;
    gp_video_buffers_boot = gp_video_buffers_active = gp_video_buffers;
    gp_video_restart_requested = false;
}

static bool gp_video_settings_pending(void)
{
    return gp_video_renderer != gp_video_renderer_boot || gp_video_buffers != gp_video_buffers_boot;
}

static rg_gui_event_t gp_video_renderer_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        gp_video_renderer = !gp_video_renderer;
        rg_settings_set_number(NS_APP, "render", gp_video_renderer);
        gp_video_restart_requested = true;
    }
    option->label = gp_video_renderer != gp_video_renderer_boot ? _("Renderer (restart)") : _("Renderer");
    const char *value = gp_video_renderer ? _("Core 1") : _("Sync");
    if (!gp_video_settings_pending() && gp_video_renderer && !gp_video_renderer_active)
        value = _("Core 1 (unavailable)");
    strcpy(option->value, value);
    return RG_DIALOG_VOID;
}

static rg_gui_event_t gp_video_buffers_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        gp_video_buffers += event == RG_DIALOG_NEXT ? 1 : -1;
        if (gp_video_buffers > 3) gp_video_buffers = 1;
        if (gp_video_buffers < 1) gp_video_buffers = 3;
        rg_settings_set_number(NS_APP, "video_buffers", gp_video_buffers);
        gp_video_restart_requested = true;
    }
    option->label = gp_video_buffers != gp_video_buffers_boot ? _("Buffering (restart)") : _("Buffering");
    const char *value = gp_video_buffers == 3 ? _("Triple") : gp_video_buffers == 2 ? _("Double") : _("Single");
    if (!gp_video_settings_pending() && gp_video_buffers_active != gp_video_buffers)
        sprintf(option->value, "%s (%d active)", value, gp_video_buffers_active);
    else
        strcpy(option->value, value);
    return RG_DIALOG_VOID;
}
