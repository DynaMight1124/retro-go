// p8_png.h
//
// Runtime support for PICO-8 ``.p8.png`` carts.
//
// Unlike ``.p8`` text carts (parsed by ``p8_text_parser.cpp``), the
// ``.p8.png`` format stores the cart's sections in two layers that
// p8_parse_text can't read directly:
//
//   1. The 128x128 visible label is a real PNG image, with the cart's
//      32 KB RAM (gfx / map / gff / sfx / music / label / code) encoded
//      in the **two least significant bits of every RGBA channel** of a
//      160x205 pixel image (32,800 pixels \xc3\x97 4 channels \xc3\x97 2 bits = 32,800 bytes =
//      0x0000..0x8000).
//
//   2. When PICO-8 saves a cart as ``.p8.png``, the ``lua`` section at
//      RAM offset 0x4300 is usually *compressed* using PICO-8's custom
//      LZ77 algorithm (not standard zlib):
//        - V1: marker ``:c:\0`` (0x3A 0x63 0x3A 0x00)
//        - V2: marker ``\0pxa`` / ``\0pxA`` (0x00 0x70 0x78 0x61|0x41)
//
// This header exposes the scaffolding for that path so the SD-loader can
// detect ``.p8.png`` input cleanly and produce a clear, actionable log
// line instead of falling through to ``p8_parse_text`` (which fails on
// PNG bytes) and dereferencing a NULL ``lua_State`` in ``flip()``.
//
// Scope of this commit (full implementation, replacing the previous
// redirect stub):
//   - PNG signature detection (``p8_png_is_png``).
//   - Real PNG decode to RGBA via the lodepng decoder already vendored
//     by the retro-go component (no new vendored deps; reaches us
//     via the ``REQUIRES retro-go`` propagation in pico8's CMakeLists).
//   - 32 KB stego-cart LSB extraction with bit-order auto-detect
//     (``:c:\\0`` V1 / ``\\0pxa`` PXA / raw Lua -- dispatches to the
//     right PICO-8 LZ77 inflater for each format and silently tries
//     both formula orderings if the Lua header doesn't match).
//   - Synthesis of a ``.p8`` text cart consumed by the existing
//     ``p8_text_parser.cpp``: ``__lua__`` / ``__gfx__`` / ``__gff__`` /
//     ``__map__`` sections in the same hex format the parser already
//     accepts.
//
// Linker-coupling notes (worth knowing because they aren't obvious):
//
//   - ``LODEPNG_NO_COMPILE_CPP`` is pinned before the include so
//     lodepng.h doesn't pull in its optional C++ wrapper
//     (``#include <vector>`` and ``#include <string>``). This TU uses
//     only the C decoder API and never references the
//     ``lodepng::State`` / ``lodepng::decode`` C++ namespace wrappers.
//
//   - ``#include <lodepng.h>`` inside ``p8_png.cpp`` is wrapped in
//     ``extern "C"`` so this C++ TU requests C-linkage symbols for
//     lodepng functions. lodepng.c is compiled as C inside
//     libretro-go.a (no mangling), so without the wrap every call would
//     fail to resolve at link time.
//
//   - ``LODEPNG_NO_COMPILE_ERROR_TEXT`` is pinned before the include so
//     that the ``lodepng_error_text`` prototype isn't even visible.
//     retro-go's component compiles lodepng.c with that flag set
//     (``-DLODEPNG_NO_COMPILE_ERROR_TEXT`` in components/retro-go/
//     CMakeLists.txt), so libretro-go.a doesn't carry the function.
//     The two printf sites log the integer code instead.
//
// All changes live inside ``pico8/``. No edits to any other component,
// sdkconfig, or top-level CMakeLists.

#ifndef P8_PNG_H
#define P8_PNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// True iff ``data`` starts with the 8-byte PNG signature
// (``\x89PNG\r\n\x1a\n``). Lightweight — does not validate IHDR/CRC.
bool p8_png_is_png(const uint8_t *data, size_t len);

// Attempt to extract a synthetic ``.p8`` text payload from a ``.p8.png``
// blob. On success, ``*out_text`` is set to a heap-allocated buffer
// (caller frees with ``free()``) containing a valid ``.p8`` text cart
// (``__lua__\\n...__gfx__\\n...__gff__\\n...__map__\\n...``) whose length
// is returned in ``*out_text_len``. On any failure (PNG decode error,
// stego-cart doesn't match either known bit-order formula, Lua
// inflate error, allocation failure) returns false after logging a
// one-line diagnostic. ``*out_text`` is always initialised to NULL on
// entry so a caller that ignores the bool result cannot read
// uninitialised memory. When requested, ``*out_rom`` receives the exact
// decoded cartridge image; ``*out_rom_len`` is the initial live-ROM region
// 0x0000..0x42ff. The caller owns both returned allocations.
bool p8_png_extract_text(const uint8_t *data, size_t len,
                         char **out_text, size_t *out_text_len,
                         uint8_t **out_rom, size_t *out_rom_len);

#ifdef __cplusplus
}
#endif

#endif // P8_PNG_H
