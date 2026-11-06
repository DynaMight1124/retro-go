#pragma once
#include "gpsp_log.h"
#ifndef GP_PERF_WORKERS
#define GP_PERF_WORKERS() ((void)0)
#endif

#if !RG_BUILD_RELEASE
typedef struct {
    int64_t since, mark;
    uint32_t frames, drawn;
    uint64_t exec, wait, display, audio;
    uint64_t drawn_exec, skipped_exec;
    uint32_t frame_exec, max_exec;
    int policy, cap;
} gbsp_perf_t;
static gbsp_perf_t gp_perf;

static void gp_perf_begin(void)
{
    gp_perf.mark = rg_system_timer();
    gp_perf.frame_exec = 0;
    if (!gp_perf.since) gp_perf.since = gp_perf.mark;
}
static void gp_perf_stage(uint64_t *total)
{
    int64_t now = rg_system_timer();
    uint32_t elapsed = (uint32_t)(now - gp_perf.mark);
    *total += elapsed;
    if (total == &gp_perf.exec) gp_perf.frame_exec = elapsed;
    gp_perf.mark = now;
}
static void gp_perf_end(bool drawn, int skip)
{
    (void)skip; /* The policy/cap describe skip behavior; countdown is transient. */
    gp_perf.frames++;
    gp_perf.drawn += drawn;
    if (drawn) gp_perf.drawn_exec += gp_perf.frame_exec;
    else gp_perf.skipped_exec += gp_perf.frame_exec;
    if (gp_perf.frame_exec > gp_perf.max_exec) gp_perf.max_exec = gp_perf.frame_exec;
    int64_t now = rg_system_timer();
    if (now - gp_perf.since < 5000000) return;
    uint32_t n = gp_perf.frames;
    uint32_t wait = gp_perf.wait / n;
    uint32_t display = gp_perf.display / n, audio = gp_perf.audio / n;
    uint32_t skipped = n - gp_perf.drawn;
    uint32_t drawn_exec = gp_perf.drawn ? gp_perf.drawn_exec / gp_perf.drawn : 0;
    uint32_t skipped_exec = skipped ? gp_perf.skipped_exec / skipped : 0;
    uint32_t emu_rate = (uint64_t)n * 100000000u / (uint64_t)(now - gp_perf.since);
    uint32_t draw_rate = (uint64_t)gp_perf.drawn * 100000000u / (uint64_t)(now - gp_perf.since);
    /* D/S are averages for drawn/skipped frames (zero if none in the window). */
    RG_LOGD("perf: emu %u.%02u fps, draw %u.%02u fps, exec D/S/max %u.%02u/%u.%02u/%u.%02u ms, "
            "wait %u.%02u ms, disp %u.%02u ms, audio %u.%02u ms, policy %d cap %d",
            (unsigned)(emu_rate / 100), (unsigned)(emu_rate % 100),
            (unsigned)(draw_rate / 100), (unsigned)(draw_rate % 100),
            (unsigned)(drawn_exec / 1000), (unsigned)((drawn_exec % 1000) / 10),
            (unsigned)(skipped_exec / 1000), (unsigned)((skipped_exec % 1000) / 10),
            (unsigned)(gp_perf.max_exec / 1000), (unsigned)((gp_perf.max_exec % 1000) / 10),
            (unsigned)(wait / 1000), (unsigned)((wait % 1000) / 10),
            (unsigned)(display / 1000), (unsigned)((display % 1000) / 10),
            (unsigned)(audio / 1000), (unsigned)((audio % 1000) / 10),
            gp_perf.policy, gp_perf.cap);
    GP_PERF_WORKERS();
    gp_perf = (gbsp_perf_t){.since=now};
}
#define GP_PERF_BEGIN() gp_perf_begin()
#define GP_PERF_STAGE(name) gp_perf_stage(&gp_perf.name)
#define GP_PERF_END(drawn, skip) gp_perf_end(drawn, skip)
#define GP_PERF_POLICY(level, limit) do { gp_perf.policy = (level); gp_perf.cap = (limit); } while (0)
#define GP_PERF_RESET() do { gp_perf = (gbsp_perf_t){0}; } while (0)
#else
#define GP_PERF_BEGIN() ((void)0)
#define GP_PERF_STAGE(name) ((void)0)
#define GP_PERF_END(drawn, skip) ((void)0)
#define GP_PERF_POLICY(level, limit) ((void)0)
#define GP_PERF_RESET() ((void)0)
#endif
