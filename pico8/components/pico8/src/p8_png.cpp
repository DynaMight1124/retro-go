// p8_png.cpp
//
// On-device ``.p8.png`` BBS cart decoder. Converts the 32 KB stego-cart
// encoded in a 160x205 RGBA PNG into the same ``.p8`` text format that
// ``p8_text_parser.cpp`` already understands, so the existing
// ``load_cart_from_rompath`` -> ``p8_parse_text`` -> ``cartParser`` ->
// ``init_lua`` chain runs unchanged for the BBS carts the user drops
// onto the SD card.
//
// Pipeline (single TU):
//   1. lodepng_decode32()  -- decodes PNG bytes to RGBA in memory.
//   2. LSB unpack          -- 2 bits/channel * 4 channels per pixel =
//                             1 cart byte per pixel. 32 KB cart = 0x8000 B.
//      Auto-detect: the two known Lexaloffle formats disagree on which
//      channel goes in the low 2 bits of each cart byte (R vs B). We
//      try the canonical (R in low bits, RGBA order) first -- it's the
//      spec documented in zepto8 / picotool / PICO-8 wiki -- and fall
//      back to the thumbyp8 alternate (A in high bits, ARGB order) if
//      the Lua header at cart[0x4300] doesn't sniff as a known
//      compression marker.
//   3. Lua inflate         -- dispatch on cart[0x4300..0x4303]:
//                                ":c:\0"  -> V1 LZ77 (this code)
//                                "\0pxa"  -> PXA MTF+bitstream (this code)
//                                else     -> raw bytes (very small carts)
//      Inflated Lua source is written into the output buffer verbatim.
//      In PXA, a zero literal terminates the source stream even when the
//      header's advertised output size has not yet been reached.
//   4. Cart-bytes -> .p8 text emitter:
//        __lua__    <inflated source text>
//        __gfx__    <16384 nibbles from cart[0x0000..0x1FFF]>
//        __gff__    <512   nibbles from cart[0x3000..0x30FF]>
//        __map__    <8192  nibbles from cart[0x2000..0x2FFF]>
//      Order and section headers match what ``p8_text_parser.cpp`` reads.
//
// We deliberately skip __music__ and __label__: those sections don't
// have any runtime effect that crashes the audio task on missing
// data. Music is only consumed when the cart explicitly calls
// music(); labels are only loaded by `_load_label` style APIs.
//
// __sfx__ IS emitted below: emitting zero-init sfx[N].duration
// entries across all 64 slots would trip Core 1's audio task with
// IntegerDivideByZero the moment Lua calls `sfx(N)` (sfx.c's
// `fill_buffer` divides by `_sfx->duration`). Real BBS carts have
// non-zero durations in many slots so emitting them verbatim from
// cart RAM at 0x3200..0x42FF keeps .p8 and .p8.png runtime-parity.
//
// All changes live inside pico8/components/pico8/. Externally we
// reuse lodepng (already vendored + compiled by the retro-go component,
// reachable here via the ``REQUIRES retro-go`` propagation in
// ``pico8/components/pico8/CMakeLists.txt``) and the existing
// heap_caps PSRAM path (p8_text_parser.cpp sets the precedent).

#include "p8_png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// <lodepng.h> resolves via the include path propagated from the
// retro-go component (see components/retro-go/CMakeLists.txt where
// ``libs/lodepng`` is added to COMPONENT_ADD_INCLUDEDIRS). We do NOT
// vendor stb_image or libpng; lodepng's built-in inflate covers DEFLATE
// and zlib which is what PNG IDAT needs.
//
// Three patches needed to keep this TU both compileable and resolvable
// at link time:
//
//  1. ``LODEPNG_NO_COMPILE_CPP``. In C++ mode, lodepng.h sets
//     ``LODEPNG_COMPILE_CPP`` and unconditionally ``#include``s
//     ``<vector>`` and ``<string>``. Those STL headers transitively
//     pull in ``<bits/iterator_concepts.h>`` (and the rest of the
//     iterator-concept machinery) which declare ``template<typename
//     _Iter>`` classes. We never use lodepng's C++ wrapper (only the C
//     decoder API below), so pin this flag out and the STL includes
//     vanish. Without this pin, lodepng.h can still compile, but
//     combined with patch 2 below it would explode with ``template
//     with C linkage`` errors from ``iter_concept.h``.
//
//  2. ``extern "C"`` wrap around ``#include <lodepng.h>``. lodepng.h
//     does NOT wrap its function declarations in ``extern "C"``.
//     lodepng.c is compiled as a C TU inside libretro-go.a (extension
//     ``.c`` in retro-go's COMPONENT_SRCDIRS), so its symbols have
//     plain C linkage -- no C++ mangling. This TU is C++ and without
//     ``extern "C"`` would emit C++-mangled link requests. The
//     mangled names never match the unmangled symbols in
//     libretro-go.a, so every call would fail to resolve. Wrapping
//     the include in ``extern "C"`` makes this TU request C linkage
//     for lodepng symbols, matching what's actually in libretro-go.a.
//
//  3. ``LODEPNG_NO_COMPILE_ERROR_TEXT``. retro-go's component compiles
//     its lodepng.c TU with ``-DLODEPNG_NO_COMPILE_ERROR_TEXT`` (see
//     components/retro-go/CMakeLists.txt::component_compile_options),
//     which strips the ``lodepng_error_text`` definition out of
//     libretro-go.a. If we leave that flag undeclared here then
//     lodepng.h will expose the prototype and we'd produce a single
//     unresolved-reference for that one symbol. Pin the same flag so
//     the prototype is hidden and the two printf sites log the integer
//     code instead of the (absent) error-text string.
//
// Patches 2 and 3 must share the include block; the linker rejects the
// file if even one lodepng function asks for a symbol that was
// compiled out retro-go-side.
//
// Pin order below is NO_CPP -> NO_ERROR_TEXT -> extern "C" -> include.
// Both ``#define``s MUST come *before* the ``extern "C" {`` because
// they suppress text inside lodepng.h that would otherwise be parsed
// under C linkage, and STL templates there are not C-linkage-safe.
#define LODEPNG_NO_COMPILE_CPP
#define LODEPNG_NO_COMPILE_ERROR_TEXT
extern "C" {
#include <lodepng.h>
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// PNG signature per RFC 2083: "\x89PNG\r\n\x1a\n".
static const uint8_t P8_PNG_SIG[8] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
};

// Upper bound for PNG width/height. PICO-8 cart PNGs are 160x205,
// label PNGs are 128x128, but we leave headroom for unusual exports.
// Anything larger than this is a hostile or corrupt file and we refuse
// to allocate before knowing the dimensions, otherwise lodepng would
// attempt to malloc RGBA buffers sized w*h*4 = OOM territory for
// an ESP32 with 320 KB internal DRAM.
#define P8_PNG_MAX_DIM  (1024)

// Cart memory layout offsets. Load these into the parser on success.
#define P8_GFX_START     0x0000
#define P8_GFX_BYTES     0x2000     // 8 KB  -> 16384 nibbles
#define P8_MAP_START     0x2000
#define P8_MAP_BYTES     0x1000     // 4 KB  -> 8192  nibbles
#define P8_GFF_START     0x3000
#define P8_GFF_BYTES     0x0100     // 256 B -> 512   nibbles
#define P8_MUSIC_START   0x3100
#define P8_MUSIC_ENTRIES 64
#define P8_SFX_START     0x3200     // sfx in RAM spans 0x3200..0x42FF (4352 B)
#define P8_SFX_ENTRIES   64         // 64 sound effects × 68 B each = 0x1100
#define P8_SFX_ENTRY_B   68         // 4-byte header + 32 notes × 2 B/note
#define P8_SFX_LINE_B    (8 + 32 * 5)  // .p8 text format: 168 hex chars per entry
#define P8_LUA_START     0x4300
#define P8_LUA_BYTES     (0x8000 - 0x4300)  // 15616 B

// V1 dictionary (Lexaloffle/zepto8 canonical, first 59 chars of
// thumbyp8's 60-char dict — theirs indexes 0..58 for b=1..0x3b, the
// 60th char is unused). The exact 59-char order matters: a single
// transposition in this alphabet would silently corrupt every V1
// cart's Lua source.
static const char V1_DICT[] =
    "\n 0123456789abcdefghijklmnopqrstuvwxyz"
    "!#%(){}[]<>+=/*:;.,~_";
_Static_assert(sizeof(V1_DICT) - 1 == 59,
               "V1 alphabet must be exactly 59 chars per Lexaloffle spec");

// ---------------------------------------------------------------------------
// PNG signature detection
// ---------------------------------------------------------------------------

bool p8_png_is_png(const uint8_t *data, size_t len)
{
    if (data == NULL || len < sizeof(P8_PNG_SIG)) return false;
    return memcmp(data, P8_PNG_SIG, sizeof(P8_PNG_SIG)) == 0;
}

// ---------------------------------------------------------------------------
// LSB extraction: two formulas; auto-detect between them
// ---------------------------------------------------------------------------
//
// Both formulas read 2 LSBs from each RGBA channel of each pixel and
// pack them into one cart byte. They differ in *channel order* and
// in *bit position within the byte*. Importantly these two formulas
// produce DIFFERENT output -- R and B bit positions get swapped --
// so picking the wrong one yields garbage ROM.
//
//   formula        chan order       bit position
//   ------------   -------------    ----------------
//   canonical      R, G, B, A       R bits 0-1, A bits 6-7
//   thumbyp8       A, R, G, B       A bits 6-7, B bits 0-1
//
// We don't know which one a given cart uses, so we extract twice
// (canonical first), sniff the Lua header at RAM[0x4300] after each,
// and pick the first one whose header bytes look like a known marker.
// If neither header matches a known marker, accept the formula whose
// Lua region begins with a convincing text prefix. Raw Lua can contain
// P8SCII/control bytes inside string literals, so checking every byte up
// to the terminating NUL incorrectly rejects valid data carts.

static void lsb_unpack_canonical(const uint8_t *rgba, int n_pixels, uint8_t *cart)
{
    // RGBA pixel order; R goes to bits 0-1 of the cart byte (low),
    // A goes to bits 6-7 (high). Reference: zepto8, picotool, and
    // Lexaloffle wiki's documented .p8.png format.
    for (int i = 0; i < n_pixels; i++) {
        const uint8_t *p = &rgba[i * 4];
        cart[i] = (uint8_t)(((p[3] & 3) << 6)  // A high
                          | ((p[2] & 3) << 4)  // B
                          | ((p[1] & 3) << 2)  // G
                          |  (p[0] & 3));     // R low
    }
}

static void lsb_unpack_thumbyp8(const uint8_t *rgba, int n_pixels, uint8_t *cart)
{
    // ARGB pixel order; A high, A R G B filled MSB-first within the
    // cart byte. Reference: thumbyp8/src/p8_p8png.c unpack_cart_bytes.
    for (int i = 0; i < n_pixels; i++) {
        const uint8_t *p = &rgba[i * 4];
        cart[i] = (uint8_t)(((p[3] & 3) << 6)  // A high
                          | ((p[0] & 3) << 4)  // R
                          | ((p[1] & 3) << 2)  // G
                          |  (p[2] & 3));     // B low
    }
}

typedef enum {
    P8_LUA_NONE = 0,   // header doesn't match any known marker
    P8_LUA_V1   = 1,   // ":c:\0"  (PICO-8 <0.2.0)
    P8_LUA_PXA  = 2,   // "\0pxa"  (PICO-8 >=0.2.0)
    P8_LUA_RAW  = 3,   // no header; raw Lua source until first NUL
} P8LuaKind;

// Sniff the first 4 bytes of cart[0x4300]. V1 / PXA have known markers.
// Anything else falls through to a raw-source prefix check in the caller.
static P8LuaKind sniff_lua_marker(const uint8_t *lua_header)
{
    if (lua_header[0] == 0x3A && lua_header[1] == 0x63 &&
        lua_header[2] == 0x3A && lua_header[3] == 0x00)
        return P8_LUA_V1;
    if (lua_header[0] == 0x00 && lua_header[1] == 0x70 &&
        lua_header[2] == 0x78 && (lua_header[3] == 0x61 || lua_header[3] == 0x41))
        return P8_LUA_PXA;
    return P8_LUA_NONE;
}

// Raw PICO-8 Lua is text at its beginning, but quoted strings may contain
// arbitrary P8SCII bytes. Validate only a 32-byte prefix: long enough that
// a wrong bit-order formula is extremely unlikely to pass accidentally,
// while remaining before embedded cart data in real raw-code exports.
static bool looks_like_raw_lua(const uint8_t *cart, int cart_len_bytes)
{
    const int start = P8_LUA_START;
    const int prefix_end = start + 32 < cart_len_bytes
                         ? start + 32 : cart_len_bytes;
    int non_whitespace = 0;
    bool plausible_start = false;

    for (int i = start; i < prefix_end; i++) {
        uint8_t b = cart[i];
        if (b == 0x00)
            return plausible_start && non_whitespace >= 2;
        if (b == '\t' || b == '\n' || b == '\r' || b == ' ')
            continue;
        if (b < 0x20 || b > 0x7e)
            return false;
        if (!plausible_start) {
            // Lua statements normally begin with an identifier/keyword,
            // a comment, label, or PICO-8's '?' print shorthand.
            plausible_start = ((b >= 'a' && b <= 'z') ||
                               (b >= 'A' && b <= 'Z') ||
                               b == '_' || b == '-' || b == ':' ||
                               b == '#' || b == '?');
            if (!plausible_start) return false;
        }
        non_whitespace++;
    }
    return plausible_start && non_whitespace >= 8;
}

// Try both formulas. Returns -1 on total failure, 0 or 1 for the
// formula index that worked. Writes the chosen cart to *cart_out and
// the Lua kind to *kind_out.
//
// IMPORTANT: this function evaluates BOTH formulas into scratch buffers
// before deciding. The earlier "canonical short-circuit" tried canonical
// first and returned "raw" the moment canonical's bytes happened to pass
// the old whole-source printability check -- which happens easily because V1
// compressed data and even random-looking RGB bits often fall in the
// 0x20..0x7E printable range. That meant the alternate formula (which
// often HAS the real V1/PXA marker for modern BBS carts) was never
// even examined, and the cart got loaded with garbled Lua. The
// "evaluate both" strategy below fixes this -- we run canonical and
// alternate, then promote in priority order:
//   1. clearest V1/PXA marker win (decisive regardless of printables)
//   2. clear raw-prefix win (one formula plausible, the other not)
//   3. both plausible tiebreak (canonical by default)
static int unpack_with_auto_detect(const uint8_t *rgba, int w, int h,
                                    uint8_t *cart_out, P8LuaKind *kind_out)
{
    int n_pixels = w * h;
    if (n_pixels > 0x8000) n_pixels = 0x8000;
    if (n_pixels < P8_LUA_START + 4) return -1;

    uint8_t *cart_a = (uint8_t *)malloc(0x8000);
    uint8_t *cart_b = (uint8_t *)malloc(0x8000);
    if (cart_a == NULL || cart_b == NULL) {
        free(cart_a); free(cart_b);
        return -1;
    }
    memset(cart_a, 0, 0x8000);
    memset(cart_b, 0, 0x8000);

    lsb_unpack_canonical(rgba, n_pixels, cart_a);
    P8LuaKind k_a = sniff_lua_marker(cart_a + P8_LUA_START);
    bool raw_a = looks_like_raw_lua(cart_a, n_pixels);

    lsb_unpack_thumbyp8(rgba, n_pixels, cart_b);
    P8LuaKind k_b = sniff_lua_marker(cart_b + P8_LUA_START);
    bool raw_b = looks_like_raw_lua(cart_b, n_pixels);

    int chosen_formula = -1;
    P8LuaKind chosen_kind = P8_LUA_NONE;
    uint8_t *chosen_cart = NULL;

    if (k_a == P8_LUA_V1 || k_a == P8_LUA_PXA) {
        chosen_formula = 0; chosen_kind = k_a; chosen_cart = cart_a;
    } else if (k_b == P8_LUA_V1 || k_b == P8_LUA_PXA) {
        chosen_formula = 1; chosen_kind = k_b; chosen_cart = cart_b;
    } else if (raw_a && !raw_b) {
        chosen_formula = 0; chosen_kind = P8_LUA_RAW; chosen_cart = cart_a;
    } else if (raw_b && !raw_a) {
        chosen_formula = 1; chosen_kind = P8_LUA_RAW; chosen_cart = cart_b;
    } else if (raw_a) {
        chosen_formula = 0; chosen_kind = P8_LUA_RAW; chosen_cart = cart_a;
    }

    if (chosen_formula >= 0) {
        memcpy(cart_out, chosen_cart, 0x8000);
        *kind_out = chosen_kind;
        free(cart_a); free(cart_b);
        return chosen_formula;
    }
    free(cart_a); free(cart_b);
    return -1;
}

// ---------------------------------------------------------------------------
// V1 LZ77 inflater (header ":c:\0" -- PICO-8 <0.2.0)
// ---------------------------------------------------------------------------
//
// Body layout:
//   byte 0x00         -> literal: emit next byte
//   byte 0x01..0x3b   -> dictionary index: emit V1_DICT[byte - 1]
//   byte 0x3c..0xff   -> back-reference of length 2..17 and offset
//                        1..(0xc4 * 16 + 0x0f):
//                          next byte = [offset_low_4][length_high_4]
//                                     = ((b2 & 0x0f) | ((b - 0x3c) << 4))
//                          offset = (b - 0x3c) * 16 + (b2 & 0x0f)
//                          length = (b2 >> 4) + 2

static int inflate_v1(const uint8_t *src, size_t src_len,
                       char *out, size_t out_max)
{
    if (src_len < 8) return -1;
    int raw_len = (src[4] << 8) | src[5];
    if (raw_len < 0 || (size_t)raw_len > out_max) return -1;
    size_t i = 8;
    int oi = 0;
    while (oi < raw_len && i < src_len) {
        unsigned char b = src[i++];
        if (b == 0x00) {
            if (i >= src_len) break;
            char c = (char)src[i++];
            if (c == 0) c = '\n';        // never let a NUL truncate C string
            out[oi++] = c;
        } else if (b <= 0x3b) {
            out[oi++] = V1_DICT[b - 1];
        } else {
            if (i >= src_len) break;
            unsigned char b2 = src[i++];
            int offset = (b - 0x3c) * 16 + (b2 & 0x0f);
            int clen   = (b2 >> 4) + 2;
            if (offset <= 0 || offset > oi) break;
            int start = oi - offset;
            for (int k = 0; k < clen && oi < raw_len; k++) {
                out[oi] = out[start + k];
                oi++;
            }
        }
    }
    return oi;
}

// ---------------------------------------------------------------------------
// PXA inflater (header "\0pxa" -- PICO-8 >=0.2.0)
// ---------------------------------------------------------------------------
//
// Bit reader is LSB-first within each byte. After the 8-byte header:
//   flag bit 1:
//     1  -> LITERAL
//       read variable-bit-width MTF index:
//         nbits=4 initially; read 1 bit; while that bit is 1, nbits++
//         (continuation chain). Then read nbits bits and add
//         (1 << nbits) - 16 to recover the dictionary index.
//       emit mtf[idx], move-to-front.
//     0  -> BACK-REFERENCE or RAW EMBED
//       offset-width: 1 bit; if 0 -> 15-bit offset; else 1 more bit,
//         if 1 -> 5-bit, else 10-bit. Read offset bits + 1.
//       if width==10 AND offset==1: it's an embedded raw byte stream
//         read 8 bits at a time, emit until first 0x00 (terminator).
//       Otherwise: length is base 3; read 3-bit chunks, add to length
//         while chunk == 7 (escape). Copy "length" bytes from
//         out[oi - offset].

typedef struct {
    const uint8_t *data;
    int pos;
    int lim;
} BitR;

static int br_read(BitR *r, int n)
{
    int v = 0;
    for (int i = 0; i < n; i++) {
        if (r->pos >= r->lim) return v;
        int bi = r->pos >> 3;
        int bo = r->pos & 7;
        v |= ((r->data[bi] >> bo) & 1) << i;
        r->pos++;
    }
    return v;
}

static int inflate_pxa(const uint8_t *src, size_t src_len,
                        char *out, size_t out_max)
{
    if (src_len < 8) return -1;
    int raw_len = (src[4] << 8) | src[5];
    if (raw_len < 0 || (size_t)raw_len > out_max) return -1;
    int comp_len = (src[6] << 8) | src[7];
    (void)comp_len;

    BitR r;
    r.data = src + 8;
    r.pos  = 0;
    r.lim  = (int)((src_len > 8 ? src_len - 8 : 0) * 8);

    uint8_t mtf[256];
    for (int i = 0; i < 256; i++) mtf[i] = (uint8_t)i;

    int oi = 0;
    int safety = 0;
    while (oi < raw_len) {
        if (++safety > raw_len * 50) break;   // runaway guard
        int flag = br_read(&r, 1);
        if (flag) {
            int nbits = 4;
            while (br_read(&r, 1)) {
                nbits++;
                if (nbits > 16) break;
            }
            int idx = br_read(&r, nbits) + (1 << nbits) - 16;
            if (idx < 0 || idx >= 256) break;
            uint8_t value = mtf[idx];
            // PXA uses a literal zero as the end-of-source marker. Treating
            // it as a newline appends data beyond the real Lua stream. This
            // does not remove newlines which legitimately precede the marker.
            if (value == 0) break;
            out[oi++] = (char)value;
            for (int k = idx; k > 0; k--) mtf[k] = mtf[k - 1];
            mtf[0] = value;
        } else {
            int off_bits;
            int s0 = br_read(&r, 1);
            if (s0 == 0) {
                off_bits = 15;
            } else {
                int s1 = br_read(&r, 1);
                off_bits = s1 ? 5 : 10;
            }
            int offset = br_read(&r, off_bits) + 1;
            if (off_bits == 10 && offset == 1) {
                // Embedded raw byte stream; read 8 bits until 0x00.
                while (oi < raw_len) {
                    int byte = br_read(&r, 8);
                    if (byte == 0) break;
                    char c = (char)byte;
                    if (c == 0) c = '\n';
                    out[oi++] = c;
                }
                continue;
            }
            int length = 3;
            while (1) {
                int chunk = br_read(&r, 3);
                length += chunk;
                if (chunk != 7) break;
            }
            if (offset > oi || offset <= 0) break;
            int start = oi - offset;
            for (int k = 0; k < length && oi < raw_len; k++) {
                out[oi] = out[start + k];
                oi++;
            }
        }
    }
    return oi;
}

// ---------------------------------------------------------------------------
// Raw (uncompressed) Lua source -> output buffer (copies until NUL or end)
// ---------------------------------------------------------------------------

static int inflate_raw(const uint8_t *src, size_t src_len,
                       char *out, size_t out_max)
{
    size_t n = 0;
    for (size_t i = 0; i < src_len && n + 1 < (size_t)out_max; i++) {
        uint8_t b = src[i];
        if (b == 0x00) break;            // NUL terminates Lua source
        if (b == 0x00) out[n++] = '\n';  // unreachable; defensive
        else            out[n++] = (char)b;
    }
    return (int)n;
}

// ---------------------------------------------------------------------------
// Cart bytes -> .p8 text emitter
// ---------------------------------------------------------------------------
//
// Sections emitted (in this order):
//   __lua__    <inflated Lua source text terminated by \n\0>
//   __gfx__    <16384 hex characters>
//   __gff__    <512   hex characters>
//   __map__    <8192  hex characters>
//
// We omit a blank-line gap between __lua__ and __gfx__ so that p8_text_parser
// can pick up the next header cleanly. Internally we just keep a running
// pointer into the output buffer; helpers below return the byte count
// they wrote.

static const char HEX[] = "0123456789abcdef";

static int emit_header(char *out, const char *header)
{
    int n = (int)strlen(header);
    memcpy(out, header, n);
    out[n] = '\n';
    return n + 1;
}

static int emit_hex_of_cart(char *out, const uint8_t *cart,
                            int cart_start, int n_bytes)
{
    int oi = 0;
    for (int i = 0; i < n_bytes; i++) {
        uint8_t b = cart[cart_start + i];
        out[oi++] = HEX[(b >> 4) & 0x0F];
        out[oi++] = HEX[b & 0x0F];
    }
    return oi;
}

// __gfx__ text is one pixel nibble per character, left-to-right. Packed
// cartridge RAM stores the left/even pixel in the low nibble, unlike normal
// hexadecimal byte notation used by __map__ and __gff__.
static int emit_gfx_of_cart(char *out, const uint8_t *cart)
{
    int oi = 0;
    for (int i = 0; i < P8_GFX_BYTES; ++i) {
        uint8_t b = cart[P8_GFX_START + i];
        out[oi++] = HEX[b & 0x0f];
        out[oi++] = HEX[(b >> 4) & 0x0f];
    }
    return oi;
}

static int emit_music_of_cart(char *out, const uint8_t *cart)
{
    int oi = 0;
    for (int p = 0; p < P8_MUSIC_ENTRIES; ++p) {
        const uint8_t *m = cart + P8_MUSIC_START + p * 4;
        uint8_t flags = (uint8_t)(((m[0] >> 7) & 1)
                       | (((m[1] >> 7) & 1) << 1)
                       | (((m[2] >> 7) & 1) << 2));
        out[oi++] = HEX[(flags >> 4) & 0x0f];
        out[oi++] = HEX[flags & 0x0f];
        out[oi++] = ' ';
        for (int c = 0; c < 4; ++c) {
            uint8_t id = m[c] & 0x7f;
            out[oi++] = HEX[(id >> 4) & 0x0f];
            out[oi++] = HEX[id & 0x0f];
        }
        out[oi++] = '\n';
    }
    return oi;
}

// Cart RAM stores 32 two-byte notes followed by the four editor/speed/loop
// bytes. The .p8 text representation puts those four bytes first, so this
// emitter deliberately reorders the record.
// PICO-8's wiki documents:
//
//   bits  0..5  pitch     (0..63)
//   bits  6..8  waveform  (0..7)
//   bits  9..11 volume    (0..7)
//   bits 12..14 effect    (0..7)
//
// The .p8 text format packs the same bits as 5 hex chars per note:
// 2 chars for pitch (any 8 bits, upper 2 don't matter -- PICO-8
// clamps to 0..63 anyway), then 1 char each for waveform/volume/
// effect. Per-emit invariant is enforced by SFXParser/notéParser
// in pico8/components/pico8/src/parser.c.
//
// The 64 entries × 68 RAM bytes each fills 0x3200..0x42FF exactly --
// the is_printable_lua byte-budget and the budget math further
// down both assume 64 entries. If a future cart uses fewer slots
// the trailing entries will be all-zero, which is the same state
// PICO-8 itself shows when the user hasn't filled a slot in the
// editor; that's the correct behaviour for an uninitialised slot
// (Lua's `sfx(N)` against an empty slot would still divide-by-zero,
// but only if the cart explicitly asked, and that's a cart bug not
// a decode bug).
static int emit_sfx_of_cart(char *out, const uint8_t *cart, int cart_start)
{
    int oi = 0;
    for (int i = 0; i < P8_SFX_ENTRIES; i++) {
        int base = cart_start + i * P8_SFX_ENTRY_B;
        // Trailing 4-byte RAM header -> leading 8 text chars. SFXParser reads:
        //   chars 0..1 = editor / spare
        //   chars 2..3 = duration (THIS MUST GO NON-ZERO OR THE
        //                  AUDIO TASK DIVIDES BY ZERO; non-zero is
        //                  guaranteed by any non-empty slot).
        //   chars 4..5 = loop_start
        //   chars 6..7 = loop_end
        for (int j = 0; j < 4; j++) {
            uint8_t b = cart[base + 64 + j];
            out[oi++] = HEX[(b >> 4) & 0x0F];
            out[oi++] = HEX[b & 0x0F];
        }
        // 32 notes × 2 RAM bytes/note -> 5 hex chars/note.
        for (int n = 0; n < 32; n++) {
            uint8_t lo = cart[base + n * 2 + 0];
            uint8_t hi = cart[base + n * 2 + 1];
            uint16_t word = (uint16_t)(lo | ((uint16_t)hi << 8));
            uint8_t pitch    = (uint8_t)(word & 0x3F);
            // Bit 15 distinguishes custom SFX instruments from the eight
            // built-in waveforms. Reconstitute it as text waveform 8..15;
            // dropping it made PNG carts silently lose whole music voices.
            uint8_t waveform = (uint8_t)(((word >> 6) & 0x07)
                                       | ((word >> 12) & 0x08));
            uint8_t volume   = (uint8_t)((word >> 9) & 0x07);
            uint8_t effect   = (uint8_t)((word >> 12) & 0x07);
            out[oi++] = HEX[(pitch >> 4) & 0x0F];
            out[oi++] = HEX[pitch & 0x0F];
            out[oi++] = HEX[waveform & 0x0F];
            out[oi++] = HEX[volume & 0x0F];
            out[oi++] = HEX[effect & 0x0F];
        }
        out[oi++] = '\n';
    }
    return oi;
}

// ---------------------------------------------------------------------------
// Top-level dispatch
// ---------------------------------------------------------------------------

bool p8_png_extract_text(const uint8_t *data, size_t len,
                         char **out_text, size_t *out_text_len,
                         uint8_t **out_rom, size_t *out_rom_len)
{
    // Always initialise outputs so a caller that ignores the bool
    // return cannot dereference uninitialised pointers on the false
    // path. Same posture as p8_text_parser.
    if (out_text)     *out_text     = NULL;
    if (out_text_len) *out_text_len = 0;
    if (out_rom)      *out_rom      = NULL;
    if (out_rom_len)  *out_rom_len  = 0;

    if (!p8_png_is_png(data, len)) return false;

    // 0. Bounds check via lodepng_inspect BEFORE the decode allocates
    //    an RGBA pixel buffer. PICO-8 cart PNGs are 160x205 = 32800
    //    pixels; refuse anything pathological enough to OOM an ESP32
    //    if dropped by an attacker or corrupted on disk. inspect_by
    //    design touches only the IHDR chunk -- no IDAT inflate -- so
    //    it's the right cheap gate for a hostile PNG.
    LodePNGState inspect_state;
    lodepng_state_init(&inspect_state);
    unsigned w = 0, h = 0;
    unsigned inspect_err = lodepng_inspect(&w, &h, &inspect_state, data, len);
    lodepng_state_cleanup(&inspect_state);
    if (inspect_err != 0) {
        printf("pico8: .p8.png lodepng_inspect failed (code %u)\n",
               inspect_err);
        return false;
    }
    if (w == 0 || h == 0 || w > P8_PNG_MAX_DIM || h > P8_PNG_MAX_DIM) {
        printf("pico8: .p8.png dimensions %ux%u out of bounds (max %u)\n",
               w, h, P8_PNG_MAX_DIM);
        return false;
    }
    if (w * h < P8_LUA_START + 4) {
        printf("pico8: .p8.png is %ux%u -- too small for a PICO-8 cart\n",
               w, h);
        return false;
    }

    // 1. Decode PNG into RGBA via the lodepng decoder that retro-go
    //    already builds. Returns malloc'd buffer of w*h*4 bytes.
    unsigned char *rgba = NULL;
    unsigned err = lodepng_decode32(&rgba, &w, &h, data, (size_t)len);
    if (err != 0 || rgba == NULL) {
        printf("pico8: .p8.png lodepng decode failed (code %u)\n",
               err);
        if (rgba) free(rgba);
        return false;
    }

    // 2. LSB extract the 32 KB stego-cart, auto-detecting whether the
    //    cart uses the canonical (R-low, RGBA) formula or the
    //    thumbyp8 alternate (A-high, ARGB).
    uint8_t *cart = (uint8_t *)calloc(1, 0x8000);
    if (!cart) {
        free(rgba);
        printf("pico8: .p8.png out of memory (cart buffer)\n");
        return false;
    }
    P8LuaKind kind = P8_LUA_NONE;
    int formula = unpack_with_auto_detect(rgba, (int)w, (int)h, cart, &kind);
    if (formula < 0) {
        printf("pico8: .p8.png neither bit-order formula produced a\n"
               "       recognisable Lua header at RAM[0x4300].\n"
               "       The PNG might be an image (not a cart) or use a\n"
               "       PICO-8 export format this build doesn't handle.\n");
        free(rgba);
        free(cart);
        return false;
    }
    // Diagnostic: dump first 8 bytes of cart[0x4300..] hex, under BOTH
    // formulas. Lets us eyeball the chosen byte layout and compare
    // against the un-picked formula -- critical when the auto-detect
    // settles on "raw" but the cart is actually V1/PXA-compressed.
    // Real V1 marker   =  3a 63 3a 00  (" :c:\0")
    // Real PXA marker  =  00 70 78 61  (" \0pxa")
    // Sensible Lua start = "--", "function", "local", "if", "for", ...
    // Anything else here usually means the bit-order is wrong.
    //
    // The "other formula" malloc is 32 KB transient -- well within the
    // 320 KB internal-DRAM budget on Odroid-Go. Safe to leave in for
    // debug builds; can be stripped later by gating on a P8_PNG_DEBUG
    // macro if logging volume becomes a concern.
    printf("pico8: .p8.png lua header (chosen=%s): %02x %02x %02x %02x %02x %02x %02x %02x\n",
           formula == 0 ? "canonical" : "thumbyp8-alternate",
           cart[P8_LUA_START+0], cart[P8_LUA_START+1],
           cart[P8_LUA_START+2], cart[P8_LUA_START+3],
           cart[P8_LUA_START+4], cart[P8_LUA_START+5],
           cart[P8_LUA_START+6], cart[P8_LUA_START+7]);
    {
        uint8_t *other_cart = (uint8_t *)malloc(0x8000);
        if (other_cart != NULL) {
            memset(other_cart, 0, 0x8000);
            int npix = (int)w * (int)h;
            if (npix > 0x8000) npix = 0x8000;
            if (formula == 0)
                lsb_unpack_thumbyp8(rgba, npix, other_cart);
            else
                lsb_unpack_canonical(rgba, npix, other_cart);
            printf("pico8: .p8.png lua header (other =%s): %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   formula == 0 ? "thumbyp8-alternate" : "canonical",
                   other_cart[P8_LUA_START+0], other_cart[P8_LUA_START+1],
                   other_cart[P8_LUA_START+2], other_cart[P8_LUA_START+3],
                   other_cart[P8_LUA_START+4], other_cart[P8_LUA_START+5],
                   other_cart[P8_LUA_START+6], other_cart[P8_LUA_START+7]);
            free(other_cart);
        }
    }
    printf("pico8: .p8.png Lua format %s (%s bit-order)\n",
           kind == P8_LUA_V1 ? "V1"
           : kind == P8_LUA_PXA ? "PXA"
           : "raw",
           formula == 0 ? "canonical" : "thumbyp8-alternate");

    // Nothing beyond this point needs the decoded 160x205 RGBA image. Free
    // its ~128 KB allocation before creating the Lua and synthesized text
    // buffers rather than retaining all three until the end. Winterwood's
    // large PXA source exposed the old peak-allocation cleanup as a TLSF
    // assertion in free(rgba); shortening this lifetime also materially
    // reduces transient PSRAM pressure for every PNG cart.
    free(rgba);
    rgba = NULL;

    // 3. Inflate Lua region from cart[0x4300..0x8000]. Output into a
    //    generous buffer (Lua can be up to 64 KB on a maxed-out cart,
    //    and compressed carts at 0x4300 can have raw lengths larger
    //    than the compressed input).
    char *lua_buf = (char *)malloc(0x10000);
    if (!lua_buf) {
        free(rgba);
        free(cart);
        printf("pico8: .p8.png out of memory (Lua buffer)\n");
        return false;
    }
    int lua_len = -1;
    const uint8_t *lua_src = cart + P8_LUA_START;
    size_t lua_src_max = P8_LUA_BYTES;
    switch (kind) {
        case P8_LUA_V1:
            lua_len = inflate_v1(lua_src, lua_src_max, lua_buf, 0x10000 - 1);
            break;
        case P8_LUA_PXA:
            lua_len = inflate_pxa(lua_src, lua_src_max, lua_buf, 0x10000 - 1);
            break;
        case P8_LUA_RAW:
            lua_len = inflate_raw(lua_src, lua_src_max, lua_buf, 0x10000 - 1);
            break;
        default:
            break;
    }
    if (lua_len < 0) {
        printf("pico8: .p8.png Lua inflate failed for %s\n",
               kind == P8_LUA_V1 ? "V1"
               : kind == P8_LUA_PXA ? "PXA" : "raw");
        free(lua_buf);
        free(rgba);
        free(cart);
        return false;
    }
    lua_buf[lua_len] = '\0';

    // Diagnostic: dump first 32 chars of the inflated Lua source with
    // ``\xNN`` escapes for non-printable bytes. If the auto-detect
    // picked the wrong bit-order formula, these bytes will look like
    // random ASCII -- confirming the bug. If bit-order is right but
    // the inflate path itself is broken, you'll see partial Lua then
    // a hole. Cheap; helpful in every rebuild cycle until BBS carts
    // are known to work.
    printf("pico8: .p8.png inflated Lua [0..32): \"");
    for (int _di = 0; _di < 32 && _di < lua_len; _di++) {
        unsigned char _c = (unsigned char)lua_buf[_di];
        if (_c >= 0x20 && _c <= 0x7E) printf("%c", _c);
        else printf("\\x%02x", _c);
    }
    printf("\" (total %d bytes)\n", lua_len);

    // 4. Synthesize a .p8 text buffer in the format p8_text_parser
    //    reads: __lua__\n<source>\n__gfx__\n<hex> ... The parser
    //    does not strictly require blank-line separators between
    //    sections, but we leave single \n gaps for readability.
    size_t total = 0;
    total += strlen("__lua__") + 1;                 // header + \n
    total += (size_t)lua_len + 2;                   // source + \n + \n
    total += strlen("__gfx__") + 1;
    total += P8_GFX_BYTES * 2 + 2;
    total += strlen("__gff__") + 1;
    total += P8_GFF_BYTES * 2 + 2;
    total += strlen("__map__") + 1;
    total += P8_MAP_BYTES * 2 + 1;                   // last no trailing \n\n    total += strlen("__sfx__") + 1;                  // "__sfx__\n"
    total += P8_SFX_ENTRIES * (P8_SFX_LINE_B + 1);   // 64 lines × (168 hex + '\n')
    total += 1;                                      // trailing '\n' after __sfx__
    total += strlen("__music__") + 1;
    total += P8_MUSIC_ENTRIES * 12 + 1;

    char *text = (char *)malloc(total);
    if (!text) {
        printf("pico8: .p8.png out of memory (text buffer %u B)\n",
               (unsigned)total);
        free(lua_buf);
        free(rgba);
        free(cart);
        return false;
    }

    size_t off = 0;
    off += emit_header(text + off, "__lua__");
    memcpy(text + off, lua_buf, (size_t)lua_len);
    off += (size_t)lua_len;
    text[off++] = '\n';
    text[off++] = '\n';

    off += emit_header(text + off, "__gfx__");
    off += emit_gfx_of_cart(text + off, cart);
    text[off++] = '\n';
    text[off++] = '\n';

    off += emit_header(text + off, "__gff__");
    off += emit_hex_of_cart(text + off, cart, P8_GFF_START, P8_GFF_BYTES);
    text[off++] = '\n';
    text[off++] = '\n';

    off += emit_header(text + off, "__map__");
    off += emit_hex_of_cart(text + off, cart, P8_MAP_START, P8_MAP_BYTES);
    text[off++] = '\n';

    off += emit_header(text + off, "__sfx__");
    off += emit_sfx_of_cart(text + off, cart, P8_SFX_START);
    text[off++] = '\n';

    off += emit_header(text + off, "__music__");
    off += emit_music_of_cart(text + off, cart);
    text[off++] = '\n';

    // Diagnostic: first SFX entry's duration byte (slot 0, RAM byte 1)
    // and the first three slot-0 note bytes, so a real-device boot log
    // can confirm we are feeding non-zero sfx durations into the engine.
    // Cheap (a handful of %02x prints) and only on the .p8.png decode
    // path so the .p8 text-path runtime cost is unchanged.
    {
        int base = P8_SFX_START + 0 * P8_SFX_ENTRY_B;
        printf("pico8: .p8.png sfx[0] duration=%02x notes=%02x%02x "
               "(and 31 more slots, total %u entries)\n",
               cart[base + 65],
               cart[base + 0], cart[base + 1],
               (unsigned)P8_SFX_ENTRIES);
    }

    free(lua_buf);
    free(rgba); // NULL after early image-buffer release; safe on failures too

    if (out_text)     *out_text     = text;
    if (out_text_len) *out_text_len = off;
    if (out_rom) {
        *out_rom = cart;
        if (out_rom_len) *out_rom_len = P8_LUA_START;
    } else {
        free(cart);
    }
    return true;
}
