#pragma once
#include <rg_system.h>
#include <stdio.h>
#include "gbsp_frameskip_controller.h"

static gp_fs_t gp_frameskip;
static int frameskip_opt = -1;
static uint32_t gp_frameskip_budget;
static int64_t gp_frameskip_second;

static void gp_frameskip_resume(void)
{
    uint32_t budget = app->frameTime > 0 ? (uint32_t)app->frameTime : 16666;
    if (budget != gp_frameskip_budget) {
        gp_fs_init(&gp_frameskip, gp_frameskip.profile, frameskip_opt);
        gp_frameskip_budget = budget;
    }
    gp_fs_clear_window(&gp_frameskip);
    gp_frameskip_second = rg_system_timer();
    skip_next_frame = 0;
    app->frameskip = -1; /* One owner: disable the platform's auto controller. */
}

static void gp_frameskip_start(void)
{
#if defined(RISCV_ARCH)
    int backend = GP_FS_P4;
#elif defined(XTENSA_ARCH)
    int backend = GP_FS_S3;
#else
    int backend = GP_FS_INTERPRETER;
#endif
    gp_fs_init(&gp_frameskip, gp_fs_profile(backend),
        rg_settings_get_number(NS_APP, "frameskip", -1));
    frameskip_opt = gp_frameskip.mode;
    gp_frameskip_resume();
    RG_LOGI("frameskip: %s (level %d, limit %d)", frameskip_opt < 0 ? "Auto" : "Fixed",
        gp_frameskip.level, gp_frameskip.profile.max_level);
}

static void gp_frameskip_pause(void)
{
    gp_fs_clear_window(&gp_frameskip);
    app->frameskip = gp_frameskip.level; /* Menu status bars. */
}

/* Runtime S3 JIT recovery must receive the same tuning as a built interpreter. */
static inline void gp_frameskip_select_backend(int backend)
{
    gp_fs_profile_t profile = gp_fs_profile(backend);
    if (profile.max_level != gp_frameskip.profile.max_level ||
        profile.raise_gain != gp_frameskip.profile.raise_gain) {
        /* Startup may have clamped a fixed interpreter preference while the
         * JIT profile was selected. Recover it once the actual backend is known. */
        gp_fs_init(&gp_frameskip, profile,
            rg_settings_get_number(NS_APP, "frameskip", frameskip_opt));
        frameskip_opt = gp_frameskip.mode;
        gp_frameskip_resume();
    }
}

static rg_gui_event_t gbsp_frameskip_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    int next = frameskip_opt;
    if (event == RG_DIALOG_PREV && next > -1) next--;
    if (event == RG_DIALOG_NEXT && next < gp_frameskip.profile.max_level) next++;
    if (next != frameskip_opt) {
        frameskip_opt = next;
        rg_settings_set_number(NS_APP, "frameskip", next);
        gp_fs_init(&gp_frameskip, gp_frameskip.profile, next);
        skip_next_frame = 0;
        app->frameskip = gp_frameskip.level;
    }
    if (frameskip_opt < 0) strcpy(option->value, _("Auto"));
    else if (!frameskip_opt) strcpy(option->value, _("0 (off)"));
    else sprintf(option->value, "%d of %d", frameskip_opt, frameskip_opt + 1);
    return RG_DIALOG_VOID;
}

static void gp_frameskip_record(uint32_t us, bool drawn)
{
    gp_fs_record(&gp_frameskip, us, drawn);
}

static void gp_frameskip_end_frame(void)
{
    if (app->frameTime > 0 && gp_frameskip_budget != (uint32_t)app->frameTime)
        gp_frameskip_resume();
    int64_t now = rg_system_timer();
    if (now - gp_frameskip_second >= 1000000) {
        int previous = gp_frameskip.level;
        (void)previous; /* RG_LOGD is compiled out of release builds. */
        if (gp_fs_update(&gp_frameskip, gp_frameskip_budget))
            RG_LOGD("frameskip: %d -> %d (draw %u us, skip %u us, budget %u us)",
                previous, gp_frameskip.level, (unsigned)gp_frameskip.last_drawn,
                (unsigned)gp_frameskip.last_skipped, (unsigned)gp_frameskip_budget);
        gp_frameskip_second = now;
    }
    skip_next_frame = gp_fs_next(&gp_frameskip, skip_next_frame);
    app->frameskip = -1;
}
