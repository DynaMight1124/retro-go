#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include <pico/pico_int.h>
#include <pico/pico.h>

// Hardware is locked at 22kHz for DAC/I2S stability
#define HW_AUDIO_SAMPLE_RATE 22050
// Core synthesizes at 11kHz for maximum CPU savings
#define EMU_AUDIO_SAMPLE_RATE 11025
// Safer buffer length to prevent overflows during PAL/NTSC sync jitter
#define AUDIO_BUFFER_LENGTH 1024

static rg_surface_t *updates[1];
static rg_surface_t *currentUpdate;
static rg_app_t *app;

static bool sound_enabled = true;
static bool z80_disabled = false;
static bool sprite_accel = false;
static bool fast_renderer = false;
static bool no_vdp_fifo = true;
static bool no_idle_det = false;
static int region_override = 0; // 0=Auto, 1=Japan, 4=USA, 8=Europe
static bool audio_sync_done = false;

static int16_t *synthesis_buffer;
static rg_audio_sample_t *i2s_buffer;

static const char *SETTING_SOUND_ENABLE  = "sound_enable";
static const char *SETTING_Z80_DISABLE   = "z80_disable";
static const char *SETTING_SPRITE_ACCEL  = "sprite_accel";
static const char *SETTING_FAST_RENDERER = "fast_renderer";
static const char *SETTING_NO_VDP_FIFO   = "no_vdp_fifo";
static const char *SETTING_NO_IDLE_DET   = "no_idle_det";
static const char *SETTING_REGION        = "region";

void lprintf(const char *fmt, ...) {}
void cache_flush_d_inval_i(void *start_addr, void *end_addr) {}

static const char *get_bios_filename(int *region, const char *cd_fname)
{
    static char bios_path[128];
    const char *bios_name = "bios_CD_U.bin";

    if (*region == 4)      bios_name = "bios_CD_U.bin"; // US
    else if (*region == 8) bios_name = "bios_CD_E.bin"; // EU
    else if (*region == 1) bios_name = "bios_CD_J.bin"; // JP

    snprintf(bios_path, sizeof(bios_path), "%s/%s", RG_BASE_PATH_BIOS, bios_name);
    return bios_path;
}

static void sync_pico_settings()
{
    // High-performance baseline flags (Mono Core synthesis)
    uint32_t flags = POPT_ACC_SPRITES;
    
    if (!z80_disabled) flags |= POPT_EN_Z80;
    
    if (sound_enabled) 
    {
        flags |= (POPT_EN_FM | POPT_EN_PSG | POPT_FM_YM2612 | POPT_DIS_FM_SSGEG);
        flags |= POPT_EN_MCD_CDDA;
        flags |= POPT_EN_MCD_PCM;
    }
    
    flags |= POPT_EN_MCD_GFX;
    
    // Performance Hacks (On = Hack Active/Faster, Off = Accurate/Slower)
    if (sprite_accel) flags |= POPT_ACC_SPRITES;
    if (fast_renderer) flags |= (POPT_ALT_RENDERER | POPT_DIS_32C_BORDER);
    if (no_vdp_fifo)  flags |= POPT_DIS_VDP_FIFO;
    if (no_idle_det)  flags |= POPT_DIS_IDLE_DET;

    PicoIn.opt = flags;
    PicoIn.sndRate = EMU_AUDIO_SAMPLE_RATE;
    PicoIn.regionOverride = region_override;
    PicoIn.autoRgnOrder = 0x814; // Prefer USA (NTSC), then Japan, then Europe
    PicoIn.sndOut = sound_enabled ? synthesis_buffer : NULL;

    if (fast_renderer) {
        PicoDrawSetOutFormat(PDF_8BIT, 0);
    } else {
        PicoDrawSetOutFormat(PDF_RGB555, 0);
    }

    // Fully synchronize core region, PAL/NTSC state, and VDP screen timing variables
    PicoLoopPrepare();
}

static rg_gui_event_t sound_enable_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        sound_enabled = !sound_enabled;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_SOUND_ENABLE, sound_enabled);
    }
    strcpy(option->value, sound_enabled ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t z80_disable_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        z80_disabled = !z80_disabled;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_Z80_DISABLE, z80_disabled);
    }
    strcpy(option->value, z80_disabled ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t sprite_accel_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        sprite_accel = !sprite_accel;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_SPRITE_ACCEL, sprite_accel);
    }
    strcpy(option->value, sprite_accel ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t fast_renderer_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        fast_renderer = !fast_renderer;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_FAST_RENDERER, fast_renderer);
    }
    strcpy(option->value, fast_renderer ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t no_vdp_fifo_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        no_vdp_fifo = !no_vdp_fifo;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_NO_VDP_FIFO, no_vdp_fifo);
    }
    strcpy(option->value, no_vdp_fifo ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t no_idle_det_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        no_idle_det = !no_idle_det;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_NO_IDLE_DET, no_idle_det);
    }
    strcpy(option->value, no_idle_det ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t region_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    static const int vals[] = {0, 4, 8, 1};
    static const char *names[] = {"Auto", "USA (60Hz)", "Europe (50Hz)", "Japan (60Hz)"};
    int idx = 0;
    for (int i = 0; i < 4; i++) if (vals[i] == region_override) idx = i;
    if (event == RG_DIALOG_PREV) idx = (idx + 3) % 4;
    else if (event == RG_DIALOG_NEXT) idx = (idx + 1) % 4;
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        region_override = vals[idx];
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_REGION, region_override);
    }
    strcpy(option->value, names[idx]);
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Enable Sound"), "-", RG_DIALOG_FLAG_NORMAL, &sound_enable_cb};
    *dest++ = (rg_gui_option_t){0, _("Region"), "-", RG_DIALOG_FLAG_NORMAL, &region_cb};
    *dest++ = (rg_gui_option_t){0, _("HACK - Fast Renderer"), "-", RG_DIALOG_FLAG_NORMAL, &fast_renderer_cb};
    *dest++ = (rg_gui_option_t){0, _("HACK - Disable Z80"), "-", RG_DIALOG_FLAG_NORMAL, &z80_disable_cb};
    *dest++ = (rg_gui_option_t){0, _("HACK - Sprite Accel"), "-", RG_DIALOG_FLAG_NORMAL, &sprite_accel_cb};
    *dest++ = (rg_gui_option_t){0, _("HACK - No VDP FIFO"), "-", RG_DIALOG_FLAG_NORMAL, &no_vdp_fifo_cb};
    *dest++ = (rg_gui_option_t){0, _("HACK - No Idle Det"), "-", RG_DIALOG_FLAG_NORMAL, &no_idle_det_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

void *plat_mmap(unsigned long addr, size_t size, int need_exec, int is_fixed) { return rg_alloc(size, (need_exec ? MEM_EXEC : 0) | MEM_SLOW); }
void *plat_mremap(void *ptr, size_t oldsize, size_t newsize) { void *n = rg_alloc(newsize, MEM_SLOW); if (n && ptr) { memcpy(n, ptr, oldsize); free(ptr); } return n; }
void plat_munmap(void *ptr, size_t size) { free(ptr); }
void emu_video_mode_change(int start_line, int line_count, int start_col, int col_count) {}
void emu_32x_startup(void) {}
static bool load_state_handler(const char *filename) { return PicoState(filename, 0) == 0; }
static bool save_state_handler(const char *filename) { return PicoState(filename, 1) == 0; }
static bool reset_handler(bool hard) { PicoReset(); return true; }
static bool screenshot_handler(const char *filename, int width, int height) { return rg_surface_save_image_file(currentUpdate, filename, width, height); }
static void event_handler(int event, void *arg) { if (event == RG_EVENT_REDRAW) rg_display_submit(currentUpdate, 0); }

static void write_sound(int len)
{
    audio_sync_done = true;
    if (!i2s_buffer) return;

    int emu_frames = len / 2; // bytes to samples
    int hw_frames = 0;

    if (sound_enabled && emu_frames > 0 && synthesis_buffer)
    {
        if (emu_frames > AUDIO_BUFFER_LENGTH) emu_frames = AUDIO_BUFFER_LENGTH;
        for (int i = 0; i < emu_frames; i++) {
            // 11kHz Mono -> 22kHz Stereo via 32-bit packed writes
            uint32_t packed = (uint16_t)synthesis_buffer[i] | ((uint32_t)(uint16_t)synthesis_buffer[i] << 16);
            uint32_t *dst32 = (uint32_t *)&i2s_buffer[i * 2];
            dst32[0] = packed; // L+R of sample N
            dst32[1] = packed; // L+R of duplicated sample N
        }
        hw_frames = emu_frames * 2;
    }
    else
    {
        // Submit silence for hardware-level I2S DMA pacing
        hw_frames = (Pico.m.pal ? 441 : 368); // ~1/50 or ~1/60 at 22k
        memset(i2s_buffer, 0, hw_frames * sizeof(rg_audio_sample_t));
    }
    rg_audio_submit(i2s_buffer, hw_frames);
}

void app_main(void)
{
    app = rg_system_init(&(const rg_config_t){
        .sampleRate = HW_AUDIO_SAMPLE_RATE,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
        .mallocAlwaysInternal = 8192,
        .handlers = { 
            .reset = &reset_handler, 
            .screenshot = &screenshot_handler, 
            .event = &event_handler, 
            .options = &options_handler,
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
        },
    });

    sound_enabled  = rg_settings_get_number(NS_APP, SETTING_SOUND_ENABLE, 1);
    z80_disabled   = rg_settings_get_number(NS_APP, SETTING_Z80_DISABLE, 0);
    sprite_accel   = rg_settings_get_number(NS_APP, SETTING_SPRITE_ACCEL, 0);
    fast_renderer  = rg_settings_get_number(NS_APP, SETTING_FAST_RENDERER, 0);
    no_vdp_fifo    = rg_settings_get_number(NS_APP, SETTING_NO_VDP_FIFO, 1);
    no_idle_det    = rg_settings_get_number(NS_APP, SETTING_NO_IDLE_DET, 0);
    region_override = rg_settings_get_number(NS_APP, SETTING_REGION, 0);

    updates[0] = rg_surface_create(320, 240, RG_PIXEL_565_LE, MEM_FAST);
    if (!updates[0]) updates[0] = rg_surface_create(320, 240, RG_PIXEL_565_LE, MEM_SLOW);
    currentUpdate = updates[0];

    PicoIn.writeSound = write_sound;
    PicoInit();
    
    // Core synthesizes Mono 11kHz
    synthesis_buffer = rg_alloc(AUDIO_BUFFER_LENGTH * sizeof(int16_t), MEM_FAST);
    // Destination for upsampled Stereo 22kHz
    i2s_buffer = rg_alloc(AUDIO_BUFFER_LENGTH * 2 * sizeof(rg_audio_sample_t), MEM_FAST);
    PicoIn.sndOut = synthesis_buffer;

    // PRE-INITIALIZE sndRate to prevent divide-by-zero during media loading
    PicoIn.sndRate = EMU_AUDIO_SAMPLE_RATE;

    size_t rom_size = 0; void *rom_data = NULL;
    if (rg_extension_match(app->romPath, "zip")) {
        rg_storage_unzip_file(app->romPath, NULL, &rom_data, &rom_size, RG_FILE_ALIGN_64KB);
        PicoLoadMedia(app->romPath, rom_data, rom_size, NULL, get_bios_filename, NULL, NULL);
    } else {
        PicoLoadMedia(app->romPath, NULL, 0, NULL, get_bios_filename, NULL, NULL);
    }

    if (PicoIn.AHW & PAHW_MCD) {
        RG_LOGI("Sega CD Hardware Active.\n");
    }

    sync_pico_settings();
    PicoPower();

    if (app->bootFlags & RG_BOOT_RESUME)
    {
        rg_emu_load_state(app->saveSlot);
    }

    int skipFrames = 0;
    bool prevFrameSkipped = true; // Force palette build on first frame

    while (true) {
        const int64_t startTime = rg_system_timer();
        audio_sync_done = false;

        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION)) {
            if (joystick & RG_KEY_MENU) rg_gui_game_menu();
            else rg_gui_options_menu();
            continue;
        }
        PicoIn.pad[0] = 0;
        if (joystick & RG_KEY_UP)    PicoIn.pad[0] |= 0x0001;
        if (joystick & RG_KEY_DOWN)  PicoIn.pad[0] |= 0x0002;
        if (joystick & RG_KEY_LEFT)  PicoIn.pad[0] |= 0x0004;
        if (joystick & RG_KEY_RIGHT) PicoIn.pad[0] |= 0x0008;
        if (joystick & RG_KEY_B)     PicoIn.pad[0] |= 0x0010;
        if (joystick & RG_KEY_A)     PicoIn.pad[0] |= 0x0020;
        if (joystick & RG_KEY_Y)     PicoIn.pad[0] |= 0x0040;
        if (joystick & RG_KEY_START) PicoIn.pad[0] |= 0x0080;

        bool drawFrame = (skipFrames == 0);

        PicoIn.skipFrame = !drawFrame;
        if (drawFrame) {
            if (prevFrameSkipped)
                Pico.m.dirtyPal = 1;
            if (!fast_renderer) {
                // Pitch must always be the surface pitch (320px)
                PicoDrawSetOutBufMD(currentUpdate->data, 320 * 2);
            }
        }
        
        PicoFrame();

        if (sound_enabled) {
            if (!audio_sync_done) {
                write_sound(0);
            }
        } else {
            int64_t elapsed = rg_system_timer() - startTime;
            if (app->frameTime - elapsed > 2000) {
                write_sound(0);
            }
        }
        
        if (drawFrame) {
            int width = (Pico.video.reg[12] & 1) ? 320 : 256;
            int height = (Pico.m.pal && (Pico.video.reg[1] & 8)) ? 240 : 224;
            
            if (width != currentUpdate->width || height != currentUpdate->height) {
                currentUpdate->width = width;
                currentUpdate->height = height;
            }

            if (fast_renderer) {
                PicoDrawUpdateHighPal();
                uint16_t *dst = (uint16_t *)currentUpdate->data;
                int x_offset = (320 - width) / 2;
                uint8_t  *src = Pico.est.Draw2FB + 8;
                uint16_t *pal = Pico.est.HighPal;

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        dst[x_offset + x] = pal[src[x]];
                    }
                    src += 328;
                    dst += 320; // Hardware surface is always 320px wide
                }
                currentUpdate->offset = x_offset * 2;
            } else {
                currentUpdate->offset = ((320 - width) / 2) * 2;
            }
            rg_display_submit(currentUpdate, 0);
        }

        int64_t elapsed = rg_system_timer() - startTime;
        rg_system_tick(elapsed);

        // Dynamically update Retro-Go's tick rate depending on NTSC vs PAL game region
        int target_fps = Pico.m.pal ? 50 : 60;
        if (rg_system_get_tick_rate() != target_fps) {
            rg_system_set_tick_rate(target_fps);
        }

        prevFrameSkipped = !drawFrame;

        if (skipFrames == 0) {
            int frame_elapsed = rg_system_timer() - startTime;
            if (app->frameskip > 0) {
                skipFrames = app->frameskip;
            } else if (frame_elapsed > app->frameTime + 1500) {
                skipFrames = 1;
            }
        } else if (skipFrames > 0) {
            skipFrames--;
        }
    }
}
