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
#define ENABLE_AUDIO 0

#define AUDIO_SAMPLE_RATE 16000

#define RENDER_WIDTH  SM64_RENDER_WIDTH
#define RENDER_HEIGHT SM64_RENDER_HEIGHT
#define DISPLAY_WIDTH  SCREEN_WIDTH
#define DISPLAY_HEIGHT SCREEN_HEIGHT
#define SM64_GO_BUILD_MARKER "sm64-go-retro-go"

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
static int current_fb = 0;
static volatile int sm64_debug_frame = -1;
static volatile int sm64_debug_stage = 0;
static volatile uint32_t sm64_debug_heartbeat = 0;
static volatile bool engine_inited = false;
static TaskHandle_t sm64_task_handle = NULL;

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
static void retrogo_controller_read(OSContPad *pad) {
    uint32_t input = rg_input_read_gamepad();

    if (input & RG_KEY_MENU) {
        rg_gui_game_menu();
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
static bool retrogo_audio_init(void) { return true; }
static int retrogo_audio_buffered(void) { return 0; }
static int retrogo_audio_get_desired_buffered(void) { return 544; }
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

static bool s_wm_do_render = true;
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
    int64_t last_time = rg_system_timer();
    const int target_us = 1000000 / 30; // 30 FPS target (33333 us)

    while (1) {
        int64_t now = rg_system_timer();
        int elapsed_us = now - last_time;
        int audio_frames = 1;

        if (elapsed_us >= target_us) {
            audio_frames = elapsed_us / target_us;
            if (audio_frames > 4) audio_frames = 4; // Max 4 frames of audio to prevent blocking death spiral
            last_time += audio_frames * target_us;
        } else {
            vTaskDelay(1);
            continue; // Wait until it's time for the next frame
        }

        int64_t startTime = rg_system_timer();
        sm64_debug_frame = frame_count;
        sm64_debug_stage = SM64_STAGE_LOOP_BEGIN;
        sm64_debug_heartbeat++;

        /* Process ONE frame of SM64 game logic and rendering */
        sm64_debug_stage = SM64_STAGE_GFX_START;
        gfx_start_frame();
        sm64_debug_stage = SM64_STAGE_GAME_LOOP;
        game_loop_one_iteration();
        sm64_debug_stage = SM64_STAGE_GFX_END;
        gfx_end_frame();
        
        frame_count++;

#if ENABLE_AUDIO
        if (configEnableSound) {
            /* Audio Catch-up Loop: Generate enough audio to cover the actual elapsed time */
            for (int a = 0; a < audio_frames; a++) {
                s16 engine_buffer[544 * 2 * 2]; // 1088 stereo samples
                s16 out_buffer[544 * 2];        // 544 stereo samples output
                
                // Generate two 60Hz internal ticks of audio (32000Hz)
                create_next_audio_buffer(engine_buffer, 544);
                create_next_audio_buffer(engine_buffer + 544 * 2, 544);
                
                // Downsample to 16000Hz hardware rate
                for (int i = 0; i < 544; i++) {
                    out_buffer[i * 2 + 0] = engine_buffer[i * 4 + 0];
                    out_buffer[i * 2 + 1] = engine_buffer[i * 4 + 1];
                }
                
                // Submit to DMA. It will only block if we generate faster than real-time.
                retrogo_audio_play((uint8_t *)out_buffer, 544 * 2 * sizeof(s16));
            }
        }
#endif

        /* Switch surfaces for double buffering */
        current_fb = (current_fb + 1) % 2;

        sm64_debug_stage = SM64_STAGE_COPY_FB;
        if (gfx_output != NULL && fb_surfaces[current_fb]->data != NULL) {
            const uint32_t *src = gfx_output;
            const uint32_t *overlay = gfx_overlay_active ? gfx_overlay_output : NULL;
            uint16_t *dst = (uint16_t *)fb_surfaces[current_fb]->data;
            if (DISPLAY_WIDTH == RENDER_WIDTH * 2 && DISPLAY_HEIGHT == RENDER_HEIGHT * 2) {
                for (int y = 0; y < RENDER_HEIGHT; y++) {
                    const uint32_t *src_row = src + y * RENDER_WIDTH;
                    uint16_t *dst_row0 = dst + (y * 2) * DISPLAY_WIDTH;
                    uint16_t *dst_row1 = dst_row0 + DISPLAY_WIDTH;
                    const uint32_t *overlay_row0 = overlay ? overlay + (y * 2) * DISPLAY_WIDTH : NULL;
                    const uint32_t *overlay_row1 = overlay_row0 ? overlay_row0 + DISPLAY_WIDTH : NULL;
                    for (int x = 0; x < RENDER_WIDTH; x++) {
                        const uint32_t base_pixel = src_row[x];
                        const int dx = x * 2;
                        uint32_t pixel = base_pixel;
                        if (overlay_row0) {
                            pixel = sm64_rgba32_blend_over(overlay_row0[dx], pixel);
                        }
                        dst_row0[dx] = sm64_rgba32_to_rgb565_native(pixel);
                        pixel = base_pixel;
                        if (overlay_row0) {
                            pixel = sm64_rgba32_blend_over(overlay_row0[dx + 1], pixel);
                        }
                        dst_row0[dx + 1] = sm64_rgba32_to_rgb565_native(pixel);
                        pixel = base_pixel;
                        if (overlay_row1) {
                            pixel = sm64_rgba32_blend_over(overlay_row1[dx], pixel);
                        }
                        dst_row1[dx] = sm64_rgba32_to_rgb565_native(pixel);
                        pixel = base_pixel;
                        if (overlay_row1) {
                            pixel = sm64_rgba32_blend_over(overlay_row1[dx + 1], pixel);
                        }
                        dst_row1[dx + 1] = sm64_rgba32_to_rgb565_native(pixel);
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

        /* Submit to display */
        sm64_debug_stage = SM64_STAGE_DISPLAY_SUBMIT;
        rg_display_submit(fb_surfaces[current_fb], RG_DISPLAY_WRITE_NOSYNC);

        int64_t endTime = rg_system_timer();
        sm64_debug_stage = SM64_STAGE_TICK;
        rg_system_tick(endTime - startTime);

        sm64_debug_stage = SM64_STAGE_DELAY;
        vTaskDelay(1);
    }
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 30,
        .storageRequired = true,
        .romRequired = false,
        .mallocAlwaysInternal = 0,
    };

    app = rg_system_init(&config);
    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
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
    rg_task_create("sm64_task", sm64_task, NULL, SM64_TASK_STACK_SIZE, 1, RG_TASK_PRIORITY_5, 0);

    int last_sm64_frame = -1;
    uint32_t last_sm64_heartbeat = 0;
    
    int stalled_seconds = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int frame = sm64_debug_frame;
        uint32_t heartbeat = sm64_debug_heartbeat;
        if (engine_inited && frame == last_sm64_frame && heartbeat == last_sm64_heartbeat) {
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
