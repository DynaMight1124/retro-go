#include <rg_system.h>
#include <stdlib.h>
#include <string.h>

#include "gpsp_esp.h"

// GBA native output resolution
#define GBA_SCREEN_WIDTH  240
#define GBA_SCREEN_HEIGHT 160

// Must match GBA_SOUND_FREQUENCY in the gpSP core (sound.h == 32 * 1024)
#define AUDIO_SAMPLE_RATE   (32 * 1024)
#define AUDIO_BUFFER_LENGTH (AUDIO_SAMPLE_RATE / 60 + 1)

static const char *SETTING_MAX_FRAMESKIP = "max_frameskip";
static const char *SETTING_GBSP_FRAME_DOUBLE_BUFFERING = "gba_frame_double_buffering";

// Rendering skip flag owned by the gpSP core (defined in gpsp_esp.c).
// When non-zero, the core's PPU skips scanline rendering for that frame.
extern uint32_t skip_next_frame;
static int max_frameskip = 5;
static bool frame_double_buffering = true;

static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;
static rg_app_t *app;

static bool screenshot_handler(const char *filename, int width, int height)
{
    return rg_surface_save_image_file(currentUpdate, filename, width, height);
}

static bool save_state_handler(const char *filename)
{
    size_t len = gpsp_state_size();
    void *buffer = malloc(len);
    if (!buffer)
        return false;
    gpsp_save_state_buf(buffer);
    bool success = rg_storage_write_file(filename, buffer, len, 0);
    free(buffer);
    return success;
}

static bool load_state_handler(const char *filename)
{
    size_t len = gpsp_state_size();
    void *buffer = malloc(len);
    if (!buffer)
        return false;
    bool success = rg_storage_read_file(filename, &buffer, &len, RG_FILE_USER_BUFFER)
                    && gpsp_load_state_buf(buffer);
    free(buffer);
    return success;
}

static bool reset_handler(bool hard)
{
    gpsp_reset();
    return true;
}

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
        rg_display_submit(currentUpdate, 0);
}

static rg_gui_event_t change_max_frameskip(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        if (event == RG_DIALOG_PREV && --max_frameskip < 0)
            max_frameskip = 5;
        if (event == RG_DIALOG_NEXT && ++max_frameskip > 5)
            max_frameskip = 0;
        rg_settings_set_number(NS_APP, SETTING_MAX_FRAMESKIP, max_frameskip);
    }
    sprintf(option->value, "%d", max_frameskip);

    return RG_DIALOG_VOID;
}

static rg_gui_event_t toggle_frame_double_buffering(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        frame_double_buffering = !frame_double_buffering;
        // TODO: add a per game setting
        rg_settings_set_number(NS_APP, SETTING_GBSP_FRAME_DOUBLE_BUFFERING, frame_double_buffering);
        if (rg_gui_confirm(_("Double frame buffering changed!"), _("For these changes to take effect you must restart your device.\nrestart now?"), true))
        {
            rg_system_exit();
        }
        
    }
    strcpy(option->value, frame_double_buffering ? _("On") : _("Off"));

    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Change max frameskip"), "-", RG_DIALOG_FLAG_NORMAL, &change_max_frameskip};
    *dest++ = (rg_gui_option_t){0, _("Double frame buffering"), "-", RG_DIALOG_FLAG_NORMAL, &toggle_frame_double_buffering};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

// Map retro-go gamepad bits to the GBA P1 button layout expected by
// gpsp_set_buttons(): bit0=A,1=B,2=Select,3=Start,4=Right,5=Left,6=Up,7=Down,8=R,9=L
static uint16_t map_buttons(uint32_t joystick)
{
    uint16_t b = 0;
    if (joystick & RG_KEY_A)      b |= 0x001;
    if (joystick & RG_KEY_B)      b |= 0x002;
    if (joystick & RG_KEY_SELECT) b |= 0x004;
    if (joystick & RG_KEY_START)  b |= 0x008;
    if (joystick & RG_KEY_RIGHT)  b |= 0x010;
    if (joystick & RG_KEY_LEFT)   b |= 0x020;
    if (joystick & RG_KEY_UP)     b |= 0x040;
    if (joystick & RG_KEY_DOWN)   b |= 0x080;
    if (joystick & RG_KEY_R)      b |= 0x100;
    if (joystick & RG_KEY_L)      b |= 0x200;
    return b;
}


void app_main(void)
{
    app = rg_system_init(&(const rg_config_t){
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
        .handlers = {
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
            .reset = &reset_handler,
            .screenshot = &screenshot_handler,
            .event = &event_handler,
            .options = &options_handler,
        },
    });

    // load settings
    max_frameskip = rg_settings_get_number(NS_APP, SETTING_MAX_FRAMESKIP, 1);
    frame_double_buffering = rg_settings_get_number(NS_APP, SETTING_GBSP_FRAME_DOUBLE_BUFFERING, true);
    
    if(frame_double_buffering)
    {
        updates[0] = rg_surface_create(GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT + 1, RG_PIXEL_565_LE, MEM_FAST);
        updates[1] = rg_surface_create(GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT + 1, RG_PIXEL_565_LE, MEM_FAST);
    }
    else
    {
        updates[0] = rg_surface_create(GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT + 1, RG_PIXEL_565_LE, MEM_FAST);
        updates[1] = updates[0];
    }
    
    if (!updates[0] || !updates[1])
        RG_PANIC("Failed to allocate framebuffers");
    updates[0]->height = GBA_SCREEN_HEIGHT;
    updates[1]->height = GBA_SCREEN_HEIGHT;
    currentUpdate = updates[0];

    // Have the core render straight into the surface we submit to the display.
    // Must be set BEFORE gpsp_init() so it doesn't allocate its own framebuffer.
    gpsp_set_framebuffer(currentUpdate->data);

    if (!gpsp_init())
        RG_PANIC("gpSP init failed");

    if (gpsp_load_rom(app->romPath) != 0)
        RG_PANIC("Could not load the game file.");

    if (app->bootFlags & RG_BOOT_RESUME)
        rg_emu_load_state(app->saveSlot);

#ifdef HAVE_DYNAREC
    RG_LOGI("emulation loop (RISC-V dynarec)");
#else
    RG_LOGI("emulation loop (Interpreter)");
#endif

    static rg_audio_sample_t mixbuffer[AUDIO_BUFFER_LENGTH];

    while (true)
    {
        const int64_t startTime = rg_system_timer();
        uint32_t joystick = rg_input_read_gamepad();

        bool drawFrame = skip_next_frame == 0;
        bool slowFrame = false;

        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION))
        {
            if (joystick & RG_KEY_MENU)
                rg_gui_game_menu();
            else
                rg_gui_options_menu();
            memset(mixbuffer, 0, sizeof(mixbuffer));
            continue;
        }

        gpsp_set_buttons(map_buttons(joystick));

        // gpsp_run_frame() runs one full frame; the PPU honors skip_next_frame.
        gpsp_run_frame();

        if (drawFrame)
        {
            slowFrame = rg_display_is_busy();
            rg_display_submit(currentUpdate, 0);
            currentUpdate = updates[currentUpdate == updates[0]];
            gpsp_set_framebuffer(currentUpdate->data);
        }

        size_t frames_count = gpsp_get_audio((int16_t *)mixbuffer, AUDIO_BUFFER_LENGTH);

        rg_system_tick(rg_system_timer() - startTime);

        rg_audio_submit(mixbuffer, frames_count);


        int32_t local_frameskip = app->frameskip;
        if(local_frameskip > max_frameskip)
            local_frameskip = max_frameskip;

        if (skip_next_frame == 0)
        {
            int elapsed = rg_system_timer() - startTime;
            if (local_frameskip > 0)
                skip_next_frame = local_frameskip;
            else if (elapsed > app->frameTime + 1500) // Allow some jitter
                skip_next_frame = 1; // (elapsed / frameTime)
            else if (drawFrame && slowFrame)
                skip_next_frame = 1;
        }
        else if (skip_next_frame > 0)
        {
            skip_next_frame--;
        }
    }
}