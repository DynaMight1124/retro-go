/*
 * QEMU Proxy for OPL2/3 emulation by MAME team
 *
 * Copyright (c) 2004-2005 Vassili Karpov (malc)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "adlib.h"
#include "t386_audio_rate.h"
#ifdef ESP_PLATFORM
#include "rg_utils.h"
#endif
#ifdef RETRO_GO
uint32_t get_uticks(void);
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//#define DEBUG

#define ADLIB_KILL_TIMERS 1

#define ADLIB_DESC "Yamaha YM3812 (OPL2)"

#ifdef DEBUG
#include "qemu/timer.h"
#endif

#ifdef DEBUG
#define ldebug(...) dolog (__VA_ARGS__)
#else
#define ldebug(...)
#endif

#ifdef USE_FMOPL
#include "fmopl.h"
#define SHIFT 1

struct AdlibState {
    uint32_t freq;
    uint32_t port;
    int ticking[2];
    uint32_t deadline[2];
    int enabled;
    int active;
    int16_t held_sample;
    int repeat_pending;
#ifdef DEBUG
    int64_t exp[2];
#endif
    void *voice;
    FM_OPL *opl;
};
void adlib_set_enabled(AdlibState *s,int enabled) { s->enabled=enabled!=0; }
int adlib_get_enabled(const AdlibState *s) { return s->enabled; }

static void adlib_stop_opl_timer (AdlibState *s, size_t n)
{
    OPLTimerOver (s->opl, n);
    s->ticking[n] = 0;
}

static void adlib_kill_timers (AdlibState *s)
{
    size_t i;

    for (i = 0; i < 2; ++i) {
        if (s->ticking[i]
#ifdef RETRO_GO
            && (int32_t)(get_uticks()-s->deadline[i])>=0
#endif
            ) {
#ifdef RETRO_GO
            /* OPLTimerOver rearms through timer_handler. Bound catch-up to one
             * overflow per service, retaining status without a long loop. */
            s->ticking[i]=0;
            OPLTimerOver(s->opl,i);
#else
            adlib_stop_opl_timer (s, i);
#endif
        }
    }
}

void adlib_write(void *opaque, uint32_t nport, uint32_t val)
{
    AdlibState *s = opaque;
    /* Speaker mode removes OPL from guest I/O, rather than merely muting it. */
    if (!s->enabled) return;
    int a = nport & 3;

    s->active = 1;
//    AUD_set_active_out (s->voice, 1);

    adlib_kill_timers (s);

    OPLWrite (s->opl, a, val);
}

uint32_t adlib_read(void *opaque, uint32_t nport)
{
    AdlibState *s = opaque;
    if (!s->enabled) return 0xff; /* Unconnected 8-bit ISA port. */
    int a = nport & 3;

    adlib_kill_timers (s);
    return OPLRead (s->opl, a);
}

static void timer_handler (void *opaque, int c, FLOAT interval_Sec)
{
    AdlibState *s = opaque;
    unsigned n = c & 1;
#ifdef DEBUG
    double interval;
    int64_t exp;
#endif

    if (interval_Sec == 0.0) {
        s->ticking[n] = 0;
        return;
    }

    s->ticking[n] = 1;
#ifdef RETRO_GO
    double us=(double)interval_Sec*1000000.0;
    uint32_t period=(uint32_t)us;
    if((double)period<us)period++;
    if(!period)period=1;
    s->deadline[n]=get_uticks()+period;
#endif
#ifdef DEBUG
    interval = NANOSECONDS_PER_SECOND * interval_Sec;
    exp = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + interval;
    s->exp[n] = exp;
#endif

//    AUD_init_time_stamp_out (s->voice, &s->ats);
}

void adlib_callback (void *opaque, uint8_t *stream, int free)
{
    AdlibState *s = opaque;
    int samples;

    samples = free >> SHIFT;
    adlib_kill_timers(s);
    if (!(s->active && s->enabled) || !samples) {
        return;
    }
#ifdef RETRO_GO
    /* FM -> output mixer. Preserve repeats across arbitrary chunks. */
    int16_t *dest=(int16_t*)stream;
    while(s->repeat_pending && samples) {
        *dest++=s->held_sample;s->repeat_pending--;samples--;
    }
    while(samples) {
        int count=MIN(samples,64);
        int needed=(count+T386_FM_REPEAT-1)/T386_FM_REPEAT;
        int16_t source[64/T386_FM_REPEAT];
        YM3812UpdateOne(s->opl,source,needed);
        int produced=0;
        for(int i=0;i<needed;i++) {
            int repeats=MIN((int)T386_FM_REPEAT,count-produced);
            for(int j=0;j<repeats;j++) {*dest++=source[i];produced++;}
            if(repeats<(int)T386_FM_REPEAT) {s->held_sample=source[i];s->repeat_pending=T386_FM_REPEAT-repeats;}
        }
        samples-=count;
    }
#else
    YM3812UpdateOne (s->opl, (void *) stream, samples);
#endif
}

void adlib_free(AdlibState *s)
{
    if (s->opl) {
        OPLDestroy (s->opl);
        s->opl = NULL;
    }

    s->active = 0;
    s->enabled = 0;
    free(s);
}

AdlibState *adlib_new()
{
#ifdef ESP_PLATFORM
    AdlibState *s = rg_alloc(sizeof(AdlibState),MEM_FAST);
#else
    AdlibState *s = malloc(sizeof(AdlibState));
#endif
    if(!s)return NULL;
    memset(s, 0, sizeof(AdlibState));
#ifdef RETRO_GO
    s->freq = T386_FM_SAMPLE_RATE;
#else
    s->freq = 44100;
#endif
    s->opl = OPLCreate (3579545, s->freq);
    if (!s->opl) {
        free(s);
        return NULL;
    }
    else {
        OPLSetTimerHandler(s->opl, timer_handler, s);
        s->enabled = 1;
    }
    return s;
}
#else
struct AdlibState {
};
void adlib_set_enabled(AdlibState *s,int enabled) { (void)s;(void)enabled; }
int adlib_get_enabled(const AdlibState *s) { (void)s;return 0; }

void adlib_write(void *opaque, uint32_t nport, uint32_t val)
{
}

uint32_t adlib_read(void *opaque, uint32_t nport)
{
    return -1;
}

void adlib_callback (void *opaque, uint8_t *stream, int free)
{
    return;
}

void adlib_free(AdlibState *s)
{
}

AdlibState *adlib_new()
{
    AdlibState *s = malloc(sizeof(AdlibState));
    memset(s, 0, sizeof(AdlibState));
    return s;
}
#endif
