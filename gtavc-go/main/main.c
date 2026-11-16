/*
 * GTA Vice City J2ME - Retro-Go platform port
 */

#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game.h"
#include "world.h"
#include "anim_data.h"
#include "assets.h"
#include "snd.h"
#include "j2me.h"
#include "save.h"
#include "gtavc_profile.h"
#include "video_compose.h"
#include "audio_service.h"
#include "radio_stream.h"
#include "timing.h"
#include "screenshot.h"

gtavc_prof_t g_prof = {0};
rg_task_t *gtavc_profile_game_task = NULL;

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_565_BE
#else
#define FB_PIXEL_FORMAT RG_PIXEL_565_LE
#endif

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define AUDIO_SAMPLE_RATE GTAVC_AUDIO_RATE
#define AUDIO_TARGET_FPS  30
#define GAME_TASK_STACK_SIZE  (32 * 1024)

static rg_surface_t *surfaces[2] = {NULL, NULL};
static int current_surface = 0;
static int last_surface = -1;
static rg_app_t *app = NULL;
static uint32_t prev_gamepad = 0;
static uint32_t input_blocked = UINT32_MAX;
static bool resume_pending = false;
static bool timing_reset_pending = false;

#define MENU_HOLD_DELAY_US 400000 /* 400 ms hold threshold for Retro-Go system menus */

#define SETTING_FRAMESKIP "frameskip_mode"

enum {
    FS_OFF   = 0,  /* Off: render every frame */
    FS_AUTO  = 1,  /* Auto: skip 1 frame when render time exceeds 35ms */
    FS_SKIP1 = 2,  /* Skip 1: 1 draw, 1 skip */
    FS_SKIP2 = 3,  /* Skip 2: 1 draw, 2 skips */
};

static int g_frameskip_mode = FS_OFF;
static AutoFrameskip auto_frameskip;

static void suspend_auto_frameskip(void) {
    Timing_suspendAuto(&auto_frameskip);
    if (app) app->frameskip = -1;
}
static void log_native_options(void) {
    if (GTA_game) Game_logOptions(GTA_game);
}

static rg_gui_event_t frameskip_update_cb(rg_gui_option_t *option, rg_gui_event_t event) {
    int mode = g_frameskip_mode;
    int max = 3;

    if (event == RG_DIALOG_PREV)
        mode = mode > 0 ? mode - 1 : max;
    if (event == RG_DIALOG_NEXT)
        mode = mode < max ? mode + 1 : 0;

    if (mode != g_frameskip_mode) {
        g_frameskip_mode = mode;
        rg_settings_set_number(NS_APP, SETTING_FRAMESKIP, mode);
        suspend_auto_frameskip();
        timing_reset_pending = true;
        RG_LOGI("Frameskip: %s", mode == FS_OFF ? "Off" : mode == FS_AUTO ? "Auto" : mode == FS_SKIP1 ? "Skip 1" : "Skip 2");
        return RG_DIALOG_REDRAW;
    }

    const char *names[] = {"Off", "Auto", "Skip 1", "Skip 2"};
    strcpy(option->value, names[mode]);
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest) {
    *dest++ = (rg_gui_option_t){0, _("Frameskip"), "-", RG_DIALOG_FLAG_NORMAL, &frameskip_update_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

/* Clear engine state directly: synthetic releases could trigger a sprint tap. */
static void reset_input(void) {
    if (GTA_game) {
        GTA_game->down = 0;
        GTA_game->edges = 0;
        GTA_game->sprintPress = 0;
    }
    prev_gamepad = 0;
    input_blocked = UINT32_MAX;
}

static uint32_t filter_input(uint32_t raw) {
    input_blocked &= raw;
    return raw & ~input_blocked;
}

static void show_system_menu(bool options, uint32_t held_key) {
    reset_input();
    suspend_auto_frameskip();
    AudioService_pause();
    /* Shared GUI owns mute/unmute. No producer can hold the device lock now. */
    if (options) rg_gui_options_menu();
    else rg_gui_game_menu();
    while (rg_input_read_gamepad() & held_key) rg_task_delay(20);
    reset_input();
    AudioService_resume();
}

static bool file_exists(const char *dir, const char *name) {
    char p[600];
    snprintf(p, sizeof(p), "%s/%s", dir, name);
    FILE *f = fopen(p, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static const char *resolve_assets_dir(const char *rom_path) {
    static char asset_dir[512];

    if (rom_path && rom_path[0] && strlen(rom_path) < sizeof(asset_dir)) {
        char parent[512];
        snprintf(parent, sizeof(parent), "%s", rom_path);
        char *slash = strrchr(parent, '/');
        if (slash) {
            if (slash == parent) slash[1] = '\0';
            else *slash = '\0';
        } else {
            snprintf(parent, sizeof(parent), ".");
        }
        /* Prefer the selected entry's assets subfolder. A flat .bin launch
           remains supported without opening or interpreting the entry file. */
        const char *suffixes[] = {"/assets", "", "/gtavc/assets", "/gtavc"};
        for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
            int n = snprintf(asset_dir, sizeof(asset_dir), "%s%s", parent, suffixes[i]);
            if (n > 0 && (size_t)n < sizeof(asset_dir) && file_exists(asset_dir, "mapidx.bin"))
                return asset_dir;
        }
    }

    if (file_exists(RG_BASE_PATH_ROMS "/gtavc/assets", "mapidx.bin"))
        return RG_BASE_PATH_ROMS "/gtavc/assets";
    if (file_exists(RG_BASE_PATH_ROMS "/gtavc", "mapidx.bin"))
        return RG_BASE_PATH_ROMS "/gtavc";
    if (file_exists("assets", "mapidx.bin"))
        return "assets";

    return RG_BASE_PATH_ROMS "/gtavc/assets";
}

static inline uint32_t IRAM_ATTR pack_two_argb_to_565_be(uint32_t c0, uint32_t c1) {
    uint32_t p0 = (((c0 >> 16) & 0xF8) | ((c0 >> 13) & 0x07)) | ((((c0 >> 5) & 0xE0) | ((c0 >> 3) & 0x1F)) << 8);
    uint32_t p1 = (((c1 >> 16) & 0xF8) | ((c1 >> 13) & 0x07)) | ((((c1 >> 5) & 0xE0) | ((c1 >> 3) & 0x1F)) << 8);
    return (p1 << 16) | p0;
}

static inline uint32_t IRAM_ATTR pack_two_argb_to_565_le(uint32_t c0, uint32_t c1) {
    uint32_t p0 = ((c0 >> 8) & 0xF800) | ((c0 >> 5) & 0x07E0) | ((c0 >> 3) & 0x001F);
    uint32_t p1 = ((c1 >> 8) & 0xF800) | ((c1 >> 5) & 0x07E0) | ((c1 >> 3) & 0x001F);
    return (p1 << 16) | p0;
}

static void IRAM_ATTR blit_argb_to_surface(const int *src, rg_surface_t *dst, int width, int height) {
    uint32_t *out32 = (uint32_t *)dst->data;
    const uint32_t *s = (const uint32_t *)src;
    int total_pairs = (width * height) >> 1;
#if FB_PIXEL_FORMAT == RG_PIXEL_565_BE
    for (int i = 0; i < total_pairs; i += 4) {
        out32[i + 0] = pack_two_argb_to_565_be(s[0], s[1]);
        out32[i + 1] = pack_two_argb_to_565_be(s[2], s[3]);
        out32[i + 2] = pack_two_argb_to_565_be(s[4], s[5]);
        out32[i + 3] = pack_two_argb_to_565_be(s[6], s[7]);
        s += 8;
    }
#else
    for (int i = 0; i < total_pairs; i += 4) {
        out32[i + 0] = pack_two_argb_to_565_le(s[0], s[1]);
        out32[i + 1] = pack_two_argb_to_565_le(s[2], s[3]);
        out32[i + 2] = pack_two_argb_to_565_le(s[4], s[5]);
        out32[i + 3] = pack_two_argb_to_565_le(s[6], s[7]);
        s += 8;
    }
#endif
}

static void handle_input(uint32_t curr, uint32_t prev) {
    /* Aggregate physical aliases before emitting logical key edges. */
    const struct { uint32_t mask; int key; } keymap[] = {
        { RG_KEY_UP, KEY_UP }, { RG_KEY_DOWN, KEY_DOWN },
        { RG_KEY_LEFT, KEY_LEFT }, { RG_KEY_RIGHT, KEY_RIGHT },
        { RG_KEY_A, KEY_NUM1 }, { RG_KEY_B, KEY_NUM5 },
        { RG_KEY_START | (rg_input_key_is_present(RG_KEY_X) ? RG_KEY_X : 0), KEY_NUM0 },
        { RG_KEY_SELECT | (rg_input_key_is_present(RG_KEY_Y) ? RG_KEY_Y : 0), KEY_STAR },
        { RG_KEY_L, KEY_NUM4 }, { RG_KEY_R, KEY_NUM6 },
    };
    for (size_t i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
        bool down = (curr & keymap[i].mask) != 0;
        bool was_down = (prev & keymap[i].mask) != 0;
        if (down && !was_down) Game_keyPressed(GTA_game, keymap[i].key);
        if (!down && was_down) Game_keyReleased(GTA_game, keymap[i].key);
    }
}

static bool screenshot_handler(const char *filename, int width, int height) {
    if (last_surface < 0 || !surfaces[last_surface]) return false;
    return Screenshot_save(surfaces[last_surface], filename, width, height);
}

static void redraw_completed_frame(void) {
    if (last_surface >= 0 && surfaces[last_surface])
        rg_display_submit(surfaces[last_surface], 0);
}

static void submit_completed_frame(void) {
    /* Called only after the entire native surface has been composed. The
       blocking submission releases the prior queued surface for our next draw. */
    rg_display_submit(surfaces[current_surface], 0);
    last_surface = current_surface;
    current_surface ^= 1;
}

static void initialize_display_scaling(void) {
    /* Retro-Go already restored valid choices during display initialization.
       Seed its existing app setting only when a choice has not been saved. */
    double saved = rg_settings_get_number(NS_APP, "DispScaling", -1);
    if (saved < RG_DISPLAY_SCALING_OFF || saved >= RG_DISPLAY_SCALING_COUNT)
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
}

static bool save_state_handler(const char *filename) {
    if (!GTA_game || !GTA_game->world || !GTA_game->started) return false;
    if (!Save_writePath(GTA_game->world, filename)) return false;
    Save_writeOptions(GTA_game);
    return true;
}

static bool load_state_handler(const char *filename) {
    resume_pending = false;
    AudioService_pause();
    bool ok = Game_loadProgress(GTA_game, filename);
    if (ok) {reset_input();timing_reset_pending = true;}
    AudioService_resume();
    return ok;
}

static bool resume_if_ready(void) {
    if (!resume_pending || !GTA_game || GTA_game->state != GAME_S_MENU ||
        !GTA_game->world || GTA_game->loadErr || GTA_game->loadI < GTA_game->total) return false;
    resume_pending = false;
    if (!rg_emu_load_state(app->saveSlot))
        rg_gui_alert("Resume failed", "The game menu is still available.");
    return true;
}

static bool reset_handler(bool hard) {
    (void)hard;
    resume_pending = false;
    AudioService_pause();
    if (GTA_game) {
        Game_stop(GTA_game);
        Game_free(GTA_game);
    }
    GTA_game = Game_new();
    if (!GTA_game) {AudioService_resume();return false;}
    reset_input();
    log_native_options();
    GTA_game->nativeOutput = true;
    Game_start(GTA_game);
    suspend_auto_frameskip();
    timing_reset_pending = true;
    AudioService_resume();
    return true;
}

static void event_handler(int event, void *arg) {
    (void)arg;
    if (event == RG_EVENT_SHUTDOWN) {
        AudioService_stop();
        rg_audio_set_mute(true);
        if (GTA_game) {
            Game_stop(GTA_game);
            Game_free(GTA_game);
            GTA_game = NULL;
        }
    } else if (event == RG_EVENT_SPEEDUP) {
        /* Shared frameTime and audio rate already reflect the selected speed. */
        suspend_auto_frameskip();
        timing_reset_pending = true;
    } else if (event == RG_EVENT_REDRAW) {
        redraw_completed_frame();
    }
}

static void game_task(void *arg) {
    (void)arg;
    gtavc_profile_game_task = rg_task_current();

    const char *assets_dir = resolve_assets_dir(app->romPath);
    if (!file_exists(assets_dir, "mapidx.bin")) {
        rg_gui_alert("Missing Assets", "Copy game data to:\n" RG_BASE_PATH_ROMS "/gtavc/assets\nFlat /gtavc layout is also supported.");
        rg_system_exit();
        return;
    }

    rg_storage_mkdir(RG_BASE_PATH_SAVES "/gtavc");
    platform_set_assets_dir(assets_dir);
    platform_set_data_dir(RG_BASE_PATH_SAVES "/gtavc");
    platform_fast_mode = 1;
    platform_frame_limit = 1;

    RG_LOGI("Assets: %s; output/HUD=320x240; audio=%dHz stereo; progress checkpoint saves", assets_dir, AUDIO_SAMPLE_RATE);
    RG_LOGD("[PROF] task placement: game/render core 1 priority 1; loader core 0 priority 2; audio core 1 priority 2");

    RG_LOGD("[PROF] terrain_detail=near/far (far-only trial reverted)");
    RG_LOGD("[PROF] cached terrain storage=lossless signed16 vertices/UVs, unsigned16 indices");
    GTA_game = Game_new();
    if (!GTA_game) {
        rg_gui_alert("Error", "Failed to initialize game engine");
        rg_system_exit();
        return;
    }

    reset_input();
    log_native_options();
    GTA_game->nativeOutput = true;
    Game_start(GTA_game);
    suspend_auto_frameskip();
    timing_reset_pending = true;

    surfaces[0] = rg_surface_create(SCREEN_WIDTH, SCREEN_HEIGHT, FB_PIXEL_FORMAT, MEM_SLOW | MEM_NOPANIC);
    surfaces[1] = rg_surface_create(SCREEN_WIDTH, SCREEN_HEIGHT, FB_PIXEL_FORMAT, MEM_SLOW | MEM_NOPANIC);
    if (!surfaces[0] || !surfaces[1]) {
        rg_gui_alert("Memory error", "Could not allocate display buffers.");
        rg_system_exit();
        return;
    }
    current_surface = 0;
    last_surface = -1;
    initialize_display_scaling();

    /* Dedicated 22.05 kHz audio mixer on Core 1. */
    if (!AudioService_start()) {
        rg_gui_alert("Audio error", "Could not start audio task.");
        rg_system_exit();
        return;
    }

    /* Restore the user's Retro-Go frameskip choice; fresh installs default Off. */
    g_frameskip_mode = rg_settings_get_number(NS_APP, SETTING_FRAMESKIP, FS_OFF);
    if (g_frameskip_mode < FS_OFF || g_frameskip_mode > FS_SKIP2) g_frameskip_mode = FS_OFF;
    RG_LOGI("Frameskip: %s; simulation=%dHz", g_frameskip_mode == FS_OFF ? "Off" : g_frameskip_mode == FS_AUTO ? "Auto" : g_frameskip_mode == FS_SKIP1 ? "Skip 1" : "Skip 2", AUDIO_TARGET_FPS);
    prev_gamepad = 0;
    int skip_frames = 0;
    Timing clock;
    Timing_reset(&clock, rg_system_timer());
    int64_t last_profile_us = rg_system_timer();
    suspend_auto_frameskip();
    int64_t menu_press_time = 0;
    bool menu_long_active = false;
    int64_t option_press_time = 0;
    bool option_long_active = false;

    while (GTA_game && GTA_game->running) {
        if (timing_reset_pending) {
            suspend_auto_frameskip();
            Timing_reset(&clock, rg_system_timer());
            last_profile_us = rg_system_timer();
            skip_frames = 0;
            memset(&g_prof, 0, sizeof(g_prof));
            timing_reset_pending = false;
        }
        if (resume_if_ready()) {
            Timing_reset(&clock, rg_system_timer());
            last_profile_us = rg_system_timer();
            reset_input();
            skip_frames = 0;
            continue;
        }
        int64_t now_us = rg_system_timer();
        int auto_action = Timing_autoPolicy(&auto_frameskip, g_frameskip_mode,
                                            GTA_game->state == GAME_S_PLAY, now_us);
        if (auto_action != TIMING_AUTO_KEEP) {
            app->frameskip = auto_action;
            if (auto_action == -1 && g_frameskip_mode == FS_AUTO) skip_frames = 0;
            if (auto_action == 1) RG_LOGD("Auto frameskip enabled after gameplay warm-up");
        }
        int ticks = Timing_due(&clock, now_us, app->frameTime);
        if (!ticks) {
            int64_t wait_us = clock.next_us - now_us;
            if (wait_us >= 10000) rg_task_delay(10);
            else rg_usleep((uint32_t)wait_us);
            continue;
        }
        if (GTA_game->state != GAME_S_PLAY) ticks = 1;
        uint32_t batch_start_us = (uint32_t)rg_system_timer();
        bool render_batch = (skip_frames == 0);

        /* 1. Process Input */
        uint32_t curr_gamepad = filter_input(rg_input_read_gamepad());
        handle_input(curr_gamepad, prev_gamepad);

        /* 2. MENU: short tap = in-game pause/menu, long hold = Retro-Go game menu */
        if (curr_gamepad & RG_KEY_MENU) {
            if (!(prev_gamepad & RG_KEY_MENU)) {
                menu_press_time = rg_system_timer();
                menu_long_active = false;
            } else if (!menu_long_active && (rg_system_timer() - menu_press_time >= MENU_HOLD_DELAY_US)) {
                menu_long_active = true;
                show_system_menu(false, RG_KEY_MENU);
                Timing_reset(&clock, rg_system_timer());
                last_profile_us = rg_system_timer();
                prev_gamepad = 0;
                menu_press_time = 0;
                menu_long_active = false;
                skip_frames = 0;
                continue;
            }
        } else if (prev_gamepad & RG_KEY_MENU) {
            if (!menu_long_active) {
                Game_keyPressed(GTA_game, KEY_POUND);
                Game_keyReleased(GTA_game, KEY_POUND);
            }
            menu_press_time = 0;
            menu_long_active = false;
        }

        /* 3. OPTION: short tap = switch weapon, long hold = Retro-Go options menu */
        if (curr_gamepad & RG_KEY_OPTION) {
            if (!(prev_gamepad & RG_KEY_OPTION)) {
                option_press_time = rg_system_timer();
                option_long_active = false;
            } else if (!option_long_active && (rg_system_timer() - option_press_time >= MENU_HOLD_DELAY_US)) {
                option_long_active = true;
                show_system_menu(true, RG_KEY_OPTION);
                Timing_reset(&clock, rg_system_timer());
                last_profile_us = rg_system_timer();
                prev_gamepad = 0;
                option_press_time = 0;
                option_long_active = false;
                skip_frames = 0;
                continue;
            }
        } else if (prev_gamepad & RG_KEY_OPTION) {
            if (!option_long_active) {
                Game_keyPressed(GTA_game, KEY_NUM3);
                Game_keyReleased(GTA_game, KEY_NUM3);
            }
            option_press_time = 0;
            option_long_active = false;
        }

        prev_gamepad = curr_gamepad;

        /* Fixed simulation ticks; catch-up skips only video, not input or audio. */
        bool profiling_play = (GTA_game->state == GAME_S_PLAY);
        for (int tick = 0; tick < ticks && GTA_game->running; tick++) {
            uint32_t frame_start_us = tick ? (uint32_t)rg_system_timer() : batch_start_us;
            bool draw_frame = render_batch && tick == ticks - 1;
            int input_state_before = GTA_game->state;
            Game_frame(GTA_game, Timing_stepMs(&clock), draw_frame);
            if (GTA_game->state != input_state_before) reset_input();
            if (GTA_game->loadErr || !GTA_game->running) {
                if (GTA_game->loadMsg[0]) {
                    AudioService_pause();
                    RG_LOGW("Game stopped: %s", GTA_game->loadMsg);
                    rg_gui_alert("Game error", GTA_game->loadMsg);
                }
                GTA_game->running = false;
                break;
            }
            GTAVC_PROF(g_prof.draw_setting = GTA_game->oDraw);
            GTAVC_PROF(g_prof.world_width = GTA_game->r.W);
            GTAVC_PROF(g_prof.world_height = GTA_game->r.H);
            GTAVC_PROF(g_prof.draw_far_z = GTA_game->world ? GTA_game->world->drawDist * 64 : 0);

            /* 4. Update display surface (double buffered with DMA overlap) */

            if (draw_frame && GTA_game->scr) {
                int64_t t_bl0 = GTAVC_PROFILE_TIMER();
                if (Video_hasOverlay()) {
                    Video_compose565(surfaces[current_surface]->data,
                                     surfaces[current_surface]->stride / sizeof(uint16_t),
                                     FB_PIXEL_FORMAT == RG_PIXEL_565_BE);
                    GTAVC_PROF(g_prof.compose_frames++);
                    GTAVC_PROF(g_prof.compose_blocks += Video_dirtyBlocks());
                } else {
                    blit_argb_to_surface(GTA_game->scr, surfaces[current_surface], SCREEN_WIDTH, SCREEN_HEIGHT);
                }
                GTAVC_PROF(g_prof.t_blit_us += (uint64_t)(rg_system_timer() - t_bl0));
                int64_t t_dw0 = GTAVC_PROFILE_TIMER();
                submit_completed_frame();
                GTAVC_PROF(g_prof.t_disp_wait_us += (uint64_t)(rg_system_timer() - t_dw0));
            } else {
                GTAVC_PROF(g_prof.skipped_frames++);
            }

            /* 5. Heartbeat for FPS and load statistics */
            uint32_t busy_us = (uint32_t)rg_system_timer() - frame_start_us;
            rg_system_tick(busy_us);

            /* 7. Profiling statistics report */
            uint32_t work_us = (uint32_t)GTAVC_PROFILE_TIMER() - frame_start_us;
            GTAVC_PROF(g_prof.t_total_us += work_us);
            if (work_us > g_prof.work_peak_us) GTAVC_PROF(g_prof.work_peak_us = work_us);

            if (profiling_play && GTA_game->state == GAME_S_PLAY) GTAVC_PROF(g_prof.count++);
            if (GTA_game->state != GAME_S_PLAY) break;
        }
        /* One automatic controller: use the shared system monitor's decision. */
        skip_frames = render_batch ? Timing_skip(g_frameskip_mode, app->frameskip) :
                      (skip_frames > 0 ? skip_frames - 1 : 0);
        int64_t end_us = rg_system_timer();
        if (profiling_play && GTA_game->state == GAME_S_PLAY) {
            GTAVC_PROF(g_prof.t_wall_us += (uint64_t)(end_us - last_profile_us));
            gtavc_prof_report_if_needed();
        } else {
            suspend_auto_frameskip();
            memset(&g_prof, 0, sizeof(g_prof));
            clock.next_us = end_us + app->frameTime;
        }
        last_profile_us = end_us;
    }

    AudioService_stop();
    rg_system_exit();
}

void app_main(void) {
    const rg_config_t config = {
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = AUDIO_TARGET_FPS,
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
    rg_system_set_tick_rate(AUDIO_TARGET_FPS);
    resume_pending = (app->bootFlags & RG_BOOT_RESUME) != 0;
    if (!RadioStream_init()) {
        rg_gui_alert("Startup error", "Could not initialize radio control.");
        rg_system_exit();
        return;
    }

    /* Controlled affinity trial: keep renderer execution off the loader's core.
       Audio/display retain their higher priorities and existing placement. */
    if (!rg_task_create("gtavc_main", &game_task, NULL, GAME_TASK_STACK_SIZE, 1, RG_TASK_PRIORITY_1, 1)) {
        rg_gui_alert("Startup error", "Could not start game task.");
        rg_system_exit();
        return;
    }

    /* Reuse the registered main task for bounded radio I/O. The fixed platform
       task table has no spare slot for another application worker. */
    RadioStream_run();
}
