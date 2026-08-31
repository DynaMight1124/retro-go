#ifndef ENGINE
#define ENGINE
#include "data.h"
#include "engine.h"
#include <rg_system.h>   // rg_system_panic
#include <rg_utils.h>    // rg_alloc, MEM_SLOW

// backend.h exposes the platform shim (now()/gfx_flip()/delay()/handle_input()
// etc.) with explicit `extern "C"` linkage. We include it here so the
// declarations are visible to the engine TU's own function bodies (init_lua,
// flip) BEFORE the text-included pico8api.c drags the same header into scope
// — without an early declaration, GCC's implicit-function-declaration
// fallback produces an unmangled call site that the C TU's unmangled
// definition can't satisfy at link time.
// pico8_globals.h transitively comes via data.h; it declares the byte-array
// externs (stdlib_stdlib_lua, artifacts_*_lua, artifacts_*_p8). The matching
// `extern`-prefixed stub definitions live in pico8_globals.c.

// z8::fix32 is required by synth.c, sfx.c, and pico8api.c (text-included
// below). engine.h no longer includes lua/fix32.h (it would pull C++ headers
// into C consumers of engine.h), so we include it explicitly here. This
// header is C++-only by design — the directory `lua/` is added to the
// component's include path with C++17 set in CMakeLists.txt.
#include "lua/fix32.h"

// Engine-TU-private singletons. These used to live in data.h as `static`
// declarations but caused `-Wunused-variable` warnings in every non-engine
// TU (pico8_globals.c, retro_go_backend.c, p8_text_parser.cpp) because each
// of those got a private copy that they never touched. Centralising them
// here means only the engine TU carries them in BSS.
// PICO-8 RAM is 32 KB spanning addresses 0x0000..0x7FFF. The Lua runtime's
// OP_POKE / OP_PEEK opcodes (lvm.cpp) and the pico8api.c::_lua_poke /
// _lua_poke4 C-bindings both use the raw address as an index into this
// array — i.e. a cart's `poke(0x4300, value)` writes ram[0x4300], NOT
// ram[addr - 0x4300]. The earlier `0x5DFF - 0x4300` (= 6911) allocation
// caused every cart's poke() to overrun into adjacent BSS, smashing
// `DrawState drawstate` and Lua VM pointers. Manifested as:
//
//   * `Lua error in _draw/_update: attempt to index local X (a number
//     value)` — Lua state corruption made locals look like numbers.
//   * `assert failed: 0x4037.../` panic with reboot — Lua GC hit a
//     corrupted heap pointer.
//
// Allocating the full 32 KB eliminates the overrun; OOB is still
// possible at the boundary (poke(0x8000, ...)) so add explicit bounds
// checks in _lua_poke / _lua_poke4 as a belt-and-braces guard.
static const uint32_t P8_RAM_SIZE = 0x10000;
static const uint32_t P8_CART_ROM_SIZE = 0x4300;
static uint8_t ram[0x8000];      // hot cartridge/display RAM
static uint8_t *ram_high = NULL; // 0x8000..0xffff work RAM, allocated in PSRAM
// Keep an immutable copy of the packed cartridge data for reload().  This is
// cold data (read only when a cart explicitly reloads memory), so prefer
// PSRAM rather than spending another 17 KB of latency-sensitive internal RAM.
// rg_alloc() safely falls back to general 8-bit memory when PSRAM is absent.
static uint8_t *cart_rom = NULL;
static size_t cart_rom_len = 0;
static int engine_last_busy_us = 0;
static int engine_tick_rate = 30;
static int engine_requested_frame_rate = 0;
static bool engine_draw_frame = true;
// PICO-8's framebuffer is effectively live during long _init() work. Some
// carts call flip(), then draw a loading message before spending many seconds
// generating data. Retro-Go submits a copied surface only at flip boundaries,
// so allow one follow-up presentation after that first explicit init flip.
// This state is consumed by _lua_print() and has no steady-state frame cost.
static bool engine_inside_init = false;
static bool engine_init_followup_pending = false;
static bool engine_init_followup_presented = false;
static uint32_t engine_init_progress_last_ms = 0;
static DrawState drawstate;

// PICO-8 exposes five cartridge-defined pause-menu entries. Lua owns the
// callback functions through registry references; the fixed label storage is
// safe to hand to Retro-Go's asynchronous menu renderer.
struct CartMenuItem {
    char label[17];
    int callback_ref;
    uint16_t input_mask;
};
static CartMenuItem cart_menu_items[5];
static int active_cart_menu_item = -1;

enum {
    P8_CART_REQUEST_NONE = 0,
    P8_CART_REQUEST_LOAD = 1,
    P8_CART_REQUEST_BREADCRUMB = 2,
    P8_CART_REQUEST_RUN = 3,
};
#define P8_CART_REQUEST_TEXT_MAX 255
static int pending_cart_request = P8_CART_REQUEST_NONE;
static char pending_cart_filename[P8_CART_REQUEST_TEXT_MAX + 1];
static char pending_cart_breadcrumb[P8_CART_REQUEST_TEXT_MAX + 1];
static char pending_cart_param[P8_CART_REQUEST_TEXT_MAX + 1];
static char current_cart_param[P8_CART_REQUEST_TEXT_MAX + 1];

// PICO-8 permits cstore(..., filename) to modify a companion cart before a
// load(). Keep that hand-off transient and in PSRAM rather than rewriting the
// ROM on SD. A compact range list avoids a second 17 KB validity bitmap;
// real multicarts normally issue only a handful of contiguous transfers.
#define P8_CROSS_CART_STORE_RANGES 8
struct CrossCartStoreRange {
    uint16_t dst;
    uint16_t len;
};
static uint8_t *cross_cart_store_data = NULL;
static CrossCartStoreRange cross_cart_store_ranges[P8_CROSS_CART_STORE_RANGES];
static uint8_t cross_cart_store_range_count = 0;
static bool cross_cart_store_pending = false;

#include "parser.c"
#include "synth.c"
#include "sfx.c"
#include "pico8api.c"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include <stdio.h>      // printf() across the TU; (cart_panic_handler used to fprintf but now uses rg_system_panic)
#include <string.h>

static bool     wants_to_quit = false;

static lua_State *L = NULL;
void registerLuaFunctions(void);

extern "C" void engine_set_frame_stats(int busy_us, int tick_rate) {
    engine_last_busy_us = busy_us < 0 ? 0 : busy_us;
    engine_tick_rate = tick_rate > 0 ? tick_rate : 30;
}

// The display-mode register changes only how the completed 128x128 packed
// framebuffer is presented. Keep that transformation in the Retro-Go
// backend, but expose the byte through this narrow bridge because PICO-8 RAM
// is intentionally private to the engine translation unit.
extern "C" uint8_t engine_screen_mode(void) {
    return ram[0x5f2c];
}

// `flip()` resets the cart's PICO-8 frame order on every call and guards
// `_init` with a once-only flag. The flag previously lived inside flip()
// as a static-local, which meant a hard reset (retro-go reset_cb ->
// engine_init()) couldn't re-fire `_init`. Move it to module scope so
// engine_init() can clear it.
static bool     _init_done = false;
static bool     cart_runtime_failed = false;
static uint32_t button_hold_frames[7] = {0};

extern "C" void engine_input_reset(void) {
    memset(button_hold_frames, 0, sizeof(button_hold_frames));
}

extern "C" int engine_take_cart_request(char *filename, size_t filename_size,
                                         char *breadcrumb, size_t breadcrumb_size,
                                         char *param, size_t param_size) {
    int request = pending_cart_request;
    if (request == P8_CART_REQUEST_NONE) return request;

    if (filename && filename_size)
        snprintf(filename, filename_size, "%s", pending_cart_filename);
    if (breadcrumb && breadcrumb_size)
        snprintf(breadcrumb, breadcrumb_size, "%s", pending_cart_breadcrumb);
    if (param && param_size)
        snprintf(param, param_size, "%s", pending_cart_param);

    pending_cart_request = P8_CART_REQUEST_NONE;
    pending_cart_filename[0] = '\0';
    pending_cart_breadcrumb[0] = '\0';
    pending_cart_param[0] = '\0';
    return request;
}

extern "C" void engine_set_cart_param(const char *param) {
    snprintf(current_cart_param, sizeof(current_cart_param), "%s",
             param ? param : "");
}

extern "C" bool engine_request_restart(const char *param) {
    // Retro-Go invokes reset while the frame loop is paused in its game menu.
    // Defer VM destruction until control returns to main.c, exactly as run()
    // does after a protected Lua callback. Never overwrite a transition that
    // the cart already requested.
    if (pending_cart_request != P8_CART_REQUEST_NONE) {
        RG_LOGW("pico8: reset ignored while cart request %d is pending",
                pending_cart_request);
        return false;
    }
    pending_cart_filename[0] = '\0';
    pending_cart_breadcrumb[0] = '\0';
    snprintf(pending_cart_param, sizeof(pending_cart_param), "%s",
             param ? param : "");
    pending_cart_request = P8_CART_REQUEST_RUN;
    return true;
}

extern "C" bool engine_cart_request_pending(void) {
    return pending_cart_request != P8_CART_REQUEST_NONE;
}

// Complete btnp()'s held-button behaviour after the backend has populated
// buttons/buttons_frame. PICO-8's 0x5f5c/0x5f5d values are expressed in
// 30 Hz frames, so _update60 carts use twice the configured delay. A zero
// register selects the documented defaults; 255 disables repeat entirely.
static void update_button_repeat(bool at_60fps) {
    const uint32_t scale = at_60fps ? 2u : 1u;
    const uint8_t configured_delay = ram[0x5f5c];
    const uint32_t delay = (configured_delay ? configured_delay : 15u) * scale;
    const uint32_t rate = (ram[0x5f5d] ? ram[0x5f5d] : 4u) * scale;

    for (int i = 0; i < 7; ++i) {
        if (!buttons[i]) {
            button_hold_frames[i] = 0;
            continue;
        }

        if (buttons_frame[i]) {
            // The backend identified a real 0->1 edge. Starting the counter
            // here also lets handle_input_reset suppress buttons carried out
            // of Retro-Go's menu until they are released and pressed again.
            button_hold_frames[i] = 1;
            continue;
        }

        if (button_hold_frames[i] == 0) continue;
        ++button_hold_frames[i];
        if (configured_delay == 255) continue;

        const uint32_t held = button_hold_frames[i];
        if (held > delay && ((held - delay - 1u) % rate) == 0)
            buttons_frame[i] = 1;
    }
}

// Lua's default atpanic returns to the host framework in a way that's
// opaque to ESP32: a Lua-internal uncaught error (e.g. `__index` metamethod
// looping, recursion past a Lua-side limit) bubbles back out as the next
// instruction fetch landing on a NULL closure pointer, surfacing as
// `Guru Meditation ... InstrFetchProhibited at 0x00000000` with no
// recoverable context. Install our own handler that hands the message
// to `rg_system_panic()` (which paints a blue crash screen with the
// reason on the LCD and reboots) so the user — without a serial
// console — can actually read what the cart was trying to do when it
// panicked. The `__attribute__((noreturn))` on rg_system_panic means
// the function never returns to Lua, so the re-raise path is never
// entered.
static int cart_panic_handler(lua_State* _L) {
    const char* msg = lua_tostring(_L, -1);
    rg_system_panic("Lua Panic", msg ? msg : "(no message)");
    return 0;  // unreachable — rg_system_panic is noreturn
}

static int cart_traceback_handler(lua_State *state);

static int p_init_lua(lua_State* _L) {
    luaL_checkversion(_L);
    lua_gc(_L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(_L);  /* open libraries */
    lua_gc(_L, LUA_GCRESTART, 0);
    return 1;
}

bool init_lua(const uint8_t* bytecode, uint16_t code_len) {
    L = luaL_newstate();
    if (L == NULL) {
        printf("cannot create LUA state: not enough memory\n");
        return false;
    }
    // Install the panic handler BEFORE any user code runs so an in-cart
    // panic (Lua-level, not the C-stack overflow case above) logs to the
    // serial console and at least carries the reason alongside the
    // reboot banner — without this, the EngineLoop reboot prints only
    // `Guru Meditation Error: ... InstrFetchProhibited` with no message
    // about what the cart was trying to do.
    lua_atpanic(L, cart_panic_handler);
    lua_setpico8memory(L, ram);
    lua_setpico8memoryhigh(L, ram_high);
    lua_pushcfunction(L, &p_init_lua);

    int status = lua_pcall(L, 0, 1, 0);
    if (status != LUA_OK) {
        // The result of lua_toboolean(L, -1) is currently unused (we error
        // out either way) but keep the call to log the failure reason via
        // lua_tostring.
        (void)lua_toboolean(L, -1);
        printf("Error loading lua VM: %s\n", lua_tostring(L, lua_gettop(L)));
        return false;
    }

    registerLuaFunctions();

    // Preserve native helpers whose compatibility-prelude definitions are
    // either slower or less accurate. In particular, the Lua bitwise
    // fallbacks only terminate for small non-negative integers; native
    // fixed-point operations correctly handle values such as 0xffff.fffe.
    // Native flr preserves the prelude's missing-argument zero while avoiding
    // a Lua wrapper plus math.floor call for this exceptionally hot helper.
    // Native min/max preserve the required nil-first behaviour while also
    // applying PICO-8's boolean/numeric-string coercion.
    static const char native_names[][6] = {
        {'c', 'o', 's', 0},
        {'s', 'i', 'n', 0},
        {'a', 't', 'a', 'n', '2', 0},
        {'s', 'q', 'r', 't', 0},
        {'a', 'b', 's', 0},
        {'c', 'e', 'i', 'l', 0},
        {'f', 'l', 'r', 0},
        {'s', 'g', 'n', 0},
        {'m', 'a', 'x', 0},
        {'m', 'i', 'n', 0},
        {'m', 'i', 'd', 0},
        {'b', 'a', 'n', 'd', 0},
        {'b', 'o', 'r', 0},
        {'b', 'x', 'o', 'r', 0},
        {'b', 'n', 'o', 't', 0},
        {'s', 'h', 'l', 0},
        {'s', 'h', 'r', 0},
        {'l', 's', 'h', 'r', 0},
        {'r', 'o', 't', 'l', 0},
        {'r', 'o', 't', 'r', 0},
    };
    static const int native_count =
        (int)(sizeof(native_names) / sizeof(native_names[0]));
    int native_refs[native_count];
    for (int i = 0; i < native_count; ++i) {
        lua_getglobal(L, native_names[i]);
        native_refs[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    uint32_t start_time = now();
    uint32_t end_time;
    int error = luaL_loadbuffer(L, (const char*)stdlib_stdlib_lua, stdlib_stdlib_lua_len, "stdlib") || lua_pcall(L, 0, 0, 0);
    if (error) {
        goto handle_error;
    }

    for (int i = 0; i < native_count; ++i) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, native_refs[i]);
        lua_setglobal(L, native_names[i]);
        luaL_unref(L, LUA_REGISTRYINDEX, native_refs[i]);
    }

    end_time = now();
    // %u (not %d) because end_time/start_time are uint32_t.
    printf("stdlib loaded, took %ums\n", (unsigned)(end_time-start_time));
    start_time = now();

    error = luaL_loadbuffer(L, (const char*)bytecode, code_len, "cart");
    if (error == LUA_OK) {
        // Top-level cart code commonly performs data decoding before _init is
        // defined. Use the same protected traceback as frame callbacks so a
        // startup error identifies both the helper and its call site instead
        // of reporting only one densely-minified source line.
        lua_pushcfunction(L, cart_traceback_handler);
        lua_insert(L, -2);
        int error_handler = lua_gettop(L) - 1;
        error = lua_pcall(L, 0, 0, error_handler);
        if (error == LUA_OK)
            lua_remove(L, error_handler);
    }
    if (error) {
        goto handle_error;
    }

    end_time = now();
    printf("cart loaded, took %ums\n", (unsigned)(end_time-start_time));
    return true;

handle_error:
    printf("Fail: %s\n", lua_tostring(L, lua_gettop(L)));
    lua_close(L);
    L = NULL;
    return false;
}

void reset_transparency() {
    memset(drawstate.transparent, 0, sizeof(drawstate.transparent));
    drawstate.transparent[0] = 1;
    for (int i = 0; i < 16; ++i) {
        ram[0x5f00 + i] = (uint8_t)((pal_map[i] & 0x0f)
                         | (drawstate.transparent[i] ? 0x10 : 0));
    }
}
void engine_init(void) {
    // PICO-8 screen memory is RAM 0x6000..0x7fff. Use it directly as the
    // packed framebuffer so drawing, peek/poke and VM memory opcodes remain
    // coherent without a second per-pixel store.
    frontbuffer = ram + 0x6000;
    engine_inside_init = false;
    engine_init_followup_pending = false;
    engine_init_followup_presented = false;
    engine_init_progress_last_ms = 0;
    engine_requested_frame_rate = 0;

    // Reset first so any subsequent flip() that races against this
    // engine_init() call re-runs the cart's _init. Without this, a
    // hard reset from the Retro-Go menu dismisses the menu but the
    // frozen cart state stays in place until power-cycle.
    _init_done = false;
    cart_runtime_failed = false;
    engine_input_reset();
    p8_tline_reset();

    if (L) {
        for (int i = 0; i < 5; ++i) {
            if (cart_menu_items[i].callback_ref >= 0)
                luaL_unref(L, LUA_REGISTRYINDEX,
                           cart_menu_items[i].callback_ref);
        }
    }
    for (int i = 0; i < 5; ++i) {
        cart_menu_items[i].label[0] = '\0';
        cart_menu_items[i].callback_ref = LUA_NOREF;
        cart_menu_items[i].input_mask = 0;
    }
    active_cart_menu_item = -1;

    // DrawState defaults (formerly default-member initializers in data.h —
    // stripped so data.h is C-compatible. We reapply them explicitly here so
    // drawstate matches the engine's documented PICO-8 initial state.)
    drawstate.pen_color = 7;
    drawstate.bg_color  = 0;
    drawstate.clip_x     = 0;
    drawstate.clip_y     = 0;
    drawstate.clip_w     = SCREEN_WIDTH;
    drawstate.clip_h     = SCREEN_HEIGHT;
    drawstate.camera_x   = 0;
    drawstate.camera_y   = 0;
    drawstate.line_x     = 0;
    drawstate.line_y     = 0;
    drawstate.line_active = 0;
    drawstate.cursor_x   = 0;
    drawstate.cursor_y   = 0;
    drawstate.fill_pattern = 0;
    drawstate.fill_flags = 0;

    memset(&fontsheet.sprite_data, 0xFF, sizeof(fontsheet.sprite_data));
    memset(map_data, 0, sizeof(map_data));
    memset(ram, 0, sizeof(ram));
    // PICO-8 starts each cart with a live PRNG rather than an all-zero state
    // (which the rotate/add generator can never escape). Explicit srand()
    // calls remain deterministic and replace this state completely.
    p8_rng_seed_bits(now());
    ram[0x5f20] = 0;
    ram[0x5f21] = 0;
    ram[0x5f22] = SCREEN_WIDTH;
    ram[0x5f23] = SCREEN_HEIGHT;
    ram[0x5f25] = drawstate.pen_color;
    ram[0x5f26] = drawstate.cursor_x;
    ram[0x5f27] = drawstate.cursor_y;
    reset_all_palettes();
    // PICO-8 hardware-state defaults. Carts may relocate map storage by
    // poking 0x5f56/0x5f57; Pizza Panda moves it to 0x8000 so its level data
    // does not overwrite the title artwork in shared GFX2/MAP2 memory.
    ram[0x5f54] = 0x00;
    ram[0x5f55] = 0x60;
    ram[0x5f56] = 0x20;
    ram[0x5f57] = 0x80;
    if (ram_high == NULL)
        ram_high = (uint8_t *)rg_alloc(0x8000, MEM_SLOW);
    if (ram_high == NULL)
        RG_PANIC("pico8: unable to allocate upper 32 KB RAM");
    memset(ram_high, 0, 0x8000);

    if (!cart_rom)
        cart_rom = (uint8_t *)rg_alloc(P8_CART_ROM_SIZE, MEM_SLOW);
    memset(cart_rom, 0, P8_CART_ROM_SIZE);
    cart_rom_len = 0;

    memset(cartdata, 0, sizeof(cartdata));

    memset(audiobuf, 0, sizeof(audiobuf));
    music_reset();

    channels[0].id = 0;
    channels[1].id = 1;
    channels[2].id = 2;
    channels[3].id = 3;

    channels[0].sfx = NULL;
    channels[1].sfx = NULL;
    channels[2].sfx = NULL;
    channels[3].sfx = NULL;

    for (int c = 0; c < 4; ++c) {
        channels[c].sfx_id = 0;
        channels[c].offset = 0;
        channels[c].end_note = NOTES_PER_SFX;
        channels[c].is_music = 0;
        channels[c].loop_released = 0;
        channels[c].phi = int32_t{0};
        reset_channel_instrument(&channels[c]);
        reset_channel_noise(&channels[c], (uint32_t)c);
    }

    // FX_SLIDE prev_ defaults at boot: zero means the first note's
    // FX_SLIDE on a fresh engine_init would slide from key=0 (very low)
    // to the note's key. Carts reset these via sfx() calls (see
    // pico8api.c::_lua_sfx) before any note plays, so this is the
    // baseline only until the first sfx() lands.
    channels[0].prev_key = 0;
    channels[0].prev_vol = 0;
    channels[1].prev_key = 0;
    channels[1].prev_vol = 0;
    channels[2].prev_key = 0;
    channels[2].prev_vol = 0;
    channels[3].prev_key = 0;
    channels[3].prev_vol = 0;

    printf("Parsing font \n");
    assert(artifacts_font_lua_len <= sizeof(fontsheet.sprite_data));
    memcpy(fontsheet.sprite_data, artifacts_font_lua, artifacts_font_lua_len);

//    init_pink_noise_gen(&osc);
}

extern "C" void engine_prepare_cart_load(void) {
    // PICO-8 load() replaces cart ROM (0x0000..0x42ff) and the Lua program,
    // while preserving the machine RAM above it. Snekburd deliberately keeps
    // an auxiliary sprite bank and checksum in that preserved upper RAM.
    if (L) {
        lua_close(L);
        L = NULL;
    }

    _init_done = false;
    engine_inside_init = false;
    engine_init_followup_pending = false;
    engine_init_followup_presented = false;
    engine_init_progress_last_ms = 0;
    engine_requested_frame_rate = 0;
    cart_runtime_failed = false;
    wants_to_quit = false;
    engine_input_reset();
    p8_tline_reset();

    for (int i = 0; i < 5; ++i) {
        cart_menu_items[i].label[0] = '\0';
        cart_menu_items[i].callback_ref = LUA_NOREF;
        cart_menu_items[i].input_mask = 0;
    }
    active_cart_menu_item = -1;

    // load() starts the replacement cart with PICO-8's default display
    // palette. Keep general upper RAM intact for multicart hand-off data,
    // but do not leak the outgoing cart's 0x5f10..0x5f1f colour remaps.
    // Pico Night Punkin fades that palette completely black before loading
    // fnf-select.p8, whose first frames assume the normal display palette.
    reset_display_palette();

    // Stop voices before their SFX backing data is replaced by cartParser().
    music_reset();
    memset(audiobuf, 0, sizeof(audiobuf));
    for (int c = 0; c < 4; ++c) {
        channels[c].sfx = NULL;
        channels[c].sfx_id = 0;
        channels[c].offset = 0;
        channels[c].end_note = NOTES_PER_SFX;
        channels[c].is_music = 0;
        channels[c].loop_released = 0;
        channels[c].phi = int32_t{0};
        channels[c].prev_key = 0;
        channels[c].prev_vol = 0;
        reset_channel_noise(&channels[c], (uint32_t)c);
    }

    memset(cart_rom, 0, P8_CART_ROM_SIZE);
    cart_rom_len = 0;
}

void cartParser(const GameCart* parsingCart) {
	// Load the canonical packed cart ROM before Lua executes. In particular,
	// Celeste 2 keeps PX9-compressed levels inside gfx memory at 0x1000 and
	// reads them through peek(); the old split-only loader left ram[] zeroed.
	if (parsingCart->rom && parsingCart->rom_len) {
		size_t rom_len = parsingCart->rom_len;
		if (rom_len > P8_CART_ROM_SIZE) rom_len = P8_CART_ROM_SIZE;
		memcpy(cart_rom, parsingCart->rom, rom_len);
		cart_rom_len = rom_len;
		memcpy(ram, parsingCart->rom, rom_len);
		uint32_t checksum = 2166136261u;
		for (size_t i = 0; i < rom_len; ++i) {
			checksum ^= ram[i];
			checksum *= 16777619u;
		}
		RG_LOGI("pico8: loaded %u-byte cart ROM into RAM (fnv=%08lx)",
		        (unsigned)rom_len, (unsigned long)checksum);
	}

	assert(parsingCart->gfx_len <= sizeof(spritesheet.sprite_data));
	memcpy(spritesheet.sprite_data, parsingCart->gfx, parsingCart->gfx_len);

	assert(parsingCart->gff_len <= sizeof(spritesheet.flags));
	memcpy(spritesheet.flags, parsingCart->gff, parsingCart->gff_len);

	assert(parsingCart->map_len <= sizeof(map_data));
	memcpy(map_data, parsingCart->map, parsingCart->map_len);

        if (parsingCart->gfx_len > (64*128)) { // 64 half-sized lines (128bytes) == 32 256 lines
                                               // these are LSB and have to be flipped
            for(uint16_t i=32; i<(parsingCart->gfx_len/256); i++) {
                mapParser(parsingCart->gfx+(i*256), i, map_data);
            }
        }
        for(uint8_t i=0; i<(parsingCart->sfx_len/168); i++) {
                SFXParser(parsingCart->sfx+(i*168), i, sfx);
        }
        memset(music_patterns, 0x40, sizeof(music_patterns));
        if (parsingCart->music) {
            const uint8_t *m = parsingCart->music;
            size_t count = parsingCart->music_len / 11;
            if (count > 64) count = 64;
            for (size_t i = 0; i < count; ++i, m += 11) {
                music_patterns[i].flags = (parseChar(m[0]) << 4) | parseChar(m[1]);
                for (int c = 0; c < 4; ++c) {
                    music_patterns[i].channels[c] =
                        (parseChar(m[3 + c * 2]) << 4) | parseChar(m[4 + c * 2]);
                }
            }
        }

        // Apply any cstore(..., filename) hand-off after the destination
        // cart's normal graphics/map views have been parsed. p8_ram_write()
        // updates canonical RAM and all expanded hot views together.
        if (cross_cart_store_pending && cross_cart_store_data) {
            for (uint8_t range = 0;
                 range < cross_cart_store_range_count; ++range) {
                const uint32_t dst = cross_cart_store_ranges[range].dst;
                const uint32_t len = cross_cart_store_ranges[range].len;
                for (uint32_t i = 0; i < len; ++i) {
                    const uint8_t value = cross_cart_store_data[dst + i];
                    cart_rom[dst + i] = value;
                    p8_ram_write(dst + i, value);
                }
                const size_t end = (size_t)dst + len;
                if (cart_rom_len < end) cart_rom_len = end;
            }
            RG_LOGI("pico8: applied %u cross-cart cstore range(s)",
                    (unsigned)cross_cart_store_range_count);
            cross_cart_store_range_count = 0;
            cross_cart_store_pending = false;
        }
}
void registerLuaFunctions(void) {
    // PICO-8's typeable pattern and button glyphs are ordinary identifier
    // characters whose one-character global names contain numeric constants.
    // Keeping them as globals (rather than lexer tokens) also permits names
    // such as `\x87pos` and table fields such as `{\x8b=...}`.
    static const struct { uint8_t glyph; int32_t value; } glyph_globals[] = {
        {0x80, 0xffff}, {0x81, 0xa5a5}, {0x82, 0x8fbb},
        {0x84, 0x8282}, {0x85, 0x2337}, {0x86, 0x3777},
        {0x87, 0x6773},
        {0x8b, 0}, {0x91, 1}, {0x94, 2},
        {0x83, 3}, {0x8e, 4}, {0x97, 5},
    };
    for (size_t i = 0; i < sizeof(glyph_globals) / sizeof(glyph_globals[0]); ++i) {
        char name[2] = {(char)glyph_globals[i].glyph, '\0'};
        lua_pushinteger(L, glyph_globals[i].value);
        lua_setglobal(L, name);
    }

    lua_pushcfunction(L, _lua_spr);
    lua_setglobal(L, "spr");
    lua_pushcfunction(L, _lua_sspr);
    lua_setglobal(L, "sspr");
    lua_pushcfunction(L, _lua_cls);
    lua_setglobal(L, "cls");
    lua_pushcfunction(L, _lua_palt);
    lua_setglobal(L, "palt");
    lua_pushcfunction(L, _lua_pal);
    lua_setglobal(L, "pal");
    lua_pushcfunction(L, _lua_print);
    lua_setglobal(L, "print");
    lua_pushcfunction(L, _lua_rectfill);
    lua_setglobal(L, "rectfill");
    lua_pushcfunction(L, _lua_rect);
    lua_setglobal(L, "rect");
    lua_pushcfunction(L, _lua_line);
    lua_setglobal(L, "line");
    lua_pushcfunction(L, _lua_tline);
    lua_setglobal(L, "tline");
    lua_pushcfunction(L, _lua_set_fps);
    lua_setglobal(L, "_set_fps");
    lua_pushcfunction(L, _lua_circ);
    lua_setglobal(L, "circ");
    lua_pushcfunction(L, _lua_circfill);
    lua_setglobal(L, "circfill");
    lua_pushcfunction(L, _lua_oval);
    lua_setglobal(L, "oval");
    lua_pushcfunction(L, _lua_ovalfill);
    lua_setglobal(L, "ovalfill");
    lua_pushcfunction(L, _lua_btn);
    lua_setglobal(L, "btn");
    lua_pushcfunction(L, _lua_btnp);
    lua_setglobal(L, "btnp");
    lua_pushcfunction(L, _lua_update_buttons);
    lua_setglobal(L, "_update_buttons");
    lua_pushcfunction(L, _lua_map);
    lua_setglobal(L, "map");
    // Older PICO-8 carts used mapdraw() before the API was shortened to
    // map(). PICO-8 retains the old spelling as a compatibility alias;
    // Across The River calls it directly from _draw(). Register the same
    // native renderer under both names, with no wrapper or runtime branch.
    lua_pushcfunction(L, _lua_map);
    lua_setglobal(L, "mapdraw");
    lua_pushcfunction(L, _lua_srand);
    lua_setglobal(L, "srand");
    lua_pushcfunction(L, _lua_rnd);
    lua_setglobal(L, "rnd");
    lua_pushcfunction(L, _lua_pset);
    lua_setglobal(L, "pset");
    lua_pushcfunction(L, _lua_pget);
    lua_setglobal(L, "pget");
    lua_pushcfunction(L, _lua_fget);
    lua_setglobal(L, "fget");
    lua_pushcfunction(L, _lua_fset);
    lua_setglobal(L, "fset");
    lua_pushcfunction(L, _lua_mset);
    lua_setglobal(L, "mset");
    lua_pushcfunction(L, _lua_mget);
    lua_setglobal(L, "mget");
    // Pixel-mask collision code can call sget() hundreds of times per tick.
    // Its VM fast-call preserves the same packed-RAM semantics without a full
    // Lua-to-C transition for each sprite pixel.
    lua_pushcfastcall(L, NULL, FCF_SGET);
    lua_setglobal(L, "sget");
    lua_pushcfunction(L, _lua_sset);
    lua_setglobal(L, "sset");
    lua_pushcfunction(L, _lua_time);
    lua_setglobal(L, "t");
    lua_pushcfunction(L, _lua_time);
    lua_setglobal(L, "time");
    lua_pushcfunction(L, _lua_sfx);
    lua_setglobal(L, "sfx");
    lua_pushcfunction(L, _lua_printh);
    lua_setglobal(L, "printh");
    lua_pushcfunction(L, _lua_cartdata);
    lua_setglobal(L, "cartdata");
    lua_pushcfunction(L, _lua_dget);
    lua_setglobal(L, "dget");
    lua_pushcfunction(L, _lua_dset);
    lua_setglobal(L, "dset");
    lua_pushcfunction(L, _lua_menuitem);
    lua_setglobal(L, "menuitem");
    lua_pushcfunction(L, _lua_music);
    lua_setglobal(L, "music");
    lua_pushcfunction(L, _lua_camera);
    lua_setglobal(L, "camera");
    lua_pushcfunction(L, _lua_stat);
    lua_setglobal(L, "stat");
    lua_pushcfunction(L, _lua_clip);
    lua_setglobal(L, "clip");
    lua_pushcfunction(L, _lua_color);
    lua_setglobal(L, "color");
    lua_pushcfunction(L, _lua_poke);
    lua_setglobal(L, "poke");
    lua_pushcfunction(L, _lua_poke4);
    lua_setglobal(L, "poke4");
    lua_pushcfunction(L, _lua_peek);
    lua_setglobal(L, "peek");
    lua_pushcfunction(L, _lua_peek2);
    lua_setglobal(L, "peek2");
    lua_pushcfunction(L, _lua_sub);
    lua_setglobal(L, "sub");
    lua_pushcfunction(L, _lua_memset);
    lua_setglobal(L, "memset");
    lua_pushcfunction(L, _lua_memcpy);
    lua_setglobal(L, "memcpy");
    lua_pushcfunction(L, _lua_flip);
    lua_setglobal(L, "flip");
    lua_pushcfunction(L, _lua_fillp);
    lua_setglobal(L, "fillp");
    lua_pushcfunction(L, _lua_cstore);
    lua_setglobal(L, "cstore");
    lua_pushcfunction(L, _lua_reload);
    lua_setglobal(L, "reload");
    lua_pushcfunction(L, _lua_load);
    lua_setglobal(L, "load");
    lua_pushcfunction(L, _lua_run);
    lua_setglobal(L, "run");
    lua_pushcfunction(L, _extcmd);
    lua_setglobal(L, "extcmd");
    lua_pushcfunction(L, _lua_cursor);
    lua_setglobal(L, "cursor");

    // --- peek4 / poke2 --------------------------------------------------
    lua_pushcfunction(L, _lua_peek4);
    lua_setglobal(L, "peek4");
    lua_pushcfunction(L, _lua_poke2);
    lua_setglobal(L, "poke2");

    // --- type / tostr / tonum -------------------------------------------
    lua_pushcfunction(L, _lua_type);
    lua_setglobal(L, "type");
    lua_pushcfunction(L, _lua_tostr);
    lua_setglobal(L, "tostr");
    lua_pushcfunction(L, _lua_tonum);
    lua_setglobal(L, "tonum");

    // --- table helpers: add / del / deli / count / foreach / all --------
    lua_pushcfunction(L, _lua_add);
    lua_setglobal(L, "add");
    lua_pushcfunction(L, _lua_del);
    lua_setglobal(L, "del");
    lua_pushcfunction(L, _lua_deli);
    lua_setglobal(L, "deli");
    lua_pushcfunction(L, _lua_count);
    lua_setglobal(L, "count");
    lua_pushcfunction(L, _lua_foreach);
    lua_setglobal(L, "foreach");
    lua_pushcfunction(L, _lua_all);
    lua_setglobal(L, "all");
    lua_pushcfunction(L, _lua_inext);
    lua_setglobal(L, "inext");
    lua_pushcfunction(L, _lua_pack);
    lua_setglobal(L, "pack");

    // --- string helpers: chr / ord / split ------------------------------
    lua_pushcfunction(L, _lua_chr);
    lua_setglobal(L, "chr");
    lua_pushcfunction(L, _lua_ord);
    lua_setglobal(L, "ord");
    lua_pushcfunction(L, _lua_split);
    lua_setglobal(L, "split");

    // Register before the compatibility prelude is loaded; init_lua keeps a
    // registry reference and restores this native implementation afterward.
    lua_pushcfunction(L, _lua_flr);
    lua_setglobal(L, "flr");
}

// `lua_getglobal` pushes the function onto the stack; we check its type then
// MUST pop it before returning, otherwise repeat invocations from `flip()`'s
// per-frame dispatch accumulate one stack slot each minute — eventually
// overflows the Lua stack and surfaces as a `-1` from `lua_isfunction` or a
// pcall into non-function. The earlier revision also printed "Function %s
// does not exist" at 90Hz whenever a cart omitted `_update60`, which we want
// silent.
bool _lua_fn_exists(const char* fn) {
    lua_getglobal(L, fn);
    bool exists = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return exists;
}

static int cart_traceback_handler(lua_State *state) {
    const char *message = lua_tostring(state, 1);
    if (message == NULL) message = "(non-string Lua error)";

    // Preserve a compact view of the failing Lua frames before pcall unwinds
    // them. This is error-only work: it has no cost during successful cart
    // execution, and avoids cart-specific probes when compact decoders lose
    // a zero-indexed field or generated global.
    for (int level = 1; level <= 5; ++level) {
        lua_Debug ar;
        if (!lua_getstack(state, level, &ar)) break;
        if (!lua_getinfo(state, "nSlf", &ar)) break; // pushes frame function
        const int function_index = lua_gettop(state);
        printf("Lua frame %d: %s:%d %s\n", level,
               ar.short_src[0] ? ar.short_src : "?", ar.currentline,
               ar.name ? ar.name : "?");

        for (int local = 1; local <= 12; ++local) {
            const char *name = lua_getlocal(state, &ar, local);
            if (!name) break;
            const int type = lua_type(state, -1);
            printf("  local %s=%s", name, lua_typename(state, type));
            if (type == LUA_TNUMBER)
                printf("(%g)", (double)lua_tonumber(state, -1));
            else if (type == LUA_TSTRING)
                printf("(%s)", lua_tostring(state, -1));
            else if (type == LUA_TTABLE) {
                printf("[#%u", (unsigned)lua_rawlen(state, -1));
                for (int key = 0; key <= 4; ++key) {
                    lua_rawgeti(state, -1, key);
                    if (lua_isnumber(state, -1))
                        printf(" %d=%g", key,
                               (double)lua_tonumber(state, -1));
                    else if (lua_isnil(state, -1))
                        printf(" %d=nil", key);
                    else if (lua_istable(state, -1)) {
                        printf(" %d={", key);
                        lua_getfield(state, -1, "c");
                        if (lua_isstring(state, -1))
                            printf("c=%s", lua_tostring(state, -1));
                        else
                            printf("c=%s", luaL_typename(state, -1));
                        lua_pop(state, 1);
                        printf("}");
                    }
                    lua_pop(state, 1);
                }
                printf("]");
            }
            printf("\n");
            lua_pop(state, 1);
        }

        for (int upvalue = 1; upvalue <= 8; ++upvalue) {
            const char *name = lua_getupvalue(state, function_index, upvalue);
            if (!name) break;
            const int type = lua_type(state, -1);
            printf("  upvalue %s=%s", name, lua_typename(state, type));
            if (type == LUA_TNUMBER)
                printf("(%g)", (double)lua_tonumber(state, -1));
            else if (type == LUA_TTABLE) {
                printf("[#%u", (unsigned)lua_rawlen(state, -1));
                for (int key = 0; key <= 4; ++key) {
                    lua_rawgeti(state, -1, key);
                    if (lua_isnumber(state, -1))
                        printf(" %d=%g", key,
                               (double)lua_tonumber(state, -1));
                    else if (lua_isnil(state, -1))
                        printf(" %d=nil", key);
                    lua_pop(state, 1);
                }
                printf("]");
            }
            printf("\n");
            lua_pop(state, 1);
        }
        lua_pop(state, 1); // frame function from lua_getinfo(..., "f", ...)
    }

    luaL_traceback(state, state, message, 1);
    return 1;
}

// lua_getglobal pushes the function. lua_pcall(L, 0, 1, 0) consumes it and
// replaces the top of stack with either the return value (1 value, on success)
// or the error message (1 string, on LUA_ERRRUN). Net delta vs. before
// lua_getglobal is zero — we pop 1 to discard the result-or-error. The
// earlier `lua_pop(L, lua_gettop(L))` was catastrophic: it popped the
// ENTIRE remaining Lua stack on every frame, silently corrupting the VM.
uint8_t _to_lua_call(const char* fn) {
    if (!engine_draw_frame &&
        fn[0] == '_' && fn[1] == 'd' && fn[2] == 'r' &&
        fn[3] == 'a' && fn[4] == 'w' && fn[5] == 0)
        return 1;

    lua_getglobal(L, fn);
    if (!lua_isfunction(L, -1)) {
        // No such function registered (e.g. cart omits _update60 and _update);
        // pop silently rather than thrashing through pcall with a non-callable.
        lua_pop(L, 1);
        return 1;
    }

    // Defensive: refuse to invoke the cart callback if the Lua VM stack is
    // approaching the depth that drove valdi.p8 into a FreeRTOS main-task
    // C-stack collapse. The previous guard sat at top>5000, but valdi.p8
    // measured top=4584 _before_ its C stack collapsed — i.e. the guard
    // never fired (5000 > 4584) and we let one too many recursive C frame
    // push the FreeRTOS `main` task stack past 0, ending in
    // `PC=0x00000000 InstrFetchProhibited`. 3500 sits 1000+ Lua-stack-units
    // below the known crash point, leaving the engine room to unwind the
    // cart's recursion naturally on the next frame. C-task-stack overflow
    // is best fixed long-term by increasing the Retro-Go `main` task stack
    // in `pico8/main/main.c`; this guard exists to short-circuit the
    // cart-side runaway before that happens. We log once per ~250 stack-
    // unit climb to avoid serial flood.
    static int last_logged_top = 0;
    int top = lua_gettop(L) - 1; // -1: we just pushed the function
    if (top > 3500) {
        if (top - last_logged_top > 250) {
            printf("Lua stack near-full (top=%d), skipping %s to avoid recursion crash\n",
                   top, fn);
            last_logged_top = top;
        }
        lua_pop(L, 1);
        return 1;
    }

    lua_pushcfunction(L, cart_traceback_handler);
    lua_insert(L, -2);
    int error_handler = lua_gettop(L) - 1;
    if (lua_pcall(L, 0, 1, error_handler) == LUA_OK) {
        lua_pop(L, 2);
        return 0;
    } else {
        // Only the error message is on top; pcall removed the function.
        // `lua_tostring` returns NULL if the top value isn't a string —
        // passing NULL to printf("%s") segfaults. Guard with lua_isstring
        // so errored cfunction errors that don't string-convert (e.g.
        // numeric __tostring failing) still produce a usable log line.
        const char* err = lua_isstring(L, -1)
                              ? lua_tostring(L, -1)
                              : lua_typename(L, lua_type(L, -1));
        printf("Lua error in %s: %s\n", fn, err);
        cart_runtime_failed = true;
        // One-shot: on the first 'for' limit error, dump the Lua stack
        // and traceback so we can see what read_byte() actually returned.
        static bool dumped_traceback = false;
        if (!dumped_traceback && strstr(err, "limit must be a number")) {
            dumped_traceback = true;
            // Keep host diagnostics independent of the cart-visible global
            // environment. PICO-8 has no desktop Lua `debug` table, but the
            // auxiliary C API can still construct the same traceback.
            luaL_traceback(L, L, err, 1);
            printf("TRACEBACK: %s\n",
                   lua_tostring(L, -1) ? lua_tostring(L, -1) : "(null)");
            lua_pop(L, 1);

            // Dump read_byte's upvalues AND call peek(0x2000+2) directly
            // to see what peek returns vs what read_byte returns.
            lua_getglobal(L, "read_byte");
            if (lua_isfunction(L, -1)) {
                if (lua_iscfunction(L, -1)) {
                    printf("read_byte: is a C function\n");
                } else {
                    int fidx = lua_gettop(L);
                    int nups = 0;
                    for (int ui = 1; ; ui++) {
                        const char* name = lua_getupvalue(L, fidx, ui);
                        if (!name) break;
                        nups++;
                        int ut = lua_type(L, -1);
                        printf("  upvalue[%d] '%s': type=%d", ui, name, ut);
                        if (ut == LUA_TNUMBER) printf(" val=%f", (double)lua_tonumber(L, -1));
                        if (ut == LUA_TNIL) printf(" (nil)");
                        printf("\n");                        // If this is _ENV, dump byte_offset and peek from it
                        if (ut == LUA_TTABLE && strcmp(name, "_ENV") == 0) {
                            // Get _ENV["byte_offset"]
                            lua_getfield(L, -1, "byte_offset");
                            int ebt = lua_type(L, -1);
                            printf("    _ENV['byte_offset']: type=%d", ebt);
                            if (ebt == LUA_TNUMBER) printf(" val=%f", (double)lua_tonumber(L, -1));
                            if (ebt == LUA_TNIL) printf(" (nil)");
                            printf("\n");
                            lua_pop(L, 1);
                            // Get _ENV["peek"] and call it if it's a function
                            lua_getfield(L, -1, "peek");
                            int pkt = lua_type(L, -1);
                            printf("    _ENV['peek']: type=%d", pkt);
                            if (pkt == LUA_TNIL) printf(" (nil!)");
                            printf("\n");
                            if (pkt == LUA_TFUNCTION) {
                                // Call peek(0x2002) directly
                                lua_pushinteger(L, 0x2002);
                                if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                                    int rt = lua_type(L, -1);
                                    printf("    peek(0x2002): type=%d", rt);
                                    if (rt == LUA_TNUMBER) printf(" val=%f", (double)lua_tonumber(L, -1));
                                    printf("\n");
                                } else {
                                    printf("    peek(0x2002) ERROR: %s\n", lua_tostring(L, -1) ? lua_tostring(L, -1) : "(null)");
                                }
                            }
                            // Pop either the peek value (nil) or the pcall result
                            lua_pop(L, 1);
                        }
                        lua_pop(L, 1);
                    }
                    printf("read_byte: Lua closure, nups=%d\n", nups);
                }
                // Now call read_byte() itself
                if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
                    int t = lua_type(L, -1);
                    printf("read_byte() returned: type=%d", t);
                    if (t == LUA_TNUMBER) printf(" val=%f", (double)lua_tonumber(L, -1));
                    if (t == LUA_TSTRING) printf(" str=[%s]", lua_tostring(L, -1));
                    printf("\n");
                } else {
                    printf("read_byte() ERROR: %s\n", lua_tostring(L, -1) ? lua_tostring(L, -1) : "(null)");
                }
                lua_pop(L, 1);
            } else { lua_pop(L, 1); }

            // Dump byte_offset global
            lua_getglobal(L, "byte_offset");
            printf("byte_offset: type=%d", lua_type(L, -1));
            if (lua_isnumber(L, -1)) printf(" val=%f", (double)lua_tonumber(L, -1));
            printf("\n");
            lua_pop(L, 1);

            // Dump num_trk global
            lua_getglobal(L, "num_trk");
            printf("num_trk: type=%d", lua_type(L, -1));
            if (lua_isnumber(L, -1)) printf(" val=%f", (double)lua_tonumber(L, -1));
            printf("\n");
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
        return 1;
    }
}

// PICO-8 frame order, mapped onto retro-go's per-tick `flip()` call:
//   1. handle_input — fills `buttons[7]` and `buttons_frame[7]` so the cart's
//      `_update` sees fresh `btn()`/`btnp()` state.
//   2. _init ONCE on the first frame (PICO-8 startup callback).
//   3. _update60 per frame if defined, else _update (PICO-8 expects 60Hz but
//      falls back to 30Hz when the cart omits _update60 — Celeste Classic is
//      30Hz).
//   4. _draw per frame if defined.
//   5. gfx_flip — submit the nibble-packed frontbuffer to the display.
// Pacing is owned by the Retro-Go adapter after it records busy time.
//
// Before this dispatch existed, init_lua would pc all the cart chunk (which
// only DEFINES `_init`/`_update`/`_draw` as Lua globals without calling them),
// and `flip()` would render an empty frontbuffer, hence the user's reported
// `R:0+0` while FPS=33 indicated the outer main loop was running but no Lua
// callback was ever invoked.
void flip(bool draw_frame) {
    engine_draw_frame = draw_frame;
    wants_to_quit = handle_input();

    // If `load_cart_from_rompath` failed (parse error e.g. .p8.png before
    // steganographic decode was wired up, or init_lua failure), the
    // engine's lua_State `L` is still NULL. Every Lua-touching path below
    // (lua_gc, _init, _update, _draw, _lua_fn_exists) dereferences `L`
    // and tripped a `Guru Meditation ... LoadProhibited at 0x0000000c`
    // panic on the user's valdi.p8-followup retest. Bail out cleanly:
    // handle_input already ran (so MENU still works for the user to exit),
    // gfx_flip submits the blank frontbuffer the engine reset to during
    // engine_init(), and the frame loop returns without touching any Lua
    // API. The parse-failure log line was emitted by
    // load_cart_from_rompath / p8_parse_text / p8_png_extract_text
    // before we got here, so a serial console traces ".. parse failed"
    // → blank screen → MENU → exit without a panic.
    if (L == NULL) {
        if (engine_draw_frame) gfx_flip();
        return;
    }

    // Trigger a single GC step every 64 frames and a full collect every
    // ~34 seconds (~1024 frames at 30 fps). carts that accumulate tables
    // without ever calling collectgarbage() — valdi.p8 in particular —
    // borrow a few hundred bytes per frame and would otherwise creep until
    // Lua tripped an OOM at the next allocation, surfacing as silent heap
    // exhaustion. The bounded step keeps the per-frame cost small; the
    // periodic full collect catches long-lived garbage that won't release
    // through stepping alone.
    static int frame_count = 0;
    int fc = frame_count++;
    if ((fc & 63) == 0) {
        lua_gc(L, LUA_GCSTEP, 4);
    } else if ((fc & 1023) == 0) {
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
    // _init_done lives at module scope so engine_init() can reset it
    // (see the file-scope declaration near the top of this TU).
    if (!_init_done) {
        _init_done = true;
        if (_lua_fn_exists("_init")) {
            engine_inside_init = true;
            (void)_to_lua_call("_init");
            engine_inside_init = false;
        }
    }

    // PICO-8 stops a cart at its first unhandled runtime error. Keep input
    // and the last submitted frame alive so Retro-Go's menu remains usable,
    // but do not call partially initialised callbacks every tick and flood
    // serial output with secondary failures.
    if (cart_runtime_failed) {
        gfx_flip();
        return;
    }

    const bool update60 = _lua_fn_exists("_update60");
    update_button_repeat(engine_frame_rate() == 60);

    // Some valid carts perform menu/control work from _draw() rather than
    // _update() (PICO-BALL's launcher does this). If frameskip suppresses the
    // exact frame containing a btnp() edge, handle_input() clears that edge
    // on the next tick and the press is lost. Force only input-edge frames to
    // render/present; frames between presses retain the requested frameskip,
    // so this has no steady-state cost and held gameplay input remains on the
    // normal simulation path. Generated btnp repeats are included too.
    if (!engine_draw_frame) {
        for (int i = 0; i < 7; ++i) {
            if (buttons_frame[i]) {
                engine_draw_frame = true;
                break;
            }
        }
    }

    if (update60) {
        (void)_to_lua_call("_update60");
    } else if (_lua_fn_exists("_update")) {
        (void)_to_lua_call("_update");
    }
    if (cart_runtime_failed) {
        gfx_flip();
        return;
    }

    if (_lua_fn_exists("_draw")) {
        if (engine_draw_frame) {
            (void)_to_lua_call("_draw");
        }
    }

    if (engine_draw_frame) {
        gfx_flip();
    }

}

int engine_frame_rate(void) {
    if (engine_requested_frame_rate == 30 || engine_requested_frame_rate == 60)
        return engine_requested_frame_rate;
    return (L != NULL && _lua_fn_exists("_update60")) ? 60 : 30;
}

void engine_collect_garbage(void) {
    if (L) lua_gc(L, LUA_GCCOLLECT, 0);
}

const char *engine_menuitem_label(int slot) {
    if (slot < 0 || slot >= 5 || cart_menu_items[slot].label[0] == '\0')
        return NULL;
    return cart_menu_items[slot].label;
}

bool engine_menuitem_invoke(int slot, uint8_t buttons) {
    if (!L || slot < 0 || slot >= 5) return false;
    CartMenuItem *item = &cart_menu_items[slot];
    if (item->label[0] == '\0' || item->callback_ref < 0) return false;

    // Bits 8..10 of MENUITEM's index disable L, R, and X respectively.
    // The callback button values are the normal PICO-8 bitfield values
    // (1, 2 and 32), so X needs mapping to mask bit 10 rather than a blind
    // left shift into bit 13.
    uint16_t disable_mask = 0;
    if (buttons & 1u) disable_mask |= 0x100u;
    if (buttons & 2u) disable_mask |= 0x200u;
    if (buttons & 32u) disable_mask |= 0x400u;
    if (item->input_mask & disable_mask) return true;

    active_cart_menu_item = slot;
    lua_rawgeti(L, LUA_REGISTRYINDEX, item->callback_ref);
    lua_pushinteger(L, buttons);
    int status = lua_pcall(L, 1, 1, 0);
    active_cart_menu_item = -1;
    if (status != LUA_OK) {
        RG_LOGE("pico8: menuitem %d callback failed: %s", slot + 1,
                lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    bool keep_open = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return keep_open;
}
#endif

