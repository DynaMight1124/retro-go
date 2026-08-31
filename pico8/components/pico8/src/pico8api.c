#include "pico8api.h"
#include "engine.h"
#include "backend.h"
#include "lua/lauxlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>     // floorf — _lua_tline texture-stream sampling     // int32_t — disambiguates fix32 ctor calls (engine.cpp now text-includes us as C++)

// --- PICO-8 number coercion helpers ---------------------------------------
//
// PICO-8 auto-coerces strings to numbers everywhere (e.g. `cls("0")` works).
// Standard Lua's luaL_checkinteger / luaL_optinteger reject strings with
// "bad argument: number expected, got string".  These wrappers try
// lua_tonumber first (which auto-coerces) and fall back to the strict
// check only when the value isn't a number-like string.

static int p8_checkinteger(lua_State* L, int idx) {
    if (lua_isnumber(L, idx))
        return (int)lua_tonumber(L, idx);
    if (lua_isstring(L, idx)) {
        const char* s = lua_tostring(L, idx);
        if (s && s[0]) {
            char* end = NULL;
            long v = strtol(s, &end, 0);
            if (end && end != s) return (int)v;
            double d = strtod(s, &end);
            if (end && end != s) return (int)d;
        }
        // PICO-8 coerces unparseable/empty strings to 0 — never raises.
        return 0;
    }
    // Not a number, not a string (boolean, nil, table, function, …).
    // PICO-8 returns 0 for non-string non-number too.
    return 0;
}

static int p8_optinteger(lua_State* L, int idx, int def) {
    if (lua_isnoneornil(L, idx)) return def;
    return p8_checkinteger(L, idx);
}

static lua_Number p8_checknumber(lua_State* L, int idx) {
    if (lua_isnumber(L, idx))
        return lua_tonumber(L, idx);
    if (lua_isstring(L, idx)) {
        const char* s = lua_tostring(L, idx);
        if (s && s[0]) {
            char* end = NULL;
            float v = strtof(s, &end);
            if (end && end != s) return v;
        }
        // PICO-8 coerces unparseable/empty strings to 0 — never raises.
        return 0.0;
    }
    return 0.0;
}

static lua_Number p8_optnumber(lua_State* L, int idx, lua_Number def) {
    if (lua_isnoneornil(L, idx)) return def;
    return p8_checknumber(L, idx);
}

int _lua_flr(lua_State* L) {
    // PICO-8 flr() accepts a missing/nil/non-numeric argument as zero. Keep
    // that compatibility while avoiding the prelude's Lua call followed by
    // a second C call to math.floor for every coordinate/noise sample.
    const lua_Number value = p8_optnumber(
        L, 1, lua_Number::frombits(int32_t{0}));
    lua_pushnumber(L, z8::fix32::floor(value));
    return 1;
}

// peek4/poke4 transfer the raw 32-bit representation of a PICO-8 number,
// not its truncated integer part. lua_Number is z8::fix32 in this port, so
// retain all 16 integer and 16 fractional bits verbatim.
static uint32_t p8_checkfixedbits(lua_State* L, int idx) {
    return (uint32_t)p8_checknumber(L, idx).bits();
}

static inline color_t p8_display_color(int value);

// Write canonical PICO-8 RAM and keep the engine's fast expanded views in
// sync. Normal rendering still reads the expanded arrays directly.
static inline uint8_t p8_ram_read(uint32_t addr) {
    if (addr < 0x8000) return ram[addr];
    if (addr < P8_RAM_SIZE && ram_high != NULL) return ram_high[addr - 0x8000];
    return 0;
}

static inline void p8_ram_write(uint32_t addr, uint8_t value) {
    if (addr >= P8_RAM_SIZE) return;
    if (addr < 0x8000) ram[addr] = value;
    else if (ram_high != NULL) ram_high[addr - 0x8000] = value;

    // SFX RAM is writable state, not merely cartridge data. Spatial-audio
    // carts such as X-Wing copy a source effect to slot 63 with poke4(),
    // alter its packed note volumes, then call sfx(63). Keep the expanded
    // mixer cache coherent with those uncommon writes; the audio hot path
    // itself remains unchanged.
    if (addr >= 0x3200 && addr < 0x4300) {
        const uint32_t offset = addr - 0x3200;
        const uint8_t slot = (uint8_t)(offset / 68);
        const uint8_t field = (uint8_t)(offset % 68);
        const uint32_t base = 0x3200u + (uint32_t)slot * 68u;

        if (field < 64) {
            const uint8_t note_index = field >> 1;
            const uint32_t note_addr = base + (uint32_t)note_index * 2u;
            const uint16_t word = (uint16_t)p8_ram_read(note_addr)
                                | ((uint16_t)p8_ram_read(note_addr + 1) << 8);
            Note *note = &sfx[slot].notes[note_index];
            note->key = word & 0x3f;
            note->waveform = (uint8_t)(((word >> 6) & 0x07)
                                     | ((word >> 12) & 0x08));
            note->volume = (word >> 9) & 0x07;
            note->effect = (word >> 12) & 0x07;
        } else if (field == 65) {
            sfx[slot].duration = value;
        } else if (field == 66) {
            sfx[slot].loop_start = value;
        } else if (field == 67) {
            sfx[slot].loop_end = value;
        }
    }

    if (addr < 0x2000) {
        size_t pixel = (size_t)addr * 2;
        spritesheet.sprite_data[pixel] = value & 0x0f;
        spritesheet.sprite_data[pixel + 1] = value >> 4;
        if (addr >= 0x1000)
            map_data[4096 + (addr - 0x1000)] = value;
    } else if (addr < 0x3000) {
        map_data[addr - 0x2000] = value;
    } else if (addr < 0x3100) {
        spritesheet.flags[addr - 0x3000] = value;
    }

    // PICO-8 exposes its draw state through RAM. Carts use poke(), poke2(),
    // memcpy(), and memset() here interchangeably with the graphics APIs, so
    // keep the engine's expanded hot state coherent after every such write.
    if (addr >= 0x5f00 && addr <= 0x5f0f) {
        const uint8_t index = (uint8_t)(addr - 0x5f00);
        pal_map[index] = value & 0x0f;
        // PICO-8 draw-palette entries use bit 4 as the transparency flag;
        // bits 0..3 select the remapped colour. Snow builds distance-faded
        // PICO-8 carts in the wild use both packed transparency encodings.
        // Current carts such as Snow use bit 4, while older carts such as
        // Dank Tomb deliberately add bit 7 before memcpy()ing a complete
        // draw palette into 0x5f00. The mapped colour remains the low
        // nibble in either form. Accept both on cart RAM writes, but keep
        // API-generated state canonical (bit 4) below.
        drawstate.transparent[index] = (value & 0x90) != 0;
    } else if (addr >= 0x5f10 && addr <= 0x5f1f) {
        palette[addr - 0x5f10] = p8_display_color(value);
    } else if (addr >= 0x5f20 && addr <= 0x5f23) {
        uint16_t x0 = MIN((uint16_t)ram[0x5f20], (uint16_t)SCREEN_WIDTH);
        uint16_t y0 = MIN((uint16_t)ram[0x5f21], (uint16_t)SCREEN_HEIGHT);
        uint16_t x1 = MIN((uint16_t)ram[0x5f22], (uint16_t)SCREEN_WIDTH);
        uint16_t y1 = MIN((uint16_t)ram[0x5f23], (uint16_t)SCREEN_HEIGHT);
        drawstate.clip_x = (uint8_t)x0;
        drawstate.clip_y = (uint8_t)y0;
        drawstate.clip_w = (uint8_t)(x1 > x0 ? x1 - x0 : 0);
        drawstate.clip_h = (uint8_t)(y1 > y0 ? y1 - y0 : 0);
    } else if (addr == 0x5f25) {
        drawstate.pen_color = value;
    } else if (addr == 0x5f26) {
        drawstate.cursor_x = value;
    } else if (addr == 0x5f27) {
        drawstate.cursor_y = value;
    } else if (addr == 0x5f28 || addr == 0x5f29) {
        drawstate.camera_x = (int16_t)(ram[0x5f28]
                           | ((uint16_t)ram[0x5f29] << 8));
    } else if (addr == 0x5f2a || addr == 0x5f2b) {
        drawstate.camera_y = (int16_t)(ram[0x5f2a]
                           | ((uint16_t)ram[0x5f2b] << 8));
    } else if (addr == 0x5f31 || addr == 0x5f32) {
        drawstate.fill_pattern = (uint16_t)(ram[0x5f31]
                               | ((uint16_t)ram[0x5f32] << 8));
    } else if (addr == 0x5f33) {
        drawstate.fill_flags = (uint8_t)((drawstate.fill_flags & ~4u)
                             | ((value & 1u) ? 4u : 0u));
    }
}

typedef struct {
    uint8_t page;
    uint16_t width;
    uint32_t capacity;
    bool default_layout;
} P8MapLayout;

// PICO-8 0.2.4+ lets carts relocate map storage with the hardware-state
// registers at 0x5f56/0x5f57. In particular, a base page of 0x80 places the
// map in upper user RAM instead of overwriting the lower half of the sprite
// sheet. Keep the common 128x64 layout on the existing flat map_data array;
// remapped carts take the slightly slower RAM-address path only while their
// map is relocated.
static inline bool p8_map_layout(P8MapLayout* layout) {
    uint8_t page = ram[0x5f56];
    uint16_t width = ram[0x5f57] ? ram[0x5f57] : 256;

    // Pages 0x30..0x3f alias the shared-memory pages 0x10..0x1f.
    if (page >= 0x30 && page <= 0x3f) page -= 0x20;

    layout->page = page;
    layout->width = width;
    layout->default_layout = page == 0x20 && width == 128;

    if (page >= 0x10 && page <= 0x2f) {
        layout->capacity = 0x2000;
        return true;
    }
    if (page >= 0x80) {
        layout->capacity = 0x10000u - ((uint32_t)page << 8);
        return true;
    }
    layout->capacity = 0;
    return false;
}

static inline bool p8_map_address(const P8MapLayout* layout,
                                  int32_t x, int32_t y, uint32_t* addr) {
    if (x < 0 || y < 0 || x >= layout->width) return false;
    uint32_t offset = (uint32_t)y * layout->width + (uint32_t)x;
    if (offset >= layout->capacity) return false;

    if (layout->page >= 0x80) {
        *addr = ((uint32_t)layout->page << 8) + offset;
    } else {
        // The 8-KB lower map region is a ring: pages 0x20..0x2f are followed
        // by their 0x30..0x3f aliases, which resolve to 0x10..0x1f. This is
        // why the default page 0x20 exposes MAP followed by shared GFX2/MAP2.
        uint32_t page = layout->page + (offset >> 8);
        page = 0x10u + ((page - 0x10u) & 0x1fu);
        *addr = (page << 8) | (offset & 0xffu);
    }
    return true;
}

void gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const palidx_t color);

// Store a palette-mapped pixel after the caller has clipped its coordinates.
// Sprite loops use this directly so they do not repeat four clip comparisons
// and the palette lookup for every opaque pixel.
static inline void put_pixel_mapped_unchecked(uint8_t x, uint8_t y, uint8_t mapped) {
    const uint16_t offset = ((uint16_t)y << 6) + (x >> 1);
    const uint32_t target = (uint32_t)ram[0x5f55] << 8;
    if (target == 0x6000) {
        const uint8_t old = frontbuffer[offset];
        frontbuffer[offset] = (x & 1)
            ? (uint8_t)((old & 0x0f) | (mapped << 4))
            : (uint8_t)((old & 0xf0) | mapped);
        return;
    }

    // PICO-8 lets carts redirect graphics commands to another 0x2000-byte
    // RAM region through 0x5f55. Flooded Caves draws its procedural level
    // into sprite RAM (page 0) and later memcpy()s that image to the screen.
    // Keep the canonical RAM and expanded sprite/map views coherent.
    const uint32_t addr = target + offset;
    if (addr >= P8_RAM_SIZE) return;
    const uint8_t old = p8_ram_read(addr);
    p8_ram_write(addr, (x & 1)
        ? (uint8_t)((old & 0x0f) | (mapped << 4))
        : (uint8_t)((old & 0xf0) | mapped));
}

// callers have to ensure this is not called with x > SCREEN_WIDTH or y > SCREEN_HEIGHT
static inline void put_pixel_raw(uint8_t x, uint8_t y, palidx_t p){
    if (x < drawstate.clip_x || y < drawstate.clip_y ||
        x >= drawstate.clip_x + drawstate.clip_w ||
        y >= drawstate.clip_y + drawstate.clip_h) return;
    const uint8_t mapped = pal_map[p & 0x0f];
    put_pixel_mapped_unchecked(x, y, mapped);
}

static inline void put_pixel(uint8_t x, uint8_t y, palidx_t p){
    // Bit 0b0.001 makes the secondary palette apply to primitive drawing as
    // well as sprites. The draw palette is resolved first, then the selected
    // secondary-palette nibble becomes the final framebuffer colour.
    if (drawstate.fill_flags & 1) {
        const uint8_t primary = pal_map[p & 0x0f] & 0x0f;
        const uint8_t pair = ram[0x5f60 + primary];
        const uint16_t bit =
            (uint16_t)(0x8000u >> (((y & 3) << 2) | (x & 3)));
        if (drawstate.fill_pattern & bit) {
            if (drawstate.fill_flags & 4) return;
            put_pixel_mapped_unchecked(x, y, pair >> 4);
        } else {
            put_pixel_mapped_unchecked(x, y, pair & 0x0f);
        }
        return;
    }
    if (drawstate.fill_pattern != 0) {
        uint16_t bit = (uint16_t)(0x8000u >> (((y & 3) << 2) | (x & 3)));
        if (drawstate.fill_pattern & bit) {
            if (drawstate.fill_flags & 4) return;
            p = (p >> 4) & 0x0f;
        } else p &= 0x0f;
    }
    put_pixel_raw(x, y, p);
}

static inline void put_sprite_pixel(uint8_t x, uint8_t y, palidx_t p){
    const uint8_t primary = pal_map[p & 0x0f] & 0x0f;
    if (drawstate.fill_flags & 2) {
        const uint8_t pair = ram[0x5f60 + primary];
        const uint16_t bit =
            (uint16_t)(0x8000u >> (((y & 3) << 2) | (x & 3)));
        if (drawstate.fill_pattern & bit) {
            if (drawstate.fill_flags & 4) return;
            put_pixel_mapped_unchecked(x, y, pair >> 4);
        } else {
            put_pixel_mapped_unchecked(x, y, pair & 0x0f);
        }
        return;
    }
    put_pixel_mapped_unchecked(x, y, primary);
}

// Store two already-clipped sprite pixels at an even destination x. Sprite
// transparency is per source colour, so preserve either destination nibble
// independently when required. The common fully-opaque pair becomes one
// packed framebuffer write rather than two read-modify-write operations.
static inline void put_sprite_pair_unchecked(uint8_t x, uint8_t y,
                                             uint8_t left, uint8_t right) {
    const bool left_transparent = drawstate.transparent[left];
    const bool right_transparent = drawstate.transparent[right];
    if (left_transparent && right_transparent) return;

    uint8_t *dst = &frontbuffer[((uint16_t)y << 6) + (x >> 1)];
    const uint8_t mapped_left = pal_map[left & 0x0f];
    const uint8_t mapped_right = pal_map[right & 0x0f];
    if (ram[0x5f55] != 0x60) {
        if (!left_transparent)
            put_pixel_mapped_unchecked(x, y, mapped_left);
        if (!right_transparent)
            put_pixel_mapped_unchecked((uint8_t)(x + 1), y, mapped_right);
        return;
    }
    if (!left_transparent && !right_transparent) {
        *dst = (uint8_t)(mapped_left | (mapped_right << 4));
        return;
    }

    uint8_t packed = *dst;
    if (!left_transparent)
        packed = (uint8_t)((packed & 0xf0) | mapped_left);
    if (!right_transparent)
        packed = (uint8_t)((packed & 0x0f) | (mapped_right << 4));
    *dst = packed;
}

// Draw an inclusive horizontal primitive span. Solid fills are packed two
// pixels per framebuffer byte after handling the two possible edge nibbles;
// fillp() spans retain per-pixel evaluation because their colour/transparent
// decision depends on x and y. Screen and draw clipping are resolved once.
static inline void put_hspan_clipped(int x0, int x1, int y, palidx_t p) {
    const int clip_bottom = drawstate.clip_y + drawstate.clip_h;
    if (y < 0 || y >= SCREEN_HEIGHT ||
        y < drawstate.clip_y || y >= clip_bottom)
        return;

    int left = MAX(0, MAX(x0, (int)drawstate.clip_x));
    int right = MIN(SCREEN_WIDTH - 1,
                    MIN(x1, (int)drawstate.clip_x + drawstate.clip_w - 1));
    if (left > right) return;

    if (drawstate.fill_pattern != 0 || ram[0x5f55] != 0x60) {
        for (int x = left; x <= right; ++x)
            put_pixel((uint8_t)x, (uint8_t)y, p);
        return;
    }

    const uint8_t mapped = pal_map[p & 0x0f];
    const uint8_t packed = (uint8_t)(mapped | (mapped << 4));
    const uint16_t row = (uint16_t)y << 6;

    if (left & 1) {
        uint8_t *edge = &frontbuffer[row + (left >> 1)];
        *edge = (uint8_t)((*edge & 0x0f) | (mapped << 4));
        ++left;
    }

    int pairs = (right - left + 1) >> 1;
    if (pairs > 0) {
        memset(frontbuffer + row + (left >> 1), packed, (size_t)pairs);
        left += pairs << 1;
    }

    if (left <= right) {
        uint8_t *edge = &frontbuffer[row + (left >> 1)];
        *edge = (uint8_t)((*edge & 0xf0) | mapped);
    }
}

// Draw an inclusive vertical primitive span. The packed framebuffer prevents
// a memset as used by horizontal spans, but resolving clipping, fill state,
// and palette mapping once still removes most of ovalfill()'s per-pixel work.
static inline void put_vspan_clipped(int x, int y0, int y1, palidx_t p) {
    const int clip_right = drawstate.clip_x + drawstate.clip_w;
    if (x < 0 || x >= SCREEN_WIDTH ||
        x < drawstate.clip_x || x >= clip_right)
        return;

    int top = MAX(0, MAX(y0, (int)drawstate.clip_y));
    int bottom = MIN(SCREEN_HEIGHT - 1,
                     MIN(y1, (int)drawstate.clip_y + drawstate.clip_h - 1));
    if (top > bottom) return;

    if (drawstate.fill_pattern != 0 || ram[0x5f55] != 0x60) {
        for (int y = top; y <= bottom; ++y)
            put_pixel((uint8_t)x, (uint8_t)y, p);
        return;
    }

    const uint8_t mapped = pal_map[p & 0x0f];
    uint8_t *dst = frontbuffer + ((uint16_t)top << 6) + (x >> 1);
    if (x & 1) {
        for (int y = top; y <= bottom; ++y, dst += SCREEN_WIDTH / 2)
            *dst = (uint8_t)((*dst & 0x0f) | (mapped << 4));
    } else {
        for (int y = top; y <= bottom; ++y, dst += SCREEN_WIDTH / 2)
            *dst = (uint8_t)((*dst & 0xf0) | mapped);
    }
}

static inline void guarded_put_pixel(int16_t x, int16_t y, palidx_t p){
	if(x>=0&&x<SCREEN_WIDTH && y<SCREEN_HEIGHT&&y>=0) {
		put_pixel(x, y, p);
	}
}

void gfx_ovalfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, palidx_t p){
   int a = abs (x1 - x0), b = abs (y1 - y0), b1 = b & 1; /* values of diameter */
   long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
   long err = dx + dy + b1 * a * a, e2; /* error of 1.step */

   if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
   if (y0 > y1) y0 = y1; /* .. exchange them */
   y0 += (b + 1) / 2;
   y1 = y0-b1;   /* starting pixel */
   a *= 8 * a; b1 = 8 * b * b;
   do
   {
	   put_vspan_clipped(x0, y1, y0, p);
	   if (x1 != x0) put_vspan_clipped(x1, y1, y0, p);
       e2 = 2 * err;
       if (e2 >= dx)
       {
          x0++;
          x1--;
          err += dx += b1;
       } /* x step */
       if (e2 <= dy)
       {
          y0++;
          y1--;
          err += dy += a;
       }  /* y step */
   } while (x0 <= x1);
   while (y0-y1 < b)
   {  /* too early stop of flat ellipses a=1 */
       guarded_put_pixel(x0-1, y0, p); /* -> finish tip of ellipse */
       guarded_put_pixel(x1+1, y0++, p);
       guarded_put_pixel(x0-1, y1, p);
       guarded_put_pixel(x1+1, y1--, p);
   }
}

void gfx_oval(int16_t x0, int16_t y0, int16_t x1, int16_t y1, palidx_t p){
   int a = abs (x1 - x0), b = abs (y1 - y0), b1 = b & 1; /* values of diameter */
   long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
   long err = dx + dy + b1 * a * a, e2; /* error of 1.step */

   if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
   if (y0 > y1) y0 = y1; /* .. exchange them */
   y0 += (b + 1) / 2;
   y1 = y0-b1;   /* starting pixel */
   a *= 8 * a; b1 = 8 * b * b;
   do
   {
       guarded_put_pixel(x1, y0, p); /*   I. Quadrant */
       guarded_put_pixel(x0, y0, p); /*  II. Quadrant */
       guarded_put_pixel(x0, y1, p); /* III. Quadrant */
       guarded_put_pixel(x1, y1, p); /*  IV. Quadrant */
       e2 = 2 * err;
       if (e2 >= dx)
       {
          x0++;
          x1--;
          err += dx += b1;
       } /* x step */
       if (e2 <= dy)
       {
          y0++;
          y1--;
          err += dy += a;
       }  /* y step */
   } while (x0 <= x1);
   while (y0-y1 < b)
   {  /* too early stop of flat ellipses a=1 */
       guarded_put_pixel(x0-1, y0, p); /* -> finish tip of ellipse */
       guarded_put_pixel(x1+1, y0++, p);
       guarded_put_pixel(x0-1, y1, p);
       guarded_put_pixel(x1+1, y1--, p);
   }
}

// Keep the full encoded PICO-8 colour here. The low nibble is still the
// framebuffer palette index, while high bits such as 0x1800 carry optional
// shape flags when enabled through 0x5f34.
void gfx_circlefill(int16_t x, int16_t y, int16_t radius, int32_t p){
    if (radius < 0) return;

    const bool inverted = (ram[0x5f34] & 0x02)
                       && ((p & 0x1800) == 0x1800);

    // This is the same integer-pixel predicate as the old square scan:
    // dx*dx + dy*dy <= radius*radius. Walk symmetric rows instead, reducing
    // the geometry work to O(radius), then let the packed span writer handle
    // the actual pixels and clipping.
    const int32_t r_sq = (int32_t)radius * radius;
    int extent = radius;

    // 0x5f34 bit 1 inverts filled circles: draw every pixel outside the
    // circle instead of its interior. PICO-BALL uses this as a cheap
    // full-screen peephole after rendering gameplay. Keep the same O(radius)
    // boundary walk and packed span path rather than testing all 16K pixels.
    // The register enables encoded-colour shape flags; inversion itself is
    // requested per call by bits 0x1800. Ordinary circfill() calls must stay
    // ordinary after the mode is enabled (PICO-BALL's menus use many of
    // them), otherwise each small UI circle overwrites almost the full screen.
    if (inverted) {
        for (int sy = 0; sy < y - radius; ++sy)
            put_hspan_clipped(0, SCREEN_WIDTH - 1, sy, p);

        for (int dy = 0; dy <= radius; ++dy) {
            while (extent > 0 &&
                   (int32_t)extent * extent + (int32_t)dy * dy > r_sq)
                --extent;
            const int y0 = y + dy;
            const int y1 = y - dy;
            put_hspan_clipped(0, x - extent - 1, y0, p);
            put_hspan_clipped(x + extent + 1, SCREEN_WIDTH - 1, y0, p);
            if (dy != 0) {
                put_hspan_clipped(0, x - extent - 1, y1, p);
                put_hspan_clipped(x + extent + 1, SCREEN_WIDTH - 1, y1, p);
            }
        }

        for (int sy = y + radius + 1; sy < SCREEN_HEIGHT; ++sy)
            put_hspan_clipped(0, SCREEN_WIDTH - 1, sy, p);
        return;
    }

    for (int dy = 0; dy <= radius; ++dy) {
        while (extent > 0 &&
               (int32_t)extent * extent + (int32_t)dy * dy > r_sq)
            --extent;
        put_hspan_clipped(x - extent, x + extent, y + dy, p);
        if (dy != 0)
            put_hspan_clipped(x - extent, x + extent, y - dy, p);
    }
}
void gfx_circle(int32_t centreX, int32_t centreY, int32_t radius, palidx_t color){
    const int32_t diameter = (radius * 2);

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - diameter);

    while (x >= y) {
        //  Each of the following renders an octant of the circle
        guarded_put_pixel(centreX + x, centreY - y, color);
        guarded_put_pixel(centreX + x, centreY + y, color);
        guarded_put_pixel(centreX - x, centreY - y, color);
        guarded_put_pixel(centreX - x, centreY + y, color);
        guarded_put_pixel(centreX + y, centreY - x, color);
        guarded_put_pixel(centreX + y, centreY + x, color);
        guarded_put_pixel(centreX - y, centreY - x, color);
        guarded_put_pixel(centreX - y, centreY + x, color);

        if (error <= 0) {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            --x;
            tx += 2;
            error += (tx - diameter);
        }
    }
}


void gfx_cls(palidx_t c) {
    const uint8_t packed = (uint8_t)(((c & 0xf) << 4) | (c & 0xf));
    const uint32_t target = (uint32_t)ram[0x5f55] << 8;
    if (target == 0x6000) {
        memset(frontbuffer, packed, SCREEN_WIDTH * SCREEN_HEIGHT / 2);
    } else {
        for (uint32_t offset = 0; offset < SCREEN_WIDTH * SCREEN_HEIGHT / 2;
             ++offset) {
            p8_ram_write(target + offset, packed);
        }
    }
    drawstate.clip_x = 0;
    drawstate.clip_y = 0;
    drawstate.clip_w = SCREEN_WIDTH;
    drawstate.clip_h = SCREEN_HEIGHT;
    drawstate.cursor_x = 0;
    drawstate.cursor_y = 0;
    ram[0x5f26] = 0;
    ram[0x5f27] = 0;
    ram[0x5f20] = drawstate.clip_x;
    ram[0x5f21] = drawstate.clip_y;
    ram[0x5f22] = (uint8_t)(drawstate.clip_x + drawstate.clip_w);
    ram[0x5f23] = (uint8_t)(drawstate.clip_y + drawstate.clip_h);
}

void gfx_rect(int16_t x0, int16_t y0, int16_t x2, int16_t y2, const palidx_t color) {
    // RECT/RECTFILL take two corners, not an origin and positive extent.
    // Normalize before clipping so either corner order draws the same shape.
    // Alone in Pico's software triangle rasterizer emits spans in both
    // directions and otherwise loses one winding of its geometry.
    if (x0 > x2) { int16_t tmp = x0; x0 = x2; x2 = tmp; }
    if (y0 > y2) { int16_t tmp = y0; y0 = y2; y2 = tmp; }
    if (x2 < 0 || y2 < 0 || x0 >= SCREEN_WIDTH || y0 >= SCREEN_HEIGHT)
        return;

    x0 = MAX(x0, 0);
    x2 = MIN(x2, SCREEN_WIDTH - 1);
    y0 = MAX(y0, 0);
    y2 = MIN(y2, SCREEN_HEIGHT - 1);

    for(int16_t y=y0; y<=y2; y++)
        for(int16_t x=x0; x<=x2; x++)
            if ((y==y0) || (y==y2) || (x==x0) || (x==x2))
                guarded_put_pixel(x, y, color);
}

void gfx_rectfill(int16_t x0, int16_t y0, int16_t x2, int16_t y2, const palidx_t color) {
    // this is _inclusive_
    if (x0 > x2) { int16_t tmp = x0; x0 = x2; x2 = tmp; }
    if (y0 > y2) { int16_t tmp = y0; y0 = y2; y2 = tmp; }
    if (x2 < 0 || y2 < 0 || x0 >= SCREEN_WIDTH || y0 >= SCREEN_HEIGHT)
        return;

    x0 = MAX(x0, 0);
    x2 = MIN(x2, SCREEN_WIDTH - 1);
    y0 = MAX(y0, 0);
    y2 = MIN(y2, SCREEN_HEIGHT - 1);

    for (int16_t y = y0; y <= y2; ++y)
        put_hspan_clipped(x0, x2, y, color);
}


inline palidx_t get_pixel(uint8_t x, uint8_t y) {
    const uint16_t offset = ((uint16_t)y << 6) + (x >> 1);
    const uint32_t target = (uint32_t)ram[0x5f55] << 8;
    const uint8_t packed = target == 0x6000
                         ? frontbuffer[offset]
                         : p8_ram_read(target + offset);
    return (x & 1) ? (packed >> 4) : (packed & 0x0f);
}

/* Ordinary map tiles are unscaled, unflipped 8x8 sprite blits. The generic
   sprite renderer already clips once per tile, but then performs a packed
   framebuffer read/modify/write for every opaque pixel. Pairing aligned
   neighbours lets the common opaque case store a complete byte. Specialized
   sprite-memory, draw-target, and fill-pattern modes stay on render(). */
static inline void p8_render_map_tile_packed(uint8_t sprite,
                                             int32_t dst_x,
                                             int32_t dst_y) {
    const int32_t clip_right = MIN(SCREEN_WIDTH,
        (int32_t)drawstate.clip_x + drawstate.clip_w);
    const int32_t clip_bottom = MIN(SCREEN_HEIGHT,
        (int32_t)drawstate.clip_y + drawstate.clip_h);
    int32_t xmin = MAX(0, MAX(-dst_x,
        (int32_t)drawstate.clip_x - dst_x));
    int32_t ymin = MAX(0, MAX(-dst_y,
        (int32_t)drawstate.clip_y - dst_y));
    const int32_t xmax = MIN(8, MIN(SCREEN_WIDTH - dst_x,
                                    clip_right - dst_x));
    const int32_t ymax = MIN(8, MIN(SCREEN_HEIGHT - dst_y,
                                    clip_bottom - dst_y));
    if (xmin >= xmax || ymin >= ymax) return;

    const int32_t source_x = (sprite & 0x0f) * 8;
    const int32_t source_y = (sprite >> 4) * 8;
    for (int32_t y = ymin; y < ymax; ++y) {
        const uint8_t *source = spritesheet.sprite_data
            + (source_y + y) * 128 + source_x;
        const uint8_t screen_y = (uint8_t)(dst_y + y);
        int32_t x = xmin;

        if ((dst_x + x) & 1) {
            const uint8_t value = source[x];
            if (!drawstate.transparent[value])
                put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y,
                                           pal_map[value & 0x0f]);
            ++x;
        }
        for (; x + 1 < xmax; x += 2) {
            put_sprite_pair_unchecked((uint8_t)(dst_x + x), screen_y,
                                      source[x], source[x + 1]);
        }
        if (x < xmax) {
            const uint8_t value = source[x];
            if (!drawstate.transparent[value])
                put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y,
                                           pal_map[value & 0x0f]);
        }
    }
}


static void map(int16_t mapX, int16_t mapY, int16_t screenX, int16_t screenY,
                int16_t cellW, int16_t cellH, uint8_t layerFlags=0) {

    // Clip negative source cells instead of rejecting the complete map.
    // Camera-following carts commonly request a one-tile border beyond the
    // map at its top/left edge. The visible source begins at cell zero and
    // its destination must advance by the number of skipped cells so world
    // alignment remains unchanged (Moonrace exposes this as a whole track
    // layer blinking off whenever its camera crosses either edge).
    if (mapX < 0) {
        const int skipped = -(int)mapX;
        if (skipped >= cellW) return;
        mapX = 0;
        cellW -= skipped;
        screenX += skipped * 8;
    }
    if (mapY < 0) {
        const int skipped = -(int)mapY;
        if (skipped >= cellH) return;
        mapY = 0;
        cellH -= skipped;
        screenY += skipped * 8;
    }

    P8MapLayout layout;
    if (!p8_map_layout(&layout) || cellW <= 0 || cellH <= 0) return;
    const int32_t map_height = (int32_t)(layout.capacity / layout.width);

    /* Intersect the requested cell rectangle with both map storage and the
       active draw viewport before walking it. A bare map() requests the
       entire 128x64 map, but at most roughly 17x17 cells can touch the
       128x128 screen. The old inner loop still visited all 128 columns for
       every visible row, and layered carts such as Celeste repeat that scan
       several times per frame. Destination offsets remain relative to the
       original request, so clipping cannot shift world alignment. */
    const int32_t columns = MIN((int32_t)cellW,
                                (int32_t)layout.width - mapX);
    const int32_t rows = MIN((int32_t)cellH, map_height - mapY);
    if (columns <= 0 || rows <= 0) return;

    const int32_t clip_left = MAX(0, (int32_t)drawstate.clip_x);
    const int32_t clip_top = MAX(0, (int32_t)drawstate.clip_y);
    const int32_t clip_right = MIN(SCREEN_WIDTH,
        (int32_t)drawstate.clip_x + drawstate.clip_w);
    const int32_t clip_bottom = MIN(SCREEN_HEIGHT,
        (int32_t)drawstate.clip_y + drawstate.clip_h);
    if (clip_left >= clip_right || clip_top >= clip_bottom) return;

    const int32_t base_x = (int32_t)screenX - drawstate.camera_x;
    const int32_t base_y = (int32_t)screenY - drawstate.camera_y;
    int32_t first_column = base_x < clip_left
        ? (clip_left - base_x) / 8 : 0;
    int32_t first_row = base_y < clip_top
        ? (clip_top - base_y) / 8 : 0;
    int32_t end_column = base_x < clip_right
        ? (clip_right - base_x + 7) / 8 : 0;
    int32_t end_row = base_y < clip_bottom
        ? (clip_bottom - base_y + 7) / 8 : 0;
    first_column = MAX(0, first_column);
    first_row = MAX(0, first_row);
    end_column = MIN(columns, end_column);
    end_row = MIN(rows, end_row);
    if (first_column >= end_column || first_row >= end_row) return;

    const bool packed_tiles = !(drawstate.fill_flags & 2)
                           && ram[0x5f54] == 0
                           && ram[0x5f55] == 0x60;

    // Loop iterators stay signed so cart-controlled coordinates cannot wrap
    // into a different part of map memory.
    for (int32_t row = first_row; row < end_row; ++row) {
        const int32_t y = (int32_t)mapY + row;
        const int32_t ty = (int32_t)screenY + row * 8;

        for (int32_t column = first_column; column < end_column; ++column) {
            const int32_t x = (int32_t)mapX + column;
            const int32_t tx = (int32_t)screenX + column * 8;

            uint8_t sprite;
            if (layout.default_layout) {
                sprite = map_data[x + y * 128];
            } else {
                uint32_t addr;
                if (!p8_map_address(&layout, x, y, &addr)) break;
                sprite = p8_ram_read(addr);
            }
            if(sprite==0) continue;

            // Map cells and the flags table both use the full uint8_t sprite
            // range, so every possible cell value is a valid flags index.
            uint8_t flags = spritesheet.flags[sprite];

            // PICO-8's layer argument is an any-bit mask: a tile is drawn
            // when at least one requested flag is present. Requiring every
            // bit hid most terrain in carts which combine layers (UFO Swamp
            // Odyssey requests mask 30 for its complete foreground).
            if ((layerFlags == 0 || (flags & layerFlags) != 0)
                && sprite != 0) {
                if (packed_tiles) {
                    p8_render_map_tile_packed(
                        sprite, base_x + column * 8, base_y + row * 8);
                } else {
                    render(&spritesheet, sprite, (int16_t)tx, (int16_t)ty,
                           -1, false, false);
                }
            }
        }
    }
}

void render_text(Spritesheet* s, uint16_t sprite, int16_t x0, int16_t y0, uint8_t width_ratio, uint8_t height_ratio) {
    // The PICO-8 default fontsheet is a 128x128 sprite sheet (matches the
    // sprite_data layout used by `spr()`). Each glyph is one of the 256
    // 8x8 cells arranged in a 16x16 grid. Default-font glyphs render only
    // the first 4 columns x 6 rows of each cell; the right 4 columns and
    // bottom 2 rows are zero padding so adjacent glyphs (cursor advances
    // by 4 px) read transparent pixels and don't bleed into neighbours.
    //
    // Bounds-check the sprite index so carts that decode UTF-8 multi-byte
    // markers into out-of-range cell numbers (very large values) bail out
    // cleanly instead of reading past the 16KB fontsheet.
    if (sprite >= 256) return;

    const uint8_t sprite_count = 16;
    const uint8_t xIndex = sprite % sprite_count;
    const uint8_t yIndex = sprite / sprite_count;
    uint8_t val;

    if (drawstate.bg_color) {
        // Backing-rectangle clears the 4x6 cell-and-cursor footprint behind
        // each char (one column and one row beyond the visible glyph so
        // invert-mode ink sits cleanly in the gap to the next glyph).
        for (int y=0; y<7*height_ratio; y++) {
            const int screen_y = y0 + y - 1;
            if (screen_y < 0 || screen_y >= SCREEN_HEIGHT) continue;
            for (int x=0; x<9*width_ratio; x++) {
                const int screen_x = x0 + x - 1;
                if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;
                put_pixel_raw((uint8_t)screen_x, (uint8_t)screen_y,
                              drawstate.bg_color);
            }
        }
    }
    for (int y=0; y<6*height_ratio; y++) {
        const int screen_y = y0 + y;
        if (screen_y < 0 || screen_y >= SCREEN_HEIGHT) continue;
        // y/height_ratio and x/width_ratio collapse to 0..5 / 0..7 inside
        // the 8x8 cell at (xIndex, yIndex) of the 128x128 fontsheet.
        for (int x=0; x<8*width_ratio; x++) {
            const int screen_x = x0 + x;
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;
            val = s->sprite_data[y/height_ratio*128 + x/width_ratio + xIndex*8 + yIndex*8*128];
            if (val != 0) {
                put_pixel_raw((uint8_t)screen_x, (uint8_t)screen_y,
                              drawstate.pen_color);
            }
        }
    }
}

static inline uint8_t print_param(uint8_t c) {
    // P8SCII parameters are a superset of hexadecimal: 0..9, a..f, then
    // g=16 and so on. Accept uppercase too because external .p8 sources can
    // preserve it even though the PICO-8 editor normally displays lowercase.
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return 0;
}

int _lua_set_fps(lua_State* L) {
    // Undocumented but widely used by newer carts and multicart loaders.
    // PICO-8 supports switching between its two callback rates at runtime.
    // Keep unusual values deterministic by selecting the nearest supported
    // rate; numeric strings are accepted through the normal PICO coercion.
    int requested = p8_checkinteger(L, 1);
    engine_requested_frame_rate = requested > 30 ? 60 : 30;
    return 0;
}

static inline int8_t print_hex_nibble(uint8_t c) {
    if (c >= '0' && c <= '9') return (int8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
    return -1;
}

static void render_inline_hex_glyph(const char* hex, int16_t x, int16_t y) {
    // PICO-8's \^: command stores one byte per row, low bit on the left.
    // It is always an unpadded 8 by 8 glyph, independent of font scaling.
    for (int row = 0; row < 8; ++row) {
        int8_t hi = print_hex_nibble((uint8_t)hex[row * 2]);
        int8_t lo = print_hex_nibble((uint8_t)hex[row * 2 + 1]);
        if (hi < 0 || lo < 0) return;
        uint8_t bits = (uint8_t)((hi << 4) | lo);
        for (int col = 0; col < 8; ++col) {
            if ((bits & (1u << col)) != 0)
                guarded_put_pixel(x + col, y + row, drawstate.pen_color);
        }
    }
}

static inline void put_text_pixel_clipped(int x, int y, palidx_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    put_pixel_raw((uint8_t)x, (uint8_t)y, color);
}

static int p8_custom_font_width(uint8_t c, uint8_t width_override,
                                uint8_t width2_override,
                                uint8_t *vertical_adjust) {
    int width = c >= 128 ? width2_override : width_override;
    if (width == 0)
        width = p8_ram_read(c >= 128 ? 0x5601 : 0x5600);

    uint8_t adjustment = 0;
    if ((p8_ram_read(0x5605) & 1u) && c >= 16) {
        uint32_t index = (uint32_t)c - 16u;
        uint8_t packed = p8_ram_read(0x5608u + (index >> 1));
        adjustment = (index & 1u) ? (packed >> 4) : (packed & 0x0f);
        int delta = adjustment & 7u;
        if (delta >= 4) delta -= 8;
        width += delta;
    }

    if (vertical_adjust) *vertical_adjust = (adjustment & 8u) ? 1 : 0;
    return width > 0 ? width : 0;
}

// PICO-8 custom fonts live directly in RAM at 0x5600: eight little-endian
// bitmap rows per P8SCII character. Keep this separate from the default-font
// atlas so ordinary print() calls retain their existing fast path.
static int render_custom_text(uint8_t c, int16_t x, int16_t y,
                              uint8_t width_ratio, uint8_t height_ratio,
                              uint8_t width_override,
                              uint8_t width2_override,
                              uint8_t char_height, bool padding) {
    uint8_t vertical_adjust = 0;
    int advance = p8_custom_font_width(c, width_override, width2_override,
                                       &vertical_adjust);
    int cell_width = advance + (padding ? 1 : 0);
    int glyph_x = x + (int8_t)p8_ram_read(0x5603);
    int glyph_y = y + (int8_t)p8_ram_read(0x5604) - vertical_adjust;

    if (drawstate.bg_color) {
        for (int py = 0; py < (int)char_height * height_ratio; ++py) {
            for (int px = 0; px < cell_width * width_ratio; ++px)
                put_text_pixel_clipped(x + px, y + py, drawstate.bg_color);
        }
    }

    uint32_t glyph_addr = 0x5600u + (uint32_t)c * 8u;
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = p8_ram_read(glyph_addr + row);
        if (bits == 0) continue;
        for (int col = 0; col < 8; ++col) {
            if ((bits & (1u << col)) == 0) continue;
            for (int sy = 0; sy < height_ratio; ++sy) {
                for (int sx = 0; sx < width_ratio; ++sx) {
                    put_text_pixel_clipped(glyph_x + col * width_ratio + sx,
                                           glyph_y + row * height_ratio + sy,
                                           drawstate.pen_color);
                }
            }
        }
    }
    return cell_width * width_ratio;
}

static int render_print_glyph(uint8_t c, int16_t x, int16_t y,
                              bool custom_font, bool custom_padding,
                              uint8_t char_width, uint8_t char_height,
                              uint8_t custom_width,
                              uint8_t custom_width2,
                              uint8_t width_ratio,
                              uint8_t height_ratio) {
    if (custom_font) {
        return render_custom_text(c, x, y, width_ratio, height_ratio,
                                  custom_width, custom_width2, char_height,
                                  custom_padding);
    }

    render_text(&fontsheet, c, x, y, width_ratio, height_ratio);
    return (c >= 128 ? 8 : char_width) * width_ratio;
}

int16_t _print(const char* text, size_t textLen, int16_t x, int16_t y, int16_t paletteIdx) {
    // FIXME: this only works for ascii
    // FIXME: this should crop, and return the "cropped" number
    drawstate.pen_color = paletteIdx;

    int16_t print_x_offset = x;
    int16_t print_y_offset = y;
	int16_t print_home_x = x;
	int16_t print_home_y = y;
	int16_t rightmost_x = x;
    const uint8_t default_attributes = p8_ram_read(0x5f58);
    const bool default_attributes_enabled = (default_attributes & 1u) != 0;
    bool custom_font = default_attributes_enabled
                    && (default_attributes & 0x80u) != 0;
    bool custom_padding = default_attributes_enabled
                       && (default_attributes & 0x02u) != 0;
    uint8_t custom_width = p8_ram_read(0x5f59) & 0x0f;
    uint8_t custom_width2 = p8_ram_read(0x5f5a) & 0x0f;
	uint8_t char_width = 4;
	uint8_t char_height = custom_font
        ? ((p8_ram_read(0x5f59) >> 4) ? (p8_ram_read(0x5f59) >> 4)
                                      : p8_ram_read(0x5602))
        : 6;
	uint8_t tab_width = custom_font
        ? ((p8_ram_read(0x5f5a) >> 4) ? (p8_ram_read(0x5f5a) >> 4)
                                      : p8_ram_read(0x5606))
        : 16;
    if (char_height == 0) char_height = 8;
    if (tab_width == 0) tab_width = 16;
    int16_t print_width_ratio = 1;
    int16_t print_height_ratio = 1;
    if (default_attributes_enabled) {
        if (default_attributes & 0x04u) print_width_ratio = 2;
        if (default_attributes & 0x08u) print_height_ratio = 2;
    }
    int16_t old_pen;

	// P8SCII strings are also a compact binary-transfer mechanism. Dinky Kong
	// installs its complete custom font with a roughly 2 KB print string, so
	// an 8-bit cursor silently parsed only a modulo-256 fragment of the data.
	size_t i = 0;
	while (i<textLen) {
		bool printed_double_wide = false;
		uint8_t c = text[i];
        // FIXME: have to handle all control chars in a row
		switch(c) {
			case 0: // explicit end-of-print marker ("\\0")
				goto print_done;
			case 1: { // repeat next character P0 times ("\\*P0c")
				if (i + 2 >= textLen) goto print_done;
				uint8_t count = print_param((uint8_t)text[i + 1]);
				uint8_t repeated = (uint8_t)text[i + 2];
				if (repeated > 15) {
					for (uint8_t n = 0; n < count; ++n) {
						print_x_offset += render_print_glyph(
                            repeated, print_x_offset, print_y_offset,
                            custom_font, custom_padding, char_width,
                            char_height, custom_width, custom_width2,
                            print_width_ratio, print_height_ratio);
						if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
					}
				}
				i += 3;
				continue;
			}
			case 3: // shift cursor horizontally by P0 - 16 ("\\-P0")
				if (i + 1 >= textLen) goto print_done;
				print_x_offset += print_param((uint8_t)text[i + 1]) - 16;
				if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
				i += 2;
				continue;
			case 4: // shift cursor vertically by P0 - 16 ("\\|P0")
				if (i + 1 >= textLen) goto print_done;
				print_y_offset += print_param((uint8_t)text[i + 1]) - 16;
				i += 2;
				continue;
			case 5: // shift cursor by P0 - 16, P1 - 16 ("\\+P0P1")
				if (i + 2 >= textLen) goto print_done;
				print_x_offset += print_param((uint8_t)text[i + 1]) - 16;
				print_y_offset += print_param((uint8_t)text[i + 2]) - 16;
				if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
				i += 3;
				continue;
			case 6: // \^ change rendering modes
				if (i + 1 >= textLen) goto print_done;
				i++;
				c = text[i];
				if (c == ':') { // inline 8x8 glyph encoded as 16 hex digits
					if (i + 16 >= textLen) goto print_done;
					render_inline_hex_glyph(text + i + 1, print_x_offset,
					                        print_y_offset);
					print_x_offset += 8;
					if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
					i += 17;
					continue;
				}
					switch(c) {
					case 'w':
						print_width_ratio = 2; // FIXME, maybe *= 2?
						break;
					case 't':
						print_height_ratio = 2;
						break;
					case 'i': // inverted
						old_pen = drawstate.pen_color;
						drawstate.pen_color = drawstate.bg_color;
						drawstate.bg_color = old_pen;
						break;
                    case 'g': // home
                        print_x_offset = print_home_x;
                        print_y_offset = print_home_y;
                        break;
                    case 'h': // set home to the current cursor
                        print_home_x = print_x_offset;
                        print_home_y = print_y_offset;
                        break;
                    case 'c': // clear to P0 and move cursor/home to 0,0
						if (i + 1 >= textLen) goto print_done;
						gfx_cls(print_param((uint8_t)text[++i]));
						print_x_offset = print_home_x = 0;
						print_y_offset = print_home_y = 0;
                        break;
					case 'x': // set character advance width
						if (i + 1 >= textLen) goto print_done;
						char_width = print_param((uint8_t)text[++i]);
						if (char_width == 0) char_width = 1;
                        custom_width = char_width;
                        custom_width2 = char_width;
                        break;
                    case 'y': // set character/line height
						if (i + 1 >= textLen) goto print_done;
						char_height = print_param((uint8_t)text[++i]);
						if (char_height == 0) char_height = 1;
                        break;
                    case 's': // set tab-stop width
						if (i + 1 >= textLen) goto print_done;
						tab_width = print_param((uint8_t)text[++i]);
						if (tab_width == 0) tab_width = 1;
                        break;
					case 'd': // per-character delay (timing ignored)
					case 'r': // right-hand wrap boundary (not rendered yet)
						if (i + 1 >= textLen) goto print_done;
						i++;
                        break;
					case 'j': // jump to absolute P0*4, P1*4
						if (i + 2 >= textLen) goto print_done;
						print_x_offset = print_param((uint8_t)text[i + 1]) * 4;
						print_y_offset = print_param((uint8_t)text[i + 2]) * 4;
						i += 2;
						if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
                        break;
                    case '!': { // poke remaining bytes: \^!aaaabinary-data
                        // Four hexadecimal address digits are followed by
                        // bytes copied verbatim through the end of the print
                        // string. Snekburd uses this to switch 0x5f54 to its
                        // auxiliary sprite bank without a separate poke().
						if (i + 4 >= textLen) goto print_done;
                        uint32_t addr = 0;
                        for (int digit = 1; digit <= 4; ++digit) {
                            int8_t nibble = print_hex_nibble(
                                (uint8_t)text[i + digit]);
                            if (nibble < 0) goto print_done;
                            addr = (addr << 4) | (uint8_t)nibble;
                        }
						size_t payload = i + 5;
                        while (payload < textLen)
                            p8_ram_write(addr++, (uint8_t)text[payload++]);
                        i = textLen;
                        continue;
                    }
                    case '@': { // poke counted bytes: \^@aaaannnnbinary-data
						if (i + 8 >= textLen) goto print_done;
                        uint32_t addr = 0;
                        uint32_t count = 0;
                        for (int digit = 1; digit <= 8; ++digit) {
                            int8_t nibble = print_hex_nibble(
                                (uint8_t)text[i + digit]);
                            if (nibble < 0) goto print_done;
                            if (digit <= 4)
                                addr = (addr << 4) | (uint8_t)nibble;
                            else
                                count = (count << 4) | (uint8_t)nibble;
                        }
						size_t payload = i + 9;
						size_t available = textLen - payload;
						size_t copied = count < available ? count : available;
						for (size_t n = 0; n < copied; ++n)
                            p8_ram_write(addr + n,
                                         (uint8_t)text[payload + n]);
						i = payload + copied;
                        if (copied < count) goto print_done;
                        continue;
                    }
                    case '-': // disable something
						if (i + 1 >= textLen) goto print_done;
						i++;
                        c = text[i];
                        switch(c) {
							case 'w':
								print_width_ratio = 1;
								break;
							case 't':
								print_height_ratio = 1;
								break;
                            case 'i': // inverted
                                // FIXME probably not how it should be done
                                old_pen = drawstate.pen_color;
                                drawstate.pen_color = drawstate.bg_color;
                                drawstate.bg_color = old_pen;
                                break;
                            default:
                                // Unsupported print attributes are silent in
                                // PICO-8.  Some carts emit them per glyph, so
                                // logging here can flood UART and dominate a
                                // frame without providing actionable output.
                                break;
                        }
                        break;
				}
				i++;
				continue;
			case 8: // backspace
				print_x_offset -= custom_font
                    ? (p8_custom_font_width(' ', custom_width,
                                            custom_width2, NULL)
                       + (custom_padding ? 1 : 0)) * print_width_ratio
                    : char_width * print_width_ratio;
				i++;
				continue;
			case 9: { // tab to the next four-character boundary
				int relative_x = print_x_offset - print_home_x;
				int remainder = relative_x % tab_width;
				if (remainder < 0) remainder += tab_width;
				print_x_offset += tab_width - remainder;
				if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
				i++;
				continue;
			}
			case '\n':
                print_x_offset = print_home_x;
                print_y_offset += char_height * print_height_ratio;
				i++;
				continue;
			case 11: // decoration is not rendered yet; consume P0 and glyph
				i = (i + 2 < textLen) ? i + 3 : textLen;
				continue;
			case '\f':
				if (i + 1 >= textLen) goto print_done;
				drawstate.pen_color = print_param((uint8_t)text[i + 1]);
				i += 2;
				continue;
			case '\r': // carriage return
				print_x_offset = print_home_x;
				i++;
				continue;
			case 14: // switch to font stored at 0x5600
                custom_font = true;
                char_height = (p8_ram_read(0x5f59) >> 4)
                            ? (p8_ram_read(0x5f59) >> 4)
                            : p8_ram_read(0x5602);
                tab_width = (p8_ram_read(0x5f5a) >> 4)
                          ? (p8_ram_read(0x5f5a) >> 4)
                          : p8_ram_read(0x5606);
                if (char_height == 0) char_height = 8;
                if (tab_width == 0) tab_width = 16;
				i++;
				continue;
			case 15: // switch back to the built-in font
                custom_font = false;
                char_width = 4;
                char_height = 6;
                tab_width = 16;
				i++;
				continue;
			case 0x2: // \#
				if (i + 1 >= textLen) goto print_done;
				drawstate.bg_color = print_param((uint8_t)text[i + 1]);
				i += 2;
				continue;
			case 0xe2: 
				i++;
				c = text[i];
				switch(c) {
					case 0x9d:// ❎ = 0xe2 0x9d 0x8e
						printed_double_wide = true;
						c = 151; // X in font
						i += 1;
						break;
					case 0x99:// ❤️ = 0xe2 0x99 0xa5
						printed_double_wide = true;
						c = 135; // heart in font
						i += 1;
						break;
					case 0xac:// U/L/D
						i++;
						c = text[i];
						switch (c) {
							case 0x86: // U
								printed_double_wide = true;
								c = 9*16+4; // UP
								i += 3;
								break;
							case 0x85: // L
								printed_double_wide = true;
								c = 8*16+11; // L
								i += 3;
								break;
							case 0x87: // D
								printed_double_wide = true;
								c = 8*16+3; // D
								i += 3;
								break;
						}
						break;
					case 0x9e: // RIGHT = 0xe2 0x9e +4
						printed_double_wide = true;
						c = 9*16+1; // RIGHT
						i += 4;
						break;
				}
				break;
			case 0xf0: // 🅾  = 0xf0 0x9f 0x85 0xbe
				printed_double_wide = true;
				c = 142; // "circle" in font (square)
				i += 6;
				break;
		}
		if (c > 15) {
			print_x_offset += render_print_glyph(
                c, print_x_offset, print_y_offset, custom_font,
                custom_padding, char_width, char_height, custom_width,
                custom_width2, print_width_ratio, print_height_ratio);
			if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
			i++;
		} else {
			// Unsupported control codes must still make progress. The imported
			// renderer previously left i unchanged here and locked the whole
			// emulator on otherwise valid strings (Pizza Panda uses \\- and \\|
			// cursor controls in its first frame).
			i++;
		}
		if (printed_double_wide && c < 128 && !custom_font) {
			print_x_offset += (char_width * print_width_ratio);
			if (print_x_offset > rightmost_x) rightmost_x = print_x_offset;
		}
	}

print_done:
	drawstate.bg_color = 0;
	return rightmost_x;
}

enum {
    LINE_LEFT = 1,
    LINE_RIGHT = 2,
    LINE_TOP = 4,
    LINE_BOTTOM = 8
};

static int line_outcode(int64_t x, int64_t y,
                        int left, int top, int right, int bottom) {
    int code = 0;
    if (x < left) code |= LINE_LEFT;
    else if (x > right) code |= LINE_RIGHT;
    if (y < top) code |= LINE_TOP;
    else if (y > bottom) code |= LINE_BOTTOM;
    return code;
}

// Clip before Bresenham. Perspective carts can generate endpoints tens of
// thousands of pixels off screen as a vertex crosses the near plane; walking
// that invisible distance can otherwise monopolize the emulator task.
static bool clip_line_to_draw(int32_t *ax, int32_t *ay,
                              int32_t *bx, int32_t *by) {
    const int left = MAX(0, (int)drawstate.clip_x);
    const int top = MAX(0, (int)drawstate.clip_y);
    const int right = MIN(SCREEN_WIDTH - 1,
        (int)drawstate.clip_x + drawstate.clip_w - 1);
    const int bottom = MIN(SCREEN_HEIGHT - 1,
        (int)drawstate.clip_y + drawstate.clip_h - 1);
    if (left > right || top > bottom) return false;

    int64_t x0 = *ax, y0 = *ay, x1 = *bx, y1 = *by;
    for (int pass = 0; pass < 8; ++pass) {
        const int c0 = line_outcode(x0, y0, left, top, right, bottom);
        const int c1 = line_outcode(x1, y1, left, top, right, bottom);
        if (!(c0 | c1)) {
            *ax = (int32_t)x0; *ay = (int32_t)y0;
            *bx = (int32_t)x1; *by = (int32_t)y1;
            return true;
        }
        if (c0 & c1) return false;

        const int code = c0 ? c0 : c1;
        int64_t x, y;
        if (code & LINE_TOP) {
            if (y1 == y0) return false;
            y = top;
            x = x0 + (x1 - x0) * (top - y0) / (y1 - y0);
        } else if (code & LINE_BOTTOM) {
            if (y1 == y0) return false;
            y = bottom;
            x = x0 + (x1 - x0) * (bottom - y0) / (y1 - y0);
        } else if (code & LINE_RIGHT) {
            if (x1 == x0) return false;
            x = right;
            y = y0 + (y1 - y0) * (right - x0) / (x1 - x0);
        } else {
            if (x1 == x0) return false;
            x = left;
            y = y0 + (y1 - y0) * (left - x0) / (x1 - x0);
        }
        if (code == c0) { x0 = x; y0 = y; }
        else { x1 = x; y1 = y; }
    }
    return false;
}

// Bresenham line algorithm, after clipping to at most the visible 128 pixels.
void gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const palidx_t color) {
    if (!clip_line_to_draw(&x0, &y0, &x1, &y1)) return;

    int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, e2; /* error value e_xy */

    for (;;){  /* loop */
        guarded_put_pixel(x0,y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}
int _lua_print(lua_State* L) {
    const int argcount = lua_gettop(L);
    size_t textLen = 0;
    const char* text;
    switch (lua_type(L, 1)) {
        case LUA_TSTRING:
        case LUA_TNUMBER:
            // lua_tolstring uses the VM's fixed-point number formatting.
            text = lua_tolstring(L, 1, &textLen);
            break;
        case LUA_TNIL:
            text = "[nil]";
            textLen = 5;
            break;
        case LUA_TBOOLEAN:
            text = lua_toboolean(L, 1) ? "true" : "false";
            textLen = strlen(text);
            break;
        case LUA_TTABLE:
            // PICO-8 print()/tostr() observe a table's __tostring method.
            if (luaL_callmeta(L, 1, "__tostring")) {
                text = luaL_tolstring(L, -1, &textLen);
            } else {
                text = "[table]";
                textLen = 7;
            }
            break;
        case LUA_TNONE:
            text = "";
            textLen = 0;
            break;
        default: {
            char typeText[24];
            snprintf(typeText, sizeof(typeText), "[%s]", luaL_typename(L, 1));
            lua_pushstring(L, typeText);
            text = lua_tolstring(L, -1, &textLen);
            break;
        }
    }
    const bool has_xy = argcount >= 3
                     && !lua_isnoneornil(L, 2)
                     && !lua_isnoneornil(L, 3);
    const int16_t x = has_xy ? p8_checkinteger(L, 2)
                             : drawstate.cursor_x;
    const int16_t y = has_xy ? p8_checkinteger(L, 3)
                             : drawstate.cursor_y;
    int16_t paletteIdx = drawstate.pen_color;
    int color_arg = 0;
    if (argcount == 2) color_arg = 2;       // print(text, colour)
    else if (argcount >= 4) color_arg = 4;  // print(text, x, y, colour)
    if (color_arg != 0) {
        // PICO-8 distinguishes an omitted colour (current pen) from an
        // explicitly supplied nil (numeric colour zero). Islander's outline
        // helper intentionally forwards a nil fourth argument for black.
        paletteIdx = lua_isnil(L, color_arg)
                   ? 0 : p8_checkinteger(L, color_arg);
    }

    int16_t rightmost_x = _print(text, textLen,
                                 x-drawstate.camera_x,
                                 y-drawstate.camera_y, paletteIdx);
    ram[0x5f25] = drawstate.pen_color;
    // PICO-8 returns the right-most x coordinate reached while printing.
    // Convert the renderer's screen coordinate back into cart coordinates;
    // carts use this result to measure styled text off-screen.
    lua_pushinteger(L, rightmost_x + drawstate.camera_x);

    // With no explicit position, PICO-8 prints at the current cursor and
    // advances it by one character row. Explicit-position measurements and
    // labels do not disturb the implicit cursor.
    if (!has_xy) {
        int next_y = y + 6;
        if (next_y > SCREEN_HEIGHT - 6) next_y = SCREEN_HEIGHT - 6;
        drawstate.cursor_y = (uint8_t)next_y;
        ram[0x5f27] = drawstate.cursor_y;
    }
    // A cart can call flip() early in _init(), then draw a loading logo/text
    // before doing a long procedural-generation pass. Native PICO-8 exposes
    // those subsequent framebuffer writes while Retro-Go's copied surface
    // would otherwise remain on the pre-message (often blank) frame. Present
    // the first complete text-bearing init frame once; normal gameplay and
    // later explicit flip loops retain the existing zero-overhead path.
    if (engine_init_followup_pending) {
        gfx_flip();
        engine_init_followup_pending = false;
        engine_init_followup_presented = true;
        engine_init_progress_last_ms = now();
    }
    return 1;
}

int _lua_palt(lua_State* L) {
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) {
        // reset for all colors
        reset_transparency();
        return 0;
    }
    if (argcount == 1) {
        uint16_t bitfield = p8_checkinteger(L, 1);
        for(uint8_t idx = 0; idx < 16; idx++) {
            // Packed palt() is written in colour order, starting at the
            // highest bit: palt(0b1100...) makes colours 0 and 1 transparent.
            // Snekburd relies on this ordering for its generated island art.
            drawstate.transparent[idx] =
                (bitfield & (uint16_t)(0x8000u >> idx)) != 0;
            ram[0x5f00 + idx] = (uint8_t)((pal_map[idx] & 0x0f)
                                 | (drawstate.transparent[idx] ? 0x10 : 0));
        }
        return 0;
    }
    uint8_t idx = p8_checkinteger(L, 1);
    // Bounds-check: drawstate.transparent is 16 entries. A cart calling
    // palt(16..255, true) would OOB-write past the array into adjacent
    // drawstate fields, corrupting clip/camera/pen state.
    if (idx >= 16) return 0;
    bool transparent = lua_toboolean(L, 2);
    drawstate.transparent[idx] = transparent;
    ram[0x5f00 + idx] = (uint8_t)((pal_map[idx] & 0x0f)
                         | (transparent ? 0x10 : 0));

    return 0;
}

static const color_t p8_extended_palette[16] = {
    to_rgb565(0x29, 0x18, 0x14), to_rgb565(0x11, 0x1d, 0x35),
    to_rgb565(0x42, 0x21, 0x36), to_rgb565(0x12, 0x53, 0x59),
    to_rgb565(0x74, 0x2f, 0x29), to_rgb565(0x49, 0x33, 0x3b),
    to_rgb565(0xa2, 0x88, 0x79), to_rgb565(0xf3, 0xef, 0x7d),
    to_rgb565(0xbe, 0x12, 0x50), to_rgb565(0xff, 0x6c, 0x24),
    to_rgb565(0xa8, 0xe7, 0x2e), to_rgb565(0x00, 0xb5, 0x43),
    to_rgb565(0x06, 0x5a, 0xb5), to_rgb565(0x75, 0x46, 0x65),
    to_rgb565(0xff, 0x6e, 0x59), to_rgb565(0xff, 0x9d, 0x81),
};

static inline color_t p8_display_color(int value) {
    uint8_t index = (uint8_t)value & 0x8f;
    return (index & 0x80) ? p8_extended_palette[index & 0x0f]
                          : original_palette[index & 0x0f];
}

static void reset_draw_palette(void) {
    memcpy(pal_map, orig_pal_map, sizeof(orig_pal_map));
    reset_transparency();
}

static void reset_display_palette(void) {
    memcpy(palette, original_palette, sizeof(original_palette));
    for (int i = 0; i < 16; ++i) ram[0x5f10 + i] = (uint8_t)i;
}

static void reset_secondary_palette(void) {
    // Each entry packs the colour for a zero pattern bit in the low nibble
    // and the colour for a one bit in the high nibble. PICO-8's system
    // default is black for both halves of every entry, not an identity map.
    // Into Ruins intentionally supplies only colours 1..14 to pal(table, 2);
    // leaving colours 0 and 15 as identity pairs makes every hidden cave
    // tile retain its colour-15 pixels and cover the player.
    for (int i = 0; i < 16; ++i)
        ram[0x5f60 + i] = 0;
}

static void reset_all_palettes(void) {
    reset_draw_palette();
    reset_display_palette();
    reset_secondary_palette();
}

static void _replace_palette(uint8_t palIdx, lua_State* L) {
    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, 1);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2))
    {
        // stack now contains: -1 => value; -2 => key; -3 => table
        const int value = p8_checkinteger(L, -1);
        const int key = p8_checkinteger(L, -2);
        // Array-style palette tables use keys 1..16; key 16 represents
        // colour zero. Sparse tables may also explicitly use key zero.
        if (key >= 0 && key <= 16) {
            const uint8_t idx = (uint8_t)key & 0x0f;
            if (palIdx == 0) {
                pal_map[idx] = (uint8_t)value & 0x0f;
                ram[0x5f00 + idx] = (uint8_t)((pal_map[idx] & 0x0f)
                                     | (drawstate.transparent[idx]
                                        ? 0x10 : 0));
            } else if (palIdx == 1) {
                palette[idx] = p8_display_color(value);
                ram[0x5f10 + idx] = (uint8_t)value & 0x8f;
            } else if (palIdx == 2) {
                ram[0x5f60 + idx] = (uint8_t)value;
            }
        }
        // pop value, leaving original key
        lua_pop(L, 1);
        // stack now contains: -1 => key; -2 => table
    }
    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
}

int _lua_pal(lua_State* L) {
    // pal(c0,c1,p): p=0 draw palette, p=1 display palette, p=2 secondary.
    // Palette 2 stores packed two-colour entries used by sprite fill patterns.
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) {
        reset_all_palettes();
        return 0;
    }
    if (lua_istable(L, 1)) {
        uint8_t palIdx = p8_optinteger(L, 2, 0);
        if (palIdx <= 2) _replace_palette(palIdx, L);
        return 0;
    }

    // If c0 itself is nil/missing, behave like pal() (full reset) rather than
    // raising a Lua error (some carts call pal(maybe_nil) every frame).
    if (lua_isnoneornil(L, 1)) {
        reset_all_palettes();
        return 0;
    }

    // Captain Neat-O In The Time Nexus (.p8.png) calls `pal(some_bool, ...)`
    // at cart `_draw` line 140 every frame. PICO-8 doesn't define this case,
    // so `p8_optinteger(L, 1, 0)` raises `bad argument #1 to 'pal'
    // (number expected, got boolean)` and the cart spams the serial log
    // 60 Hz. Catch boolean arg 1 explicitly and degrade to a silent no-op
    // (palette unchanged) — same "leave state alone" semantics PICO-8 carts
    // expect when the third arg is missing.
    if (lua_isboolean(L, 1)) {
        return 0;
    }

    // With one numeric argument, reset that complete palette.
    if (argcount == 1) {
        int palIdx = p8_checkinteger(L, 1);
        if (palIdx == 0) reset_draw_palette();
        else if (palIdx == 1) reset_display_palette();
        else if (palIdx == 2) reset_secondary_palette();
        return 0;
    }

    int origIdx = p8_checkinteger(L, 1);
    int newIdx = p8_optinteger(L, 2, origIdx);
    int palIdx = p8_optinteger(L, 3, 0);
    if (origIdx < 0 || origIdx >= 16) return 0;

    if (palIdx == 0) {
        // Since PICO-8 0.2.0, state-setting graphics APIs return their
        // previous state. Carts use this to make a temporary remap without
        // allocating a hard-coded inverse palette:
        //   old[c] = pal(c, outline)
        //   ...draw...
        //   pal(old)
        // Crowded Dungeon relies on this for item and cursor interiors.
        const uint8_t previous = pal_map[origIdx] & 0x0f;
        pal_map[origIdx] = (uint8_t)newIdx & 0x0f;
        ram[0x5f00 + origIdx] = (uint8_t)((pal_map[origIdx] & 0x0f)
                                 | (drawstate.transparent[origIdx]
                                    ? 0x10 : 0));
        lua_pushinteger(L, previous);
        return 1;
    } else if (palIdx == 1) {
        const uint8_t previous = ram[0x5f10 + origIdx] & 0x8f;
        palette[origIdx] = p8_display_color(newIdx);
        ram[0x5f10 + origIdx] = (uint8_t)newIdx & 0x8f;
        lua_pushinteger(L, previous);
        return 1;
    } else if (palIdx == 2) {
        const uint8_t previous = ram[0x5f60 + origIdx];
        ram[0x5f60 + origIdx] = (uint8_t)newIdx;
        lua_pushinteger(L, previous);
        return 1;
    }
    return 0;
}

inline void cls(uint8_t palIdx = 0) {
    gfx_cls(palIdx);
}
int _lua_cls(lua_State* L) {
    uint8_t palIdx = p8_optinteger(L, 1, 0);
    cls(palIdx);
    return 0;
}

int _lua_sspr(lua_State* L) {
    // sspr( sx, sy, sw, sh, dx, dy, [dw,] [dh,] [flip_x,] [flip_y] )
    int sx = p8_optinteger(L, 1, 0);
    int sy = p8_checkinteger(L, 2);
    int sw = p8_checkinteger(L, 3);
    int sh = p8_checkinteger(L, 4);
    int dx = p8_checkinteger(L, 5);
    int dy = p8_checkinteger(L, 6);
    int dw = p8_optinteger(L, 7, sw);
    int dh = p8_optinteger(L, 8, sh);
    bool flip_x = lua_toboolean(L, 9);
    bool flip_y = lua_toboolean(L, 10);
    render_stretched(&spritesheet, sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);
    return 0;
}

// Forward declaration for `render_many` — its `inline` body lives near the
// end of this file, but `spr()` (next function) calls into it. Without this
// forward decl, the compiler fails with "'render_many' was not declared in
// this scope" at the line inside spr()'s body that calls it. engine.h's
// earlier prototype was removed in round 14 to keep C consumers (which can't
// see `z8::fix32`) away from this header, so the prototype must live here
// next to the .c file instead.
inline void render_many(Spritesheet* s, uint16_t n, int16_t x0, int16_t y0, int paletteIdx, bool flip_x, bool flip_y, z8::fix32 width, z8::fix32 height);

inline void spr(uint16_t n, z8::fix32 x, z8::fix32 y, z8::fix32 w = z8::fix32(1.0f), z8::fix32 h = z8::fix32(1.0f), bool flip_x = false, bool flip_y = false) {
    render_many(&spritesheet, n, (int16_t)x, (int16_t)y, -1, flip_x, flip_y, w, h);
}

int _lua_spr(lua_State* L) {
    uint8_t argcount = lua_gettop(L);
    if (argcount < 3)
        return 0;

    int n = p8_optinteger(L, 1, -1);
    if (n==-1)
        return 0;
    int x = p8_checkinteger(L, 2);
    int y = p8_checkinteger(L, 3);
    // PICO-8 accepts fractional sprite dimensions. Carts use this to draw
    // partial tiles; Dinky Kong's five-pixel-high platforms are emitted as
    // spr(..., width, 5/8). Truncating through p8_optinteger turned that
    // height into zero even though _render already handles fix32 dimensions.
    z8::fix32 w = p8_optnumber(L, 4, z8::fix32(int32_t{1}));
    z8::fix32 h = p8_optnumber(L, 5, z8::fix32(int32_t{1}));

    bool flip_x = false;
    bool flip_y = false;

    if (argcount >= 6)
        flip_x = lua_toboolean(L, 6);
    if (argcount >= 7)
        flip_y = lua_toboolean(L, 7);

    // `spr()` takes (uint16_t, fix32, fix32, ...). The locals `n`, `x`, `y`
    // are `int`; cast through fix32(int32_t) to make the implicit
    // conversion unambiguous in C++.
    spr((uint16_t)n, z8::fix32(int32_t{x}), z8::fix32(int32_t{y}), w, h, flip_x, flip_y);

    return 0;
}

int _lua_line(lua_State* L) {
    const int argcount = lua_gettop(L);

    // line() clears the implicit endpoint used by the shorthand form.
    if (argcount == 0) {
        drawstate.line_active = 0;
        return 0;
    }

    // PICO-8 accepts line(x, y [, col]) as a continuation from the endpoint
    // of the previous line. The first shorthand call after line() only seeds
    // that endpoint and does not draw anything.
    if (argcount == 2 || argcount == 3) {
        int16_t x1 = p8_checkinteger(L, 1);
        int16_t y1 = p8_checkinteger(L, 2);
        int col = p8_optinteger(L, 3, drawstate.pen_color);
        drawstate.pen_color = col;

        if (drawstate.line_active) {
            gfx_line(drawstate.line_x - drawstate.camera_x,
                     drawstate.line_y - drawstate.camera_y,
                     x1 - drawstate.camera_x,
                     y1 - drawstate.camera_y,
                     col);
        }

        drawstate.line_x = x1;
        drawstate.line_y = y1;
        drawstate.line_active = 1;
        return 0;
    }

    // A malformed one-argument call has no useful endpoint pair.
    if (argcount < 4)
        return 0;

    int16_t x0 = p8_checkinteger(L, 1);
    int16_t y0 = p8_checkinteger(L, 2);
    int16_t x1 = p8_checkinteger(L, 3);
    int16_t y1 = p8_checkinteger(L, 4);
    int col = p8_optinteger(L, 5, drawstate.pen_color);
    drawstate.pen_color = col;
    drawstate.line_x = x1;
    drawstate.line_y = y1;
    drawstate.line_active = 1;
    gfx_line(x0-drawstate.camera_x, y0-drawstate.camera_y, x1-drawstate.camera_x, y1-drawstate.camera_y, col);
    return 0;
}

int _lua_rect(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    int16_t x2 = p8_checkinteger(L, 3);
    int16_t y2 = p8_checkinteger(L, 4);
    int col = p8_optinteger(L, 5, drawstate.pen_color);
    drawstate.pen_color = col;
	    
    gfx_rect(x-drawstate.camera_x, y-drawstate.camera_y, x2-drawstate.camera_x, y2-drawstate.camera_y, col);
    return 0;
}

int _lua_rectfill(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    int16_t x2 = p8_checkinteger(L, 3);
    int16_t y2 = p8_checkinteger(L, 4);
    int col = p8_optinteger(L, 5, drawstate.pen_color);
    drawstate.pen_color = col;

    gfx_rectfill(x-drawstate.camera_x, y-drawstate.camera_y, x2-drawstate.camera_x, y2-drawstate.camera_y, col);
    return 0;
}

// Numeric hot path used by OP_CALL in lvm.cpp. Keep all renderer state here
// so the shortcut is exactly the ordinary rectfill() operation after Lua
// argument conversion, including camera, clipping, palette, and fillp().
void p8_rectfill_fast_numeric(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              int has_color, int color) {
    const int col = has_color ? color : drawstate.pen_color;
    drawstate.pen_color = col;
    gfx_rectfill(x0 - drawstate.camera_x, y0 - drawstate.camera_y,
                 x1 - drawstate.camera_x, y1 - drawstate.camera_y, col);
}

int _lua_circ(lua_State* L) {
    int x = p8_checkinteger(L, 1);
    int y = p8_checkinteger(L, 2);
    int r = p8_optinteger(L, 3, 4);
    int col = p8_optinteger(L, 4, drawstate.pen_color);
    drawstate.pen_color = col;

    gfx_circle(x-drawstate.camera_x, y-drawstate.camera_y, r, col);
    return 0;
}

int _lua_oval(lua_State* L) {
    int x0 = p8_checkinteger(L, 1);
    int y0 = p8_checkinteger(L, 2);
    int x1 = p8_checkinteger(L, 3);
    int y1 = p8_checkinteger(L, 4);
    int col = p8_optinteger(L, 5, drawstate.pen_color);
    drawstate.pen_color = col;

    gfx_oval(x0-drawstate.camera_x, y0-drawstate.camera_y, x1-drawstate.camera_x, y1-drawstate.camera_y, col);
    return 0;
}
int _lua_ovalfill(lua_State* L) {
    int x0 = p8_checkinteger(L, 1);
    int y0 = p8_checkinteger(L, 2);
    int x1 = p8_checkinteger(L, 3);
    int y1 = p8_checkinteger(L, 4);
    int col = p8_optinteger(L, 5, drawstate.pen_color);
    drawstate.pen_color = col;

    gfx_ovalfill(x0-drawstate.camera_x, y0-drawstate.camera_y, x1-drawstate.camera_x, y1-drawstate.camera_y, col);
    return 0;
}

int _lua_circfill(lua_State* L) {
    int x = p8_checkinteger(L, 1);
    int y = p8_checkinteger(L, 2);
    int r = p8_optinteger(L, 3, 4);
    int col = p8_optinteger(L, 4, drawstate.pen_color);
    drawstate.pen_color = col;

    gfx_circlefill(x-drawstate.camera_x, y-drawstate.camera_y, r, col);
    return 0;
}

int _lua_map(lua_State* L) {
    P8MapLayout layout;
    if (!p8_map_layout(&layout)) return 0;

    int mapX = p8_optinteger(L, 1, 0);
    int mapY = p8_optinteger(L, 2, 0);
    int screenX = p8_optinteger(L, 3, 0);
    int screenY = p8_optinteger(L, 4, 0);
    // PICO-8 defaults omitted dimensions to the whole active map. Bounds and
    // screen clipping in map() keep this inexpensive for MAP() and wrapped
    // maps that begin near an edge.
    int cellW = p8_optinteger(L, 5, layout.width);
    int cellH = p8_optinteger(L, 6, layout.capacity / layout.width);
    uint32_t layerFlags = p8_optinteger(L, 7, 0x0);

    // Remapped maps can be wider/taller than the legacy 128x64 layout. The
    // renderer bounds each request against the active mapping's capacity.
    if (cellW <= 0 || cellH <= 0) return 0;
    cellW = MIN(cellW, 256);
    cellH = MIN(cellH, 256);
    map(mapX, mapY, screenX, screenY, cellW, cellH, layerFlags);
    return 0;
}

uint8_t btn(lua_State* L, uint8_t* _buttons) {
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) {
	uint8_t bitfield = 0;
	for(uint8_t i=0; i<7; i++) {
	    bitfield |= ((_buttons[i]) << i);
	}
    	return bitfield;
    } else if (argcount >= 1) {
    	int idx = p8_optinteger(L, 1, -1);
	if(idx==-1) return 0;
	// Optional 2nd arg is the player index. We only emulate player 0
	// (PICO-8's primary player). Player 1+ has no input mapped.
	if (argcount >= 2) {
	    int player = p8_optinteger(L, 2, 0);
	    if (player != 0) return 0;
	}
	if (idx < 0 || idx >= 7) return 0;
    	return _buttons[idx];
    }
    return 0;
}
int _lua_btnp(lua_State* L) {
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) {
        lua_pushinteger(L, btn(L, buttons_frame));
        return 1;
    }
    lua_pushboolean(L, btn(L, buttons_frame));
    return 1;
}
int _lua_btn(lua_State* L) {
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) {
        lua_pushinteger(L, btn(L, buttons));
        return 1;
    }
    lua_pushboolean(L, btn(L, buttons));
    return 1;
}

// Refresh the controller state immediately. PICO-8 exposes this internal
// helper to carts which run modal input loops/coroutines between normal
// update ticks. Besides retaining the currently-held btn() state, the
// backend's transition tracking consumes the btnp() edge which entered the
// modal step, so the same press cannot spill into its next screen.
//
// This is deliberately demand-driven: ordinary carts retain the single
// input poll per emulated tick and pay no additional cost.
int _lua_update_buttons(lua_State* L) {
    (void)L;
    handle_input();
    return 0;
}

// PICO-8 exposes its two-word PRNG state at 0x5f44..0x5f4b. Keep RAM as the
// source of truth so peek(), poke(), memcpy(), and cart chaining can inspect
// or restore the sequence. These aligned memcpy operations compile to native
// 32-bit loads/stores on ESP32 while avoiding strict-aliasing violations.
static inline uint32_t p8_rng_load(uint32_t address) {
    uint32_t value;
    memcpy(&value, ram + address, sizeof(value));
    return value;
}

static inline void p8_rng_store(uint32_t address, uint32_t value) {
    memcpy(ram + address, &value, sizeof(value));
}

static inline uint32_t p8_rng_step_words(uint32_t *a, uint32_t *b) {
    *a = ((*a >> 16) | (*a << 16)) + *b;
    *b += *a;
    return *a;
}

static inline uint32_t p8_rng_next(void) {
    uint32_t a = p8_rng_load(0x5f44);
    uint32_t b = p8_rng_load(0x5f48);
    p8_rng_step_words(&a, &b);
    p8_rng_store(0x5f44, a);
    p8_rng_store(0x5f48, b);
    return a;
}

static void p8_rng_seed_bits(uint32_t seed) {
    // PICO-8 discards the sign bit, substitutes a fixed non-zero seed for
    // zero, then warms the generator for 32 rounds.
    seed &= 0x7fffffffu;
    uint32_t b = seed ? seed : 0xdeadbeefu;
    uint32_t a = b ^ 0xbead29bau;
    for (int i = 0; i < 32; ++i)
        p8_rng_step_words(&a, &b);
    p8_rng_store(0x5f44, a);
    p8_rng_store(0x5f48, b);
}

int _lua_srand(lua_State* L) {
    // The seed is the complete 16.16 bit pattern, including its fraction.
    // Into Ruins intentionally uses 0x5b04.17cb for a deterministic map.
    const lua_Number seed = p8_checknumber(L, 1);
    p8_rng_seed_bits((uint32_t)seed.bits());
    return 0;
}
int _lua_rnd(lua_State* L) {
    if(lua_istable(L, 1)) {
        size_t len = lua_rawlen(L, 1);
        if (len == 0) {
            lua_pushnil(L);
            return 1;
        }
        // PICO-8 implements rnd(table) as table[flr(rnd(#table)) + 1].
        // Use the same fixed-point range so seeded procedural carts consume
        // exactly one PRNG value and choose the same element as the console.
        const uint32_t range = (uint32_t)len << 16;
        const size_t choice = (size_t)((p8_rng_next() % range) >> 16);
        lua_rawgeti(L, 1, (lua_Integer)choice + 1);
	return 1;
    }
    lua_Number limit = p8_optnumber(L, 1, lua_Number(int32_t{1}));
    const uint32_t range = (uint32_t)limit.bits();
    // rnd(0) returns zero without advancing the generator. For all other
    // values PICO-8 uses an unsigned modulo of the raw 16.16 range.
    const uint32_t result = range ? p8_rng_next() % range : 0;
    lua_pushnumber(L, lua_Number::frombits((int32_t)result));
    return 1;
}

inline uint8_t _sget(int16_t x, int16_t y) {
    if (x < 0 || x > 127 || y < 0 || y > 127)
        return 0;
    return spritesheet.sprite_data[y*128+x];
}
int _lua_sset(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    int16_t c = p8_optinteger(L, 3, drawstate.pen_color);
	if(x>=0 && x<128 && y>=0 && y<128) {
		uint32_t addr = (uint32_t)y * 64 + (uint32_t)x / 2;
		uint8_t value = p8_ram_read(addr);
		if (x & 1) value = (uint8_t)((value & 0x0f) | ((c & 0x0f) << 4));
		else value = (uint8_t)((value & 0xf0) | (c & 0x0f));
		p8_ram_write(addr, value);
	}
    return 0;
}
int _lua_sget(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);

    lua_pushinteger(L, _sget(x, y));
    return 1;
}

int _lua_fget(lua_State* L) {
    int n = p8_checkinteger(L, 1);

    // `spritesheet.flags` is 256 bytes — clamp `n` so an oversized cart
    // sprite number doesn't OOB-read into adjacent BSS (same crash class
    // as the map() case above).
    uint8_t p = (n >= 0 && n < 256) ? spritesheet.flags[n] : 0;
    // PICO-8 distinguishes an omitted second argument from an explicitly
    // supplied nil. fget(n) returns the complete bitfield, while
    // fget(n, nil) coerces nil to flag index 0 and returns a boolean. This
    // matters for wrappers such as `fget(mget(x,y), optional_flag)`:
    // Pigments relies on the nil case being a false boolean for unflagged
    // floor tiles. Treating it as numeric bitfield 0 makes every such tile
    // truthy under Lua's boolean rules.
    if (lua_gettop(L) < 2) {
        lua_pushinteger(L, p);
    } else {
        int bitfield = p8_checkinteger(L, 2);
        bool result = bitfield >= 0 && bitfield < 8
                   && ((1u << bitfield) & p) != 0;
        lua_pushboolean(L, result);
    }
    return 1;
}

int _lua_fset(lua_State* L) {
    int n = p8_checkinteger(L, 1);
    if (n < 0 || n >= 256) return 0;

    if (lua_gettop(L) < 3) {
        spritesheet.flags[n] = (uint8_t)p8_checkinteger(L, 2);
    } else {
        int bit = p8_checkinteger(L, 2);
        bool enabled = lua_toboolean(L, 3);
        if (bit >= 0 && bit < 8) {
            uint8_t mask = (uint8_t)(1u << bit);
            if (enabled) spritesheet.flags[n] |= mask;
            else spritesheet.flags[n] &= (uint8_t)~mask;
        }
    }
    p8_ram_write(0x3000u + (uint32_t)n, spritesheet.flags[n]);
    return 0;
}

int _lua_mset(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    uint8_t n = p8_checkinteger(L, 3);
    P8MapLayout layout;
    uint32_t addr;
    if (!p8_map_layout(&layout) || !p8_map_address(&layout, x, y, &addr))
        return 0;
    p8_ram_write(addr, n);
    return 0;
}

int _lua_mget(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    P8MapLayout layout;
    uint32_t addr;
    if (!p8_map_layout(&layout) || !p8_map_address(&layout, x, y, &addr)) {
        lua_pushinteger(L, 0);
        return 1;
    }
    uint8_t p = layout.default_layout ? map_data[y * 128 + x]
                                      : p8_ram_read(addr);
    lua_pushinteger(L, p);
    return 1;
}

int _lua_pget(lua_State* L) {
    // pget() observes the same camera transform as pset() and the drawing
    // primitives. This is more than a visual detail: carts render masks to
    // an off-origin world position and immediately read them back. Keeping
    // the inputs signed is essential for that pattern (Driftmania builds its
    // rotated car hitboxes around world coordinate 0 with camera(-64,-64)).
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    int16_t tx = x - drawstate.camera_x;
    int16_t ty = y - drawstate.camera_y;
    uint16_t p = (tx < 0 || tx >= SCREEN_WIDTH ||
                  ty < 0 || ty >= SCREEN_HEIGHT)
               ? 0 : get_pixel((uint8_t)tx, (uint8_t)ty);
    lua_pushinteger(L, p);
    return 1;
}

inline void _pset(int16_t x, int16_t y, int16_t idx) {
    drawstate.pen_color = idx;
    if(drawstate.transparent[idx] == 1)
        return;
    int16_t tx = x-drawstate.camera_x;
    int16_t ty = y-drawstate.camera_y;
    if (tx < 0 || tx >= SCREEN_WIDTH || ty < 0 || ty  >= SCREEN_HEIGHT) return;
    put_pixel(tx, ty, idx);
}

// Numeric hot path used by OP_CALL in lvm.cpp.  Keeping the draw operation
// here makes the VM shortcut share pset()'s camera, clip, transparency,
// fill-pattern, draw-palette and pen-state behavior instead of duplicating
// renderer state inside the Lua core.  The ordinary C API remains the
// fallback for coercion and non-standard call forms.
static inline void p8_present_init_progress(void) {
    if (!engine_inside_init || !engine_init_followup_presented
        || !engine_draw_frame) return;
    const uint32_t current_ms = now();
    if ((uint32_t)(current_ms - engine_init_progress_last_ms) < 500) return;
    gfx_flip();
    engine_init_progress_last_ms = current_ms;
}

void p8_pset_fast_numeric(int16_t x, int16_t y, int has_color, int color) {
    const uint8_t idx = has_color ? (uint8_t)color : drawstate.pen_color;
    _pset(x, y, idx);
    p8_present_init_progress();
}

int _lua_pset(lua_State* L) {
    int16_t x = p8_checkinteger(L, 1);
    int16_t y = p8_checkinteger(L, 2);
    uint8_t idx = p8_optinteger(L, 3, drawstate.pen_color);
    _pset(x, y, idx);
    p8_present_init_progress();
    return 0;
}

int _lua_time(lua_State* L) {
    float delta = (float)(now() - bootup_time)/1000.0f;
    lua_pushnumber(L, delta);
    return 1;
}

int _lua_dget(lua_State* L) {
    const int idx = p8_checkinteger(L, 1);
    // Bounds-check: cartdata is 64 uint32_t entries. Mirror _lua_dset's guard
    // so an out-of-range idx returns 0 instead of OOB-reading adjacent BSS.
    if (idx >= 64) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushnumber(L, lua_Number::frombits((int32_t)cartdata[idx]));
    return 1;
}

int _lua_dset(lua_State* L) {
    const int idx = p8_checkinteger(L, 1);
    // Bounds-check: cartdata is 64 uint32_t entries. A cart calling
    // dset(64..255, val) would OOB-write past cartdata[] into adjacent
    // BSS, corrupting engine state. Silent no-op for out-of-range.
    if (idx < 0 || idx >= 64) return 0;
    // PICO-8's numeric coercion treats nil/non-numeric values as zero.
    // Multicarts commonly persist sparse table slots directly (Freezing
    // Knights writes absent enemy ids this way).
    const lua_Number val = p8_checknumber(L, 2);
    cartdata[idx] = (uint32_t)val.bits();
    cartdata_mark_dirty();
    return 0;
}

int _lua_cartdata(lua_State* L) {
    const char *id = luaL_checkstring(L, 1);
    if (!cartdata_open(id, cartdata, sizeof(cartdata))) {
        return luaL_error(L, "could not open cartdata namespace");
    }
    return 0;
}


int _lua_printh(lua_State* L) {
    const char* val = luaL_checkstring(L, 1);
    printf("> %s\n", val);
    fflush(stdout);
    return 0;
}


int _lua_stat(lua_State* L) {
    uint8_t n = p8_checkinteger(L, 1);
    if (n >=16 && n<=26) // 16..26 == 46..56
        n += 30;
    switch(n) {
        case 0: { // Lua memory used, in KiB
            int kb = lua_gc(L, LUA_GCCOUNT, 0);
            uint32_t bytes = (uint32_t)lua_gc(L, LUA_GCCOUNTB, 0);
            lua_pushnumber(L, kb + ((lua_Number)bytes / 1024));
            break;
        }
        case 1: { // measured host CPU load for the most recently completed tick
            float budget_us = 1000000.0f / (float)engine_tick_rate;
            lua_pushnumber(L, (lua_Number)((float)engine_last_busy_us / budget_us));
            break;
        }
        case 4: // no host clipboard is exposed to cartridges
            lua_pushliteral(L, "");
            break;
        case 6: // parameter string supplied by load()
            lua_pushstring(L, current_cart_param);
            break;
        case 7: // current frame rate; pacing is locked to the cart tick rate
        case 8: // target frame rate
            lua_pushinteger(L, engine_tick_rate);
            break;
        case 28: // no keyboard device exposed to cartridges
            lua_pushboolean(L, 0);
            break;
        case 30: // no keyboard device exposed to cartridges
        case 31:
            lua_pushboolean(L, 0);
            break;
        case 32: case 33: case 34: case 36: case 38: case 39:
            lua_pushinteger(L, 0); // no pointer device
            break;
        case 46: case 47: case 48: case 49: {
            int channel = n - 46;
            lua_pushinteger(L, channels[channel].sfx ? channels[channel].sfx_id : -1);
            break;
        }
        case 50: case 51: case 52: case 53: {
            int channel = n - 50;
            if (channels[channel].sfx == NULL || channels[channel].sfx->duration == 0) {
                lua_pushinteger(L, -1);
            } else {
                uint32_t note = channels[channel].offset /
                    ((uint32_t)SAMPLES_PER_DURATION * channels[channel].sfx->duration);
                lua_pushinteger(L, note < NOTES_PER_SFX ? note : NOTES_PER_SFX - 1);
            }
            break;
        }
        case 54:
            // music() is consumed by the synth task, but PICO-8 exposes the
            // requested pattern to stat(24) immediately. Returning the old
            // pattern during hand-off can break first-frame cache setup.
            lua_pushinteger(L, music_pending_pattern != -2
                               ? music_pending_pattern : music_pattern);
            break;
        case 55:
            lua_pushinteger(L, music_patterns_played);
            break;
        case 56:
            lua_pushinteger(L, music_elapsed / SAMPLES_PER_DURATION);
            break;
        case 57:
            lua_pushboolean(L, music_pattern >= 0);
            break;
        case 80: case 81: case 82: case 83: case 84: case 85:
        case 90: case 91: case 92: case 93: case 94: case 95: {
            time_t now = time(NULL);
            struct tm clock_value;
            struct tm *valid = n < 90
                ? gmtime_r(&now, &clock_value)
                : localtime_r(&now, &clock_value);
            if (valid == NULL) {
                lua_pushinteger(L, 0);
                break;
            }
            int field = n % 10;
            int value = 0;
            switch (field) {
                case 0: value = clock_value.tm_year + 1900; break;
                case 1: value = clock_value.tm_mon + 1; break;
                case 2: value = clock_value.tm_mday; break;
                case 3: value = clock_value.tm_hour; break;
                case 4: value = clock_value.tm_min; break;
                case 5: value = clock_value.tm_sec; break;
            }
            lua_pushinteger(L, value);
            break;
        }
        case 100: // no breadcrumb when launched directly by Retro-Go
        case 101: // no BBS cart id for locally launched carts
            lua_pushnil(L);
            break;
        case 102: // desktop/local-cart convention: numeric zero, not nil
            lua_pushinteger(L, 0);
            break;
        case 110: // frame-by-frame host mode is not exposed
        case 120: // no dropped-file serial channel
        case 121: // no dropped-image serial channel
            lua_pushboolean(L, 0);
            break;
        default:
            printf("Warn: got stat(%d) which is not implemented\n", n);
            lua_pushinteger(L, 0);
    }
    return 1;
}

int _lua_sfx(lua_State* L) {
    int16_t n       = p8_optinteger(L, 1, -1);
    int16_t channel = p8_optinteger(L, 2, -1);
    int16_t offset  = p8_optinteger(L, 3, 0);
    int16_t length  = p8_optinteger(L, 4, 0);

    // Negative N values are channel commands. With the default negative
    // channel they apply to all four channels: sfx(-1) stops everything,
    // while sfx(-2) releases loops and lets each sound finish.
    if (n == -1 || n == -2) {
        int first = channel < 0 ? 0 : channel;
        int last = channel < 0 ? 3 : channel;
        if (first < 0 || last >= 4) return 0;
        for (int c = first; c <= last; ++c) {
            if (n == -1) {
                channels[c].sfx = NULL;
                channels[c].sfx_id = 0;
                channels[c].offset = 0;
                channels[c].is_music = 0;
                channels[c].loop_released = 0;
                channels[c].phi = int32_t{0};
            } else if (channels[c].sfx != NULL) {
                channels[c].loop_released = 1;
            }
        }
        return 0;
    }

    // Channel -2 stops this particular SFX wherever it is playing.
    if (channel == -2) {
        if (n >= 0 && n < 64) {
            for (int c = 0; c < 4; ++c) {
                if (channels[c].sfx != NULL && channels[c].sfx_id == n) {
                    channels[c].sfx = NULL;
                    channels[c].sfx_id = 0;
                    channels[c].offset = 0;
                    channels[c].is_music = 0;
                    channels[c].loop_released = 0;
                    channels[c].phi = int32_t{0};
                }
            }
        }
        return 0;
    }

    // SFX storage is exactly 64 entries. The old code indexed sfx[n]
    // unchecked, allowing malformed carts to walk into adjacent audio state.
    if (n < 0 || n >= 64) return 0;

    // PICO-8 keeps a channel's oscillator continuous when a cart reissues
    // the same SFX on that channel with a new note offset. This is commonly
    // used as a cheap continuously variable engine tone. Resetting phi and
    // the noise state on every call introduces a hard discontinuity at the
    // cart update rate (Driftmania calls sfx(50,3,offset,0) every frame).
    // Capture this before duplicate suppression temporarily clears voices.
    bool continue_same_voice = channel >= 0 && channel < 4
                            && channels[channel].sfx != NULL
                            && channels[channel].sfx_id == n;

    // Starting an SFX already playing elsewhere moves it instead of making
    // a duplicate copy.
    for (int c = 0; c < 4; ++c) {
        if (channels[c].sfx != NULL && channels[c].sfx_id == n) {
            channels[c].sfx = NULL;
            channels[c].is_music = 0;
        }
    }

    if (channel == -1) {
        // Prefer an idle channel that is not reserved by active music. If
        // every available channel is busy, steal a non-reserved SFX channel.
        channel = -1;
        for (int c = 0; c < 4; ++c) {
            if (music_channel_mask & (1 << c))
                continue;
            if (channels[c].sfx == NULL) {
                channel = c;
                break;
            }
        }
        if (channel < 0) {
            for (int c = 0; c < 4; ++c) {
                if (music_channel_mask & (1 << c))
                    continue;
                channel = c;
                break;
            }
        }
    }
    if (channel < 0 || channel >= 4) return 0;

    SFX *playback_sfx = &sfx[n];
    if (playback_sfx->duration == 0) {
        bool populated = false;
        for (uint8_t note = 0; note < NOTES_PER_SFX; ++note) {
            if (playback_sfx->notes[note].volume != 0) {
                populated = true;
                break;
            }
        }
        if (!populated) return 0;

        // Wolfenstein's player gunshot is a populated speed-zero sfx(0).
        // Normalising it in SFXParser changed the shared slot globally and
        // also altered Snekburd's custom-instrument music. Scope the legacy
        // speed-one fallback to this direct playback channel instead.
        direct_zero_speed_sfx[channel] = *playback_sfx;
        direct_zero_speed_sfx[channel].duration = 1;
        playback_sfx = &direct_zero_speed_sfx[channel];
    }

    if (offset < 0) offset = 0;
    if (offset >= NOTES_PER_SFX) return 0;
    int end_note = length > 0 ? offset + length : NOTES_PER_SFX;
    if (end_note > NOTES_PER_SFX) end_note = NOTES_PER_SFX;
    if (end_note <= offset) return 0;

    // With no explicit LENGTH, loop_start>0 and loop_end==0 is PICO-8's
    // shortened non-looping SFX notation.
    if (length <= 0 && sfx[n].loop_start > 0 && sfx[n].loop_end == 0
        && sfx[n].loop_start < end_note) {
        end_note = sfx[n].loop_start;
    }

    channels[channel].offset    = (uint32_t)offset * SAMPLES_PER_DURATION
                                * playback_sfx->duration;
    channels[channel].sfx       = playback_sfx;
    channels[channel].sfx_id    = n;
    channels[channel].end_note  = (uint8_t)end_note;
    channels[channel].is_music  = 0;
    channels[channel].loop_released = 0;
    if (!continue_same_voice) {
        channels[channel].phi = int32_t{0};
        reset_channel_instrument(&channels[channel]);
        reset_channel_noise(&channels[channel],
                            ((uint32_t)n << 8) | channel);
    }
    channels[channel].prev_key  = sfx[n].notes[offset].key;
    channels[channel].prev_vol  = sfx[n].notes[offset].volume;
    return 0;
}

int _lua_music(lua_State* L) {
    int pattern = p8_optinteger(L, 1, 0);
    int fade_ms = p8_optinteger(L, 2, 0);
    int mask = p8_optinteger(L, 3, 0);
    music_request(pattern, fade_ms, mask);
    return 0;
}

int _lua_menuitem(lua_State* L) {
    int raw_index;
    bool update_current = lua_isnoneornil(L, 1);
    if (update_current) {
        if (active_cart_menu_item < 0) return 0;
        raw_index = active_cart_menu_item + 1;
    } else {
        raw_index = p8_checkinteger(L, 1);
    }

    int slot = (raw_index & 0xff) - 1;
    if (slot < 0 || slot >= 5) return 0;
    CartMenuItem *item = &cart_menu_items[slot];

    // From inside a callback, nil selects the current slot. Label-only
    // updates retain the callback and mask (the documented dynamic-label
    // idiom); a supplied function replaces the callback.
    if (update_current) {
        if (lua_isstring(L, 2)) {
            const char *label = lua_tostring(L, 2);
            size_t label_len = strlen(label);
            if (label_len > 16) label_len = 16;
            memcpy(item->label, label, label_len);
            item->label[label_len] = '\0';
        }
        if (lua_isfunction(L, 3)) {
            if (item->callback_ref >= 0)
                luaL_unref(L, LUA_REGISTRYINDEX, item->callback_ref);
            lua_pushvalue(L, 3);
            item->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        return 0;
    }

    // A missing label or callback removes the slot.
    if (!lua_isstring(L, 2) || !lua_isfunction(L, 3)) {
        if (item->callback_ref >= 0)
            luaL_unref(L, LUA_REGISTRYINDEX, item->callback_ref);
        item->callback_ref = LUA_NOREF;
        item->label[0] = '\0';
        item->input_mask = 0;
        return 0;
    }

    const char *label = lua_tostring(L, 2);
    size_t label_len = strlen(label);
    if (label_len > 16) label_len = 16;

    if (item->callback_ref >= 0)
        luaL_unref(L, LUA_REGISTRYINDEX, item->callback_ref);
    lua_pushvalue(L, 3);
    item->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    memcpy(item->label, label, label_len);
    item->label[label_len] = '\0';
    item->input_mask = (uint16_t)raw_index & 0xff00u;
    return 0;
}

int _lua_stub(lua_State* L) {
	// TODO: implement
    return 0;
}

int _lua_camera(lua_State* L) {
    int32_t x = p8_optinteger(L, 1, 0);
    int32_t y = p8_optinteger(L, 2, 0);
    int32_t old_x = drawstate.camera_x;
    int32_t old_y = drawstate.camera_y;

    drawstate.camera_x = x;
    drawstate.camera_y = y;
    p8_ram_write(0x5f28, (uint8_t)x);
    p8_ram_write(0x5f29, (uint8_t)(x >> 8));
    p8_ram_write(0x5f2a, (uint8_t)y);
    p8_ram_write(0x5f2b, (uint8_t)(y >> 8));

    lua_pushinteger(L, old_x);
    lua_pushinteger(L, old_y);
    return 2;
}

int _lua_clip(lua_State* L) {
    uint8_t old_x = drawstate.clip_x;
    uint8_t old_y = drawstate.clip_y;
    uint8_t old_w = drawstate.clip_w;
    uint8_t old_h = drawstate.clip_h;

    uint8_t argcount = lua_gettop(L);
    if(argcount == 0) {
        drawstate.clip_x = 0;
        drawstate.clip_y = 0;
        drawstate.clip_w = SCREEN_WIDTH;
        drawstate.clip_h = SCREEN_HEIGHT;
    } else {
        int x = p8_checkinteger(L, 1);
        int y = p8_checkinteger(L, 2);
        int w = p8_checkinteger(L, 3);
        int h = p8_checkinteger(L, 4);
        bool previous = lua_toboolean(L, 5);

        // Keep clipping arithmetic signed until the requested rectangle has
        // been intersected with the 128x128 display. Casting x/y to uint8_t
        // first made negative coordinates wrap to 255, producing an empty
        // clip instead of the visible portion of an off-screen rectangle.
        int x0 = MAX(0, MIN(x, SCREEN_WIDTH));
        int y0 = MAX(0, MIN(y, SCREEN_HEIGHT));
        int x1 = (w > 0) ? MAX(0, MIN(x + w, SCREEN_WIDTH)) : x0;
        int y1 = (h > 0) ? MAX(0, MIN(y + h, SCREEN_HEIGHT)) : y0;

        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;

        // PICO-8's fifth argument clips the requested rectangle by the
        // existing one. It is an intersection, not a relative offset.
        if (previous) {
            const int old_x1 = old_x + old_w;
            const int old_y1 = old_y + old_h;
            x0 = MAX(x0, (int)old_x);
            y0 = MAX(y0, (int)old_y);
            x1 = MIN(x1, old_x1);
            y1 = MIN(y1, old_y1);
            if (x1 < x0) x1 = x0;
            if (y1 < y0) y1 = y0;
        }

        drawstate.clip_x = (uint8_t)x0;
        drawstate.clip_y = (uint8_t)y0;
        drawstate.clip_w = (uint8_t)(x1 - x0);
        drawstate.clip_h = (uint8_t)(y1 - y0);
    }

    uint16_t clip_x1 = MIN((uint16_t)drawstate.clip_x
                         + (uint16_t)drawstate.clip_w,
                           (uint16_t)SCREEN_WIDTH);
    uint16_t clip_y1 = MIN((uint16_t)drawstate.clip_y
                         + (uint16_t)drawstate.clip_h,
                           (uint16_t)SCREEN_HEIGHT);
    ram[0x5f20] = drawstate.clip_x;
    ram[0x5f21] = drawstate.clip_y;
    ram[0x5f22] = (uint8_t)clip_x1;
    ram[0x5f23] = (uint8_t)clip_y1;

    lua_pushinteger(L, old_x);
    lua_pushinteger(L, old_y);
    lua_pushinteger(L, old_w);
    lua_pushinteger(L, old_h);
    return 4;
}

int _lua_color(lua_State* L) {
    uint8_t c = p8_optinteger(L, 1, 6);
    uint8_t old_color = drawstate.pen_color;
    drawstate.pen_color = c;
    ram[0x5f25] = drawstate.pen_color;
    lua_pushinteger(L, old_color);
    return 1;
}

int _lua_peek(lua_State* L) {
    // PICO-8's `peek(addr)` reads an 8-bit value from cart RAM at the
    // given address. Mirror `_lua_poke`: any address past the 32 KB
    // PICO-8 RAM (0x0000..0x7FFF) returns 0 instead of OOB-reading
    // adjacent BSS, which previously crashed Celeste2's `_init` (cart
    // line 600) when probing memory past the writable user-data region.
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    int count = p8_optinteger(L, 2, 1);
    if (count <= 0) return 0;
    // A counted peek returns one Lua result per byte. Loader carts commonly
    // pass those results straight to chr(); Into Ruins transfers a 10,019
    // byte entity database this way, so an arbitrary 8 KB cap silently
    // truncated valid data. Bound the request by the emulated 64 KB address
    // space instead, then let Lua's own stack guard reject allocation failure.
    uint32_t available = addr < P8_RAM_SIZE ? P8_RAM_SIZE - addr : 0;
    if ((uint32_t)count > available) count = (int)available;
    if (!lua_checkstack(L, count))
        return luaL_error(L, "peek result count too large");
    int pushed = 0;
    while (pushed < count && addr < P8_RAM_SIZE) {
        lua_pushinteger(L, p8_ram_read(addr++));
        pushed++;
    }
    return pushed;
}

int _lua_peek2(lua_State* L) {
    // PICO-8's peek2(addr, [count]) returns one 16-bit little-endian value
    // per two-byte stride. Solitomb restores five state words with
    // `peek2(0x5e7e, 5)`; returning only the first left its third result nil
    // and crashed the title coroutine. Keep the values as integer-valued
    // fix32 numbers and bound the result list to complete words in 64 KB RAM.
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    int count = p8_optinteger(L, 2, 1);
    if (count <= 0) return 0;
    uint32_t available = addr <= P8_RAM_SIZE - 2
                           ? (P8_RAM_SIZE - addr) / 2 : 0;
    if ((uint32_t)count > available) count = (int)available;
    if (count == 0) {
        lua_pushinteger(L, 0);
        return 1;
    }
    if (!lua_checkstack(L, count))
        return luaL_error(L, "peek2 result count too large");
    for (int result = 0; result < count; ++result, addr += 2) {
        uint16_t value = (uint16_t)(p8_ram_read(addr) |
                                   ((uint16_t)p8_ram_read(addr + 1) << 8));
        lua_pushinteger(L, value);
    }
    return count;
}

int _lua_poke(lua_State* L) {
    // Bounds-checked writes to PICO-8 RAM, with a mirror into the visible
    // frontbuffer when the address falls in the screen region
    // (0x6000..0x7FFF) — that mirror is what makes hand-drawn pixels visible
    // on the next gfx_flip.
    //
    // Two guards avoid the most pernicious OOB class: (1) discard any
    // poke past RAM end up front so addr+arg cannot quietly wrap and write
    // to ram[0..N] from a huge address; (2) keep offset == addr+arg in
    // unsigned 32-bit and compare against sizeof(ram) directly without an
    // `+N` on the LHS (which would wrap on uint32_t overflow).
    int argcount = lua_gettop(L);
    if (argcount < 1) return 0;
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    if (addr >= P8_RAM_SIZE) return 0;
    // PICO-8's API coercion supplies zero for a missing value. This concise
    // reset idiom is common in carts: poke("0x5f54") restores the default
    // sprite-sheet mapping after drawing from a relocated bank.
    if (argcount == 1) {
        p8_ram_write(addr, 0);
        return 0;
    }
    for (int arg = 0; arg < argcount - 1; ++arg) {
	uint8_t value = p8_checkinteger(L, 2+arg);
	uint32_t offset = addr + (uint32_t)arg;
	if (offset >= P8_RAM_SIZE) break;
	p8_ram_write(offset, value);
    }
    return 0;
}

int _lua_flip(lua_State* L) {
    // A cart-requested flip presents the framebuffer immediately but must
    // not run the outer emulator dispatcher. Re-entering flip() from a Lua
    // draw callback recursively invoked update/draw until the ESP32 task
    // stack was corrupted (Tiny Hawk transitions and Valdi both use this).
    // gfx_flip() waits for Retro-Go to release the submitted surface before
    // copying the next frame, preserving asynchronous display ownership.
    if (engine_draw_frame) {
        gfx_flip();
        if (engine_inside_init && !engine_init_followup_presented)
            engine_init_followup_pending = true;
    }
    // A few carts (Gar's Den among them) run a splash loop inside _init and
    // use flip()/btnp() to wait for acknowledgement. The outer dispatcher
    // cannot poll input until _init returns, so refresh button edges at the
    // explicit frame boundary just as native PICO-8 does.
    (void)handle_input();
    return 0;
}

int _lua_sub(lua_State* L) {
    // PICO-8's `sub(str, [start], [end])` is Lua stdlib's `string.sub`
    // re-exposed as a bare global (`sub(...)` instead of `string.sub(...)`).
    // We delegate by re-pulling `string.sub` from the `string` table and
    // re-dispatching with the cart's original args on the stack — Lua's
    // native impl already handles all of: 1-based indexing, negative
    // offsets (-1 = last char), end<start empty-string convention, etc.
    //
    // The Captain Neat-O In The Time Nexus cart calls `sub(...)` at cart
    // line 37 (_init) without the `string.` prefix; without this binding
    // it raises `attempt to call global 'sub' (a nil value)` and the
    // entire game fails to boot — pessimistic `_init` ordering means the
    // cart's other globals (player, etc.) never get assigned, cascading
    // into the `attempt to index global 'player' (a nil value)` errors
    // the user has been seeing every frame.
    //
    // Stack re-arrangement: cart pushed `(self_arg1, self_arg2, ...)` at
    // indices [1..n]. After `lua_getglobal("string"); lua_getfield(..., "sub")`,
    // the stack is [args..., "string", "sub"]. We pop `string`, then `lua_insert(1)`
    // moves `sub` from the top of stack to position 1, so the shape is
    // [sub_callable, arg1, arg2, ...] — exactly what `lua_call` expects.
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "sub");
    lua_remove(L, -2);  // pop "string" table; callable stays on top
    lua_insert(L, 1);   // move callable to slot 1, args shift up by one
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

int _lua_memset(lua_State* L) {
    // PICO-8's memset(destaddr, value, len): fill the cart-RAM byte range
    // [destaddr, destaddr+len) with `value` (0..255). len is in bytes, not
    // bit-cells; each byte becomes value verbatim.
    //
    // OOB-safe via three layered guards reviewed-in:
    //   (1) `len_signed <= 0` rejects the common PICO-8 idiom
    //       `memset(addr, 0, counter)` where `counter` may go negative at
    //       loop exit — without this catch the `(uint32_t)` cast wraps
    //       `-1` to `0xFFFFFFFF` and the inner loop runs `sizeof(ram) -
    //       destaddr` writes on the first iteration, which is NOT the
    //       no-op carts expect.
    //   (2) `destaddr >= sizeof(ram)` rejects OOB starting addresses up
    //       front so we never enter the loop with a bad base.
    //   (3) `len = MIN(len, sizeof(ram) - destaddr)` caps the iteration
    //       count so `destaddr + i` cannot overflow uint32_t mid-loop
    //       (would otherwise wrap from `0x8000-1` past 0 and OOB-write
    //       ram[0..N] after a single false break).
    //
    // Mirror into frontbuffer (0x6000..0x7FFF) so carts that screen-blit
    // via memset see the pixels on the next gfx_flip without an extra
    // poke() round-trip, just like `_lua_poke` does. The cap guarantees
    // `destaddr + i < sizeof(ram)` so the `<= 0x7fff` upper-bound is
    // implicit; `addr >= 0x6000` alone is sufficient.
    //
    // Captain Neat-O calls memset at cart line 1260 (_update60); previously
    // raised `attempt to call global 'memset' (a nil value)` per frame.
    uint32_t destaddr = (uint16_t)p8_checkinteger(L, 1);
    uint8_t  value    = (uint8_t)p8_checkinteger(L, 2);
    lua_Integer len_signed = p8_checkinteger(L, 3);
    if (len_signed <= 0) return 0;
    if (destaddr >= P8_RAM_SIZE) return 0;
    uint32_t len = (uint32_t)len_signed;
    uint32_t cap = P8_RAM_SIZE - destaddr;
    if (len > cap) len = cap;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t addr = destaddr + i;
        p8_ram_write(addr, value);
    }
    return 0;
}

int _lua_memcpy(lua_State* L) {
    // PICO-8's memcpy(dst, src, n): copy n bytes from cart-RAM[src..src+n)
    // to cart-RAM[dst..dst+n). Both addresses are 16-bit, n is a byte
    // count (not bit-cells). For overlapping ranges PICO-8 docs say the
    // behaviour is undefined -- carts that need overlap-safe copying use
    // memmove (still a TODO -- most carts including Cab Ride use non-
    // overlapping ranges only).
    //
    // OOB mirrors the _lua_memset layered-guard idiom exactly so a
    // negative Lua int for n (or for the addresses after the cast)
    // cannot wrap and produce a phantom copy:
    //   (1) n_signed <= 0           -> no-op on negative byte count.
    //   (2) dst >= sizeof(ram)      -> no-op on out-of-window dst.
    //   (3) src >= sizeof(ram)      -> no-op on out-of-window src.
    //   (4) n = MIN(n, sizeof(ram)-dst, sizeof(ram)-src) caps on
    //       dst+i and src+i so neither can wrap uint32_t mid-loop.
    //
    // Mirror into frontbuffer on writes to 0x6000..0x7FFF (same as
    // poke/memset) so carts that screen-blit via memcpy see the pixels
    // on the next gfx_flip without a per-byte poke round-trip.
    //
    // Cab Ride (.p8.png) calls memcpy at cart _draw line 1938 every
    // frame; previously raised attempt to call global 'memcpy' (a nil
    // value) per frame.
    uint32_t dst = (uint16_t)p8_checkinteger(L, 1);
    uint32_t src = (uint16_t)p8_checkinteger(L, 2);
    lua_Integer n_signed = p8_checkinteger(L, 3);
    if (n_signed <= 0) return 0;
    if (dst >= P8_RAM_SIZE || src >= P8_RAM_SIZE) return 0;
    uint32_t n = (uint32_t)n_signed;
    uint32_t cap_dst = P8_RAM_SIZE - dst;
    uint32_t cap_src = P8_RAM_SIZE - src;
    if (n > cap_dst) n = cap_dst;
    if (n > cap_src) n = cap_src;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t d = dst + i;
        uint32_t s = src + i;
        p8_ram_write(d, p8_ram_read(s));
    }
    return 0;
}

// tline's one-argument form changes how many low bits of each 16.16 Lua
// coordinate are treated as the fractional part of a spritesheet pixel.
// PICO-8 defaults to 13: one map tile (8 pixels) is therefore 1.0 in Lua.
// Higher precision is useful to perspective renderers; Poom selects 17 and
// supplies its wall coordinates multiplied by 16. Keep this as one byte of
// engine state and leave the inner sampler integer-only.
static uint8_t tline_precision_bits = 13;

static void p8_tline_reset(void) {
    tline_precision_bits = 13;
}

static inline uint32_t p8_tline_tile_scale(uint8_t value, uint8_t shift) {
    // A precision above 28 cannot represent a complete eight-pixel tile in
    // the 32-bit coordinate. Returning zero for that scale also avoids an
    // undefined 32-bit shift without adding 64-bit work to normal spans.
    return shift < 32 ? ((uint32_t)value << shift) : 0u;
}

int _lua_tline(lua_State* L) {
    // PICO-8's tline(x0, y0, x1, y1, mx, my, mdx, mdy, [mask]):
    // The single-argument extension selects coordinate precision for later
    // calls. Clamp to the bit width of the underlying fixed-point coordinate
    // so malformed values cannot produce undefined shifts.
    uint8_t argcount = lua_gettop(L);
    if (argcount == 1) {
        int precision = p8_checkinteger(L, 1);
        if (precision < 0) precision = 0;
        if (precision > 31) precision = 31;
        tline_precision_bits = (uint8_t)precision;
        return 0;
    }

    // With the default precision of 13, mx/my are map-tile coordinates: an
    // integer selects a tile and each 0.125 step selects one pixel within it.
    // Sampling uses native fixed-point bits for accuracy and speed, honours
    // map wrapping/offset registers, relocated maps, layers, transparency,
    // the draw palette, clipping, and camera state.
    if (argcount < 6) return 0;
    int16_t x0   = (int16_t)p8_checkinteger(L, 1);
    int16_t y0   = (int16_t)p8_checkinteger(L, 2);
    int16_t x1   = (int16_t)p8_checkinteger(L, 3);
    int16_t y1   = (int16_t)p8_checkinteger(L, 4);
    uint32_t mx  = p8_checkfixedbits(L, 5);
    uint32_t my  = p8_checkfixedbits(L, 6);
    uint32_t mdx = lua_isnoneornil(L, 7)
                 ? 0x00002000u : p8_checkfixedbits(L, 7); // 0.125
    uint32_t mdy = lua_isnoneornil(L, 8)
                 ? 0u : p8_checkfixedbits(L, 8);
    uint8_t mask = (uint8_t)p8_optinteger(L, 9, 0);

    P8MapLayout layout;
    if (!p8_map_layout(&layout)) return 0;
    const uint32_t map_height = layout.capacity / layout.width;

    // Hardware wrap/offset registers are expressed in whole 8-pixel tiles.
    // Scale them once per span. The helper keeps this in cheap 32-bit integer
    // operations on ESP32 while defining the extreme precision cases too.
    const uint8_t pixel_shift = tline_precision_bits;
    const uint8_t tile_shift = (uint8_t)(pixel_shift + 3);
    const uint32_t x_mask =
        p8_tline_tile_scale(ram[0x5f38], tile_shift) - 1u;
    const uint32_t y_mask =
        p8_tline_tile_scale(ram[0x5f39], tile_shift) - 1u;
    const uint32_t x_offset =
        p8_tline_tile_scale(ram[0x5f3a], tile_shift);
    const uint32_t y_offset =
        p8_tline_tile_scale(ram[0x5f3b], tile_shift);
    const bool draw_sprite_zero = (ram[0x5f36] & 0x08u) != 0;

    // Perspective carts commonly build their view from many horizontal
    // tline() spans which extend well beyond both sides of the display.
    // Driftmania's spans are about 255 pixels wide for a 128-pixel screen.
    // The generic Bresenham loop used to perform map and sprite lookups for
    // every off-screen sample and reject it only at the final pixel store.
    // Clip horizontal spans first, advancing the texture stream by the exact
    // number of skipped samples so the visible result remains identical.
    if (y0 == y1) {
        const int screen_y = (int)y0 - drawstate.camera_y;
        const int clip_left = MAX(0, (int)drawstate.clip_x);
        const int clip_top = MAX(0, (int)drawstate.clip_y);
        const int clip_right = MIN(SCREEN_WIDTH - 1,
            (int)drawstate.clip_x + drawstate.clip_w - 1);
        const int clip_bottom = MIN(SCREEN_HEIGHT - 1,
            (int)drawstate.clip_y + drawstate.clip_h - 1);
        if (clip_left > clip_right || clip_top > clip_bottom
            || screen_y < clip_top || screen_y > clip_bottom)
            return 0;

        const int world_left = clip_left + drawstate.camera_x;
        const int world_right = clip_right + drawstate.camera_x;
        const int x_step = x0 <= x1 ? 1 : -1;
        int first_x;
        int last_x;
        if (x_step > 0) {
            first_x = MAX((int)x0, world_left);
            last_x = MIN((int)x1, world_right);
            if (first_x > last_x) return 0;
        } else {
            first_x = MIN((int)x0, world_right);
            last_x = MAX((int)x1, world_left);
            if (first_x < last_x) return 0;
        }

        const uint32_t skipped = (uint32_t)abs(first_x - (int)x0);
        mx += mdx * skipped;
        my += mdy * skipped;

        int world_x = first_x;
        for (;;) {
            const uint32_t sample_x = (mx & x_mask) + x_offset;
            const uint32_t sample_y = (my & y_mask) + y_offset;
            const uint32_t pixel_x = sample_x >> pixel_shift;
            const uint32_t pixel_y = sample_y >> pixel_shift;
            const uint32_t tile_x = pixel_x >> 3;
            const uint32_t tile_y = pixel_y >> 3;

            if (tile_x < layout.width && tile_y < map_height) {
                uint8_t sprite_id;
                if (layout.default_layout) {
                    sprite_id = map_data[tile_x + tile_y * 128];
                } else {
                    uint32_t address;
                    sprite_id = p8_map_address(&layout, tile_x, tile_y,
                                                &address)
                              ? p8_ram_read(address) : 0;
                }

                if ((sprite_id != 0 || draw_sprite_zero)
                    && (mask == 0
                        || (spritesheet.flags[sprite_id] & mask) != 0)) {
                    const uint8_t sub_x = (uint8_t)(pixel_x & 7u);
                    const uint8_t sub_y = (uint8_t)(pixel_y & 7u);
                    const uint8_t val = spritesheet.sprite_data[
                        (((sprite_id >> 4) * 8 + sub_y) * 128)
                        + (sprite_id & 0x0f) * 8 + sub_x];
                    if (!drawstate.transparent[val]) {
                        const uint8_t out_x =
                            (uint8_t)(world_x - drawstate.camera_x);
                        if (drawstate.fill_flags & 2)
                            put_sprite_pixel(out_x, (uint8_t)screen_y, val);
                        else
                            put_pixel_mapped_unchecked(
                                out_x, (uint8_t)screen_y,
                                pal_map[val & 0x0f]);
                    }
                }
            }

            if (world_x == last_x) break;
            world_x += x_step;
            mx += mdx;
            my += mdy;
        }
        return 0;
    }

    // Bresenham state (mirrors gfx_line's structure, walking the
    // SCREEN line independently of the texture stream).
    int16_t dx = abs((int)(x1 - x0));
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs((int)(y1 - y0));
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;
    int16_t cur_x = x0;
    int16_t cur_y = y0;
    const int clip_left = MAX(0, (int)drawstate.clip_x);
    const int clip_top = MAX(0, (int)drawstate.clip_y);
    const int clip_right = MIN(SCREEN_WIDTH,
        (int)drawstate.clip_x + drawstate.clip_w);
    const int clip_bottom = MIN(SCREEN_HEIGHT,
        (int)drawstate.clip_y + drawstate.clip_h);

    for (;;) {
        uint32_t sample_x = (mx & x_mask) + x_offset;
        uint32_t sample_y = (my & y_mask) + y_offset;
        uint32_t pixel_x = sample_x >> pixel_shift;
        uint32_t pixel_y = sample_y >> pixel_shift;
        uint32_t tile_x = pixel_x >> 3;
        uint32_t tile_y = pixel_y >> 3;
        uint8_t sub_x = (uint8_t)(pixel_x & 7u);
        uint8_t sub_y = (uint8_t)(pixel_y & 7u);

        if (tile_x < layout.width && tile_y < map_height) {
            uint8_t sprite_id;
            if (layout.default_layout) {
                sprite_id = map_data[tile_x + tile_y * 128];
            } else {
                uint32_t address;
                sprite_id = p8_map_address(&layout, tile_x, tile_y, &address)
                          ? p8_ram_read(address) : 0;
            }

            if ((sprite_id != 0 || draw_sprite_zero)
                && (mask == 0
                    || (spritesheet.flags[sprite_id] & mask) != 0)) {
                // Look up the per-pixel color in the 128x128 spritesheet
                // (matches _render's offset arithmetic so tline and spr()
                // agree on what "the same sprite_id coords" mean visually).
                uint8_t xIndex = sprite_id & 0xf;
                uint8_t yIndex = sprite_id >> 4;
                uint8_t val = spritesheet.sprite_data[
                    (yIndex * 8 + sub_y) * 128
                    + xIndex * 8 + sub_x];
                int screen_x = cur_x - drawstate.camera_x;
                int screen_y = cur_y - drawstate.camera_y;
                if (!drawstate.transparent[val]
                    && screen_x >= clip_left && screen_x < clip_right
                    && screen_y >= clip_top && screen_y < clip_bottom) {
                    put_sprite_pixel((uint8_t)screen_x,
                                     (uint8_t)screen_y, val);
                }
            }
        }

        if (cur_x == x1 && cur_y == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; cur_x += sx; }
        if (e2 <= dx) { err += dx; cur_y += sy; }

        mx += mdx;
        my += mdy;
    }
    return 0;
}

// --- peek4 / poke2 / type / tostr / tonum / table ops / chr / ord --------

int _lua_peek4(lua_State* L) {
    // PICO-8's peek4(addr, [count]) returns one raw 16.16 value per four-byte
    // stride. Pushing these as lua_Integer would discard the low 16 bits (and
    // truncate the high half to int16_t), so construct every result directly
    // from its raw fixed-point bit pattern.
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    int count = p8_optinteger(L, 2, 1);
    if (count <= 0) return 0;
    uint32_t available = addr <= P8_RAM_SIZE - 4
                           ? (P8_RAM_SIZE - addr) / 4 : 0;
    if ((uint32_t)count > available) count = (int)available;
    if (count == 0) {
        lua_pushnumber(L, z8::fix32::frombits(0));
        return 1;
    }
    if (!lua_checkstack(L, count))
        return luaL_error(L, "peek4 result count too large");
    for (int result = 0; result < count; ++result, addr += 4) {
        uint32_t value = (uint32_t)p8_ram_read(addr)
                       | ((uint32_t)p8_ram_read(addr + 1) << 8)
                       | ((uint32_t)p8_ram_read(addr + 2) << 16)
                       | ((uint32_t)p8_ram_read(addr + 3) << 24);
        lua_pushnumber(L, z8::fix32::frombits((int32_t)value));
    }
    return count;
}

int _lua_poke2(lua_State* L) {
    // PICO-8's poke2(addr, v1, v2, ...) writes each 16-bit value as 2 LE
    // bytes at consecutive 2-byte strides (addr+0, addr+2, addr+4, ...).
    // OOB mirrors poke4's guard: upfront addr check, per-iteration cap.
    uint8_t argcount = lua_gettop(L);
    if (argcount == 0) return 0;
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    if (addr > P8_RAM_SIZE - 2) return 0;
    for (uint8_t arg = 0; arg < argcount - 1; arg++) {
        uint16_t value = (uint16_t)p8_checkinteger(L, 2 + arg);
        uint32_t offset = addr + (uint32_t)arg * 2;
        if (offset + 2 > P8_RAM_SIZE) break;
        p8_ram_write(offset + 0, (uint8_t)(value & 0xFF));
        p8_ram_write(offset + 1, (uint8_t)((value >> 8) & 0xFF));
    }
    return 0;
}

int _lua_type(lua_State* L) {
    // PICO-8's type(v) returns "number", "string", "table", "boolean",
    // "nil", or "function" — identical to Lua's built-in type().  We
    // override the stdlib registration so the name matches PICO-8 docs
    // exactly and any future fix32-specific type tweaks land here.
    luaL_checkany(L, 1);
    lua_pushstring(L, luaL_typename(L, 1));
    return 1;
}

int _lua_tostr(lua_State* L) {
    // PICO-8's second argument is a format bitfield, not a truth value:
    //   bit 0: raw hexadecimal fixed-point representation
    //   bit 1: signed 32-bit raw representation
    // The legacy boolean form remains supported (true == bit 0).
    if (lua_gettop(L) == 0) {
        lua_pushliteral(L, "");
        return 1;
    }
    if (lua_isnil(L, 1)) {
        lua_pushliteral(L, "[nil]");
        return 1;
    }

    int flags = 0;
    if (lua_isboolean(L, 2))
        flags = lua_toboolean(L, 2) ? 1 : 0;
    else if (!lua_isnoneornil(L, 2))
        flags = p8_checkinteger(L, 2);

    // lua_isnumber() is deliberately permissive and returns true for strings
    // that can be converted to numbers. PICO-8 tostr() must preserve an
    // existing string byte-for-byte: compact decoders use marker strings
    // such as ".0", then inspect the leading dot after tostr(). Checking
    // lua_isnumber() first changed ".1" into "0.1000", losing the marker and
    // corrupting Sneaky Stealy's zero-indexed lookup tables.
    if (lua_type(L, 1) == LUA_TSTRING) {
        lua_pushvalue(L, 1);
    } else if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_Number value = lua_tonumber(L, 1);
        uint32_t raw = (uint32_t)value.bits();
        float n = (float)value;
        char buf[32];
        if (flags & 0x1) {
            if (flags & 0x2) {
                snprintf(buf, sizeof(buf), "0x%08x", (unsigned)raw);
            } else {
                snprintf(buf, sizeof(buf), "0x%04x.%04x",
                         (unsigned)(raw >> 16), (unsigned)(raw & 0xffffu));
            }
        } else if (flags & 0x2) {
            snprintf(buf, sizeof(buf), "%ld", (long)(int32_t)raw);
        } else if (n == (float)(int)n) {
            snprintf(buf, sizeof(buf), "%d", (int)n);
        } else {
            snprintf(buf, sizeof(buf), "%.4f", n);
        }
        lua_pushstring(L, buf);
    } else {
        // booleans, nil, functions — use Lua's tostring
        lua_getglobal(L, "tostring");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
        } else {
            lua_pop(L, 1);
            lua_pushstring(L, "");
        }
    }
    return 1;
}

int _lua_tonum(lua_State* L) {
    // PICO-8's tonum(val, [format_flags]): numbers pass through and booleans
    // become 1/0.  Crucially, an unparseable string returns no value (nil in
    // an expression), unless flag 0x4 requests a zero fallback.  Keeping
    // this distinct from p8_checknumber() matters: compact cart decoders use
    // `tonum(field) or field` to preserve command names such as "poke".
    //
    // Flags mirror the PICO-8 serialization helpers:
    //   0x1  parse an unsigned hexadecimal integer without a 0x prefix;
    //        non-hexadecimal characters contribute a zero nibble
    //   0x2  interpret the parsed integer as raw signed 16.16 bits
    //   0x4  return 0 instead of no value when conversion fails
    luaL_checkany(L, 1);
    // lua_isnumber() also accepts convertible strings. Returning the original
    // stack value in that case leaked a string from tonum("0x...."), which
    // then became raw object bits in fast numeric calls such as shr().
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_pushvalue(L, 1);
        return 1;
    }

    if (lua_isboolean(L, 1)) {
        lua_pushinteger(L, lua_toboolean(L, 1) ? 1 : 0);
        return 1;
    }

    const int flags = lua_isnoneornil(L, 2) ? 0 : p8_checkinteger(L, 2);
    if (lua_isstring(L, 1)) {
        size_t len = 0;
        const char* s = lua_tolstring(L, 1, &len);

        if ((flags & 0x1) && len > 0) {
            uint32_t value = 0;
            for (size_t i = 0; i < len; ++i) {
                const unsigned char ch = (unsigned char)s[i];
                uint32_t nibble = 0;
                if (ch >= '0' && ch <= '9') nibble = ch - '0';
                else if (ch >= 'a' && ch <= 'f') nibble = ch - 'a' + 10;
                else if (ch >= 'A' && ch <= 'F') nibble = ch - 'A' + 10;
                value = (value << 4) | nibble;
            }

            if (flags & 0x2)
                lua_pushnumber(L, z8::fix32::frombits((int32_t)value));
            else
                lua_pushnumber(L,
                    z8::fix32::frombits((int32_t)(value << 16)));
            return 1;
        }

        if ((flags & 0x2) && len > 0) {
            char* end = NULL;
            long value = strtol(s, &end, 10);
            if (end && end != s && *end == '\0') {
                lua_pushnumber(L,
                    z8::fix32::frombits((int32_t)(uint32_t)value));
                return 1;
            }
        } else {
            int converted = 0;
            lua_Number value = lua_tonumberx(L, 1, &converted);
            if (converted) {
                lua_pushnumber(L, value);
                return 1;
            }
        }
    }

    if (flags & 0x4) {
        lua_pushinteger(L, 0);
        return 1;
    }
    return 0;
}

// Kept externally visible so lvm.cpp can recognize this exact built-in
// iterator and bypass the generic C CallInfo path.  Do not use the shortcut
// for arbitrary C closures: all()'s three-upvalue layout and deletion-aware
// index rule below are part of that specialization's contract.
int _all_iter(lua_State* L) {
    // Iterator body for all(t). Upvalue 1 = table, upvalue 2 = the index
    // yielded last time, and upvalue 3 = the value yielded there. If the
    // caller deleted that value, the next item has shifted into the same
    // index and must not be skipped (the PICO-8 manual relies on this).
    int i = (int)lua_tointeger(L, lua_upvalueindex(2));
    if (i <= 0) {
        i = 1;
    } else {
        lua_rawgeti(L, lua_upvalueindex(1), i);
        int previous_still_here = lua_rawequal(L, -1, lua_upvalueindex(3));
        lua_pop(L, 1);
        if (previous_still_here) i++;
    }

    int len = (int)lua_rawlen(L, lua_upvalueindex(1));
    if (i > len) return 0;

    lua_pushinteger(L, i);
    lua_replace(L, lua_upvalueindex(2));
    lua_rawgeti(L, lua_upvalueindex(1), i);
    lua_pushvalue(L, -1);
    lua_replace(L, lua_upvalueindex(3));
    return 1;
}

static int _all_empty_iter(lua_State* L) {
    (void)L;
    return 0;
}

int _lua_inext(lua_State* L) {
    // Integer iterator used directly as:
    //   for i, value in inext, table do ... end
    // With no initial control value PICO-8 starts immediately before index 1.
    luaL_checktype(L, 1, LUA_TTABLE);
    int index = (int)p8_optinteger(L, 2, 0) + 1;
    lua_pushinteger(L, index);
    lua_rawgeti(L, 1, index);
    return lua_isnil(L, -1) ? 1 : 2;
}

static int _all_string_iter(lua_State* L) {
    // PICO-8 also permits all(s), yielding each raw P8SCII byte as a
    // one-character string. Keep the source string alive in upvalue 1 and
    // the next byte offset in upvalue 2. Using an explicit length preserves
    // embedded NUL bytes and avoids allocating a copy of the whole string.
    size_t len = 0;
    const char* str = lua_tolstring(L, lua_upvalueindex(1), &len);
    size_t index = (size_t)lua_tointeger(L, lua_upvalueindex(2));
    if (str == NULL || index >= len) return 0;

    lua_pushinteger(L, (lua_Integer)(index + 1));
    lua_replace(L, lua_upvalueindex(2));
    lua_pushlstring(L, str + index, 1);
    return 1;
}

int _lua_all(lua_State* L) {
    // PICO-8's all(t): returns an iterator function for use in
    //   for v in all(t) do ... end
    // The original PicoPico Lua helper deliberately accepts nil and returns
    // an already-exhausted iterator. Optional child lists commonly rely on
    // this (Pizza Panda calls all(a._ab) when the object has no skins).
    if (lua_isnoneornil(L, 1)) {
        lua_pushcfunction(L, _all_empty_iter);
        return 1;
    }

    if (lua_type(L, 1) == LUA_TSTRING) {
        lua_pushvalue(L, 1);           // upvalue 1: source string
        lua_pushinteger(L, 0);         // upvalue 2: next byte offset
        lua_pushcclosure(L, _all_string_iter, 2);
        return 1;
    }

    // Other non-nil values must still be tables. The iterator walks the array
    // portion [1..#t] yielding each element.
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, 1);           // upvalue 1: table reference
    lua_pushinteger(L, 0);         // upvalue 2: current index (0 = before first)
    lua_pushnil(L);                // upvalue 3: previously yielded value
    lua_pushcclosure(L, _all_iter, 3);
    return 1;
}

int _lua_pack(lua_State* L) {
    // PICO-8 exposes pack(...) as a global. Avoid aliasing Lua 5.2's
    // table.pack implementation here: that routine moves the newly-created
    // table into stack slot 1 and consumes the arguments backwards. This
    // fixed-point Lua fork has shown a startup lock while Pizza Panda calls
    // that path repeatedly from its map decoder. A direct forward copy is
    // equivalent, preserves nil arguments through the `n` field, and keeps
    // the caller's argument slots stable throughout construction.
    int count = lua_gettop(L);
    lua_createtable(L, count, 1);
    int result = lua_gettop(L);

    for (int i = 1; i <= count; ++i) {
        lua_pushvalue(L, i);
        lua_rawseti(L, result, i);
    }

    lua_pushinteger(L, (lua_Integer)count);
    lua_setfield(L, result, "n");
    return 1;
}

int _lua_add(lua_State* L) {
    // PICO-8's add(t, v, [i]): appends v to table t (or inserts at i).
    // Without i: t[#t + 1] = v.  With i: shift elements and insert.
    // PicoPico's original Lua helper treated a nil destination as a no-op.
    // Preserve that compatibility while keeping other non-table values as
    // errors, so malformed carts are not silently hidden.
    if (lua_isnoneornil(L, 1)) {
        if (lua_isnone(L, 2)) return 0;
        lua_pushvalue(L, 2);
        return 1;
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    if (lua_isnoneornil(L, 2)) return 0;
    int pos = (int)p8_optinteger(L, 3, 0);
    if (pos <= 0) {
        // Append: most common case
        int len = (int)lua_rawlen(L, 1);
        lua_pushvalue(L, 2);
        lua_rawseti(L, 1, len + 1);
    } else {
        // Insert at pos: shift elements right
        int len = (int)lua_rawlen(L, 1);
        for (int i = len; i >= pos; i--) {
            lua_rawgeti(L, 1, i);
            lua_rawseti(L, 1, i + 1);
        }
        lua_pushvalue(L, 2);
        lua_rawseti(L, 1, pos);
    }
    lua_pushvalue(L, 2);
    return 1;
}

int _lua_del(lua_State* L) {
    // PICO-8's del(t, v): removes the first element equal to v from table t.
    // Returns the deleted element, or no value if not found. The compatibility
    // helper historically treats a nil list as an empty optional list; carts
    // rely on this when removing an effect from a collection that was never
    // created (Crowded Dungeon's object-move path is one example).
    if (lua_isnoneornil(L, 1)) return 0;
    luaL_checktype(L, 1, LUA_TTABLE);
    // Particle/entity lists normally live entirely in Lua's dense array part.
    // Shift those with one overlap-safe move; sparse/hash-backed sequences
    // return -1 and retain the generic raw-table implementation below.
    int dense_result = lua_p8_delarray(L, 1, 2);
    if (dense_result >= 0) {
        if (dense_result > 0) {
            lua_pushvalue(L, 2);
            return 1;
        }
        return 0;
    }

    int len = (int)lua_rawlen(L, 1);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, i);
        if (lua_rawequal(L, -1, 2)) {
            lua_pop(L, 1);
            // Shift elements left
            for (int j = i; j < len; j++) {
                lua_rawgeti(L, 1, j + 1);
                lua_rawseti(L, 1, j);
            }
            // Clear last slot and return the deleted value.
            lua_pushnil(L);
            lua_rawseti(L, 1, len);
            lua_pushvalue(L, 2);
            return 1;
        }
        lua_pop(L, 1);
    }
    return 0;
}

int _lua_deli(lua_State* L) {
    // PICO-8's deli(t, [i]): remove by 1-based index, defaulting to the
    // final element. Return the deleted value, or no value for an invalid
    // index. This differs from del(t, value), which never treats a missing
    // value as a request to pop the array.
    luaL_checktype(L, 1, LUA_TTABLE);
    int len = (int)lua_rawlen(L, 1);
    int pos = lua_isnoneornil(L, 2) ? len : (int)p8_checkinteger(L, 2);
    if (pos < 1 || pos > len) return 0;

    lua_rawgeti(L, 1, pos);
    for (int j = pos; j < len; j++) {
        lua_rawgeti(L, 1, j + 1);
        lua_rawseti(L, 1, j);
    }
    lua_pushnil(L);
    lua_rawseti(L, 1, len);
    return 1;
}

int _lua_count(lua_State* L) {
    // PICO-8's count(t, [v]): returns #t, or the count of elements
    // matching v when a second arg is given.  Returns 0 for nil.
    if (lua_gettop(L) < 1 || lua_isnoneornil(L, 1)) {
        lua_pushinteger(L, 0);
        return 1;
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    if (lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2)) {
        int n = 0;
        int len = (int)lua_rawlen(L, 1);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, 1, i);
            if (lua_rawequal(L, -1, 2)) n++;
            lua_pop(L, 1);
        }
        lua_pushinteger(L, n);
    } else {
        lua_pushinteger(L, (int)lua_rawlen(L, 1));
    }
    return 1;
}

int _lua_foreach(lua_State* L) {
    // PICO-8's foreach(t, f): calls f(v) for every element in t.
    // Snapshot the array before dispatch, but only invoke entries which still
    // belong to the live source table. Carts commonly delete from the table
    // inside the callback, so a simple rising index skips shifted entries. A
    // pure snapshot has the opposite problem: if a callback replaces the
    // whole table (Celeste changes rooms from inside its object update), it
    // continues updating stale objects from the previous room.
    //
    // The cursor makes the no-mutation and delete-current cases linear. The
    // fallback scan is only needed when a callback removes/reorders some other
    // entry or replaces the table contents during iteration.
    // PICO-8 implements foreach(t, f) in terms of all(t), and all(nil)
    // returns an exhausted iterator. Consequently foreach(nil, f) is a
    // silent no-op. Pizza Panda leaves its optional dialogue draw list nil
    // on the title screen and relies on this behaviour.
    if (lua_isnoneornil(L, 1)) return 0;

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int len = (int)lua_rawlen(L, 1);

    lua_createtable(L, len, 0);
    int snapshot = lua_gettop(L);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, i);
        lua_rawseti(L, snapshot, i);
    }

    int cursor = 1;
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, snapshot, i);
        int item = lua_gettop(L);
        int live_len = (int)lua_rawlen(L, 1);
        int found = 0;

        if (cursor <= live_len) {
            lua_rawgeti(L, 1, cursor);
            if (lua_rawequal(L, item, -1)) found = cursor;
            lua_pop(L, 1);
        }

        if (!found) {
            for (int j = 1; j <= live_len; j++) {
                lua_rawgeti(L, 1, j);
                int equal = lua_rawequal(L, item, -1);
                lua_pop(L, 1);
                if (equal) {
                    found = j;
                    break;
                }
            }
        }

        if (found) {
            int before_len = live_len;
            lua_pushvalue(L, 2);
            lua_pushvalue(L, item);
            lua_call(L, 1, 0);

            int after_len = (int)lua_rawlen(L, 1);
            int stayed = 0;
            if (found <= after_len) {
                lua_rawgeti(L, 1, found);
                stayed = lua_rawequal(L, item, -1);
                lua_pop(L, 1);
            }

            if (after_len == before_len && found <= after_len) {
                cursor = stayed ? found + 1 : found;
            } else {
                // A deletion shifts the next live entry into this slot.
                cursor = found;
            }

            /* PICO-8's foreach is a live all(t) traversal. If a callback
               replaces its current entry or appends an entry, that new value
               is eligible later in the same traversal. Tower of Archeos
               replaces a completed timer with its input-controller entity
               and needs that controller initialized before the next draw.

               Preserve the room-change protection above: only extend the
               snapshot when at least one of its original values still belongs
               to the live table. A wholesale replacement (such as Celeste
               changing rooms) must not update the new room in this frame.
               Pure shrinking deletions avoid this deliberately slower path. */
            if (after_len >= before_len && (!stayed || after_len > before_len)) {
                int survivor = 0;
                for (int old = 1; old <= len && !survivor; ++old) {
                    lua_rawgeti(L, snapshot, old);
                    int old_item = lua_gettop(L);
                    for (int live = 1; live <= after_len; ++live) {
                        lua_rawgeti(L, 1, live);
                        int equal = lua_rawequal(L, old_item, -1);
                        lua_pop(L, 1);
                        if (equal) { survivor = 1; break; }
                    }
                    lua_pop(L, 1);
                }

                if (survivor) {
                    for (int live = 1; live <= after_len; ++live) {
                        lua_rawgeti(L, 1, live);
                        int live_item = lua_gettop(L);
                        int known = 0;
                        for (int old = 1; old <= len; ++old) {
                            lua_rawgeti(L, snapshot, old);
                            known = lua_rawequal(L, live_item, -1);
                            lua_pop(L, 1);
                            if (known) break;
                        }
                        if (!known) {
                            lua_pushvalue(L, live_item);
                            lua_rawseti(L, snapshot, ++len);
                        }
                        lua_pop(L, 1);
                    }
                }
            }
        }

        lua_pop(L, 1);
    }
    return 0;
}

int _lua_chr(lua_State* L) {
    // PICO-8's chr(n, ...) converts every argument to a P8SCII byte and
    // returns their concatenation. In particular, chr(peek(addr, count))
    // is its compact binary-to-string transfer idiom. Explicit lengths in
    // luaL_Buffer preserve embedded NUL bytes.
    int argcount = lua_gettop(L);
    luaL_Buffer buffer;
    // Counted peek() can feed hundreds or thousands of results directly to
    // chr(). Reserve the final string once: incremental luaL_addchar growth
    // may place intermediate buffer objects above that still-live argument
    // list, which is needlessly fragile for packed multicart data and costs
    // extra allocations. A fixed output pointer also makes this path O(n)
    // with one allocation.
    char* output = luaL_buffinitsize(L, &buffer, (size_t)argcount);
    for (int arg = 1; arg <= argcount; ++arg) {
        int n = p8_checkinteger(L, arg);
        if (n < 0) n = 0;
        if (n > 255) n = 255;
        output[arg - 1] = (char)n;
    }
    luaL_pushresultsize(&buffer, (size_t)argcount);
    return 1;
}

int _lua_ord(lua_State* L) {
    // PICO-8's ord(s, [index], [num_results]) can return a whole byte range;
    // loader carts use those multiple results directly as poke arguments.
    int idx = (int)p8_optinteger(L, 2, 1);
    int count = (int)p8_optinteger(L, 3, 1);
    if (count <= 0) return 0;
    if (!lua_checkstack(L, count))
        return luaL_error(L, "ord result range is too large");

    // PICO-8's multi-result form doubles as a compact zero-fill idiom:
    // ord(nil, 1, n) yields n zero bytes. Oblivion Eve passes these results
    // directly to poke() to clear its multicart hand-off area.
    if (lua_isnoneornil(L, 1)) {
        if (lua_gettop(L) < 3) {
            lua_pushnil(L);
            return 1;
        }
        for (int i = 0; i < count; ++i) lua_pushinteger(L, 0);
        return count;
    }

    // stat(31) returns false when no keyboard character is pending. PICO-8
    // permits ord(false) and yields no character; treating it as a required
    // string stops keyboard-aware carts such as Picopicotron every frame.
    if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        return 1;
    }

    size_t len = 0;
    const char* s = luaL_checklstring(L, 1, &len);
    if (idx < 1 || idx > (int)len) {
        lua_pushnil(L);
        return 1;
    }

    int available = (int)len - idx + 1;
    if (count > available) count = available;

    const unsigned char *bytes = (const unsigned char *)s + idx - 1;
    for (int i = 0; i < count; ++i)
        lua_pushinteger(L, bytes[i]);
    return count;
}

static void p8_split_store_field(lua_State *L, int table_index, int *count,
                                 const char *field, size_t field_len,
                                 bool convert) {
    lua_pushlstring(L, field, field_len);
    if (convert && field_len > 0 && lua_isnumber(L, -1)) {
        lua_Number value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_pushnumber(L, value);
    }
    lua_rawseti(L, table_index, ++*count);
}

int _lua_split(lua_State* L) {
    // PICO-8 returns nil when the source itself is nil. This distinction from
    // an empty string matters: carts use the result as an optional-record
    // test before unpacking it (The Lost Night is one example).
    if (lua_isnoneornil(L, 1)) {
        lua_pushnil(L);
        return 1;
    }

    // PICO-8's split(s, [separator], [convert_numbers]): splits a string
    // by separator (default ",") and returns a table.  When
    // convert_numbers is true (default), numeric substrings are
    // converted to numbers.  Common in data-loading carts.
    size_t slen = 0;
    const char* s = luaL_checklstring(L, 1, &slen);
    const char* sep = ",";
    size_t seplen = 1;
    int chunk_size = 0;
    int separator_type = lua_type(L, 2);
    if (separator_type == LUA_TNUMBER) {
        chunk_size = p8_checkinteger(L, 2);
        if (chunk_size <= 0) chunk_size = 1;
    } else if (separator_type == LUA_TSTRING) {
        sep = lua_tolstring(L, 2, &seplen);
    }
    // PICO-8 and the original PicoPico implementation leave the default
    // comma separator in place for other types. Some released carts use
    // split(data, true) to request the default number conversion explicitly.
    bool convert = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3);

    lua_newtable(L);
    int tbl_idx = lua_gettop(L);
    int count = 0;

    // A numeric separator is a byte-group size, not the textual rendering
    // of that number. Preserve a shorter final group.
    if (chunk_size > 0) {
        for (size_t offset = 0; offset < slen; offset += chunk_size) {
            size_t field_len = slen - offset;
            if (field_len > (size_t)chunk_size) field_len = chunk_size;
            p8_split_store_field(L, tbl_idx, &count, s + offset, field_len,
                                 convert);
        }
        return 1;
    }

    // PICO-8 treats an empty separator as a request to split into individual
    // P8SCII bytes. Apply the same numeric conversion as ordinary fields.
    // Explicit lengths preserve embedded NUL bytes and avoid locale or
    // multibyte character processing.
    if (seplen == 0) {
        for (size_t i = 0; i < slen; ++i) {
            p8_split_store_field(L, tbl_idx, &count, s + i, 1, convert);
        }
        return 1;
    }

    if (slen == 0) {
        if (seplen > 0) {
            lua_pushlstring(L, s, slen);
            lua_rawseti(L, tbl_idx, 1);
        }
        return 1;
    }

    const char* p = s;
    const char* end = s + slen;
    while (p <= end) {
        const char* found = NULL;
        if (seplen > 0 && seplen <= (size_t)(end - p)) {
            // Search for separator
            for (const char* q = p; q <= end - seplen; q++) {
                if (memcmp(q, sep, seplen) == 0) { found = q; break; }
            }
        }
        size_t partlen = found ? (size_t)(found - p) : (size_t)(end - p);
        if (partlen > 0 || found) {
            p8_split_store_field(L, tbl_idx, &count, p, partlen, convert);
        }
        if (!found) break;
        p = found + seplen;
        // Handle trailing separator: add empty string if at end
        if (p == end) {
            lua_pushstring(L, "");
            lua_rawseti(L, tbl_idx, ++count);
            break;
        }
    }
    return 1;
}

int _lua_fillp(lua_State* L) {
    if (lua_gettop(L) == 0 || lua_isnoneornil(L, 1)) {
        drawstate.fill_pattern = 0;
        drawstate.fill_flags = 0;
        ram[0x5f31] = 0;
        ram[0x5f32] = 0;
        ram[0x5f33] = 0;
        return 0;
    }
    lua_Number value = lua_tonumber(L, 1);
    uint32_t bits = (uint32_t)value.bits();
    drawstate.fill_pattern = (uint16_t)(bits >> 16);
    drawstate.fill_flags = (uint8_t)((bits >> 13) & 7);
    ram[0x5f31] = (uint8_t)drawstate.fill_pattern;
    ram[0x5f32] = (uint8_t)(drawstate.fill_pattern >> 8);
    ram[0x5f33] = (drawstate.fill_flags & 4) ? 1 : 0;
    return 0;
}
int _lua_cursor(lua_State* L) {
    const int16_t old_cursor_x = drawstate.cursor_x;
    const int16_t old_cursor_y = drawstate.cursor_y;
    const int16_t old_cursor_c = drawstate.pen_color;

	const int16_t x = p8_optinteger(L, 1, 0);
    const int16_t y = p8_optinteger(L, 2, 0);
    const int16_t paletteIdx = p8_optinteger(L, 3, drawstate.pen_color);

	drawstate.cursor_x = x;
	drawstate.cursor_y = y;
	drawstate.pen_color = paletteIdx;
    ram[0x5f25] = drawstate.pen_color;
    ram[0x5f26] = drawstate.cursor_x;
    ram[0x5f27] = drawstate.cursor_y;

    lua_pushnumber(L, old_cursor_x);
    lua_pushnumber(L, old_cursor_y);
    lua_pushnumber(L, old_cursor_c);

    return 3;
}

int _lua_cstore(lua_State* L) {
    // cstore([dest], [source], [len], [filename]) copies live PICO-8 RAM
    // into the cartridge-data image subsequently read by reload(). Keep the
    // overlay in memory: mutating the user's ROM on SD would violate
    // Retro-Go's ROM ownership model and turn an emulator API call into an
    // irreversible filesystem write.
    const int dst_signed = p8_optinteger(L, 1, 0);
    const int src_signed = p8_optinteger(L, 2, 0);
    const int len_signed = p8_optinteger(L, 3, P8_CART_ROM_SIZE);
    if (dst_signed < 0 || src_signed < 0 || len_signed <= 0 || !cart_rom)
        return 0;

    uint32_t dst = (uint32_t)dst_signed;
    uint32_t src = (uint32_t)src_signed;
    uint32_t len = (uint32_t)len_signed;
    if (dst >= P8_CART_ROM_SIZE || src >= P8_RAM_SIZE) return 0;

    const uint32_t dst_cap = P8_CART_ROM_SIZE - dst;
    const uint32_t src_cap = P8_RAM_SIZE - src;
    if (len > dst_cap) len = dst_cap;
    if (len > src_cap) len = src_cap;

    if (lua_gettop(L) >= 4 && !lua_isnoneornil(L, 4)) {
        const char *filename = lua_tostring(L, 4);
        if (filename && filename[0]) {
            if (!cross_cart_store_data)
                cross_cart_store_data =
                    (uint8_t *)rg_alloc(P8_CART_ROM_SIZE, MEM_SLOW);
            if (!cross_cart_store_data)
                return luaL_error(L, "cstore: unable to allocate cart overlay");
            if (cross_cart_store_range_count >= P8_CROSS_CART_STORE_RANGES)
                return luaL_error(L, "cstore: too many cross-cart ranges");

            for (uint32_t i = 0; i < len; ++i)
                cross_cart_store_data[dst + i] = p8_ram_read(src + i);
            cross_cart_store_ranges[cross_cart_store_range_count++] = {
                (uint16_t)dst, (uint16_t)len
            };
            cross_cart_store_pending = true;
            RG_LOGI("pico8: staged cstore %04lx..%04lx for %s",
                    (unsigned long)dst,
                    (unsigned long)(dst + len - 1), filename);
            return 0;
        }
    }

    for (uint32_t i = 0; i < len; ++i)
        cart_rom[dst + i] = p8_ram_read(src + i);

    const size_t written_end = (size_t)dst + len;
    if (cart_rom_len < written_end) cart_rom_len = written_end;
    return 0;
}

int _lua_reload(lua_State* L) {
    // reload([dest], [source], [len], [filename]) restores bytes from the
    // immutable cartridge image. With no arguments PICO-8 restores the whole
    // 0x0000..0x42ff cartridge-data window.
    // A filename reads a same-directory companion through the Retro-Go
    // adapter without replacing the running cart or mutating anything on SD.
    const uint8_t *source_rom = cart_rom;
    size_t source_rom_len = cart_rom_len;
    uint8_t *external_rom = NULL;
    if (lua_gettop(L) >= 4 && !lua_isnoneornil(L, 4)) {
        const char *filename = lua_tostring(L, 4);
        if (filename && filename[0]) {
            external_rom = (uint8_t *)rg_alloc(P8_CART_ROM_SIZE, MEM_SLOW);
            if (!external_rom)
                return luaL_error(L, "reload: unable to allocate cart buffer");
            if (!pico8_read_cart_rom(filename, external_rom,
                                     P8_CART_ROM_SIZE, &source_rom_len)) {
                free(external_rom);
                return luaL_error(L, "reload: unable to read %s", filename);
            }
            source_rom = external_rom;
        }
    }

    uint32_t dst = (uint16_t)p8_optinteger(L, 1, 0);
    uint32_t src = (uint16_t)p8_optinteger(L, 2, 0);
    lua_Integer len_signed = p8_optinteger(L, 3, 0x4300);
    if (len_signed <= 0 || !source_rom || source_rom_len == 0) {
        free(external_rom);
        return 0;
    }
    if (dst >= P8_RAM_SIZE || src >= source_rom_len) {
        free(external_rom);
        return 0;
    }

    uint32_t len = (uint32_t)len_signed;
    uint32_t dst_cap = P8_RAM_SIZE - dst;
    uint32_t src_cap = (uint32_t)source_rom_len - src;
    if (len > dst_cap) len = dst_cap;
    if (len > src_cap) len = src_cap;

    // Use the canonical write helper so the fast expanded sprite/map/flag
    // views and framebuffer remain coherent with raw PICO-8 RAM.
    for (uint32_t i = 0; i < len; ++i)
        p8_ram_write(dst + i, source_rom[src + i]);
    free(external_rom);
    return 0;
}

static void p8_copy_cart_request_text(char *dst, size_t dst_size,
                                      const char *src) {
    if (dst_size == 0) return;
    if (src == NULL) src = "";
    size_t len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

int _lua_load(lua_State* L) {
    // A cart cannot replace its own Lua state while this C function is on
    // that state's call stack. Record the request and let the outer Retro-Go
    // frame loop perform the swap after the current protected call returns.
    const char *filename = luaL_checkstring(L, 1);
    const char *breadcrumb = lua_isnoneornil(L, 2)
                           ? "" : luaL_checkstring(L, 2);
    const char *param = lua_isnoneornil(L, 3)
                      ? "" : luaL_checkstring(L, 3);

    p8_copy_cart_request_text(pending_cart_filename,
                              sizeof(pending_cart_filename), filename);
    p8_copy_cart_request_text(pending_cart_breadcrumb,
                              sizeof(pending_cart_breadcrumb), breadcrumb);
    p8_copy_cart_request_text(pending_cart_param,
                              sizeof(pending_cart_param), param);
    pending_cart_request = P8_CART_REQUEST_LOAD;
    return 0;
}

int _lua_run(lua_State* L) {
    // Like load(), run() must wait until the active protected Lua call has
    // returned before replacing its VM. PARAM_STR is optional and becomes
    // available to the restarted cart through stat(6).
    const char *param = lua_isnoneornil(L, 1)
                      ? "" : luaL_checkstring(L, 1);
    pending_cart_filename[0] = '\0';
    pending_cart_breadcrumb[0] = '\0';
    p8_copy_cart_request_text(pending_cart_param,
                              sizeof(pending_cart_param), param);
    pending_cart_request = P8_CART_REQUEST_RUN;
    return 0;
}

int _extcmd(lua_State* L) {
    const char* val = luaL_checkstring(L, 1);
    if (strcmp(val, "breadcrumb") == 0) {
        pending_cart_request = P8_CART_REQUEST_BREADCRUMB;
        pending_cart_filename[0] = '\0';
        pending_cart_breadcrumb[0] = '\0';
        pending_cart_param[0] = '\0';
        return 0;
    }
    printf("Got extcmd: %s, ignoring\n", val);
    return 0;
}

int _lua_poke4(lua_State* L) {
    // PICO-8 poke4(addr, v1, v2, ...) writes each value's raw 16.16 bit
    // pattern as four bytes at offsets addr+0, addr+4, ... . Converting v
    // through p8_checkinteger() loses the fractional half, which breaks
    // binary checksums and ordinary values such as poke4(addr, 3.5).
    //
    // The earlier implementation had three compounding bugs:
    //   1) Used 4-bit nibble masks (0x000f, 0x00f0, 0x0f00, 0xf000) shifted
    //      right by 0/8/16/24 — that collapses the upper three bytes of
    //      every arg to 0. Only `value & 0x0f` ever reached ram, and even
    //      that as a single byte.
    //   2) Frontbuffer branch walked with `arg*sizeof(uint8_t)=arg*1`
    //      stride, so consecutive args' 4-byte writes OVERLAPPED by 3
    //      bytes each.
    //   3) No bounds check — any poke4 past ram smashed adjacent BSS.
    //
    // The new implementation uses 1-byte masks (0x000000ff etc.), 4-byte
    // stride, bounds-guarded write, and mirrors into the visible frontbuffer
    // when the address falls in the screen region so carts that screen-blit
    // via poke4 see the pixels on the next gfx_flip without a per-frame sync.
    int argcount = lua_gettop(L);
    if (argcount==0) return 0;
    uint32_t addr = (uint16_t)p8_checkinteger(L, 1);
    // Upfront addr guard: a cart that passes an address beyond the usable
    // 32-KB PICO-8 RAM (e.g. 0xFFFFFFFC from a negative lua_Integer cast)
    // would otherwise let addr+arg*4 overflow uint32_t and slip past the
    // per-iteration bounds check.
    if (addr > P8_RAM_SIZE - 4) return 0;
    for (int arg = 0; arg < argcount - 1; ++arg) {
        uint32_t value = p8_checkfixedbits(L, 2 + arg);
        uint32_t offset = addr + (uint32_t)arg * 4;
        if (offset + 4 > P8_RAM_SIZE) break;
        p8_ram_write(offset + 0, (uint8_t)(value >> 0));
        p8_ram_write(offset + 1, (uint8_t)(value >> 8));
        p8_ram_write(offset + 2, (uint8_t)(value >> 16));
        p8_ram_write(offset + 3, (uint8_t)(value >> 24));
    }
    return 0;
}
inline void _fast_render(Spritesheet* s, uint16_t sx, uint16_t sy, int16_t x0, int16_t y0) {
    uint16_t val;

    int16_t ymin = MAX(0, -(y0-drawstate.camera_y));
    int16_t xmin = MAX(0, -(x0-drawstate.camera_x));

    int16_t ymax = 8;
    int16_t xmax = 8;

    ymax = MAX(0, MIN(SCREEN_HEIGHT-(int16_t)(y0-drawstate.camera_y), ymax));
    xmax = MAX(0, MIN(SCREEN_WIDTH -(int16_t)(x0-drawstate.camera_x), xmax));

    xmin = MIN(xmin, xmax);
    ymin = MIN(ymin, ymax);

    if(xmin>=xmax) return;

    for (uint16_t y=ymin; y<ymax; y++) {
        int16_t screen_y = y0+y-drawstate.camera_y;

        for (uint16_t x=xmin; x<xmax; x++) {
            uint16_t screen_x = x0+x-drawstate.camera_x;
            // if (screen_x >= SCREEN_WIDTH) break;
            val = s->sprite_data[(sy+y)*128 + x + sx];
            if (drawstate.transparent[val] == 0) {
                put_sprite_pixel(screen_x, screen_y, val);
            }
        }
    }
}

static inline bool p8_relocated_sprite_base(Spritesheet *sheet,
                                             uint32_t *base) {
    if (sheet != &spritesheet || ram[0x5f54] == 0) return false;
    uint32_t candidate = (uint32_t)ram[0x5f54] << 8;
    if (candidate > P8_RAM_SIZE - 0x2000) return false;
    *base = candidate;
    return true;
}

static inline uint8_t p8_relocated_sprite_pixel(uint32_t base, int x, int y) {
    uint32_t nibble = (uint32_t)y * 128u + (uint32_t)x;
    uint8_t packed = p8_ram_read(base + (nibble >> 1));
    return (nibble & 1u) ? (packed >> 4) : (packed & 0x0f);
}

void _render(Spritesheet* s, uint16_t sx, uint16_t sy, int16_t x0, int16_t y0, int paletteIdx, bool flip_x, bool flip_y, z8::fix32 width, z8::fix32 height) {
    int sprite_w = z8::fix32::ceil(8 * width);
    int sprite_h = z8::fix32::ceil(8 * height);
    if (sprite_w <= 0 || sprite_h <= 0 || sx >= 128 || sy >= 128) return;

    // Bound the source rectangle before applying flips. Flip coordinates are
    // relative to this full rectangle, not the clipped destination subset.
    sprite_w = MIN(sprite_w, 128 - (int)sx);
    sprite_h = MIN(sprite_h, 128 - (int)sy);

    const int dst_x = x0 - drawstate.camera_x;
    const int dst_y = y0 - drawstate.camera_y;
    const int clip_right = drawstate.clip_x + drawstate.clip_w;
    const int clip_bottom = drawstate.clip_y + drawstate.clip_h;

    // Intersect screen and draw clip once per sprite. Every coordinate in the
    // inner loops is safe for the unchecked packed-pixel store.
    int xmin = MAX(0, MAX(-dst_x, (int)drawstate.clip_x - dst_x));
    int ymin = MAX(0, MAX(-dst_y, (int)drawstate.clip_y - dst_y));
    int xmax = MIN(sprite_w, MIN(SCREEN_WIDTH - dst_x, clip_right - dst_x));
    int ymax = MIN(sprite_h, MIN(SCREEN_HEIGHT - dst_y, clip_bottom - dst_y));
    if (xmin >= xmax || ymin >= ymax) return;

    const bool override_color = paletteIdx >= 0;
    const uint8_t mapped_override = override_color
        ? pal_map[(uint8_t)paletteIdx & 0x0f] : 0;

    // Sprite fill patterns are uncommon and substantially more expensive than
    // an ordinary blit. Keep them in a separate path so the established
    // packed/direct renderer for normal carts pays no per-pixel flag branch.
    if (drawstate.fill_flags & 2) {
        uint32_t pattern_base;
        const bool relocated =
            p8_relocated_sprite_base(s, &pattern_base);
        for (int y = ymin; y < ymax; ++y) {
            const int source_y = flip_y ? (sprite_h - 1 - y) : y;
            const uint8_t screen_y = (uint8_t)(dst_y + y);
            for (int x = xmin; x < xmax; ++x) {
                const int source_x = flip_x ? (sprite_w - 1 - x) : x;
                const uint8_t value = relocated
                    ? p8_relocated_sprite_pixel(
                        pattern_base, sx + source_x, sy + source_y)
                    : s->sprite_data[
                        (sy + source_y) * 128 + sx + source_x];
                if (drawstate.transparent[value]) continue;
                put_sprite_pixel((uint8_t)(dst_x + x), screen_y,
                                 override_color
                                     ? (uint8_t)paletteIdx : value);
            }
        }
        return;
    }

    uint32_t relocated_base;
    if (p8_relocated_sprite_base(s, &relocated_base)) {
        for (int y = ymin; y < ymax; ++y) {
            const int source_y = flip_y ? (sprite_h - 1 - y) : y;
            const uint8_t screen_y = (uint8_t)(dst_y + y);

            for (int x = xmin; x < xmax; ++x) {
                const int source_x = flip_x ? (sprite_w - 1 - x) : x;
                const uint8_t value = p8_relocated_sprite_pixel(
                    relocated_base, sx + source_x, sy + source_y);
                if (drawstate.transparent[value]) continue;

                const uint8_t mapped = override_color
                    ? mapped_override : pal_map[value & 0x0f];
                put_pixel_mapped_unchecked((uint8_t)(dst_x + x),
                                           screen_y, mapped);
            }
        }
        return;
    }

    for (int y = ymin; y < ymax; ++y) {
        const int source_y = flip_y ? (sprite_h - 1 - y) : y;
        const uint8_t *source = s->sprite_data +
            (sy + source_y) * 128 + sx;
        const uint8_t screen_y = (uint8_t)(dst_y + y);

        for (int x = xmin; x < xmax; ++x) {
            const int source_x = flip_x ? (sprite_w - 1 - x) : x;
            const uint8_t value = source[source_x];
            if (drawstate.transparent[value]) continue;

            const uint8_t mapped = override_color
                ? mapped_override : pal_map[value & 0x0f];
            put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y, mapped);
        }
    }
    return;

#if 0
    palidx_t p;
    uint16_t val;

    int16_t ymin = MAX(0, -(y0-drawstate.camera_y));
    int16_t xmin = MAX(0, -(x0-drawstate.camera_x));

    int16_t ymax = z8::fix32::ceil(8*height);
    int16_t xmax = z8::fix32::ceil(8*width);

//    ymax = MAX(0, MIN((SCREEN_HEIGHT-1)-(int16_t)(y0-drawstate.camera_y+ymax), ymax));
//    xmax = MAX(0, MIN((SCREEN_WIDTH -1)-(int16_t)(x0-drawstate.camera_x+xmax), xmax));

    // Defensive: clip `xmax`/`ymax` against the spritesheet footprint so
    // carts that pass `spr(n, x, y, w, h)` with `w>1` or `h>1` near the
    // bottom-right edge of the 128x128 spritesheet don't OOB-read
    // `s->sprite_data`. The default spr() call uses w=h=1, but Celeste and
    // similar carts call spr() with larger dimensions for scaled tiles.
    // Without this clamp, spr(254, x, y, 2, 2) would request pixels from
    // sprite_data[127*128+x+sx]+8 which lives past the 16384-byte buffer.
    if (sy >= 128 || sx >= 128) return;
    if (128 - sy < ymax) ymax = (int16_t)(128 - sy);
    if (128 - sx < xmax) xmax = (int16_t)(128 - sx);

    xmin = MIN(xmin, xmax);
    ymin = MIN(ymin, ymax);

    if(xmin>=xmax) return;

	for (int16_t y=ymin; y<ymax; y++) {
		int16_t screen_y = y0+y-drawstate.camera_y;
		//if (screen_y < 0) continue;
		if (screen_y >= SCREEN_HEIGHT) return;

		for (int16_t x=xmin; x<xmax; x++) {
			int16_t screen_x = x0+x-drawstate.camera_x;

			if (screen_x >= SCREEN_WIDTH) break;
			int16_t source_x = flip_x ? (xmax - 1 - x) : x;
			int16_t source_y = flip_y ? (ymax - 1 - y) : y;
			val = s->sprite_data[(sy+source_y)*128 + source_x + sx];
			if (drawstate.transparent[val] != 0) {
				continue;
			}

			if (paletteIdx != -1) {
				p = paletteIdx;
			} else {
				p = val;
			}

			put_sprite_pixel(screen_x, screen_y, p);

		}
	}
#endif
}

inline void render_many(Spritesheet* s, uint16_t n, int16_t x0, int16_t y0, int paletteIdx, bool flip_x, bool flip_y, z8::fix32 width, z8::fix32 height) {
    const uint8_t xIndex = n % 16;
    const uint8_t yIndex = n / 16;
    _render(s, xIndex*8, yIndex*8, x0, y0, paletteIdx, flip_x, flip_y, width, height);
}

inline void render(Spritesheet* s, uint16_t n, uint16_t x0, uint16_t y0, int paletteIdx, bool flip_x, bool flip_y) {
    const uint8_t xIndex = n % 16;
    const uint8_t yIndex = n / 16;
    // The trailing `1, 1` are z8::fix32 width/height arguments; cast through
    // int32_t to disambiguate the ctor from `int` literal `1` in C++.
    _render(s, xIndex*8, yIndex*8, x0, y0, paletteIdx, flip_x, flip_y,
            z8::fix32(int32_t{1}), z8::fix32(int32_t{1}));
}

void render_stretched(Spritesheet* s, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh, bool flip_x, bool flip_y) {

    // fix32 has no operator/(int32_t); cast the int divisor through
    // z8::fix32 explicitly (same pattern as synth.c::ret / 9 and
    // sfx.c::freq / SAMPLE_RATE).
    if (sw <= 0 || sh <= 0 || dw == 0 || dh == 0) return;

    // PICO-8 accepts negative SSPR destination dimensions. DX/DY then name
    // the opposite edge and the image is mirrored on that axis. PICO-BALL
    // uses a width that crosses through zero while its player turns; treating
    // every negative width as empty made the character disappear for half of
    // the turn. Normalise once here so both the 1:1 and scaled hot paths can
    // keep their existing positive-size loops.
    if (dw < 0) {
        dx += dw;
        dw = -dw;
        flip_x = !flip_x;
    }
    if (dh < 0) {
        dy += dh;
        dh = -dh;
        flip_y = !flip_y;
    }
    // Destination coordinates are in world space. Reject only after applying
    // camera(), otherwise sspr() sprites on later rooms (world x >= 128) are
    // incorrectly discarded even though the camera places them on screen.
    // Across the River flips actors with eight 1-pixel sspr() strips and
    // exposes this on its right-hand bank.
    const int screen_dx = dx - drawstate.camera_x;
    const int screen_dy = dy - drawstate.camera_y;
    if (screen_dx >= SCREEN_WIDTH || screen_dy >= SCREEN_HEIGHT ||
        screen_dx + dw <= 0 || screen_dy + dh <= 0) return;

    // sspr() is commonly used as a variable-sized sprite blit with no
    // scaling at all (sw==dw, sh==dh). Cattle Crisis draws almost every
    // actor this way. The generic scaler below recalculates fixed-point
    // source coordinates and repeats clip/palette checks for every pixel,
    // even in that 1:1 case. Intersect source, destination and draw clip once
    // and use the same unchecked packed-framebuffer store as spr().
    if (sw == dw && sh == dh) {
        const int dst_x = screen_dx;
        const int dst_y = screen_dy;
        const int clip_right = drawstate.clip_x + drawstate.clip_w;
        const int clip_bottom = drawstate.clip_y + drawstate.clip_h;

        int xmin = MAX(0, MAX(-dst_x, (int)drawstate.clip_x - dst_x));
        int ymin = MAX(0, MAX(-dst_y, (int)drawstate.clip_y - dst_y));
        int xmax = MIN(dw, MIN(SCREEN_WIDTH - dst_x, clip_right - dst_x));
        int ymax = MIN(dh, MIN(SCREEN_HEIGHT - dst_y, clip_bottom - dst_y));

        // Clip against the source sheet while preserving flip coordinates
        // relative to the cart's complete requested source rectangle.
        if (flip_x) {
            xmin = MAX(xmin, sx + sw - 128);
            xmax = MIN(xmax, sx + sw);
        } else {
            xmin = MAX(xmin, -sx);
            xmax = MIN(xmax, 128 - sx);
        }
        if (flip_y) {
            ymin = MAX(ymin, sy + sh - 128);
            ymax = MIN(ymax, sy + sh);
        } else {
            ymin = MAX(ymin, -sy);
            ymax = MIN(ymax, 128 - sy);
        }
        if (xmin >= xmax || ymin >= ymax) return;

        if (drawstate.fill_flags & 2) {
            uint32_t pattern_base;
            const bool relocated =
                p8_relocated_sprite_base(s, &pattern_base);
            for (int y = ymin; y < ymax; ++y) {
                const int source_y =
                    sy + (flip_y ? sh - 1 - y : y);
                const uint8_t screen_y = (uint8_t)(dst_y + y);
                for (int x = xmin; x < xmax; ++x) {
                    const int source_x =
                        sx + (flip_x ? sw - 1 - x : x);
                    const uint8_t value = relocated
                        ? p8_relocated_sprite_pixel(
                            pattern_base, source_x, source_y)
                        : s->sprite_data[source_y * 128 + source_x];
                    if (!drawstate.transparent[value])
                        put_sprite_pixel((uint8_t)(dst_x + x),
                                         screen_y, value);
                }
            }
            return;
        }

        uint32_t relocated_base;
        if (p8_relocated_sprite_base(s, &relocated_base)) {
            for (int y = ymin; y < ymax; ++y) {
                const int source_y = sy + (flip_y ? sh - 1 - y : y);
                const uint8_t screen_y = (uint8_t)(dst_y + y);
                for (int x = xmin; x < xmax; ++x) {
                    const int source_x = sx + (flip_x ? sw - 1 - x : x);
                    const uint8_t value = p8_relocated_sprite_pixel(
                        relocated_base, source_x, source_y);
                    if (drawstate.transparent[value]) continue;
                    put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y,
                                               pal_map[value & 0x0f]);
                }
            }
            return;
        }

        for (int y = ymin; y < ymax; ++y) {
            const int source_y = sy + (flip_y ? sh - 1 - y : y);
            const uint8_t *source = s->sprite_data + source_y * 128 +
                (flip_x ? sx + sw - 1 : sx);
            const int source_step = flip_x ? -1 : 1;
            const uint8_t screen_y = (uint8_t)(dst_y + y);
            int x = xmin;

            // Align the packed destination so each following iteration owns
            // one complete byte (low nibble first, then high nibble).
            if ((dst_x + x) & 1) {
                const uint8_t value = source[x * source_step];
                if (!drawstate.transparent[value])
                    put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y,
                                               pal_map[value & 0x0f]);
                ++x;
            }

            for (; x + 1 < xmax; x += 2) {
                put_sprite_pair_unchecked(
                    (uint8_t)(dst_x + x), screen_y,
                    source[x * source_step],
                    source[(x + 1) * source_step]);
            }

            if (x < xmax) {
                const uint8_t value = source[x * source_step];
                if (!drawstate.transparent[value])
                    put_pixel_mapped_unchecked((uint8_t)(dst_x + x), screen_y,
                                               pal_map[value & 0x0f]);
            }
        }
        return;
    }

    // Scaled blits must obey the active draw clip just like the 1:1 path.
    // Wolfenstein renders walls as 1-pixel source columns stretched to the
    // wall height; without this intersection nearby walls painted over its
    // persistent 15-pixel HUD despite clip(0,0,128,113).
    const int clip_right = drawstate.clip_x + drawstate.clip_w;
    const int clip_bottom = drawstate.clip_y + drawstate.clip_h;
    const int xmin = MAX(0, MAX(-screen_dx,
                               (int)drawstate.clip_x - screen_dx));
    const int ymin = MAX(0, MAX(-screen_dy,
                               (int)drawstate.clip_y - screen_dy));
    const int xmax = MIN(dw, MIN(SCREEN_WIDTH - screen_dx,
                                 clip_right - screen_dx));
    const int ymax = MIN(dh, MIN(SCREEN_HEIGHT - screen_dy,
                                 clip_bottom - screen_dy));
    if (xmin >= xmax || ymin >= ymax) return;

    uint32_t ratio_x = ((uint32_t)sw << 16) / (uint32_t)dw;
    uint32_t ratio_y = ((uint32_t)sh << 16) / (uint32_t)dh;

    uint32_t relocated_base;
    if (p8_relocated_sprite_base(s, &relocated_base)) {
        for (int y = ymin; y < ymax; ++y) {
            int screen_y = screen_dy + y;

            int sample_y = (int)(((uint32_t)y * ratio_y) >> 16);
            if (flip_y) sample_y = sh - 1 - sample_y;
            sample_y += sy;
            if (sample_y < 0 || sample_y >= 128) continue;

            for (int x = xmin; x < xmax; ++x) {
                int sample_x = (int)(((uint32_t)x * ratio_x) >> 16);
                if (flip_x) sample_x = sw - 1 - sample_x;
                sample_x += sx;
                if (sample_x < 0 || sample_x >= 128) continue;

                int screen_x = screen_dx + x;
                uint8_t value = p8_relocated_sprite_pixel(
                    relocated_base, sample_x, sample_y);
                if (!drawstate.transparent[value])
                    put_sprite_pixel((uint8_t)screen_x,
                                     (uint8_t)screen_y, value);
            }
        }
        return;
    }

    for (int y=ymin; y<ymax; y++) {
        int16_t screen_y = screen_dy+y;
        int sample_y = (int)(((uint32_t)y * ratio_y) >> 16);
        if (flip_y) sample_y = sh - 1 - sample_y;
        sample_y += sy;
        if (sample_y < 0 || sample_y >= 128) continue;
        int yoff = sample_y * 128;

        for (int x=xmin; x<xmax; x++) {
	    //if(dx+x-drawstate.camera_x < 0) continue;
	    //if(dx+x-drawstate.camera_x >= SCREEN_WIDTH) continue;
            // PICO-8 has 16 PICO-8 colors (indices 0..15) — DO NOT mask. The
            // previous `% 15` collapsed pixel value 15 (peach) to 0
            // (transparent), which manifested as opaque "boxes" / "holes"
            // wherever carts used `sspr` to blit backgrounds or sprites
            // that contained color-15 pixels. PICO-8's transparency comes
            // purely from the per-color table set via `palt()`, never from
            // masking the value here.
            int sample_x = (int)(((uint32_t)x * ratio_x) >> 16);
            if (flip_x) sample_x = sw - 1 - sample_x;
            sample_x += sx;
            if (sample_x < 0 || sample_x >= 128) continue;
            int screen_x = screen_dx + x;
            uint8_t val = s->sprite_data[yoff + sample_x];
            if (drawstate.transparent[val] == 0){
                put_sprite_pixel((uint8_t)screen_x, (uint8_t)screen_y, val);
            }
        }
    }
}
