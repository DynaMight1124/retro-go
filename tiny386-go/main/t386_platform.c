#include "rg_system.h"
#include "t386_platform.h"
#include "t386_clock.h"
#include "t386_file.h"
#include "t386_firmware.h"
#include <limits.h>
#include <string.h>
static unsigned char *arena;
static size_t arena_used,arena_size,ram_size;
static time_t epoch;
uint64_t t386_host_time_us(void) {return (uint64_t)rg_system_timer();}
uint64_t t386_guest_timer_us(void) {return t386_clock_now_us();}
time_t t386_guest_time(void) {return epoch+(time_t)(t386_clock_now_us()/1000000);}
uint32_t get_uticks(void) {return (uint32_t)t386_clock_now_us();}
void t386_platform_init(long size) {ram_size=size; epoch=time(NULL); t386_clock_init(); t386_clock_pause();}
void *bigmalloc(size_t size) {return rg_alloc(size,MEM_SLOW);}
void pcmalloc_init(void *ptr,long len) {arena=ptr; arena_size=len; arena_used=0;}
void *pcmalloc(long size) {
    RG_ASSERT(size>0 && (size_t)size<=SIZE_MAX-31,"Invalid core allocation");
    size_t aligned=((size_t)size+31)&~(size_t)31;
    RG_ASSERT(aligned<=arena_size-arena_used,"Tiny386 device arena exhausted");
    void *p=arena+arena_used; arena_used+=aligned; memset(p,0,aligned); return p;
}
int load_rom(void *mem,const char *file,uword addr,int backward) {
    int len;
    if(!t386_firmware_load(mem,ram_size,file,addr,backward,&len)) {
        RG_LOGE("Cannot load BIOS safely: %s",file);
        RG_PANIC("Invalid BIOS size, address, or read failure");
    }
    RG_LOGI("BIOS %s: %d bytes loaded into guest RAM",file,len);
    return len;
}
