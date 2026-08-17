#include <rg_system.h>
#include <rg_gui.h>
#include <string.h>
#include <math.h>

/* ESP-IDF headers */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* SM64 headers */
#include "sm64.h"
#include "buffers.h"
#include "game/memory.h"
#include "game/object_list_processor.h"
#include "buffers/zbuffer.h"
#include "buffers/framebuffers.h"
#include "buffers/gfx_output_buffer.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/gfx/gfx_soft.h"
#include "pc/gfx/gfx_window_manager_api.h"
#include "pc/audio/audio_api.h"
#include "pc/controller/controller_api.h"
#include "pc/configfile.h"

// Set to 1 to enable audio, 0 to bypass to save CPU
#define ENABLE_AUDIO 1

#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_ENGINE_SAMPLES_HIGH 272
#define AUDIO_ENGINE_SAMPLES_LOW  256
#define AUDIO_SUBMIT_SLOT_COUNT    3
#define AUDIO_SUBMIT_MAX_FRAMES    (AUDIO_ENGINE_SAMPLES_HIGH * 2)

#define RENDER_WIDTH  SM64_RENDER_WIDTH
#define RENDER_HEIGHT SM64_RENDER_HEIGHT
#define DISPLAY_WIDTH  SCREEN_WIDTH
#define DISPLAY_HEIGHT SCREEN_HEIGHT
#define SM64_GO_BUILD_MARKER "sm64-go-retro-go"

#define SETTING_SOUND_ENABLED   "SoundEnabled"
#define SETTING_DISPLAY_SCALING "DispScaling"

#if CONFIG_IDF_TARGET_ESP32S3
#define DEFAULT_SOUND_ENABLED 0
#else
#define DEFAULT_SOUND_ENABLED 1
#endif

/* SM64 task stack size.
 * ESP32-S3 has only ~64 KB free in the main task-stack heap at boot; 40 KB fails xTaskCreate.
 * Reduce the SM64 game-task stack to 30 KB on S3 (loop body uses ~17-20 KB hwm per watchdog).
 * P4 (and other chips) keep the original 40 KB. */
#if CONFIG_IDF_TARGET_ESP32S3
#define SM64_TASK_STACK_SIZE 30720
#else
#define SM64_TASK_STACK_SIZE 40960
#endif

static rg_app_t *app;
static rg_surface_t *fb_surfaces[2];
static rg_surface_t *last_complete_surface;
static int current_fb = 0;
static volatile int sm64_debug_frame = -1;
static volatile int sm64_debug_stage = 0;
static volatile uint32_t sm64_debug_heartbeat = 0;
static volatile bool engine_inited = false;
static volatile bool sm64_shutting_down = false;
static TaskHandle_t sm64_task_handle = NULL;
static bool sm64_in_menu = false;
static bool sm64_reset_timing = false;
static uint32_t suppressed_input = 0;

static bool s_wm_do_render = true;

#if ENABLE_AUDIO
static void retrogo_audio_wait_idle(void);
#endif

/* Global state for the engine */
extern void main_pool_init(void *start, void *end);
extern struct MemoryPool *gEffectsMemoryPool;
extern struct MemoryPool *mem_pool_init(u32 size, u32 side);
extern void game_loop_one_iteration(void);
extern void sound_init(void);
extern void audio_init(void);
extern void config_gfx_pool(void);
extern void thread5_game_loop(void *arg);
extern void create_next_audio_buffer(s16 *samples, u32 num_samples);
extern u32 gGlobalTimer;

/* Graphics Glue */
extern uint32_t *gfx_output;
extern uint32_t *gfx_overlay_output;
extern bool gfx_overlay_active;
extern int gfx_overlay_min_x;
extern int gfx_overlay_min_y;
extern int gfx_overlay_max_x;
extern int gfx_overlay_max_y;
extern int16_t gfx_overlay_row_min_x[DISPLAY_HEIGHT];
extern int16_t gfx_overlay_row_max_x[DISPLAY_HEIGHT];

extern struct GfxRenderingAPI gfx_soft_api;

#if RG_SCREEN_PIXEL_FORMAT == 0
#define SM64_OUTPUT_PIXEL_FORMAT RG_PIXEL_565_BE
#else
#define SM64_OUTPUT_PIXEL_FORMAT RG_PIXEL_565_LE
#endif

static inline uint16_t sm64_rgba32_to_rgb565_native(uint32_t c) {
    uint16_t rgb565 = (uint16_t)(((c & 0x000000F8u) << 8) |
                                 ((c & 0x0000FC00u) >> 5) |
                                 ((c & 0x00F80000u) >> 19));
#if RG_SCREEN_PIXEL_FORMAT == 0
    return (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
#else
    return rgb565;
#endif
}

static inline uint32_t sm64_rgba32_blend_over(uint32_t src, uint32_t dst) {
    const uint32_t a = (src >> 24) & 0xFFu;
    if (a == 0) {
        return dst;
    }
    if (a == 0xFFu) {
        return src;
    }

    const uint32_t ia = 0xFFu - a;
    const uint32_t r = ((src & 0x000000FFu) * a + (dst & 0x000000FFu) * ia) / 0xFFu;
    const uint32_t g = (((src >> 8) & 0xFFu) * a + ((dst >> 8) & 0xFFu) * ia) / 0xFFu;
    const uint32_t b = (((src >> 16) & 0xFFu) * a + ((dst >> 16) & 0xFFu) * ia) / 0xFFu;
    return 0xFF000000u | r | (g << 8) | (b << 16);
}

enum {
    SM64_STAGE_BOOT = 0,
    SM64_STAGE_LOOP_BEGIN,
    SM64_STAGE_GFX_START,
    SM64_STAGE_GAME_LOOP,
    SM64_STAGE_DISPLAY_LIST,
    SM64_STAGE_GFX_END,
    SM64_STAGE_COPY_FB,
    SM64_STAGE_DISPLAY_SUBMIT,
    SM64_STAGE_AUDIO,
    SM64_STAGE_TICK,
    SM64_STAGE_DELAY,
};

static const char *sm64_stage_name(int stage) {
    switch (stage) {
        case SM64_STAGE_BOOT: return "boot";
        case SM64_STAGE_LOOP_BEGIN: return "loop_begin";
        case SM64_STAGE_GFX_START: return "gfx_start";
        case SM64_STAGE_GAME_LOOP: return "game_loop";
        case SM64_STAGE_DISPLAY_LIST: return "display_list";
        case SM64_STAGE_GFX_END: return "gfx_end";
        case SM64_STAGE_COPY_FB: return "copy_fb";
        case SM64_STAGE_DISPLAY_SUBMIT: return "display_submit";
        case SM64_STAGE_AUDIO: return "audio";
        case SM64_STAGE_TICK: return "tick";
        case SM64_STAGE_DELAY: return "delay";
        default: return "unknown";
    }
}

static const char *sm64_task_state_name(eTaskState state) {
    switch (state) {
        case eRunning: return "running";
        case eReady: return "ready";
        case eBlocked: return "blocked";
        case eSuspended: return "suspended";
        case eDeleted: return "deleted";
        case eInvalid: return "invalid";
        default: return "unknown";
    }
}

void send_display_list(struct SPTask *spTask) {
    if (!engine_inited) return;

    sm64_debug_stage = SM64_STAGE_DISPLAY_LIST;
    gfx_run((Gfx *)spTask->task.t.data_ptr);
    sm64_debug_stage = SM64_STAGE_GAME_LOOP;
}

/* Input Handling */
static void retrogo_open_menu(bool options_menu) {
    __atomic_store_n(&sm64_in_menu, true, __ATOMIC_RELEASE);
#if ENABLE_AUDIO
    /* Retro-Go's menus take the audio device lock while open. */
    retrogo_audio_wait_idle();
#endif
    if (options_menu) {
        rg_gui_options_menu();
    } else {
        rg_gui_game_menu();
    }
    /* Do not treat time spent in a modal menu as emulation time. Also prevent
     * the button used to dismiss it from leaking into the game until released. */
    suppressed_input = rg_input_read_gamepad();
    sm64_reset_timing = true;
    __atomic_store_n(&sm64_in_menu, false, __ATOMIC_RELEASE);
}

static void retrogo_controller_read(OSContPad *pad) {
    uint32_t input = rg_input_read_gamepad();
    suppressed_input &= input;
    input &= ~suppressed_input;

    if (input & RG_KEY_OPTION) {
        retrogo_open_menu(true);
        return;
    }

    if (input & RG_KEY_MENU) {
        retrogo_open_menu(false);
        return;
    }

    if (input & RG_KEY_START) pad->button |= START_BUTTON;
    if (input & RG_KEY_SELECT) pad->button |= Z_TRIG;
    if (input & RG_KEY_A) pad->button |= A_BUTTON;
    if (input & RG_KEY_B) pad->button |= B_BUTTON;
    
    if (input & RG_KEY_UP) pad->button |= U_JPAD;
    if (input & RG_KEY_DOWN) pad->button |= D_JPAD;
    if (input & RG_KEY_LEFT) pad->button |= L_JPAD;
    if (input & RG_KEY_RIGHT) pad->button |= R_JPAD;

    pad->stick_x = 0;
    pad->stick_y = 0;
    
    if (input & RG_KEY_LEFT) pad->stick_x = -64;
    if (input & RG_KEY_RIGHT) pad->stick_x = 64;
    if (input & RG_KEY_UP) pad->stick_y = 64;
    if (input & RG_KEY_DOWN) pad->stick_y = -64;
}

s32 osContInit(OSMesgQueue *mq, u8 *controllerBits, OSContStatus *status) {
    *controllerBits = 1;
    return 0;
}

s32 osContStartReadData(OSMesgQueue *mesg) {
    return 0;
}

void osContGetReadData(OSContPad *pad) {
    pad->button = 0;
    pad->stick_x = 0;
    pad->stick_y = 0;
    pad->errnum = 0;
    retrogo_controller_read(pad);
}

/* Audio Handling */
typedef struct {
    rg_audio_frame_t *frames;
    size_t count;
    uint32_t in_use;
} sm64_audio_submit_slot_t;

static sm64_audio_submit_slot_t audio_submit_slots[AUDIO_SUBMIT_SLOT_COUNT];
static rg_task_t *audio_submit_task_handle;
static uint32_t audio_submit_task_ready;
static uint32_t audio_submit_task_stopped;
static uint32_t audio_submit_producer_active;
static unsigned audio_submit_slot_index;

static void retrogo_audio_submit_task(void *arg) {
    rg_task_msg_t msg;
    __atomic_store_n(&audio_submit_task_ready, 1, __ATOMIC_RELEASE);

    while (rg_task_receive(&msg, -1)) {
        if (msg.type == RG_TASK_MSG_STOP) {
            break;
        }

        sm64_audio_submit_slot_t *slot = (sm64_audio_submit_slot_t *)msg.dataPtr;
        rg_audio_submit(slot->frames, slot->count);
        __atomic_store_n(&slot->in_use, 0, __ATOMIC_RELEASE);
    }

    __atomic_store_n(&audio_submit_task_stopped, 1, __ATOMIC_RELEASE);
}

static void retrogo_audio_submit_init(void) {
    rg_audio_frame_t *frames = rg_alloc(
        AUDIO_SUBMIT_SLOT_COUNT * AUDIO_SUBMIT_MAX_FRAMES * sizeof(*frames), MEM_FAST);
    if (!frames) {
        RG_PANIC("Failed to allocate SM64 audio submit buffers!");
    }

    for (int i = 0; i < AUDIO_SUBMIT_SLOT_COUNT; i++) {
        audio_submit_slots[i].frames = frames + i * AUDIO_SUBMIT_MAX_FRAMES;
        audio_submit_slots[i].count = 0;
        audio_submit_slots[i].in_use = 0;
    }

    audio_submit_task_handle = rg_task_create(
        "sm64_audio", retrogo_audio_submit_task, NULL, 3072,
        AUDIO_SUBMIT_SLOT_COUNT, RG_TASK_PRIORITY_2, 1);
    if (!audio_submit_task_handle) {
        RG_PANIC("Failed to start SM64 audio submit task!");
    }

    while (!__atomic_load_n(&audio_submit_task_ready, __ATOMIC_ACQUIRE)) {
        vTaskDelay(1);
    }
}

static sm64_audio_submit_slot_t *retrogo_audio_acquire_submit_slot(void) {
    sm64_audio_submit_slot_t *slot = &audio_submit_slots[audio_submit_slot_index];
    while (__atomic_load_n(&slot->in_use, __ATOMIC_ACQUIRE)) {
        vTaskDelay(1);
    }
    audio_submit_slot_index = (audio_submit_slot_index + 1) % AUDIO_SUBMIT_SLOT_COUNT;
    return slot;
}

static void retrogo_audio_queue_submit(sm64_audio_submit_slot_t *slot, size_t count) {
    slot->count = count;
    __atomic_store_n(&slot->in_use, 1, __ATOMIC_RELEASE);
    if (!rg_task_send(audio_submit_task_handle,
                      &(rg_task_msg_t){.dataPtr = slot}, -1)) {
        __atomic_store_n(&slot->in_use, 0, __ATOMIC_RELEASE);
        RG_PANIC("Failed to queue SM64 audio!");
    }
}

static void retrogo_audio_wait_idle(void) {
    /* The game task is the only producer. Once every slot is released, the
     * worker has returned from rg_audio_submit() and no submission is queued. */
    for (int i = 0; i < AUDIO_SUBMIT_SLOT_COUNT; i++) {
        while (__atomic_load_n(&audio_submit_slots[i].in_use, __ATOMIC_ACQUIRE)) {
            vTaskDelay(1);
        }
    }
}

static bool retrogo_audio_begin_submit(void) {
    if (__atomic_load_n(&sm64_shutting_down, __ATOMIC_ACQUIRE)) {
        return false;
    }

    __atomic_add_fetch(&audio_submit_producer_active, 1, __ATOMIC_ACQ_REL);
    if (__atomic_load_n(&sm64_shutting_down, __ATOMIC_ACQUIRE)) {
        __atomic_sub_fetch(&audio_submit_producer_active, 1, __ATOMIC_RELEASE);
        return false;
    }
    return true;
}

static void retrogo_audio_end_submit(void) {
    __atomic_sub_fetch(&audio_submit_producer_active, 1, __ATOMIC_RELEASE);
}

static void retrogo_audio_shutdown(void) {
    if (!audio_submit_task_handle ||
        __atomic_load_n(&audio_submit_task_stopped, __ATOMIC_ACQUIRE)) {
        return;
    }

    while (__atomic_load_n(&audio_submit_producer_active, __ATOMIC_ACQUIRE)) {
        vTaskDelay(1);
    }
    retrogo_audio_wait_idle();
    if (!rg_task_send(audio_submit_task_handle,
                      &(rg_task_msg_t){.type = RG_TASK_MSG_STOP}, -1)) {
        RG_LOGE("Failed to stop SM64 audio submit task");
        return;
    }

    while (!__atomic_load_n(&audio_submit_task_stopped, __ATOMIC_ACQUIRE)) {
        vTaskDelay(1);
    }
}

static bool retrogo_audio_init(void) { return true; }
static int retrogo_audio_buffered(void) { return 0; }
static int retrogo_audio_get_desired_buffered(void) { return AUDIO_ENGINE_SAMPLES_HIGH; }
static void retrogo_audio_play(const uint8_t *buf, size_t len) {
    rg_audio_submit((rg_audio_sample_t *)buf, len / 4);
}

static struct AudioAPI retrogo_audio_api = {
    .init = retrogo_audio_init,
    .buffered = retrogo_audio_buffered,
    .get_desired_buffered = retrogo_audio_get_desired_buffered,
    .play = retrogo_audio_play,
};

/* Window Manager Glue */
static void wm_init(const char *game_name, bool start_in_fullscreen) {}
static void wm_set_keyboard_callbacks(bool (*on_key_down)(int), bool (*on_key_up)(int), void (*on_all_keys_up)(void)) {}
static void wm_set_fullscreen_changed_callback(void (*callback)(bool)) {}
static void wm_set_fullscreen(bool enable) {}
static void wm_main_loop(void (*run_one_game_iter)(void)) {}
static void wm_get_dimensions(uint32_t *width, uint32_t *height) {
    *width = RENDER_WIDTH;
    *height = RENDER_HEIGHT;
}
static void wm_handle_events(void) {}

static void wm_swap_buffers_begin(void) {}
static void wm_swap_buffers_end(void) {}
static double wm_get_time(void) { return (double)rg_system_timer() / 1000000.0; }
static void wm_shutdown(void) {}

static bool wm_start_frame(void) { return s_wm_do_render; }

static struct GfxWindowManagerAPI retrogo_wm_api = {
    .init = wm_init,
    .set_keyboard_callbacks = wm_set_keyboard_callbacks,
    .set_fullscreen_changed_callback = wm_set_fullscreen_changed_callback,
    .set_fullscreen = wm_set_fullscreen,
    .main_loop = wm_main_loop,
    .get_dimensions = wm_get_dimensions,
    .handle_events = wm_handle_events,
    .start_frame = wm_start_frame,
    .swap_buffers_begin = wm_swap_buffers_begin,
    .swap_buffers_end = wm_swap_buffers_end,
    .get_time = wm_get_time,
    .shutdown = wm_shutdown,
};

static rg_gui_event_t retrogo_sound_option_cb(rg_gui_option_t *option,
                                               rg_gui_event_t event) {
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT ||
        event == RG_DIALOG_ENTER) {
        configEnableSound = !configEnableSound;
        rg_settings_set_number(NS_APP, SETTING_SOUND_ENABLED, configEnableSound);
        RG_LOGI("Sound %s", configEnableSound ? "enabled" : "disabled");
    }

    strcpy(option->value, configEnableSound ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static void retrogo_options_handler(rg_gui_option_t *dest) {
    *dest++ = (rg_gui_option_t){
        0, _("Sound"), "-", RG_DIALOG_FLAG_NORMAL, &retrogo_sound_option_cb
    };
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

static bool retrogo_screenshot_handler(const char *filename, int width, int height) {
    rg_surface_t *surface = __atomic_load_n(
        &last_complete_surface, __ATOMIC_ACQUIRE);
    return surface && rg_surface_save_image_file(surface, filename, width, height);
}

static bool retrogo_save_state_handler(const char *filename) {
    (void)filename;
    rg_gui_alert(_("Not supported"),
                 _("Retro-Go save states are not available.\n"
                   "Use SM64's in-game save option for progress."));
    return false;
}

static bool retrogo_load_state_handler(const char *filename) {
    (void)filename;
    rg_gui_alert(_("Not supported"),
                 _("Retro-Go save states are not available.\n"
                   "SM64 loads native progress when the game starts."));
    return false;
}

static bool retrogo_reset_handler(bool hard) {
    (void)hard;
    rg_gui_alert(_("Not supported"),
                 _("In-place reset is not available.\n"
                   "Quit and relaunch SM64 to restart."));
    return false;
}

static void retrogo_event_handler(int event, void *data) {
    if (event == RG_EVENT_SHUTDOWN) {
        __atomic_store_n(&sm64_shutting_down, true, __ATOMIC_RELEASE);
#if ENABLE_AUDIO
        retrogo_audio_shutdown();
#endif
    } else if (event == RG_EVENT_REDRAW) {
        rg_surface_t *surface = __atomic_load_n(
            &last_complete_surface, __ATOMIC_ACQUIRE);
        if (surface) {
            rg_display_submit(surface, 0);
        }
    }
}

struct AudioAPI *audio_api = &retrogo_audio_api;
struct GfxWindowManagerAPI *wm_api = &retrogo_wm_api;
struct GfxRenderingAPI *rendering_api = &gfx_soft_api;


void sm64_task(void *pvParameters) {
    sm64_task_handle = xTaskGetCurrentTaskHandle();
    RG_LOGI("SM64 Task Started. build=%s render=%dx%d display=%dx%d",
            SM64_GO_BUILD_MARKER, RENDER_WIDTH, RENDER_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    RG_LOGI("Initializing Graphics and Audio...");
    gfx_init(wm_api, rendering_api, "SM64-Go", false);
    
    audio_init();
    sound_init();

    RG_LOGI("Initializing SM64 Game Loop...");
    thread5_game_loop(NULL);

    engine_inited = true;
    RG_LOGI("Starting main loop...");

    int frame_count = 0;
    int64_t last_frame_time = rg_system_timer();
    const int target_us = 1000000 / 30; // 30 FPS target (33333 us)
    const int max_catchup_ticks = 4;
#if ENABLE_AUDIO
    unsigned audio_sample_phase = 0;
    retrogo_audio_submit_init();
#endif

    while (1) {
        if (__atomic_load_n(&sm64_shutting_down, __ATOMIC_ACQUIRE)) {
            vTaskDelay(1);
            continue;
        }

        int64_t now = rg_system_timer();
        const int64_t elapsed_us = now - last_frame_time;
        if (elapsed_us < target_us) {
            vTaskDelay(1);
            continue;
        }

        int ticks_due = elapsed_us / target_us;
        if (ticks_due > max_catchup_ticks) {
            /* Avoid fast-forwarding after exceptional stalls such as loading or
             * debugger pauses. Normal 13-30 FPS rendering needs only 1-3 ticks. */
            ticks_due = max_catchup_ticks;
            last_frame_time = now - (int64_t)ticks_due * target_us;
        }
        last_frame_time += (int64_t)ticks_due * target_us;

        /* Run exactly the number of elapsed simulation ticks, rendering only
         * the final one. The rendered tick is included in ticks_due; it is not
         * an additional tick after catch-up. */
        for (int tick = 0; tick < ticks_due; tick++) {
            const bool render_this_tick = tick == ticks_due - 1;
            int64_t startTime = rg_system_timer();
            s_wm_do_render = render_this_tick;
            sm64_debug_frame = frame_count;
            sm64_debug_stage = SM64_STAGE_LOOP_BEGIN;
            sm64_debug_heartbeat++;

            /* Process one SM64 simulation tick. The window-manager callback
             * suppresses only display-list rendering on catch-up ticks. */
            sm64_debug_stage = SM64_STAGE_GFX_START;
            gfx_start_frame();
            sm64_debug_stage = SM64_STAGE_GAME_LOOP;
            game_loop_one_iteration();
            sm64_debug_stage = SM64_STAGE_GFX_END;
            gfx_end_frame();

            frame_count++;

#if ENABLE_AUDIO
            if (configEnableSound && retrogo_audio_begin_submit()) {
                /* Keep the engine's two 60 Hz audio updates per 30 Hz game
                 * tick. At 16 kHz, a repeating 272/272/256 cadence across
                 * those updates produces exactly 16000 frames per second. */
                sm64_audio_submit_slot_t *audio_slot = retrogo_audio_acquire_submit_slot();
                s16 *audio_engine_buffer = (s16 *)audio_slot->frames;
                u32 output_samples = 0;
                sm64_debug_stage = SM64_STAGE_AUDIO;
                for (int update = 0; update < 2; update++) {
                    const u32 num_audio_samples = audio_sample_phase == 2
                        ? AUDIO_ENGINE_SAMPLES_LOW
                        : AUDIO_ENGINE_SAMPLES_HIGH;
                    audio_sample_phase = (audio_sample_phase + 1) % 3;
                    create_next_audio_buffer(audio_engine_buffer + output_samples * 2,
                                             num_audio_samples);
                    output_samples += num_audio_samples;
                }

                retrogo_audio_queue_submit(audio_slot, output_samples);
                retrogo_audio_end_submit();
            }
#endif

            if (render_this_tick) {
                /* Switch surfaces for double buffering after each rendered game tick. */
                current_fb = (current_fb + 1) % 2;

                sm64_debug_stage = SM64_STAGE_COPY_FB;
            if (gfx_output != NULL && fb_surfaces[current_fb]->data != NULL) {
                const uint32_t *src = gfx_output;
                const uint32_t *overlay = gfx_overlay_active ? gfx_overlay_output : NULL;
                uint16_t *dst = (uint16_t *)fb_surfaces[current_fb]->data;
                if (DISPLAY_WIDTH == RENDER_WIDTH * 2 && DISPLAY_HEIGHT == RENDER_HEIGHT * 2) {
                    /* Convert each 160x120 source pixel once, then duplicate the
                     * native RGB565 value into both output rows. */
                    for (int y = 0; y < RENDER_HEIGHT; y++) {
                        const uint32_t *src_row = src + y * RENDER_WIDTH;
                        uint32_t *dst_row0 = (uint32_t *)(dst + (y * 2) * DISPLAY_WIDTH);
                        uint32_t *dst_row1 = dst_row0 + (DISPLAY_WIDTH / 2);
                        for (int x = 0; x < RENDER_WIDTH; x++) {
                            const uint16_t pixel = sm64_rgba32_to_rgb565_native(src_row[x]);
                            const uint32_t pair = (uint32_t)pixel | ((uint32_t)pixel << 16);
                            dst_row0[x] = pair;
                            dst_row1[x] = pair;
                        }
                    }

                    /* High-resolution HUD/text occupies only a small part of most
                     * frames. Blend its nontransparent pixels over the converted
                     * base instead of scanning the full PSRAM overlay surface. */
                    if (overlay) {
                        const int y0 = gfx_overlay_min_y < 0 ? 0 : gfx_overlay_min_y;
                        const int y1 = gfx_overlay_max_y > DISPLAY_HEIGHT
                            ? DISPLAY_HEIGHT : gfx_overlay_max_y;
                        for (int y = y0; y < y1; y++) {
                            const int row_min_x = gfx_overlay_row_min_x[y];
                            const int row_max_x = gfx_overlay_row_max_x[y];
                            const int x0 = row_min_x < 0 ? 0 : row_min_x;
                            const int x1 = row_max_x > DISPLAY_WIDTH
                                ? DISPLAY_WIDTH : row_max_x;
                            const uint32_t *overlay_row = overlay + y * DISPLAY_WIDTH;
                            const uint32_t *src_row = src + (y >> 1) * RENDER_WIDTH;
                            uint16_t *dst_row = dst + y * DISPLAY_WIDTH;
                            for (int x = x0; x < x1; x++) {
                                const uint32_t overlay_pixel = overlay_row[x];
                                if ((overlay_pixel >> 24) != 0) {
                                    const uint32_t pixel = sm64_rgba32_blend_over(
                                        overlay_pixel, src_row[x >> 1]);
                                    dst_row[x] = sm64_rgba32_to_rgb565_native(pixel);
                                }
                            }
                        }
                    }
                } else {
                    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
                        const int src_y = (y * RENDER_HEIGHT) / DISPLAY_HEIGHT;
                        const uint32_t *src_row = src + src_y * RENDER_WIDTH;
                        const uint32_t *overlay_row = overlay ? overlay + y * DISPLAY_WIDTH : NULL;
                        uint16_t *dst_row = dst + y * DISPLAY_WIDTH;
                        for (int x = 0; x < DISPLAY_WIDTH; x++) {
                            const int src_x = (x * RENDER_WIDTH) / DISPLAY_WIDTH;
                            uint32_t pixel = src_row[src_x];
                            if (overlay_row) {
                                pixel = sm64_rgba32_blend_over(overlay_row[x], pixel);
                            }
                            dst_row[x] = sm64_rgba32_to_rgb565_native(pixel);
                        }
                    }
                }
            }
                sm64_debug_stage = SM64_STAGE_DISPLAY_SUBMIT;
                rg_display_submit(fb_surfaces[current_fb], 0);
                __atomic_store_n(&last_complete_surface, fb_surfaces[current_fb],
                                 __ATOMIC_RELEASE);
            }

            int64_t endTime = rg_system_timer();
            const bool resumed_from_menu = sm64_reset_timing;
            sm64_reset_timing = false;
            sm64_debug_stage = SM64_STAGE_TICK;
            rg_system_tick(resumed_from_menu ? 0 : endTime - startTime);
            if (resumed_from_menu) {
                last_frame_time = endTime;
                break;
            }
        }
    }
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 30,
        .storageRequired = true,
        .romRequired = false,
        .handlers = {
            .loadState = &retrogo_load_state_handler,
            .saveState = &retrogo_save_state_handler,
            .reset = &retrogo_reset_handler,
            .screenshot = &retrogo_screenshot_handler,
            .event = &retrogo_event_handler,
            .options = &retrogo_options_handler,
        },
        .mallocAlwaysInternal = 0,
    };

    app = rg_system_init(&config);
    configEnableSound = rg_settings_get_number(
        NS_APP, SETTING_SOUND_ENABLED, DEFAULT_SOUND_ENABLED) != 0;
    RG_LOGI("Sound %s (saved setting or target default)",
            configEnableSound ? "enabled" : "disabled");
    if (!rg_settings_exists(NS_APP, SETTING_DISPLAY_SCALING)) {
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
    }
    app->frameskip = -1;

    RG_LOGI("Initializing SM64-Go Memory Pools... build=%s render=%dx%d display=%dx%d",
            SM64_GO_BUILD_MARKER, RENDER_WIDTH, RENDER_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Allocate SM64 internal buffers and object pool in PSRAM
    sm64_init_buffers();
    sm64_init_objects();
    sm64_init_gfx_output_buffer();
    sm64_init_framebuffers();
    sm64_init_zbuffer();
    sm64_init_gfx_pc();

    // Allocate 4MB pool in PSRAM
    size_t pool_size = 4 * 1024 * 1024;
    void *pool = rg_alloc(pool_size, MEM_SLOW);
    if (!pool) {
        RG_PANIC("Failed to allocate SM64 memory pool!");
    }
    memset(pool, 0, pool_size);
    main_pool_init(pool, (uint8_t *)pool + pool_size);
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);

    RG_LOGI("Initializing Display...");
    fb_surfaces[0] = rg_surface_create(DISPLAY_WIDTH, DISPLAY_HEIGHT, SM64_OUTPUT_PIXEL_FORMAT, MEM_SLOW);
    fb_surfaces[1] = rg_surface_create(DISPLAY_WIDTH, DISPLAY_HEIGHT, SM64_OUTPUT_PIXEL_FORMAT, MEM_SLOW);
    if (!fb_surfaces[0] || !fb_surfaces[1]) {
        RG_PANIC("Failed to allocate framebuffer surfaces!");
    }

    // Launch SM64 in a separate task. Stack size is reduced on S3 (see SM64_TASK_STACK_SIZE).
    if (!rg_task_create("sm64_task", sm64_task, NULL, SM64_TASK_STACK_SIZE,
                        1, RG_TASK_PRIORITY_5, 0)) {
        RG_PANIC("Failed to start SM64 game task!");
    }

    int last_sm64_frame = -1;
    uint32_t last_sm64_heartbeat = 0;
    
    int stalled_seconds = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int frame = sm64_debug_frame;
        uint32_t heartbeat = sm64_debug_heartbeat;
        bool in_menu = __atomic_load_n(&sm64_in_menu, __ATOMIC_ACQUIRE);
        if (engine_inited && !in_menu &&
            frame == last_sm64_frame && heartbeat == last_sm64_heartbeat) {
            stalled_seconds++;
            if (stalled_seconds >= 1 && stalled_seconds <= 12) {
                TaskHandle_t live_task = xTaskGetHandle("sm64_task");
                eTaskState task_state = live_task ? eTaskGetState(live_task) : eDeleted;
                UBaseType_t stack_hwm = live_task ? uxTaskGetStackHighWaterMark(live_task) : 0;
                RG_LOGE("sm64_watchdog: stalled %ds frame=%d hb=%lu timer=%d stage=%s task=%s stack=%lu",
                        stalled_seconds, frame, (unsigned long)heartbeat, (int)gGlobalTimer,
                        sm64_stage_name(sm64_debug_stage),
                        sm64_task_state_name(task_state), (unsigned long)stack_hwm);
            }
        } else {
            last_sm64_frame = frame;
            last_sm64_heartbeat = heartbeat;
            stalled_seconds = 0;
        }
    }
}
