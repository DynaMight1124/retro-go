#include "t386_audio_output.h"
#include <string.h>

static bool paused;
static unsigned fade_length,fade_remaining;
static rg_audio_frame_t last;

static void start_fade(void) {
    int rate=rg_audio_get_sample_rate();
    fade_length=rate>200?(unsigned)rate/200:1; /* 5ms, independent of chunk size. */
    fade_remaining=fade_length;
}
void t386_audio_output_prime(void) {
    static const rg_audio_frame_t silence[64];
    int rate=rg_audio_get_sample_rate();
    /* Queue 50ms of signed silence through Retro-Go, which performs the correct
     * conversion for each sink. Never submit raw internal-DAC midpoint words. */
    size_t remaining=rate>0?((size_t)rate+19)/20:64;
    while(remaining) {
        size_t count=remaining<64?remaining:64;
        rg_audio_submit(silence,count);
        remaining-=count;
    }
}
void t386_audio_output_init(void) {
    paused=false;last=(rg_audio_frame_t){0};
    start_fade();t386_audio_output_prime();
}
bool t386_audio_output_is_paused(void) {return paused;}
void t386_audio_output_pause(bool value) {
    if(paused==value)return;
    if(value) {
        /* Join the end of the submitted stream to zero before parking it. */
        rg_audio_frame_t ramp[64];
        for(unsigned i=0;i<64;i++) {
            ramp[i].left=(int32_t)last.left*(int)(63-i)/64;
            ramp[i].right=(int32_t)last.right*(int)(63-i)/64;
        }
        rg_audio_submit(ramp,64);
        last=(rg_audio_frame_t){0};
        t386_audio_output_prime();
    } else start_fade();
    paused=value;
}
void t386_audio_output_prepare(rg_audio_frame_t *frames,size_t count) {
    if(!count)return;
    if(paused)memset(frames,0,count*sizeof(*frames));
    else if(fade_remaining) {
        for(size_t i=0;i<count&&fade_remaining;i++) {
            unsigned gain=fade_length-fade_remaining+1;
            frames[i].left=(int32_t)frames[i].left*(int)gain/(int)fade_length;
            frames[i].right=(int32_t)frames[i].right*(int)gain/(int)fade_length;
            fade_remaining--;
        }
    }
    last=frames[count-1];
}
