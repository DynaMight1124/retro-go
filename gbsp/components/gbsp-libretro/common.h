/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
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

#ifndef COMMON_H
#define COMMON_H
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

/* Endianness: gpSP uses MSB_FIRST as the single source of truth for big-
 * endian targets. Toolchain-specific macros are mapped onto MSB_FIRST here
 * so that the rest of the codebase can use one consistent token. Define
 * -DMSB_FIRST in CFLAGS (or rely on the auto-detection below) on big-endian
 * hosts; little-endian is the default when MSB_FIRST is not defined.
 */
#ifndef MSB_FIRST
  #if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&             \
      (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define MSB_FIRST 1
  #elif defined(__BIG_ENDIAN__)
    #define MSB_FIRST 1
  #elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    #define MSB_FIRST 1
  #elif defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) &&                     \
      (__BYTE_ORDER == __BIG_ENDIAN)
    #define MSB_FIRST 1
  #endif
#endif

#else /* GBSP_BACKEND_VARIANT */

#endif /* GBSP_BACKEND_VARIANT */
#define ror(dest, value, shift)                                               \
  dest = ((value) >> (shift)) | ((value) << (32 - (shift)))                   \

#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define MIN(a,b)  ((a) < (b) ? (a) : (b))

#if defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
#if defined(_WIN32)
  #define PATH_SEPARATOR "\\"
  #define PATH_SEPARATOR_CHAR '\\'
#else
  #define PATH_SEPARATOR "/"
  #define PATH_SEPARATOR_CHAR '/'
#endif

/* hot code called by the Xtensa dynarec's translated code: in IRAM, so that
   the 32 KB instruction cache is left to the translated code (PSRAM) */
#if defined(XTENSA_ARCH) && defined(ESP_PLATFORM)
#include "esp_attr.h"
#define XT_HOT IRAM_ATTR
#else
#define XT_HOT
#endif

/* Core-1 renderer code that core 0 pays for: its flash fetches go through the
   same SPI0 cache controller as core 0's PSRAM-resident translated code, so
   core 1 reading its renderer out of flash is a request core 0 waits behind.
   Measured on the board, 2026-10-10, Mario Kart Super Circuit: 54% of core 1's
   ticks were flash code; moving the hot 24.7 KB to IRAM took that to 19% and
   core 0's frame from 19.30 to 18.44 ms, 51 -> 54 fps. It costs 24.7 KB of the
   internal RAM every gbsp game shares and left 60 KB free on the board, with
   nothing refused. */
#if defined(XTENSA_ARCH) && defined(ESP_PLATFORM)
#define GBA_C1_HOT IRAM_ATTR
#else
#define GBA_C1_HOT
#endif
#else /* GBSP_BACKEND_VARIANT */
#if defined(_WIN32)
  #define PATH_SEPARATOR "\\"
  #define PATH_SEPARATOR_CHAR '\\'
#else
  #define PATH_SEPARATOR "/"
  #define PATH_SEPARATOR_CHAR '/'
#endif
#endif /* GBSP_BACKEND_VARIANT */

/* On x86 we pass arguments via registers instead of stack */
#ifdef X86_ARCH
  #define function_cc __attribute__((regparm(2)))
#else
  #define function_cc
#endif

#ifdef ARM_ARCH

#define _BSD_SOURCE // sync
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#endif /* ARM_ARCH */

// Huge thanks to pollux for the heads up on using native file I/O
// functions on PSP for vastly improved memstick performance.

#ifdef PSP
  #include <pspkernel.h>
  #include <pspdebug.h>
  #include <pspctrl.h>
  #include <pspgu.h>
  #include <pspaudio.h>
  #include <pspaudiolib.h>
  #include <psprtc.h>
  #include <time.h>
#else
  typedef unsigned char u8;
  typedef signed char s8;
  typedef unsigned short int u16;
  typedef signed short int s16;
  typedef unsigned int u32;
  typedef signed int s32;
  typedef unsigned long long int u64;
  typedef signed long long int s64;
#endif

#if defined(USE_XBGR1555_FORMAT)
  #define convert_palette(value)  \
    (value & 0x7FFF)
#else
  #define convert_palette(value) \
    (((value & 0x1F) << 11) | ((value & 0x03E0) << 1) | ((value >> 10) & 0x1F))
#endif

#define GBA_SCREEN_WIDTH  (240)
#define GBA_SCREEN_HEIGHT (160)
#define GBA_SCREEN_PITCH  (240)

// The buffer is 16 bit color depth.
// We reserve extra memory at the end for extra effects (winobj rendering).
#define GBA_SCREEN_BUFFER_SIZE  \
  (GBA_SCREEN_PITCH * (GBA_SCREEN_HEIGHT + 1) * sizeof(uint16_t))
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

#ifdef MSB_FIRST
  #define netorder32(value) (value)
#else
  #define netorder32(value) __builtin_bswap32(value)
#endif
#else /* GBSP_BACKEND_VARIANT */

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define netorder32(value) (value)
#else
  #define netorder32(value) __builtin_bswap32(value)
#endif
#endif /* GBSP_BACKEND_VARIANT */

typedef u32 fixed16_16;
typedef u32 fixed8_24;

#define float_to_fp16_16(value)                                               \
  (fixed16_16)((value) * 65536.0)                                             \

#define fp16_16_to_float(value)                                               \
  (float)((value) / 65536.0)                                                  \

#define u32_to_fp16_16(value)                                                 \
  ((value) << 16)                                                             \

#define fp16_16_to_u32(value)                                                 \
  ((value) >> 16)                                                             \

#define fp16_16_fractional_part(value)                                        \
  ((value) & 0xFFFF)                                                          \

#define float_to_fp8_24(value)                                                \
  (fixed8_24)((value) * 16777216.0)                                           \

#define fp8_24_fractional_part(value)                                         \
  ((value) & 0xFFFFFF)                                                        \

#define fixed_div(numerator, denominator, bits)                               \
  (((numerator * (1 << bits)) + (denominator / 2)) / denominator)             \

#define address8(base, offset)                                                \
  *((u8 *)((u8 *)base + (offset)))                                            \

#define address16(base, offset)                                               \
  *((u16 *)((u8 *)base + (offset)))                                           \

#define address32(base, offset)                                               \
  *((u32 *)((u8 *)base + (offset)))                                           \

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
#define eswap8(value)  (value)
#ifdef MSB_FIRST
  #define eswap16(value) __builtin_bswap16(value)
  #define eswap32(value) __builtin_bswap32(value)
#else
  #define eswap16(value) (value)
  #define eswap32(value) (value)
#endif
#else /* GBSP_BACKEND_VARIANT */
#define eswap8(value)  (value)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define eswap16(value) __builtin_bswap16(value)
  #define eswap32(value) __builtin_bswap32(value)
#else
  #define eswap16(value) (value)
  #define eswap32(value) (value)
#endif
#endif /* GBSP_BACKEND_VARIANT */

#define  readaddress8(base, offset) eswap8( address8( base, offset))
#define readaddress16(base, offset) eswap16(address16(base, offset))
#define readaddress32(base, offset) eswap32(address32(base, offset))

#define read_ioreg(regnum) (eswap16(io_registers[(regnum)]))
#define write_ioreg(regnum, val) io_registers[(regnum)] = eswap16(val)
#define read_ioreg32(regnum) (read_ioreg(regnum) | (read_ioreg((regnum)+1) << 16))

#define read_dmareg(regnum, dmachan) (eswap16(io_registers[(regnum) + (dmachan) * 6]))
#define write_dmareg(regnum, dmachan, val) io_registers[(regnum) + (dmachan) * 6] = eswap16(val)
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

#ifdef ESP_PLATFORM
  #include "sdkconfig.h"
  #include <stdlib.h>
  #include <string.h>
  #include <stdarg.h>
  #include <sys/time.h>
  #include <time.h>
#else
  #include <unistd.h>
  #include <time.h>
  #include <stdlib.h>
  #include <string.h>
  #include <stdarg.h>
#endif
#include "cpu.h"
#else /* GBSP_BACKEND_VARIANT */

#ifdef RETRO_GO
#include <rg_system.h>
#endif
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cpu.h"
#endif /* GBSP_BACKEND_VARIANT */
#include "gba_memory.h"
#include "savestate.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "main.h"
#include "cheats.h"
#include "serial.h"

#endif
