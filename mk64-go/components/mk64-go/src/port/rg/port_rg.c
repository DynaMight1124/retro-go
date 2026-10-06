/* Retro-Go host hooks for the MK64 single-threaded game loop. */
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <ultra64.h>
#include <rg_system.h>
#include "port.h"
#include "main.h"
#include "gfx_rg.h"
#include "phase_profile.h"
#include "diagnostic.h"
#include "menu_timing.h"

Mk64MenuTiming sMk64MenuTiming;

extern Gfx *gDisplayListHead;
extern struct GfxPool *gGfxPool;
extern uintptr_t gSegmentTable[16];
static unsigned sFrame;
static unsigned sRenderFrames;
static bool sRenderEnabled = true;

void mk64_rg_set_render_enabled(bool enabled)
{
    sRenderEnabled = enabled;
}
u32 port_time_us(void);
#if MK64_RG_PHASE_PROFILE
static u32 sLogTime;
static u32 sPhaseSum[RG_PHASE_COUNT], sPhaseCurrent[RG_PHASE_COUNT];
static u32 sPhaseLast;
static unsigned sPhaseFrames, sPhaseSamples;
static int sPhaseActive, sPhaseQuiet, sPhaseCourse = -1;

/* Subtract synchronous PORT_LOG time from game phases. Renderer RG_LOG calls
 * are avoided by sampling only frames without the periodic renderer report. */
static u32 profile_clock(void)
{
    return port_time_us() - sLogTime;
}

void port_rg_profile_begin(int racing, int course)
{
    unsigned next = sFrame + 1;
    if (!racing || course != sPhaseCourse) {
        memset(sPhaseSum, 0, sizeof(sPhaseSum));
        sPhaseFrames = sPhaseSamples = 0;
    }
    sPhaseCourse = course;
    sPhaseActive = racing;
    /* Retain the previous quiet sample set for comparable CPU averages. */
    sPhaseQuiet = next > 4 && next != 24 && next % 30 != 0 &&
                  !(sRenderEnabled && (sRenderFrames + 1) % MK64_RG_REPORT_INTERVAL == 0);
    memset(sPhaseCurrent, 0, sizeof(sPhaseCurrent));
    sPhaseLast = profile_clock();
}

void port_rg_profile_mark(int phase)
{
    if (!sPhaseActive) return;
    u32 now = profile_clock();
    sPhaseCurrent[phase] += now - sPhaseLast;
    sPhaseLast = now;
}

void port_rg_profile_finish(void)
{
    if (!sPhaseActive) return;
    if (sPhaseQuiet) {
        for (int i = 0; i < RG_PHASE_COUNT; ++i)
            sPhaseSum[i] += sPhaseCurrent[i];
        ++sPhaseSamples;
    }
    if (++sPhaseFrames < 60) return;
    if (sPhaseSamples) {
        PORT_LOG("CPU frame %u course %d: %u quiet samples, avg us: sim %u kart %u objects %u list %u other %u draw %u\n",
                 sFrame, sPhaseCourse, sPhaseSamples,
                 sPhaseSum[RG_PHASE_SIM] / sPhaseSamples,
                 sPhaseSum[RG_PHASE_KART] / sPhaseSamples,
                 sPhaseSum[RG_PHASE_OBJECTS] / sPhaseSamples,
                 sPhaseSum[RG_PHASE_LIST] / sPhaseSamples,
                 sPhaseSum[RG_PHASE_OTHER] / sPhaseSamples,
                 sPhaseSum[RG_PHASE_DRAW] / sPhaseSamples);
    }
    memset(sPhaseSum, 0, sizeof(sPhaseSum));
    sPhaseFrames = sPhaseSamples = 0;
}
#endif

float gPortDrawDist = PORT_DRAW_DIST;
int gfx_trace_frames;
s32 gPortLogDefer;

u32 port_time_us(void)
{
    return (u32)rg_system_timer();
}

void port_log(const char *format, ...)
{
#if RG_BUILD_RELEASE
    (void)format;
#else
    /* PSP reports every slow race iteration. Keep a useful warning without
     * flooding the serial console while the software renderer is being tuned. */
    if (strncmp(format, "stall:", 6) == 0) {
        static u32 last_stall_us;
        static bool reported_stall;
        u32 now = port_time_us();
        if (reported_stall && now - last_stall_us < 3000000u) return;
        last_stall_us = now;
        reported_stall = true;
    }
    char line[256];
#if MK64_RG_PHASE_PROFILE
    u32 log_started = port_time_us();
#endif
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    do { RG_LOGD("MK64 game: %s", line); } while (0);
#if MK64_RG_PHASE_PROFILE
    sLogTime += port_time_us() - log_started;
#endif
#endif
}

void port_log_flush(void)
{
}

void port_gfx_start_frame(void)
{
    if (sRenderEnabled) {
        ++sRenderFrames;
        mk64_rg_gfx_begin();
    }
}

void port_gfx_run(Gfx *list)
{
    unsigned frame = ++sFrame;
    if (!sRenderEnabled) return;
    if (list != gGfxPool->gfxPool || gDisplayListHead < list ||
        gDisplayListHead > list + GFX_POOL_SIZE) {
        RG_LOGE("MK64 display list bounds invalid on frame %u", frame);
        return;
    }
    unsigned count = (unsigned)(gDisplayListHead - list);
    if (frame <= 4 || frame == 24 || frame % MK64_RG_REPORT_INTERVAL == 0)
        do { RG_LOGD("MK64 display list frame %u: %u top-level commands", frame, count); } while (0);
    if (frame == 2 || frame == 120)
        do { RG_LOGD("MK64 segment B base on frame %u: %p",
                frame, (void *)gSegmentTable[11]); } while (0);
    mk64_rg_gfx_walk(list, count, frame);
}

void port_gfx_end_frame(void)
{
    if (sRenderEnabled && !mk64_rg_gfx_present(sFrame))
        RG_LOGE("MK64 display surface unavailable on frame %u", sFrame);
}

void port_input_poll(void)
{
}

void port_fb_copy_request(s32 x, s32 y, s32 width, s32 height, u16 *target)
{
    mk64_rg_gfx_capture(x, y, width, height, target);
}
