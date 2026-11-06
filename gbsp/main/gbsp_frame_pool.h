#pragma once
#include <stdbool.h>

typedef struct {
    int count, draw, pending, submitted, shown;
} gp_frame_pool_t;

static inline void gp_frame_pool_init(gp_frame_pool_t *p, int count)
{
    *p = (gp_frame_pool_t){count, 0, -1, -1, 0};
}

/* Caller has finished rendering. Older unsent frames may be dropped. */
static inline void gp_frame_pool_publish(gp_frame_pool_t *p)
{
    p->pending = p->draw;
    p->draw = -1;
}

/* Caller may submit only after the platform releases the preceding frame. */
static inline void gp_frame_pool_submit(gp_frame_pool_t *p)
{
    if (p->pending < 0) return;
    p->submitted = p->shown = p->pending;
    p->pending = -1;
}

static inline int gp_frame_pool_next(gp_frame_pool_t *p, bool busy)
{
    if (p->draw >= 0) return p->draw;
    for (int i = 0; i < p->count; i++) {
        if (i != p->pending && (i != p->submitted || (p->count == 1 && !busy)))
            return p->draw = i;
    }
    return -1;
}
