#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <inttypes.h>

#include <pico/pico_int.h>
#include <pico/pico.h>

#ifndef PICODRIVE_PROFILE_LEVEL
#define PICODRIVE_PROFILE_LEVEL 0
#endif

// Classic ESP32 omits Sega CD and keeps the lightweight mono synthesis path,
// duplicated to the 22.05kHz rate expected by its DAC. Faster targets use the
// Retro-Go high-quality rate, which also closely matches the RF5C164 PCM rate.
// Keeping PAL blocks at 640 frames obeys the shared audio driver's contract.
#if defined(CONFIG_IDF_TARGET_ESP32)
#define HW_AUDIO_SAMPLE_RATE 22050
#define EMU_AUDIO_SAMPLE_RATE 11025
#define EMU_AUDIO_CHANNELS 1
#define AUDIO_RATE_MULTIPLIER 2
#else
#define HW_AUDIO_SAMPLE_RATE 32000
#define EMU_AUDIO_SAMPLE_RATE 32000
#define EMU_AUDIO_CHANNELS 2
#define AUDIO_RATE_MULTIPLIER 1
#endif

// PAL produces the largest per-tick block. Keep two spare samples for the
// core's fractional sample-count and scanline rounding.
#define AUDIO_BUFFER_LENGTH (EMU_AUDIO_SAMPLE_RATE / 50 + 2)
#define AUDIO_OUTPUT_BUFFER_LENGTH \
    (AUDIO_BUFFER_LENGTH * AUDIO_RATE_MULTIPLIER + 1)
#define PERF_REPORT_TICKS 120

static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;
static rg_surface_t *lastUpdate;
static rg_app_t *app;

static bool sound_enabled = true;
static bool fast_renderer = false;
static bool no_idle_det = false;
#if !defined(CONFIG_IDF_TARGET_ESP32)
static bool fm_filter = false;
#define FM_FILTER_ENABLED fm_filter
#else
#define FM_FILTER_ENABLED false
#endif
static bool six_button_pad = false;
static int region_override = 0; // 0=Auto, 1=Japan, 4=USA, 8=Europe

static int16_t *synthesis_buffer;
static rg_audio_sample_t *i2s_buffer;
static int pending_audio_frames;
#if PICODRIVE_PROFILE_LEVEL >= 1
static unsigned audio_callbacks_this_tick;
#endif
static bool compensate_i2s_tail_drop;

#if PICODRIVE_PROFILE_LEVEL >= 1
typedef struct {
    unsigned ticks;
    unsigned rendered;
    unsigned skipped;
    unsigned display_late;
    int64_t work_us;
    int64_t display_submit_us;
    unsigned audio_callbacks;
    int64_t core_us;
    int64_t rendered_core_us;
    int64_t skipped_core_us;
    int64_t audio_wait_us;
} perf_stats_t;

static perf_stats_t perf_stats;
#endif

static const char *SETTING_SOUND_ENABLE  = "sound_enable";
static const char *SETTING_FAST_RENDERER = "fast_renderer";
static const char *SETTING_NO_IDLE_DET   = "no_idle_det";
static const char *SETTING_SIX_BUTTON_PAD = "six_button_pad";
#if !defined(CONFIG_IDF_TARGET_ESP32)
static const char *SETTING_FM_FILTER     = "fm_filter";
#endif
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
    // High-performance baseline flags
    uint32_t flags = POPT_EN_Z80 | POPT_ACC_SPRITES;
    
#if EMU_AUDIO_CHANNELS == 2
    flags |= POPT_EN_STEREO;
#endif
    
    if (sound_enabled) 
    {
        flags |= (POPT_EN_FM | POPT_EN_PSG | POPT_FM_YM2612);
        flags |= POPT_EN_MCD_CDDA;
        flags |= POPT_EN_MCD_PCM;
#if defined(CONFIG_IDF_TARGET_ESP32)
        // SSG-EG is uncommon and relatively expensive on the weakest target.
        flags |= POPT_DIS_FM_SSGEG;
#else
        if (fm_filter) flags |= POPT_EN_FM_FILTER;
#endif
    }
    
    flags |= POPT_EN_MCD_GFX;

    // Optional fast renderer; accurate rendering remains the safe default.
    if (fast_renderer) flags |= POPT_ALT_RENDERER;
    /*
     * Retro-Go submits the native 256-pixel H32 viewport and scales it in the
     * display task. Asking PicoDrive to add 32-pixel side borders only causes
     * FinalizeLine8bit() to overlap-copy every scanline; those borders are
     * cropped again below and never reach the display.
    */
    flags |= POPT_DIS_32C_BORDER;
    if (no_idle_det)  flags |= POPT_DIS_IDLE_DET;

    PicoIn.opt = flags;
    PicoIn.sndRate = EMU_AUDIO_SAMPLE_RATE;
    PicoIn.regionOverride = region_override;
    PicoIn.autoRgnOrder = 0x814; // Prefer USA (NTSC), then Japan, then Europe
    PicoIn.sndOut = sound_enabled ? synthesis_buffer : NULL;

    /*
     * Match PicoDrive's renderer contract. The accurate line renderer uses
     * its indexed finalizer, while the alternate tile renderer writes its
     * indexed output directly and must have no line finalizer selected.
     */
    PicoDrawSetOutFormat(fast_renderer ? PDF_NONE : PDF_8BIT, 0);

    // Respect PicoDrive's compatibility database even when the user selected
    // the normal three-button controller.
    PicoSetInputDevice(0, (six_button_pad ||
        (PicoIn.quirks & PQUIRK_FORCE_6BTN)) ?
        PICO_INPUT_PAD_6BTN : PICO_INPUT_PAD_3BTN);

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

static rg_gui_event_t no_idle_det_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        no_idle_det = !no_idle_det;
        sync_pico_settings();
        // PicoDrive normally configures its opcode detector at reset. Apply
        // this runtime option immediately as a compatibility fallback.
        if (!(PicoIn.AHW & PAHW_MCD)) {
            if (no_idle_det)
                SekFinishIdleDet();
            else
                SekInitIdleDet();
        }
        rg_settings_set_number(NS_APP, SETTING_NO_IDLE_DET, no_idle_det);
    }
    strcpy(option->value, no_idle_det ? _("Off") : _("On"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t controller_cb(rg_gui_option_t *option,
                                    rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        six_button_pad = !six_button_pad;
        sync_pico_settings();
        rg_settings_set_number(NS_APP, SETTING_SIX_BUTTON_PAD,
                               six_button_pad);
    }
    strcpy(option->value, six_button_pad ? _("6 Button") : _("3 Button"));
    return RG_DIALOG_VOID;
}

#if !defined(CONFIG_IDF_TARGET_ESP32)
static rg_gui_event_t fm_filter_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        fm_filter = !fm_filter;
        sync_pico_settings();
        PsndRerate(1);
        rg_settings_set_number(NS_APP, SETTING_FM_FILTER, fm_filter);
    }
    strcpy(option->value, fm_filter ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}
#endif

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
        PsndRerate(1);
        rg_settings_set_number(NS_APP, SETTING_REGION, region_override);
    }
    strcpy(option->value, names[idx]);
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Enable Sound"), "-", RG_DIALOG_FLAG_NORMAL, &sound_enable_cb};
    *dest++ = (rg_gui_option_t){0, _("Region"), "-", RG_DIALOG_FLAG_NORMAL, &region_cb};
    *dest++ = (rg_gui_option_t){0, _("Controller"), "-", RG_DIALOG_FLAG_NORMAL, &controller_cb};
    *dest++ = (rg_gui_option_t){0, _("Fast Renderer (Unstable)"), "-", RG_DIALOG_FLAG_NORMAL, &fast_renderer_cb};
#if !defined(CONFIG_IDF_TARGET_ESP32)
    *dest++ = (rg_gui_option_t){0, _("FM Filter"), "-", RG_DIALOG_FLAG_NORMAL, &fm_filter_cb};
#endif
    *dest++ = (rg_gui_option_t){0, _("Idle Detection"), "-", RG_DIALOG_FLAG_NORMAL, &no_idle_det_cb};
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
static bool screenshot_handler(const char *filename, int width, int height) { return rg_surface_save_image_file(lastUpdate ? lastUpdate : currentUpdate, filename, width, height); }
static void event_handler(int event, void *arg) { if (event == RG_EVENT_REDRAW) rg_display_submit(lastUpdate ? lastUpdate : currentUpdate, 0); }

static void write_sound(int len)
{
#if PICODRIVE_PROFILE_LEVEL >= 1
    audio_callbacks_this_tick++;
#endif
    if (!i2s_buffer) return;

    int emu_frames = len / (sizeof(int16_t) * EMU_AUDIO_CHANNELS);
    if (!sound_enabled || emu_frames <= 0 || !synthesis_buffer) return;

    if (emu_frames > AUDIO_BUFFER_LENGTH) emu_frames = AUDIO_BUFFER_LENGTH;
#if EMU_AUDIO_CHANNELS == 2
    // Native interleaved stereo maps directly to Retro-Go frames.
    memcpy(i2s_buffer, synthesis_buffer,
           emu_frames * sizeof(rg_audio_sample_t));
#else
    for (int i = 0; i < emu_frames; i++) {
        // PsndGetSamples clears sndOut after this callback, so convert now but
        // defer submission. Duplicate mono samples for the 22.05kHz DAC.
        uint32_t packed = (uint16_t)synthesis_buffer[i] |
                          ((uint32_t)(uint16_t)synthesis_buffer[i] << 16);
        uint32_t *dst32 = (uint32_t *)&i2s_buffer[i * 2];
        dst32[0] = packed;
        dst32[1] = packed;
    }
#endif
    pending_audio_frames = emu_frames * AUDIO_RATE_MULTIPLIER;
}

static int prepare_audio_submit(void)
{
    if (pending_audio_frames > 0)
        return pending_audio_frames;

    // PsndStartFrame maintains the exact fractional PAL/NTSC sample count even
    // when synthesis is disabled or the core did not invoke writeSound.
    int emu_frames = Pico.snd.len_use;
    if (emu_frames < 0) emu_frames = 0;
    if (emu_frames > AUDIO_BUFFER_LENGTH) emu_frames = AUDIO_BUFFER_LENGTH;

    int hw_frames = emu_frames * AUDIO_RATE_MULTIPLIER;
    if (i2s_buffer && hw_frames > 0)
        memset(i2s_buffer, 0, hw_frames * sizeof(rg_audio_sample_t));
    return hw_frames;
}

#if PICODRIVE_PROFILE_LEVEL >= 2
static int profiler_avg_us(enum pprof_points point, unsigned samples,
                           int cpu_mhz)
{
    if (samples == 0 || cpu_mhz <= 0)
        return 0;

    uint64_t divisor = (uint64_t)samples * (unsigned)cpu_mhz;
    return (int)((pprof_counters.cycles[point] + divisor / 2) / divisor);
}

static unsigned profiler_calls_per_tick(enum pprof_points point,
                                        unsigned ticks)
{
    if (ticks == 0)
        return 0;
    return (pprof_counters.calls[point] + ticks / 2) / ticks;
}

#if PICODRIVE_PROFILE_LEVEL >= 3
static const char *profiler_cdda_type(void)
{
#if defined(CONFIG_IDF_TARGET_ESP32)
    return "none";
#else
    if (!(PicoIn.AHW & PAHW_MCD))
        return "none";

    /*
     * A CHD uses one shared pm_file for all tracks. cdd_play_audio() therefore
     * inherits the owning data track's CT_BIN type even when libchdr is
     * supplying audio samples. Report the backing stream type first so the
     * profiler describes the work actually being performed.
     */
    if (Pico_mcd->cdda_stream != NULL &&
        ((pm_file *)Pico_mcd->cdda_stream)->type == PMT_CHD)
        return "chd";

    switch (Pico_mcd->cdda_type) {
        case CT_RAW: return "raw";
        case CT_CHD: return "chd";
        case CT_MP3: return "mp3";
        case CT_WAV: return "wav";
        case CT_OGG: return "ogg";
        default: return "none";
    }
#endif
}
#endif
#endif

#if PICODRIVE_PROFILE_LEVEL >= 1
static void report_performance(void)
{
    if (perf_stats.ticks < PERF_REPORT_TICKS)
        return;

    unsigned ticks = perf_stats.ticks;
    unsigned rendered = perf_stats.rendered;
    unsigned skipped = perf_stats.skipped;
    RG_LOGI("perf hw=%s renderer=%s sound=%s fmfir=%s fifo=%s idle=%s "
            "ticks=%u draw=%u skip=%u late=%u work=%dus "
            "core=%dus core_draw=%dus core_skip=%dus disp_submit=%dus "
            "audio_wait=%dus audio_cb=%u\n",
            (PicoIn.AHW & PAHW_MCD) ? "mcd" : "md",
            fast_renderer ? "fast" : "accurate",
            sound_enabled ? "on" : "off",
            FM_FILTER_ENABLED ? "on" : "off",
            (PicoIn.opt & POPT_DIS_VDP_FIFO) ? "off" : "on",
            no_idle_det ? "off" : "on",
            ticks, rendered, skipped,
            perf_stats.display_late,
            (int)(perf_stats.work_us / ticks),
            (int)(perf_stats.core_us / ticks),
            (int)(rendered ? perf_stats.rendered_core_us / rendered : 0),
            (int)(skipped ? perf_stats.skipped_core_us / skipped : 0),
            (int)(perf_stats.display_submit_us / ticks),
            (int)(perf_stats.audio_wait_us / ticks),
            perf_stats.audio_callbacks);

#if PICODRIVE_PROFILE_LEVEL >= 2
    int cpu_mhz = rg_system_get_cpu_speed();
    /*
     * Retro-Go's logger has a 300-byte local buffer. Keep each profiler line
     * comfortably below that limit; an overlong vsnprintf result otherwise
     * makes its newline append index beyond the buffer.
     */
    RG_LOGI("prof cpu=%dMHz avg_us/tick frame=%d m68k=%d z80=%d "
            "calls/tick m68k=%u z80=%u idle_patches=%d\n",
            cpu_mhz,
            profiler_avg_us(pp_frame, ticks, cpu_mhz),
            profiler_avg_us(pp_m68k, ticks, cpu_mhz),
            profiler_avg_us(pp_z80, ticks, cpu_mhz),
            profiler_calls_per_tick(pp_m68k, ticks),
            profiler_calls_per_tick(pp_z80, ticks),
            SekIdlePatchCount());

    RG_LOGI("prof vdp avg_us/rendered draw=%d scene=%d finish=%d orcopy=%d "
            "sound avg_us/tick tail=%d fm=%d psg=%d out_mix=%d\n",
            profiler_avg_us(pp_draw, rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_scene, rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_finish, rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_orcopy, rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_sound, ticks, cpu_mhz),
            profiler_avg_us(pp_fm, ticks, cpu_mhz),
            profiler_avg_us(pp_psg, ticks, cpu_mhz),
            profiler_avg_us(pp_mix, ticks, cpu_mhz));

    RG_LOGI("prof vdp parts avg_us/rendered planes_lo=%d sprites_lo=%d "
            "planes_hi=%d sprites_hi=%d\n",
            profiler_avg_us(pp_vdp_planes_lo,
                            rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_sprites_lo,
                            rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_planes_hi,
                            rendered ? rendered : ticks, cpu_mhz),
            profiler_avg_us(pp_vdp_sprites_hi,
                            rendered ? rendered : ticks, cpu_mhz));
#endif

#if PICODRIVE_PROFILE_LEVEL >= 3
    RG_LOGI("prof cd avg_us/tick s68k=%d pcm_gen=%d pcm_mix=%d cdda=%d "
            "cdda_read=%d cdda_mix=%d event=%d gfx=%d type=%s "
            "calls/tick s68k=%u pcm_gen=%u event=%u\n",
            profiler_avg_us(pp_s68k, ticks, cpu_mhz),
            profiler_avg_us(pp_pcm_gen, ticks, cpu_mhz),
            profiler_avg_us(pp_pcm_mix, ticks, cpu_mhz),
            profiler_avg_us(pp_cdda, ticks, cpu_mhz),
            profiler_avg_us(pp_cdda_read, ticks, cpu_mhz),
            profiler_avg_us(pp_cdda_mix, ticks, cpu_mhz),
            profiler_avg_us(pp_cd_event, ticks, cpu_mhz),
            profiler_avg_us(pp_cd_gfx, ticks, cpu_mhz),
            profiler_cdda_type(),
            profiler_calls_per_tick(pp_s68k, ticks),
            profiler_calls_per_tick(pp_pcm_gen, ticks),
            profiler_calls_per_tick(pp_cd_event, ticks));

    RG_LOGI("prof chd avg_us/tick hunk=%d io=%d audio_copy=%d "
            "calls/window hunk=%" PRIu32 " io=%" PRIu32 "\n",
            profiler_avg_us(pp_chd_hunk, ticks, cpu_mhz),
            profiler_avg_us(pp_chd_io, ticks, cpu_mhz),
            profiler_avg_us(pp_chd_audio_copy, ticks, cpu_mhz),
            pprof_counters.calls[pp_chd_hunk],
            pprof_counters.calls[pp_chd_io]);
#endif

    memset(&perf_stats, 0, sizeof(perf_stats));
#if PICODRIVE_PROFILE_LEVEL >= 2
    pprof_reset();
#endif
}
#endif

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
    fast_renderer  = rg_settings_get_number(NS_APP, SETTING_FAST_RENDERER, 0);
    no_idle_det    = rg_settings_get_number(NS_APP, SETTING_NO_IDLE_DET, 0);
    six_button_pad = rg_settings_get_number(NS_APP, SETTING_SIX_BUTTON_PAD, 0);
#if !defined(CONFIG_IDF_TARGET_ESP32)
    fm_filter      = rg_settings_get_number(NS_APP, SETTING_FM_FILTER, 0);
#endif
    region_override = rg_settings_get_number(NS_APP, SETTING_REGION, 0);

    /*
     * PicoDrive's indexed renderer layout is 328x256: 240 possible display
     * rows plus eight guard rows above and below. Retro-Go submits only the
     * active 224/240-row viewport, but the complete backing allocation keeps
     * tile and sprite scratch writes away from the adjacent palette.
     */
    updates[0] = rg_surface_create(328, 256, RG_PIXEL_PAL565_LE, MEM_FAST);
    updates[1] = rg_surface_create(328, 256, RG_PIXEL_PAL565_LE, MEM_FAST);
    currentUpdate = updates[0];

    PicoIn.writeSound = write_sound;
    PicoInit();
    
    synthesis_buffer = rg_alloc(AUDIO_BUFFER_LENGTH * EMU_AUDIO_CHANNELS *
                                sizeof(int16_t), MEM_FAST);
    i2s_buffer = rg_alloc(AUDIO_OUTPUT_BUFFER_LENGTH *
                          sizeof(rg_audio_sample_t), MEM_FAST);
    PicoIn.sndOut = synthesis_buffer;
    const char *audio_driver = rg_audio_get_driver();
    compensate_i2s_tail_drop = audio_driver && strcmp(audio_driver, "i2s") == 0;
    RG_LOGI("Audio path: core=%dHz %s, output=%dHz stereo, driver=%s%s\n",
            EMU_AUDIO_SAMPLE_RATE,
            EMU_AUDIO_CHANNELS == 2 ? "stereo" : "mono",
            HW_AUDIO_SAMPLE_RATE,
            audio_driver ? audio_driver : "none",
            compensate_i2s_tail_drop ? " (tail compensated)" : "");

    // PRE-INITIALIZE sndRate to prevent divide-by-zero during media loading
    PicoIn.sndRate = EMU_AUDIO_SAMPLE_RATE;

    /*
     * PicoDrive can stream ZIP entries directly into its final padded ROM
     * allocation. Pre-unzipping here retained a second full ROM image and can
     * exhaust classic ESP32 PSRAM before PicoCartAlloc() runs.
     */
    PicoLoadMedia(app->romPath, NULL, 0, NULL, get_bios_filename, NULL, NULL);

    if (PicoIn.AHW & PAHW_MCD) {
        RG_LOGI("Sega CD Hardware Active.\n");
    }

    sync_pico_settings();
    PicoPower();
    pprof_init();

    if (app->bootFlags & RG_BOOT_RESUME)
    {
        rg_emu_load_state(app->saveSlot);
    }

    int skipFrames = 0;
    bool prevFrameSkipped = true; // Force palette build on first frame

    while (true) {
        const int64_t startTime = rg_system_timer();
        pending_audio_frames = 0;
#if PICODRIVE_PROFILE_LEVEL >= 1
        audio_callbacks_this_tick = 0;
#endif
        int64_t display_submit_us = 0;

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
        // Six-button extension: Genesis X/Y/Z/Mode -> Retro-Go L/X/R/Select.
        if (joystick & RG_KEY_R)      PicoIn.pad[0] |= 0x0100;
        if (joystick & RG_KEY_X)      PicoIn.pad[0] |= 0x0200;
        if (joystick & RG_KEY_L)      PicoIn.pad[0] |= 0x0400;
        if (joystick & RG_KEY_SELECT) PicoIn.pad[0] |= 0x0800;

        bool drawFrame = (skipFrames == 0);

        PicoIn.skipFrame = !drawFrame;
        if (drawFrame) {
            if (prevFrameSkipped)
                Pico.m.dirtyPal = 1;
            PicoDrawSetOutBufMD(currentUpdate->data, currentUpdate->stride);
            PicoDraw2SetOutBuf(currentUpdate->data, currentUpdate->stride);
        }
        
#if PICODRIVE_PROFILE_LEVEL >= 1
        int64_t core_start = rg_system_timer();
#endif
        PicoFrame();
#if PICODRIVE_PROFILE_LEVEL >= 1
        int64_t core_us = rg_system_timer() - core_start;
#endif
        
        if (drawFrame) {
            int width = (Pico.video.reg[12] & 1) ? 320 : 256;
            int height = (Pico.m.pal && (Pico.video.reg[1] & 8)) ? 240 : 224;
            
            if (width != currentUpdate->width || height != currentUpdate->height) {
                currentUpdate->width = width;
                currentUpdate->height = height;
            }

            PicoDrawUpdateHighPal();
            memcpy(currentUpdate->palette, Pico.est.HighPal,
                   256 * sizeof(currentUpdate->palette[0]));

            int x_offset = 8;
            int y_offset = (height == 224) ? 8 : 0;
            currentUpdate->offset = y_offset * currentUpdate->stride + x_offset;

            int64_t submit_start = rg_system_timer();
            rg_display_submit(currentUpdate, 0);
            display_submit_us = rg_system_timer() - submit_start;
            lastUpdate = currentUpdate;
            currentUpdate = updates[currentUpdate == updates[0]];
        }

        // Dynamically update Retro-Go's tick rate depending on NTSC vs PAL game region
        int target_fps = Pico.m.pal ? 50 : 60;
        if (rg_system_get_tick_rate() != target_fps) {
            rg_system_set_tick_rate(target_fps);
        }

        int audio_frames = prepare_audio_submit();
        int64_t work_us = rg_system_timer() - startTime -
                          display_submit_us;
        if (work_us < 0) work_us = 0;
        int64_t active_us = work_us + display_submit_us;
        bool slowFrame = active_us > app->frameTime + 1500;
        rg_system_tick(work_us);

#if PICODRIVE_PROFILE_LEVEL >= 1
        int64_t audio_start = rg_system_timer();
#endif
        if (i2s_buffer && audio_frames > 0) {
            int submit_frames = audio_frames;
            if (compensate_i2s_tail_drop &&
                audio_frames < AUDIO_OUTPUT_BUFFER_LENGTH) {
                // Retro-Go's current I2S final partial write omits its last
                // frame. Append a duplicate that can be safely discarded.
                i2s_buffer[audio_frames] = i2s_buffer[audio_frames - 1];
                submit_frames++;
            }
            rg_audio_submit(i2s_buffer, submit_frames);
        }
#if PICODRIVE_PROFILE_LEVEL >= 1
        int64_t audio_wait_us = rg_system_timer() - audio_start;
#endif

#if PICODRIVE_PROFILE_LEVEL >= 1
        perf_stats.ticks++;
        perf_stats.rendered += drawFrame;
        perf_stats.skipped += !drawFrame;
        perf_stats.display_late += slowFrame;
        perf_stats.work_us += work_us;
        perf_stats.display_submit_us += display_submit_us;
        perf_stats.audio_callbacks += audio_callbacks_this_tick;
        perf_stats.core_us += core_us;
        if (drawFrame)
            perf_stats.rendered_core_us += core_us;
        else
            perf_stats.skipped_core_us += core_us;
        perf_stats.audio_wait_us += audio_wait_us;
        report_performance();
#endif

        prevFrameSkipped = !drawFrame;

        if (skipFrames == 0) {
            if (app->frameskip > 0) {
                skipFrames = app->frameskip;
            } else if (slowFrame) {
                skipFrames = 1;
            }
        } else if (skipFrames > 0) {
            skipFrames--;
        }
    }
}
