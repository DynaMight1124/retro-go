#ifndef PICO_RETRO_GO_PPROF_H
#define PICO_RETRO_GO_PPROF_H

#include <stdint.h>

#ifndef PPROF_LEVEL
#define PPROF_LEVEL 2
#endif

/*
 * Lightweight, application-local implementation of PicoDrive's profiling
 * hooks. The CPU cycle counter keeps the hot-path overhead low enough to
 * measure scanline-level CPU and VDP work on ESP32-class targets.
 *
 * Timings are inclusive. For example, a sound register write made by the 68K
 * can contribute to both the m68k and fm/psg counters.
 */
enum pprof_points {
  pp_main,
  pp_frame,
  pp_draw,
  pp_vdp_scene,
  pp_vdp_planes_lo,
  pp_vdp_sprites_lo,
  pp_vdp_planes_hi,
  pp_vdp_sprites_hi,
  pp_vdp_finish,
  pp_vdp_orcopy,
  pp_sound,
  pp_m68k,
  pp_s68k,
  pp_mem68,
  pp_z80,
  pp_msh2,
  pp_ssh2,
  pp_memsh,
  pp_dummy,
  pp_fm,
  pp_psg,
  pp_pcm_gen,
  pp_pcm_mix,
  pp_cdda,
  pp_cdda_read,
  pp_cdda_mix,
  pp_chd_hunk,
  pp_chd_io,
  pp_chd_audio_copy,
  pp_mix,
  pp_cd_event,
  pp_cd_gfx,
  pp_total_points
};

struct pp_counters {
  uint64_t cycles[pp_total_points];
  uint32_t calls[pp_total_points];
};

extern struct pp_counters pprof_counters;
extern unsigned int pprof_refcounts[pp_total_points];

/*
 * Kept out of this header so esp_cpu.h does not leak the ESP-IDF REG_READ and
 * REG_WRITE macros into PicoDrive sources such as the SVP interpreter.
 */
uint32_t pprof_get_one(void);

/* Genesis/core points are level 2; Sega CD and CHD points are level 3. */
#define PPROF_POINT_LEVEL_main           2
#define PPROF_POINT_LEVEL_frame          2
#define PPROF_POINT_LEVEL_draw           2
#define PPROF_POINT_LEVEL_vdp_scene      2
#define PPROF_POINT_LEVEL_vdp_planes_lo  2
#define PPROF_POINT_LEVEL_vdp_sprites_lo 2
#define PPROF_POINT_LEVEL_vdp_planes_hi  2
#define PPROF_POINT_LEVEL_vdp_sprites_hi 2
#define PPROF_POINT_LEVEL_vdp_finish     2
#define PPROF_POINT_LEVEL_vdp_orcopy     2
#define PPROF_POINT_LEVEL_sound          2
#define PPROF_POINT_LEVEL_m68k           2
#define PPROF_POINT_LEVEL_s68k           3
#define PPROF_POINT_LEVEL_mem68          2
#define PPROF_POINT_LEVEL_z80            2
#define PPROF_POINT_LEVEL_msh2           2
#define PPROF_POINT_LEVEL_ssh2           2
#define PPROF_POINT_LEVEL_memsh          2
#define PPROF_POINT_LEVEL_dummy          2
#define PPROF_POINT_LEVEL_fm             2
#define PPROF_POINT_LEVEL_psg            2
#define PPROF_POINT_LEVEL_pcm_gen        3
#define PPROF_POINT_LEVEL_pcm_mix        3
#define PPROF_POINT_LEVEL_cdda           3
#define PPROF_POINT_LEVEL_cdda_read      3
#define PPROF_POINT_LEVEL_cdda_mix       3
#define PPROF_POINT_LEVEL_chd_hunk       3
#define PPROF_POINT_LEVEL_chd_io         3
#define PPROF_POINT_LEVEL_chd_audio_copy 3
#define PPROF_POINT_LEVEL_mix            2
#define PPROF_POINT_LEVEL_cd_event       3
#define PPROF_POINT_LEVEL_cd_gfx         3

#define pprof_start(point) { \
    uint32_t pp_start_##point = 0; \
    if (PPROF_LEVEL >= PPROF_POINT_LEVEL_##point) { \
      pp_start_##point = pprof_get_one(); \
      pprof_refcounts[pp_##point]++; \
    }

#define pprof_end(point) \
    { \
      if (PPROF_LEVEL >= PPROF_POINT_LEVEL_##point) { \
        uint32_t pp_delta_##point = pprof_get_one() - pp_start_##point; \
        if (--pprof_refcounts[pp_##point] == 0) { \
          pprof_counters.cycles[pp_##point] += pp_delta_##point; \
          pprof_counters.calls[pp_##point]++; \
        } \
      } \
    } \
  }

/* Exclude a nested invocation from the outer counter of the same type. */
#define pprof_end_sub(point) \
    { \
      if (PPROF_LEVEL >= PPROF_POINT_LEVEL_##point) { \
        uint32_t pp_delta_##point = pprof_get_one() - pp_start_##point; \
        if (--pprof_refcounts[pp_##point] != 0) \
          pprof_counters.cycles[pp_##point] -= pp_delta_##point; \
      } \
    } \
  }

void pprof_init(void);
void pprof_reset(void);
void pprof_finish(void);

#endif
