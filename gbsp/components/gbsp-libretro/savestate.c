#include "gpsp_video.h"
/* gameplaySP
 *
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

#include "common.h"

const u8 *state_mem_read_ptr;
u8 *state_mem_write_ptr;

#include "gpsp_bson.h"

/* All document pointers originate in the fully bounded root validation below. */
bool bson_contains_key(const u8 *src, const char *key, u8 type)
{
  gp_bson_field_t f;
  return gp_bson_find(src, key, &f) && f.type == type;
}
const u8 *bson_find_key(const u8 *src, const char *key)
{
  gp_bson_field_t f;
  return gp_bson_find(src, key, &f) && (f.type == BSON_TYPE_DOC || f.type == BSON_TYPE_ARR) ? f.value : NULL;
}
bool bson_read_int32(const u8 *src, const char *key, u32 *out)
{
  return gp_bson_i32(src, key, out);
}
bool bson_read_int32_array(const u8 *src, const char *key, u32 *out, unsigned count)
{
  return gp_bson_array(src, key, out, count);
}
bool bson_read_bytes(const u8 *src, const char *key, void *out, unsigned count)
{
  return gp_bson_bytes(src, key, out, count);
}

static bool gba_load_state_impl(const void* src)
{
  u32 i, tmp;
  u8* srcptr = (u8*)src;
  if (!gp_bson_validate(src, GBA_STATE_MEM_SIZE)) return false;
  u32 docsize = bson_read_u32(srcptr);
  if (docsize != GBA_STATE_MEM_SIZE)
    return false;

  if (!bson_read_int32(srcptr, "info-magic", &tmp) || tmp != GBA_STATE_MAGIC)
    return false;
  if (!bson_read_int32(srcptr, "info-version", &tmp) || tmp != GBA_STATE_VERSION)
    return false;

  // Validate that the state file makes sense before unconditionally reading it.
  if (!cpu_check_savestate(srcptr) ||
      !input_check_savestate(srcptr) ||
      !main_check_savestate(srcptr) ||
      !memory_check_savestate(srcptr) ||
      !sound_check_savestate(srcptr))
     return false;

  if (!(cpu_read_savestate(srcptr) &&
      input_read_savestate(srcptr) &&
      main_read_savestate(srcptr) &&
      memory_read_savestate(srcptr) &&
      sound_read_savestate(srcptr)))
  {
     // TODO: this should not happen if the validation above is accurate.
     return false;
  }

  // Generate converted palette (since it is not saved)
  for(i = 0; i < 512; i++)
  {
     palette_ram_converted[i] = convert_palette(eswap16(palette_ram[i]));
  }

  video_reload_counters();

  // Reset most of the frame state and dynarec state
#ifdef HAVE_DYNAREC
  if (dynarec_enable)
    flush_dynarec_caches();
#endif

  instruction_count = 0;
#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
  reg[OAM_UPDATED] = 1;
#ifdef RETRO_GO
  {
    extern u8 gbsp_pal_dirty;   /* video.cpp: the renderer's palette copy */
    gbsp_pal_dirty = 1;
#if defined(XTENSA_ARCH) && defined(ESP_PLATFORM)
    extern void gbsp_vram_mark_all(void);   /* video.cpp: the renderer's VRAM copy */
    gbsp_vram_mark_all();
#endif
  }
#endif

#else /* GBSP_BACKEND_VARIANT */
  reg[OAM_UPDATED] = 1;

#endif /* GBSP_BACKEND_VARIANT */
  return true;
}

void gba_save_state(void* dst)
{
  u8 *stptr = (u8*)dst;
  u8 *wrptr = (u8*)dst;

  // Initial bson size
  bson_write_u32(wrptr, 0);

  // Add some info fields
  bson_write_int32(wrptr, "info-magic", GBA_STATE_MAGIC);
  bson_write_int32(wrptr, "info-version", GBA_STATE_VERSION);

  wrptr += cpu_write_savestate(wrptr);
  wrptr += input_write_savestate(wrptr);
  wrptr += main_write_savestate(wrptr);
  wrptr += memory_write_savestate(wrptr);
  wrptr += sound_write_savestate(wrptr);

  // The padding space is pushed into a padding field for easy parsing
  {
    unsigned padsize = GBA_STATE_MEM_SIZE - (wrptr - stptr);
    padsize -= 1 + 9 + 4 + 1 + 1;
    *wrptr++ = 0x05;    // Byte array
    bson_write_cstring(wrptr, "zpadding");
    bson_write_u32(wrptr, padsize);
    *wrptr++ = 0;
    wrptr += padsize;
  }

  *wrptr++ = 0;

  // Update the doc size  
  bson_write_u32(stptr, wrptr - stptr);
}

bool gba_load_state(const void *src)
{
  gbsp_render_wait();
  bool loaded = gba_load_state_impl(src);
  /* A failed reader can have partially restored RAM too. */
  gbsp_render_invalidate();
  return loaded;
}
