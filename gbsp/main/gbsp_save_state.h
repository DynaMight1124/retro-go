#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GBSP_SAVE_DEFER_US INT64_C(15000000)
#define GBSP_SAVE_RTC_INTERVAL_US INT64_C(60000000)

/* Only the emulation thread touches this state. Worker completion arrives via
 * a queue: the snapshot cannot be reused until that acknowledgement is read. */
typedef struct {
    bool dirty, inflight, flight_data;
    size_t capacity; /* Largest cartridge capacity observed this session. */
    int64_t last_dirty, last_attempt, last_rtc;
    int64_t first_dirty, flight_since;
} gbsp_save_state_t;

static inline void gbsp_save_touch(gbsp_save_state_t *s, size_t capacity, int64_t now)
{
    if (capacity > s->capacity) s->capacity=capacity;
    if (!s->dirty) s->first_dirty=now;
    s->dirty=true;
    s->last_dirty=now;
}

static inline bool gbsp_save_begin(gbsp_save_state_t *s, int64_t now,
                                  bool force, bool quiet, bool rtc_due)
{
    if (s->inflight || (!s->dirty && !rtc_due)) return false;
    /* Bus activity and continuous SRAM writes may defer work, not starve it.
     * Still respect retry spacing and never overwrite an in-flight snapshot. */
    bool overdue = (s->dirty && now-s->first_dirty >= GBSP_SAVE_DEFER_US) ||
        (rtc_due && now-s->last_rtc >= GBSP_SAVE_RTC_INTERVAL_US+GBSP_SAVE_DEFER_US);
    if (!force && (now-s->last_attempt < 5000000 ||
                   (!overdue && (!quiet || (s->dirty && now-s->last_dirty < 2000000))))) return false;
    s->flight_data=s->dirty;
    s->flight_since=s->first_dirty;
    s->dirty=false;
    s->inflight=true;
    s->last_attempt=now;
    return true;
}

static inline void gbsp_save_complete(gbsp_save_state_t *s, bool ok, int64_t now)
{
    if (!ok && s->flight_data) {
        if (!s->dirty || s->flight_since < s->first_dirty)
            s->first_dirty=s->flight_since;
        s->dirty=true;
    }
    if (ok) s->last_rtc=now;
    s->inflight=false;
}
