#ifndef GTAVC_TIMING_H
#define GTAVC_TIMING_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {int64_t ready_us;bool playing,enabled;} AutoFrameskip;
#define TIMING_AUTO_KEEP (-2)
static inline void Timing_suspendAuto(AutoFrameskip*t){t->playing=t->enabled=false;t->ready_us=0;}
/* Shared monitor samples once a second. Three seconds of uninterrupted play
   flushes paused/loading statistics before enabling it. No load-based tuning
   here: KEEP hands all subsequent decisions to the shared controller. */
static inline int Timing_autoPolicy(AutoFrameskip*t,int mode,bool playing,int64_t now){
    if(mode!=1 || !playing){Timing_suspendAuto(t);return -1;}
    if(!t->playing){t->playing=true;t->ready_us=now+3000000;}
    if(now<t->ready_us)return -1;
    if(!t->enabled){t->enabled=true;return 1;}
    return TIMING_AUTO_KEEP;
}
typedef struct {int64_t next_us;unsigned phase;} Timing;
static inline void Timing_reset(Timing*t,int64_t now){t->next_us=now;t->phase=0;}
/* Eight ticks bound catch-up after exceptional stalls. Menu/loading resets
   discard paused time explicitly instead of fast-forwarding the game. */
static inline int Timing_due(Timing*t,int64_t now,int period){
    if(period<1)period=33333;
    if(now<t->next_us)return 0;
    int64_t count=(now-t->next_us)/period+1;
    if(count>8){t->next_us=now+period;return 8;}
    t->next_us+=count*period;return (int)count;
}
static inline int Timing_stepMs(Timing*t){t->phase+=1000;int dt=t->phase/30;t->phase%=30;return dt;}
static inline int Timing_skip(int mode,int shared){
    if(mode==1)return shared<0?0:shared>5?5:shared;
    return mode==2?1:mode==3?2:0;
}
#endif
