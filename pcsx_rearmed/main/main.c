#include <rg_system.h>
#include <rg_surface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Fix MAXPATHLEN conflict
#include <sys/param.h>
#undef MAXPATHLEN
#include "../components/pcsx_rearmed/include/config.h"

#undef _
#include "../components/pcsx_rearmed/libpcsxcore/psxcommon.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxcounters.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxmem.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxmem_map.h"

// Fix EPC conflict on Xtensa
#ifdef EPC
#undef EPC
#endif
#include "../components/pcsx_rearmed/libpcsxcore/r3000a.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxinterpreter.h"
#include "../components/pcsx_rearmed/frontend/plat.h"
#include "../components/pcsx_rearmed/frontend/plugin_lib.h"
#include "../components/pcsx_rearmed/libpcsxcore/misc.h"
#include "../components/pcsx_rearmed/libpcsxcore/plugins.h"
#include "../components/pcsx_rearmed/libpcsxcore/sio.h"
#include "../components/pcsx_rearmed/libpcsxcore/cdrom-async.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxbios.h"

/* The engine's psxcommon.h uses an identity fallback for this macro. */
#undef _
#define _(String) rg_gettext(String)

#define MAP_FAILED ((void *)-1)
#define PSX_SAVE_DIR RG_BASE_PATH_SAVES "/psx"
#define PSX_CARD1_PATH PSX_SAVE_DIR "/pcsx-card1.mcd"

/* Real BIOS support is retained below for future investigation, but the
 * current P4 port always boots with HLE. */
#define PCSX_REAL_BIOS_ENABLED 0


rg_surface_t *display_surface = NULL;
static rg_app_t *app;
static bool sound_enabled = false;
static bool sound_fast_mode = true;
static int cycle_multiplier = 400;
#ifdef PCSX_DUAL_DYNAREC
static bool cpu_use_rv32 = false;
static bool cpu_use_rv32_active = false;
#endif
static bool core_ready = false;
static const char *const SETTING_SOUND_ENABLED = "SoundEnabled";
static const char *const SETTING_SOUND_QUALITY = "SoundFast";
static const char *const SETTING_CYCLE_MULTIPLIER = "CycleMultiplier";
#ifdef PCSX_DUAL_DYNAREC
static const char *const SETTING_CPU_CORE = "CpuCoreRv32";
#endif

extern struct rearmed_cbs pl_rearmed_cbs;
extern void emu_set_default_config(void);
extern void retrogo_spu_set_options(bool enabled, bool fast_mode);
extern int OpenPlugins(void);
extern void SysReset(void);
extern unsigned short in_keystate[8];
extern unsigned int frame_counter;
extern uint64_t retrogo_display_wait_us;

static void report_memory_card_error(void)
{
    const char *message = sioTakeCardError();
    if (message)
        rg_gui_alert(_("Memory card error"), message);
}
#ifdef PCSX_PORT_PROFILE
extern volatile uint32_t retrogo_vout_flip_count;
extern void builtin_GPUgetProfile(uint64_t *render_us, uint64_t *scanout_us,
                                 uint32_t *render_calls,
                                 uint32_t *scanout_calls);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#ifdef PCSX_NDRC_RV32_FULL_CORE
extern int ndrc_dbg_trace_begin(const char *path);
extern void ndrc_dbg_trace_end(void);
extern int ndrc_dbg_compare_begin(const char *path);
extern int ndrc_dbg_stopped(void);
extern void builtin_SPUresetDiagnostic(void);
#endif
#if defined(PCSX_PORT_PROFILE) && (defined(PCSX_NDRC_RV32_FULL_CORE) || defined(PCSX_DUAL_DYNAREC))
extern void new_dynarec_print_stats(void);
#endif
extern void lightrec_plugin_debug_poll(void);
extern bool lightrec_plugin_failed(void);
#ifdef PCSX_PORT_PROFILE
extern void lightrec_plugin_get_profile(uint64_t *gte_cycles,
                                        uint32_t *gte_calls,
                                        uint32_t *gte_no_flags_calls,
                                        uint64_t opcode_cycles[64],
                                        uint32_t opcode_calls[64]);
#ifdef PCSX_DUAL_DYNAREC
extern void rv32_plugin_get_profile(uint64_t *gte_cycles,
                                    uint32_t *gte_calls,
                                    uint32_t *gte_no_flags_calls,
                                    uint64_t opcode_cycles[64],
                                    uint32_t opcode_calls[64]);
#endif
extern void renderer_get_profile(uint64_t group_cycles[12],
                                 uint32_t group_calls[12]);
extern void renderer_get_ft4_profile(uint16_t cf[3], uint64_t cycles[3],
                                     uint32_t calls[3], uint32_t *overflow);
extern void gpu_async_get_profile(uint64_t wait_us[3],
                                  uint32_t wait_calls[3]);
extern void builtin_GPUgetChainProfile(uint64_t *parse_cycles,
                                       uint32_t *nodes,
                                       uint32_t *words,
                                       uint32_t *parse_calls);
static const char *const renderer_group_names[12] = {
    "FILL", "F3", "FT3", "F4", "FT4", "G3",
    "GT3", "G4", "GT4", "LINE", "TILE", "OTHER"
};
#endif

#ifdef PCSX_NDRC_RV32_FULL_CORE
#define RV32_DIFF_PAGE_OFFSET 0x0007b000u
#define RV32_DIFF_PAGE_WORDS (0x1000u / sizeof(uint32_t))
#define RV32_DIFF_WATCH_OFFSET 0x0007b7a8u
#define RV32_DIFF_WATCH_WORDS 6
#define RV32_DIFF_MAX_FRAMES 100

static uint32_t rv32_diff_page_hash(void)
{
    const uint32_t *words = (const uint32_t *)
        (psxRegs.ptrs.psxM + RV32_DIFF_PAGE_OFFSET);
    uint32_t hash = 2166136261u;

    for (unsigned int i = 0; i < RV32_DIFF_PAGE_WORDS; i++)
        hash = (hash ^ words[i]) * 16777619u;
    return hash;
}
#endif
#endif

static bool save_state_handler(const char *filename)
{
    if (!core_ready)
        return false;
    RG_LOGI("Saving state: %s", filename);
    return SaveState(filename) == 0;
}

static bool load_state_handler(const char *filename)
{
    if (!core_ready)
        return false;
    RG_LOGI("Loading state: %s", filename);
    return LoadState(filename) == 0;
}

static bool reset_handler(bool hard)
{
    (void)hard;
    if (!core_ready)
        return false;

    RG_LOGI("Resetting emulated PlayStation");
    SysReset();
    if (LoadCdrom() != 0) {
        RG_LOGE("LoadCdrom failed after reset");
        return false;
    }
    psxBiosSetupBootState();
    return true;
}

static bool screenshot_handler(const char *filename, int width, int height)
{
    if (!display_surface)
        return false;
    return rg_surface_save_image_file(display_surface, filename,
                                      width, height);
}

static void event_handler(int event, void *arg)
{
    (void)arg;
    if (event == RG_EVENT_REDRAW && display_surface) {
        while (rg_display_is_busy())
            rg_task_yield();
        rg_display_submit(display_surface, 0);
    } else if (event == RG_EVENT_SHUTDOWN) {
        rg_audio_set_mute(true);
        if (core_ready) {
            SaveMcd(Config.Mcd1, Mcd1Data, 0, MCD_SIZE);
            rg_storage_commit();
            report_memory_card_error();
        }
    }
}

static void *wrap_psxMapHook(unsigned long addr, size_t size, enum psxMapTag tag, int *can_retry_addr) {
    size_t alignment = 512 * 1024;
    void *ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#ifdef PCSX_PORT_PROFILE
    RG_LOGI("psxMapHook: addr=%lx size=%x tag=%d -> ptr=%p", addr, (int)size, (int)tag, ptr);
#endif
    *can_retry_addr = 1;
    return ptr ? ptr : MAP_FAILED;
}

static void wrap_psxUnmapHook(void *ptr, size_t size, enum psxMapTag tag) {
#ifdef PCSX_PORT_PROFILE
    RG_LOGI("psxUnmapHook: ptr=%p size=%x", ptr, (int)size);
#endif
    if (ptr && ptr != MAP_FAILED) free(ptr);
}

static rg_gui_event_t sound_enabled_cb(rg_gui_option_t *option,
                                       rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        sound_enabled = !sound_enabled;
        rg_settings_set_number(NS_APP, SETTING_SOUND_ENABLED, sound_enabled);
        retrogo_spu_set_options(sound_enabled, sound_fast_mode);
    }
    strcpy(option->value, sound_enabled ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t sound_quality_cb(rg_gui_option_t *option,
                                       rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        sound_fast_mode = !sound_fast_mode;
        rg_settings_set_number(NS_APP, SETTING_SOUND_QUALITY,
                               sound_fast_mode);
        retrogo_spu_set_options(sound_enabled, sound_fast_mode);
    }
    strcpy(option->value, sound_fast_mode ? _("Fast") : _("Accurate"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t cycle_multiplier_cb(rg_gui_option_t *option,
                                           rg_gui_event_t event)
{
    static const int values[] = {200, 250, 300, 350, 400};
    int index = 0;

    while (index + 1 < (int)(sizeof(values) / sizeof(values[0]))
           && values[index] != cycle_multiplier)
        index++;

    if (event == RG_DIALOG_PREV && index > 0)
        index--;
    else if (event == RG_DIALOG_NEXT
             && index + 1 < (int)(sizeof(values) / sizeof(values[0])))
        index++;

    if (cycle_multiplier != values[index]) {
        cycle_multiplier = values[index];
        Config.cycle_multiplier = cycle_multiplier;
        rg_settings_set_number(NS_APP, SETTING_CYCLE_MULTIPLIER,
                               cycle_multiplier);
        /* Both the interpreter and new_dynarec implement this hook. The
         * latter discards timing-dependent compiled blocks before applying
         * the new multiplier. */
        if (psxCpu != NULL && psxCpu->ApplyConfig != NULL)
            psxCpu->ApplyConfig();
        RG_LOGI("CPU cycle multiplier changed to %d.%d",
                cycle_multiplier / 100, (cycle_multiplier / 10) % 10);
    }

    sprintf(option->value, "%d.%d",
            cycle_multiplier / 100, (cycle_multiplier / 10) % 10);
    return RG_DIALOG_VOID;
}

#ifdef PCSX_DUAL_DYNAREC
static rg_gui_event_t cpu_core_cb(rg_gui_option_t *option,
                                  rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        cpu_use_rv32 = !cpu_use_rv32;
        rg_settings_set_number(NS_APP, SETTING_CPU_CORE, cpu_use_rv32);
        RG_LOGI("CPU core changed to %s (applies next launch)",
                cpu_use_rv32 ? "RV32 dynarec" : "Lightrec");
    }
    strcpy(option->value, cpu_use_rv32 ? _("RV32 dynarec") : _("Lightrec"));
    return RG_DIALOG_VOID;
}
#endif

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Sound"), "-",
            RG_DIALOG_FLAG_NORMAL, &sound_enabled_cb};
    *dest++ = (rg_gui_option_t){0, _("Sound quality"), "-",
            RG_DIALOG_FLAG_NORMAL, &sound_quality_cb};
    *dest++ = (rg_gui_option_t){0, _("CPU cycle multiplier"), "-",
            RG_DIALOG_FLAG_NORMAL, &cycle_multiplier_cb};
#ifdef PCSX_DUAL_DYNAREC
    *dest++ = (rg_gui_option_t){0, _("CPU core (next launch)"), "-",
            RG_DIALOG_FLAG_NORMAL, &cpu_core_cb};
#endif
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

#if defined(CONFIG_IDF_TARGET_ESP32P4) && PCSX_REAL_BIOS_ENABLED
typedef struct {
    char name[MAXPATHLEN];
} bios_search_t;

static bool ascii_ends_with(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > text_len)
        return false;
    text += text_len - suffix_len;
    while (*suffix) {
        if (tolower((unsigned char)*text++) !=
            tolower((unsigned char)*suffix++))
            return false;
    }
    return true;
}

static int find_bios_cb(const rg_scandir_t *file, void *arg)
{
    bios_search_t *result = arg;
    if (file->is_file && file->size == 512 * 1024
        && (ascii_ends_with(file->basename, ".bin")
            || ascii_ends_with(file->basename, ".rom"))) {
        snprintf(result->name, sizeof(result->name), "%s", file->basename);
        return RG_SCANDIR_STOP;
    }
    return RG_SCANDIR_CONTINUE;
}

static void configure_bios(void)
{
    static const char *const preferred[] = {
        "psxonpsp660.bin", "scph101.bin", "scph5501.bin",
        "scph7001.bin", "scph1001.bin"
    };
    char path[RG_PATH_MAX + 1];
    bios_search_t found = {{0}};

    snprintf(Config.BiosDir, sizeof(Config.BiosDir), "%s",
             RG_BASE_PATH_BIOS);
    snprintf(Config.Bios, sizeof(Config.Bios), "HLE");
    Config.HLE = 1;

    rg_storage_mkdir(RG_BASE_PATH_BIOS);
    for (unsigned int i = 0;
         i < sizeof(preferred) / sizeof(preferred[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s",
                 RG_BASE_PATH_BIOS, preferred[i]);
        rg_stat_t stat = rg_storage_stat(path);
        if (stat.is_file && stat.size == 512 * 1024) {
            snprintf(found.name, sizeof(found.name), "%s", preferred[i]);
            break;
        }
    }

    if (found.name[0] == '\0')
        rg_storage_scandir(RG_BASE_PATH_BIOS, find_bios_cb, &found,
                           RG_SCANDIR_FILES | RG_SCANDIR_STAT |
                           RG_SCANDIR_SORT);

    if (found.name[0] != '\0') {
        snprintf(Config.Bios, sizeof(Config.Bios), "%s", found.name);
        Config.HLE = 0;
        RG_LOGI("BIOS: using %s/%s", Config.BiosDir, Config.Bios);
    } else {
        RG_LOGW("BIOS: no valid 512 KiB image found in %s; using HLE",
                Config.BiosDir);
    }
}
#else
static void configure_bios(void)
{
    snprintf(Config.BiosDir, sizeof(Config.BiosDir), "%s", RG_BASE_PATH_BIOS);
    snprintf(Config.Bios, sizeof(Config.Bios), "HLE");
    Config.HLE = 1;
    RG_LOGI("BIOS: HLE");
}
#endif

static void configure_memory_cards(void)
{
    /* Match physical PSX behavior: all games share the card inserted in slot
     * 1. This also avoids allocating a separate 128 KiB image per title. */
    bool folder_ready = rg_storage_mkdir(PSX_SAVE_DIR);
    if (!folder_ready)
        RG_LOGE("Unable to create memory-card folder: %s", PSX_SAVE_DIR);

    snprintf(Config.Mcd1, sizeof(Config.Mcd1),
             PSX_CARD1_PATH);
    snprintf(Config.Mcd2, sizeof(Config.Mcd2), "none");

    RG_LOGI("Memory card 1: %s", Config.Mcd1);
    RG_LOGI("Memory card 2: disabled");
}

// Platform hooks for PCSX-ReARMed
void plat_init(void) {}
void plat_finish(void) {}
void plat_minimize(void) {}
void *plat_prepare_screenshot(int *w, int *h, int *bpp) { return NULL; }
void plat_gvideo_open(int is_pal) {}
void *plat_gvideo_set_mode(int *w, int *h, int *bpp) {
    *w = 320; *h = 240; *bpp = 16;
    if (display_surface) return display_surface->data;
    return NULL;
}
void *plat_gvideo_flip(void) {
    if (display_surface) rg_display_submit(display_surface, 0);
    return display_surface ? display_surface->data : NULL;
}
void plat_gvideo_close(void) {}

void emu_task(void *pvParameters) {
    RG_LOGI("Emulator Task starting initialization...");

    emu_set_default_config();
    retrogo_spu_set_options(sound_enabled, sound_fast_mode);
    configure_memory_cards();
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    Config.Cpu = CPU_DYNAREC;
#ifdef PCSX_DUAL_DYNAREC
    cpu_use_rv32_active = cpu_use_rv32;
    psxSelectRv32Dynarec(cpu_use_rv32_active);
    RG_LOGI("CPU core: %s", cpu_use_rv32_active ? "RV32 dynarec" : "Lightrec");
#elif defined(PCSX_NDRC_RV32_FULL_CORE)
    RG_LOGI("CPU core: RV32 dynarec");
#else
    RG_LOGI("CPU core: Lightrec");
#endif
#else
    Config.Cpu = CPU_INTERPRETER;
#endif
    Config.PsxType = PSX_TYPE_NTSC;
    Config.SlowBoot = 0;
    configure_bios();
    
    /* This is cycles charged per emulated instruction: a higher value
     * underclocks the guest CPU and reduces host work. */
    Config.cycle_multiplier = cycle_multiplier;
    
    psxMapHook = wrap_psxMapHook;
    psxUnmapHook = wrap_psxUnmapHook;

    strcpy(Config.Gpu, "builtin_gpu");
    strcpy(Config.Spu, "builtin_spu");
    SetIsoFile(app->romPath);

    RG_LOGI("Loading plugins...");
    if (LoadPlugins() == -1) RG_PANIC("LoadPlugins failed");
    
    RG_LOGI("Initializing core...");
    if (psxInit() != 0) RG_PANIC("psxInit failed");
    
    RG_LOGI("Opening plugins...");
    if (OpenPlugins() == -1) RG_PANIC("OpenPlugins failed");

    if (CheckCdrom() != 0)
        RG_PANIC("Unable to recognize the game disc");
    LoadMcds(Config.Mcd1, Config.Mcd2);
    
    RG_LOGI("Resetting core...");
    SysReset();

    /* Preserve the tested GPU policy: do not discard guest render updates
     * merely because the host cannot reach the console's VBlank rate. */
    pl_rearmed_cbs.frameskip = -1;
    pl_rearmed_cbs.fskip_force = pl_rearmed_cbs.fskip_advice = 0;

    RG_LOGI("Loading CDROM EXE from %s...", app->romPath);
    if (LoadCdrom() != 0)
        RG_PANIC("Unable to load the game executable");
    
    psxBiosSetupBootState();
    core_ready = true;

    /* Audio provides backpressure when enabled. Keep a conditional absolute
     * deadline limiter for silent/fast frames, without adding a fixed sleep. */
    int tick_rate = (int)(psxGetFps() + 0.5);
    rg_system_set_tick_rate(tick_rate);
    int64_t next_frame_deadline = rg_system_timer() + app->frameTime;
    RG_LOGI("Emulation pacing: %d Hz (%d us/tick)",
            tick_rate, app->frameTime);

#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(PCSX_NDRC_RV32_FULL_CORE)
    /* Upstream Ari64's DRC_DBG compares the recompiler against an interpreter
     * trace. Record an extended startup window, reset the complete machine,
     * then replay that reference from RV32. This is independent of the game
     * executable address and stops at the first persistent architectural
     * mismatch instead of relying on title-specific handoff points. */
    const char *diff_path = RG_BASE_PATH_CACHE "/pcsx-rv32-diff.trace";
    const unsigned int diff_initial_frame_counter = frame_counter;
    static uint32_t diff_page_hashes[RV32_DIFF_MAX_FRAMES];
    static uint32_t diff_page_cycles[RV32_DIFF_MAX_FRAMES];
    static uint32_t diff_watch_words[RV32_DIFF_MAX_FRAMES]
                                    [RV32_DIFF_WATCH_WORDS];
    unsigned int diff_reference_frames = 0;
    unsigned int diff_replay_frame = 0;
    bool diff_page_failed = false;
    if (ndrc_dbg_trace_begin(diff_path) == 0) {
        RG_LOGI("RV32 DIFF: recording cycles 23700000..34000000 (128 MiB cap)...");
        /* HLE softCall/softCallInException dispatch through psxCpu, not the
         * outer Execute function. Keep nested execution in the reference
         * CPU too, otherwise native callbacks disappear from the trace. */
        R3000Acpu *replay_cpu = psxCpu;
        psxCpu = &psxInt;
        psxInt.Init();
        psxInt.Reset();
        for (int reference_frame = 0; reference_frame < 100; reference_frame++) {
            psxRegs.stop = 0;
            psxInt.Execute(&psxRegs);
            diff_page_hashes[reference_frame] = rv32_diff_page_hash();
            diff_page_cycles[reference_frame] = psxRegs.cycle;
            memcpy(diff_watch_words[reference_frame],
                   psxRegs.ptrs.psxM + RV32_DIFF_WATCH_OFFSET,
                   sizeof(diff_watch_words[reference_frame]));
            diff_reference_frames = reference_frame + 1;
            rg_system_tick(0);
            if (ndrc_dbg_stopped())
                break;
            if ((reference_frame + 1) % 10 == 0)
                RG_LOGI("RV32 DIFF: reference frame %d/100", reference_frame + 1);
            rg_task_delay(10);
        }
        psxInt.Shutdown();
        ndrc_dbg_trace_end();
        psxCpu = replay_cpu;

        RG_LOGI("RV32 DIFF: resetting for native replay...");
        /* Restore cold SPU state before BIOS setup writes its defaults. */
        builtin_SPUresetDiagnostic();
        SysReset();
        if (LoadCdrom() == -1)
            RG_LOGE("RV32 DIFF: LoadCdrom replay failed");
        psxBiosSetupBootState();
        /* psxRcntInit resets hSyncCount but deliberately retains the global
         * frame counter. Its parity controls GPU status bits 31 and 13 in
         * interlaced modes, so replay must restore the reference's baseline. */
        RG_LOGI("RV32 DIFF: restoring frame counter %u -> %u for matching GPU field parity",
                frame_counter, diff_initial_frame_counter);
        frame_counter = diff_initial_frame_counter;
        ndrc_dbg_compare_begin(diff_path);
    }
#endif

    if (app->bootFlags & RG_BOOT_RESUME) {
        RG_LOGI("Restoring Retro-Go state slot %u", app->saveSlot);
        if (!rg_emu_load_state(app->saveSlot))
            RG_LOGW("Unable to restore state slot %u", app->saveSlot);
    }
    report_memory_card_error();
    next_frame_deadline = rg_system_timer() + app->frameTime;

    RG_LOGI("Emulation loop starting at PC: 0x%08x", (unsigned int)psxRegs.pc);

#ifdef PCSX_PORT_PROFILE
    int frame_count = 0;
    uint64_t profile_total_us = 0;
#endif
    while (true) {
        int64_t start_time = rg_system_timer();
        int64_t audio_busy_start = rg_audio_get_counters().busyTime;
        uint64_t display_wait_start = retrogo_display_wait_us;

        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION)) {
            in_keystate[0] = 0;
            if (joystick & RG_KEY_MENU)
                rg_gui_game_menu();
            else
                rg_gui_options_menu();
            pl_rearmed_cbs.fskip_force = pl_rearmed_cbs.fskip_advice = 0;
            next_frame_deadline = rg_system_timer() + app->frameTime;
            continue;
        }

        in_keystate[0] = 0;
        if (joystick & RG_KEY_UP)     in_keystate[0] |= (1 << DKEY_UP);
        if (joystick & RG_KEY_DOWN)   in_keystate[0] |= (1 << DKEY_DOWN);
        if (joystick & RG_KEY_LEFT)   in_keystate[0] |= (1 << DKEY_LEFT);
        if (joystick & RG_KEY_RIGHT)  in_keystate[0] |= (1 << DKEY_RIGHT);
        if (joystick & RG_KEY_SELECT) in_keystate[0] |= (1 << DKEY_SELECT);
        if (joystick & RG_KEY_START)  in_keystate[0] |= (1 << DKEY_START);
        if (joystick & RG_KEY_A)      in_keystate[0] |= (1 << DKEY_CIRCLE);
        if (joystick & RG_KEY_B)      in_keystate[0] |= (1 << DKEY_CROSS);
        if (joystick & RG_KEY_X)      in_keystate[0] |= (1 << DKEY_TRIANGLE);
        if (joystick & RG_KEY_Y)      in_keystate[0] |= (1 << DKEY_SQUARE);
        if (joystick & RG_KEY_L)      in_keystate[0] |= (1 << DKEY_L1);
        if (joystick & RG_KEY_R)      in_keystate[0] |= (1 << DKEY_R1);

        /* PCSX calls pl_frame_limit() from EmuUpdate() at VBlank. The
         * Retro-Go implementation sets stop there, so Execute() runs one
         * emulated frame while dynarecs remain inside their dispatcher until
         * each scheduled event. Driving ExecuteBlock() here forces a full
         * register sync and C transition for every guest branch block. */
        psxRegs.stop = 0;
        psxCpu->Execute(&psxRegs);

#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(PCSX_NDRC_RV32_FULL_CORE)
        if (!diff_page_failed && diff_replay_frame < diff_reference_frames) {
            uint32_t actual_hash = rv32_diff_page_hash();
            uint32_t expected_hash = diff_page_hashes[diff_replay_frame];

            if (actual_hash != expected_hash) {
                const uint32_t *actual_words = (const uint32_t *)
                    (psxRegs.ptrs.psxM + RV32_DIFF_WATCH_OFFSET);
                RG_LOGE("RV32 DIFF PAGE: first mismatch after frame %u: "
                        "cycle=%u/%u hash=%08x/%08x",
                        diff_replay_frame + 1, (unsigned int)psxRegs.cycle,
                        (unsigned int)diff_page_cycles[diff_replay_frame],
                        (unsigned int)actual_hash,
                        (unsigned int)expected_hash);
                for (unsigned int i = 0; i < RV32_DIFF_WATCH_WORDS; i++)
                    RG_LOGE("RV32 DIFF PAGE word %u @%08x: %08x/%08x",
                            i, 0x80000000u + RV32_DIFF_WATCH_OFFSET + i * 4,
                            (unsigned int)actual_words[i],
                            (unsigned int)diff_watch_words[diff_replay_frame][i]);
                diff_page_failed = true;
            }
            diff_replay_frame++;
        }
        if (ndrc_dbg_stopped() || diff_page_failed) {
            RG_LOGI("RV32 DIFF: replay stopped; capture the comparison result above.");
            /* Preserve the first failure instead of clearing stop and
             * continuing with a state the comparator has rejected. */
            while (true) {
                if (rg_input_read_gamepad() & RG_KEY_MENU)
                    rg_gui_game_menu();
                rg_system_tick(0);
                rg_task_delay(20);
            }
        }
#endif

        /* A fatal core exit cannot make progress. Do not restart it forever. */
#if defined(CONFIG_IDF_TARGET_ESP32P4)
        if (psxCpu == &psxRec && lightrec_plugin_failed()) {
            RG_LOGE("CPU core stopped; returning to launcher.");
            rg_system_exit();
        }
#endif

        int64_t elapsed_us = rg_system_timer() - start_time;
        int64_t wait_us = rg_audio_get_counters().busyTime - audio_busy_start +
            (int64_t)(retrogo_display_wait_us - display_wait_start);
        int64_t busy_us = elapsed_us > wait_us ? elapsed_us - wait_us : 0;
#ifdef PCSX_PORT_PROFILE
        profile_total_us += elapsed_us;

        if (++frame_count % 60 == 0) {
            uint64_t gpu_render_us, gpu_scanout_us;
            uint32_t gpu_render_calls, gpu_scanout_calls;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
            uint64_t gte_cycles;
            uint32_t gte_calls;
            uint32_t gte_no_flags_calls;
            uint64_t gte_opcode_cycles[64];
            uint32_t gte_opcode_calls[64];
            uint32_t gte_top[3] = {0, 0, 0};
            uint64_t renderer_group_cycles[12];
            uint32_t renderer_group_calls[12];
            uint32_t renderer_top[3] = {12, 12, 12};
            uint16_t ft4_cf[3];
            uint64_t ft4_cycles[3];
            uint32_t ft4_calls[3];
            uint32_t ft4_overflow;
            uint64_t async_wait_us[3];
            uint32_t async_wait_calls[3];
            uint64_t gpu_chain_parse_cycles;
            uint32_t gpu_chain_nodes;
            uint32_t gpu_chain_words;
            uint32_t gpu_chain_parse_calls;
#endif

            builtin_GPUgetProfile(&gpu_render_us, &gpu_scanout_us,
                                  &gpu_render_calls, &gpu_scanout_calls);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#ifdef PCSX_DUAL_DYNAREC
            if (cpu_use_rv32_active)
                rv32_plugin_get_profile(&gte_cycles, &gte_calls,
                                        &gte_no_flags_calls,
                                        gte_opcode_cycles,
                                        gte_opcode_calls);
            else
#endif
                lightrec_plugin_get_profile(&gte_cycles, &gte_calls,
                                            &gte_no_flags_calls,
                                            gte_opcode_cycles,
                                            gte_opcode_calls);
            for (uint32_t op = 0; op < 64; op++) {
                for (uint32_t rank = 0; rank < 3; rank++) {
                    if (gte_opcode_cycles[op] >
                        gte_opcode_cycles[gte_top[rank]]) {
                        for (uint32_t move = 2; move > rank; move--)
                            gte_top[move] = gte_top[move - 1];
                        gte_top[rank] = op;
                        break;
                    }
                }
            }
            renderer_get_profile(renderer_group_cycles,
                                 renderer_group_calls);
            for (uint32_t group = 0; group < 12; group++) {
                for (uint32_t rank = 0; rank < 3; rank++) {
                    if (renderer_top[rank] == 12 ||
                        renderer_group_cycles[group] >
                        renderer_group_cycles[renderer_top[rank]]) {
                        for (uint32_t move = 2; move > rank; move--)
                            renderer_top[move] = renderer_top[move - 1];
                        renderer_top[rank] = group;
                        break;
                    }
                }
            }
            renderer_get_ft4_profile(ft4_cf, ft4_cycles, ft4_calls,
                                     &ft4_overflow);
            gpu_async_get_profile(async_wait_us, async_wait_calls);
            builtin_GPUgetChainProfile(&gpu_chain_parse_cycles,
                                       &gpu_chain_nodes,
                                       &gpu_chain_words,
                                       &gpu_chain_parse_calls);
#endif
            RG_LOGI("Loop: %d PC=%08x Op=%08x Cycle=%u next=%u IRQ=%08x "
                    "I=%08x/%08x GPU=%u Vout=%u ra=%08x sp=%08x "
                    "CP0=%08x/%08x/%08x/%08x "
                    "Time=%u us/f Gpu=%u+%u us/f calls=%u+%u",
                    frame_count, (unsigned int)psxRegs.pc,
                    (unsigned int)intFakeFetch(psxRegs.pc),
                    (unsigned int)psxRegs.cycle,
                    (unsigned int)psxRegs.next_interupt,
                    (unsigned int)psxRegs.interrupt,
                    (unsigned int)psxHu32(0x1070),
                    (unsigned int)psxHu32(0x1074),
                    frame_counter,
                    (unsigned int)retrogo_vout_flip_count,
                    (unsigned int)psxRegs.GPR.n.ra,
                    (unsigned int)psxRegs.GPR.n.sp,
                    (unsigned int)psxRegs.CP0.n.Cause,
                    (unsigned int)psxRegs.CP0.n.EPC,
                    (unsigned int)psxRegs.CP0.n.BadVAddr,
                    (unsigned int)psxRegs.CP0.n.SR,
                    (unsigned int)(profile_total_us / 60),
                    (unsigned int)(gpu_render_us / 60),
                    (unsigned int)(gpu_scanout_us / 60),
                    (unsigned int)gpu_render_calls,
                    (unsigned int)gpu_scanout_calls);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
            /* rg_system_vlog() has a fixed 300-byte buffer and historically
             * trusts vsnprintf's would-have-written length. Keep optional
             * profiler output separate from the already long frame report so
             * a large counter cannot make that shared logger overrun itself. */
            if (gte_calls != 0)
                RG_LOGI("GTE: %u us/f calls=%u nf=%u%% "
                        "top=%02x:%u/%u,%02x:%u/%u,%02x:%u/%u",
                        (unsigned int)(gte_cycles /
                            (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                        (unsigned int)gte_calls,
                        (unsigned int)((gte_no_flags_calls * 100u) / gte_calls),
                        (unsigned int)gte_top[0],
                        (unsigned int)(gte_opcode_cycles[gte_top[0]] /
                            (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                        (unsigned int)(gte_opcode_calls[gte_top[0]] / 60),
                        (unsigned int)gte_top[1],
                        (unsigned int)(gte_opcode_cycles[gte_top[1]] /
                            (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                        (unsigned int)(gte_opcode_calls[gte_top[1]] / 60),
                        (unsigned int)gte_top[2],
                        (unsigned int)(gte_opcode_cycles[gte_top[2]] /
                            (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                        (unsigned int)(gte_opcode_calls[gte_top[2]] / 60));
            RG_LOGI("UNAI: top=%s:%u/%u,%s:%u/%u,%s:%u/%u",
                    renderer_group_names[renderer_top[0]],
                    (unsigned int)(renderer_group_cycles[renderer_top[0]] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(renderer_group_calls[renderer_top[0]] / 60),
                    renderer_group_names[renderer_top[1]],
                    (unsigned int)(renderer_group_cycles[renderer_top[1]] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(renderer_group_calls[renderer_top[1]] / 60),
                    renderer_group_names[renderer_top[2]],
                    (unsigned int)(renderer_group_cycles[renderer_top[2]] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(renderer_group_calls[renderer_top[2]] / 60));
            RG_LOGI("FT4: cf=%03x:%u/%u,%03x:%u/%u,%03x:%u/%u overflow=%u",
                    (unsigned int)ft4_cf[0],
                    (unsigned int)(ft4_cycles[0] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(ft4_calls[0] / 60),
                    (unsigned int)ft4_cf[1],
                    (unsigned int)(ft4_cycles[1] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(ft4_calls[1] / 60),
                    (unsigned int)ft4_cf[2],
                    (unsigned int)(ft4_cycles[2] /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(ft4_calls[2] / 60),
                    (unsigned int)ft4_overflow);
            RG_LOGI("AsyncWait: full=%u/%u scan=%u/%u space=%u/%u us/f/calls",
                    (unsigned int)(async_wait_us[0] / 60),
                    (unsigned int)async_wait_calls[0],
                    (unsigned int)(async_wait_us[1] / 60),
                    (unsigned int)async_wait_calls[1],
                    (unsigned int)(async_wait_us[2] / 60),
                    (unsigned int)async_wait_calls[2]);
            RG_LOGI("GpuChain: parse=%u us/f nodes=%u words=%u calls=%u /f",
                    (unsigned int)(gpu_chain_parse_cycles /
                        (60 * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)),
                    (unsigned int)(gpu_chain_nodes / 60),
                    (unsigned int)(gpu_chain_words / 60),
                    (unsigned int)(gpu_chain_parse_calls / 60));
#ifdef PCSX_NDRC_RV32_FULL_CORE
            new_dynarec_print_stats();
#elif defined(PCSX_DUAL_DYNAREC)
            if (cpu_use_rv32_active)
                new_dynarec_print_stats();
#endif
#endif
            profile_total_us = 0;
        }
#endif

        rg_system_tick(busy_us);
        report_memory_card_error();

        /* app->frameTime also follows Retro-Go's speed control. Catch up
         * after an occasional slow compile, but reset the deadline after a
         * menu or a longer stall instead of running a burst of late frames. */
        int frame_time_us = app->frameTime;
        int64_t now = rg_system_timer();
        if (now - next_frame_deadline > (int64_t)frame_time_us * 3) {
            next_frame_deadline = now + frame_time_us;
        } else {
            if (now < next_frame_deadline)
                rg_usleep((uint32_t)(next_frame_deadline - now));
            next_frame_deadline += frame_time_us;
        }
    }
}

void app_main(void) {
    app = rg_system_init(&(const rg_config_t){
        .sampleRate = 44100,
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

    sound_enabled = rg_settings_get_number(NS_APP,
                                            SETTING_SOUND_ENABLED, 0) != 0;
    sound_fast_mode = rg_settings_get_number(NS_APP,
                                              SETTING_SOUND_QUALITY, 1) != 0;
#ifdef PCSX_DUAL_DYNAREC
    cpu_use_rv32 = rg_settings_get_number(NS_APP, SETTING_CPU_CORE, 0) != 0;
    if (cpu_use_rv32 && (rg_input_read_gamepad() & RG_KEY_SELECT)) {
        cpu_use_rv32 = false;
        rg_settings_set_number(NS_APP, SETTING_CPU_CORE, 0);
        RG_LOGW("SELECT held at startup: restored Lightrec CPU core");
    }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    /* A previous diagnostic build exposed an interpreter option that can
     * stall during real-BIOS reset. Ignore and clear that saved selection. */
    if (rg_settings_get_number(NS_APP, "CpuInterpreter", 0) != 0) {
        rg_settings_set_number(NS_APP, "CpuInterpreter", 0);
        RG_LOGW("Cleared unsupported CPU interpreter preference");
    }
#endif
    cycle_multiplier = rg_settings_get_number(NS_APP,
                                               SETTING_CYCLE_MULTIPLIER, 400);
    if (cycle_multiplier < 200 || cycle_multiplier > 400
        || cycle_multiplier % 50 != 0)
        cycle_multiplier = 400;

    RG_LOGI("Starting PCSX-ReARMed for retro-go...");
    display_surface = rg_surface_create(320, 240, RG_PIXEL_565_LE, 0);
    if (!display_surface)
        RG_PANIC("Display surface allocation failed");

    // Pre-allocate VRAM
    extern uint16_t *g_vram_p;
    if (!g_vram_p) {
        g_vram_p = heap_caps_aligned_alloc(64, 1024 * 512 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!g_vram_p) RG_PANIC("VRAM allocation failed! Out of SPIRAM?");
        memset(g_vram_p, 0, 1024 * 512 * 2);
    }

    if (!rg_task_create("emu_task", emu_task, NULL, 48 * 1024, 0, RG_TASK_PRIORITY_5, 1)) {
        RG_PANIC("Failed to create emu_task");
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
#if defined(CONFIG_IDF_TARGET_ESP32P4)
        lightrec_plugin_debug_poll();
#endif
    }
}
