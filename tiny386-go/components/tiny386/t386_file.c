#include "t386_file.h"
#include "t386_profile_config.h"
#include <limits.h>
#include <string.h>
#if T386_ENABLE_PROFILING
static T386FileProfile file_profile;
const T386FileProfile *t386_file_profile_get(void) { return &file_profile; }
void t386_file_profile_reset(void) { memset(&file_profile,0,sizeof(file_profile)); }
#endif
#ifdef ESP_PLATFORM
#include <unistd.h>
#endif
static bool seek_file(FILE *f,uint64_t offset) {
    if(!f || offset>INT64_MAX) return false;
#ifdef _WIN32
    return _fseeki64(f,(__int64)offset,SEEK_SET)==0;
#else
    /* IDF's off_t may be 32-bit. Reject truncation rather than wrapping. */
    off_t pos=(off_t)offset;
    if(pos<0 || (uint64_t)pos!=offset) return false;
    return fseeko(f,pos,SEEK_SET)==0;
#endif
}
bool t386_file_read(FILE *f,uint64_t offset,void *buf,size_t count) {
#if defined(ESP_PLATFORM) && T386_ENABLE_PROFILING
    extern uint64_t t386_host_time_us(void);
    uint64_t started=t386_host_time_us();
    bool sought=seek_file(f,offset);
    uint64_t reading=t386_host_time_us();
    size_t received=sought?fread(buf,1,count,f):0;
    bool ok=sought && received==count;
    uint64_t ended=t386_host_time_us();
    uint64_t elapsed=ended-started;
    file_profile.calls++;if(!ok)file_profile.failures++;
    file_profile.requested_bytes+=count;file_profile.total_bytes+=received;
    file_profile.seek_us+=reading-started;file_profile.read_us+=ended-reading;
    if(elapsed>2000)file_profile.slow++;
    if(elapsed>file_profile.max_us) {
        file_profile.max_us=elapsed>UINT32_MAX?UINT32_MAX:(unsigned)elapsed;
        file_profile.max_seek_us=reading-started>UINT32_MAX?UINT32_MAX:(unsigned)(reading-started);
        file_profile.max_read_us=ended-reading>UINT32_MAX?UINT32_MAX:(unsigned)(ended-reading);
        file_profile.max_count=count;file_profile.max_offset=offset;
    }
    return ok;
#else
    return seek_file(f,offset) && fread(buf,1,count,f)==count;
#endif
}
bool t386_file_write(FILE *f,uint64_t offset,const void *buf,size_t count) {
    return seek_file(f,offset) && t386_file_write_current(f,buf,count);
}
bool t386_file_write_current(FILE *f,const void *buf,size_t count) {
    if(!f)return false;
    bool ok=fwrite(buf,1,count,f)==count;
    /* Publish C-library buffered writes for alternate image handles. FatFS
     * durability remains the responsibility of the existing fsync path. */
    return fflush(f)==0 && ok;
}
bool t386_file_flush(FILE *f) {
    if(!f || ferror(f) || fflush(f)) return false;
#ifdef ESP_PLATFORM
    /* Flush FatFS metadata as well as the C library's stream buffer. */
    return fsync(fileno(f))==0;
#else
    return true;
#endif
}
bool t386_file_close(FILE **f) {
    if(!*f) return true;
    bool ok=!ferror(*f);
    if(fclose(*f)) ok=false;
    *f=NULL;
    return ok;
}
bool t386_file_load_rom(void *memory,size_t capacity,const char *path,uint32_t address,bool backward,int *loaded) {
    *loaded=0;
    FILE *f=fopen(path,"rb");
    if(!f) return false;
    long len=-1;
    if(!fseek(f,0,SEEK_END)) len=ftell(f);
    bool ok=len>0 && len<=INT_MAX;
    size_t start=address;
    if(backward) {if((uint64_t)len>start) ok=false; else start-=len;}
    if(start>capacity || (uint64_t)len>capacity-start) ok=false;
    if(ok) ok=t386_file_read(f,0,(unsigned char*)memory+start,(size_t)len);
    ok=t386_file_close(&f)&&ok;
    if(ok) *loaded=(int)len;
    return ok;
}
