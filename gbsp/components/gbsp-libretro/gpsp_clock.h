#pragma once
#include <stdint.h>
#include <stdbool.h>

#define GPSP_CLOCK_MAX_TIME INT64_C(253402300799) /* end of year 9999 */
#define GPSP_CLOCK_OFFLINE_TIME INT64_C(946728000) /* 2000-01-01 noon UTC: no pre-2000 date at UTC-12 */

static inline bool gpsp_clock_time_valid(int64_t seconds)
{
    return seconds > 0 && seconds <= GPSP_CLOCK_MAX_TIME;
}

/* Monotonic elapsed time keeps gameplay speed and wall-clock corrections from
 * slowing or rewinding a cartridge clock. The host supplies both time sources. */
typedef struct {
    int64_t epoch, origin_us, last;
} gpsp_clock_t;

static inline void gpsp_clock_start(gpsp_clock_t *c, int64_t wall, int64_t saved, int64_t us)
{
    if (!gpsp_clock_time_valid(saved)) saved = 0;
    if (!gpsp_clock_time_valid(wall)) wall = saved ? saved : GPSP_CLOCK_OFFLINE_TIME;
    c->epoch = wall > saved ? wall : saved;
    c->last = c->epoch;
    c->origin_us = us;
}

static inline int64_t gpsp_clock_read(gpsp_clock_t *c, int64_t wall, int64_t us)
{
    int64_t seconds = us >= c->origin_us ? (us - c->origin_us) / 1000000 : 0;
    int64_t now = seconds > GPSP_CLOCK_MAX_TIME - c->epoch ? GPSP_CLOCK_MAX_TIME : c->epoch + seconds;
    if (gpsp_clock_time_valid(wall) && wall > now) {
        now = wall;
        c->epoch = now;
        c->origin_us = us;
    }
    if (now < c->last) now = c->last;
    c->last = now;
    return now;
}

/* Explicit little-endian, versioned metadata; never write native time_t. */
static inline void gpsp_clock_encode(unsigned char out[16], int64_t seconds)
{
    out[0]='G'; out[1]='R'; out[2]='T'; out[3]='C';
    out[4]=1; out[5]=out[6]=out[7]=0;
    for (int i=0; i<8; i++) out[8+i]=(unsigned char)((uint64_t)seconds >> (i*8));
}

/* A failed decode never changes the caller's timestamp. */
static inline const char *gpsp_clock_decode_error(const unsigned char *data, unsigned len, int64_t *seconds)
{
    if (len != 16) return "wrong size";
    if (data[0]!='G' || data[1]!='R' || data[2]!='T' || data[3]!='C') return "unrecognized format";
    if (data[4]!=1) return "unsupported version";
    if (data[5] || data[6] || data[7]) return "invalid reserved bytes";
    uint64_t value=0;
    for (int i=0; i<8; i++) value |= (uint64_t)data[8+i] << (i*8);
    if (value > (uint64_t)GPSP_CLOCK_MAX_TIME) return "timestamp beyond year 9999";
    *seconds=(int64_t)value;
    return 0;
}

static inline bool gpsp_clock_decode(const unsigned char *data, unsigned len, int64_t *seconds)
{
    return gpsp_clock_decode_error(data, len, seconds) == 0;
}
