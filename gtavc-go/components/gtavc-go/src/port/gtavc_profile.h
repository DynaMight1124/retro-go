#ifndef GTAVC_PROFILE_H
#define GTAVC_PROFILE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rg_system.h>
#include "port_defaults.h"

/* Opt-in DEBUG measurement only. Release always disables all profiling. */
#ifndef GTAVC_PROFILE_ENABLED
#define GTAVC_PROFILE_ENABLED 0
#endif
#ifndef GTAVC_DETAILED_PROFILE
#define GTAVC_DETAILED_PROFILE 0
#endif
#if RG_BUILD_RELEASE
#undef GTAVC_PROFILE_ENABLED
#define GTAVC_PROFILE_ENABLED 0
#endif
#if !GTAVC_PROFILE_ENABLED
#undef GTAVC_DETAILED_PROFILE
#define GTAVC_DETAILED_PROFILE 0
#endif
#define GTAVC_PROF(statement) do { if (GTAVC_PROFILE_ENABLED) { statement; } } while (0)
#define GTAVC_PROFILE_TIMER() (GTAVC_PROFILE_ENABLED ? rg_system_timer() : 0)
#define GTAVC_DETAIL_LOG(...) do { if (GTAVC_DETAILED_PROFILE) RG_LOGD(__VA_ARGS__); } while (0)

typedef enum {
    PROF_STREAM, PROF_POOLS, PROF_MISSION, PROF_RADIO, PROF_PLAYER,
    PROF_SAVE, PROF_ACTORS, PROF_CAMERA, PROF_WANTED, PROF_SOUND,
    PROF_TILE_OPEN, PROF_TILE_ALLOC, PROF_TILE_READ, PROF_TILE_PARSE,
    PROF_TILE_INSTALL, PROF_TILE_FALLBACK, PROF_AUDIO_WAIT, PROF_MODEL_LOAD, PROF_STAGE_COUNT
} gtavc_prof_stage_t;

typedef struct {
    uint64_t stage_us[PROF_STAGE_COUNT];
    uint32_t stage_peak_us[PROF_STAGE_COUNT];
    uint32_t stage_calls[PROF_STAGE_COUNT];
    uint32_t tile_open_failed, tile_oom, tile_parse_failed, tile_installed;
    uint32_t tile_deferred, tile_read_failed, tile_read_bytes;
    uint32_t worker_jobs, worker_busy_ticks, worker_wait_ticks, worker_cancelled;
    uint64_t worker_open_us, worker_read_us, worker_decode_us;
    uint32_t count;
    int draw_setting, draw_far_z;
    int world_width, world_height;
    uint32_t compose_frames, compose_blocks;
    uint64_t t_wall_us;
    uint64_t t_transform_us, t_setup_us;
    uint64_t t_flat_us, t_tex_us, t_tint_us, t_clip_us;
    uint64_t t_sky_us, t_post_us, t_depth_clear_us, t_colour_clear_us;
    uint32_t vertices, meshes, tri_candidates, clipped;
    uint64_t t_update_us;
    uint64_t t_clear_us;
    uint64_t t_tiles_us;
    uint64_t t_ents_us;
    uint64_t t_render_us;
    uint64_t t_upscale_us;
    uint64_t t_overlay_us;
    uint64_t t_blit_us;
    uint64_t t_disp_wait_us;
    uint64_t t_total_us;
    uint32_t tris;
    uint32_t skipped_frames;
    uint32_t update_peak_us, work_peak_us;
} gtavc_prof_t;

extern gtavc_prof_t g_prof;
extern rg_task_t *gtavc_profile_game_task;

static inline void gtavc_prof_stage(gtavc_prof_stage_t stage, int64_t start) {
#if GTAVC_DETAILED_PROFILE
    uint32_t us = (uint32_t)(rg_system_timer() - start);
    g_prof.stage_us[stage] += us;
    g_prof.stage_calls[stage]++;
    if (us > g_prof.stage_peak_us[stage]) g_prof.stage_peak_us[stage] = us;
#else
    (void)stage; (void)start;
#endif
}

/* Coarse scopes only: never wrap individual samples or pixels. */
#if GTAVC_DETAILED_PROFILE
#define GTAVC_TIME(stage, statement) do { \
    int64_t gtavc_scope_start = rg_system_timer(); \
    statement; \
    gtavc_prof_stage(stage, gtavc_scope_start); \
} while (0)
#else
#define GTAVC_TIME(stage, statement) do { statement; } while (0)
#endif

static inline void gtavc_prof_report_if_needed(void) {
#if GTAVC_PROFILE_ENABLED
    if (g_prof.t_wall_us < 5000000 || !g_prof.count) return;
    double c = g_prof.count;
    double drawn = c > g_prof.skipped_frames ? c - g_prof.skipped_frames : 0;
    double ms = drawn ? 1.0 / drawn / 1000.0 : 0;
    RG_LOGD("[PROF] world=%dx%d tick_fps=%.1f submit_fps=%.1f work/tick=%.1fms render/draw=%.1fms skips=%u",
            g_prof.world_width, g_prof.world_height,
            c * 1000000.0 / g_prof.t_wall_us, drawn * 1000000.0 / g_prof.t_wall_us,
            g_prof.t_total_us / c / 1000.0, g_prof.t_render_us * ms, (unsigned)g_prof.skipped_frames);
    uint64_t buffer_clear = g_prof.t_depth_clear_us + g_prof.t_colour_clear_us;
    double setup_ms = g_prof.t_clear_us > buffer_clear ? (g_prof.t_clear_us - buffer_clear) * ms : 0;
    RG_LOGD("[PROF] ms update/tick=%.1f; per_draw clear=%.1f(setup=%.1f depth=%.1f colour=%.1f) sky/post=%.1f tiles=%.1f entities=%.1f upscale=%.1f HUD=%.1f convert=%.1f wait=%.1f; io_errors=%u oom=%u",
            g_prof.t_update_us / c / 1000.0, g_prof.t_clear_us * ms,
            setup_ms, g_prof.t_depth_clear_us * ms, g_prof.t_colour_clear_us * ms,
            (g_prof.t_sky_us + g_prof.t_post_us) * ms,
            g_prof.t_tiles_us * ms, g_prof.t_ents_us * ms,
            g_prof.t_upscale_us * ms, g_prof.t_overlay_us * ms,
            g_prof.t_blit_us * ms, g_prof.t_disp_wait_us * ms,
            (unsigned)(g_prof.tile_open_failed + g_prof.tile_read_failed + g_prof.tile_parse_failed),
            (unsigned)g_prof.tile_oom);
    memset(&g_prof, 0, sizeof(g_prof));
#endif
}

#endif /* GTAVC_PROFILE_H */
