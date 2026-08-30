#ifndef P8_TEXT_PARSER_H
#define P8_TEXT_PARSER_H

// Runtime parser for PICO-8 ``.p8`` text cart files.
//
// The desktop / firmware-baked path runs ``scripts/to_c.py`` at build
// time which mirrors the same logic; this file is the C++ equivalent
// used when loading carts from an SD card (e.g. on Lilka).
//
// We deliberately do NOT compile the Lua source to bytecode here:
// ``luaL_loadbuffer`` from z8lua accepts source directly, so we hand
// it the raw text and let the parser do the work.

#include "data.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A cart loaded at runtime. Owns all allocated buffers.
// ``cart`` is a normal ``GameCart`` whose pointer members reference the
// buffers inside this struct, so it can be passed to ``cartParser`` and
// ``init_lua`` unchanged.
typedef struct LoadedCart {
    GameCart cart;
    uint8_t* code_buf;
    uint8_t* gfx_buf;
    uint8_t* gff_buf;
    uint8_t* map_buf;
    uint8_t* sfx_buf;
    uint8_t* music_buf;
    uint8_t* label_buf;
    uint8_t* rom_buf;
    char     name_buf[32];
} LoadedCart;

// Parse ``data`` (``len`` bytes) of a ``.p8`` text cart into ``out``.
// ``display_name`` is a NUL-terminated user-visible name (truncated to
// fit ``name_buf``). Returns ``true`` on success. On failure, any
// partial allocations are released so it is safe to discard ``out``.
bool p8_parse_text(const char* data, size_t len,
                   const char* display_name, LoadedCart* out);

// Parse a text cart used only as a reload() data bank. Unlike
// p8_parse_text(), this accepts carts without an __lua__ section while still
// reconstructing their packed 0x0000..0x42ff ROM image. Executable cart loads
// must continue to use p8_parse_text() so a data-only cart cannot be launched.
bool p8_parse_text_data(const char* data, size_t len,
                        const char* display_name, LoadedCart* out);

// Release every buffer owned by ``cart`` and zero its pointers.
void p8_cart_free(LoadedCart* cart);

// Replace the reconstructed text-cart ROM with an exact decoded image (used
// by .p8.png). Takes ownership of `rom` on success.
bool p8_cart_take_rom(LoadedCart* cart, uint8_t* rom, size_t rom_len);

#ifdef __cplusplus
}
#endif

#endif
