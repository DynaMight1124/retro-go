#include "gpsp_log.h"
#include "gpsp_audio_format.h"
#define AUDIO_SAMPLE_RATE GPSP_AUDIO_SAMPLE_RATE
#define AUDIO_BUFFER_LENGTH GPSP_AUDIO_MAX_FRAMES
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
#include <rg_system.h>
#include <rg_display.h>
#include <rg_storage.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#else /* GBSP_BACKEND_VARIANT */
#include <rg_system.h>
#include <stdio.h>
#endif /* GBSP_BACKEND_VARIANT */
#include <stdlib.h>
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
#include <string.h>

#include "gpsp_esp.h"

// GBA native output resolution
#define GBA_SCREEN_WIDTH  240
#define GBA_SCREEN_HEIGHT 160




// Rendering skip flag owned by the gpSP core (defined in gpsp_esp.c).
// When non-zero, the core's PPU skips scanline rendering for that frame.
extern uint32_t skip_next_frame;


static rg_app_t *app;
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
#include <string.h>

#include "../components/gbsp-libretro/common.h"
#include "../components/gbsp-libretro/memmap.h"
#include "../components/gbsp-libretro/sound.h"
#include "../components/gbsp-libretro/gba_memory.h"
#include "../components/gbsp-libretro/gba_cc_lut.h"

extern u32 gamepak_buffer_count;

#ifdef HAVE_DYNAREC
#include "xjit_exec.h"
#endif


/* ---- Xtensa dynarec glue (xtensa-68000-dynarec, pjcau) -------------------
 * HAVE_DYNAREC / XTENSA_ARCH / SMALL_TRANSLATION_CACHE must be defined for THIS
 * file as well (main/CMakeLists.txt): gbsp_memory_t changes size with them.   */
extern u8 gamepak_backup_dirty;           /* gba_memory.c: set on every backup write */
void gbsp_render_wait(void);              /* video.cpp: core 1 has drawn every queued line */
extern int gbsp_render_core1;             /* video.cpp: 1 once the renderer task runs */
void gbsp_display_poll(void);             /* video.cpp calls periodically during rendering */
#ifdef HAVE_DYNAREC
int dynarec_enable = 0;                   /* 1 = translated code, 0 = interpreter     */
extern int xt_give_up;                    /* xtensa_stub.c: SMC storm, use interpreter*/
extern u32 xt_exec_delta;
extern u32 xt_rom_translations, xt_rom_flushes;
extern u8 *rom_translation_cache, *ram_translation_cache;
extern u8 *rom_translation_ptr, *ram_translation_ptr;
void xt_rblk_alloc(void);                 /* cpu_threaded.c: per-block SMC records    */
static xj_exec_t jit_mem;
#define JIT_GUARD (16 * 1024)
static u8 *jit_guard[2];
static char jit_flag_path[320];           /* "<rom>.jit": see jit_flag_read()         */
static bool jit_trying, jit_restart;
static int64_t jit_t0;
#endif


u32 idle_loop_target_pc = 0xFFFFFFFF;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
boot_mode selected_boot_mode = boot_game;

u32 skip_next_frame = 0;
int sprite_limit = 1;

gbsp_memory_t *gbsp_memory;

static rg_app_t *app;
#else /* GBSP_BACKEND_VARIANT */
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * ESP32 & ESP32-S3: Original Retro-Go Interpreter path
 * ═══════════════════════════════════════════════════════════════════════════ */
#include "../components/gbsp-libretro/common.h"
#include "../components/gbsp-libretro/memmap.h"
#include "../components/gbsp-libretro/sound.h"
#include "../components/gbsp-libretro/gba_memory.h"
#include "../components/gbsp-libretro/gba_cc_lut.h"


u32 idle_loop_target_pc = 0xFFFFFFFF;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
boot_mode selected_boot_mode = boot_game;

u32 skip_next_frame = 0;
int sprite_limit = 1;

gbsp_memory_t *gbsp_memory;

static rg_app_t *app;
#endif /* GBSP_BACKEND_VARIANT */

#include "gbsp_controls.h"
#include "gbsp_options.h"
#include "gbsp_save.h"
#include "gbsp_screenshot.h"
#include "gbsp_display.h"
#include "gbsp_state.h"
#include "gbsp_worker_stats.h"
#include "gbsp_perf.h"
#include "gbsp_loading.h"
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */


#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

/* S3 ROM paging/display activity provides a quiet-window hint. The shared
 * scheduler bounds deferral so active gameplay cannot starve battery saves. */
static bool gp_save_bus_quiet(void)
{
    static int32_t frames;
    static u32 pages;
    static int64_t last_activity;
    int32_t current_frames = rg_display_get_counters().totalFrames;
    if (frames != current_frames || pages != gamepak_page_loads) {
        frames = current_frames;
        pages = gamepak_page_loads;
        last_activity = rg_system_timer();
    }
    return rg_system_timer() - last_activity >= 2000000;
}

void netpacket_poll_receive()
{
}

void netpacket_send(uint16_t client_id, const void *buf, size_t len)
{
}



#else /* GBSP_BACKEND_VARIANT */


void netpacket_poll_receive()
{
}

void netpacket_send(uint16_t client_id, const void *buf, size_t len)
{
}


#endif /* GBSP_BACKEND_VARIANT */

static bool reset_handler(bool hard)
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
{
    gp_display_quiesce();
    gp_save_flush_sync();
    gpsp_reset();
    return true;
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
{
    gp_display_quiesce();
    gp_save_flush_sync();
    reset_gba();
#ifdef HAVE_DYNAREC
    if (dynarec_enable)
        flush_dynarec_caches();
#endif
    return true;
#else /* GBSP_BACKEND_VARIANT */
{
    gp_display_quiesce();
    gp_save_flush_sync();
    reset_gba();
    return true;
#endif /* GBSP_BACKEND_VARIANT */
}
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_SHUTDOWN) {
        gp_display_quiesce();
        gp_save_shutdown();
    }
    if (event == RG_EVENT_REDRAW)
        gp_display_redraw();
}

#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

#ifdef HAVE_DYNAREC
static void jit_flag_write(int status, int fails);
#endif

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_SHUTDOWN)
    {
        gp_display_quiesce();
        gp_save_shutdown();
#ifdef HAVE_DYNAREC
        if (jit_trying)
        {
            /* a clean exit proves the dynarec as well as two minutes do */
            jit_flag_write(0, 0);
            jit_trying = false;
        }
#endif
    }
    if (event == RG_EVENT_REDRAW)
    {
        gp_display_redraw();
    }
}



void set_fastforward_override(bool fastforward)
{
}



#ifdef HAVE_DYNAREC
/* Per game, "<rom>.jit" in the cache directory:
 *   absent    never tried, or proven (two minutes of play, or a clean exit)
 *   T<n>      on probation; n = starts that crashed or hung on the dynarec
 *   O<n>      off: n = 3 after repeated crashes, 0 when chosen in the menu
 *   X<n>      the dynarec gave up (the game rewrites its own code)
 * A power cut or an app switch also leaves T behind, so the reset reason
 * decides whether it counts: only a panic or a watchdog is the dynarec's fault.
 * Without that, one battery cut inside the probation window disabled the
 * dynarec for a game for ever. */
#define JIT_FAILS_MAX 3

static bool jit_reset_was_a_crash(void)
{
    switch (esp_reset_reason())
    {
    case ESP_RST_PANIC:    return true;
    case ESP_RST_INT_WDT:  return true;
    case ESP_RST_TASK_WDT: return true;
    case ESP_RST_WDT:      return true;
    default:               return false;   /* POWERON, SW, EXT, BROWNOUT, ... */
    }
}

static int jit_flag_read(int *fails)
{
    *fails = 0;
    FILE *f = fopen(jit_flag_path, "rb");
    if (!f)
        return 0;
    int c = fgetc(f), n = 0, d;
    while ((d = fgetc(f)) >= '0' && d <= '9')
    {
        n = n * 10 + (d - '0');
        if (n > 99)
            break;
    }
    fclose(f);
    if (c == EOF)
        return 0;
    *fails = n;
    return c == '0' ? 'O' : c;   /* older builds wrote '0' for off */
}

static void jit_flag_write(int status, int fails)
{
    if (!status)
    {
        remove(jit_flag_path);
        return;
    }
    FILE *f = fopen(jit_flag_path, "wb");
    if (f)
    {
        fprintf(f, "%c%d", status, fails);
        fclose(f);
    }
}

static void jit_mark_ok(void)
{
    if (jit_trying && rg_system_timer() - jit_t0 > 120 * 1000000)
    {
        jit_flag_write(0, 0);
        jit_trying = false;
    }
}

static rg_gui_event_t jit_toggle_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        if (dynarec_enable)
        {
            dynarec_enable = 0;
            jit_trying = false;
            jit_flag_write('O', 0);
            GPSP_TRACE("dynarec: off for this game, interpreter");
        }
        else if (!jit_restart)
        {
            jit_flag_write(0, 0);
            jit_restart = true;   /* the caches are sized at boot: restart */
        }
    }
    strcpy(option->value, dynarec_enable ? _("On") : jit_restart ? _("On (restart)") : _("Off"));
    return RG_DIALOG_VOID;
}
#endif

#else /* GBSP_BACKEND_VARIANT */

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_SHUTDOWN) {
        gp_display_quiesce();
        gp_save_shutdown();
    }
    if (event == RG_EVENT_REDRAW)
    {
        gp_display_redraw();
    }
}



void set_fastforward_override(bool fastforward)
{
}





#endif /* GBSP_BACKEND_VARIANT */
static void options_handler(rg_gui_option_t *dest)
{
    dest = gbsp_common_options(dest);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

#ifdef HAVE_DYNAREC
    *dest++ = (rg_gui_option_t){0, _("Fast CPU (dynarec)"), "-", RG_DIALOG_FLAG_NORMAL, &jit_toggle_cb};
#endif
#endif /* GBSP_BACKEND_VARIANT */
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
static uint16_t map_buttons(uint32_t joystick)
{
    return gbsp_gba_buttons(joystick);
}
void app_main(void)
{
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
#ifdef HAVE_DYNAREC
#define GBSP_TASK_STACK (20 * 1024)
static void gbsp_main(void);
static void gbsp_task(void *arg)
{
    gbsp_main();
}
void app_main(void)
{
    if (xTaskCreatePinnedToCore(gbsp_task, "gbsp", GBSP_TASK_STACK, NULL, uxTaskPriorityGet(NULL), NULL, 0) != pdPASS)
        gbsp_main();
    vTaskDelete(NULL);
}
static void gbsp_main(void)
#else
void app_main(void)
#endif
{
#else /* GBSP_BACKEND_VARIANT */
void app_main(void)
{
#endif /* GBSP_BACKEND_VARIANT */
    app = rg_system_init(&(const rg_config_t){
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
        .handlers = {
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
            .reset = &reset_handler,
            .screenshot = &gp_display_screenshot,
            .event = &event_handler,
            .options = &options_handler,
        },
    });
    gbsp_settings_load();
    gp_frameskip_start();
    gp_save_alloc();
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

    // load settings

    gp_display_init();
    gp_display_configure_renderer();

    gpsp_bios_init(GPSP_BIOS_PATH);
    if (!gpsp_init())
        RG_PANIC("gpSP init failed");

    gp_load_begin();
    if (gpsp_load_rom(app->romPath) != 0)
        RG_PANIC("Could not load the game file.");

    gp_load_finish();

    RG_LOGI("emulation loop (RISC-V dynarec)");

    static rg_audio_sample_t mixbuffer[AUDIO_BUFFER_LENGTH];

#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */



    gp_display_init();

    gbsp_memory = rg_alloc(sizeof(*gbsp_memory), MEM_ANY);
    RG_LOGI("gbsp_memory=%p (%u KB)", gbsp_memory, (unsigned)(sizeof(*gbsp_memory) / 1024));

    libretro_supports_bitmasks = true;
    retro_set_input_state(input_cb);

    gp_display_configure_renderer();
#ifdef HAVE_DYNAREC
    {
        char *path = rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath);
        RG_ASSERT(path && strlen(path) + 4 < sizeof(jit_flag_path), "JIT path too long");
        snprintf(jit_flag_path, sizeof(jit_flag_path), "%s.jit", path);
        *strrchr(path, '/') = '\0';
        RG_ASSERT(rg_storage_mkdir(path), "Cannot create JIT metadata directory");
        free(path);

        int fails = 0;
        int flag = jit_flag_read(&fails);
        bool crashed = jit_reset_was_a_crash();

        if (flag == 'T')
        {
            if (crashed)
                fails++;
            if (fails >= JIT_FAILS_MAX)
            {
                jit_flag_write('O', fails);
                flag = 'O';
                RG_LOGW("dynarec: it crashed on this game %d times (reset reason %d); "
                        "using the interpreter from now on (Options > Fast CPU to retry)",
                        fails, (int)esp_reset_reason());
            }
            else
            {
                jit_flag_write('T', fails);
                RG_LOGW("dynarec: the last start %s (reset reason %d), attempt %d of %d",
                        crashed ? "crashed" : "did not finish but did not crash",
                        (int)esp_reset_reason(), fails + 1, JIT_FAILS_MAX);
            }
        }

        if ((flag == 0 || flag == 'T') &&
            xj_exec_alloc_psram(&jit_mem, ROM_TRANSLATION_CACHE_SIZE + JIT_GUARD + RAM_TRANSLATION_CACHE_SIZE + JIT_GUARD))
        {
            rom_translation_cache = jit_mem.data;
            jit_guard[0] = jit_mem.data + ROM_TRANSLATION_CACHE_SIZE;
            ram_translation_cache = jit_guard[0] + JIT_GUARD;
            jit_guard[1] = ram_translation_cache + RAM_TRANSLATION_CACHE_SIZE;
            memset(jit_guard[0], 0, JIT_GUARD);
            memset(jit_guard[1], 0, JIT_GUARD);
            rom_translation_ptr = rom_translation_cache;
            ram_translation_ptr = ram_translation_cache;
            xt_exec_delta = jit_mem.exec - (u32)(uintptr_t)jit_mem.data;
            dynarec_enable = 1;
            jit_flag_write('T', fails);
            jit_trying = true;
            jit_t0 = rg_system_timer();
            GPSP_TRACE("dynarec: caches %u KB at %p (exec %08x)",
                    (unsigned)((ROM_TRANSLATION_CACHE_SIZE + RAM_TRANSLATION_CACHE_SIZE) / 1024),
                    jit_mem.data, (unsigned)jit_mem.exec);
        }
        else if (flag == 0 || flag == 'T')
        {
            RG_LOGW("dynarec: no memory for the translation caches, interpreter");
        }
        else
        {
            RG_LOGW("dynarec: off for this game (%s), interpreter",
                    flag == 'X' ? "gave up: the game rewrites its own code" : "disabled");
        }

        if (dynarec_enable)
            xt_rblk_alloc();  /* per-block SMC records: a store only rewinds its own block */
    }
#endif
    gpsp_bios_init(GPSP_BIOS_PATH);
    init_gamepak_buffer();
    RG_LOGI("ROM cache: %u blocks of 1 MB, cpu %d MHz, %u KB PSRAM free (%u KB largest)",
            (unsigned)gamepak_buffer_count, rg_system_get_cpu_speed(),
            (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
            (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024));
    init_sound();

    memset(gamepak_backup, 0xff, sizeof(gamepak_backup));
    /* RTC autodetect: the gpSP database enables the clock for the games that
       have one (Pokemon Ruby/Sapphire/Emerald among them), so berries, tides
       and the day/night cycle follow the handheld's clock. Rumble and the
       link port stay off: this build has no motor and no link partner. */
    gp_load_begin();
    if (load_gamepak(NULL, app->romPath, FEAT_AUTODETECT, FEAT_DISABLE, SERIAL_MODE_DISABLED) != 0)
    {
        RG_PANIC("Could not load the game file.");
    }
    RG_LOGI("ROM: %u KB, %s in the cache", (unsigned)(gamepak_size / 1024),
            (gamepak_size <= gamepak_buffer_count * 1024u * 1024u) ? "fits" : "does not fit");

    RG_LOGI("reset_gba");
    reset_gba();

    gp_load_finish();

    rg_audio_sample_t mixbuffer[AUDIO_BUFFER_LENGTH] = {0};



#else /* GBSP_BACKEND_VARIANT */

    gp_display_init();

    gbsp_memory = rg_alloc(sizeof(*gbsp_memory), MEM_ANY);
    RG_LOGI("gbsp_memory=%p", gbsp_memory);
    gp_display_configure_renderer();

    libretro_supports_bitmasks = true;
    retro_set_input_state(input_cb);
    gpsp_bios_init(GPSP_BIOS_PATH);
    init_gamepak_buffer();
    init_sound();

    memset(gamepak_backup, 0xff, sizeof(gamepak_backup));
    gp_load_begin();
    if (load_gamepak(NULL, app->romPath, FEAT_AUTODETECT, FEAT_DISABLE, SERIAL_MODE_DISABLED) != 0)
    {
        RG_PANIC("Could not load the game file.");
    }

    RG_LOGI("reset_gba");
    reset_gba();

    gp_load_finish();

    RG_LOGI("emulation loop (interpreter)");

    rg_audio_sample_t mixbuffer[AUDIO_BUFFER_LENGTH] = {0};

#endif /* GBSP_BACKEND_VARIANT */
    while (true)
    {
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
#ifdef HAVE_DYNAREC
        gp_frameskip_select_backend(dynarec_enable ? GP_FS_S3 : GP_FS_INTERPRETER);
#else
        gp_frameskip_select_backend(GP_FS_INTERPRETER);
#endif
#endif /* GBSP_BACKEND_VARIANT */
        const int64_t startTime = rg_system_timer();
        uint32_t joystick = rg_input_read_gamepad();
        gp_input_keys = joystick;
        const bool drawn = !skip_next_frame;
        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION)) {
            GP_PERF_RESET();
            gp_display_quiesce();
            gp_save_flush_sync();
            gp_state_recover_slots();
            gamepak_menu_begin();
            gp_frameskip_pause();
        }
        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION))
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
        {
#ifdef HAVE_DYNAREC
            jit_mark_ok();
#endif
            if (joystick & RG_KEY_MENU)
#else /* GBSP_BACKEND_VARIANT */
        {
            if (joystick & RG_KEY_MENU)
#endif /* GBSP_BACKEND_VARIANT */
                rg_gui_game_menu();
            else
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
                rg_gui_options_menu();
            gamepak_menu_end();
            gp_frameskip_resume();
            gp_display_apply_pending();
            memset(mixbuffer, 0, sizeof(mixbuffer));
            continue;
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
                rg_gui_options_menu();
            gamepak_menu_end();
            gp_frameskip_resume();
            gp_display_apply_pending();
            memset(&mixbuffer, 0, sizeof(mixbuffer));
#ifdef HAVE_DYNAREC
            if (jit_restart)
            {
                /* the user turned the dynarec back on: caches and probation are
                   set up at boot, so start the game again now */
                jit_restart = false;
                rg_system_restart();
            }
#endif
            continue;
#else /* GBSP_BACKEND_VARIANT */
                rg_gui_options_menu();
            gamepak_menu_end();
            gp_frameskip_resume();
            gp_display_apply_pending();
            memset(&mixbuffer, 0, sizeof(mixbuffer));
            continue;
#endif /* GBSP_BACKEND_VARIANT */
        }
        if (!skip_next_frame) gp_display_begin();
        GP_PERF_BEGIN();
        const int64_t gp_exec_start = rg_system_timer();
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

        gpsp_set_buttons(map_buttons(joystick));

        // gpsp_run_frame() runs one full frame; the PPU honors skip_next_frame.
        gpsp_run_frame();
        gp_frameskip_record((uint32_t)(rg_system_timer() - gp_exec_start), drawn);
        GP_PERF_STAGE(exec);
        gp_save_tick(true);

        if (drawn)
        {
            gp_display_finish();
        }

        GP_PERF_STAGE(display);
        size_t frames_count = gpsp_get_audio((int16_t *)mixbuffer, AUDIO_BUFFER_LENGTH);

        rg_system_tick(rg_system_timer() - startTime);

        rg_audio_submit(mixbuffer, frames_count);
        GP_PERF_STAGE(audio);

        gp_frameskip_end_frame();
        GP_PERF_POLICY(gp_frameskip.level, gp_frameskip.profile.max_level);
        GP_PERF_END(drawn, gp_frameskip.level);
    }
}
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

        update_input();
        rumble_frame_reset();
        clear_gamepak_stickybits();

#ifdef HAVE_DYNAREC
        if (dynarec_enable)
        {
            execute_arm_translate(execute_cycles);
            if (xt_give_up)
            {
                RG_LOGW("dynarec: giving up for this game (code rewritten continuously)");
                dynarec_enable = 0;
                jit_trying = false;
                jit_flag_write('X', 0);
            }
            else
                jit_mark_ok();
        }
        else
#endif
            execute_arm(execute_cycles);
        gp_frameskip_record((uint32_t)(rg_system_timer() - gp_exec_start), drawn);
        GP_PERF_STAGE(exec);

        if (!skip_next_frame)
        {
            gbsp_render_wait();
            GP_PERF_STAGE(wait);

            gp_display_finish();
            GP_PERF_STAGE(display);
        }

        gp_save_tick(gp_save_bus_quiet());
        size_t frames_count = sound_read_samples((s16 *)mixbuffer, AUDIO_BUFFER_LENGTH);
        const int64_t t_mix_end = rg_system_timer();

        rg_system_tick(t_mix_end - startTime);

        rg_audio_submit(mixbuffer, frames_count);
        GP_PERF_STAGE(audio);
        /* The audio sink can block for most of a frame. The display often
           frees up during that wait; submit then instead of losing the slot
           until the next emulated frame. */
        gbsp_display_poll();

        gp_frameskip_end_frame();
        GP_PERF_POLICY(gp_frameskip.level, gp_frameskip.profile.max_level);
        GP_PERF_END(drawn, gp_frameskip.level);

    }

    RG_PANIC("GBsP Ended");
}
#else /* GBSP_BACKEND_VARIANT */

        update_input();
        rumble_frame_reset();
        clear_gamepak_stickybits();
        execute_arm(execute_cycles);
        gp_frameskip_record((uint32_t)(rg_system_timer() - gp_exec_start), drawn);
        GP_PERF_STAGE(exec);

        if (!skip_next_frame)
        {
            gp_display_finish();
        }

        GP_PERF_STAGE(display);
        gp_save_tick(false);
        size_t frames_count = sound_read_samples((s16 *)mixbuffer, AUDIO_BUFFER_LENGTH);

        rg_system_tick(rg_system_timer() - startTime);

        rg_audio_submit(mixbuffer, frames_count);
        GP_PERF_STAGE(audio);
        gp_frameskip_end_frame();
        GP_PERF_POLICY(gp_frameskip.level, gp_frameskip.profile.max_level);
        GP_PERF_END(drawn, gp_frameskip.level);

    }

    RG_PANIC("GBsP Ended");
}
#endif /* GBSP_BACKEND_VARIANT */
