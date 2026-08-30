#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "data.h"          // Spritesheet
// `lua/fix32.h` is intentionally NOT included here — it pulls in C++-only
// headers (`<cmath>`, `<algorithm>`, `<type_traits>`). engine.h is consumed
// by C TUs (retro_go_backend.c) that would fail to parse those. The engine
// TU (engine.cpp) gets z8::fix32 transitively via the text-included
// sfx.c / synth.c / pico8api.c, all of which include lua/fix32.h directly.

#define BTN_IDX_LEFT 	0
#define BTN_IDX_RIGHT 	1
#define BTN_IDX_UP 		2
#define BTN_IDX_DOWN 	3
#define BTN_IDX_A 		4
#define BTN_IDX_B 		5
#define BTN_IDX_SEL 	255
#define BTN_IDX_START 	255

#ifdef __cplusplus
extern "C" {
#endif

void render(Spritesheet* s, uint16_t n, uint16_t x0, uint16_t y0, int paletteIdx, bool flip_x, bool flip_y);
void render_stretched(Spritesheet* s, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh, bool flip_x, bool flip_y);
// `render_many` is `inline`-defined inside pico8api.c (text-included into
// engine.cpp) and ONLY used by `spr()` in the same TU. It uses `z8::fix32`
// for its width/height params, so the prototype lives next to the .c
// definition rather than here where C consumers would choke on the missing
// `z8` namespace.
// _fast_render was declared inline-orphan in the upstream tree; the engine TU
// has no definition reachable here, so leaving it would produce an undefined-
// reference linker error when any caller uses it. Dropped until a real
// definition is reintroduced.
void reset_transparency();
void flip(bool draw_frame);
int engine_frame_rate(void);
uint8_t engine_screen_mode(void);
void engine_input_reset(void);
const char *engine_menuitem_label(int slot);
bool engine_menuitem_invoke(int slot, uint8_t buttons);

// Cross-TU bridge prototypes. main.c in pico8/main declares these as plain
// `extern void …(void);` (C linkage), so they MUST be inside the
// `extern "C"` block here. Otherwise engine.cpp's now-C++ definitions get
// mangled (`_Z11engine_initv`) and the linker never matches main.c's lookup.
void engine_init(void);
void engine_prepare_cart_load(void);
int engine_take_cart_request(char *filename, size_t filename_size,
                             char *breadcrumb, size_t breadcrumb_size,
                             char *param, size_t param_size);
void engine_set_cart_param(const char *param);
bool engine_request_restart(const char *param);
bool engine_cart_request_pending(void);
void engine_collect_garbage(void);
bool init_lua(const uint8_t *bytecode, uint16_t code_len);
void cartParser(const GameCart *cart);

// Retro-Go adapter bridge used by reload(..., filename). Resolves only a
// same-directory companion cart and returns its packed 0x0000..0x42ff ROM
// image without changing the running Lua VM or current cartridge.
bool pico8_read_cart_rom(const char *filename, uint8_t *dest,
                         size_t dest_size, size_t *rom_length);

// Commented-out reference prototypes kept for documentation; not visible
// from main.c which is C. (See backend.h for the real put_pixel/get_pixel
// scaffolding signatures.)
//static inline void put_pixel(uint8_t x, uint8_t y, const color_t p);
//uint16_t get_pixel(uint8_t x, uint8_t y);

// Backend-side tracks its own copy of this as `volatile bool suspended;` in
// retro_go_backend.c (it shadows any potential declaration here).
volatile static bool suspended;

#ifdef __cplusplus
}
#endif
#endif
