#ifndef P8_TEXT_PARSER_C
#define P8_TEXT_PARSER_C

#include "p8_text_parser.h"

#include <stdlib.h>
#include <string.h>

#define P8_ROM_SIZE     0x4300
#define P8_GFX_SIZE     0x2000
#define P8_MAP_ADDR     0x2000
#define P8_MAP_SIZE     0x1000
#define P8_GFF_ADDR     0x3000
#define P8_GFF_SIZE     0x0100
#define P8_MUSIC_ADDR   0x3100
#define P8_SFX_ADDR     0x3200
#define P8_SFX_SIZE     0x1100

// Optional: prefer PSRAM on ESP32-S3 (Lilka) so we don't blow DRAM.
#if defined(LILKA_BACKEND) || defined(ESP_BACKEND)
  #include <esp_heap_caps.h>
  static inline void* p8_alloc(size_t n) {
      void* p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!p) p = malloc(n);
      return p;
  }
  static inline void p8_free(void* p) {
      if (!p) return;
      // heap_caps_free handles both SPIRAM and regular heap
      heap_caps_free(p);
  }
#else
  static inline void* p8_alloc(size_t n) { return malloc(n); }
  static inline void  p8_free(void* p)   { free(p); }
#endif

// Map a single hex character to its nibble value, or 0xFF on error.
static inline uint8_t p8_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    return 0xFF;
}

// Convert a stream of hex digit chars to one nibble per byte (gfx/label).
// Non-hex chars are skipped. Returns the number of nibbles written.
static size_t p8_nibbles(const char* src, size_t src_len, uint8_t* dst) {
    size_t n = 0;
    for (size_t i = 0; i < src_len; ++i) {
        uint8_t v = p8_hex_nibble(src[i]);
        if (v == 0xFF) continue;
        dst[n++] = v;
    }
    return n;
}

// Convert a stream of hex digit chars to one byte per pair (gff/map).
// Non-hex chars are skipped. msn first, then lsn. Returns the number of
// bytes written.
static size_t p8_hex_bytes(const char* src, size_t src_len, uint8_t* dst) {
    size_t n = 0;
    uint8_t high = 0xFF;
    for (size_t i = 0; i < src_len; ++i) {
        uint8_t v = p8_hex_nibble(src[i]);
        if (v == 0xFF) continue;
        if (high == 0xFF) {
            high = v;
        } else {
            dst[n++] = (uint8_t)((high << 4) | v);
            high = 0xFF;
        }
    }
    return n;
}

// Identify which section a header line starts (returns 0 if not a header).
enum P8Section {
    P8_NONE = 0,
    P8_LUA,
    P8_GFX,
    P8_GFF,
    P8_LABEL,
    P8_MAP,
    P8_SFX,
    P8_MUSIC,
};

static int p8_match_header(const char* line, size_t len) {
    // Trim trailing CR
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        --len;
    }
    if (len == 7  && memcmp(line, "__lua__",   7)  == 0) return P8_LUA;
    if (len == 7  && memcmp(line, "__gfx__",   7)  == 0) return P8_GFX;
    if (len == 7  && memcmp(line, "__gff__",   7)  == 0) return P8_GFF;
    if (len == 7  && memcmp(line, "__map__",   7)  == 0) return P8_MAP;
    if (len == 7  && memcmp(line, "__sfx__",   7)  == 0) return P8_SFX;
    if (len == 9  && memcmp(line, "__label__", 9)  == 0) return P8_LABEL;
    if (len == 9  && memcmp(line, "__music__", 9)  == 0) return P8_MUSIC;
    return P8_NONE;
}

void p8_cart_free(LoadedCart* cart) {
    if (!cart) return;
    p8_free(cart->code_buf);  cart->code_buf  = NULL;
    p8_free(cart->gfx_buf);   cart->gfx_buf   = NULL;
    p8_free(cart->gff_buf);   cart->gff_buf   = NULL;
    p8_free(cart->map_buf);   cart->map_buf   = NULL;
    p8_free(cart->sfx_buf);   cart->sfx_buf   = NULL;
    p8_free(cart->music_buf); cart->music_buf = NULL;
    p8_free(cart->label_buf); cart->label_buf = NULL;
    p8_free(cart->rom_buf);   cart->rom_buf   = NULL;
    memset(&cart->cart, 0, sizeof(cart->cart));
}

bool p8_cart_take_rom(LoadedCart* cart, uint8_t* rom, size_t rom_len) {
    if (!cart || !rom || rom_len < P8_ROM_SIZE) return false;
    p8_free(cart->rom_buf);
    cart->rom_buf = rom;
    cart->cart.rom = rom;
    cart->cart.rom_len = P8_ROM_SIZE;
    return true;
}

// Construct the real packed 0x0000..0x42ff cartridge image for a text cart.
// The renderer keeps an expanded one-byte-per-pixel spritesheet for speed;
// this parallel load-time representation is what peek()/poke() consume.
static bool p8_build_rom(LoadedCart* out) {
    out->rom_buf = (uint8_t*)p8_alloc(P8_ROM_SIZE);
    if (!out->rom_buf) return false;
    memset(out->rom_buf, 0, P8_ROM_SIZE);

    // __gfx__: text pixels are left-to-right nibbles. PICO-8 RAM stores the
    // left/even pixel in the low nibble and the right/odd pixel in the high.
    size_t pixels = out->cart.gfx_len;
    if (pixels > P8_GFX_SIZE * 2) pixels = P8_GFX_SIZE * 2;
    for (size_t i = 0; i < pixels; i += 2) {
        uint8_t lo = out->gfx_buf[i] & 0x0f;
        uint8_t hi = (i + 1 < pixels) ? (out->gfx_buf[i + 1] & 0x0f) : 0;
        out->rom_buf[i >> 1] = (uint8_t)(lo | (hi << 4));
    }

    size_t map_len = out->cart.map_len;
    if (map_len > P8_MAP_SIZE) map_len = P8_MAP_SIZE;
    if (map_len) memcpy(out->rom_buf + P8_MAP_ADDR, out->map_buf, map_len);

    size_t gff_len = out->cart.gff_len;
    if (gff_len > P8_GFF_SIZE) gff_len = P8_GFF_SIZE;
    if (gff_len) memcpy(out->rom_buf + P8_GFF_ADDR, out->gff_buf, gff_len);

    // __music__: "ff aabbccdd". Channel IDs occupy seven bits; the four
    // low flag bits preserve bit 7 of all four packed channel bytes. Bits
    // 0..2 also have the normal begin/end/stop meaning. Restoring bit 3 is
    // essential for carts that use music memory as opaque reload() data.
    size_t music_count = out->cart.music_len / 11;
    if (music_count > 64) music_count = 64;
    for (size_t p = 0; p < music_count; ++p) {
        const uint8_t *line = out->music_buf + p * 11;
        uint8_t flags = (uint8_t)((p8_hex_nibble((char)line[0]) << 4)
                                | p8_hex_nibble((char)line[1]));
        for (int c = 0; c < 4; ++c) {
            uint8_t id = (uint8_t)((p8_hex_nibble((char)line[3 + c * 2]) << 4)
                                 | p8_hex_nibble((char)line[4 + c * 2]));
            out->rom_buf[P8_MUSIC_ADDR + p * 4 + c] =
                (uint8_t)((id & 0x7f) |
                          ((flags & (1u << c)) ? 0x80 : 0));
        }
    }

    // __sfx__: text has four metadata bytes first, then 32 five-nibble
    // notes. RAM stores the 64 note bytes first and metadata last.
    size_t sfx_count = out->cart.sfx_len / 168;
    if (sfx_count > 64) sfx_count = 64;
    for (size_t s = 0; s < sfx_count; ++s) {
        const uint8_t *line = out->sfx_buf + s * 168;
        uint8_t *dst = out->rom_buf + P8_SFX_ADDR + s * 68;
        for (int n = 0; n < 32; ++n) {
            const uint8_t *note = line + 8 + n * 5;
            uint16_t pitch = (uint16_t)((p8_hex_nibble((char)note[0]) << 4)
                                      | p8_hex_nibble((char)note[1]));
            uint16_t wave = p8_hex_nibble((char)note[2]);
            uint16_t vol = p8_hex_nibble((char)note[3]);
            uint16_t effect = p8_hex_nibble((char)note[4]);
            // Waveforms 8..15 select a custom instrument. The low three
            // waveform bits remain at 6..8 and the high/custom bit is stored
            // as bit 15 of the note word. Keeping it also makes arbitrary
            // cstore()/reload() data round-trip through a text cart exactly.
            uint16_t word = (uint16_t)((pitch & 0x3f) | ((wave & 7) << 6)
                                    | ((vol & 7) << 9) | ((effect & 7) << 12)
                                    | ((wave & 8) << 12));
            dst[n * 2] = (uint8_t)word;
            dst[n * 2 + 1] = (uint8_t)(word >> 8);
        }
        for (int b = 0; b < 4; ++b) {
            dst[64 + b] = (uint8_t)((p8_hex_nibble((char)line[b * 2]) << 4)
                                  | p8_hex_nibble((char)line[b * 2 + 1]));
        }
    }

    out->cart.rom = out->rom_buf;
    out->cart.rom_len = P8_ROM_SIZE;
    return true;
}

static bool p8_parse_text_impl(const char* data, size_t len,
                               const char* display_name, LoadedCart* out,
                               bool require_code) {
    if (!data || !out) return false;
    memset(out, 0, sizeof(*out));

    // ---- First pass: find section spans so we can size buffers. ----
    // We don't allocate intermediate copies; just record [start, end)
    // offsets for each section's payload bytes.
    struct Span { size_t start, end; bool present; };
    struct Span spans[8];
    memset(spans, 0, sizeof(spans));

    int cur = P8_NONE;
    size_t cur_payload_start = 0;
    size_t i = 0;
    while (i <= len) {
        // Find next newline or end-of-buffer.
        size_t line_start = i;
        while (i < len && data[i] != '\n') ++i;
        size_t line_end = i; // exclusive, no newline
        // Trim CR
        size_t trimmed_end = line_end;
        if (trimmed_end > line_start && data[trimmed_end - 1] == '\r') --trimmed_end;

        int header = p8_match_header(data + line_start, trimmed_end - line_start);
        if (header) {
            // Close previous section
            if (cur != P8_NONE) {
                spans[cur].end = line_start; // up to (not including) this header line
            }
            cur = header;
            cur_payload_start = (i < len) ? (i + 1) : i;
            spans[cur].start = cur_payload_start;
            spans[cur].end   = cur_payload_start;
            spans[cur].present = true;
        }

        if (i >= len) {
            if (cur != P8_NONE) spans[cur].end = len;
            break;
        }
        ++i; // skip the '\n'
    }

    // ---- LUA: copy raw source (including newlines), NUL-terminate. ----
    if (spans[P8_LUA].present) {
        size_t lua_len = spans[P8_LUA].end - spans[P8_LUA].start;
        // strip trailing whitespace
        while (lua_len > 0) {
            char c = data[spans[P8_LUA].start + lua_len - 1];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') --lua_len;
            else break;
        }
        out->code_buf = (uint8_t*)p8_alloc(lua_len + 1);
        if (!out->code_buf) goto fail;
        if (lua_len) memcpy(out->code_buf, data + spans[P8_LUA].start, lua_len);
        out->code_buf[lua_len] = 0;
        out->cart.code = out->code_buf;
        out->cart.code_len = (uint16_t)lua_len;
    }

    // ---- GFX: 1 nibble per char (max 128*128 = 16384). ----
    if (spans[P8_GFX].present) {
        size_t gfx_raw = spans[P8_GFX].end - spans[P8_GFX].start;
        if (gfx_raw > 0) {
            out->gfx_buf = (uint8_t*)p8_alloc(gfx_raw); // upper bound
            if (!out->gfx_buf) goto fail;
            size_t n = p8_nibbles(data + spans[P8_GFX].start, gfx_raw, out->gfx_buf);
            out->cart.gfx = out->gfx_buf;
            out->cart.gfx_len = (uint16_t)n;
        }
    }

    // ---- GFF: 2 chars per byte (sprite flags). ----
    if (spans[P8_GFF].present) {
        size_t raw = spans[P8_GFF].end - spans[P8_GFF].start;
        if (raw > 0) {
            out->gff_buf = (uint8_t*)p8_alloc(raw / 2 + 1);
            if (!out->gff_buf) goto fail;
            size_t n = p8_hex_bytes(data + spans[P8_GFF].start, raw, out->gff_buf);
            out->cart.gff = out->gff_buf;
            out->cart.gff_len = (uint16_t)n;
        }
    }

    // ---- LABEL: 1 nibble per char. ----
    if (spans[P8_LABEL].present) {
        size_t raw = spans[P8_LABEL].end - spans[P8_LABEL].start;
        if (raw > 0) {
            out->label_buf = (uint8_t*)p8_alloc(raw);
            if (!out->label_buf) goto fail;
            size_t n = p8_nibbles(data + spans[P8_LABEL].start, raw, out->label_buf);
            out->cart.label = out->label_buf;
            out->cart.label_len = (uint16_t)n;
        }
    }

    // ---- MAP: 2 chars per byte. ----
    if (spans[P8_MAP].present) {
        size_t raw = spans[P8_MAP].end - spans[P8_MAP].start;
        if (raw > 0) {
            out->map_buf = (uint8_t*)p8_alloc(raw / 2 + 1);
            if (!out->map_buf) goto fail;
            size_t n = p8_hex_bytes(data + spans[P8_MAP].start, raw, out->map_buf);
            out->cart.map = out->map_buf;
            out->cart.map_len = (uint16_t)n;
        }
    }

    // ---- SFX: raw concatenated hex text (one entry = 168 chars).
    //      SFXParser() expects the raw text bytes, not parsed nibbles. ----
    if (spans[P8_SFX].present) {
        size_t raw = spans[P8_SFX].end - spans[P8_SFX].start;
        if (raw > 0) {
            out->sfx_buf = (uint8_t*)p8_alloc(raw);
            if (!out->sfx_buf) goto fail;
            // Filter out newlines (keep only hex chars so each SFX is exactly 168 chars).
            size_t n = 0;
            for (size_t k = 0; k < raw; ++k) {
                char c = data[spans[P8_SFX].start + k];
                if (c == '\n' || c == '\r') continue;
                out->sfx_buf[n++] = (uint8_t)c;
            }
            out->cart.sfx = out->sfx_buf;
            out->cart.sfx_len = (uint16_t)n;
        }
    }

    // ---- MUSIC: compact text lines, "ff aabbccdd" (11 bytes/line). ----
    if (spans[P8_MUSIC].present) {
        size_t raw = spans[P8_MUSIC].end - spans[P8_MUSIC].start;
        if (raw > 0) {
            out->music_buf = (uint8_t*)p8_alloc(raw);
            if (!out->music_buf) goto fail;
            size_t n = 0;
            for (size_t k = 0; k < raw; ++k) {
                char c = data[spans[P8_MUSIC].start + k];
                if (c == '\n' || c == '\r') continue;
                out->music_buf[n++] = (uint8_t)c;
            }
            out->cart.music = out->music_buf;
            out->cart.music_len = (uint16_t)n;
        }
    }

    if (!p8_build_rom(out)) goto fail;

    // ---- Display name. ----
    {
        const char* n = display_name ? display_name : "cart";
        size_t nl = strlen(n);
        if (nl >= sizeof(out->name_buf)) nl = sizeof(out->name_buf) - 1;
        memcpy(out->name_buf, n, nl);
        out->name_buf[nl] = 0;
        out->cart.name = out->name_buf;
        out->cart.name_len = (uint8_t)nl;
    }

    // Executable carts need Lua source. reload() banks, however, may consist
    // only of packed-ROM sections. Still reject an unrelated/empty text file
    // rather than silently treating it as a zero-filled cartridge.
    return require_code ? out->code_buf != NULL
                        : (out->code_buf != NULL || spans[P8_GFX].present ||
                           spans[P8_GFF].present || spans[P8_MAP].present ||
                           spans[P8_SFX].present || spans[P8_MUSIC].present);

fail:
    p8_cart_free(out);
    return false;
}

bool p8_parse_text(const char* data, size_t len,
                   const char* display_name, LoadedCart* out) {
    return p8_parse_text_impl(data, len, display_name, out, true);
}

bool p8_parse_text_data(const char* data, size_t len,
                        const char* display_name, LoadedCart* out) {
    return p8_parse_text_impl(data, len, display_name, out, false);
}

#endif // P8_TEXT_PARSER_C
