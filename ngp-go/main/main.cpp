#include <rg_system.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>

#include "../components/race/race-memory.h"
#include "../components/race/types.h"
#include "../components/race/tlcs900h.h"
#include "../components/race/input.h"
#include "../components/race/flash.h"
#include "../components/race/neopopsound.h"
#include "../components/race/graphics.h"
#include "../components/race/state.h"

// Constants
#define AUDIO_SAMPLE_RATE 22050
#define AUDIO_BUFFER_CAPACITY ((AUDIO_SAMPLE_RATE + 59) / 60)
#define NGP_WIDTH  160
#define NGP_HEIGHT 152
#define NGP_STATE_MAGIC 0x5350474EU /* "NGPS" in little endian */
#define NGP_STATE_VERSION 2

// Globals for Retro-Go
static rg_app_t *app;
static rg_surface_t *updates[2];
static rg_surface_t *last_complete_surface;
static int current_update;
static rg_audio_sample_t *audio_buffer;
static char *battery_path;
static bool emu_active;
static size_t rom_size;

// Task Synchronization
static rg_task_t *audio_task_handle;
static volatile bool audio_task_running;
static volatile bool audio_task_busy;
static volatile bool audio_paused;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t payload_size;
    uint32_t rom_size;
    uint32_t payload_checksum;
    uint8_t rom_signature[0x40];
} ngp_state_header_t;

typedef struct {
    int64_t started_us;
    int64_t core_us;
    int64_t draw_core_us;
    int64_t skip_core_us;
    int64_t submit_us;
    uint32_t frames;
    uint32_t drawn;
    uint32_t skipped;
    uint32_t display_late;
    uint32_t audio_drops;
} perf_window_t;

static perf_window_t perf_window;

static void map_vdp_tables(void);

static rg_surface_t *get_complete_surface(void) {
    return last_complete_surface ? last_complete_surface : updates[current_update];
}

static uint32_t state_checksum(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 2166136261U;
    while (size--)
        hash = (hash ^ *bytes++) * 16777619U;
    return hash;
}

// RACE Core Global Variables
extern "C" {
    EMUINFO m_emuInfo;
    int tipo_consola = 1; 
    int m_bIsActive = 1;
    int gfx_hacks = 0;
    volatile unsigned g_frame_ready = 0;
    bool s_interlace_parity = false;

    unsigned char *sprite_table = NULL;
    unsigned char *pattern_table = NULL;
    unsigned short *patterns = NULL;
    unsigned short *tile_table_front = NULL;
    unsigned short *tile_table_back = NULL;
    unsigned short *palette_table = NULL;
    unsigned char *bw_palette_table = NULL;
    unsigned char *sprite_palette_numbers = NULL;
    unsigned char *scanlineY = NULL;
    unsigned char *frame0Pri = NULL;
    unsigned char *frame1Pri = NULL;
    unsigned char *wndTopLeftX = NULL;
    unsigned char *wndTopLeftY = NULL;
    unsigned char *wndSizeX = NULL;
    unsigned char *wndSizeY = NULL;
    unsigned char *scrollSpriteX = NULL;
    unsigned char *scrollSpriteY = NULL;
    unsigned char *scrollFrontX = NULL;
    unsigned char *scrollFrontY = NULL;
    unsigned char *scrollBackX = NULL;
    unsigned char *scrollBackY = NULL;
    unsigned char *bgSelect = NULL;
    unsigned short *bgTable = NULL;
    unsigned char *oowSelect = NULL;
    unsigned short *oowTable = NULL;
    unsigned char *color_switch = NULL;
    unsigned char *rasterY = NULL;
    unsigned short *drawBuffer = NULL;

    extern unsigned char *my_pc;
    extern int finscan;
    void audio_dac_init(void);
    int Cz80_allocate_flag_tables(void);
    void flashStartup(void);
    void tlcs_init(void);
    void tlcs_reinit(void);
    void Z80_Init(void);
    void Z80_Reset(void);
    int sound_allocate_state(void);
    void cz80_bind_memory(void);
}

// Background Audio Task
static void audio_task(void *arg) {
    (void)arg;
    rg_task_msg_t msg;
    static uint16_t psg_buf[AUDIO_BUFFER_CAPACITY];
    static uint16_t dac_buf[AUDIO_BUFFER_CAPACITY];

    audio_task_running = true;
    while (rg_task_receive(&msg, -1)) {
        if (msg.type == RG_TASK_MSG_STOP)
            break;

        audio_task_busy = true;
        if (!audio_paused) {
            const size_t sample_count = msg.dataInt;
            sound_update(psg_buf, sample_count * sizeof(uint16_t));
            dac_update(dac_buf, sample_count * sizeof(uint16_t));

            for (size_t i = 0; i < sample_count; i++) {
                int32_t sample = (int16_t)psg_buf[i] + (int16_t)dac_buf[i];
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                audio_buffer[i].left = audio_buffer[i].right = (int16_t)sample;
            }
            rg_audio_submit(audio_buffer, sample_count);
        }
        audio_task_busy = false;
    }
    audio_task_busy = false;
    audio_task_running = false;
}

static void pause_audio_task(bool pause) {
    audio_paused = pause;
    if (pause && audio_task_handle) {
        while (audio_task_busy || rg_task_messages_waiting(audio_task_handle) != 0)
            rg_task_delay(1);
    }
}

static void stop_audio_task(void) {
    if (!audio_task_handle)
        return;

    pause_audio_task(true);
    rg_task_msg_t stop_msg = {};
    stop_msg.type = RG_TASK_MSG_STOP;
    rg_task_send(audio_task_handle, &stop_msg, -1);
    while (audio_task_running)
        rg_task_delay(1);
    audio_task_handle = NULL;
}

static void record_performance(bool drawn, int64_t core_us, int64_t submit_us,
                               bool display_late, bool audio_dropped) {
    const int64_t now = rg_system_timer();
    if (!perf_window.started_us)
        perf_window.started_us = now;

    perf_window.frames++;
    perf_window.core_us += core_us;
    perf_window.submit_us += submit_us;
    if (drawn) {
        perf_window.drawn++;
        perf_window.draw_core_us += core_us;
    } else {
        perf_window.skipped++;
        perf_window.skip_core_us += core_us;
    }
    if (display_late)
        perf_window.display_late++;
    if (audio_dropped)
        perf_window.audio_drops++;

    const int64_t interval_us = now - perf_window.started_us;
    if (interval_us < 3000000)
        return;

    const uint32_t frame_rate10 = (uint32_t)((uint64_t)perf_window.frames * 10000000ULL / interval_us);
    const uint32_t draw_rate10 = (uint32_t)((uint64_t)perf_window.drawn * 10000000ULL / interval_us);
    const uint32_t avg_core_us = (uint32_t)(perf_window.core_us / perf_window.frames);
    const uint32_t avg_draw_us = perf_window.drawn ? (uint32_t)(perf_window.draw_core_us / perf_window.drawn) : 0;
    const uint32_t avg_skip_us = perf_window.skipped ? (uint32_t)(perf_window.skip_core_us / perf_window.skipped) : 0;
    const uint32_t avg_submit_us = perf_window.drawn ? (uint32_t)(perf_window.submit_us / perf_window.drawn) : 0;

    RG_LOGD("NGP perf: frame=%" PRIu32 ".%" PRIu32
            " draw=%" PRIu32 ".%" PRIu32
            " core=%" PRIu32 "us (draw:%" PRIu32 " skip:%" PRIu32 ")"
            " submit=%" PRIu32 "us late=%" PRIu32 " audio_drop=%" PRIu32 " fs=%d",
            frame_rate10 / 10, frame_rate10 % 10,
            draw_rate10 / 10, draw_rate10 % 10,
            avg_core_us, avg_draw_us, avg_skip_us, avg_submit_us,
            perf_window.display_late, perf_window.audio_drops, app->frameskip);

    memset(&perf_window, 0, sizeof(perf_window));
}

// Utils
static void set_defaults_after_boot(void) {
    switch (tlcsMemReadW(0x00200020)) {
        case 0x0059:
        case 0x0061:
            tlcsMemWriteB(0x0020001F, 0xFF);
            break;
    }
    tlcsMemWriteB(0x00006F91, tlcsMemReadB(0x00200023));
    tlcsMemWriteB(0x00006F87, 0x01);
    tlcsMemWriteB(0x00006F84, 0x40);
    tlcsMemWriteB(0x00006F85, 0x00);
    tlcsMemWriteB(0x00006F86, 0x00);
    tlcsMemWriteB(0x00004000, 0xC0);
    if (frame0Pri) *frame0Pri |= 0x80 | 0x40;
    if (bgSelect) *bgSelect |= 0x80;

    // ROM specific scanline timing hack from master branch
    finscan = 198;
    if (mainrom[0x000020] == 0x65 || mainrom[0x000020] == 0x93) finscan = 199;
}

static void load_battery() {
    size_t size = 0;
    void *data = NULL;
    if (rg_storage_read_file(battery_path, &data, &size, 0)) {
        if (data && size == 0x10000) {
            memcpy(ngpSaveBuf, data, 0x10000);
            ngpSaveBufDirty = 0;
        }
        free(data);
        return;
    }

    /* Migrate SRAM written by early ngp-go builds, which used
     * Saves/<rom-basename>.sav instead of Retro-Go's per-ROM SRAM path. */
    const char *basename = strrchr(app->romPath, '/');
    basename = basename ? basename + 1 : app->romPath;
    char legacy_path[RG_PATH_MAX + 1];
    const int prefix_len = snprintf(legacy_path, sizeof(legacy_path),
        "%s/%s", RG_BASE_PATH_SAVES, basename);
    if (prefix_len <= 0 || (size_t)prefix_len >= sizeof(legacy_path))
        return;
    char *extension = strrchr(legacy_path, '.');
    char *suffix = extension ? extension : legacy_path + prefix_len;
    if ((size_t)(suffix - legacy_path) + sizeof(".sav") > sizeof(legacy_path))
        return;
    strcpy(suffix, ".sav");

    data = NULL;
    size = 0;
    if (rg_storage_read_file(legacy_path, &data, &size, 0)) {
        if (data && size == 0x10000) {
            memcpy(ngpSaveBuf, data, 0x10000);
            const bool migrated = rg_storage_write_file(
                battery_path, ngpSaveBuf, 0x10000, RG_FILE_ATOMIC_WRITE);
            if (migrated)
                RG_LOGI("Migrated legacy SRAM to '%s'", battery_path);
            ngpSaveBufDirty = migrated ? 0 : 1;
        }
        free(data);
    }
}

static void save_battery() {
    if (ngpSaveBufActive && ngpSaveBufDirty) {
        if (rg_storage_write_file(battery_path, ngpSaveBuf, 0x10000, RG_FILE_ATOMIC_WRITE)) {
            ngpSaveBufDirty = 0;
        }
    }
}

// Retro-Go Handlers
static bool load_state_handler(const char *filename) {
    size_t size = 0;
    void *data = NULL;
    const size_t payload_size = (size_t)state_get_size();
    const size_t expected_size = sizeof(ngp_state_header_t) + payload_size;

    if (!rg_storage_read_file(filename, &data, &size, 0))
        return false;

    bool success = false;
    if (data && size == expected_size) {
        const ngp_state_header_t *header = (const ngp_state_header_t *)data;
        if (header->magic == NGP_STATE_MAGIC &&
            header->version == NGP_STATE_VERSION &&
            header->header_size == sizeof(*header) &&
            header->payload_size == payload_size &&
            header->rom_size == (uint32_t)rom_size &&
            memcmp(header->rom_signature, mainrom, sizeof(header->rom_signature)) == 0) {
            void *payload = (uint8_t *)data + sizeof(*header);
            success = header->payload_checksum == state_checksum(payload, payload_size) &&
                state_restore_mem(payload) != 0;
            if (success) {
                my_pc = (unsigned char *)get_address(gen_regsPC);
                audio_dac_init();
                g_frame_ready = 0;
                s_interlace_parity = false;
            }
            success = success && my_pc != NULL;
        }
    }

    if (!success)
        RG_LOGE("Invalid, incompatible, or wrong-ROM NGP save state");
    free(data);
    return success;
}

static bool save_state_handler(const char *filename) {
    const size_t payload_size = (size_t)state_get_size();
    const size_t size = sizeof(ngp_state_header_t) + payload_size;
    ngp_state_header_t *header = (ngp_state_header_t *)rg_alloc(size, MEM_SLOW);
    if (!header)
        return false;

    memset(header, 0, sizeof(*header));
    header->magic = NGP_STATE_MAGIC;
    header->version = NGP_STATE_VERSION;
    header->header_size = (uint16_t)sizeof(*header);
    header->payload_size = (uint32_t)payload_size;
    header->rom_size = (uint32_t)rom_size;
    memcpy(header->rom_signature, mainrom, sizeof(header->rom_signature));

    void *payload = (uint8_t *)header + sizeof(*header);
    bool success = state_store_mem(payload) != 0;
    if (success) {
        header->payload_checksum = state_checksum(payload, payload_size);
        success = rg_storage_write_file(filename, header, size, 0);
    }
    free(header);
    return success;
}

static bool reset_handler(bool hard) {
    (void)hard;
    ngp_mem_init();
    map_vdp_tables();
    tlcs_init();
    Z80_Reset();
    sound_init(AUDIO_SAMPLE_RATE);
    audio_dac_init();
    g_frame_ready = 0;
    s_interlace_parity = false;
    ngpInputState = 0;
    my_pc = (unsigned char *)get_address(gen_regsPC);
    set_defaults_after_boot();
    return my_pc != NULL && graphics_init();
}

static bool screenshot_handler(const char *filename, int width, int height) {
    rg_surface_t *surface = get_complete_surface();
    return surface && rg_surface_save_image_file(surface, filename, width, height);
}

static void event_handler(int event, void *arg) {
    (void)arg;
    if (event == RG_EVENT_SHUTDOWN) {
        save_battery();
        emu_active = false;
        stop_audio_task();
    } else if (event == RG_EVENT_REDRAW) {
        rg_surface_t *surface = get_complete_surface();
        if (surface)
            rg_display_submit(surface, 0);
    }
}

static bool update_input() {
    static bool menu_key_held;
    uint32_t gamepad = rg_input_read_gamepad();
    uint32_t state = 0;
    const uint32_t menu_keys = RG_KEY_MENU | RG_KEY_OPTION;

    if (!(gamepad & menu_keys))
        menu_key_held = false;

    if (!menu_key_held && (gamepad & menu_keys)) {
        ngpInputState = 0;
        menu_key_held = true;
        save_battery();
        pause_audio_task(true);
        if (gamepad & RG_KEY_MENU) rg_gui_game_menu();
        else rg_gui_options_menu();
        pause_audio_task(false);
        return true;
    }

    if (gamepad & RG_KEY_UP)    state |= (1u << 0);
    if (gamepad & RG_KEY_DOWN)  state |= (1u << 1);
    if (gamepad & RG_KEY_LEFT)  state |= (1u << 2);
    if (gamepad & RG_KEY_RIGHT) state |= (1u << 3);
    if (gamepad & RG_KEY_A)     state |= (1u << 4);
    if (gamepad & RG_KEY_B)     state |= (1u << 5);
    if (gamepad & (RG_KEY_START | RG_KEY_SELECT)) state |= (1u << 6);

    ngpInputState = state;
    return false;
}

static void map_vdp_tables() {
    sprite_table           = (unsigned char*)  get_address(0x00008800);
    pattern_table          = (unsigned char*)  get_address(0x0000A000);
    patterns               = (unsigned short*) pattern_table;
    tile_table_front       = (unsigned short*) get_address(0x00009000);
    tile_table_back        = (unsigned short*) get_address(0x00009800);
    palette_table          = (unsigned short*) get_address(0x00008200);
    bw_palette_table       = (unsigned char*)  get_address(0x00008100);
    sprite_palette_numbers = (unsigned char*)  get_address(0x00008C00);
    scanlineY              = (unsigned char*)  get_address(0x00008009);
    frame0Pri              = (unsigned char*)  get_address(0x00008000);
    frame1Pri              = (unsigned char*)  get_address(0x00008030);
    wndTopLeftX            = (unsigned char*)  get_address(0x00008002);
    wndTopLeftY            = (unsigned char*)  get_address(0x00008003);
    wndSizeX               = (unsigned char*)  get_address(0x00008004);
    wndSizeY               = (unsigned char*)  get_address(0x00008005);
    scrollSpriteX          = (unsigned char*)  get_address(0x00008020);
    scrollSpriteY          = (unsigned char*)  get_address(0x00008021);
    scrollFrontX           = (unsigned char*)  get_address(0x00008032);
    scrollFrontY           = (unsigned char*)  get_address(0x00008033);
    scrollBackX            = (unsigned char*)  get_address(0x00008034);
    scrollBackY            = (unsigned char*)  get_address(0x00008035);
    bgSelect               = (unsigned char*)  get_address(0x00008118);
    bgTable                = (unsigned short*) get_address(0x000083E0);
    oowSelect              = (unsigned char*)  get_address(0x00008012);
    oowTable               = (unsigned short*) get_address(0x000083F0);
    color_switch           = (unsigned char*)  get_address(0x00006F91);
    static unsigned char s_dummy_scan = 0;
    if (!scanlineY) scanlineY = &s_dummy_scan;
    rasterY = scanlineY;
}

extern "C" void app_main() {
    rg_config_t config;
    memset(&config, 0, sizeof(config));
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.frameRate = 60;
    config.storageRequired = true;
    config.romRequired = true;
    config.handlers.loadState = load_state_handler;
    config.handlers.saveState = save_state_handler;
    config.handlers.reset = reset_handler;
    config.handlers.screenshot = screenshot_handler;
    config.handlers.event = event_handler;

    app = rg_system_init(&config);
    rg_system_set_tick_rate(60);

    battery_path = rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath);
    if (!rg_storage_mkdir(rg_dirname(battery_path)))
        RG_LOGE("Unable to create SRAM directory");

    void *rom_data = NULL;
    if (rg_extension_match(app->romPath, "zip")) {
        if (!rg_storage_unzip_file(app->romPath, NULL, &rom_data, &rom_size, RG_FILE_ALIGN_16KB))
            rg_system_panic("ROM", "Failed to unzip ROM");
    } else {
        if (!rg_storage_read_file(app->romPath, &rom_data, &rom_size, RG_FILE_ALIGN_16KB))
            rg_system_panic("ROM", "Failed to load ROM");
    }

    if (!ngp_mem_alloc_init(64 * 1024, 0x10000))
        rg_system_panic("MEM", "Failed to allocate NGP memory");
    if (!ngp_mem_set_rom(rom_data, rom_size))
        rg_system_panic("ROM", "Invalid NGP ROM image");
    setFlashSize(rom_size);
    if (!ngpSaveBufActive)
        rg_system_panic("SRAM", "Failed to allocate cartridge save memory");
    
    m_emuInfo.machine = 1;
    m_emuInfo.romSize = (int)rom_size;
    tipo_consola = 1;

    cz80_bind_memory();
    if (!Cz80_allocate_flag_tables())
        rg_system_panic("Z80", "Failed to allocate flag tables");
    ngp_mem_init();
    map_vdp_tables();

    tlcs_init();
    Z80_Init();
    Z80_Reset();
    my_pc = (unsigned char *)get_address(gen_regsPC);
    if (!my_pc)
        rg_system_panic("NGP CPU", "ROM contains an invalid boot address");

    RG_LOGI("NGP boot: ROM=%u bytes, entry=0x%06x", (unsigned)rom_size,
            (unsigned)gen_regsPC);

    set_defaults_after_boot();

    for (int i = 0; i < 2; ++i) {
        /* Sequential framebuffer writes measured essentially the same in
         * PSRAM. Reserve one internal surface for the display pipeline and
         * spend the other surface's DRAM on CPU-active tables. */
        const uint32_t memory = i == 0 ? MEM_FAST : MEM_SLOW;
        updates[i] = rg_surface_create(NGP_WIDTH, NGP_HEIGHT, RG_PIXEL_565_BE, memory);
        if (!updates[i])
            rg_system_panic("VIDEO", "Failed to allocate framebuffers");
    }
    current_update = 0;
    drawBuffer = (unsigned short *)updates[current_update]->data;

    if (!graphics_init())
        rg_system_panic("VIDEO", "Failed to initialize RACE graphics");

    load_battery();
    sound_init(AUDIO_SAMPLE_RATE);
    audio_dac_init();
    if (!sound_allocate_state())
        rg_system_panic("AUDIO", "Failed to allocate RACE sound state");

    if (app->bootFlags & RG_BOOT_RESUME)
        rg_emu_load_state(app->saveSlot);

    audio_buffer = (rg_audio_sample_t *)rg_alloc(
        AUDIO_BUFFER_CAPACITY * sizeof(rg_audio_sample_t), MEM_FAST);
    if (!audio_buffer)
        rg_system_panic("AUDIO", "Failed to allocate audio buffer");

    emu_active = true;
    audio_task_handle = rg_task_create(
        "ngp_sound", audio_task, NULL, 2048, 2, RG_TASK_PRIORITY_2, 0);
    if (!audio_task_handle)
        rg_system_panic("AUDIO", "Failed to start audio task");
    while (!audio_task_running)
        rg_task_delay(1);

    int64_t last_battery_save = rg_system_timer();
    uint32_t audio_sample_remainder = 0;
    int skip_frames = 0;
    /* RACE's simulation-only frames are still expensive, so Retro-Go's
     * generic escalation to five consecutive skips reduces presentation to
     * single-digit FPS without recovering full emulation speed. Disable the
     * long-term controller and retain the normal one-frame overrun recovery
     * below. Faster targets will render every frame once they meet budget. */
    app->frameskip = -1;
    
    while (emu_active) {
        const int64_t startTime = rg_system_timer();
        bool display_late = false;
        bool audio_dropped = false;
        int64_t submit_us = 0;
        int64_t audio_wait_us = 0;
        const bool draw_frame = skip_frames == 0;

        if (update_input())
            continue;

        const int64_t core_start = rg_system_timer();
        tlcs_execute(0, draw_frame);
        const int64_t core_us = rg_system_timer() - core_start;

        audio_sample_remainder += AUDIO_SAMPLE_RATE;
        const uint32_t audio_samples = audio_sample_remainder / 60;
        audio_sample_remainder %= 60;
        rg_task_msg_t audio_msg = {};
        audio_msg.dataInt = audio_samples;
        /* The audio sink is Retro-Go's real-time clock. A non-blocking send
         * lets faster targets run the emulation far above 60 Hz and merely
         * discard the excess audio. Once the two-frame queue is full, wait
         * for the sound task instead. */
        const int64_t audio_wait_start = rg_system_timer();
        audio_dropped = !rg_task_send(audio_task_handle, &audio_msg, -1);
        audio_wait_us = rg_system_timer() - audio_wait_start;

        if (draw_frame) {
            display_late = rg_display_is_busy();
            const int64_t submit_start = rg_system_timer();
            rg_display_submit(updates[current_update], 0);
            submit_us = rg_system_timer() - submit_start;
            last_complete_surface = updates[current_update];

            current_update ^= 1;
            drawBuffer = (unsigned short *)updates[current_update]->data;
        }

        if (rg_system_timer() - last_battery_save > 5000000) {
            save_battery();
            last_battery_save = rg_system_timer();
        }

        int64_t frameEndTime = rg_system_timer();
        /* Pacing wait is idle time, not emulator workload. */
        rg_system_tick((int)(frameEndTime - startTime - audio_wait_us));
        record_performance(draw_frame, core_us, submit_us, display_late, audio_dropped);

        if (skip_frames == 0) {
            const int64_t elapsed = frameEndTime - startTime;
            if (app->frameskip > 0)
                skip_frames = app->frameskip;
            else if (elapsed > app->frameTime + 1500)
                skip_frames = 1;
            else if (draw_frame && display_late)
                skip_frames = 1;
        } else {
            skip_frames--;
        }
    }

    save_battery();
    stop_audio_task();
    free(audio_buffer);
    while (rg_display_is_busy())
        rg_task_delay(1);
    rg_surface_free(updates[0]);
    rg_surface_free(updates[1]);
    free(battery_path);
    flashShutdown();
    ngp_mem_free();
    free(rom_data);
    rg_system_exit();
}
