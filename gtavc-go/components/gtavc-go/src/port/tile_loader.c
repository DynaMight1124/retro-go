#include "tile_loader.h"
#include "tile_cache_stream.h"
#include <rg_system.h>
#include <string.h>

#define TILE_LOADER_BUFFER 65536

enum { LOADER_IDLE, LOADER_QUEUED, LOADER_RUNNING, LOADER_READY, LOADER_DRAIN };
static struct {
    rg_mutex_t *lock;
    rg_task_t *task;
    unsigned generation;
    int state;
    TileLoadRequest request;
    TileLoadResult result;
    bool textureJob;
    TileTextureRequest textureRequest;
    TileTextureResult textureResult;
} loader;

static bool isCancelled(TileLoadCancelled fn,void *arg) { return fn && fn(arg); }

void TileLoader_freeTextures(TileTextureResult *out) {
    for (int i=0;i<TILE_TEXTURE_BATCH;i++) {free(out->pixels[i]);out->pixels[i]=NULL;}
}

typedef struct { FILE *file; char path[600]; unsigned count; } AtlasReader;
static void closeAtlas(AtlasReader *reader) {
    if(reader->file) fclose(reader->file);
    memset(reader,0,sizeof(*reader));
}

static void readTextures(AtlasReader *reader,const TileTextureRequest *q,TileTextureResult *out,
                         TileLoadCancelled cancelled,void *arg) {
    memset(out,0,sizeof(*out));out->status=TL_DECODE_ERROR;
    int64_t start=rg_system_timer(),stamp;
    if(q->count<1 || q->count>TILE_TEXTURE_BATCH) goto done;
    out->count=q->count;
    memcpy(out->indices,q->indices,sizeof(out->indices));
    if(isCancelled(cancelled,arg)) {out->status=TL_CANCELLED;goto done;}
    if(reader->file && strcmp(reader->path,q->path)) closeAtlas(reader);
    if(!reader->file) {
        stamp=rg_system_timer();reader->file=fopen(q->path,"rb");
        if(!reader->file) {out->status=TL_NO_CACHE;goto done;}
        snprintf(reader->path,sizeof(reader->path),"%s",q->path);
        uint8_t header[8];
        if(fread(header,1,8,reader->file)!=8 || memcmp(header,"XAT1",4)) goto done;
        reader->count=(unsigned)header[4]|((unsigned)header[5]<<8)|
                      ((unsigned)header[6]<<16)|((unsigned)header[7]<<24);
        out->open_us=(uint64_t)(rg_system_timer()-stamp);
        if(!reader->count || reader->count>8192) goto done;
    }
    for(int n=0;n<q->count;n++) {
        if(isCancelled(cancelled,arg)) {out->status=TL_CANCELLED;goto done;}
        if(q->ids[n]<0 || (unsigned)q->ids[n]>=reader->count) goto done;
        int *pixels=malloc(4096);
        if(!pixels) {out->status=TL_ALLOC_ERROR;goto done;}
        out->pixels[n]=pixels;
        stamp=rg_system_timer();
        if(fseek(reader->file,8L+(long)q->ids[n]*4096L,SEEK_SET)!=0 ||
           fread(pixels,1,4096,reader->file)!=4096) {out->status=TL_IO_ERROR;goto done;}
        uint32_t elapsed=(uint32_t)(rg_system_timer()-stamp);
        out->read_us+=elapsed;out->bytes_read+=4096;
        if(elapsed>out->read_peak_us) out->read_peak_us=elapsed;
    }
    out->status=TL_OK;
done:
    if(isCancelled(cancelled,arg)) out->status=TL_CANCELLED;
    if(out->status!=TL_OK) {TileLoader_freeTextures(out);closeAtlas(reader);}
    out->elapsed_us=(uint64_t)(rg_system_timer()-start);
}

void TileLoader_readTextures(const TileTextureRequest *q,TileTextureResult *out,
                             TileLoadCancelled cancelled,void *arg) {
    AtlasReader reader={0};
    readTextures(&reader,q,out,cancelled,arg);
    closeAtlas(&reader); /* standalone backend retains its close-before-return contract */
}

void TileLoader_decodeFile(const TileLoadRequest *q,TileLoadResult *out,TileLoadCancelled cancelled,void *arg) {
    memset(out,0,sizeof(*out));
    out->i=q->i;out->j=q->j;out->k=q->k;out->status=TL_DECODE_ERROR;
    int64_t start=rg_system_timer(),stamp;
    FILE *file=NULL;
    uint8_t *buffer=NULL;
    TileCacheStream *decoder=NULL;
    if(isCancelled(cancelled,arg)) {out->status=TL_CANCELLED;goto done;}
    stamp=rg_system_timer();
    file=fopen(q->path,"rb");
    if(!file) {
        out->stats.open_us=(uint64_t)(rg_system_timer()-stamp);
        out->status=TL_NO_CACHE;goto done;
    }
    if(fseek(file,0,SEEK_END)!=0) {out->status=TL_IO_ERROR;goto done;}
    long length=ftell(file);
    if(fseek(file,0,SEEK_SET)!=0) {out->status=TL_IO_ERROR;goto done;}
    out->stats.open_us=(uint64_t)(rg_system_timer()-stamp);
    if(length<12 || length>4*1024*1024) goto done;
    out->stats.file_bytes=(int)length;
    int capacity=length<TILE_LOADER_BUFFER?(int)length:TILE_LOADER_BUFFER;
    stamp=rg_system_timer();
    buffer=malloc((size_t)capacity);
    if(buffer) decoder=Tile_cacheStreamCreate((size_t)length,q->i,q->j,q->ox,q->oz,q->tileMm);
    out->stats.alloc_us=(uint64_t)(rg_system_timer()-stamp);
    if(!decoder) {out->status=TL_ALLOC_ERROR;goto done;}
    int got=0,buffered=0;
    for(;;) {
        if(isCancelled(cancelled,arg)) {out->status=TL_CANCELLED;break;}
        int chunk=(int)length-got;
        if(chunk>capacity-buffered) chunk=capacity-buffered;
        if(chunk<=0) break;
        stamp=rg_system_timer();
        size_t n=fread(buffer+buffered,1,(size_t)chunk,file);
        uint32_t elapsed=(uint32_t)(rg_system_timer()-stamp);
        out->stats.read_us+=elapsed;out->stats.read_calls++;
        if(elapsed>out->stats.read_peak_us) out->stats.read_peak_us=elapsed;
        if(!n) {out->status=TL_IO_ERROR;break;}
        got+=(int)n;buffered+=(int)n;out->stats.bytes_read+=(uint32_t)n;
        if(isCancelled(cancelled,arg)) {out->status=TL_CANCELLED;break;}
        size_t used=0;
        stamp=rg_system_timer();
        int status=Tile_cacheStreamFeed(decoder,buffer,(size_t)buffered,&used);
        elapsed=(uint32_t)(rg_system_timer()-stamp);
        out->stats.decode_us+=elapsed;out->stats.decode_calls++;
        if(elapsed>out->stats.decode_peak_us) out->stats.decode_peak_us=elapsed;
        if(status==1) {out->tile=Tile_cacheStreamTake(decoder);out->status=TL_OK;break;}
        if(status<0 || got==(int)length) break;
        buffered-=(int)used;
        memmove(buffer,buffer+used,(size_t)buffered);
        rg_task_yield();
    }
done:
    Tile_cacheStreamFree(decoder);
    free(buffer);
    if(file) fclose(file);
    if(isCancelled(cancelled,arg)) {
        Tile_free(out->tile);out->tile=NULL;out->status=TL_CANCELLED;
    }
    out->elapsed_us=(uint64_t)(rg_system_timer()-start);
}

static bool cancelledGeneration(void *arg) {
    unsigned generation=*(unsigned*)arg;
    rg_mutex_take(loader.lock,-1);
    bool cancelled=loader.generation!=generation;
    rg_mutex_give(loader.lock);
    return cancelled;
}
static void tileWorker(void *arg) {
    (void)arg;
    AtlasReader reader={0}; /* worker-private; never accessed by cancellation caller */
    for(;;) {
        TileLoadRequest request;
        TileTextureRequest textureRequest;
        bool textureJob=false;
        unsigned generation=0;
        bool have_job=false;
        bool drain=false;
        rg_mutex_take(loader.lock,-1);
        if(loader.state==LOADER_DRAIN) drain=true;
        if(loader.state==LOADER_QUEUED) {
            textureJob=loader.textureJob;
            if(textureJob) textureRequest=loader.textureRequest;
            else request=loader.request;
            generation=loader.generation;
            loader.state=LOADER_RUNNING;have_job=true;
        }
        rg_mutex_give(loader.lock);
        if(drain) {
            closeAtlas(&reader);
            rg_mutex_take(loader.lock,-1);
            loader.state=LOADER_IDLE;
            rg_mutex_give(loader.lock);
            continue;
        }
        if(!have_job) {rg_task_delay(10);continue;}
        TileLoadResult result={0};TileTextureResult textures={0};
        if(textureJob) readTextures(&reader,&textureRequest,&textures,cancelledGeneration,&generation);
        else {
            closeAtlas(&reader); /* one cached handle, no overlap with geometry files */
            TileLoader_decodeFile(&request,&result,cancelledGeneration,&generation);
        }
        rg_mutex_take(loader.lock,-1);
        if(loader.generation==generation && (textureJob?textures.status:result.status)!=TL_CANCELLED) {
            if(textureJob) loader.textureResult=textures;
            else loader.result=result;
            loader.state=LOADER_READY;
            rg_mutex_give(loader.lock);
        } else {
            /* Keep RUNNING until cleanup finishes: waitIdle must not return
               while a stale private tile still owns memory. */
            rg_mutex_give(loader.lock);
            Tile_free(result.tile);
            TileLoader_freeTextures(&textures);
            closeAtlas(&reader);
            rg_mutex_take(loader.lock,-1);
            loader.state=LOADER_IDLE;
            rg_mutex_give(loader.lock);
        }
    }
}
bool TileLoader_start(void) {
    if(loader.task) return true;
    loader.lock=rg_mutex_create();
    if(!loader.lock) return false;
    /* SDSPI waits for an interrupt-driven transaction for each 512-byte sector.
       Above the busy game task (priority 1), completion can resume us immediately
       instead of waiting for an equal-priority scheduler time slice per sector.
       Keep affinity 0; audio/display tasks run on core 1. */
    loader.task=rg_task_create("gtavc_loader",tileWorker,NULL,6*1024,1,RG_TASK_PRIORITY_2,0);
    if(!loader.task) {rg_mutex_free(loader.lock);loader.lock=NULL;return false;}
    return true;
}
bool TileLoader_ioBusy(void) {
    if(!loader.lock)return false;
    if(!rg_mutex_take(loader.lock,0))return true;
    bool busy=loader.state==LOADER_QUEUED || loader.state==LOADER_RUNNING || loader.state==LOADER_DRAIN;
    rg_mutex_give(loader.lock);
    return busy;
}
bool TileLoader_submit(const TileLoadRequest *request) {
    if(!loader.task || !request || !rg_mutex_take(loader.lock,0)) return false;
    bool accepted=loader.state==LOADER_IDLE;
    if(accepted) {
        loader.textureJob=false;loader.request=*request;
        loader.request.path[sizeof(loader.request.path)-1]=0;
        loader.generation++;loader.state=LOADER_QUEUED;
    }
    rg_mutex_give(loader.lock);
    return accepted;
}
bool TileLoader_poll(TileLoadResult *result) {
    if(!loader.task || !result || !rg_mutex_take(loader.lock,0)) return false;
    bool ready=loader.state==LOADER_READY && !loader.textureJob;
    if(ready) {
        *result=loader.result;
        loader.result.tile=NULL;loader.state=LOADER_IDLE;
    }
    rg_mutex_give(loader.lock);
    return ready;
}
bool TileLoader_submitTextures(const TileTextureRequest *request) {
    if(!loader.task || !request || request->count<1 || request->count>TILE_TEXTURE_BATCH ||
       !rg_mutex_take(loader.lock,0)) return false;
    bool accepted=loader.state==LOADER_IDLE;
    if(accepted) {
        loader.textureJob=true;loader.textureRequest=*request;
        loader.textureRequest.path[sizeof(loader.textureRequest.path)-1]=0;
        loader.generation++;loader.state=LOADER_QUEUED;
    }
    rg_mutex_give(loader.lock);return accepted;
}
bool TileLoader_pollTextures(TileTextureResult *result) {
    if(!loader.task || !result || !rg_mutex_take(loader.lock,0)) return false;
    bool ready=loader.state==LOADER_READY && loader.textureJob;
    if(ready) {
        *result=loader.textureResult;
        memset(&loader.textureResult,0,sizeof(loader.textureResult));
        loader.state=LOADER_IDLE;
    }
    rg_mutex_give(loader.lock);return ready;
}
void TileLoader_cancel(void) {
    if(!loader.task) return;
    Tile *discard=NULL;
    TileTextureResult textures={0};
    rg_mutex_take(loader.lock,-1);
    loader.generation++;
    if(loader.state==LOADER_READY) {
        if(loader.textureJob) {
            textures=loader.textureResult;memset(&loader.textureResult,0,sizeof(loader.textureResult));
        } else {discard=loader.result.tile;loader.result.tile=NULL;}
    }
    /* IDLE can retain a worker-private atlas after a consumed batch. The worker
       must close it before waitIdle or a replacement job is allowed through. */
    if(loader.state!=LOADER_RUNNING) loader.state=LOADER_DRAIN;
    rg_mutex_give(loader.lock);
    Tile_free(discard);
    TileLoader_freeTextures(&textures);
}
void TileLoader_waitIdle(void) {
    if(!loader.task) return;
    TileLoader_cancel();
    for(;;) {
        rg_mutex_take(loader.lock,-1);
        bool idle=loader.state==LOADER_IDLE;
        rg_mutex_give(loader.lock);
        if(idle) return;
        rg_task_delay(10);
    }
}
