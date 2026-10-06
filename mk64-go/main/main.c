#include <rg_system.h>
#include <rg_gui.h>
#include <rg_input.h>
#include <rg_utils.h>
#include <rg_settings.h>
#include <string.h>
#include <stdlib.h>
#include "audio_rg.h"
#include "gfx_rg.h"
#include "frame_pacing.h"
#include "menu_timing.h"
#include <stdio.h>
#include <esp_heap_caps.h>
#include <ultra64.h>

#include "rom.h"
#include "assets.h"
#include "resident.h"
#include "course_segments.h"
#include "segment_bridge.h"
#include "ultra_bridge.h"
#include "diagnostic.h"

extern const unsigned char d_course_mario_raceway_dl_0[];
extern const unsigned char d_course_mario_raceway_packed_dl_0[];
extern const unsigned char common_grand_prix_human_item_curve[];
extern const unsigned char D_0D009158[];
extern const unsigned char silver_trophy_dl[];
extern const unsigned char ceremony_data_seg11_vtx_0[];
extern const unsigned char ceremony_data_seg11_vtx_160[];
extern const unsigned char startup_logo_dl[];
extern const unsigned char startup_logo_seg6_vtx_1B40[];
extern const unsigned char startup_logo_seg6_vtx_1D40[];
extern const unsigned char startup_logo_seg6_vtx_1F40[];
extern char *gCourseNames[];
extern void port_game_init(void);
extern void port_game_loop_one_iteration(void);
extern const char *port_decoder_error(void);
extern u32 gGlobalTimer;
extern int32_t gGamestate;
extern int32_t gGamestateNext;
extern int32_t gMenuSelection;
extern void *port_seg_to_ptr(uintptr_t address);

/* MK64 advances two VI ticks per game frame. */
#define MK64_FRAME_RATE 30

static rg_gui_event_t sound_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT || event == RG_DIALOG_ENTER) {
        bool enabled = !mk64_rg_audio_requested();
        mk64_rg_audio_set_enabled(enabled);
        rg_settings_set_number(NS_APP, "SoundEnabled", enabled);
    }
    strcpy(option->value, mk64_rg_audio_requested() ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t screenshot_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    (void)option;
    if (event == RG_DIALOG_ENTER) {
        char *path = rg_emu_get_path(RG_PATH_SCREENSHOT, rg_system_get_app()->romPath);
        bool saved = rg_emu_screenshot(path, 0, 0);
        free(path);
        rg_gui_alert(_("Screenshot"), saved ? _("Screenshot saved.") : _("Could not save screenshot."));
    }
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Sound"), "-", RG_DIALOG_FLAG_NORMAL, sound_option};
    *dest++ = (rg_gui_option_t){1, _("Save screenshot"), NULL, RG_DIALOG_FLAG_NORMAL, screenshot_option};
    *dest = (rg_gui_option_t)RG_DIALOG_END;
}

static bool sRestartRequested;

static bool reset_handler(bool hard)
{
    (void)hard;
    /* Restart after the GUI returns, while audio is drained. Native EEPROM
     * progress survives; recomp pointers cannot be reset like emulator RAM. */
    sRestartRequested = true;
    return true;
}

static bool unsupported_state_handler(const char *filename)
{
    (void)filename;
    rg_gui_alert(_("Save states unavailable"),
                 _("Mario Kart 64 saves progress automatically.\n"
                   "Mid-race save states are not supported."));
    return false;
}

static void event_handler(int event, void *data)
{
    (void)data;
    if (event == RG_EVENT_SHUTDOWN) {
        mk64_rg_audio_shutdown();
        if (!mk64_eeprom_flush()) RG_LOGE("MK64 native progress flush failed during shutdown");
    } else if (event == RG_EVENT_REDRAW) {
        mk64_rg_gfx_redraw();
    }
}

static bool handle_system_menu(void)
{
    unsigned keys = rg_input_read_gamepad();
    if (!(keys & (RG_KEY_MENU | RG_KEY_OPTION))) return false;
    /* The GUI locks the audio device. Finish worker submissions first. */
    mk64_rg_audio_pause(true);
    if (!mk64_eeprom_flush()) {
        rg_gui_alert(_("Progress save failed"), _("Could not save progress. Check SD card space."));
    } else if (keys & RG_KEY_OPTION) {
        rg_gui_options_menu();
    } else {
        rg_gui_game_menu();
    }
    /* Release the whole combo and any GUI action buttons before game input. */
    rg_input_wait_for_key(RG_KEY_ALL, false, -1);
    if (sRestartRequested) {
        rg_app_t *app = rg_system_get_app();
        rg_system_switch_app(NULL, app->configNs, app->romPath, 0, RG_BOOT_NORMAL);
        return true;
    }
    mk64_rg_audio_pause(false);
    mk64_rg_audio_ensure_init();
    return true;
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = MK64_RG_AUDIO_RATE,
        .handlers = {
            .options = options_handler, .event = event_handler,
            .screenshot = mk64_rg_gfx_screenshot, .reset = reset_handler,
            .saveState = unsupported_state_handler, .loadState = unsupported_state_handler,
        },
        .frameRate = MK64_FRAME_RATE,
        .storageRequired = true,
        .romRequired = true,
    };
    rg_app_t *app = rg_system_init(&config);
    mk64_rg_audio_set_enabled(rg_settings_get_number(NS_APP, "SoundEnabled", 1));
    mk64_rg_audio_pause(false);
    mk64_rom_t rom;
    mk64_rom_result_t result = mk64_rom_open(&rom, app->romPath);

    if (result == MK64_ROM_OK) {
        mk64_ultra_set_rom(&rom);
        RG_LOGI("MK64 USA ROM opened: %lu bytes, byte order %u",
                (unsigned long)rom.size, rom.byte_order);
        char error[96];
        uint8_t pi_header[4];
        bool ready = osPiStartDma(NULL, 0, 0, 0, pi_header,
                                  sizeof(pi_header), NULL) == 0 &&
                     pi_header[0] == 0x80 && pi_header[1] == 0x37 &&
                     pi_header[2] == 0x12 && pi_header[3] == 0x40;
        if (!ready)
            snprintf(error, sizeof(error), "ROM DMA header check failed");
        else
            RG_LOGI("MK64: ROM-backed PI DMA header verified");
        if (ready)
            ready = mk64_assets_prepare(&rom, NULL, error, sizeof(error));
        if (ready)
            ready = mk64_resident_load(&rom, error, sizeof(error));
        if (ready) mk64_segment_set_course(0);
        if (ready &&
            (mk64_course_segment_lookup(0, 6, 0) != d_course_mario_raceway_dl_0 ||
             mk64_course_segment_lookup(0, 7, 0) != d_course_mario_raceway_packed_dl_0 ||
             port_seg_to_ptr(0x06000000u) != d_course_mario_raceway_dl_0 ||
             port_seg_to_ptr(0x07000000u) != d_course_mario_raceway_packed_dl_0)) {
            snprintf(error, sizeof(error), "Course segment mapping failed");
            ready = false;
        }
        mk64_segment_set_course(-1);
        if (ready)
            RG_LOGI("MK64: Mario Raceway segment 6/7 resolver verified");
        if (ready)
            RG_LOGI("MK64: metadata course 0: %s", gCourseNames[0]);
        if (ready) {
            mk64_segment_set_logo(true);
            mk64_segment_set_ceremony(true);
            if (port_seg_to_ptr(0x0d008150u) != common_grand_prix_human_item_curve ||
                port_seg_to_ptr(0x0d009158u) != D_0D009158 ||
                port_seg_to_ptr(0x0b000fe0u) != silver_trophy_dl ||
                port_seg_to_ptr(0x0b000000u) != ceremony_data_seg11_vtx_0 ||
                port_seg_to_ptr(0x0b000160u) != ceremony_data_seg11_vtx_160 ||
                port_seg_to_ptr(0x06002b00u) != startup_logo_dl ||
                port_seg_to_ptr(0x06001b40u) != startup_logo_seg6_vtx_1B40 ||
                port_seg_to_ptr(0x06001d40u) != startup_logo_seg6_vtx_1D40 ||
                port_seg_to_ptr(0x06001f40u) != startup_logo_seg6_vtx_1F40) {
                snprintf(error, sizeof(error), "Static segment mapping failed");
                ready = false;
            }
            mk64_segment_set_ceremony(false);
            if (ready && port_seg_to_ptr(0x0b001418u) != NULL) {
                snprintf(error, sizeof(error), "Inactive ceremony segment remained mapped");
                ready = false;
            }
            mk64_segment_set_logo(false);
            if (ready)
                RG_LOGI("MK64: common, ceremony and logo segment resolver verified");
        }
        if (ready) {
            RG_LOGI("MK64: game bootstrap starting");
            port_game_init();
            RG_LOGI("MK64: game bootstrap complete, timer %u, PSRAM free %u bytes",
                    (unsigned)gGlobalTimer,
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            mk64_pacer_t pacer = {0};
            unsigned rendered_ticks = 0, skipped_ticks = 0;
            mk64_menu_timing_reset();
            int64_t menu_report_at = 0;
            for (uint32_t iteration = 0; ; ++iteration) {
                sMk64MenuTiming.enabled = false; /* Exclude Retro-Go GUI pauses. */
                if (handle_system_menu()) {
                    pacer = (mk64_pacer_t){0};
                    continue;
                }
                int64_t started = rg_system_timer();
                rg_app_t *app = rg_system_get_app();
                bool draw_frame = mk64_pacer_begin(&pacer, started, app->frameTime, app->frameskip);
                mk64_rg_set_render_enabled(draw_frame);
                bool report = iteration < 4 || iteration == 23 ||
                              (iteration + 1) % MK64_RG_REPORT_INTERVAL == 0;
                if (report && MK64_RG_PROFILE)
                    do { RG_LOGD("MK64: game iteration %lu starting, state %ld next %ld menu %ld",
                            (unsigned long)(iteration + 1), (long)gGamestate, (long)gGamestateNext,
                            (long)gMenuSelection); } while (0);
                int32_t previous_state = gGamestate;
                int32_t previous_menu = gMenuSelection;
                sMk64MenuTiming.enabled = MK64_RG_MENU_TIMING && previous_state != 4;
                port_game_loop_one_iteration();
                if (port_decoder_error()) {
                    snprintf(error, sizeof(error), "%s", port_decoder_error());
                    ready = false;
                    break;
                }
                uint32_t busy_us = (uint32_t)(rg_system_timer() - started);
                if (sMk64MenuTiming.enabled)
                    mk64_menu_timing_tick(busy_us, port_audio_out_queued_bytes());
                if (gGamestate != previous_state || gMenuSelection != previous_menu)
                    menu_report_at = rg_system_timer() + 500000;
                if (sMk64MenuTiming.ticks &&
                    ((menu_report_at && rg_system_timer() >= menu_report_at) || report)) {
                    Mk64MenuStage *s = sMk64MenuTiming.stage;
                    (void)s; /* Debug log arguments are removed in release builds. */
                    do { RG_LOGD("MK64 menu %ld sound %s timing us sum/max: ticks %u tickmax %u ROM %u/%u (%u B) "
                            "MIO %u/%u TKMK %u/%u audio %u/%u DSPwait %u/%u PCMwait %u/%u queueMin %u B",
                            (long)gMenuSelection, mk64_rg_audio_requested() ? "on" : "off",
                            (unsigned)sMk64MenuTiming.ticks, (unsigned)sMk64MenuTiming.max_tick_us,
                            (unsigned)s[0].us, (unsigned)s[0].max_us, (unsigned)s[0].bytes,
                            (unsigned)s[1].us, (unsigned)s[1].max_us,
                            (unsigned)s[2].us, (unsigned)s[2].max_us,
                            (unsigned)s[5].us, (unsigned)s[5].max_us,
                            (unsigned)s[3].us, (unsigned)s[3].max_us,
                            (unsigned)s[4].us, (unsigned)s[4].max_us,
                            (unsigned)sMk64MenuTiming.min_queued_bytes); } while (0);
                    mk64_menu_timing_reset();
                    menu_report_at = 0;
                }
                if (gGamestate != previous_state || gMenuSelection != previous_menu)
                    do { RG_LOGD("MK64: transition at iteration %lu: state %ld -> %ld, "
                            "menu %ld -> %ld", (unsigned long)(iteration + 1),
                            (long)previous_state, (long)gGamestate,
                            (long)previous_menu, (long)gMenuSelection); } while (0);
                if (previous_menu != 10 && gMenuSelection == 10)
                    do { RG_LOGD("MK64: title screen active; press A or Start to open the main menu"); } while (0);
                if (report && MK64_RG_PROFILE)
                    do { RG_LOGD("MK64: game iteration %lu complete, state %ld next %ld menu %ld, "
                            "timer %u, busy %u us, PSRAM free %u bytes",
                            (unsigned long)(iteration + 1), (long)gGamestate, (long)gGamestateNext,
                            (long)gMenuSelection,
                            (unsigned)gGlobalTimer, (unsigned)busy_us,
                            (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); } while (0);
                if (draw_frame) ++rendered_ticks; else ++skipped_ticks;
                if (report) {
                    if (MK64_RG_PROFILE) mk64_rg_audio_report();
                    Mk64CaptureStats tv = mk64_rg_gfx_capture_stats();
                    (void)tv;
                    do { RG_LOGD("MK64 pacing: rendered %u skipped %u, policy %d, period %d us, "
                            "TV copies %u avg %u max %u us",
                            rendered_ticks, skipped_ticks, app->frameskip, app->frameTime,
                            tv.copies, tv.copies ? tv.us / tv.copies : 0, tv.max_us); } while (0);
                }
                rg_system_tick(busy_us);
                int64_t wait_us = mk64_pacer_finish(&pacer, rg_system_timer(), draw_frame, app->frameskip);
                if (gGamestate != previous_state || gMenuSelection != previous_menu) {
                    /* The next phase starts with a visible frame and no load debt. */
                    pacer = (mk64_pacer_t){0};
                } else if (wait_us > 0) {
                    rg_usleep((uint32_t)wait_us);
                }
            }
        }
        mk64_rg_audio_shutdown();
        mk64_ultra_set_rom(NULL);
        mk64_rom_close(&rom);
        if (!ready)
            rg_gui_alert(_("Mario Kart 64"), error);
    } else {
        const char *message = _("Could not read the selected ROM.");
        if (result == MK64_ROM_NOT_FOUND)
            message = _("Select a Mario Kart 64 ROM in the launcher.");
        else if (result == MK64_ROM_INVALID)
            message = _("The selected file is not a Mario Kart 64 ROM.");
        else if (result == MK64_ROM_UNSUPPORTED_REGION)
            message = _("Only the USA (NTSC) ROM is supported.");
        rg_gui_alert(_("Mario Kart 64"), message);
    }
    rg_system_exit();
}
