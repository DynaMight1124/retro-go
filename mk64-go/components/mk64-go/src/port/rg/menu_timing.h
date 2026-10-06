#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum { MK64_MENU_ROM, MK64_MENU_MIO0, MK64_MENU_TKMK,
       MK64_MENU_DSP_WAIT, MK64_MENU_PCM_WAIT, MK64_MENU_AUDIO, MK64_MENU_STAGES };
typedef struct { uint32_t calls, us, max_us, bytes; } Mk64MenuStage;
typedef struct {
    Mk64MenuStage stage[MK64_MENU_STAGES];
    uint32_t ticks, max_tick_us, min_queued_bytes;
    bool enabled;
} Mk64MenuTiming;
/* All writers run on the game thread; neither audio worker updates this. */
extern Mk64MenuTiming sMk64MenuTiming;
static inline void mk64_menu_timing_reset(void)
{
    memset(&sMk64MenuTiming, 0, sizeof(sMk64MenuTiming));
    sMk64MenuTiming.min_queued_bytes = UINT32_MAX;
}
static inline void mk64_menu_timing_add(unsigned stage, uint32_t us, uint32_t bytes)
{
    if (!sMk64MenuTiming.enabled) return;
    Mk64MenuStage *s = &sMk64MenuTiming.stage[stage];
    ++s->calls;
    s->us += us;
    s->bytes += bytes;
    if (us > s->max_us) s->max_us = us;
}
static inline void mk64_menu_timing_queue(uint32_t queued_bytes)
{
    if (sMk64MenuTiming.enabled && queued_bytes < sMk64MenuTiming.min_queued_bytes)
        sMk64MenuTiming.min_queued_bytes = queued_bytes;
}
static inline void mk64_menu_timing_tick(uint32_t us, uint32_t queued_bytes)
{
    ++sMk64MenuTiming.ticks;
    if (us > sMk64MenuTiming.max_tick_us) sMk64MenuTiming.max_tick_us = us;
    mk64_menu_timing_queue(queued_bytes);
}
