#pragma once
#include <stdbool.h>
#include <stdint.h>

/* deadline is the start time of the next real game tick, not a render timer. */
typedef struct {
    int64_t deadline;
    int period;
    unsigned skip_remaining;
    unsigned consecutive_skips;
    bool initialized;
} mk64_pacer_t;

static inline bool mk64_pacer_begin(mk64_pacer_t *p, int64_t now, int period, int frameskip)
{
    if (period <= 0) period = 1000000 / 30;
    if (!p->initialized || p->period != period || now - p->deadline > 250000) {
        p->deadline = now;
        p->period = period;
        p->skip_remaining = 0;
        p->consecutive_skips = 0;
        p->initialized = true;
    }
    bool draw;
    if (frameskip < 0 || p->consecutive_skips >= 5) {
        /* Audio backpressure may leave no catch-up headroom even on skipped
         * ticks. Bound display starvation rather than wait for debt forever. */
        p->skip_remaining = 0;
        draw = true;
    } else if (p->skip_remaining) {
        --p->skip_remaining;
        draw = false;
    } else {
        /* Instantaneous deadline recovery supplements the platform skip count. */
        draw = now <= p->deadline + 1500;
    }
    p->consecutive_skips = draw ? 0 : p->consecutive_skips + 1;
    return draw;
}

static inline int64_t mk64_pacer_finish(mk64_pacer_t *p, int64_t now, bool rendered, int frameskip)
{
    /* The platform skip count is a recovery budget when a draw overruns.
     * Cheap frames (especially menus) can still all be displayed at 30Hz. */
    if (rendered)
        p->skip_remaining = frameskip > 0 && now > p->deadline + p->period
                            ? (unsigned)frameskip : 0;
    p->deadline += p->period;
    /* Do not replay time spent loading assets or stalled on external work. */
    if (now - p->deadline > 250000) {
        p->deadline = now + p->period;
        p->skip_remaining = 0;
        p->consecutive_skips = 0;
    }
    return p->deadline > now ? p->deadline - now : 0;
}
