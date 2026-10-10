#pragma once
#include "port_logic.h"
#include <limits.h>

typedef struct {
    uint32_t last_ms;
    double fraction_ms;
    int64_t busy_us;
} q2_timing_t;

static inline void q2_timing_reset(q2_timing_t *timing, uint32_t now_ms)
{
    timing->last_ms = now_ms;
    timing->fraction_ms = 0;
    timing->busy_us = 0;
}

static inline int q2_timing_msec(q2_timing_t *timing, uint32_t now_ms, float speed)
{
    uint32_t elapsed = now_ms - timing->last_ms;
    timing->last_ms = now_ms;
    return q2_scale_msec(elapsed, speed, &timing->fraction_ms);
}

static inline void q2_timing_finish(q2_timing_t *timing, int64_t start_us,
                                   int64_t end_us, int display_wait_us, bool interrupted)
{
    if (interrupted) {
        // Load/UI wall time is neither engine CPU work nor the next simulation delta.
        q2_timing_reset(timing, (uint32_t)(end_us / 1000));
        return;
    }
    int64_t busy_us = end_us - start_us - display_wait_us;
    if (busy_us > 0) timing->busy_us += busy_us;
}

static inline int q2_timing_take_busy(q2_timing_t *timing)
{
    int result = timing->busy_us > INT_MAX ? INT_MAX : (int)timing->busy_us;
    timing->busy_us = 0;
    return result;
}
