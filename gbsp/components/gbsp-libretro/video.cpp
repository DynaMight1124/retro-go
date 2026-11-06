/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2023 David Guillen Fandos <david@davidgf.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

extern "C" {
  #include "common.h"
}

u16* gba_screen_pixels = NULL;

#include "gpsp_video.h"
#include "gpsp_log.h"
#define GPSP_VIDEO_SNAPSHOTS 1
#define get_screen_pixels()   gba_screen_pixels

#if defined(GPSP_VIDEO_SNAPSHOTS)
/* The renderer (core 1, drawing lines late) reads its own copy of VRAM:
   CPU and DMA write the live VRAM and mark 1 KB pages dirty, and the dirty
   pages are copied into the renderer's copy when the next line is queued,
   after the lines queued before the write are drawn. A VRAM write no longer
   waits for core 1 (Sonic's vblank DMA, TMNT's OBJ tiles: ~2 ms/frame). */
#define GBSP_RVRAM 1
extern "C" { u8 *gbsp_rvram; u32 gbsp_vram_dirty[3]; u8 gbsp_vram_dirty_any; void gbsp_vram_mark_all(void); }
static inline u8 *gbsp_live_vram(void) { return vram; }   /* before vram is redefined */
#define live_vram gbsp_live_vram()
#undef vram
static u8 *render_vram;
#define vram render_vram   /* everything below is the renderer's view */
#endif

#ifdef GPSP_VIDEO_SNAPSHOTS
/* esp32-emu-turbo: the scanline renderer reads a snapshot of the display
   registers and affine references taken at that line's hblank (core 0), so
   it can draw on core 1 while core 0 emulates the following lines. */
#define RLINE_IO 0x30                 /* DISPCNT .. BLDY (u16 index 0x2A) */
typedef struct { u16 io[RLINE_IO]; s32 ax[2], ay[2]; u8 oam, oamb, palb; } rline_t;
/* OAM as the line saw it: core 0 copies OAM into the other of two buffers at
   the first line after a change, so the game can rewrite OAM (in the vblank,
   for the next frame) without waiting for core 1 to draw the last lines */
#ifdef ESP_PLATFORM
static u16 (*r_oam)[512];   /* PSRAM: gbsp_render_start() */
#endif
static const u16 *render_oam = oam_ram;
static u8 r_oam_cur;
static u32 r_oam_last[2] = {~0U, ~0U};   /* index of the last queued line using it */
static bool r_oam_valid;
/* the same for the converted palette, a ring of buffers (games change it on
   several lines of a frame): palette writes set gbsp_pal_dirty (gba_memory.c).
   2026-10-13: 4 -> 32 buffers (32 KB of PSRAM). V-Rally 3 changes it on many
   lines of a frame; with 4 buffers the ring came round to one still in use
   and core 0 waited for core 1, drawing the late lines itself (GBAWAIT pal
   2.4 ms a frame, gbsp_render_sync 27 % of core 0's samples, 55.8 fps). */
#define R_PAL_N 32
#ifdef ESP_PLATFORM
static u16 (*r_pal)[512];   /* PSRAM: gbsp_render_start() */
#endif
static u16 *const live_pal = palette_ram_converted;
static const u16 *render_pal = palette_ram_converted;
static u8 r_pal_cur;
static u32 r_pal_last[R_PAL_N];
extern "C" { u32 gbsp_pal_ring = R_PAL_N; }   /* buffers in use: gba_pal4 on the card sets 4 (the A/B) */
static struct r_pal_last_init { r_pal_last_init() { for (int i = 0; i < R_PAL_N; i++) r_pal_last[i] = ~0U; } } r_pal_last_init_;
extern "C" { u8 gbsp_pal_dirty = 1; }
#define palette_ram_converted render_pal
#ifdef ESP_PLATFORM
static rline_t *rlines;   /* 160 lines, PSRAM (internal RAM is full): gbsp_render_start() */
#else
static rline_t rlines_pc[160], *rlines = rlines_pc;
static u16 r_oam_pc[2][512], (*r_oam)[512] = r_oam_pc;
static u16 r_pal_pc[R_PAL_N][512], (*r_pal)[512] = r_pal_pc;
#endif
static const u16 *render_io = io_registers;
static s32 r_affine_x[2], r_affine_y[2];
#define live_ioreg(regnum) (eswap16(io_registers[(regnum)]))
#define live_ioreg32(regnum) (live_ioreg(regnum) | (live_ioreg((regnum)+1) << 16))
#undef read_ioreg
#define read_ioreg(regnum) (eswap16(render_io[(regnum)]))
#else
#define live_ioreg(regnum) read_ioreg(regnum)
#define live_ioreg32(regnum) read_ioreg32(regnum)
#define r_affine_x affine_reference_x
#define r_affine_y affine_reference_y
#define render_oam oam_ram
#endif
#define get_screen_pitch()    GBA_SCREEN_PITCH
typedef struct {
  u16 attr0, attr1, attr2, attr3;
} t_oam;

typedef struct {
  u16 pad0[3];
  u16 dx;
  u16 pad1[3];
  u16 dmx;
  u16 pad2[3];
  u16 dy;
  u16 pad3[3];
  u16 dmy;
} t_affp;

typedef void (* bitmap_render_function)(
  u32 start, u32 end, void *dest_ptr, const u16 *pal);
typedef void (* tile_render_function)(
  u32 layer, u32 start, u32 end, void *dest_ptr, const u16 *pal);

typedef void (*render_function_u16)(
  u32 start, u32 end, u16 *scanline, u32 enable_flags);
typedef void (*render_function_u32)(
  u32 start, u32 end, u32 *scanline, u32 enable_flags);

typedef void (*window_render_function)(u16 *scanline, u32 start, u32 end);

static void render_scanline_conditional(
  u32 start, u32 end, u16 *scanline, u32 enable_flags = 0x3F);

typedef struct
{
  bitmap_render_function blit_render;
  bitmap_render_function scale_render;
  bitmap_render_function affine_render;
} bitmap_layer_render_struct;

typedef struct
{
  render_function_u16 fullcolor;
  render_function_u16 indexed_u16;
  render_function_u32 indexed_u32;
  render_function_u32 stacked;
} layer_render_struct;

// Object blending modes
#define OBJ_MOD_NORMAL     0
#define OBJ_MOD_SEMITRAN   1
#define OBJ_MOD_WINDOW     2
#define OBJ_MOD_INVALID    3

// BLDCNT color effect modes
#define COL_EFFECT_NONE   0x0
#define COL_EFFECT_BLEND  0x1
#define COL_EFFECT_BRIGHT 0x2
#define COL_EFFECT_DARK   0x3

// Background render modes
#define RENDER_NORMAL   0
#define RENDER_COL16    1
#define RENDER_COL32    2
#define RENDER_ALPHA    3


// Byte lengths of complete tiles and tile rows in 4bpp and 8bpp.

#define tile_width_4bpp   4
#define tile_size_4bpp   32
#define tile_width_8bpp   8
#define tile_size_8bpp   64

// Sprite rendering cycles
#define REND_CYC_MAX          32768   /* Theoretical max is 17920 */
#define REND_CYC_SCANLINE      1210
#define REND_CYC_REDUCED        954

// Generate bit mask (bits 9th and 10th) with information about the pixel
// status (1st and/or 2nd target) for later blending.
static inline u16 color_flags(u32 layer) {
  u32 bldcnt = read_ioreg(REG_BLDCNT);
  return (
    ((bldcnt >> layer) & 0x01) |            // 1st target
    ((bldcnt >> (layer + 7)) & 0x02)        // 2nd target
  ) << 9;
}

static const u32 map_widths[] = { 256, 512, 256, 512 };

typedef enum
{
  FULLCOLOR,  // Regular rendering, output a 16 bit color
  INDXCOLOR,  // Rendering to indexed color, so we can later apply dark/bright
  STCKCOLOR,  // Stacks two indexed pixels (+flags) to apply blending
  PIXCOPY     // Special mode used for sprites, to allow for obj-window drawing
} rendtype;

s32 affine_reference_x[2];
s32 affine_reference_y[2];

static inline s32 signext28(u32 value)
{
  s32 ret = (s32)(value << 4);
  return ret >> 4;
}

void video_reload_counters()
{
  affine_reference_x[0] = signext28(live_ioreg32(REG_BG2X_L));
  affine_reference_y[0] = signext28(live_ioreg32(REG_BG2Y_L));
  affine_reference_x[1] = signext28(live_ioreg32(REG_BG3X_L));
  affine_reference_y[1] = signext28(live_ioreg32(REG_BG3Y_L));
}


// Renders non-affine tiled background layer.
// Will process a full or partial tile (start and end within 0..8) and draw
// it in either 8 or 4 bpp mode. Honors vertical and horizontal flip.

// tile contains the tile info (contains tile index, flip bits, pal info)
// hflip causes the tile pixels lookup to be reversed (from MSB to LSB
// If isbase is not set, color 0 is interpreted as transparent, otherwise
// we are drawing the base layer, so palette[0] is used (backdrop).

template<typename dtype, rendtype rdtype, bool is8bpp, bool isbase, bool hflip>
static inline void rend_part_tile_Nbpp(u32 bg_comb, u32 px_comb,
  dtype *dest_ptr, u32 start, u32 end, u16 tile,
  const u8 *tile_base, int vertical_pixel_flip, const u16 *paltbl
) {
  // Seek to the specified tile, using the tile number and size.
  // tile_base already points to the right tile-line vertical offset
  const u8 *tile_ptr = &tile_base[(tile & 0x3FF) * (is8bpp ? 64 : 32)];
  u16 bgcolor = paltbl[0];

  // On vertical flip, apply the mirror offset
  if (tile & 0x800)
    tile_ptr += vertical_pixel_flip;

  if (is8bpp) {
    // Each byte is a color, mapped to a palete. 8 bytes can be read as 64bit
    for (u32 i = start; i < end; i++, dest_ptr++) {
      // Honor hflip by selecting bytes in the correct order
      u32 sel = hflip ? (7-i) : i;
      u8 pval = tile_ptr[sel];
      // Alhpa mode stacks previous value (unless rendering the first layer)
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = paltbl[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pval | px_comb;  // Add combine flags
        else if (rdtype == STCKCOLOR)
          // Stack pixels on top of the pixel value and combine flags
          *dest_ptr = pval | px_comb | ((isbase ? bg_comb : *dest_ptr) << 16);
      }
      else if (isbase) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = bgcolor;
        else
          *dest_ptr = 0 | bg_comb;  // Add combine flags
      }
    }
  } else {
    // In 4bpp mode, the tile[15..12] bits contain the sub-palette number.
    u16 tilepal = (tile >> 12) << 4;
    u16 pxflg = px_comb | tilepal;
    const u16 *subpal = &paltbl[tilepal];
    // Read packed pixel data, skip start pixels
    u32 tilepix = eswap32(*(u32*)tile_ptr);
    if (hflip) tilepix <<= (start * 4);
    else       tilepix >>= (start * 4);
    // Only 32 bits (8 pixels * 4 bits)
    for (u32 i = start; i < end; i++, dest_ptr++) {
      u8 pval = hflip ? tilepix >> 28 : tilepix & 0xF;
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = subpal[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pxflg | pval;
        else if (rdtype == STCKCOLOR)    // Stack pixels
          *dest_ptr = pxflg | pval | ((isbase ? bg_comb : *dest_ptr) << 16);
      }
      else if (isbase) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = bgcolor;
        else
          *dest_ptr = 0 | bg_comb;
      }
      // Advance to next packed data
      if (hflip) tilepix <<= 4;
      else       tilepix >>= 4;
    }
  }
}

// Same as above, but optimized for full tiles. Skip comments here.
template<typename dtype, rendtype rdtype, bool is8bpp, bool isbase, bool hflip>
static inline void render_tile_Nbpp(
  u32 bg_comb, u32 px_comb, dtype *dest_ptr, u16 tile,
  const u8 *tile_base, int vertical_pixel_flip, const u16 *paltbl
) {
  const u8 *tile_ptr = &tile_base[(tile & 0x3FF) * (is8bpp ? 64 : 32)];
  u16 bgcolor = paltbl[0];

  if (tile & 0x800)
    tile_ptr += vertical_pixel_flip;

  if (is8bpp) {
    for (u32 j = 0; j < 2; j++) {
      u32 tilepix = eswap32(((u32*)tile_ptr)[hflip ? 1-j : j]);
      if (tilepix) {
        for (u32 i = 0; i < 4; i++, dest_ptr++) {
          u8 pval = hflip ? (tilepix >> (24 - i*8)) : (tilepix >> (i*8));
          if (pval) {
            if (rdtype == FULLCOLOR)
              *dest_ptr = paltbl[pval];
            else if (rdtype == INDXCOLOR)
              *dest_ptr = pval | px_comb;  // Add combine flags
            else if (rdtype == STCKCOLOR)
              *dest_ptr = pval | px_comb | ((isbase ? bg_comb : *dest_ptr) << 16);
          }
          else if (isbase) {
            *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
          }
        }
      } else {
        for (u32 i = 0; i < 4; i++, dest_ptr++)
          if (isbase)
            *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
      }
    }
  } else {
    u32 tilepix = eswap32(*(u32*)tile_ptr);
    if (tilepix) {  // We can skip it all if the row is transparent
      u16 tilepal = (tile >> 12) << 4;
      u16 pxflg = px_comb | tilepal;
      const u16 *subpal = &paltbl[tilepal];
      for (u32 i = 0; i < 8; i++, dest_ptr++) {
        u8 pval = (hflip ? (tilepix >> ((7-i)*4)) : (tilepix >> (i*4))) & 0xF;
        if (pval) {
          if (rdtype == FULLCOLOR)
            *dest_ptr = subpal[pval];
          else if (rdtype == INDXCOLOR)
            *dest_ptr = pxflg | pval;
          else if (rdtype == STCKCOLOR)
            *dest_ptr = pxflg | pval | ((isbase ? bg_comb : *dest_ptr) << 16);
        }
        else if (isbase) {
          *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
        }
      }
    } else if (isbase) {
      // In this case we simply fill the pixels with background pixels
      for (u32 i = 0; i < 8; i++, dest_ptr++)
        *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
    }
  }
}


#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
template<typename stype, rendtype rdtype, bool isbase, bool is8bpp>
static inline __attribute__((always_inline)) void render_scanline_text_fast(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
#else /* GBSP_BACKEND_VARIANT */
template<typename stype, rendtype rdtype, bool isbase, bool is8bpp>
static void render_scanline_text_fast(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
#endif /* GBSP_BACKEND_VARIANT */
{
  u32 bg_control = read_ioreg(REG_BGxCNT(layer));
  u16 vcount = read_ioreg(REG_VCOUNT);
  u32 map_size = (bg_control >> 14) & 0x03;
  u32 map_width = map_widths[map_size];
  u32 hoffset = (start + read_ioreg(REG_BGxHOFS(layer))) % 512;
  u32 voffset = (vcount + read_ioreg(REG_BGxVOFS(layer))) % 512;
  stype *dest_ptr = ((stype*)scanline) + start;

  // Calculate combine masks. These store 2 bits of info: 1st and 2nd target.
  // If set, the current pixel belongs to a layer that is 1st or 2nd target.
  u32 bg_comb = color_flags(5), px_comb = color_flags(layer);

  // Background map data is in vram, at an offset specified in 2K blocks.
  // (each map data block is 32x32 tiles, at 16bpp, so 2KB)
  u32 base_block = (bg_control >> 8) & 0x1F;
  u16 *map_base = (u16 *)&vram[base_block * 2048];
  u16 *map_ptr, *second_ptr;

  end -= start;

  // Skip the top one/two block(s) if using the bottom half
  if ((map_size & 0x02) && (voffset >= 256))
    map_base += ((map_width / 8) * 32);

  // Skip the top tiles within the block
  map_base += (((voffset % 256) / 8) * 32);

  // we might need to render from two charblocks, store a second pointer.
  second_ptr = map_ptr = map_base;

  if(map_size & 0x01) {    // If background is 512 pixels wide
    if(hoffset >= 256) {
      // If we are rendering the right block, skip a whole charblock
      hoffset -= 256;
      map_ptr += (32 * 32);
    } else {
      // If we are rendering the left block, we might overrun into the right
      second_ptr += (32 * 32);
    }
  } else {
    hoffset %= 256;     // Background is 256 pixels wide
  }

  // Skip the left blocks within the block
  map_ptr += hoffset / 8;

  // Render a single scanline of text tiles
  u32 tilewidth = is8bpp ? tile_width_8bpp : tile_width_4bpp;
  u32 vert_pix_offset = (voffset % 8) * tilewidth;
  // Calculate the pixel offset between a line and its "flipped" mirror.
  // The values can be {56, 40, 24, 8, -8, -24, -40, -56}
  s32 vflip_off = is8bpp ?
       tile_size_8bpp - 2*vert_pix_offset - tile_width_8bpp :
       tile_size_4bpp - 2*vert_pix_offset - tile_width_4bpp;

  // The tilemap base is selected via bgcnt (16KiB chunks)
  u32 tilecntrl = (bg_control >> 2) & 0x03;
  // Account for the base offset plus the tile vertical offset
  u8 *tile_base = &vram[tilecntrl * 16*1024 + vert_pix_offset];
  // Number of pixels available until the end of the tile block
  u32 pixel_run = 256 - hoffset;

  u32 tile_hoff = hoffset % 8;
  u32 partial_hcnt = 8 - tile_hoff;

  if (tile_hoff) {
    // First partial tile, only right side is visible.
    u32 todraw = MIN(end, partial_hcnt); // [1..7]
    u32 stop = tile_hoff + todraw;   // Usually 8, unless short run.

    u16 tile = eswap16(*map_ptr++);
    if (tile & 0x400)   // Tile horizontal flip
      rend_part_tile_Nbpp<stype, rdtype, is8bpp, isbase, true>(
        bg_comb, px_comb, dest_ptr, tile_hoff, stop, tile, tile_base, vflip_off, paltbl);
    else
      rend_part_tile_Nbpp<stype, rdtype, is8bpp, isbase, false>(
        bg_comb, px_comb, dest_ptr, tile_hoff, stop, tile, tile_base, vflip_off, paltbl);

    dest_ptr += todraw;
    end -= todraw;
    pixel_run -= todraw;
  }

  if (!end)
    return;

  // Now render full tiles
  u32 todraw = MIN(end, pixel_run) / 8;

  for (u32 i = 0; i < todraw; i++, dest_ptr += 8) {
    u16 tile = eswap16(*map_ptr++);
    if (tile & 0x400)   // Tile horizontal flip
      render_tile_Nbpp<stype, rdtype, is8bpp, isbase, true>(
        bg_comb, px_comb, dest_ptr, tile, tile_base, vflip_off, paltbl);
    else
      render_tile_Nbpp<stype, rdtype, is8bpp, isbase, false>(
        bg_comb, px_comb, dest_ptr, tile, tile_base, vflip_off, paltbl);
  }

  end -= todraw * 8;
  pixel_run -= todraw * 8;

  if (!end)
    return;

  // Switch to the next char block if we ran out of tiles
  if (!pixel_run)
    map_ptr = second_ptr;

  todraw = end / 8;
  for (u32 i = 0; i < todraw; i++, dest_ptr += 8) {
    u16 tile = eswap16(*map_ptr++);
    if (tile & 0x400)   // Tile horizontal flip
      render_tile_Nbpp<stype, rdtype, is8bpp, isbase, true>(
        bg_comb, px_comb, dest_ptr, tile, tile_base, vflip_off, paltbl);
    else
      render_tile_Nbpp<stype, rdtype, is8bpp, isbase, false>(
        bg_comb, px_comb, dest_ptr, tile, tile_base, vflip_off, paltbl);
  }

  end -= todraw * 8;

  // Finalize the tile rendering the left side of it (from 0 up to "end").
  if (end) {
    u16 tile = eswap16(*map_ptr++);
    if (tile & 0x400)   // Tile horizontal flip
      rend_part_tile_Nbpp<stype, rdtype, is8bpp, isbase, true>(
        bg_comb, px_comb, dest_ptr, 0, end, tile, tile_base, vflip_off, paltbl);
    else
      rend_part_tile_Nbpp<stype, rdtype, is8bpp, isbase, false>(
        bg_comb, px_comb, dest_ptr, 0, end, tile, tile_base, vflip_off, paltbl);
  }
}

// A slow version of the above function that allows for mosaic effects
template<typename stype, rendtype rdtype, bool isbase, bool is8bpp>
static void render_scanline_text_mosaic(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
{
  u32 bg_control = read_ioreg(REG_BGxCNT(layer));
  const u32 mosh = (read_ioreg(REG_MOSAIC) & 0xF) + 1;
  const u32 mosv = ((read_ioreg(REG_MOSAIC) >> 4) & 0xF) + 1;
  u16 vcount = read_ioreg(REG_VCOUNT);
  u32 map_size = (bg_control >> 14) & 0x03;
  u32 map_width = map_widths[map_size];
  u32 hoffset = (start + read_ioreg(REG_BGxHOFS(layer))) % 512;
  u16 vmosoff = vcount - vcount % mosv;
  u32 voffset = (vmosoff + read_ioreg(REG_BGxVOFS(layer))) % 512;
  stype *dest_ptr = ((stype*)scanline) + start;

  u32 bg_comb = color_flags(5), px_comb = color_flags(layer);

  u32 base_block = (bg_control >> 8) & 0x1F;
  u16 *map_base = (u16 *)&vram[base_block * 2048];
  u16 *map_ptr, *second_ptr;

  if ((map_size & 0x02) && (voffset >= 256))
    map_base += ((map_width / 8) * 32);

  map_base += (((voffset % 256) / 8) * 32);

  second_ptr = map_ptr = map_base;

  if(map_size & 0x01) {    // If background is 512 pixels wide
    if(hoffset >= 256) {
      // If we are rendering the right block, skip a whole charblock
      hoffset -= 256;
      map_ptr += (32 * 32);
    } else {
      // If we are rendering the left block, we might overrun into the right
      second_ptr += (32 * 32);
    }
  } else {
    hoffset %= 256;     // Background is 256 pixels wide
  }

  // Skip the left blocks within the block
  map_ptr += hoffset / 8;

  // Render a single scanline of text tiles
  u32 tilewidth = is8bpp ? tile_width_8bpp : tile_width_4bpp;
  u32 vert_pix_offset = (voffset % 8) * tilewidth;
  // Calculate the pixel offset between a line and its "flipped" mirror.
  // The values can be {56, 40, 24, 8, -8, -24, -40, -56}
  s32 vflip_off = is8bpp ?
       tile_size_8bpp - 2*vert_pix_offset - tile_width_8bpp :
       tile_size_4bpp - 2*vert_pix_offset - tile_width_4bpp;

  // The tilemap base is selected via bgcnt (16KiB chunks)
  u32 tilecntrl = (bg_control >> 2) & 0x03;
  // Account for the base offset plus the tile vertical offset
  u8 *tile_base = &vram[tilecntrl * 16*1024 + vert_pix_offset];

  u16 bgcolor = paltbl[0];

  // Iterate pixel by pixel, loading data every N pixels to honor mosaic effect
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
  u8 pval = 0;
  u32 mctr = 0;
  for (u32 i = 0; start < end; start++, i++, dest_ptr++) {
#else /* GBSP_BACKEND_VARIANT */
  u8 pval = 0;
  for (u32 i = 0; start < end; start++, i++, dest_ptr++) {
#endif /* GBSP_BACKEND_VARIANT */
    u16 tile = eswap16(*map_ptr);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

    // Counter-based mosaic (see render_affine_background for rationale).
    if (!mctr) {
      const u8 *tile_ptr = &tile_base[(tile & 0x3FF) * (is8bpp ? 64 : 32)];
#else /* GBSP_BACKEND_VARIANT */

    if (!(i % mosh)) {
      const u8 *tile_ptr = &tile_base[(tile & 0x3FF) * (is8bpp ? 64 : 32)];
#endif /* GBSP_BACKEND_VARIANT */

      bool hflip = (tile & 0x400);
      if (tile & 0x800)
        tile_ptr += vflip_off;

      // Load byte or nibble with pixel data.
      if (is8bpp) {
        if (hflip)
          pval = tile_ptr[7 - hoffset % 8];
        else
          pval = tile_ptr[hoffset % 8];
      } else {
        if (hflip)
          pval = (tile_ptr[(7 - hoffset % 8) >> 1] >> (((hoffset & 1) ^ 1) * 4)) & 0xF;
        else
          pval = (tile_ptr[(hoffset % 8) >> 1] >> ((hoffset & 1) * 4)) & 0xF;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      }
      mctr = mosh;
    }
    mctr--;

#else /* GBSP_BACKEND_VARIANT */
      }
    }

#endif /* GBSP_BACKEND_VARIANT */
    if (is8bpp) {
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = paltbl[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pval | px_comb;  // Add combine flags
        else if (rdtype == STCKCOLOR)
          *dest_ptr = pval | px_comb | ((isbase ? bg_comb : *dest_ptr) << 16);
      }
      else if (isbase) {
        *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
      }
    } else {
      u16 tilepal = (tile >> 12) << 4;
      u16 pxflg = px_comb | tilepal;
      const u16 *subpal = &paltbl[tilepal];
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = subpal[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pxflg | pval;
        else if (rdtype == STCKCOLOR)
          *dest_ptr = pxflg | pval | ((isbase ? bg_comb : *dest_ptr) << 16);
      }
      else if (isbase) {
        *dest_ptr = (rdtype == FULLCOLOR) ? bgcolor : 0 | bg_comb;
      }
    }

    // Need to continue from the next charblock
    hoffset++;
    if (hoffset % 8 == 0)
      map_ptr++;
    if (hoffset >= 256) {
      hoffset = 0;
      map_ptr = second_ptr;
    }
  }
}

#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
template<typename stype, rendtype rdtype, bool isbase>
static inline __attribute__((always_inline)) void render_scanline_text_body(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
#else /* GBSP_BACKEND_VARIANT */
template<typename stype, rendtype rdtype, bool isbase>
static void render_scanline_text(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
#endif /* GBSP_BACKEND_VARIANT */
{
  // Tile mode has 4 and 8 bpp modes.
  u32 bg_control = read_ioreg(REG_BGxCNT(layer));
  bool is8bpp = (read_ioreg(REG_BGxCNT(layer)) & 0x80);
  const u32 mosamount = read_ioreg(REG_MOSAIC) & 0xFF;
  bool has_mosaic = (bg_control & 0x40) && (mosamount != 0);

  if (has_mosaic) {
    if (is8bpp)
      render_scanline_text_mosaic<stype, rdtype, isbase, true>(
        layer, start, end, scanline, paltbl);
    else
      render_scanline_text_mosaic<stype, rdtype, isbase, false>(
        layer, start, end, scanline, paltbl);
  } else {
    if (is8bpp)
      render_scanline_text_fast<stype, rdtype, isbase, true>(
        layer, start, end, scanline, paltbl);
    else
      render_scanline_text_fast<stype, rdtype, isbase, false>(
        layer, start, end, scanline, paltbl);
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
  }
}

template<typename stype, rendtype rdtype, bool isbase>
static void render_scanline_text(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
{
  render_scanline_text_body<stype, rdtype, isbase>(layer, start, end, scanline, paltbl);
}

/* the tile layers of the common case (16-bit, no blending) run on core 1
   next to the Xtensa dynarec on core 0: in IRAM, out of the shared 32 KB
   instruction cache */
template<>
XT_HOT void render_scanline_text<u16, FULLCOLOR, true>(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
{
  render_scanline_text_body<u16, FULLCOLOR, true>(layer, start, end, scanline, paltbl);
}
template<>
XT_HOT void render_scanline_text<u16, FULLCOLOR, false>(u32 layer,
 u32 start, u32 end, void *scanline, const u16 * paltbl)
{
  render_scanline_text_body<u16, FULLCOLOR, false>(layer, start, end, scanline, paltbl);
}
#else /* GBSP_BACKEND_VARIANT */
  }
}
#endif /* GBSP_BACKEND_VARIANT */

static inline u8 lookup_pix_8bpp(
  u32 px, u32 py, const u8 *tile_base, const u8 *map_base, u32 map_size
) {
  // Pitch represents the log2(number of tiles per row) (from 16 to 128)
  u32 map_pitch = map_size + 4;
  // Given coords (px,py) in the background space, find the tile.
  u32 mapoff = (px / 8) + ((py / 8) << map_pitch);
  // Each tile is 8x8, so 64 bytes each.
  const u8 *tile_ptr = &tile_base[map_base[mapoff] * tile_size_8bpp];
  // Read the 8bit color within the tile.
  return tile_ptr[(px % 8) + ((py % 8) * 8)];
}


template<typename dsttype, rendtype rdtype, bool isbase>
static inline void rend_pix_8bpp(
  dsttype *dest_ptr, u8 pval, u32 bg_comb, u32 px_comb, const u16 *pal
) {
  // Alhpa mode stacks previous value (unless rendering the first layer)
  if (pval) {
    if (rdtype == FULLCOLOR)
      *dest_ptr = pal[pval];
    else if (rdtype == INDXCOLOR)
      *dest_ptr = pval | px_comb;  // Add combine flags
    else if (rdtype == STCKCOLOR)
      // Stack pixels. If base, stack the base pixel.
      *dest_ptr = pval | px_comb | ((isbase ? bg_comb : *dest_ptr) << 16);
  }
  else if (isbase) {
    // Transparent pixel, but we are base layer, so render background.
    if (rdtype == FULLCOLOR)
      *dest_ptr = pal[0];
    else
      *dest_ptr = 0 | bg_comb;  // Just backdrop color and combine flags
  }
}

template<typename dsttype, rendtype rdtype>
static inline void render_bdrop_pixel_8bpp(
  dsttype *dest_ptr, u32 bg_comb, u16 bgcol) {
  // Alhpa mode stacks previous value (unless rendering the first layer)
  if (rdtype == FULLCOLOR)
    *dest_ptr = bgcol;
  else
    *dest_ptr = 0 | bg_comb;
}

typedef void (*affine_render_function) (
  u32 layer, u32 start, u32 cnt, const u8 *map_base,
  u32 map_size, const u8 *tile_base, void *dst_ptr,
  const u16* pal);

// Affine background rendering logic.
// wrap extends the background infinitely, otherwise transparent/backdrop fill
// rotate indicates if there's any rotation (optimized version for no-rotation)
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
// mosaic applies to horizontal mosaic (vertical is adjusted via affine ref)
/* Mario Kart's road is one 195-byte instantiation of this, and on the board
   of 2026-10-10 it was 14.2% of core 1 -- fetched from flash, through the same
   SPI0 cache controller core 0 fetches its PSRAM-resident translated code
   through. The instantiations are reached through a function-pointer table, so
   the whole template goes to IRAM (64 of them, 14.1 KB) rather than being
   restructured: the attribute moves the code without changing one instruction
   of it. */
template <typename dtype, rendtype rdtype,
          bool isbase, bool wrap, bool rotate, bool mosaic>
GBA_C1_HOT static void render_affine_background(
  u32 layer, u32 start, u32 cnt, const u8 *map_base,
#else /* GBSP_BACKEND_VARIANT */
// mosaic applies to horizontal mosaic (vertical is adjusted via affine ref)
template <typename dtype, rendtype rdtype,
          bool isbase, bool wrap, bool rotate, bool mosaic>
static inline void render_affine_background(
  u32 layer, u32 start, u32 cnt, const u8 *map_base,
#endif /* GBSP_BACKEND_VARIANT */
  u32 map_size, const u8 *tile_base, void *dst_ptr_raw,
  const u16* pal) {

  dtype *dst_ptr = (dtype*)dst_ptr_raw;
  // Backdrop and current layer combine bits.
  u32 bg_comb = color_flags(5);
  u32 px_comb = color_flags(layer);

  s32 dx = (s16)read_ioreg(REG_BGxPA(layer));
  s32 dy = (s16)read_ioreg(REG_BGxPC(layer));
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

  s32 source_x = r_affine_x[layer - 2] + (start * dx);
  s32 source_y = r_affine_y[layer - 2] + (start * dy);

#else /* GBSP_BACKEND_VARIANT */

  s32 source_x = r_affine_x[layer - 2] + (start * dx);
  s32 source_y = r_affine_y[layer - 2] + (start * dy);

#endif /* GBSP_BACKEND_VARIANT */
  // Maps are squared, four sizes available (128x128 to 1024x1024)
  u32 width_height = 128 << map_size;

  // Horizontal mosaic effect.
  const u32 mosh = (mosaic ? (read_ioreg(REG_MOSAIC)) & 0xF : 0) + 1;

  if (wrap) {
    // In wrap mode the entire space is covered, since it "wraps" at the edges
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
    u8 pval = 0;
    u32 mctr = 0;
    if (rotate) {
#else /* GBSP_BACKEND_VARIANT */
    u8 pval = 0;
    if (rotate) {
#endif /* GBSP_BACKEND_VARIANT */
      for (u32 i = 0; cnt; i++, cnt--) {
        u32 pix_x = (u32)(source_x >> 8) & (width_height-1);
        u32 pix_y = (u32)(source_y >> 8) & (width_height-1);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

        // Lookup pixel and draw it (only every Nth if mosaic is on).  The
        // counter form avoids a divl per pixel: mosh is loop-invariant but
        // runtime-valued, so gcc/clang do not strength-reduce 'i % mosh'
        // into a multiply-by-magic-constant sequence.  When mosaic == false
        // the counter machinery is dead-code-eliminated at compile time.
        if (!mosaic || !mctr) {
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
          mctr = mosh;
        }
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);
        if (mosaic) mctr--;

#else /* GBSP_BACKEND_VARIANT */

        // Lookup pixel and draw it (only every Nth if mosaic is on)
        if (!mosaic || !(i % mosh))
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);

#endif /* GBSP_BACKEND_VARIANT */
        source_x += dx; source_y += dy;  // Move to the next pixel
      }
    } else {
      // Y coordinate stays contant across the walk.
      const u32 pix_y = (u32)(source_y >> 8) & (width_height-1);
      for (u32 i = 0; cnt; i++, cnt--) {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
        u32 pix_x = (u32)(source_x >> 8) & (width_height-1);
        if (!mosaic || !mctr) {
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
          mctr = mosh;
        }
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);
        if (mosaic) mctr--;
        source_x += dx;  // Only moving in the X direction.
#else /* GBSP_BACKEND_VARIANT */
        u32 pix_x = (u32)(source_x >> 8) & (width_height-1);
        if (!mosaic || !(i % mosh))
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);
        source_x += dx;  // Only moving in the X direction.
#endif /* GBSP_BACKEND_VARIANT */
      }
    }
  } else {
    u16 bgcol = pal[0];
    if (rotate) {
      // Draw backdrop pixels if necessary until we reach the background edge.
      while (cnt) {
        // Draw backdrop pixels if they lie outside of the background.
        u32 pix_x = (u32)(source_x >> 8), pix_y = (u32)(source_y >> 8);

        // Stop once we find a pixel that is actually *inside* the map.
        if (pix_x < width_height && pix_y < width_height)
          break;

        // Draw a backdrop pixel if we are the base layer.
        if (isbase)
          render_bdrop_pixel_8bpp<dtype, rdtype>(dst_ptr, bg_comb, bgcol);

        dst_ptr++;
        source_x += dx; source_y += dy;
        cnt--;
      }

      // Draw background pixels by looking them up in the map
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      u8 pval = 0;
      u32 mctr = 0;
      for (u32 i = 0; cnt; i++, cnt--) {
#else /* GBSP_BACKEND_VARIANT */
      u8 pval = 0;
      for (u32 i = 0; cnt; i++, cnt--) {
#endif /* GBSP_BACKEND_VARIANT */
        u32 pix_x = (u32)(source_x >> 8), pix_y = (u32)(source_y >> 8);

        // Check if we run out of background pixels, stop drawing.
        if (pix_x >= width_height || pix_y >= width_height)
          break;

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
        // Lookup pixel and draw it.
        if (!mosaic || !mctr) {
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
          mctr = mosh;
        }
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);
        if (mosaic) mctr--;

#else /* GBSP_BACKEND_VARIANT */
        // Lookup pixel and draw it.
        if (!mosaic || !(i % mosh))
          pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
        rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);

#endif /* GBSP_BACKEND_VARIANT */
        // Move to the next pixel, update coords accordingly
        source_x += dx; source_y += dy;
      }
    } else {
      // Specialized version for scaled-only backgrounds
      u8 pval = 0;
      const u32 pix_y = (u32)(source_y >> 8);
      if (pix_y < width_height) {  // Check if within Y-coord range
        // Draw/find till left edge
        while (cnt) {
          u32 pix_x = (u32)(source_x >> 8);
          if (pix_x < width_height)
            break;

          if (isbase)
            render_bdrop_pixel_8bpp<dtype, rdtype>(dst_ptr, bg_comb, bgcol);

          dst_ptr++;
          source_x += dx;
          cnt--;
        }
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
        // Draw actual background
        u32 mctr = 0;
        for (u32 i = 0; cnt; i++, cnt--) {
#else /* GBSP_BACKEND_VARIANT */
        // Draw actual background
        for (u32 i = 0; cnt; i++, cnt--) {
#endif /* GBSP_BACKEND_VARIANT */
          u32 pix_x = (u32)(source_x >> 8);
          if (pix_x >= width_height)
            break;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

          if (!mosaic || !mctr) {
            pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
            mctr = mosh;
          }
          rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);
          if (mosaic) mctr--;

#else /* GBSP_BACKEND_VARIANT */

          if (!mosaic || !(i % mosh))
            pval = lookup_pix_8bpp(pix_x, pix_y, tile_base, map_base, map_size);
          rend_pix_8bpp<dtype, rdtype, isbase>(dst_ptr++, pval, bg_comb, px_comb, pal);

#endif /* GBSP_BACKEND_VARIANT */
          source_x += dx;
        }
      }
    }

    // Complete the line on the right, if we ran out over the bg edge.
    // Only necessary for the base layer, otherwise we can safely finish.
    if (isbase)
      while (cnt--)
        render_bdrop_pixel_8bpp<dtype, rdtype>(dst_ptr++, bg_comb, bgcol);
  }
}


// Renders affine backgrounds. These differ substantially from non-affine
// ones. Tile maps are byte arrays (instead of 16 bit), limiting the map to
// 256 different tiles (with no flip bits and just one single 256 color pal).
// Optimize for common cases: wrap/non-wrap, scaling/rotation.
template<typename dsttype, rendtype rdtype, bool isbase>
static void render_scanline_affine(u32 layer,
 u32 start, u32 end, void *scanline, const u16 *pal)
{
  u32 bg_control = read_ioreg(REG_BGxCNT(layer));
  u32 map_size = (bg_control >> 14) & 0x03;

  // Char block base pointer
  u32 base_block = (bg_control >> 8) & 0x1F;
  u8 *map_base = &vram[base_block * 2048];
  // The tilemap base is selected via bgcnt (16KiB chunks)
  u32 tilecntrl = (bg_control >> 2) & 0x03;
  u8 *tile_base = &vram[tilecntrl * 16*1024];

  dsttype *dest_ptr = ((dsttype*)scanline) + start;
  const u32 mosamount = read_ioreg(REG_MOSAIC) & 0xFF;

  bool has_mosaic = (bg_control & 0x40) && (mosamount != 0);
  bool has_rotation = read_ioreg(REG_BGxPC(layer)) != 0;
  bool has_wrap = (bg_control >> 13) & 1;

  // Number of pixels to render
  u32 cnt = end - start;

  // Four specialized versions for faster rendering on specific cases like
  // scaling only or non-wrapped backgrounds.
  u32 fidx = (has_wrap     ? 0x4 : 0) |
             (has_rotation ? 0x2 : 0) |
             (has_mosaic   ? 0x1 : 0);

  static const affine_render_function rdfns[8] = {
    render_affine_background<dsttype, rdtype, isbase, false, false, false>,
    render_affine_background<dsttype, rdtype, isbase, false, false, true>,
    render_affine_background<dsttype, rdtype, isbase, false, true,  false>,
    render_affine_background<dsttype, rdtype, isbase, false, true,  true>,
    render_affine_background<dsttype, rdtype, isbase, true,  false, false>,
    render_affine_background<dsttype, rdtype, isbase, true,  false, true>,
    render_affine_background<dsttype, rdtype, isbase, true,  true,  false>,
    render_affine_background<dsttype, rdtype, isbase, true,  true,  true>,
  };

  rdfns[fidx](layer, start, cnt, map_base, map_size, tile_base, dest_ptr, pal);
}

template<rendtype rdmode, typename buftype, unsigned mode, typename pixfmt>
static inline void bitmap_pixel_write(
  buftype *dst_ptr, pixfmt val, const u16 * palptr, u16 px_attr
) {
  if (mode != 4)
    *dst_ptr = convert_palette(val); // Direct color, u16 bitmap
  else if (val) {
    if (rdmode == FULLCOLOR)
      *dst_ptr = palptr[val];
    else if (rdmode == INDXCOLOR)
      *dst_ptr = val | px_attr;  // Add combine flags
    else if (rdmode == STCKCOLOR)
      *dst_ptr = val | px_attr | ((*dst_ptr) << 16);  // Stack pixels
  }
}


typedef enum
{
  BLIT,     // The bitmap has no scaling nor rotation on the X axis
  SCALED,   // The bitmap features some scaling (on the X axis) but no rotation
  ROTATED   // Bitmap has rotation (and perhaps scaling too)
} bm_rendmode;

// Renders a bitmap honoring the pixel mode and any affine transformations.
// There's optimized versions for bitmaps without scaling / rotation.

template<rendtype rdtype, typename dsttype, // Rendering target type and format
         unsigned mode,
         typename pixfmt,                   // Bitmap source pixel format (8/16)
         unsigned width, unsigned height,   // Bitmap size (not screen!)
         bm_rendmode rdmode,                // Rendering mode optimization.
         bool mosaic>                       // Whether mosaic effect is used.
static inline void render_scanline_bitmap(
  u32 start, u32 end, void *scanline, const u16 * palptr
) {
  s32 dx = (s16)read_ioreg(REG_BG2PA);
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
  s32 dy = (s16)read_ioreg(REG_BG2PC);
  s32 source_x = r_affine_x[0] + (start * dx); // Always BG2
  s32 source_y = r_affine_y[0] + (start * dy);

#else /* GBSP_BACKEND_VARIANT */
  s32 dy = (s16)read_ioreg(REG_BG2PC);
  s32 source_x = r_affine_x[0] + (start * dx); // Always BG2
  s32 source_y = r_affine_y[0] + (start * dy);

#endif /* GBSP_BACKEND_VARIANT */
  // Premature abort render optimization if bitmap out of Y coordinate.
  if ((rdmode != ROTATED) && ((u32)(source_y >> 8)) >= height)
    return;

  // Modes 4 and 5 feature double buffering.
  bool second_frame = (mode >= 4) && (read_ioreg(REG_DISPCNT) & 0x10);
  pixfmt *src_ptr = (pixfmt*)&vram[second_frame ? 0xA000 : 0x0000];
  dsttype *dst_ptr = ((dsttype*)scanline) + start;
  u16 px_attr = color_flags(2);   // Always BG2

  const u32 mosh = (mosaic ? (read_ioreg(REG_MOSAIC)) & 0xF : 0) + 1;

  if (rdmode == BLIT) {
    // We just blit pixels (copy) from buffer to buffer.
    const u32 pixel_y = (u32)(source_y >> 8);
    if (source_x < 0) {
      // The bitmap starts somewhere after "start", skip those pixels.
      u32 delta = (-source_x + 255) >> 8;
      dst_ptr += delta;
      start += delta;
      source_x = 0;
    }

    u32 pixel_x = (u32)(source_x >> 8);
    u32 pixcnt = MIN(end - start, width - pixel_x);
    pixfmt *valptr = &src_ptr[pixel_x + (pixel_y * width)];
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    u32 mctr = 0;
    for (u32 i = 0; pixcnt; i++, pixcnt--, valptr++) {
      // Pretty much pixel copier (counter-based mosaic: see render_affine_background)
      if (!mosaic || !mctr) {
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
        mctr = mosh;
      }
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);
      if (mosaic) mctr--;
    }
#else /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    for (u32 i = 0; pixcnt; i++, pixcnt--, valptr++) {
      // Pretty much pixel copier
      if (!mosaic || !(i % mosh))
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);
    }
#endif /* GBSP_BACKEND_VARIANT */
  }
  else if (rdmode == SCALED) {
    // Similarly to above, but now we need to sample pixels instead.
    const u32 pixel_y = (u32)(source_y >> 8);

    // Find the "inside" of the bitmap
    while (start < end) {
      u32 pixel_x = (u32)(source_x >> 8);
      if (pixel_x < width)
        break;
      source_x += dx;
      start++;
      dst_ptr++;
    }

    u32 cnt = end - start;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    u32 mctr = 0;
    for (u32 i = 0; cnt; i++, cnt--) {
#else /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    for (u32 i = 0; cnt; i++, cnt--) {
#endif /* GBSP_BACKEND_VARIANT */
      u32 pixel_x = (u32)(source_x >> 8);
      if (pixel_x >= width)
        break;  // We reached the end of the bitmap
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

      if (!mosaic || !mctr) {
        pixfmt *valptr = &src_ptr[pixel_x + (pixel_y * width)];
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
        mctr = mosh;
      }
#else /* GBSP_BACKEND_VARIANT */

      if (!mosaic || !(i % mosh)) {
        pixfmt *valptr = &src_ptr[pixel_x + (pixel_y * width)];
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
      }
#endif /* GBSP_BACKEND_VARIANT */

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);
      if (mosaic) mctr--;
      source_x += dx;
#else /* GBSP_BACKEND_VARIANT */
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);
      source_x += dx;
#endif /* GBSP_BACKEND_VARIANT */
    }
  } else {
    // Look for the first pixel to be drawn.
    while (start < end) {
      u32 pixel_x = (u32)(source_x >> 8), pixel_y = (u32)(source_y >> 8);
      if (pixel_x < width && pixel_y < height)
        break;
      start++;
      dst_ptr++;
      source_x += dx;  source_y += dy;
    }

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    u32 mctr = 0;
    for (u32 i = 0; start < end; start++, i++) {
      u32 pixel_x = (u32)(source_x >> 8), pixel_y = (u32)(source_y >> 8);
#else /* GBSP_BACKEND_VARIANT */
    pixfmt val = 0;
    for (u32 i = 0; start < end; start++) {
      u32 pixel_x = (u32)(source_x >> 8), pixel_y = (u32)(source_y >> 8);
#endif /* GBSP_BACKEND_VARIANT */

      // Check if we run out of background pixels, stop drawing.
      if (pixel_x >= width || pixel_y >= height)
        break;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

      // Lookup pixel and draw it.  Counter-based mosaic (see
      // render_affine_background).  Before this fix the loop counter
      // 'i' was declared but never incremented; gcc constant-folded
      // 'i % mosh' to !0 and the mosaic skip never triggered, so the
      // rotated-bitmap mode-3/4/5 paths sampled every pixel instead of
      // every Nth.  Matches the SCALED-bitmap and affine-object paths,
      // which both honor BG2 horizontal mosaic correctly.
      if (!mosaic || !mctr) {
        pixfmt *valptr = &src_ptr[pixel_x + (pixel_y * width)];
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
        mctr = mosh;
      }
#else /* GBSP_BACKEND_VARIANT */

      // Lookup pixel and draw it.
      if (!mosaic || !(i % mosh)) {
        pixfmt *valptr = &src_ptr[pixel_x + (pixel_y * width)];
        val = sizeof(pixfmt) == 2 ? eswap16(*valptr) : *valptr;
      }
#endif /* GBSP_BACKEND_VARIANT */

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);
      if (mosaic) mctr--;

#else /* GBSP_BACKEND_VARIANT */
      bitmap_pixel_write<rdtype, dsttype, mode, pixfmt>(dst_ptr++, val, palptr, px_attr);

#endif /* GBSP_BACKEND_VARIANT */
      // Move to the next pixel, update coords accordingly
      source_x += dx;
      source_y += dy;
    }
  }
}

// Object/Sprite rendering logic

static const u8 obj_dim_table[3][4][2] = {
  { {8, 8}, {16, 16}, {32, 32}, {64, 64} },
  { {16, 8}, {32, 8}, {32, 16}, {64, 32} },
  { {8, 16}, {8, 32}, {16, 32}, {32, 64} }
};
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
#include "esp_attr.h"
EXT_RAM_BSS_ATTR static u8 obj_priority_list[5][160][128];
EXT_RAM_BSS_ATTR static u8 obj_priority_count[5][160];
EXT_RAM_BSS_ATTR static u8 obj_alpha_count[160];
#else
static u8 obj_priority_list[5][160][128];
static u8 obj_priority_count[5][160];
static u8 obj_alpha_count[160];
#endif

#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

#if defined(XTENSA_ARCH) && defined(ESP_PLATFORM)
/* 100 KB: PSRAM, the dynarec needs the internal RAM (IWRAM, IRAM code) */
static EXT_RAM_BSS_ATTR u8 obj_priority_list[5][160][128];
#else
static u8 obj_priority_list[5][160][128];
#endif
static u8 obj_priority_count[5][160];
static u8 obj_alpha_count[160];

#else /* GBSP_BACKEND_VARIANT */

static u8 obj_priority_list[5][160][128];
static u8 obj_priority_count[5][160];
static u8 obj_alpha_count[160];

#endif /* GBSP_BACKEND_VARIANT */
typedef struct {
  s32 obj_x, obj_y;
  s32 obj_w, obj_h;
  u32 attr1, attr2;
  bool is_double;
} t_sprite;

// Renders a tile row (8 pixels) for a regular (non-affine) object/sprite.
// tile_offset points to the VRAM offset where the data lives.
template<typename dsttype, rendtype rdtype, bool is8bpp, bool hflip>
static inline void render_obj_part_tile_Nbpp(
  u32 px_comb, dsttype *dest_ptr, u32 start, u32 end,
  u32 tile_offset, u16 palette, const u16 *pal
) {
  // Note that the last VRAM bank wrap around, hence the offset aliasing
  const u8* tile_ptr = &vram[0x10000 + (tile_offset & 0x7FFF)];
  u32 px_attr = px_comb | palette | 0x100;  // Combine flags + high palette bit

  if (is8bpp) {
    // Each byte is a color, mapped to a palete.
    for (u32 i = start; i < end; i++, dest_ptr++) {
      // Honor hflip by selecting bytes in the correct order
      u32 sel = hflip ? (7-i) : i;
      u8 pval = tile_ptr[sel];
      // Alhpa mode stacks previous value
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = pal[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pval | px_attr;  // Add combine flags
        else if (rdtype == STCKCOLOR) {
          // Stack pixels on top of the pixel value and combine flags
          // We do not stack OBJ on OBJ, rather overwrite the previous object
          if (*dest_ptr & 0x100)
            *dest_ptr = pval | px_attr | ((*dest_ptr) & 0xFFFF0000);
          else
            *dest_ptr = pval | px_attr | ((*dest_ptr) << 16);
        }
        else if (rdtype == PIXCOPY)
          *dest_ptr = dest_ptr[240];
      }
    }
  } else {
    // Only 32 bits (8 pixels * 4 bits)
    for (u32 i = start; i < end; i++, dest_ptr++) {
      u32 selb = hflip ? (3-i/2) : i/2;
      u32 seln = hflip ? ((i & 1) ^ 1) : (i & 1);
      u8 pval = (tile_ptr[selb] >> (seln * 4)) & 0xF;
      const u16 *subpal = &pal[palette];
      if (pval) {
        if (rdtype == FULLCOLOR)
          *dest_ptr = subpal[pval];
        else if (rdtype == INDXCOLOR)
          *dest_ptr = pval | px_attr;
        else if (rdtype == STCKCOLOR) {
          if (*dest_ptr & 0x100)
            *dest_ptr = pval | px_attr | ((*dest_ptr) & 0xFFFF0000);
          else
            *dest_ptr = pval | px_attr | ((*dest_ptr) << 16);  // Stack pixels
        }
        else if (rdtype == PIXCOPY)
          *dest_ptr = dest_ptr[240];
      }
    }
  }
}

// Same as above but optimized for full tiles
template<typename dsttype, rendtype rdtype, bool is8bpp, bool hflip>
static inline void render_obj_tile_Nbpp(u32 px_comb,
  dsttype *dest_ptr, u32 tile_offset, u16 palette, const u16 *pal
) {
  const u8* tile_ptr = &vram[0x10000 + (tile_offset & 0x7FFF)];
  u32 px_attr = px_comb | palette | 0x100;  // Combine flags + high palette bit

  if (is8bpp) {
    for (u32 j = 0; j < 2; j++) {
      u32 tilepix = eswap32(((u32*)tile_ptr)[hflip ? 1-j : j]);
      if (tilepix) {
        for (u32 i = 0; i < 4; i++, dest_ptr++) {
          u8 pval = hflip ? (tilepix >> (24 - i*8)) : (tilepix >> (i*8));
          if (pval) {
            if (rdtype == FULLCOLOR)
              *dest_ptr = pal[pval];
            else if (rdtype == INDXCOLOR)
              *dest_ptr = pval | px_attr;  // Add combine flags
            else if (rdtype == STCKCOLOR) {
              if (*dest_ptr & 0x100)
                *dest_ptr = pval | px_attr | ((*dest_ptr) & 0xFFFF0000);
              else
                *dest_ptr = pval | px_attr | ((*dest_ptr) << 16);
            }
            else if (rdtype == PIXCOPY)
              *dest_ptr = dest_ptr[240];
          }
        }
      } else
        dest_ptr += 4;
    }
  } else {
    u32 tilepix = eswap32(*(u32*)tile_ptr);
    if (tilepix) {   // Can skip all pixels if the row is just transparent
      for (u32 i = 0; i < 8; i++, dest_ptr++) {
        u8 pval = (hflip ? (tilepix >> ((7-i)*4)) : (tilepix >> (i*4))) & 0xF;
        const u16 *subpal = &pal[palette];
        if (pval) {
          if (rdtype == FULLCOLOR)
            *dest_ptr = subpal[pval];
          else if (rdtype == INDXCOLOR)
            *dest_ptr = pval | px_attr;
          else if (rdtype == STCKCOLOR) {  // Stack background, replace sprite
            if (*dest_ptr & 0x100)
              *dest_ptr = pval | px_attr | ((*dest_ptr) & 0xFFFF0000);
            else
              *dest_ptr = pval | px_attr | ((*dest_ptr) << 16);
          }
          else if (rdtype == PIXCOPY)
            *dest_ptr = dest_ptr[240];
        }
      }
    }
  }
}

// Renders a regular sprite (non-affine) row to screen.
// delta_x is the object X coordinate referenced from the window start.
// cnt is the maximum number of pixels to draw, honoring window, obj width, etc.
template <typename stype, rendtype rdtype, bool is8bpp, bool hflip>
static void render_object(
  s32 delta_x, u32 cnt, stype *dst_ptr, u32 tile_offset, u32 px_comb,
  u16 palette, const u16* palptr
) {
  // Tile size in bytes for each mode
  const u32 tile_bsize = is8bpp ? tile_size_8bpp : tile_size_4bpp;
  // Number of bytes to advance (or rewind) on the tile map
  const s32 tile_size_off = hflip ? -tile_bsize : tile_bsize;

  if (delta_x < 0) {      // Left part is outside of the screen/window.
    u32 offx = -delta_x;  // How many pixels did we skip from the object?
    s32 block_off = offx / 8;
    u32 tile_off = offx % 8;

    // Skip the first object tiles (skips in the flip direction)
    tile_offset += block_off * tile_size_off;

    // Render a partial tile to the left
    if (tile_off) {
      u32 residual = 8 - tile_off;   // Pixel count to complete the first tile
      u32 maxpix = MIN(residual, cnt);
      render_obj_part_tile_Nbpp<stype, rdtype, is8bpp, hflip>(
        px_comb, dst_ptr, tile_off, tile_off + maxpix, tile_offset, palette, palptr);

      // Move to the next tile
      tile_offset += tile_size_off;
      // Account for drawn pixels
      cnt -= maxpix;
      dst_ptr += maxpix;
    }
  } else {
    // Render object completely from the left. Skip the empty space to the left
    dst_ptr += delta_x;
  }

  // Render full tiles to the scan line.
  s32 num_tiles = cnt / 8;
  while (num_tiles--) {
    // Render full tiles
    render_obj_tile_Nbpp<stype, rdtype, is8bpp, hflip>(
      px_comb, dst_ptr, tile_offset, palette, palptr);
    tile_offset += tile_size_off;
    dst_ptr += 8;
  }

  // Render any partial tile on the end
  cnt = cnt % 8;
  if (cnt)
    render_obj_part_tile_Nbpp<stype, rdtype, is8bpp, hflip>(
      px_comb, dst_ptr, 0, cnt, tile_offset, palette, palptr);
}

// A slower version of the version above, that renders objects pixel by pixel.
// This allows proper mosaic effects whenever necessary.
template <typename stype, rendtype rdtype, bool is8bpp, bool hflip>
static void render_object_mosaic(
  s32 delta_x, u32 cnt, stype *dst_ptr, u32 base_tile_offset,
  u32 mosh, u32 px_comb, u16 palette, const u16* pal
) {
  const u32 tile_bsize = is8bpp ? tile_size_8bpp : tile_size_4bpp;
  const s32 tile_size_off = hflip ? -tile_bsize : tile_bsize;

  u32 offx = 0;
  if (delta_x < 0) {      // Left part is outside of the screen/window.
    offx = -delta_x;  // Number of skipped pixels
  } else {
    dst_ptr += delta_x;
  }

  u32 px_attr = px_comb | palette | 0x100;  // Combine flags + high palette bit

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
  u8 pval = 0;
  u32 mctr = 0;
  for (u32 i = 0; i < cnt; i++, offx++, dst_ptr++) {
    // Counter-based mosaic (see render_affine_background for rationale).
    if (!mctr) {
      // Load tile pixel color.
#else /* GBSP_BACKEND_VARIANT */
  u8 pval = 0;
  for (u32 i = 0; i < cnt; i++, offx++, dst_ptr++) {
    if (!(i % mosh)) {
      // Load tile pixel color.
#endif /* GBSP_BACKEND_VARIANT */
      u32 tile_offset = base_tile_offset + (offx / 8) * tile_size_off;
      const u8* tile_ptr = &vram[0x10000 + (tile_offset & 0x7FFF)];

      // Lookup for each mode and flip value.
      if (is8bpp) {
        if (hflip)
          pval = tile_ptr[7 - offx % 8];
        else
          pval = tile_ptr[offx % 8];
      } else {
        if (hflip)
          pval = (tile_ptr[(7 - offx % 8) >> 1] >> (((offx & 1) ^ 1) * 4)) & 0xF;
        else
          pval = (tile_ptr[(offx % 8) >> 1] >> ((offx & 1) * 4)) & 0xF;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      }
      mctr = mosh;
    }
    mctr--;

#else /* GBSP_BACKEND_VARIANT */
      }
    }

#endif /* GBSP_BACKEND_VARIANT */
    // Write the pixel value as required
    const u16 *subpal = &pal[palette];
    if (pval) {
      if (rdtype == FULLCOLOR)
        *dst_ptr = is8bpp ? pal[pval] : subpal[pval];
      else if (rdtype == INDXCOLOR)
        *dst_ptr = pval | px_attr;  // Add combine flags
      else if (rdtype == STCKCOLOR) {
        if (*dst_ptr & 0x100)
          *dst_ptr = pval | px_attr | ((*dst_ptr) & 0xFFFF0000);
        else
          *dst_ptr = pval | px_attr | ((*dst_ptr) << 16);
      }
      else if (rdtype == PIXCOPY)
        *dst_ptr = dst_ptr[240];
    }
  }
}


// Renders an affine sprite row to screen.
// They support 4bpp and 8bpp modes. 1D and 2D tile mapping modes.
// Their render area is limited to their size (and optionally double size)
template <typename stype, rendtype rdtype, bool mosaic, bool is8bpp, bool rotate>
static void render_affine_object(
  const t_sprite *obji, const t_affp *affp, bool is_double,
  u32 start, u32 end, stype *dst_ptr, u32 mosv, u32 mosh,
  u32 base_tile, u32 pxcomb, u16 palette, const u16 *palptr
) {
  // Tile size in bytes for each mode
  const u32 tile_bsize = is8bpp ? tile_size_8bpp : tile_size_4bpp;
  const u32 tile_bwidth = is8bpp ? tile_width_8bpp : tile_width_4bpp;

  // Affine params
  s32 dx = (s16)eswap16(affp->dx);
  s32 dy = (s16)eswap16(affp->dy);
  s32 dmx = (s16)eswap16(affp->dmx);
  s32 dmy = (s16)eswap16(affp->dmy);

  // Object dimensions and boundaries
  u32 obj_dimw = obji->obj_w;
  u32 obj_dimh = obji->obj_h;
  s32 middle_x = is_double ? obji->obj_w : (obji->obj_w / 2);
  s32 middle_y = is_double ? obji->obj_h : (obji->obj_h / 2);
  s32 obj_width  = is_double ? obji->obj_w * 2 : obji->obj_w;
  s32 obj_height = is_double ? obji->obj_h * 2 : obji->obj_h;

  s32 vcount = read_ioreg(REG_VCOUNT);
  if (mosaic)
    vcount -= vcount % mosv;
  s32 y_delta = vcount - (obji->obj_y + middle_y);

  if (obji->obj_x < (signed)start)
    middle_x -= (start - obji->obj_x);
  s32 source_x = (obj_dimw << 7) + (y_delta * dmx) - (middle_x * dx);
  s32 source_y = (obj_dimh << 7) + (y_delta * dmy) - (middle_x * dy);

  // Early optimization if Y-coord is out completely for this line.
  // (if there's no rotation Y coord remains identical throughout the line).
  if (!rotate && ((u32)(source_y >> 8)) >= (u32)obj_height)
    return;

  u32 d_start = MAX((signed)start, obji->obj_x);
  u32 d_end   = MIN((signed)end,   obji->obj_x + obj_width);
  u32 cnt = d_end - d_start;
  dst_ptr += d_start;

  bool obj1dmap = read_ioreg(REG_DISPCNT) & 0x40;
  const u32 tile_pitch = obj1dmap ? (obj_dimw / 8) * tile_bsize : 1024;
  u32 px_attr = pxcomb | palette | 0x100;  // Combine flags + high palette bit

  // Skip pixels outside of the sprite area, until we reach the sprite "inside"
  while (cnt) {
    u32 pixel_x = (u32)(source_x >> 8), pixel_y = (u32)(source_y >> 8);

    // Stop once we find a pixel that is actually *inside* the map.
    if (pixel_x < obj_dimw && pixel_y < obj_dimh)
      break;

    dst_ptr++;
    source_x += dx;
    if (rotate)
      source_y += dy;
    cnt--;
  }

  // Draw sprite pixels by looking them up first. Lookup address is tricky!
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
  u8 pixval = 0;
  u32 mctr = 0;
  for (u32 i = 0; i < cnt; i++) {
#else /* GBSP_BACKEND_VARIANT */
  u8 pixval = 0;
  for (u32 i = 0; i < cnt; i++) {
#endif /* GBSP_BACKEND_VARIANT */
    u32 pixel_x = (u32)(source_x >> 8), pixel_y = (u32)(source_y >> 8);

    // Check if we run out of the sprite, then we can safely abort.
    if (pixel_x >= obj_dimw || pixel_y >= obj_dimh)
      return;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

    // For mosaic, we "remember" the last looked up pixel.  Counter form
    // avoids a per-pixel divl (see render_affine_background).
    if (!mosaic || !mctr) {
      // Lookup pixel and draw it.
#else /* GBSP_BACKEND_VARIANT */

    // For mosaic, we "remember" the last looked up pixel.
    if (!mosaic || !(i % mosh)) {
      // Lookup pixel and draw it.
#endif /* GBSP_BACKEND_VARIANT */
      if (is8bpp) {
        // We lookup the byte directly and render it.
        const u32 tile_off =
          base_tile +                        // Character base
          ((pixel_y >> 3) * tile_pitch) +    // Skip vertical blocks
          ((pixel_x >> 3) * tile_bsize) +    // Skip horizontal blocks
          ((pixel_y & 0x7) * tile_bwidth) +  // Skip vertical rows to the pixel
          (pixel_x & 0x7);                   // Skip the horizontal offset

        pixval = vram[0x10000 + (tile_off & 0x7FFF)];   // Read pixel value!
      } else {
        const u32 tile_off =
          base_tile +                        // Character base
          ((pixel_y >> 3) * tile_pitch) +    // Skip vertical blocks
          ((pixel_x >> 3) * tile_bsize) +    // Skip horizontal blocks
          ((pixel_y & 0x7) * tile_bwidth) +  // Skip vertical rows to the pixel
          ((pixel_x >> 1) & 0x3);            // Skip the horizontal offset

        u8 pixpair = vram[0x10000 + (tile_off & 0x7FFF)]; // Read 2 pixels @4bpp
        pixval = ((pixel_x & 1) ? pixpair >> 4 : pixpair & 0xF);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      }
      mctr = mosh;
    }
    if (mosaic) mctr--;

#else /* GBSP_BACKEND_VARIANT */
      }
    }

#endif /* GBSP_BACKEND_VARIANT */
    // Render the pixel value
    if (pixval) {
      if (rdtype == FULLCOLOR)
        *dst_ptr = palptr[pixval | palette];
      else if (rdtype == INDXCOLOR)
        *dst_ptr = pixval | px_attr;  // Add combine flags
      else if (rdtype == STCKCOLOR) {
        // Stack pixels on top of the pixel value and combine flags
        if (*dst_ptr & 0x100)
          *dst_ptr = pixval | px_attr | ((*dst_ptr) & 0xFFFF0000);
        else
          *dst_ptr = pixval | px_attr | ((*dst_ptr) << 16);  // Stack pixels
      }
      else if (rdtype == PIXCOPY)
        *dst_ptr = dst_ptr[240];
    }

    // Move to the next pixel, update coords accordingly
    dst_ptr++;
    source_x += dx;
    if (rotate)
      source_y += dy;
  }
}

// Renders a single sprite on the current scanline.
// This function calls the affine or regular renderer depending on the sprite.
// Will calculate whether sprite has certain effects (flip, rotation ...) to
// use an optimized renderer function.
template <typename stype, rendtype rdtype, bool is8bpp, bool mosaic>
inline static void render_sprite(
  const t_sprite *obji, bool is_affine, u32 start, u32 end, stype *scanline,
  u32 pxcomb, const u16* palptr
) {
  s32 vcount = read_ioreg(REG_VCOUNT);
  bool obj1dmap = read_ioreg(REG_DISPCNT) & 0x40;
  const u32 msk = is8bpp && !obj1dmap ? 0x3FE : 0x3FF;
  const u32 base_tile = (obji->attr2 & msk) * 32;

  const u32 mosv = (mosaic ? (read_ioreg(REG_MOSAIC) >> 12) & 0xF : 0) + 1;
  const u32 mosh = (mosaic ? (read_ioreg(REG_MOSAIC) >>  8) & 0xF : 0) + 1;

  // Render the object scanline using the correct mode.
  // (in 4bpp mode calculate the palette number)
  // Objects use the higher palette part
  u16 pal = (is8bpp ? 0 : ((obji->attr2 >> 8) & 0xF0));

  if (is_affine) {
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
    u32 pnum = (obji->attr1 >> 9) & 0x1f;
    const t_affp *affp_base = (t_affp*)render_oam;
    const t_affp *affp = &affp_base[pnum];
#else /* GBSP_BACKEND_VARIANT */
    u32 pnum = (obji->attr1 >> 9) & 0x1f;
    const t_affp *affp_base = (t_affp*)render_oam;
    const t_affp *affp = &affp_base[pnum];
#endif /* GBSP_BACKEND_VARIANT */

    if (affp->dy == 0)     // No rotation happening (just scale)
      render_affine_object<stype, rdtype, mosaic, is8bpp, false>(
        obji, affp, obji->is_double, start, end, scanline, mosv, mosh,
        base_tile, pxcomb, pal, palptr);
    else                   // Full rotation and scaling
      render_affine_object<stype, rdtype, mosaic, is8bpp, true>(
        obji, affp, obji->is_double, start, end, scanline, mosv, mosh,
        base_tile, pxcomb, pal, palptr);
  } else {
    // The object could be out of the window, check and skip.
    if (obji->obj_x >= (signed)end || obji->obj_x + obji->obj_w <= (signed)start)
      return;

    // Non-affine objects can be flipped on both edges.
    bool hflip = obji->attr1 & 0x1000;
    bool vflip = obji->attr1 & 0x2000;

    // Calulate the vertical offset (row) to be displayed. Account for vflip.
    u32 voffset = vflip ? obji->obj_y + obji->obj_h - vcount - 1
                        : vcount - obji->obj_y;
    if (mosaic)
      voffset -= voffset % mosv;

    // Calculate base tile for the object (points to the row to be drawn).
    u32 tile_bsize  = is8bpp ? tile_size_8bpp : tile_size_4bpp;
    u32 tile_bwidth = is8bpp ? tile_width_8bpp : tile_width_4bpp;
    u32 obj_pitch = obj1dmap ? (obji->obj_w / 8) * tile_bsize : 1024;
    u32 hflip_off = hflip ? ((obji->obj_w / 8) - 1) * tile_bsize : 0;

    // Calculate the pointer to the tile.
    const u32 tile_offset =
      base_tile +                    // Char offset
      (voffset / 8) * obj_pitch +    // Select tile row offset
      (voffset % 8) * tile_bwidth +  // Skip tile rows
      hflip_off;                     // Account for horizontal flip

    // Make everything relative to start
    s32 obj_x_offset  = obji->obj_x - start;
    u32 clipped_width = obj_x_offset >= 0 ? obji->obj_w : obji->obj_w + obj_x_offset;
    u32 max_range = obj_x_offset >= 0 ? end - obji->obj_x : end - start;
    u32 max_draw = MIN(max_range, clipped_width);

    if (mosaic && mosh > 1) {
      if (hflip)
        render_object_mosaic<stype, rdtype, is8bpp, true>(
          obj_x_offset, max_draw, &scanline[start], tile_offset, mosh, pxcomb, pal, palptr);
      else
        render_object_mosaic<stype, rdtype, is8bpp, false>(
          obj_x_offset, max_draw, &scanline[start], tile_offset, mosh, pxcomb, pal, palptr);
    } else {
      if (hflip)
        render_object<stype, rdtype, is8bpp, true>(
          obj_x_offset, max_draw, &scanline[start], tile_offset, pxcomb, pal, palptr);
      else
        render_object<stype, rdtype, is8bpp, false>(
          obj_x_offset, max_draw, &scanline[start], tile_offset, pxcomb, pal, palptr);
    }
  }
}

// Renders objects on a scanline for a given priority.
// This function assumes that order_obj has been called to prepare the objects.
template <typename stype, rendtype rdtype>
void render_scanline_objs(
  u32 priority, u32 start, u32 end, void *raw_ptr, const u16* palptr
) {
  stype *scanline = (stype*)raw_ptr;
  s32 vcount = read_ioreg(REG_VCOUNT);
  s32 objn;
  u32 objcnt = obj_priority_count[priority][vcount];
  u8 *objlist = obj_priority_list[priority][vcount];

  // Render all the visible objects for this priority (back to front)
  for (objn = objcnt-1; objn >= 0; objn--) {
    // Objects in the list are pre-filtered and sorted in the appropriate order
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
    u32 objoff = objlist[objn];
    const t_oam *oamentry = &((t_oam*)render_oam)[objoff];

#else /* GBSP_BACKEND_VARIANT */
    u32 objoff = objlist[objn];
    const t_oam *oamentry = &((t_oam*)render_oam)[objoff];

#endif /* GBSP_BACKEND_VARIANT */
    u16 obj_attr0 = eswap16(oamentry->attr0);
    u16 obj_attr1 = eswap16(oamentry->attr1);
    u16 obj_shape = obj_attr0 >> 14;
    u16 obj_size = (obj_attr1 >> 14);
    bool is_affine = obj_attr0 & 0x100;
    bool is_trans = ((obj_attr0 >> 10) & 0x3) == OBJ_MOD_SEMITRAN;

    t_sprite obji = {
      .obj_x = (s32)(obj_attr1 << 23) >> 23,
      .obj_y = obj_attr0 & 0xFF,
      .obj_w = obj_dim_table[obj_shape][obj_size][0],
      .obj_h = obj_dim_table[obj_shape][obj_size][1],
      .attr1 = obj_attr1,
      .attr2 = eswap16(oamentry->attr2),
      .is_double = (obj_attr0 & 0x200) != 0,
    };

    s32 obj_maxw = (is_affine && obji.is_double) ? obji.obj_w * 2 : obji.obj_w;

    // The object could be out of the window, check and skip.
    if (obji.obj_x >= (signed)end || obji.obj_x + obj_maxw <= (signed)start)
      continue;

    // ST-OBJs force 1st target bit (forced blending)
    bool forcebld = is_trans && rdtype != FULLCOLOR;

    if (obji.obj_y > 160)
      obji.obj_y -= 256;

    // In PIXCOPY mode, we have already some stuff rendered (winout) and now
    // we render the "win-in" area for this object. The PIXCOPY function will
    // copy (merge) the two pixels depending on the result of the sprite render
    // The temporary buffer is rendered on the next scanline area.
    if (rdtype == PIXCOPY) {
      u32 sec_start = MAX((signed)start, obji.obj_x);
      u32 sec_end   = MIN((signed)end, obji.obj_x + obj_maxw);
      u32 obj_enable = read_ioreg(REG_WINOUT) >> 8;

      // Render at the next scanline!
      u16 *tmp_ptr = (u16*)&scanline[GBA_SCREEN_PITCH];
      render_scanline_conditional(sec_start, sec_end, tmp_ptr, obj_enable);
    }

    // Calculate combine masks. These store 2 bits of info: 1st and 2nd target.
    // If set, the current pixel belongs to a layer that is 1st or 2nd target.
    // For ST-objs, we set an extra bit, for later blending.
    u32 pxcomb = (forcebld ? 0x800 : 0) | color_flags(4);

    bool emosaic = (obj_attr0 & 0x1000) != 0;
    bool is_8bpp = (obj_attr0 & 0x2000) != 0;

    // Some games enable mosaic but set it to size 0 (1), so ignore.
    const u32 mosreg = read_ioreg(REG_MOSAIC) & 0xFF00;

    if (emosaic && mosreg) {
      if (is_8bpp)
        render_sprite<stype, rdtype, true, true>(
          &obji, is_affine, start, end, scanline, pxcomb, palptr);
      else
        render_sprite<stype, rdtype, false, true>(
          &obji, is_affine, start, end, scanline, pxcomb, palptr);
    } else {
      if (is_8bpp)
        render_sprite<stype, rdtype, true, false>(
          &obji, is_affine, start, end, scanline, pxcomb, palptr);
      else
        render_sprite<stype, rdtype, false, false>(
          &obji, is_affine, start, end, scanline, pxcomb, palptr);
    }
  }
}


// Goes through the object list in the OAM (from #127 to #0) and adds objects
// into a sorted list by priority for the current row.
// Invisible objects are discarded. ST-objects are flagged. Cycle counting is
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
// performed to discard excessive objects (to match HW capabilities).
GBA_C1_HOT static void order_obj(u32 video_mode)   /* 1.6% of core 1, 519 B */
{
#else /* GBSP_BACKEND_VARIANT */
// performed to discard excessive objects (to match HW capabilities).
static void order_obj(u32 video_mode)
{
#endif /* GBSP_BACKEND_VARIANT */
  u32 obj_num;
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
  u32 row;
  t_oam *oam_base = (t_oam*)render_oam;
  u16 rend_cycles[160];
#else /* GBSP_BACKEND_VARIANT */
  u32 row;
  t_oam *oam_base = (t_oam*)render_oam;
  u16 rend_cycles[160];
#endif /* GBSP_BACKEND_VARIANT */

  bool hblank_free = read_ioreg(REG_DISPCNT) & 0x20;
  u16 max_rend_cycles = !sprite_limit ? REND_CYC_MAX :
                         hblank_free  ? REND_CYC_REDUCED :
                                        REND_CYC_SCANLINE;

  memset(obj_priority_count, 0, sizeof(obj_priority_count));
  memset(obj_alpha_count, 0, sizeof(obj_alpha_count));
  memset(rend_cycles, 0, sizeof(rend_cycles));

  for(obj_num = 0; obj_num < 128; obj_num++)
  {
    t_oam *oam_ptr = &oam_base[obj_num];
    u16 obj_attr0 = eswap16(oam_ptr->attr0);

    // Bit 9 disables regular sprites (that is, non-affine ones).
    if ((obj_attr0 & 0x0300) == 0x0200)
      continue;

    u16 obj_shape = obj_attr0 >> 14;
    u32 obj_mode = (obj_attr0 >> 10) & 0x03;

    // Prohibited shape and mode
    if ((obj_shape == 0x3) || (obj_mode == OBJ_MOD_INVALID))
      continue;

    u16 obj_attr2 = eswap16(oam_ptr->attr2);

    // On bitmap modes, objs 0-511 are not usable, ingore them.
    if ((video_mode >= 3) && (!(obj_attr2 & 0x200)))
      continue;

    u16 obj_attr1 = eswap16(oam_ptr->attr1);
    // Calculate object size (from size and shape attr bits)
    u16 obj_size = (obj_attr1 >> 14);
    s32 obj_height = obj_dim_table[obj_shape][obj_size][1];
    s32 obj_width  = obj_dim_table[obj_shape][obj_size][0];
    s32 obj_y = obj_attr0 & 0xFF;

    if(obj_y > 160)
      obj_y -= 256;

    // Double size for affine sprites with double bit set
    if(obj_attr0 & 0x200)
    {
      obj_height *= 2;
      obj_width *= 2;
    }

    if(((obj_y + obj_height) > 0) && (obj_y < 160))
    {
      s32 obj_x = (s32)(obj_attr1 << 23) >> 23;

      if(((obj_x + obj_width) > 0) && (obj_x < 240))
      {
        u32 obj_priority = (obj_attr2 >> 10) & 0x03;
        bool is_affine = obj_attr0 & 0x100;
        // Clip Y coord and height to the 0..159 interval
        u32 starty = MAX(obj_y, 0);
        u32 endy   = MIN(obj_y + obj_height, 160);

        // Calculate needed cycles to render the sprite
        u16 cyccnt = is_affine ? (10 + obj_width * 2) : obj_width;

        switch (obj_mode) {
        case OBJ_MOD_SEMITRAN:
          for(row = starty; row < endy; row++)
          {
            if (rend_cycles[row] < max_rend_cycles) {
              u32 cur_cnt = obj_priority_count[obj_priority][row];
              obj_priority_list[obj_priority][row][cur_cnt] = obj_num;
              obj_priority_count[obj_priority][row] = cur_cnt + 1;
              rend_cycles[row] += cyccnt;
              // Mark the row as having semi-transparent objects
              obj_alpha_count[row] = 1;
            }
          }
          break;
        case OBJ_MOD_WINDOW:
          obj_priority = 4;
          /* fallthrough */
        case OBJ_MOD_NORMAL:
          // Add the object to the list.
          for(row = starty; row < endy; row++)
          {
            if (rend_cycles[row] < max_rend_cycles) {
              u32 cur_cnt = obj_priority_count[obj_priority][row];
              obj_priority_list[obj_priority][row][cur_cnt] = obj_num;
              obj_priority_count[obj_priority][row] = cur_cnt + 1;
              rend_cycles[row] += cyccnt;
            }
          }
          break;
        };
      }
    }
  }
}

u32 layer_order[16];
u32 layer_count;

// Sorts active BG/OBJ layers and generates an ordered list of layers.
// Things are drawn back to front, so lowest priority goes first.
static void order_layers(u32 layer_flags, u32 vcnt)
{
  bool obj_enabled = (layer_flags & 0x10);
  s32 priority;
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

  // Pre-fetch the 4 layer priorities once instead of reading io_registers
  // four times per priority (16 reads -> 4 reads per scanline).  The
  // compiler can't CSE the read across the outer loop because io_registers
  // is global and could in principle be modified by JIT-translated code
  // between iterations; that doesn't actually happen here (we are inside
  // update_scanline which runs to completion before returning to the JIT),
  // but stating it explicitly costs nothing.
  const u32 layer_prio[4] = {
    (u32)(read_ioreg(REG_BGxCNT(0)) & 0x03),
    (u32)(read_ioreg(REG_BGxCNT(1)) & 0x03),
    (u32)(read_ioreg(REG_BGxCNT(2)) & 0x03),
    (u32)(read_ioreg(REG_BGxCNT(3)) & 0x03),
  };

  layer_count = 0;
#else /* GBSP_BACKEND_VARIANT */

  layer_count = 0;
#endif /* GBSP_BACKEND_VARIANT */

  for(priority = 3; priority >= 0; priority--)
  {
    bool anyobj = obj_priority_count[priority][vcnt] > 0;
    s32 lnum;

    for(lnum = 3; lnum >= 0; lnum--)
    {
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
      if(((layer_flags >> lnum) & 1) &&
         (layer_prio[lnum] == (u32)priority))
      {
#else /* GBSP_BACKEND_VARIANT */
      if(((layer_flags >> lnum) & 1) &&
         ((read_ioreg(REG_BGxCNT(lnum)) & 0x03) == priority))
      {
#endif /* GBSP_BACKEND_VARIANT */
        layer_order[layer_count++] = lnum;
      }
    }

    if(obj_enabled && anyobj)
      layer_order[layer_count++] = priority | 0x04;
  }
}


// Blending is performed by separating an RGB value into 0G0R0B (32 bit)
// Since blending factors are at most 16, mult/add operations do not overflow
// to the neighbouring color and can be performed much faster than separatedly

// Here follow the mask value to separate/expand the color to 32 bit,
// the mask to detect overflows in the blend operation and

#define BLND_MSK (SATR_MSK | SATG_MSK | SATB_MSK)

#ifdef USE_XBGR1555_FORMAT
  #define OVFG_MSK 0x04000000
  #define OVFR_MSK 0x00008000
  #define OVFB_MSK 0x00000020
  #define SATG_MSK 0x03E00000
  #define SATR_MSK 0x00007C00
  #define SATB_MSK 0x0000001F
#else
  #define OVFG_MSK 0x08000000
  #define OVFR_MSK 0x00010000
  #define OVFB_MSK 0x00000020
  #define SATG_MSK 0x07E00000
  #define SATR_MSK 0x0000F800
  #define SATB_MSK 0x0000001F
#endif

typedef enum
{
  OBJ_BLEND,    // No effects, just blend forced-blend pixels (ie. ST objects)
  BLEND_ONLY,   // Just alpha blending (if the pixels are 1st and 2nd target)
  BLEND_BRIGHT, // Perform alpha blending if appropiate, and brighten otherwise
  BLEND_DARK,   // Same but with darken effecg
} blendtype;

// Applies blending (and optional brighten/darken) effect to a bunch of
// color-indexed pixel pairs. Depending on the mode and the pixel target
// number, blending, darken/brighten or no effect will be applied.
// Bits 0-8 encode the color index (paletted colors)
// Bit 9 is set if the pixel belongs to a 1st target layer
// Bit 10 is set if the pixel belongs to a 2nd target layer
// Bit 11 is set if the pixel belongs to a ST-object
template <blendtype bldtype, bool st_objs>
static void merge_blend(u32 start, u32 end, u16 *dst, u32 *src) {
  u32 bldalpha = read_ioreg(REG_BLDALPHA);
  u32 brightf = MIN(16, read_ioreg(REG_BLDY) & 0x1F);
  u32 blend_a = MIN(16, (bldalpha >> 0) & 0x1F);
  u32 blend_b = MIN(16, (bldalpha >> 8) & 0x1F);

  bool can_saturate = blend_a + blend_b > 16;

  if (can_saturate) {
    // If blending can result in saturation, we need to clamp output values.
    while (start < end) {
      u32 pixpair = src[start];
      // If ST-OBJ, force blending mode (has priority over other effects).
      // If regular blending mode, blend if 1st/2nd bits are set respectively.
      // Otherwise, apply other color effects if 1st bit is set.
      bool force_blend = (pixpair & 0x04000800) == 0x04000800;
      bool do_blend    = (pixpair & 0x04000200) == 0x04000200;
      if ((st_objs && force_blend) || (do_blend && bldtype == BLEND_ONLY)) {
        // Top pixel is 1st target, pixel below is 2nd target. Blend!
        u16 p1 = palette_ram_converted[(pixpair >>  0) & 0x1FF];
        u16 p2 = palette_ram_converted[(pixpair >> 16) & 0x1FF];
        u32 p1e = (p1 | (p1 << 16)) & BLND_MSK;
        u32 p2e = (p2 | (p2 << 16)) & BLND_MSK;
        u32 pfe = (((p1e * blend_a) + (p2e * blend_b)) >> 4);

        // If the overflow bit is set, saturate (set) all bits to one.
        if (pfe & (OVFR_MSK | OVFG_MSK | OVFB_MSK)) {
          if (pfe & OVFG_MSK)
            pfe |= SATG_MSK;
          if (pfe & OVFR_MSK)
            pfe |= SATR_MSK;
          if (pfe & OVFB_MSK)
            pfe |= SATB_MSK;
        }
        pfe &= BLND_MSK;
        dst[start++] = (pfe >> 16) | pfe;
      }
      else if ((bldtype == BLEND_DARK || bldtype == BLEND_BRIGHT) &&
               (pixpair & 0x200) == 0x200) {
        // Top pixel is 1st-target, can still apply bright/dark effect.
        u16 pidx = palette_ram_converted[pixpair & 0x1FF];
        u32 epixel = (pidx | (pidx << 16)) & BLND_MSK;
        u32 pa = bldtype == BLEND_DARK ? 0 : ((BLND_MSK * brightf) >> 4) & BLND_MSK;
        u32 pb = ((epixel * (16 - brightf)) >> 4) & BLND_MSK;
        epixel = (pa + pb) & BLND_MSK;
        dst[start++] = (epixel >> 16) | epixel;
      }
      else {
        dst[start++] = palette_ram_converted[pixpair & 0x1FF];   // No effects
      }
    }
  } else {
    while (start < end) {
      u32 pixpair = src[start];
      bool do_blend    = (pixpair & 0x04000200) == 0x04000200;
      bool force_blend = (pixpair & 0x04000800) == 0x04000800;
      if ((st_objs && force_blend) || (do_blend && bldtype == BLEND_ONLY)) {
        // Top pixel is 1st target, pixel below is 2nd target. Blend!
        u16 p1 = palette_ram_converted[(pixpair >>  0) & 0x1FF];
        u16 p2 = palette_ram_converted[(pixpair >> 16) & 0x1FF];
        u32 p1e = (p1 | (p1 << 16)) & BLND_MSK;
        u32 p2e = (p2 | (p2 << 16)) & BLND_MSK;
        u32 pfe = (((p1e * blend_a) + (p2e * blend_b)) >> 4) & BLND_MSK;
        dst[start++] = (pfe >> 16) | pfe;
      }
      else if ((bldtype == BLEND_DARK || bldtype == BLEND_BRIGHT) &&
               (pixpair & 0x200) == 0x200) {
        // Top pixel is 1st-target, can still apply bright/dark effect.
        u16 pidx = palette_ram_converted[pixpair & 0x1FF];
        u32 epixel = (pidx | (pidx << 16)) & BLND_MSK;
        u32 pa = bldtype == BLEND_DARK ? 0 : ((BLND_MSK * brightf) >> 4) & BLND_MSK;
        u32 pb = ((epixel * (16 - brightf)) >> 4) & BLND_MSK;
        epixel = (pa + pb) & BLND_MSK;
        dst[start++] = (epixel >> 16) | epixel;
      }
      else {
        dst[start++] = palette_ram_converted[pixpair & 0x1FF];   // No effects
      }
    }
  }
}

// Applies brighten/darken effect to a bunch of color-indexed pixels.
template <blendtype bldtype>
static void merge_brightness(u32 start, u32 end, u16 *srcdst) {
  u32 brightness = MIN(16, read_ioreg(REG_BLDY) & 0x1F);

  while (start < end) {
    u16 spix = srcdst[start];
    u16 pixcol = palette_ram_converted[spix & 0x1FF];

    if ((spix & 0x200) == 0x200) {
      // Pixel is 1st target, can apply color effect.
      u32 epixel = (pixcol | (pixcol << 16)) & BLND_MSK;
      u32 pa = bldtype == BLEND_DARK ? 0 : ((BLND_MSK * brightness) >> 4) & BLND_MSK; // B/W
      u32 pb = ((epixel * (16 - brightness)) >> 4) & BLND_MSK;  // Pixel color
      epixel = (pa + pb) & BLND_MSK;
      pixcol = (epixel >> 16) | epixel;
    }

    srcdst[start++] = pixcol;
  }
}

// Fills a segment using the backdrop color (in the right mode).
template<rendtype rdmode, typename dsttype>
void fill_line_background(u32 start, u32 end, dsttype *scanline) {
  dsttype bgcol = palette_ram_converted[0];
  u16 bg_comb = color_flags(5);
  while (start < end)
    if (rdmode == FULLCOLOR)
      scanline[start++] = bgcol;
    else
      scanline[start++] = 0 | bg_comb;
}

// Renders the backdrop color (ie. whenever no layer is active) applying
// any effects that might still apply (usually darken/brighten).
static void render_backdrop(u32 start, u32 end, u16 *scanline) {
  u16 bldcnt = read_ioreg(REG_BLDCNT);
  u16 pixcol = palette_ram_converted[0];
  u32 effect = (bldcnt >> 6) & 0x03;
  u32 bd_1st_target = ((bldcnt >> 0x5) & 0x01);

  if (bd_1st_target && effect == COL_EFFECT_BRIGHT) {
    u32 brightness = MIN(16, read_ioreg(REG_BLDY) & 0x1F);

    // Unpack 16 bit pixel for fast blending operation
    u32 epixel = (pixcol | (pixcol << 16)) & BLND_MSK;
    u32 pa = ((BLND_MSK * brightness)      >> 4) & BLND_MSK;  // White color
    u32 pb = ((epixel * (16 - brightness)) >> 4) & BLND_MSK;  // Pixel color
    epixel = (pa + pb) & BLND_MSK;
    pixcol = (epixel >> 16) | epixel;
  }
  else if (bd_1st_target && effect == COL_EFFECT_DARK) {
    u32 brightness = MIN(16, read_ioreg(REG_BLDY) & 0x1F);
    u32 epixel = (pixcol | (pixcol << 16)) & BLND_MSK;
    epixel = ((epixel * (16 - brightness)) >> 4) & BLND_MSK;  // Pixel color
    pixcol = (epixel >> 16) | epixel;
  }

  // Fill the line with that color
  while (start < end)
    scanline[start++] = pixcol;
}

// Renders all the available and enabled layers (in tiled mode).
// Walks the list of layers in visibility order and renders them in the
// specified mode (taking into consideration the first layer, etc).
template<rendtype bgmode, rendtype objmode, typename dsttype>
void tile_render_layers(u32 start, u32 end, dsttype *dst_ptr, u32 enabled_layers) {
  u32 lnum;
  u32 base_done = 0;
  u16 dispcnt = read_ioreg(REG_DISPCNT);
  u16 video_mode = dispcnt & 0x07;
  bool obj_enabled = (enabled_layers & 0x10);   // Objects are visible

  bool objlayer_is_1st_tgt = ((read_ioreg(REG_BLDCNT) >> 4) & 1) != 0;
  bool has_trans_obj = obj_alpha_count[read_ioreg(REG_VCOUNT)];

  for (lnum = 0; lnum < layer_count; lnum++) {
    u32 layer = layer_order[lnum];
    bool is_obj = layer & 0x4;
    if (is_obj && obj_enabled) {
      bool can_skip_blend = !has_trans_obj && !objlayer_is_1st_tgt;

      // If it's the first layer, make sure to fill with backdrop color.
      if (!base_done)
        fill_line_background<bgmode, dsttype>(start, end, dst_ptr);

      // Optimization: skip blending mode if no blending can happen to this layer
      if (objmode == STCKCOLOR && can_skip_blend)
        render_scanline_objs<dsttype, INDXCOLOR>(
          layer & 0x3, start, end, dst_ptr, &palette_ram_converted[0x100]);
      else
        render_scanline_objs<dsttype, objmode>(
          layer & 0x3, start, end, dst_ptr, &palette_ram_converted[0x100]);

      base_done = 1;
    }
    else if (!is_obj && ((1 << layer) & enabled_layers)) {
      bool layer_is_1st_tgt = ((read_ioreg(REG_BLDCNT) >> layer) & 1) != 0;
      bool can_skip_blend = !has_trans_obj && !layer_is_1st_tgt;

      bool is_affine = (video_mode >= 1) && (layer >= 2);
      u32 fnidx = (base_done) | (is_affine ? 2 : 0);

      // Can optimize rendering if no blending can really happen.
      // If stack mode, no blending and not base layer, we might speed up a bit
      if (bgmode == STCKCOLOR && can_skip_blend) {
        static const tile_render_function rdfns[4] = {
          render_scanline_text<dsttype, INDXCOLOR, true>,
          render_scanline_text<dsttype, INDXCOLOR, false>,
          render_scanline_affine<dsttype, INDXCOLOR, true>,
          render_scanline_affine<dsttype, INDXCOLOR, false>,
        };
        rdfns[fnidx](layer, start, end, dst_ptr, palette_ram_converted);
      } else {
        static const tile_render_function rdfns[4] = {
          render_scanline_text<dsttype, bgmode, true>,
          render_scanline_text<dsttype, bgmode, false>,
          render_scanline_affine<dsttype, bgmode, true>,
          render_scanline_affine<dsttype, bgmode, false>,
        };
        rdfns[fnidx](layer, start, end, dst_ptr, palette_ram_converted);
      }

      base_done = 1;
    }
  }

  // Render background if we did not render any active layer.
  if (!base_done)
    fill_line_background<bgmode, dsttype>(start, end, dst_ptr);
}


// Renders all layers honoring color effects (blending, brighten/darken).
// It uses different rendering routines depending on the coloring effect
// requirements, speeding up common cases where no effects are used.

// No effects use NORMAL mode (RBB565 color is written on the buffer).
// For blending, we use BLEND mode to record the two top-most pixels.
// For other effects we use COLOR16, which records an indexed color in the
// buffer (used for darken/brighten effects at later passes) or COLOR32,
// which similarly uses an indexed color for rendering but recording one
// color for the background and another one for the object layer.
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

/* 12.4% of core 1 on 2026-10-10, 2.2 KB, in flash: IRAM for the same reason */
GBA_C1_HOT static void render_w_effects(
  u32 start, u32 end, u16* scanline, u32 enable_flags,
#else /* GBSP_BACKEND_VARIANT */

static void render_w_effects(
  u32 start, u32 end, u16* scanline, u32 enable_flags,
#endif /* GBSP_BACKEND_VARIANT */
  const layer_render_struct *renderers
) {
  bool effects_enabled = enable_flags & 0x20;   // Window bit for effects.
  bool obj_blend = obj_alpha_count[read_ioreg(REG_VCOUNT)] > 0;
  u16 bldcnt = read_ioreg(REG_BLDCNT);

  // If the window bits disable effects, default to NONE
  u32 effect_type = effects_enabled ? ((bldcnt >> 6) & 0x03)
                                    : COL_EFFECT_NONE;

  switch (effect_type) {
  case COL_EFFECT_BRIGHT:
    {
      // If no layers are 1st target, no effect will really happen.
      bool some_1st_tgt = (read_ioreg(REG_BLDCNT) & 0x3F) != 0;
      // If the factor is zero, it's the same as "regular" rendering.
      bool non_zero_blend = (read_ioreg(REG_BLDY) & 0x1F) != 0;
      if (some_1st_tgt && non_zero_blend) {
        if (obj_blend) {
          u32 tmp_buf[240];
          renderers->indexed_u32(start, end, tmp_buf, enable_flags);
          merge_blend<BLEND_BRIGHT, true>(start, end, scanline, tmp_buf);
        } else {
          renderers->indexed_u16(start, end, scanline, enable_flags);
          merge_brightness<BLEND_BRIGHT>(start, end, scanline);
        }
        return;
      }
    }
    break;

  case COL_EFFECT_DARK:
    {
      // If no layers are 1st target, no effect will really happen.
      bool some_1st_tgt = (read_ioreg(REG_BLDCNT) & 0x3F) != 0;
      // If the factor is zero, it's the same as "regular" rendering.
      bool non_zero_blend = (read_ioreg(REG_BLDY) & 0x1F) != 0;
      if (some_1st_tgt && non_zero_blend) {
        if (obj_blend) {
          u32 tmp_buf[240];
          renderers->indexed_u32(start, end, tmp_buf, enable_flags);
          merge_blend<BLEND_DARK, true>(start, end, scanline, tmp_buf);
        } else {
          renderers->indexed_u16(start, end, scanline, enable_flags);
          merge_brightness<BLEND_DARK>(start, end, scanline);
        }
        return;
      }
    }
    break;

  case COL_EFFECT_BLEND:
    {
      // If no layers are 1st or 2nd target, no effect will really happen.
      bool some_1st_tgt = (read_ioreg(REG_BLDCNT) & 0x003F) != 0;
      bool some_2nd_tgt = (read_ioreg(REG_BLDCNT) & 0x3F00) != 0;
      // If 1st target is 100% opacity and 2nd is 0%, just render regularly.
      bool non_trns_tgt = (read_ioreg(REG_BLDALPHA) & 0x1F1F) != 0x001F;
      if (some_1st_tgt && some_2nd_tgt && non_trns_tgt) {
        u32 tmp_buf[240];
        renderers->stacked(start, end, tmp_buf, enable_flags);
        if (obj_blend)
          merge_blend<BLEND_ONLY, true>(start, end, scanline, tmp_buf);
        else
          merge_blend<BLEND_ONLY, false>(start, end, scanline, tmp_buf);
        return;
      }
    }
    break;

  case COL_EFFECT_NONE:
    // Default case, see below.
    break;
  };

  // Default rendering mode, without layer effects (except perhaps sprites).
  if (obj_blend) {
    u32 tmp_buf[240];
    renderers->stacked(start, end, tmp_buf, enable_flags);
    merge_blend<OBJ_BLEND, true>(start, end, scanline, tmp_buf);
  } else {
    renderers->fullcolor(start, end, scanline, enable_flags);
  }
}

#define bitmap_layer_render_functions(rdmode, dsttype, mode, ttype, w, h) {   \
 {                                                                            \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, BLIT, false>,   \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, SCALED, false>, \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, ROTATED, false>,\
 },                                                                           \
 {                                                                            \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, BLIT, true>,    \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, SCALED, true>,  \
   render_scanline_bitmap<rdmode, dsttype, mode, ttype, w, h, ROTATED, true>, \
 }                                                                            \
}

static const bitmap_layer_render_struct idx32_bmrend[3][2] =
{
  bitmap_layer_render_functions(INDXCOLOR, u32, 3, u16, 240, 160),
  bitmap_layer_render_functions(INDXCOLOR, u32, 4, u8,  240, 160),
  bitmap_layer_render_functions(INDXCOLOR, u32, 5, u16, 160, 128)
};

// Render the BG and OBJ in a bitmap scanline from start to end ONLY if
// enable_flag allows that layer/OBJ.

template<rendtype bgmode, rendtype objmode, typename dsttype>
static void bitmap_render_layers(
  u32 start, u32 end, dsttype *scanline, u32 enable_flags)
{
  u16 dispcnt = read_ioreg(REG_DISPCNT);
  bool has_trans_obj = obj_alpha_count[read_ioreg(REG_VCOUNT)];
  bool objlayer_is_1st_tgt = (read_ioreg(REG_BLDCNT) & 0x10) != 0;
  bool bg2_is_1st_tgt = (read_ioreg(REG_BLDCNT) & 0x4) != 0;

  // Fill in the renderers for a layer based on the mode type,
  static const bitmap_layer_render_struct renderers[3][2] =
  {
    bitmap_layer_render_functions(bgmode, dsttype, 3, u16, 240, 160),
    bitmap_layer_render_functions(bgmode, dsttype, 4, u8,  240, 160),
    bitmap_layer_render_functions(bgmode, dsttype, 5, u16, 160, 128)
  };

  const u32 mosamount = read_ioreg(REG_MOSAIC) & 0xFF;
  u32 bg_control = read_ioreg(REG_BG2CNT);
  u32 mmode = ((bg_control & 0x40) && (mosamount != 0)) ? 1 : 0;

  unsigned modeidx = (dispcnt & 0x07) - 3;
  const bitmap_layer_render_struct *mode_rend = &renderers[modeidx][mmode];
  const bitmap_layer_render_struct *idxm_rend = &idx32_bmrend[modeidx][mmode];

  u32 current_layer;
  u32 layer_order_pos;

  fill_line_background<bgmode, dsttype>(start, end, scanline);

  for(layer_order_pos = 0; layer_order_pos < layer_count; layer_order_pos++)
  {
    current_layer = layer_order[layer_order_pos];
    if(current_layer & 0x04)
    {
      if (enable_flags & 0x10) {
        bool can_skip_blend = !has_trans_obj && !objlayer_is_1st_tgt;

        // Optimization: skip blending mode if no blending can happen to this layer
        if (objmode == STCKCOLOR && can_skip_blend)
          render_scanline_objs<dsttype, INDXCOLOR>(
            current_layer & 3, start, end, scanline, &palette_ram_converted[0x100]);
        else
          render_scanline_objs<dsttype, objmode>(
            current_layer & 3, start, end, scanline, &palette_ram_converted[0x100]);
      }
    }
    else
    {
      if(enable_flags & 0x04) {
        s32 dx = (s16)read_ioreg(REG_BG2PA);
        s32 dy = (s16)read_ioreg(REG_BG2PC);

        // Optimization: Skip stack mode if there's no blending happening.
        bool can_skip_blend = !has_trans_obj && !bg2_is_1st_tgt;
        const bitmap_layer_render_struct *rd =
          (bgmode == STCKCOLOR && can_skip_blend) ? idxm_rend : mode_rend;

        if (dy)
          rd->affine_render(start, end, scanline, palette_ram_converted);
        else if (dx == 256)
          rd->blit_render(start, end, scanline, palette_ram_converted);
        else
          rd->scale_render(start, end, scanline, palette_ram_converted);
      }
    }
  }
}


static const layer_render_struct tile_mode_renderers = {
  .fullcolor   = tile_render_layers<FULLCOLOR, FULLCOLOR, u16>,
  .indexed_u16 = tile_render_layers<INDXCOLOR, INDXCOLOR, u16>,
  .indexed_u32 = tile_render_layers<INDXCOLOR, STCKCOLOR, u32>,
  .stacked     = tile_render_layers<STCKCOLOR, STCKCOLOR, u32>,
};

static const layer_render_struct bitmap_mode_renderers = {
  .fullcolor   = bitmap_render_layers<FULLCOLOR, FULLCOLOR, u16>,
  .indexed_u16 = bitmap_render_layers<INDXCOLOR, INDXCOLOR, u16>,
  .indexed_u32 = bitmap_render_layers<INDXCOLOR, STCKCOLOR, u32>,
  .stacked     = bitmap_render_layers<STCKCOLOR, STCKCOLOR, u32>,
};

// Renders a full scanline, given an enable_flags mask (for which layers and
// effects are enabled).
static void render_scanline_conditional(
  u32 start, u32 end, u16 *scanline, u32 enable_flags)
{
  u16 dispcnt = read_ioreg(REG_DISPCNT);
  u32 video_mode = dispcnt & 0x07;

  // Check if any layer is actually active.
  if (layer_count && (enable_flags & 0x1F)) {
    // Color effects currently only supported in indexed-color modes (tiled and mode 4)
    if(video_mode < 3)
      render_w_effects(start, end, scanline, enable_flags, &tile_mode_renderers);
    else if (video_mode == 4)
      render_w_effects(start, end, scanline, enable_flags, &bitmap_mode_renderers);
    else
      // TODO: Implement mode 3 & 5 color effects (at least partially, ie. ST objs)
      bitmap_mode_renderers.fullcolor(start, end, scanline, enable_flags);
  }
  else
    // Render the backdrop color, since no layers are enabled/visible.
    render_backdrop(start, end, scanline);
}

// Renders the are outside of all active windows
static void render_windowout_pass(u16 *scanline, u32 start, u32 end)
{
  u32 winout = read_ioreg(REG_WINOUT);
  u32 wndout_enable = winout & 0x3F;

  render_scanline_conditional(start, end, scanline, wndout_enable);
}

// Renders window-obj. This is a pixel-level windowing effect, based on sprites
// (objects) with a special rendering mode (the sprites are not themselves
// visible but rather "enable" other pixels to be rendered conditionally).
static void render_windowobj_pass(u16 *scanline, u32 start, u32 end)
{
  u32 winout = read_ioreg(REG_WINOUT);
  u32 wndout_enable = winout & 0x3F;

  // First we render the "window-out" segment.
  render_scanline_conditional(start, end, scanline, wndout_enable);

  // Now we render the objects in "copy" mode. This renders the scanline in
  // WinObj-mode to a temporary buffer and performs a "copy-mode" render.
  // In this mode, we copy pixels from the temp buffer to the final buffer
  // whenever an object pixel is rendered.
  render_scanline_objs<u16, PIXCOPY>(4, start, end, scanline, NULL);

  // TODO: Evaluate whether it's better to render the whole line and copy,
  // or render subsegments and copy as we go (depends on the pixel/obj count)
}

// If the window Y coordinates are out of the window range we can skip
// rendering the inside of the window.
inline bool in_window_y(u32 vcount, u32 top, u32 bottom) {
  // TODO: check if these are reversed when top-bottom are also reversed.
  if (top > 227)     // This causes the window to be invisible
    return false;
  if (bottom > 227)  // This makes it all visible
    return true;

  if (top > bottom)  /* Reversed: if not in the "band" */
    return vcount > top || vcount <= bottom;

  return vcount >= top && vcount < bottom;
}

// Renders window 0/1. Checks boundaries and divides the segment into
// subsegments (if necessary) rendering each one in their right mode.
// outfn is called for "out-of-window" rendering.
template<window_render_function outfn, unsigned winnum>
static void render_window_n_pass(u16 *scanline, u32 start, u32 end)
{
  u32 vcount = read_ioreg(REG_VCOUNT);
  // Check the Y coordinates to check if they fall in the right row
  u32 win_top = read_ioreg(REG_WINxV(winnum)) >> 8;
  u32 win_bot = read_ioreg(REG_WINxV(winnum)) & 0xFF;
  // Check the X coordinates and generate up to three segments
  // Clip the coordinates to the [start, end) range.
  u32 win_lraw = read_ioreg(REG_WINxH(winnum)) >> 8;
  u32 win_rraw = read_ioreg(REG_WINxH(winnum)) & 0xFF;
  u32 win_l = MAX(start, MIN(end, win_lraw));
  u32 win_r = MAX(start, MIN(end, win_rraw));
  bool goodwin = win_lraw < win_rraw;

  if (!in_window_y(vcount, win_top, win_bot) || (win_lraw == win_rraw))
    // WindowN is completely out, just render all out.
    outfn(scanline, start, end);
  else {
    // Render window withtin the clipped range
    // Enable bits for stuff inside the window (and outside)
    u32 winin = read_ioreg(REG_WININ);
    u32 wndn_enable = (winin >> (8 * winnum)) & 0x3F;

    // If the window is defined upside down, the areas are inverted.
    if (goodwin) {
      // Render [start, win_l) range (which is outside the window)
      if (win_l != start)
        outfn(scanline, start, win_l);
      // Render the actual window0 pixels
      render_scanline_conditional(win_l, win_r, scanline, wndn_enable);
      // Render the [win_l, end] range (outside)
      if (win_r != end)
        outfn(scanline, win_r, end);
    } else {
      // Render [0, win_r) range (which is "inside" window0)
      if (win_r != start)
        render_scanline_conditional(start, win_r, scanline, wndn_enable);
      // The actual window is now outside, render recursively
      outfn(scanline, win_r, win_l);
      // Render the [win_l, 240] range ("inside")
      if (win_l != end)
        render_scanline_conditional(win_l, end, scanline, wndn_enable);
    }
  }
}

// Renders a full scaleline, taking into consideration windowing effects.
// Breaks the rendering step into N steps, for each windowed region.
static void render_scanline_window(u16 *scanline)
{
  u16 dispcnt = read_ioreg(REG_DISPCNT);
  u32 win_ctrl = (dispcnt >> 13);

  // Priority decoding for windows
  switch (win_ctrl) {
  case 0x0: // No windows are active.
    render_scanline_conditional(0, 240, scanline);
    break;

  case 0x1: // Window 0
    render_window_n_pass<render_windowout_pass, 0>(scanline, 0, 240);
    break;

  case 0x2: // Window 1
    render_window_n_pass<render_windowout_pass, 1>(scanline, 0, 240);
    break;

  case 0x3: // Window 0 & 1
    render_window_n_pass<render_window_n_pass<render_windowout_pass, 1>, 0>(scanline, 0, 240);
    break;

  case 0x4: // Window Obj
    render_windowobj_pass(scanline, 0, 240);
    break;

  case 0x5: // Window 0 & Obj
    render_window_n_pass<render_windowobj_pass, 0>(scanline, 0, 240);
    break;

  case 0x6: // Window 1 & Obj
    render_window_n_pass<render_windowobj_pass, 1>(scanline, 0, 240);
    break;

  case 0x7: // Window 0, 1 & Obj
    render_window_n_pass<render_window_n_pass<render_windowobj_pass, 1>, 0>(scanline, 0, 240);
    break;
  }
}

static const u8 active_layers[] = {
  0x1F,   // Mode 0, Tile BG0-3 and OBJ
  0x17,   // Mode 1, Tile BG0-2 and OBJ
  0x1C,   // Mode 2, Tile BG2-3 and OBJ
  0x14,   // Mode 3, BMP  BG2 and OBJ
  0x14,   // Mode 4, BMP  BG2 and OBJ
  0x14,   // Mode 5, BMP  BG2 and OBJ
  0,      // Unused
  0,
};


#ifdef GBAPROF
extern "C" { int64_t gbaprof_render_us, gbaprof_wait_us; u32 gbaprof_lag159, gbaprof_syncs, gbaprof_lag80, gbaprof_wakes; }
/* wait by cause: 1/2 CPU VRAM BG/OBJ, 3/4 DMA VRAM BG/OBJ, 5 OAM, 6 palette, 7 frame end */
extern "C" { u32 gbsp_sync_src; int64_t gbaprof_wait_by[8]; }   /* scanline rendering time, read by gbsp/main/main.c */
#endif

#ifdef GPSP_VIDEO_SNAPSHOTS
/* draws one line from its snapshot (core 1 on the board, inline on the PC) */
static void render_line(u32 vcount, const rline_t *ls)
{
#ifdef GBAPROF
  const int64_t gbaprof_t0 = rg_system_timer();
#endif
  render_io = ls->io;
  render_oam = r_oam[ls->oamb];
  render_pal = r_pal[ls->palb];
  r_affine_x[0] = ls->ax[0]; r_affine_x[1] = ls->ax[1];
  r_affine_y[0] = ls->ay[0]; r_affine_y[1] = ls->ay[1];
  u16 dispcnt = read_ioreg(REG_DISPCNT);
  u16 *screen_offset = get_screen_pixels() + (vcount * get_screen_pitch());
  u32 video_mode = dispcnt & 0x07;

  // If OAM has been modified since the last scanline has been updated then
  // reorder and reprofile the OBJ lists.
  if(ls->oam)
    order_obj(video_mode);

  order_layers((dispcnt >> 8) & active_layers[video_mode], vcount);

  // If the screen is in in forced blank draw pure white.
  if(dispcnt & 0x80)
    memset(screen_offset, 0xff, 240*sizeof(u16));
  else
    render_scanline_window(screen_offset);
#ifdef GBAPROF
  gbaprof_render_us += rg_system_timer() - gbaprof_t0;
#endif
}

/* Lines queued by core 0 (gbsp_rq) and drawn (gbsp_rd), both counting up;
   rq_line[] holds each queued line's vcount. Core 0 calls gbsp_render_sync()
   before any write to palette, VRAM or OAM while lines are outstanding
   (gba_memory.c) and at the end of the frame, so every line is drawn with the
   video memory it had when it was emulated, however late core 1 draws it:
   games rewrite OAM/VRAM/palette in the vblank for the next frame (TMNT's
   turtles came out half transparent when core 1 drew the last lines after). */
extern "C" { volatile u32 gbsp_rq, gbsp_rd; }
static u8 rq_line[256];

static inline void render_next(void)
{
  u32 rd = __atomic_load_n(&gbsp_rd, __ATOMIC_RELAXED);
  u32 line = rq_line[rd & 255];
  render_line(line, &rlines[line]);
  __atomic_store_n(&gbsp_rd, rd + 1, __ATOMIC_RELEASE);
}

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
extern "C" { int gbsp_render_core1; }
static TaskHandle_t rtask;

static void render_task(void *arg)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
#ifdef GBAPROF
    gbaprof_wakes++;
#endif
    while (__atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE) != __atomic_load_n(&gbsp_rq, __ATOMIC_ACQUIRE))
    {
      __sync_synchronize();
      render_next();
    }
  }
}

/* retro-go's display task is 6 on core 1: below it the LCD DMA buffers are
   refilled as soon as they free up (the bus stays busy); since the VRAM copy,
   a late renderer rarely makes core 0 wait */
#ifndef GBSP_RENDER_PRIO
#define GBSP_RENDER_PRIO 5
#endif
/* core 0: start the line renderer on core 1 */
#ifdef GBAPROF
extern "C" { extern u32 gbsp_opt_rint; }   /* the frontend's card file */
#endif

extern "C" bool gbsp_render_configure(bool core1)
{
  render_vram = live_vram;
  if (!core1) return false;
#if !CONFIG_FREERTOS_UNICORE
  rlines = (rline_t *)heap_caps_malloc(160 * sizeof(rline_t), MALLOC_CAP_SPIRAM);
  r_oam = (u16 (*)[512])heap_caps_malloc(2 * sizeof(oam_ram), MALLOC_CAP_SPIRAM);
  r_pal = (u16 (*)[512])heap_caps_malloc(R_PAL_N * 512 * sizeof(u16), MALLOC_CAP_SPIRAM);
  if (!r_pal && (r_pal = (u16 (*)[512])heap_caps_malloc(4 * 512 * sizeof(u16), MALLOC_CAP_SPIRAM)))
    gbsp_pal_ring = 4;
  gbsp_rvram = (u8 *)heap_caps_malloc(96 * 1024, MALLOC_CAP_SPIRAM);
  if (rlines && r_oam && r_pal && gbsp_rvram &&
      xTaskCreatePinnedToCore(render_task, "gba_render", 6144, NULL, GBSP_RENDER_PRIO, &rtask, 1) == pdPASS) {
    render_vram = gbsp_rvram;
    gbsp_render_core1 = 1;
    gbsp_render_invalidate();
    return true;
  }
  heap_caps_free(rlines); rlines = NULL;
  heap_caps_free(r_oam); r_oam = NULL;
  heap_caps_free(r_pal); r_pal = NULL;
  heap_caps_free(gbsp_rvram); gbsp_rvram = NULL;
#endif
  RG_LOGW("video: Core 1 renderer unavailable; using Sync");
  return false;
}
#ifdef GBAPROF
#include "esp_cpu.h"
extern "C" { u32 gbaprof_notify_cycles, gbaprof_notifies; }
#endif
/* core 1 is woken every (GBSP_WAKE_MASK + 1) lines: the less it lags, the less
   core 0 waits in gbsp_render_sync when the game writes VRAM mid-frame */
#ifndef GBSP_WAKE_MASK
#define GBSP_WAKE_MASK 7
#endif
/* core 0: wait until core 1 has drawn every queued line */
extern "C" void gbsp_render_sync(void)
{
  if (__atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE) == __atomic_load_n(&gbsp_rq, __ATOMIC_ACQUIRE))
    return;
#ifdef GBAPROF
  const int64_t t0 = rg_system_timer();
#endif
#ifdef GBAPROF
  gbaprof_syncs++;
#endif
  xTaskNotifyGive(rtask);
  while (__atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE) != __atomic_load_n(&gbsp_rq, __ATOMIC_ACQUIRE))
    ;
  __sync_synchronize();
#ifdef GBAPROF
  gbaprof_wait_us += rg_system_timer() - t0;
  gbaprof_wait_by[gbsp_sync_src & 7] += rg_system_timer() - t0;
  gbsp_sync_src = 0;
#endif
}

#ifdef GBAPROF
extern "C" { extern u32 gbsp_opt_core1_idle; }   /* defined by the frontend */
#endif

static void line_ready(u32 vcount)
{
#ifdef GBAPROF
  /* What core 1 costs core 0 through the 32 KB instruction cache they share.
     Mario Kart's hot translated code is ~120 KB behind that cache and core 1
     streams the affine-road renderer from flash through it at the same time,
     so core 0's misses are not all its own fault. With this on no line is
     queued, core 1 is never woken, and core 0 runs exactly the same emulation
     with the cache to itself: the drop in "cpu" is the contention.

     Nothing is queued, so gbsp_rq never advances and core 0's sync loop
     (gbsp_rd == gbsp_rq) falls straight through -- it cannot hang. The picture
     holds the last frame drawn: this is a measurement, not a way to play. */
  if (gbsp_opt_core1_idle)
    return;
#endif
  if (!gbsp_render_core1)
  {
    render_line(vcount, &rlines[vcount]);
    return;
  }
  u32 rq = __atomic_load_n(&gbsp_rq, __ATOMIC_RELAXED);
  rq_line[rq & 255] = vcount;
  __atomic_store_n(&gbsp_rq, rq + 1, __ATOMIC_RELEASE);
#ifdef GBAPROF
  if (vcount == 159)
    gbaprof_lag159 += gbsp_rq - gbsp_rd;
  if (vcount == 80)
    gbaprof_lag80 += gbsp_rq - gbsp_rd;
#endif
  if ((vcount & GBSP_WAKE_MASK) == GBSP_WAKE_MASK || vcount == 159)
  {
#ifdef GBAPROF
    u32 c0 = esp_cpu_get_cycle_count();
#endif
    xTaskNotifyGive(rtask);
#ifdef GBAPROF
    gbaprof_notify_cycles += esp_cpu_get_cycle_count() - c0;
    gbaprof_notifies++;
#endif
  }
}
#else
extern "C" void gbsp_render_start(void) {}
/* PC harness: RLAG=n draws each line n lines late (and the frame's last
   ones only at the next sync), like core 1 on the board */
static int rlag = -1;
#include <stdio.h>
static u32 syncstat[4][232];   /* SYNCSTAT: syncs with lines outstanding, by region and vcount */
extern "C" u32 gbsp_sync_region;
u32 gbsp_sync_region;
static void syncstat_dump(void)
{
  static const char *n[4] = {"pal", "vram", "oam", "dma/end"};
  for (int r = 0; r < 4; r++) {
    u32 t = 0; for (int v = 0; v < 232; v++) t += syncstat[r][v];
    printf("SYNC %s total %u:", n[r], t);
    for (int v = 0; v < 232; v++) if (syncstat[r][v]) printf(" %d:%u", v, syncstat[r][v]);
    printf("\n");
  }
}
extern "C" void gbsp_render_sync(void)
{
  static int init;
  if (!init) { init = 1; if (getenv("SYNCSTAT")) atexit(syncstat_dump); }
  if (gbsp_rd != gbsp_rq) syncstat[gbsp_sync_region & 3][live_ioreg(REG_VCOUNT) % 232]++;
  gbsp_sync_region = 3;
  while (__atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE) != __atomic_load_n(&gbsp_rq, __ATOMIC_ACQUIRE))
    render_next();
}
static void line_ready(u32 vcount)
{
  if (rlag < 0) rlag = getenv("RLAG") ? atoi(getenv("RLAG")) : 0;
  rq_line[gbsp_rq & 255] = vcount;
  gbsp_rq++;
  while (gbsp_rq - gbsp_rd > (u32)rlag)
    render_next();
}
#endif
extern "C" void gbsp_render_wait(void)
{
#ifdef GBAPROF
  gbsp_sync_src = 7;
#endif
  gbsp_render_sync();
}

#ifdef GPSP_VIDEO_SNAPSHOTS
extern "C" void gbsp_display_poll(void);
#endif
#ifdef GBSP_RVRAM
extern "C" void gbsp_vram_mark_all(void)
{
  gbsp_vram_dirty[0] = gbsp_vram_dirty[1] = gbsp_vram_dirty[2] = 0xFFFFFFFF;
  gbsp_vram_dirty_any = 1;
}
/* before a line is queued: the lines queued before the VRAM writes are drawn
   with the old contents, then the written pages are copied.

   2026-10-13: in the bitmap modes with two frames (4 and 5) the line being
   queued reads only the frame DISPCNT selects (and the OBJ tiles from
   0x14000), so the pages of the other frame - the one the game is drawing -
   stay dirty and are not copied, and nothing waits for core 1 for them. They
   are copied at the first line that reads them (the flip). Not copying is
   always right for the lines already queued: they must see the old contents.
   NFS Underground's race draws its 3D scene into the back frame through the
   whole frame: the same pages were copied again at every line, with a wait
   for core 1 each time (memcpy 25 % + render sync 8 % of core 0, board
   16cae1d4). GBAPROF counts the pages copied and deferred. */
#ifdef GBAPROF
extern "C" { u32 gbsp_prof_vram_copied, gbsp_prof_vram_deferred; }
#endif
static void gbsp_vram_flush(u16 dispcnt)
{
  u32 keep[3] = {0, 0, 0};
  u32 mode = dispcnt & 7;
  if (mode == 4 || mode == 5)
  {
    /* the back frame: pages 0-39 (0x0000-0x9FFF) or 40-79 (0xA000-0x13FFF) */
    if (dispcnt & 0x10)
      keep[0] = 0xFFFFFFFF, keep[1] = 0x000000FF;
    else
      keep[1] = 0xFFFFFF00, keep[2] = 0x0000FFFF;
  }
  u32 copy[3], any = 0;
  for (int w = 0; w < 3; w++)
    any |= copy[w] = gbsp_vram_dirty[w] & ~keep[w];
  if (!any)
  {
#ifdef GBAPROF
    gbsp_prof_vram_deferred++;
#endif
    return;                 /* only back-frame pages written: nothing to wait for */
  }
  if (__atomic_load_n(&gbsp_rq, __ATOMIC_ACQUIRE) != __atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE))
  {
#ifdef GBAPROF
    gbsp_sync_src = 3;
#endif
    gbsp_render_sync();
  }
  for (int w = 0; w < 3; w++)
  {
    u32 bits = copy[w];
    gbsp_vram_dirty[w] &= ~bits;
    while (bits)
    {
      u32 p = w * 32 + __builtin_ctz(bits);
      bits &= bits - 1;
      memcpy(gbsp_rvram + p * 1024, live_vram + p * 1024, 1024);
#ifdef GBAPROF
      gbsp_prof_vram_copied++;
#endif
    }
  }
  gbsp_vram_dirty_any = (gbsp_vram_dirty[0] | gbsp_vram_dirty[1] | gbsp_vram_dirty[2]) != 0;
}
#endif

void update_scanline(void)
{
  u16 dispcnt = live_ioreg(REG_DISPCNT);
  u32 vcount = live_ioreg(REG_VCOUNT);
#ifdef GBSP_RVRAM
  if (gbsp_render_core1 && gbsp_vram_dirty_any && vcount < 160)
    gbsp_vram_flush(dispcnt);
#endif
#ifdef GPSP_VIDEO_SNAPSHOTS
  if ((vcount & 31) == 31)
    gbsp_display_poll();   /* a frame finished while the display was busy */
#endif
  u32 video_mode = dispcnt & 0x07;

  if(skip_next_frame)
    return;

  if (vcount >= 160) return;
  if (!gbsp_render_core1) {
    render_io = io_registers;
    render_oam = oam_ram;
    render_pal = live_pal;
    r_affine_x[0] = affine_reference_x[0]; r_affine_x[1] = affine_reference_x[1];
    r_affine_y[0] = affine_reference_y[0]; r_affine_y[1] = affine_reference_y[1];
    if (reg[OAM_UPDATED]) { order_obj(video_mode); reg[OAM_UPDATED] = 0; }
    order_layers((dispcnt >> 8) & active_layers[video_mode], vcount);
    u16 *screen_offset = get_screen_pixels() + vcount * get_screen_pitch();
    if (dispcnt & 0x80) memset(screen_offset, 0xff, 240 * sizeof(u16));
    else render_scanline_window(screen_offset);
  } else {
  rline_t *ls = &rlines[vcount];
  memcpy(ls->io, io_registers, sizeof(ls->io));
  ls->ax[0] = affine_reference_x[0]; ls->ax[1] = affine_reference_x[1];
  ls->ay[0] = affine_reference_y[0]; ls->ay[1] = affine_reference_y[1];
  ls->oam = reg[OAM_UPDATED] || !r_oam_valid;
  if (ls->oam)
  {
    u32 nb = r_oam_cur ^ 1;
    if ((s32)(r_oam_last[nb] - __atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE)) >= 0)   /* a queued line still reads it */
    {
#ifdef GBAPROF
      gbsp_sync_src = 5;
#endif
      gbsp_render_sync();
    }
    memcpy(r_oam[nb], oam_ram, sizeof(oam_ram));
    r_oam_cur = nb;
    r_oam_valid = true;
  }
  ls->oamb = r_oam_cur;
  if (gbsp_pal_dirty)
  {
    u32 nb = (r_pal_cur + 1) % gbsp_pal_ring;
    if ((s32)(r_pal_last[nb] - __atomic_load_n(&gbsp_rd, __ATOMIC_ACQUIRE)) >= 0)
    {
#ifdef GBAPROF
      gbsp_sync_src = 6;
#endif
      gbsp_render_sync();
    }
    memcpy(r_pal[nb], live_pal, 512 * sizeof(u16));
    r_pal_cur = nb;
    gbsp_pal_dirty = 0;
  }
  ls->palb = r_pal_cur;
  r_pal_last[r_pal_cur] = gbsp_rq;
  r_oam_last[r_oam_cur] = gbsp_rq;   /* this line's queue index */
  reg[OAM_UPDATED] = 0;
  line_ready(vcount);

  }

  // Mode 0 does not use any affine params at all.
  if (video_mode) {
    // Account for vertical mosaic effect, by correcting affine references.
    const u32 bgmosv = ((live_ioreg(REG_MOSAIC) >> 4) & 0xF) + 1;

    if (live_ioreg(REG_BG2CNT) & 0x40) {   // Mosaic enabled for this BG
      if ((vcount % bgmosv) == bgmosv-1) { // Correct after the last line
        affine_reference_x[0] += (s16)live_ioreg(REG_BG2PB) * bgmosv;
        affine_reference_y[0] += (s16)live_ioreg(REG_BG2PD) * bgmosv;
      }
    } else {
      affine_reference_x[0] += (s16)live_ioreg(REG_BG2PB);
      affine_reference_y[0] += (s16)live_ioreg(REG_BG2PD);
    }

    if (live_ioreg(REG_BG3CNT) & 0x40) {
      if ((vcount % bgmosv) == bgmosv-1) {
        affine_reference_x[1] += (s16)live_ioreg(REG_BG3PB) * bgmosv;
        affine_reference_y[1] += (s16)live_ioreg(REG_BG3PD) * bgmosv;
      }
    } else {
      affine_reference_x[1] += (s16)live_ioreg(REG_BG3PB);
      affine_reference_y[1] += (s16)live_ioreg(REG_BG3PD);
    }
  }
}
#endif /* GPSP_VIDEO_SNAPSHOTS */

#if !RG_BUILD_RELEASE
extern "C" int gbsp_render_stack_free(void)
{
#if defined(GPSP_VIDEO_SNAPSHOTS) && defined(ESP_PLATFORM)
  if (gbsp_render_core1 && rtask)
    return (int)(uxTaskGetStackHighWaterMark(rtask) * sizeof(StackType_t));
#endif
  return -1;
}
#endif

extern "C" void gbsp_video_set_framebuffer(uint16_t *pixels) { gba_screen_pixels = pixels; }

extern "C" void gbsp_render_invalidate(void)
{
  gbsp_render_wait();
  r_oam_valid = false;
  reg[OAM_UPDATED] = 1;
  for (unsigned i = 0; i < 512; i++)
    live_pal[i] = convert_palette(eswap16(palette_ram[i]));
  gbsp_pal_dirty = 1;
  gbsp_vram_mark_all();
}
