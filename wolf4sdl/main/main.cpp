/*
 * WOLF4SDL FOR RETRO-GO
 * 
 * To build for Spear of Destiny (SOD) instead of Wolfenstein 3D:
 * 1. Open "components/wolf4sdl/wl_main/version.h"
 * 2. Uncomment the line: #define SPEAR
 */

#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSIONALREADYCHOSEN

#include "wl_def.h"
#include "wl_main.h"

#define AUDIO_SAMPLE_RATE 22050

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_BE
#else
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_LE
#endif

static rg_surface_t *update;
static rg_app_t *app;

extern "C" void SDL_RG_SetSurface(rg_surface_t *surf);

// Standard Retro-Go button mapping
typedef struct {
    int rg_key;
    SDL_Scancode scancode;
    SDL_Keycode keycode;
} rg_bind_t;

static rg_bind_t binds[] = {
    {RG_KEY_UP, SDL_SCANCODE_UP, SDLK_UP},
    {RG_KEY_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
    {RG_KEY_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT},
    {RG_KEY_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT},
    {RG_KEY_A, SDL_SCANCODE_LCTRL, SDLK_LCTRL},   // Fire
    {RG_KEY_B, SDL_SCANCODE_LALT, SDLK_LALT},     // Strafe
    {RG_KEY_X, SDL_SCANCODE_SPACE, SDLK_SPACE},   // Open/Use
    {RG_KEY_Y, SDL_SCANCODE_Y, SDLK_y},           // Weapon Cycle
    {RG_KEY_L, SDL_SCANCODE_COMMA, SDLK_COMMA},   // Strafe left
    {RG_KEY_R, SDL_SCANCODE_PERIOD, SDLK_PERIOD}, // Strafe right
    {RG_KEY_START, SDL_SCANCODE_SPACE, SDLK_SPACE}, // Open/Use
    {RG_KEY_SELECT, SDL_SCANCODE_Y, SDLK_y},      // Weapon Cycle
    {RG_KEY_OPTION, SDL_SCANCODE_LSHIFT, SDLK_LSHIFT}, // Run
    {RG_KEY_MENU, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE}, // Menu
    {0, SDL_SCANCODE_UNKNOWN, (SDLKey)0}
};

static uint32_t last_input = 0;

extern "C" int SDL_RG_PollEvent(SDL_Event *event)
{
    uint32_t current_input = rg_input_read_gamepad();
    uint32_t changed = current_input ^ last_input;
    
    if (changed) {
        for (int i = 0; binds[i].rg_key; i++) {
            if (changed & binds[i].rg_key) {
                event->type = (current_input & binds[i].rg_key) ? SDL_KEYDOWN : SDL_KEYUP;
                event->key.keysym.scancode = binds[i].scancode;
                event->key.keysym.sym = binds[i].keycode;
                event->key.state = (current_input & binds[i].rg_key) ? SDL_PRESSED : SDL_RELEASED;
                
                last_input ^= binds[i].rg_key;
                return 1;
            }
        }
    }
    return 0;
}

static bool screenshot_handler(const char *filename, int width, int height)
{
    return rg_surface_save_image_file(update, filename, width, height);
}

static bool save_state_handler(const char *filename)
{
    rg_gui_alert("Not implemented", "Please use the in-game menu");
    return false;
}

static bool load_state_handler(const char *filename)
{
    rg_gui_alert("Not implemented", "Please use the in-game menu");
    return false;
}

static bool reset_handler(bool hard)
{
    return false;
}

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        rg_display_submit(update, 0);
    }
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

extern int wolf_main(int argc, char *argv[]);

extern "C" void app_main()
{
    rg_config_t config;
    memset(&config, 0, sizeof(config));
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.frameRate = 70; 
    config.storageRequired = true;
    config.romRequired = false;
    config.handlers.loadState = &load_state_handler;
    config.handlers.saveState = &save_state_handler;
    config.handlers.reset = &reset_handler;
    config.handlers.screenshot = &screenshot_handler;
    config.handlers.event = &event_handler;
    config.handlers.options = &options_handler;
    
    app = rg_system_init(&config);

    // Constant internal resolution for consistency.
    int width = 320;
    int height = 240;

    // Force display to stretch and fill the screen (ignoring aspect ratio).
    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);

    update = rg_surface_create(width, height, FB_PIXEL_FORMAT, MEM_FAST);

    SDL_RG_SetSurface(update);

    char current_datadir[350];
    strcpy(current_datadir, RG_BASE_PATH_ROMS "/wolf3d/"); // Default

    const char *romPath = app->romPath;
    if (romPath && romPath[0])
    {
        char baseDir[256];
        strncpy(baseDir, romPath, sizeof(baseDir) - 1);
        baseDir[sizeof(baseDir) - 1] = 0;
        char *lastSlash = strrchr(baseDir, '/');
        if (lastSlash)
        {
            *(lastSlash + 1) = 0;
            
            char testPath[300];
            bool found = false;
            
            // Probe for Wolf3D or SOD data
            snprintf(testPath, sizeof(testPath), "%sdata/vgahead.wl6", baseDir);
            if (rg_storage_exists(testPath)) found = true;
            
            if (!found) {
                snprintf(testPath, sizeof(testPath), "%sdata/vgahead.wl1", baseDir);
                if (rg_storage_exists(testPath)) found = true;
            }

            if (!found) {
                snprintf(testPath, sizeof(testPath), "%sdata/vgahead.sod", baseDir);
                if (rg_storage_exists(testPath)) found = true;
            }

            if (found)
            {
                snprintf(current_datadir, sizeof(current_datadir), "%sdata/", baseDir);
            }
            else
            {
                strncpy(current_datadir, baseDir, sizeof(current_datadir) - 1);
                current_datadir[sizeof(current_datadir) - 1] = 0;
            }
        }
    }

    // Prepare arguments for Wolf4SDL
    char arg0[] = "wolf4sdl";
    char arg1[] = "--res";
    char arg2[] = "320";
    char arg3[] = "240";
    char arg4[] = "--samplerate";
    char arg5[] = "11025";
    char arg6[] = "--configdir";
    char arg7[256];
    strcpy(arg7, RG_BASE_PATH_CONFIG "/wolf3d/");
    char arg8[] = "--savedir";
    char arg9[256];
    strcpy(arg9, RG_BASE_PATH_SAVES "/wolf3d/");
    char arg10[] = "--datadir";
    char arg11[350];
    strcpy(arg11, current_datadir);
    
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, NULL };
    int argc = 12;

    // Call Wolf4SDL main
    wolf_main(argc, argv);

    rg_system_exit();
}
