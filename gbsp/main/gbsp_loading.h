#pragma once
#include "gpsp_load.h"

/* rg_system_init() leaves its hourglass alone in the middle of a blank screen
   and filling the ROM cache begins right after it. A 4 MB game takes several
   seconds off the card, so that lone icon sat there for the whole load and read
   as an opaque white box that had taken over the display. Replace it with a
   loading screen: which game, and a bar that moves. These are the same
   direct-to-panel calls the hourglass itself uses, because there is no surface
   to submit yet. */
static void load_progress_cb(int percent)
{
    static bool screen_drawn;
    static int box_w = 200, bar_x, bar_y, bar_w, bar_h;
    char line[48];
    const char *name = (app && app->romPath) ? strrchr(app->romPath, '/') : NULL;
    name = name ? name + 1 : "";

    if (!screen_drawn)
    {
        const int panel_w = rg_display_get_width();
        const int panel_h = rg_display_get_height();
        /* A fixed box width keeps the text inside the panel whatever the font;
           rg_gui_draw_text() clips to it instead of running off the edge. */
        size_t cap;

        box_w = (panel_w > 64) ? panel_w - 24 : 40;
        cap = (size_t)(box_w / 8);      /* the font is at most 8 px a character */
        if (cap > 30)
            cap = 30;

        screen_drawn = true;
        rg_display_clear(C_BLACK);

        if (name[0])
        {
            if (strlen(name) > cap && cap > 3)
                snprintf(line, sizeof(line), "%.*s...", (int)(cap - 3), name);
            else
                snprintf(line, sizeof(line), "%.*s", (int)cap, name);
            rg_gui_draw_text(RG_GUI_CENTER, RG_GUI_TOP + 10, box_w, line,
                             C_WHITE, C_BLACK, RG_TEXT_ALIGN_CENTER);
        }

        /* The bar takes the hourglass's place in the middle of the panel. */
        bar_w = (panel_w > 88) ? panel_w / 2 : panel_w - 40;
        bar_h = 12;
        bar_x = (panel_w - bar_w) / 2;
        bar_y = (panel_h - bar_h) / 2;
        rg_gui_draw_rect(bar_x, bar_y, bar_w, bar_h, 1, C_WHITE, C_BLACK);
    }

    snprintf(line, sizeof(line), "Loading %d%%", percent);
    rg_gui_draw_text(RG_GUI_CENTER, bar_y + bar_h + 10, box_w, line,
                     C_WHITE, C_BLACK, RG_TEXT_ALIGN_CENTER);

    if (percent > 0)
        rg_gui_draw_rect(bar_x + 1, bar_y + 1, (bar_w - 2) * percent / 100, bar_h - 2,
                         0, C_WHITE, C_WHITE);
}


static void gp_load_begin(void)
{
    gp_display_quiesce();
    gamepak_load_progress = load_progress_cb;
    load_progress_cb(0);
}

static void gp_load_finish(void)
{
    load_progress_cb(100);
    gamepak_load_progress = NULL;
    RG_LOGI("RTC: %s", gpsp_rtc_is_enabled() ? "enabled" : "disabled");
    gp_save_init(app->romPath);
    gp_state_recover_slots();
    if (app->bootFlags & RG_BOOT_RESUME) {
        RG_LOGI("Loading resume state");
        rg_emu_load_state(app->saveSlot);
    }
}
