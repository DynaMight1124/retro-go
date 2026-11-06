#ifndef GBSP_OPTIONS_H
#define GBSP_OPTIONS_H

#include <rg_system.h>
#include <string.h>
#include "gpsp_features.h"
#include "gbsp_video_settings.h"
#include "gbsp_frameskip.h"

extern bool sound_master_enable;
static const char *SETTING_SOUND_EMULATION = "sound";
static void gbsp_settings_load(void)
{
    gp_video_settings_load();
    sound_master_enable = rg_settings_get_number(NS_APP, SETTING_SOUND_EMULATION, true);
}

static rg_gui_event_t gbsp_sound_toggle(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        sound_master_enable = !sound_master_enable;
        rg_settings_set_number(NS_APP, SETTING_SOUND_EMULATION, sound_master_enable);
    }
    strcpy(option->value, sound_master_enable ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_option_t *gbsp_common_options(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Audio enable"), "-", RG_DIALOG_FLAG_NORMAL, &gbsp_sound_toggle};
    *dest++ = (rg_gui_option_t){0, _("Renderer"), "-", RG_DIALOG_FLAG_NORMAL, &gp_video_renderer_option};
    *dest++ = (rg_gui_option_t){0, _("Buffering"), "-", RG_DIALOG_FLAG_NORMAL, &gp_video_buffers_option};
    *dest++ = (rg_gui_option_t){0, _("Frameskip"), "-", RG_DIALOG_FLAG_NORMAL, &gbsp_frameskip_option};
    return dest;
}

#endif
