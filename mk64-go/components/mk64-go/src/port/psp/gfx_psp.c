/**
 * "Window manager" for the PSP: there is no window, this only supplies the
 * screen size, frame pacing (the N64 game runs at 30 fps) and yields to the
 * audio thread.
 */
#include <PR/ultratypes.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <pspdisplay.h>

#include <PR/ultratypes.h>
#include "gfx_window_manager_api.h"
#include "gfx_screen_config.h"
#include "macros.h"

#define SCR_WIDTH 480
#define SCR_HEIGHT 272

static void gfx_psp_init(UNUSED const char* game_name, UNUSED bool start_in_fullscreen) {
}

static void gfx_psp_set_keyboard_callbacks(UNUSED bool (*on_key_down)(int scancode),
                                           UNUSED bool (*on_key_up)(int scancode),
                                           UNUSED void (*on_all_keys_up)(void)) {
}

static void gfx_psp_set_fullscreen_changed_callback(UNUSED void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
}

static void gfx_psp_set_fullscreen(UNUSED bool enable) {
}

static void gfx_psp_main_loop(void (*run_one_game_iter)(void)) {
    while (1) {
        run_one_game_iter();
    }
}

static void gfx_psp_get_dimensions(uint32_t* width, uint32_t* height) {
    *width = SCR_WIDTH;
    *height = SCR_HEIGHT;
}

static void gfx_psp_handle_events(void) {
    sceKernelDelayThread(100); // let the audio thread run
}

static bool gfx_psp_start_frame(void) {
    return true;
}

static void gfx_psp_swap_buffers_begin(void) {
}

static void gfx_psp_swap_buffers_end(void) {
    sceKernelDelayThread(100);
}

static double gfx_psp_get_time(void) {
    return sceKernelGetSystemTimeWide() / 1000000.0;
}

struct GfxWindowManagerAPI gfx_psp = {
    gfx_psp_init,
    gfx_psp_set_keyboard_callbacks,
    gfx_psp_set_fullscreen_changed_callback,
    gfx_psp_set_fullscreen,
    gfx_psp_main_loop,
    gfx_psp_get_dimensions,
    gfx_psp_handle_events,
    gfx_psp_start_frame,
    gfx_psp_swap_buffers_begin,
    gfx_psp_swap_buffers_end,
    gfx_psp_get_time,
};
