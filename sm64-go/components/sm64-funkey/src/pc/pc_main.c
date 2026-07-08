#include <stdlib.h>
#include <signal.h>
#include <string.h>
#ifdef TARGET_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "sm64.h"

#include "game/memory.h"
#include "audio/external.h"

#include "gfx/gfx_pc.h"
#include "gfx/gfx_opengl.h"
#include "gfx/gfx_soft.h"
#include "gfx/gfx_direct3d11.h"
#include "gfx/gfx_direct3d12.h"
#include "gfx/gfx_dos_api.h"
#include "gfx/gfx_dxgi.h"
#include "gfx/gfx_glx.h"
#include "gfx/gfx_sdl.h"

#include "audio/audio_api.h"
#include "audio/audio_sb16.h"
#include "audio/audio_wasapi.h"
#include "audio/audio_pulse.h"
#include "audio/audio_alsa.h"
#include "audio/audio_sdl.h"
#include "audio/audio_null.h"

#include "controller/controller_keyboard.h"

#include "compat.h"

#include "configfile.h"
#include "savestate.h"

#define CONFIG_FILE "sm64config.txt"

OSMesg D_80339BEC;
OSMesgQueue gSIEventMesgQueue;

s8 gResetTimer;
s8 D_8032C648;
s8 gDebugLevelSelect;
s8 gShowProfiler;
s8 gShowDebugText;

static struct AudioAPI *audio_api;
static struct GfxWindowManagerAPI *wm_api;
static struct GfxRenderingAPI *rendering_api;

extern int stop_menu_loop;
char *prog_name SAVESTATE_EXCLUDE;

extern void gfx_run(Gfx *commands);
extern void thread5_game_loop(void *arg);
extern void create_next_audio_buffer(s16 *samples, u32 num_samples);
void game_loop_one_iteration(void);

void dispatch_audio_sptask(UNUSED struct SPTask *spTask) {
}

void set_vblank_handler(UNUSED s32 index, UNUSED struct VblankHandler *handler, UNUSED OSMesgQueue *queue, UNUSED OSMesg *msg) {
}

static uint8_t inited = 0;

#include "game/game_init.h" // for gGlobalTimer
void send_display_list(struct SPTask *spTask) {
    if (!inited) {
        return;
    }
    gfx_run((Gfx *)spTask->task.t.data_ptr);
}

#define printf

#ifdef VERSION_EU
#define SAMPLES_HIGH 656
#define SAMPLES_LOW 640
#else
#define SAMPLES_HIGH 544
#define SAMPLES_LOW 528
#endif

void produce_one_frame(void) {
    gfx_start_frame();
    game_loop_one_iteration();

    if (configEnableSound) {
        int samples_left = audio_api->buffered();
        u32 num_audio_samples = samples_left < audio_api->get_desired_buffered() ? SAMPLES_HIGH : SAMPLES_LOW;
        s16 audio_buffer[SAMPLES_HIGH * 2 * 2];
        for (int i = 0; i < 2; i++) {
            create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
        }
        audio_api->play((u8 *)audio_buffer, 2 * num_audio_samples * 4);
    }

    gfx_end_frame();
    savestate_check();
}

#ifdef TARGET_WEB
static void em_main_loop(void) {
}

static void request_anim_frame(void (*func)(double time)) {
    EM_ASM(requestAnimationFrame(function(time) {
        dynCall("vd", $0, [time]);
    }), func);
}

static void on_anim_frame(double time) {
    static double target_time;

    time *= 0.03; // milliseconds to frame count (33.333 ms -> 1)

    if (time >= target_time + 10.0) {
        // We are lagging 10 frames behind, probably due to coming back after inactivity,
        // so reset, with a small margin to avoid potential jitter later.
        target_time = time - 0.010;
    }

    for (int i = 0; i < 2; i++) {
        // If refresh rate is 15 Hz or something we might need to generate two frames
        if (time >= target_time) {
            produce_one_frame();
            target_time = target_time + 1.0;
        }
    }

    request_anim_frame(on_anim_frame);
}
#endif

static void save_config(void) {
    configfile_save(CONFIG_FILE);
}

static void on_fullscreen_changed(bool is_now_fullscreen) {
    configFullscreen = is_now_fullscreen;
    gfx_update_dimensions();
}

void game_exit(void) {
    if (audio_api && audio_api->shutdown) audio_api->shutdown();
    gfx_shutdown();
    exit(0);
}

/* Handler for SIGUSR1, caused by closing the console */
void handle_sigusr1(int sig)
{
	//printf("Caught signal USR1 %d\n", sig);

	/* Exit menu if it was launched */
	stop_menu_loop = 1;

	/* Signal to quick save and poweoff after next loop */
	savestate_request_save(QUICKSAVE_SLOT);
}

void main_func(void) {
    static u64 pool[0x165000/8 / 4 * sizeof(void *)];
    main_pool_init(pool, pool + sizeof(pool) / sizeof(pool[0]));
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);

    configfile_load(CONFIG_FILE);
    atexit(save_config);

    /* Init USR1 Signal (for quick save and poweroff) */
	signal(SIGUSR1, handle_sigusr1);

#ifdef TARGET_WEB
    emscripten_set_main_loop(em_main_loop, 0, 0);
    request_anim_frame(on_anim_frame);
#endif

#if defined(ENABLE_DX12)
    rendering_api = &gfx_direct3d12_api;
    wm_api = &gfx_dxgi_api;
#elif defined(ENABLE_DX11)
    rendering_api = &gfx_direct3d11_api;
    wm_api = &gfx_dxgi_api;
#elif defined(ENABLE_OPENGL) || defined(ENABLE_OPENGL_LEGACY)
    rendering_api = &gfx_opengl_api;
    #if defined(__linux__) || defined(__BSD__)
        wm_api = &gfx_glx;
    #elif defined(TARGET_DOS)
        wm_api = &gfx_dos_api;
    #else
        wm_api = &gfx_sdl;
    #endif
#elif defined(ENABLE_SOFTRAST)
    rendering_api = &gfx_soft_api;
    #if defined(TARGET_DOS)
        wm_api = &gfx_dos_api;
    #else
        wm_api = &gfx_sdl;
    #endif
#else
    #error Could not pick rendering API!
#endif

    gfx_init(wm_api, rendering_api, "Super Mario 64 PC-Port", configFullscreen);

    if (configEnableSound) {
#if HAVE_WASAPI
        if (audio_api == NULL && audio_wasapi.init()) {
            audio_api = &audio_wasapi;
        }
#endif
/*
#if HAVE_PULSE_AUDIO
        if (audio_api == NULL && audio_pulse.init()) {
            audio_api = &audio_pulse;
        }
#endif
*/
/*
#if HAVE_ALSA
        if (audio_api == NULL && audio_alsa.init()) {
            audio_api = &audio_alsa;
        }
#endif
*/
        if (audio_api == NULL && audio_sdl.init()) {
            audio_api = &audio_sdl;
        }
#ifdef TARGET_DOS
        if (audio_api == NULL && audio_sb.init()) {
            audio_api = &audio_sb;
        }
#endif
    }

    if (audio_api == NULL) {
        audio_api = &audio_null;
    }

    wm_api->set_fullscreen_changed_callback(on_fullscreen_changed);
    wm_api->set_keyboard_callbacks(keyboard_on_key_down, keyboard_on_key_up, keyboard_on_all_keys_up);

    audio_init();
    sound_init();

    thread5_game_loop(NULL);
#ifdef TARGET_WEB
    /*for (int i = 0; i < atoi(argv[1]); i++) {
        game_loop_one_iteration();
    }*/
    inited = 1;
#else
    inited = 1;
    while (1) {
        wm_api->main_loop(produce_one_frame);
    }
#endif
}

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR pCmdLine, UNUSED int nCmdShow) {
    main_func();
    return 0;
}
#else
int main(UNUSED int argc, UNUSED char *argv[]) {
    prog_name = argv[0];
    for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--quickResume")) {
            savestate_request_load(QUICKSAVE_SLOT);
        }
    }
    main_func();
    return 0;
}
#endif
