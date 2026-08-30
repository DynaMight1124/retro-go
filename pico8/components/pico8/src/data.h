// data.h
// ============================================================================
// Engine-wide types, macros, and engine-TU-private globals.
//
// Designed to be a *valid C99* header (no C++ default-member initializers, no
// z8::fix32, no C++ namespaces).  This lets C TUs (retro_go_backend.c,
// pico8_globals.c) and C++ TUs (engine.c — which text-includes sfx.c,
// synth.c, pico8api.c) include it without compile errors.
//
// Include order rules:
//   - Anything that needs color_t / palidx_t / SCREEN_* should include
//     pico8_globals.h, which transitively pulls in data.h AFTER placing the
//     extern decls.
//   - Direct `#include "data.h"` callers do so first; they get the extern
//     declarations at the very bottom of this file (after the C primitives).
//
// Things explicitly NOT in this header (kept in sfx.c because they rely on
// the C++ z8::fix32 type):
//   - struct Channel
// Files referencing Channel fields must #include "sfx.h".
// ============================================================================

#ifndef DATA
#define DATA

#include <assert.h>
#include <stdint.h>

// --- Screen geometry (PICO-8 is 128x128) ------------------------------------
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  128
// hud.c allocates a buffer SCREEN_WIDTH * HUD_HEIGHT * 2 bytes for the
// top-of-screen overlay. The upstream PicoPico value is 16 (=> 4 KB).
#define HUD_HEIGHT     16

// --- Pixel types ------------------------------------------------------------
typedef uint8_t  palidx_t;   // nibble-packed frontbuffer entry
typedef uint16_t color_t;    // RGB565 colour

// --- PICO-8 readable RAM (Lua/C shared, ~7 KB) ------------------------------
// Engine-TU-private storage is declared in engine.cpp (not here) because
// declaring `static` in this header would emit a private copy in every TU
// that includes data.h (pico8_globals.c, retro_go_backend.c, p8_text_parser.cpp)
// and trigger `-Wunused-variable`. Centralising the declaration in the engine
// TU keeps BSS down and silences those warnings. Backend code never touches
// `ram` directly — it goes through the Lua C API.

// --- GameCart (rom-equivalent metadata for a loaded .p8 cart) ---------------
// Backends can fill carts at runtime (e.g. when loading .p8 files from an SD
// card). Static aggregate initialisers in any "static_game_data.h"-style
// built-in table would still work — we just don't depend on one here, since
// retro-go's launcher hands us carts from the filesystem.
struct GameCart {
    uint8_t       name_len;
    const char*   name;

    uint16_t      code_len;
    const uint8_t* code;

    uint16_t      gff_len;
    const uint8_t* gff;

    uint16_t      gfx_len;
    const uint8_t* gfx;

    uint16_t      sfx_len;
    const uint8_t* sfx;

    uint16_t      music_len;
    const uint8_t* music;

    uint16_t      map_len;
    const uint8_t* map;

    uint16_t      label_len;
    const uint8_t* label;

    // Canonical PICO-8 cartridge ROM image (0x0000..0x42ff). This is
    // copied into live RAM before Lua starts so peek(), PX9 level loaders,
    // and other memory-facing APIs see the same bytes as real PICO-8.
    uint16_t      rom_len;
    const uint8_t* rom;
};
typedef struct GameCart GameCart;

// --- Audio/Timing constants -------------------------------------------------
#define SAMPLE_RATE 22050
// SAMPLES_PER_DURATION and NOTES_PER_SFX are defined in sfx.c (text-included
// into the C++ engine TU) and consumed by retro_go_backend.c (a C TU). The
// `extern "C"` wrapper below matches the C linkage the C TU uses by default;
// without it, the C++ TU's mangled symbol would not satisfy retro_go_backend.c's
// unmangled lookup at link time. Same pattern as sfx.h's `channels` and
// `fill_buffer` declarations.
#ifdef __cplusplus
extern "C" {
#endif
extern const uint8_t SAMPLES_PER_DURATION;
extern const uint8_t NOTES_PER_SFX;
#ifdef __cplusplus
}
#endif
// SAMPLES_PER_DURATION is 183; NOTES_PER_SFX is 32. They're declared as
// `extern const` here and defined in sfx.c (engine TU).

// --- Sprite / Draw state ----------------------------------------------------
struct Spritesheet {
    uint8_t sprite_data[128 * 128]; // 16KB, could be 8 with nibble packing
    uint8_t flags[256];
};
typedef struct Spritesheet Spritesheet;

// No inline member initializers (C++11 syntax). engine_init() sets the
// pen / clip defaults explicitly. All other fields zero out by being
// `static`.
struct DrawState {
    uint8_t     pen_color;
    uint8_t     bg_color;
    uint8_t     clip_x;
    uint8_t     clip_y;
    uint8_t     clip_w;
    uint8_t     clip_h;
    int16_t     camera_x;
    int16_t     camera_y;
    int16_t     line_x;
    int16_t     line_y;
    uint8_t     line_active;
    uint8_t     cursor_x;
    uint8_t     cursor_y;
    uint8_t     transparent[16];
    uint16_t    fill_pattern;
    uint8_t     fill_flags;
};
typedef struct DrawState DrawState;

// --- Note / SFX (audio primitive types) -------------------------------------
struct Note {
    uint8_t key;        // pitch / C# / etc ; 0-0x40
    uint8_t waveform;   // triangle / ..; 0-0xF
    uint8_t volume;     // 0-7
    uint8_t effect;     // 0-7
};
typedef struct Note Note;

struct SFX {
    uint8_t id;
    uint8_t duration;
    uint8_t loop_start;
    uint8_t loop_end;
    Note notes[32];
};
typedef struct SFX SFX;

struct MusicPattern {
    uint8_t flags;
    uint8_t channels[4];
};
typedef struct MusicPattern MusicPattern;

// NOTE: `struct Channel` lives in sfx.c (it uses z8::fix32 phi, a C++ type).
// Forward declaration is implicit; TUs that need Channel fields must
// #include "sfx.h" (which is C++-only).

// --- Engine-TU-private singletons -------------------------------------------
// `drawstate` is declared in engine.cpp (see comment on `ram` above for
// rationale). engine.cpp's TU is the only writer; if it is referenced from
// any other TU via data.h, that code path will fail the compile (intentional
// — backend code should not reach into drawstate directly).

// --- RGB565 helper -----------------------------------------------------------
#define to_rgb565(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

// --- Min/max helpers ---------------------------------------------------------
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// --- Cross-TU bridge for shared engine state --------------------------------
// `pico8_globals.h` is included LAST so its extern declarations are emitted
// only after `palidx_t`, `color_t`, `SCREEN_WIDTH`, and `SCREEN_HEIGHT` are
// all in scope. The `#pragma once` in pico8_globals.h short-circuits the
// reverse direction, so the include order is acyclic in practice.
//
// When a TU enters via pico8_globals.h first, the body of that header
// re-enters data.h (which is suppressed by DATA); data.h finishes; then
// pico8_globals.h emits its extern decls with all types in scope.
#include "pico8_globals.h"

#endif // DATA
