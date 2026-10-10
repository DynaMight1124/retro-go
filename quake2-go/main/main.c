#include "adapter.h"
#include "music_esp32.h"
#include "port_logic.h"
#include "timing.h"
#include "lifecycle.h"
#include "quake2.h"
#include "client/client.h"
#include "client/snd_loc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include <stdlib.h>
#include <string.h>

#ifndef CONFIG_IDF_TARGET_ESP32P4
#error "Quake II requires ESP32-P4: its renderer and memory budget target this chip family."
#endif

static rg_app_t *app;
extern void CL_WriteConfiguration(void);
static char basedir[MAX_OSPATH];
static bool initialized, loading = true, menu_returned, native_shutdown;
static bool frame_interrupted;
static bool auto_frameskip = true;
static bool show_fps;
static int64_t last_progress;

int QG_Milliseconds(void) { return (int)(rg_system_timer() / 1000); }
const char *QG_WriteDirectory(void) { return RG_BASE_PATH_SAVES "/quake2"; }
const char *QG_ConfigDirectory(void) { return RG_BASE_PATH_CONFIG "/quake2"; }
// Bump only for incompatible saved structures, item indices or symbol semantics.
// Routine builds and address changes are handled by stable save symbol IDs.
#define Q2_SAVE_VERSION 1u
unsigned QG_SaveVersion(void) { return Q2_SAVE_VERSION; }
void QG_SaveNotice(const char *message) { q2_control_request(Q2_CONTROL_SAVE_NOTICE, message); }

void QG_LoadProgress(void)
{
    int64_t now = rg_system_timer();
    if (loading && now - last_progress > 500000) {
        frame_interrupted = true;
        rg_system_tick(0); // Loading heartbeat; exclude loading from demo comparisons.
        last_progress = now;
        rg_task_yield();
    }
}

void QG_LoadBegin(void) { loading = true; frame_interrupted = true; }
void QG_Quit(void) {
    q2_control_request(Q2_CONTROL_EXIT, NULL);
    for (;;) rg_task_delay(1000); // Terminal actions never acknowledge/resume the engine.
}
void QG_Error(const char *message) {
    q2_control_request(Q2_CONTROL_ERROR, message);
    for (;;) rg_task_delay(1000);
}

static bool unsupported_state(const char *filename)
{
    (void)filename;
    rg_gui_alert(_("Quake II"), _("Use Quake II's native Save/Load menu. Retro-Go slots are not supported yet."));
    return false;
}

static bool reset(bool hard)
{
    if (hard) rg_system_restart();
    Cbuf_AddText("disconnect\nmenu_main\n");
    return true;
}

static rg_gui_event_t frameskip_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        auto_frameskip = !auto_frameskip;
        app->frameskip = auto_frameskip ? 1 : -1;
        rg_settings_set_number(NS_APP, "AutoFrameskip", auto_frameskip);
    }
    strcpy(option->value, auto_frameskip ? _("Auto") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t fps_option(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        show_fps = !show_fps;
        rg_settings_set_number(NS_APP, "ShowFPS", show_fps);
        // The engine is blocked while the shared menu runs. Apply its cvar
        // through the command queue when gameplay resumes.
        Cbuf_AddText(show_fps ? "set cg_drawfps 1\n" : "set cg_drawfps 0\n");
    }
    strcpy(option->value, show_fps ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Frameskip"), "-", RG_DIALOG_FLAG_NORMAL, frameskip_option};
    *dest++ = (rg_gui_option_t){0, _("Show FPS"), "-", RG_DIALOG_FLAG_NORMAL, fps_option};
    *dest = (rg_gui_option_t)RG_DIALOG_END;
}

static void event_handler(int event, void *data)
{
    (void)data;
    if (event == RG_EVENT_REDRAW) q2_video_redraw();
    if (event == RG_EVENT_SPEEDUP && app && !auto_frameskip) app->frameskip = -1;
    if (event == RG_EVENT_SHUTDOWN) {
        // Shared-menu exits run on main with the engine blocked. Native Quit
        // already wrote configuration during CL_Shutdown on the engine task.
        // Stop the consumer before Retro-Go deinitializes its audio driver.
        Music_Shutdown();
        SNDDMA_Shutdown();
        if (initialized && !native_shutdown) CL_WriteConfiguration();
        rg_storage_commit();
    }
}

static void show_menu(bool options)
{
    bool mute = rg_audio_get_mute();
    q2_audio_pause(true);
    rg_audio_set_mute(true);
    // Draw without synchronous SD config writes; shutdown persists native config.
    rg_system_tick(0);
    if (options) rg_gui_options_menu(); else rg_gui_game_menu();
    // Shared reset/overclock actions reset frameskip without a speed event.
    if (!auto_frameskip) app->frameskip = -1;
    rg_audio_set_mute(mute);
    q2_video_redraw();
    q2_audio_pause(false);
    menu_returned = true;
}

void q2_menu(bool options)
{
    q2_control_request(options ? Q2_CONTROL_OPTIONS : Q2_CONTROL_MENU, NULL);
}

static void control_handler(int action, const char *message)
{
    // Only main dispatches platform actions: flash mapping and app switching
    // may disable cache and require an internal-RAM stack on ESP32-P4.
    switch (action) {
    case Q2_CONTROL_MENU: show_menu(false); break;
    case Q2_CONTROL_OPTIONS: show_menu(true); break;
    case Q2_CONTROL_EXIT: native_shutdown = true; rg_storage_commit(); rg_system_exit();
    case Q2_CONTROL_RESUME_NOTICE: unsupported_state(NULL); break;
    case Q2_CONTROL_SAVE_NOTICE:
        q2_audio_pause(true);
        rg_system_tick(0);
        rg_gui_alert(_("Quake II save"), message);
        q2_video_redraw();
        q2_audio_pause(false);
        menu_returned = true;
        break;
    case Q2_CONTROL_ERROR: rg_system_panic(_("Quake II"), message);
    }
}

static bool valid_pak(const char *path)
{
    char dir[MAX_OSPATH];
    return q2_resolve_basedir(path, dir, sizeof(dir));
}

static void quake_task(void *arg)
{
    (void)arg;
    char *argv[] = {"quake2", "+set", "basedir", basedir,
        "+set", "freelook", "1", "+set", "sensitivity", "6",
        "+set", "m_yaw", "0.033", "+set", "m_pitch", "0.033",
        "+set", "cg_drawfps", show_fps ? "1" : "0", NULL};
    RG_LOGI("Starting Quake II; content=%s; writes=%s", basedir, QG_WriteDirectory());
    Quake2_Init((int)(sizeof(argv) / sizeof(argv[0])) - 1, argv);
    initialized = true;
    loading = false;
    q2_input_configure();
    Cbuf_AddText("exec autoexec.cfg\n");
    // The shared option owns this preference, including after an old autoexec.
    Cbuf_AddText(show_fps ? "set cg_drawfps 1\n" : "set cg_drawfps 0\n");
    if (app->bootFlags & RG_BOOT_RESUME)
        q2_control_request(Q2_CONTROL_RESUME_NOTICE, NULL);
    // Variable-rate engine: 30 Hz is the P4 pacing goal, not its 90 FPS ceiling.
    // Using the ceiling as the goal would request excessive automatic frameskip.
    rg_system_set_tick_rate(30);
    q2_timing_t timing = {0};
    q2_timing_reset(&timing, (uint32_t)QG_Milliseconds());
    int skipped = 0;
    for (;;) {
        q2_input_poll();
        if (menu_returned) {
            q2_timing_reset(&timing, (uint32_t)QG_Milliseconds());
            skipped = 0;
            q2_video_take_wait();
            menu_returned = false;
        }
        uint32_t now = (uint32_t)QG_Milliseconds();
        int msec = q2_timing_msec(&timing, now, app->speed);
        if (!msec) { rg_task_delay(1); continue; }
        int frame = cls.framecount;
        q2_video_enable(skipped == 0);
        bool was_loading = loading;
        frame_interrupted = false;
        int64_t start = rg_system_timer();
        Quake2_Frame(msec);
        // Loading heartbeats and in-frame GUI requests invalidate this sample.
        // Rebase immediately so their wall time cannot enter the next frame.
        bool interrupted = was_loading || loading || frame_interrupted || menu_returned;
        q2_timing_finish(&timing, start, rg_system_timer(), q2_video_take_wait(), interrupted);
        if (interrupted) {
            skipped = 0;
            menu_returned = false;
        }
        if (cl.refresh_prepped || cls.state != ca_active) loading = false;
        if (cls.framecount != frame) {
            rg_system_tick(q2_timing_take_busy(&timing));
            if (!interrupted) {
                if (skipped) --skipped; else skipped = app->frameskip > 0 ? app->frameskip : 0;
            }
        }
        // Yield only when waiting for the engine's own rate limit, never after rendering.
        if (cls.framecount == frame) rg_task_delay(1);
    }
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = Q2_RATE, .frameRate = 30,
        .storageRequired = true, .romRequired = false, .mallocAlwaysInternal = 1,
        .handlers = {.loadState = unsupported_state, .saveState = unsupported_state,
            .reset = reset, .screenshot = q2_video_screenshot, .event = event_handler,
            .options = options_handler},
    };
    app = rg_system_init(&config);
    q2_control_init(control_handler);
    auto_frameskip = rg_settings_get_number(NS_APP, "AutoFrameskip", 1) != 0;
    show_fps = rg_settings_get_number(NS_APP, "ShowFPS", 0) != 0;
    if (!auto_frameskip) app->frameskip = -1;
    const char *path = app->romPath;
    char *selected = NULL;
    if (!path || !*path) {
        selected = rg_gui_file_picker(_("Quake II PAK"), RG_BASE_PATH_ROMS "/quake2", valid_pak, false, false);
        if (!selected) rg_system_exit();
        path = selected;
    }
    if (!q2_resolve_basedir(path, basedir, sizeof(basedir))) {
        rg_gui_alert(_("Quake II"), _("Select a Quake II .pak file."));
        rg_system_exit();
    }
    char pak0[MAX_OSPATH];
    int length = snprintf(pak0, sizeof(pak0), "%s/pak0.pak", basedir);
    if (length < 0 || length >= sizeof(pak0) || !rg_storage_exists(pak0)) {
        rg_gui_alert(_("Quake II"), _("Missing pak0.pak next to the selected file."));
        rg_system_exit();
    }
    if (selected) { app->romPath = selected; } // Keep picker selection alive for shared paths/previews.
    if (!rg_storage_mkdir(QG_WriteDirectory())) RG_PANIC("Cannot create Quake II save directory");
    if (!rg_storage_mkdir(QG_ConfigDirectory())) RG_PANIC("Cannot create Quake II config directory");
    q2_video_init();
    // The embedded local server opens UDP sockets even for single-player.
    ESP_ERROR_CHECK(esp_netif_init());
    // The inherited engine needs a large PSRAM stack; FreeRTOS dynamic tasks
    // reserve internal stacks, so use its supported static external-stack API.
    StaticTask_t *tcb = rg_alloc(sizeof(*tcb), MEM_FAST);
    StackType_t *stack = rg_alloc(256 * 1024, MEM_SLOW);
    RG_ASSERT(xTaskCreateStaticPinnedToCore(quake_task, "quake2", 256 * 1024,
        NULL, RG_TASK_PRIORITY_2, stack, tcb, 0), "Engine task allocation failed");
    // Preserve Retro-Go's registered handle and serve platform requests from
    // its internal stack. This blocks when idle; the engine has its own stack.
    q2_control_run();
}
