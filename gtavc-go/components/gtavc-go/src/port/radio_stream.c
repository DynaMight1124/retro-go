#include "radio_stream.h"
#include "tile_loader.h"
#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define RING_SAMPLES 32768u
#define READ_FRAMES 1024u
/* Cold bulk data is PSRAM-preferred; task stack and metadata remain internal. */
typedef struct {int16_t pcm[RING_SAMPLES];uint8_t raw[READ_FRAMES*4];} RadioMemory;
static struct {
    rg_mutex_t *lock;
    bool running;
    RadioMemory *memory;
    uint32_t generation, acknowledged, seed;
    char path[640];
    bool wanted, playing, ready, failed, eof;
    unsigned read, write, count, rate, phase, envelope, underruns;
} stream;

typedef struct {unsigned rate,channels,bits,align;uint32_t left;} WaveInfo;
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return u16(p)|((uint32_t)u16(p+2)<<16);}
/* Read only headers; skip data/unknown chunks by seek, never allocate the file. */
static bool waveOpen(FILE*f,WaveInfo*out,uint32_t seed) {
    uint8_t h[16];bool format=false,data=false;WaveInfo w={0};long dataAt=0;uint32_t dataSize=0;
    if(fseek(f,0,SEEK_END))return false;
    long size=ftell(f);if(size<12 || fseek(f,0,SEEK_SET))return false;
    if(fread(h,1,12,f)!=12 || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVE",4))return false;
    long pos=12;
    while((uint64_t)pos+8<=(uint64_t)size) {
        if(fseek(f,pos,SEEK_SET) || fread(h,1,8,f)!=8)return false;
        uint32_t n=u32(h+4);uint64_t payload=(uint64_t)pos+8;
        if(n>(uint64_t)size-payload)return false;
        if(!memcmp(h,"fmt ",4)) {
            if(n<16 || fread(h,1,16,f)!=16 || u16(h)!=1)return false;
            w.channels=u16(h+2);w.rate=u32(h+4);w.align=u16(h+12);w.bits=u16(h+14);
            if(w.channels<1 || w.channels>2 || (w.bits!=8 && w.bits!=16) ||
               w.rate<4000 || w.rate>48000 || w.align!=w.channels*(w.bits/8))return false;
            format=true;
        } else if(!memcmp(h,"data",4) && !data) {
            dataAt=(long)payload;dataSize=n;data=true;
        }
        if(format && data)break;
        uint64_t next=payload+n+(n&1u);if(next>(uint64_t)size || next>LONG_MAX)return false;
        pos=(long)next;
    }
    if(!format || !data || !dataSize || dataSize%w.align)return false;
    uint32_t frames=dataSize/w.align,guard=w.rate*30u;
    uint32_t start=frames>guard ? seed%(frames-guard) : 0;
    uint64_t offset=(uint64_t)dataAt+(uint64_t)start*w.align;
    if(offset>LONG_MAX || fseek(f,(long)offset,SEEK_SET))return false;
    w.left=(frames-start)*w.align;*out=w;return true;
}

void RadioStream_run(void) {
    if(!stream.lock)return;
    rg_mutex_take(stream.lock,-1);stream.running=true;rg_mutex_give(stream.lock);
    FILE *file=NULL;WaveInfo wave={0};uint32_t seen=0;
    for(;;) {
        rg_mutex_take(stream.lock,-1);
        uint32_t generation=stream.generation;
        uint32_t seed=stream.seed;
        bool changed=generation!=seen,wanted=stream.wanted;
        char path[sizeof(stream.path)];
        unsigned underruns=0;
        if(changed){memcpy(path,stream.path,sizeof(path));underruns=stream.underruns;}
        rg_mutex_give(stream.lock);
        if(changed) {
            /* Close/cancel must never wait on terrain. New radio opens can. */
            if(wanted && TileLoader_ioBusy()){rg_task_delay(10);continue;}
            rg_mutex_take(stream.lock,-1);stream.underruns=0;rg_mutex_give(stream.lock);
            if(file){fclose(file);file=NULL;}
            if(underruns)RG_LOGW("Radio buffer starved in %u audio blocks (effects continued)",underruns);
            bool ok=true;
            if(wanted) {
                file=fopen(path,"rb");ok=file && waveOpen(file,&wave,seed);
            }
            rg_mutex_take(stream.lock,-1);
            if(stream.generation!=generation) {
                rg_mutex_give(stream.lock);
                if(file){fclose(file);file=NULL;}
                continue;
            }
            stream.read=stream.write=stream.count=stream.phase=stream.envelope=0;
            stream.ready=false;stream.failed=wanted&&!ok;stream.eof=false;
            if(wanted&&ok)stream.rate=wave.rate;
            seen=generation;
            rg_mutex_give(stream.lock);
            if(!ok && file){fclose(file);file=NULL;}
            if(wanted&&!ok)RG_LOGW("Radio WAV unavailable or unsupported: %s",path);
            if(wanted&&ok)RG_LOGD("Radio stream: %u Hz, %u channel(s), %u-bit PCM; 64 KiB ring",wave.rate,wave.channels,wave.bits);
            rg_mutex_take(stream.lock,-1);
            stream.acknowledged=generation;
            rg_mutex_give(stream.lock);
            continue;
        }
        rg_mutex_take(stream.lock,-1);
        RadioMemory *memory=stream.memory;
        unsigned freeSamples=RING_SAMPLES-stream.count,write=stream.write;
        rg_mutex_give(stream.lock);
        if(!file || !memory || !wanted || !freeSamples){rg_task_delay(10);continue;}
        /* Buffered music can wait; terrain needed by a moving car cannot. A
           radio read already in flight remains bounded to <=4 KiB. */
        if(TileLoader_ioBusy()){rg_task_delay(10);continue;}
        /* Batch refills instead of chasing each tiny audio consumption step. */
        if(freeSamples<READ_FRAMES && wave.left/wave.align>=READ_FRAMES){rg_task_delay(10);continue;}
        unsigned frames=wave.left/wave.align;
        if(frames>READ_FRAMES)frames=READ_FRAMES;
        if(frames>freeSamples)frames=freeSamples;
        unsigned bytes=frames*wave.align;
        /* Neither the metadata lock nor the sound lock covers this SD read. */
        size_t got=fread(memory->raw,1,bytes,file);
        bool ok=got==bytes && frames>0;
        if(ok) {
            for(unsigned i=0;i<frames;i++) {
                int sample=0;
                for(unsigned c=0;c<wave.channels;c++) {
                    const uint8_t *p=memory->raw+i*wave.align+c*(wave.bits/8);
                    sample+=wave.bits==16 ? (int16_t)u16(p) : ((int)p[0]-128)*256;
                }
                memory->pcm[(write+i)%RING_SAMPLES]=(int16_t)(sample/(int)wave.channels);
            }
            wave.left-=bytes;
        }
        rg_mutex_take(stream.lock,-1);
        if(stream.generation==generation) {
            if(ok) {
                stream.write=(write+frames)%RING_SAMPLES;stream.count+=frames;
                stream.eof=wave.left==0;
                /* Start with two seconds of the normal 8 kHz radio buffered.
                   Higher rates are capped at half the existing ring capacity. */
                unsigned threshold=wave.rate*2;
                if(threshold>RING_SAMPLES/2)threshold=RING_SAMPLES/2;
                if(stream.count>=threshold || stream.eof)stream.ready=true;
            }else{stream.failed=true;stream.ready=false;}
        }
        rg_mutex_give(stream.lock);
        if(!ok || !wave.left){fclose(file);file=NULL;}
        if(!ok)RG_LOGW("Radio WAV read failed");
        /* One <=4 KiB read per turn; terrain has the higher worker priority. */
        rg_task_delay(1);
    }
}

bool RadioStream_init(void) {
    if(!stream.lock)stream.lock=rg_mutex_create();
    return stream.lock!=NULL;
}
bool RadioStream_open(const char *path,uint32_t seed) {
    if(!path || !*path || strlen(path)>=sizeof(stream.path))return false;
    if(!stream.lock)return false;
    if(!stream.memory) {
        RadioMemory *memory=rg_alloc(sizeof(RadioMemory),MEM_SLOW|MEM_NOPANIC);
        if(!memory)return false;
        rg_mutex_take(stream.lock,-1);stream.memory=memory;rg_mutex_give(stream.lock);
    }
    rg_mutex_take(stream.lock,-1);
    snprintf(stream.path,sizeof(stream.path),"%s",path);
    stream.seed=seed;
    stream.generation++;stream.wanted=true;stream.playing=false;
    stream.failed=stream.ready=stream.eof=false;
    rg_mutex_give(stream.lock);
    return true;
}
void RadioStream_play(void) {
    if(!stream.lock)return;
    rg_mutex_take(stream.lock,-1);stream.playing=stream.wanted;rg_mutex_give(stream.lock);
}
void RadioStream_cancel(void) {
    if(!stream.lock)return;
    rg_mutex_take(stream.lock,-1);
    if(stream.wanted)stream.generation++;
    stream.wanted=stream.playing=stream.ready=stream.failed=false;
    rg_mutex_give(stream.lock);
}
void RadioStream_close(void) {
    if(!stream.lock)return;
    RadioStream_cancel();
    rg_mutex_take(stream.lock,-1);uint32_t target=stream.generation;bool running=stream.running;rg_mutex_give(stream.lock);
    if(running)for(;;) {
        rg_mutex_take(stream.lock,-1);bool quiet=stream.acknowledged==target;rg_mutex_give(stream.lock);
        if(quiet)break;
        rg_task_delay(1);
    }
    rg_mutex_take(stream.lock,-1);RadioMemory *memory=stream.memory;stream.memory=NULL;stream.count=0;rg_mutex_give(stream.lock);
    free(memory);
}
bool RadioStream_active(void) {
    if(!stream.lock)return false;
    rg_mutex_take(stream.lock,-1);bool active=stream.wanted&&!stream.failed&&(!stream.eof || stream.count);rg_mutex_give(stream.lock);return active;
}
bool RadioStream_failed(void) {
    if(!stream.lock)return false;
    rg_mutex_take(stream.lock,-1);bool failed=stream.failed;rg_mutex_give(stream.lock);return failed;
}
void RadioStream_mix(int16_t*out,int frames,int rate) {
    if(!out || frames<=0 || rate<=0 || !stream.lock || !rg_mutex_take(stream.lock,0))return;
    if(!stream.playing || !stream.ready || !stream.memory){rg_mutex_give(stream.lock);return;}
    unsigned step=(unsigned)(((uint64_t)stream.rate<<16)/(unsigned)rate);if(!step)step=1;
    bool starved=false;
    for(int i=0;i<frames;i++) {
        unsigned nextPhase=stream.phase+step,advance=nextPhase>>16;
        unsigned need=advance>2?advance:2;
        if(!stream.count || (stream.count<need && !stream.eof)){
            if(!stream.eof)starved=true;
            continue;
        }
        int a=stream.memory->pcm[stream.read];
        int b=stream.count>1 ? stream.memory->pcm[(stream.read+1)%RING_SAMPLES] : a;
        int sample=a+((b-a)*(int)(stream.phase>>8)>>8);
        unsigned envelope=stream.envelope;
        if(stream.envelope<256)stream.envelope+=8;
        if(stream.eof) {
            uint64_t left=((uint64_t)stream.count<<16)-stream.phase;
            uint64_t fade=(uint64_t)step*96;
            if(left<fade) {
                unsigned ending=(unsigned)(left*256/fade);
                if(ending<envelope)envelope=ending;
            }
        }
        sample=(sample*(int)envelope/256)*98/256;
        for(int channel=0;channel<2;channel++) {
            int mixed=out[i*2+channel]+sample;
            out[i*2+channel]=(int16_t)(mixed>32767?32767:mixed < -32768 ? -32768:mixed);
        }
        stream.phase=nextPhase&65535;
        if(advance>stream.count)advance=stream.count;
        stream.read=(stream.read+advance)%RING_SAMPLES;stream.count-=advance;
    }
    if(starved)stream.underruns++;
    rg_mutex_give(stream.lock);
}
