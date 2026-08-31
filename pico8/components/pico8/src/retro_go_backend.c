// retro_go_backend.c
//
// Implements pico8/components/pico8/src/backend.h (the platform shim that
// PicoPico's engine expects) using retro-go's APIs.
//
// Engine contract:
//   - gfx_flip(): engine has finished drawing into its nibble-packed
//     `frontbuffer` (128x128, two pixels per byte). Copy to a 128x128
//     RG_PIXEL_PAL565_LE surface and `rg_display_submit()`.
//   - handle_input(): read `rg_input_read_gamepad()` and reflect the state
//     into the engine's `buttons[7]` and `buttons_frame[7]` arrays.
//   - init_audio(): spawn a `rg_task_create` whose loop calls
//     `fill_buffer(mono_buf, &channels[i], SAMPLES_PER_DURATION)` four times
//     and submits the mixed result via `rg_audio_submit()`. Channels[] and
//     fill_buffer are exposed through `pico8_globals.h` / `sfx.h`.

#include "backend.h"

#include "data.h"
#include "engine.h"
#include "pico8_globals.h"
#include "sfx.h"

#include <rg_audio.h>
#include <rg_display.h>
#include <rg_gui.h>
#include <rg_input.h>
#include <rg_settings.h>
#include <rg_surface.h>
#include <rg_system.h>
#include <rg_storage.h>
#include <rg_task.h>

#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Persistent cartridge data ------------------------------------------

#define CARTDATA_DIR RG_BASE_PATH_SAVES "/pico8"

static uint32_t *cartdata_store = NULL;
static size_t cartdata_store_size = 0;
static char cartdata_path[RG_PATH_MAX + 1];
static bool cartdata_dirty = false;

// cartdata IDs are logical namespaces and may contain characters unsuitable
// for FAT filenames. Two differently seeded FNV-1a hashes keep the path fixed
// length while making accidental namespace collisions vanishingly unlikely.
static uint32_t cartdata_hash(const char *id, uint32_t seed)
{
    uint32_t hash = seed;
    while (*id) {
        hash ^= (uint8_t)*id++;
        hash *= 16777619u;
    }
    return hash;
}

bool cartdata_flush(void)
{
    if (!cartdata_dirty || !cartdata_store || !cartdata_path[0]) return true;
    if (!rg_storage_mkdir(CARTDATA_DIR) ||
        !rg_storage_write_file(cartdata_path, cartdata_store,
                               cartdata_store_size, RG_FILE_ATOMIC_WRITE)) {
        RG_LOGE("pico8: failed to save cartdata %s", cartdata_path);
        return false;
    }
    cartdata_dirty = false;
    RG_LOGI("pico8: saved cartdata %s", cartdata_path);
    return true;
}

bool cartdata_open(const char *id, uint32_t *data, size_t size)
{
    if (!id || !*id || !data || size != 64 * sizeof(uint32_t)) return false;
    if (!cartdata_flush()) return false;

    const uint32_t h1 = cartdata_hash(id, 2166136261u);
    const uint32_t h2 = cartdata_hash(id, 0x9e3779b9u);
    int len = snprintf(cartdata_path, sizeof(cartdata_path),
                       CARTDATA_DIR "/%08lx%08lx.p8d",
                       (unsigned long)h1, (unsigned long)h2);
    if (len < 0 || (size_t)len >= sizeof(cartdata_path)) {
        cartdata_path[0] = '\0';
        return false;
    }

    cartdata_store = data;
    cartdata_store_size = size;
    cartdata_dirty = false;
    memset(data, 0, size);

    void *saved = NULL;
    size_t saved_size = 0;
    if (rg_storage_read_file(cartdata_path, &saved, &saved_size, 0)) {
        if (saved_size == size) {
            memcpy(data, saved, size);
            RG_LOGI("pico8: loaded cartdata namespace %s", id);
        } else {
            RG_LOGW("pico8: ignoring invalid cartdata size %u for %s",
                    (unsigned)saved_size, id);
        }
        free(saved);
    } else {
        RG_LOGI("pico8: new cartdata namespace %s", id);
    }
    return true;
}

void cartdata_mark_dirty(void)
{
    if (cartdata_store) cartdata_dirty = true;
}

// --- Display --------------------------------------------------------------

static rg_surface_t *screen = NULL;
static uint8_t last_screen_mode = 0xff;

bool init_video(void)
{
    RG_LOGI("pico8: init_video (128x%d RG_PIXEL_PAL565_LE)", SCREEN_HEIGHT);

    screen = rg_surface_create(SCREEN_WIDTH, SCREEN_HEIGHT,
                               RG_PIXEL_PAL565_LE, MEM_FAST);
    if (!screen) {
        RG_LOGE("pico8: rg_surface_create failed");
        return false;
    }
    last_screen_mode = 0xff;

    // Keep the lookup table internal: the display scaler reads it for every
    // source pixel. Allocate all 256 entries expected by indexed surfaces,
    // even though PICO-8 currently uses only the first 16.
    if (!screen->palette) {
        screen->palette = rg_alloc(256 * sizeof(uint16_t), MEM_FAST);
        if (!screen->palette) {
            rg_surface_free(screen);
            screen = NULL;
            return false;
        }
        screen->free_palette = true;
        memset(screen->palette, 0, 256 * sizeof(uint16_t));
    }
    return true;
}

void video_close(void)
{
    if (screen) {
        rg_surface_free(screen);
        screen = NULL;
    }
}

void gfx_flip(void)
{
    if (!screen || !screen->data || !screen->palette) return;

    // rg_display_submit queues the surface by pointer. Do not alter its
    // pixels or palette until the display task has released it.
    while (rg_display_is_busy()) {
        rg_task_yield();
    }

    // 1. Refresh `screen->palette` from the runtime `palette[]` so that
    //    _lua_pal()'s table-mode recolouring is reflected on screen.
    //    (Only the lower 16 entries are used since RG_PIXEL_PAL565_LE is
    //    indexed with 4-bit values.)
    for (int i = 0; i < 16; i++) {
        screen->palette[i] = palette[i];
    }

    // 2. Convert engine's nibble-packed `frontbuffer[64*128]` to the indexed
    //    Retro-Go surface. 0x5f2c selects PICO-8's presentation mode; drawing
    //    itself always targets the same packed 128x128 framebuffer. Modes
    //    1/2/3 expand the left/top quarter dimensions, while 5/6/7 mirror
    //    them. Keep mode 0/4 and unknown modes on the original tight path.
    //    Each source nibble already contains its post-pal_map index.
    uint8_t *dst = (uint8_t *)screen->data;
    const uint8_t *src = (const uint8_t *)frontbuffer;
    const int stride = screen->stride;
    const uint8_t mode = engine_screen_mode();
    if (mode != last_screen_mode) {
        RG_LOGI("pico8: screen mode %u", (unsigned)mode);
        last_screen_mode = mode;
    }

    if (mode == 1) {
        // Stretch the left 64x128 half horizontally.
        for (int y = 0; y < 128; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d = dst + y * stride;
            for (int x = 0; x < 32; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 4;
                d[dx] = d[dx + 1] = packed & 0x0f;
                d[dx + 2] = d[dx + 3] = packed >> 4;
            }
        }
    } else if (mode == 2) {
        // Stretch the top 128x64 half vertically.
        for (int y = 0; y < 64; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d0 = dst + (y * 2) * stride;
            uint8_t *d1 = d0 + stride;
            for (int x = 0; x < 64; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 2;
                d0[dx] = d1[dx] = packed & 0x0f;
                d0[dx + 1] = d1[dx + 1] = packed >> 4;
            }
        }
    } else if (mode == 3) {
        // Stretch the top-left 64x64 quarter in both dimensions.
        for (int y = 0; y < 64; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d0 = dst + (y * 2) * stride;
            uint8_t *d1 = d0 + stride;
            for (int x = 0; x < 32; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 4;
                d0[dx] = d0[dx + 1] = d1[dx] = d1[dx + 1] = packed & 0x0f;
                d0[dx + 2] = d0[dx + 3] = d1[dx + 2] = d1[dx + 3] = packed >> 4;
            }
        }
    } else if (mode == 5) {
        // Mirror the left 64x128 half onto the right half.
        for (int y = 0; y < 128; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d = dst + y * stride;
            for (int x = 0; x < 32; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 2;
                d[dx] = d[127 - dx] = packed & 0x0f;
                d[dx + 1] = d[126 - dx] = packed >> 4;
            }
        }
    } else if (mode == 6) {
        // Mirror the top 128x64 half onto the bottom half.
        for (int y = 0; y < 64; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d0 = dst + y * stride;
            uint8_t *d1 = dst + (127 - y) * stride;
            for (int x = 0; x < 64; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 2;
                d0[dx] = d1[dx] = packed & 0x0f;
                d0[dx + 1] = d1[dx + 1] = packed >> 4;
            }
        }
    } else if (mode == 7) {
        // Mirror the top-left 64x64 quarter into all four quadrants.
        for (int y = 0; y < 64; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d0 = dst + y * stride;
            uint8_t *d1 = dst + (127 - y) * stride;
            for (int x = 0; x < 32; ++x) {
                const uint8_t packed = s[x];
                const int dx = x * 2;
                const uint8_t lo = packed & 0x0f;
                const uint8_t hi = packed >> 4;
                d0[dx] = d1[dx] = d0[127 - dx] = d1[127 - dx] = lo;
                d0[dx + 1] = d1[dx + 1] = d0[126 - dx] = d1[126 - dx] = hi;
            }
        }
    } else {
        for (int y = 0; y < 128; ++y) {
            const uint8_t *s = src + y * 64;
            uint8_t *d = dst + y * stride;
            for (int x = 0; x < 64; ++x) {
                const uint8_t packed = s[x];
                d[x * 2] = packed & 0x0f;
                d[x * 2 + 1] = packed >> 4;
            }
        }
    }

    rg_display_submit(screen, 0);
}

void gfx_redraw(void)
{
    if (screen) rg_display_submit(screen, 0);
}

bool gfx_screenshot(const char *filename, int width, int height)
{
    return screen && rg_surface_save_image_file(screen, filename, width, height);
}

// HUD overlay: retro-go shows its own bottom strip (battery/FPS/etc.) on
// `RG_KEY_MENU`. The pygame-style HUD overlay in hud.c is left unused.
void draw_hud(void) { }

// --- Time -----------------------------------------------------------------

void delay(uint16_t ms)
{
    rg_task_delay(ms);
}

uint32_t now(void)
{
    // engine.c expects ms.
    return (uint32_t)(rg_system_timer() / 1000);
}

// --- Input ----------------------------------------------------------------

// PICO-8 button layout (see BTN_IDX_* in engine.h and pico8api.c::btn):
//   0 = LEFT, 1 = RIGHT, 2 = UP, 3 = DOWN, 4 = O (Z), 5 = X (X),
//   6 = PAUSE/ENTER. Retro-Go's START key is distinct from MENU/OPTION, so
//   carts can use the PICO-8 pause/Enter bit without losing system menus.
static const struct { rg_key_t key; uint8_t button; } pico8_keymap[] = {
    { RG_KEY_LEFT,  0 },
    { RG_KEY_RIGHT, 1 },
    { RG_KEY_UP,    2 },
    { RG_KEY_DOWN,  3 },
    { RG_KEY_A,     4 }, // PICO-8 O
    { RG_KEY_B,     5 }, // PICO-8 X
    { RG_KEY_START, 6 }, // PICO-8 pause / Enter
};
#define PICO8_KEYMAP_LEN (sizeof(pico8_keymap) / sizeof(pico8_keymap[0]))
static uint32_t prev_gamepad = 0;

bool handle_input(void)
{
    // Read once; compute edge transitions vs previous frame so we can fill
    // both `buttons` (currently held) and `buttons_frame` (just-pressed).
    uint32_t gamepad = rg_input_read_gamepad();
    uint32_t pressed_now = gamepad & ~prev_gamepad;

    memset(buttons,       0, sizeof(buttons));
    memset(buttons_frame, 0, sizeof(buttons_frame));

    for (size_t i = 0; i < PICO8_KEYMAP_LEN; i++) {
        if (gamepad & pico8_keymap[i].key) {
            buttons[pico8_keymap[i].button] = 1;
        }
        if (pressed_now & pico8_keymap[i].key) {
            buttons_frame[pico8_keymap[i].button] = 1;
        }
    }
    prev_gamepad = gamepad;

    return false; // not quitting; retro-go owns long-press RG_KEY_MENU
}

void handle_input_reset(void)
{
    prev_gamepad = rg_input_read_gamepad();
    memset(buttons, 0, sizeof(buttons));
    memset(buttons_frame, 0, sizeof(buttons_frame));
    engine_input_reset();
}

// --- Audio (pull model) ---------------------------------------------------

#define MONO_SAMPLES_PER_TICK  183  // Must match SAMPLES_PER_DURATION in sfx.c.
// Hardcoded rather than referencing the `extern const uint8_t` to avoid a
// Variable-Length Array (VLA) at runtime.  `SAMPLES_PER_DURATION` is not a
// preprocessor macro — it is an `extern const` defined in sfx.c, so using it
// as an array size makes `uint16_t mono_buf[MONO_SAMPLES_PER_TICK]` a VLA.
// On Xtensa (ESP32-S3) with windowed registers, a VLA can corrupt the stack
// frame pointer, causing register window spills/restores to read the 0xcccccccc
// uninitialized-fill pattern — which is the exact StoreProhibited panic we saw
// on Cab Ride.p8.png: A9/A10 = 0xcccccccc after the first sfx() call.  Using
// a literal constant forces compile-time sizing and eliminates the VLA.

static rg_audio_frame_t *stereo_frames = NULL;
static rg_task_t        *audio_task    = NULL;
// The synth owns channels[] while generating a block. Start paused so the
// cart task can initialise/replace SFX data before Core 1 first reads it.
// This pause is only used for boot, menus and cart transitions; ordinary
// gameplay never touches these atomics.
static atomic_bool       audio_paused  = ATOMIC_VAR_INIT(true);
static atomic_bool       audio_active  = ATOMIC_VAR_INIT(false);

static void audio_task_fn(void *arg)
{
    (void)arg;
    // Mono mixdown on stack. Accumulate channels in signed 32-bit precision,
    // then saturate once when converting to the S16 Retro-Go output format.
    // `buf[_offset] += sample` so passing the same buffer for all four
    // channels accumulates their samples (true PICO-8 mixer semantics).
    int32_t mono_buf[MONO_SAMPLES_PER_TICK];

    // Stack-local backup of the stereo_frames pointer.  The BSS copy can
    // be corrupted by a C-stack overflow on Core 0 (Lua thread) that
    // spills into .bss and overwrites stereo_frames with 0xcccccccc.
    // Because this variable lives on Core 1's audio task stack, it is
    // immune to Core 0's stack overflow.  We use it to restore the BSS
    // pointer if corruption is detected.
    rg_audio_frame_t *local_frames = stereo_frames;

    while (1) {
        rg_task_msg_t msg;
        if (rg_task_peek(&msg, 0) && msg.type == RG_TASK_MSG_STOP) {
            atomic_store_explicit(&audio_active, false, memory_order_release);
            free(local_frames);
            stereo_frames = NULL;
            local_frames = NULL;
            break;
        }

        if (atomic_load_explicit(&audio_paused, memory_order_acquire)) {
            rg_task_delay(1);
            continue;
        }

        // Close the small check-to-render window: a pausing cart task sets
        // audio_paused first and waits for audio_active to clear. If it won
        // the race between these two stores, do not touch synth state.
        atomic_store_explicit(&audio_active, true, memory_order_release);
        if (atomic_load_explicit(&audio_paused, memory_order_acquire)) {
            atomic_store_explicit(&audio_active, false, memory_order_release);
            continue;
        }

        memset(mono_buf, 0, sizeof(mono_buf));
        music_update(MONO_SAMPLES_PER_TICK);
        for (int c = 0; c < 4; c++) {
            fill_buffer(mono_buf, &channels[c], MONO_SAMPLES_PER_TICK);
        }

        // Guard: stereo_frames BSS pointer can be overwritten with
        // 0xcccccccc by a C-stack overflow from the Lua thread on Core 0.
        // Restore from our stack-local backup instead of re-allocating,
        // which avoids heap churn and is safe because the backup pointer
        // lives on this task's stack (immune to Core 0's overflow).
        if (!stereo_frames || (uintptr_t)stereo_frames == 0xCCCCCCCCUL) {
            RG_LOGE("pico8: stereo_frames corrupted (%p), restoring from backup",
                    (void *)stereo_frames);
            stereo_frames = local_frames;
            if (!stereo_frames) {
                // First call before init_audio finished — shouldn't happen
                // but handle gracefully.
                atomic_store_explicit(&audio_active, false,
                                      memory_order_release);
                atomic_store_explicit(&audio_active, false,
                                      memory_order_release);
                rg_task_yield();
                continue;
            }
        }

        // Signed mono -> interleaved S16 stereo. Saturation after the complete
        // four-channel mix avoids both unsigned wrap and per-channel clipping.
        // PICO-8's waveforms are normalized per voice; reserve 6 dB of master
        // headroom so coincident peaks from up to four voices do not spend
        // their tops pinned against the S16 limiter.
        for (int j = 0; j < MONO_SAMPLES_PER_TICK; j++) {
            int32_t mixed = mono_buf[j] / 2;
            if (mixed > INT16_MAX) mixed = INT16_MAX;
            else if (mixed < INT16_MIN) mixed = INT16_MIN;
            int16_t s = (int16_t)mixed;
            stereo_frames[j].left  = s;
            stereo_frames[j].right = s;
        }
        rg_audio_submit(stereo_frames, MONO_SAMPLES_PER_TICK);

        atomic_store_explicit(&audio_active, false, memory_order_release);

        rg_task_yield();
    }
    RG_LOGI("pico8 audio task exiting");
}

bool init_audio(void)
{
    RG_LOGI("pico8: init_audio (synth task on core 1)");

    atomic_store_explicit(&audio_paused, true, memory_order_release);
    atomic_store_explicit(&audio_active, false, memory_order_release);

    stereo_frames = rg_alloc(MONO_SAMPLES_PER_TICK * sizeof(rg_audio_frame_t),
                             MEM_FAST);
    if (!stereo_frames) {
        return false;
    }

    audio_task = rg_task_create("pico8_audio", audio_task_fn, NULL,
                                /*stack*/  4096,
                                /*queue*/ 8,
                                RG_TASK_PRIORITY_2,
                                /*affinity*/ 1);  // core 1
    if (!audio_task) {
        free(stereo_frames);
        stereo_frames = NULL;
        return false;
    }
    return true;
}

void audio_task_set_paused(bool paused)
{
    atomic_store_explicit(&audio_paused, paused, memory_order_release);
    if (paused) {
        // One 183-sample block is at most about 8.3 ms. Waiting for its owner
        // prevents cartParser()/engine_init() replacing SFX/channel state
        // underneath Core 1 without placing a mutex in the mixer hot path.
        while (atomic_load_explicit(&audio_active, memory_order_acquire))
            rg_task_yield();
    }
}

// On shutdown, app_main's event_cb will call rg_audio_submit cleanup
// via RG_TASK_MSG_STOP. Helper exposed for main.c.
void audio_task_stop(void)
{
    // Guard against corrupted audio_task pointer (0xcccccccc from
    // C-stack overflow on Core 0 into .bss).
    if (!audio_task || (uintptr_t)audio_task == 0xCCCCCCCCUL) {
        RG_LOGE("pico8: audio_task corrupted (%p), clearing",
                (void *)audio_task);
        audio_task = NULL;
        return;
    }
    rg_task_msg_t msg = { .type = RG_TASK_MSG_STOP };
    rg_task_send(audio_task, &msg, 1000);
    audio_task = NULL;
}

// --- Platform / status ----------------------------------------------------

bool init_platform(void)
{
    // retro-go already initialised everything via rg_system_init.
    bootup_time = now();
    return true;
}

uint8_t current_hour(void)    { return 0; }
uint8_t current_minute(void)  { return 0; }
uint8_t wifi_strength(void)   { return 0; }

uint8_t battery_left(void)
{
    rg_battery_t b = rg_input_read_battery();
    if (!b.present) return 0;
    if (b.level < 0.0f) b.level = 0.0f;
    if (b.level > 1.0f) b.level = 1.0f;
    return (uint8_t)(b.level * 100.0f);
}
