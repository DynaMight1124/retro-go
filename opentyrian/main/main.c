#include <rg_system.h>
#include <rg_display.h>
#include <rg_audio.h>
#include <rg_gui.h>
#include <rg_settings.h>
#include <string.h>
#include "opentyr.h"

extern rg_surface_t *get_tyrian_surface(void);
extern bool music_disabled;
extern void stop_song(void);
extern void restart_song(void);
extern bool wild, superWild, youAreCheating;
extern bool link_sidekicks_to_a;
extern void SDL_CloseAudio(void);
extern uint8_t gameSpeed;
extern uint8_t processorType;
extern void JE_initProcessorType(void);
extern void JE_setNewGameSpeed(void);
extern void JE_saveConfiguration(void);

static rg_app_t *app = NULL;

static bool screenshot_handler(const char *filename, int width, int height)
{
    rg_surface_t *surf = get_tyrian_surface();
    if (surf)
        return rg_surface_save_image_file(surf, filename, width, height);
    return false;
}

static bool save_state_handler(const char *filename)
{
    rg_gui_alert("Not supported", "Please use the in-game save menu");
    return false;
}

static bool load_state_handler(const char *filename)
{
    rg_gui_alert("Not supported", "Please use the in-game load menu");
    return false;
}

static bool reset_handler(bool hard)
{
    rg_system_restart();
    return true;
}

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        rg_surface_t *surf = get_tyrian_surface();
        if (surf)
            rg_display_submit(surf, 0);
    }
    else if (event == RG_EVENT_SHUTDOWN)
    {
        SDL_CloseAudio();
        rg_audio_set_mute(true);
    }
}

static rg_gui_event_t music_option_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        music_disabled = !music_disabled;
        if (music_disabled)
            stop_song();
        else
            restart_song();
        rg_settings_set_number(NS_APP, "music", !music_disabled);
        return RG_DIALOG_REDRAW;
    }
    strcpy(option->value, !music_disabled ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t wildcard_option_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    int mode = superWild ? 2 : (wild ? 1 : 0);

    if (event == RG_DIALOG_PREV)
    {
        mode = (mode + 2) % 3;
    }
    else if (event == RG_DIALOG_NEXT)
    {
        mode = (mode + 1) % 3;
    }

    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        wild = (mode >= 1);
        superWild = (mode == 2);
        rg_settings_set_number(NS_APP, "wild", mode);
        return RG_DIALOG_REDRAW;
    }

    if (mode == 2)
        strcpy(option->value, "Super Wild");
    else if (mode == 1)
        strcpy(option->value, "Wild");
    else
        strcpy(option->value, "Off");

    return RG_DIALOG_VOID;
}

static rg_gui_event_t invuln_option_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        youAreCheating = !youAreCheating;
        rg_settings_set_number(NS_APP, "invuln", youAreCheating);
        return RG_DIALOG_REDRAW;
    }
    strcpy(option->value, youAreCheating ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t sidekick_option_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        link_sidekicks_to_a = !link_sidekicks_to_a;
        rg_settings_set_number(NS_APP, "link_sidekicks", link_sidekicks_to_a);
        return RG_DIALOG_REDRAW;
    }
    strcpy(option->value, link_sidekicks_to_a ? _("Link to A") : _("Separate"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t gamespeed_option_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    int target_fps = rg_settings_get_number(NS_APP, "target_fps", 35);
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        target_fps = (target_fps == 35) ? 50 : 35;
        rg_settings_set_number(NS_APP, "target_fps", target_fps);
        gameSpeed = (target_fps == 50) ? 5 : 4;
        processorType = (target_fps == 50) ? 4 : 2;
        JE_initProcessorType();
        JE_setNewGameSpeed();
        JE_saveConfiguration();
        rg_system_set_tick_rate(target_fps);
        return RG_DIALOG_REDRAW;
    }
    strcpy(option->value, (target_fps == 50) ? _("Turbo (1.5x)") : _("Normal"));
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Game Speed"), "-", RG_DIALOG_FLAG_NORMAL, &gamespeed_option_cb};
    *dest++ = (rg_gui_option_t){0, _("Music"), "-", RG_DIALOG_FLAG_NORMAL, &music_option_cb};
    *dest++ = (rg_gui_option_t){0, _("Sidekicks"), "-", RG_DIALOG_FLAG_NORMAL, &sidekick_option_cb};
    *dest++ = (rg_gui_option_t){0, _("Wildcard"), "-", RG_DIALOG_FLAG_NORMAL, &wildcard_option_cb};
    *dest++ = (rg_gui_option_t){0, _("Invulnerability"), "-", RG_DIALOG_FLAG_NORMAL, &invuln_option_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

static void tyrian_task(void *arg)
{
    music_disabled = !rg_settings_get_number(NS_APP, "music", 1);
    link_sidekicks_to_a = rg_settings_get_number(NS_APP, "link_sidekicks", 0);
    int wild_mode = rg_settings_get_number(NS_APP, "wild", 0);
    wild = (wild_mode >= 1);
    superWild = (wild_mode == 2);
    youAreCheating = rg_settings_get_number(NS_APP, "invuln", 0);
    int target_fps = rg_settings_get_number(NS_APP, "target_fps", 35);
    gameSpeed = (target_fps == 50) ? 5 : 4;
    processorType = (target_fps == 50) ? 4 : 2;

    char *argv[] = { "opentyrian", NULL };
    tyrian_main(1, argv);
    rg_system_exit();
}

void app_main(void)
{
    static const rg_config_t config = {
        .sampleRate = 22050,
        .frameRate = 35,
        .storageRequired = true,
        .romRequired = false,
        .handlers = {
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
            .reset = &reset_handler,
            .screenshot = &screenshot_handler,
            .event = &event_handler,
            .options = &options_handler,
        },
        .mallocAlwaysInternal = 1,
    };

    app = rg_system_init(&config);
    int target_fps = rg_settings_get_number(NS_APP, "target_fps", 35);
    rg_system_set_tick_rate(target_fps);
    // OpenTyrian interleaves simulation and rendering too tightly to skip the
    // latter safely. Keep automatic frameskip disabled rather than dropping
    // already-rendered frames without recovering meaningful CPU time.
    app->frameskip = -1;

    // Use full scaling as the initial preset, while preserving the user's
    // selection from Retro-Go's Options menu on subsequent launches.
    if (!rg_settings_exists(NS_APP, "DispScaling"))
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);

    rg_task_create("tyrian_main", tyrian_task, NULL, 32 * 1024, 1, RG_TASK_PRIORITY_1, 0);

    // Keep app_main alive so tasks[0] ("main") remains valid in FreeRTOS
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
