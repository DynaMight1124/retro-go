#include "t386_runtime.h"
#include "t386_profile_config.h"
#include "t386_audio_rate.h"
#include <string.h>
#if T386_ENABLE_PROFILING
static T386PresentCopyProfile copy_profile;
#ifdef ESP_PLATFORM
extern uint64_t t386_host_time_us(void);
#endif
void t386_present_copy_profile_reset(void) {memset(&copy_profile,0,sizeof(copy_profile));}
const T386PresentCopyProfile *t386_present_copy_profile_get(void) {return &copy_profile;}
#endif
static unsigned frame_size(const T386Schedule *s) {return (T386_AUDIO_SAMPLE_RATE+s->frame_remainder)/60;}
unsigned t386_schedule_chunk(const T386Schedule *s) {unsigned left=frame_size(s)-s->frame_samples; return left<64?left:64;}
bool t386_schedule_advance(T386Schedule *s,unsigned count) {
    s->samples+=count; s->frame_samples+=count;
    if(s->frame_samples==frame_size(s)) {
        s->frame_samples=0;s->frame_remainder=(T386_AUDIO_SAMPLE_RATE+s->frame_remainder)%60;
        return true;
    } return false;
}
uint64_t t386_schedule_deadline(const T386Schedule *s) {
    return s->epoch + s->samples/T386_AUDIO_SAMPLE_RATE*1000000 + s->samples%T386_AUDIO_SAMPLE_RATE*1000000/T386_AUDIO_SAMPLE_RATE;
}
uint64_t t386_schedule_target(T386Schedule *s,uint64_t now) {
    uint64_t current=t386_schedule_deadline(s);
    uint64_t next_samples=s->samples+t386_schedule_chunk(s);
    uint64_t target=s->epoch+next_samples/T386_AUDIO_SAMPLE_RATE*1000000+next_samples%T386_AUDIO_SAMPLE_RATE*1000000/T386_AUDIO_SAMPLE_RATE;
    if(now>target && now-target>20000) {
        /* Recover short stalls by servicing overdue chunks promptly. Bound
         * catch-up after a long stall without dropping guest time or samples. */
        s->epoch+=now-current;
        target+=now-current;
    }
    return target;
}
unsigned t386_cpu_budget(uint64_t now,uint64_t deadline,unsigned estimate) {
    if(now>=deadline)return 32;
    if(!estimate)estimate=512;
    uint64_t remaining=deadline-now;
    if(remaining>=(uint64_t)estimate*4/3+1)return 512;
    uint64_t budget=remaining*384/estimate; /* Leave 25% estimated headroom. */
    if(budget<32)return 32;
    if(budget>512)return 512;
    return (unsigned)budget&~31u;
}
unsigned t386_cpu_observe(unsigned estimate,uint64_t elapsed,unsigned budget) {
    if(!budget || !elapsed)return estimate;
    /* Limit outlier influence from a long REP instruction or preemption. */
    if(elapsed>1000000)elapsed=1000000;
    uint64_t sample=elapsed*512/budget;
    if(sample>1000000)sample=1000000;
    unsigned result=(unsigned)(((uint64_t)estimate*7+sample)/8);
    return result?result:1;
}
void t386_video_set_limit(T386Video *v,unsigned fps) {
    v->limit_fps=fps==20||fps==15||fps==10?fps:0;
    v->next_us=0; /* Start the chosen policy on the first post-menu render. */
}
bool t386_video_due(const T386Video *v,uint64_t now,unsigned late,bool busy) {
    return now>=v->next_us && late<=2000 && !busy;
}
void t386_video_record(T386Video *v,uint64_t started,uint64_t ended) {
    uint64_t cost=ended>started?ended-started:0;
    v->cost_us=v->cost_us?(v->cost_us*7+cost)/8:cost;
    uint64_t budget_cost=cost>v->cost_us?cost:v->cost_us;
    uint64_t interval=budget_cost*4; /* At most 25% foreground wall time. */
    if(interval<33333)interval=33333; /* At most approximately 30 Hz. */
    if(v->limit_fps) {
        /* Preserve the cost budget; a user ceiling can only reduce rendering. */
        uint64_t minimum=1000000u/v->limit_fps;
        if(interval<minimum)interval=minimum;
    }
    v->next_us=started+interval;
}
bool t386_present_copy(const void *work,void *present,size_t size,bool busy) {
    if (busy) return false;
    memcpy(present,work,size);
    return true;
}
static bool present_copy_rows(const void *source,size_t source_stride,void *dest,
                              size_t dest_stride,size_t row_bytes,size_t rows,bool busy) {
    if(busy || !row_bytes || !rows || row_bytes>source_stride || row_bytes>dest_stride) return false;
    if(row_bytes==source_stride && row_bytes==dest_stride) {
        memcpy(dest,source,row_bytes*rows);
        return true;
    }
    const unsigned char *src=source;
    unsigned char *dst=dest;
    for(size_t y=0;y<rows;y++) memcpy(dst+y*dest_stride,src+y*source_stride,row_bytes);
    return true;
}
bool t386_present_copy_rows(const void *source,size_t source_stride,void *dest,
                           size_t dest_stride,size_t row_bytes,size_t rows,bool busy) {
#if defined(ESP_PLATFORM) && T386_ENABLE_PROFILING
    uint64_t started=t386_host_time_us();
#endif
    bool copied=present_copy_rows(source,source_stride,dest,dest_stride,row_bytes,rows,busy);
#if defined(ESP_PLATFORM) && T386_ENABLE_PROFILING
    uint64_t elapsed=t386_host_time_us()-started;
    copy_profile.us+=elapsed;copy_profile.calls++;
    if(copied)copy_profile.bytes+=(uint64_t)row_bytes*rows;
    else copy_profile.rejected++;
    if(elapsed>copy_profile.max_us)copy_profile.max_us=elapsed>UINT32_MAX?UINT32_MAX:(unsigned)elapsed;
#endif
    return copied;
}

bool t386_present_copy_indexed(const void *source,size_t source_stride,void *dest,
                              size_t dest_stride,size_t row_bytes,size_t rows,
                              const uint16_t *palette,uint16_t *dest_palette,bool busy) {
    if(!source || !dest || !palette || !dest_palette || palette==dest_palette)return false;
    if(!t386_present_copy_rows(source,source_stride,dest,dest_stride,row_bytes,rows,busy))return false;
    memcpy(dest_palette,palette,256*sizeof(*palette));
    return true;
}
bool t386_shared_video_due(const T386SharedVideo *v,bool busy) {
    return !v->skip && !busy;
}
void t386_shared_video_record(T386SharedVideo *v,int frameskip,uint64_t busy_us,
                             unsigned frame_us,bool display_busy) {
    if(v->skip) {v->skip--;return;}
    if(frameskip>0) v->skip=(unsigned)frameskip;
    else if(busy_us>(uint64_t)frame_us+1500 || display_busy) v->skip=1;
}
