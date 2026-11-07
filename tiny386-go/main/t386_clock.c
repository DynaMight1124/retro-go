#include "t386_clock.h"
#include <stdbool.h>
#include <math.h>
static uint64_t base_host, base_guest;
static double scale=1;
static bool paused;
void t386_clock_init(void) {base_host=t386_host_time_us(); base_guest=0; scale=1; paused=false;}
uint64_t t386_clock_now_us(void) {
    return base_guest+(paused?0:(uint64_t)((t386_host_time_us()-base_host)*scale));
}
void t386_clock_pause(void) {if(!paused) {base_guest=t386_clock_now_us(); paused=true;}}
void t386_clock_resume(void) {if(paused) {base_host=t386_host_time_us(); paused=false;}}
void t386_clock_set_speed(float speed) {
    if(!isfinite(speed)||speed<=0) return;
    base_guest=t386_clock_now_us(); base_host=t386_host_time_us(); scale=speed;
}
