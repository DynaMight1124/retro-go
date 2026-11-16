#include "save.h"
#include "world.h"
#include "game.h"
#include "j2me.h"
#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define SAVE_NAME "gtavc_save"
#define OPT_NAME "gtavc_opts"
#define SAVE_BYTES 50

typedef struct {
    int money,x,z,y,yaw,minutes,ammo;
    bool bat,pistol,mission;
} Progress;
static uint32_t getbe(const uint8_t*p) {return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static uint32_t getle(const uint8_t*p) {return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void putbe(uint8_t*p,int value) {uint32_t v=(uint32_t)value;p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static bool decodeProgress(const uint8_t*b,size_t n,Progress*out) {
    if(n!=46 && n!=50) return false;
    if(getle(b)!=0x54455253u || getle(b+4)!=1 || getle(b+8)!=n-12) return false;
    const uint8_t*p=b+12;uint32_t version=getbe(p);
    if((version!=1 && version!=2) || n!=(version==1?46u:50u)) return false;
    if(p[28]>1 || p[29]>1 || (version==2 && getbe(p+34)>1)) return false;
    Progress v={.money=(int32_t)getbe(p+4),.x=(int32_t)getbe(p+8),.z=(int32_t)getbe(p+12),
                .y=(int32_t)getbe(p+16),.yaw=(int32_t)getbe(p+20),.minutes=(int32_t)getbe(p+24),
                .ammo=(int32_t)getbe(p+30),.bat=p[28]!=0,.pistol=p[29]!=0,
                .mission=version==2 && getbe(p+34)!=0};
    if(v.minutes<0 || v.minutes>=1440 || v.ammo<0) return false;
    *out=v;return true;
}
static bool readBytes(const char*path,uint8_t*b,size_t*n) {
    FILE*f=fopen(path,"rb");if(!f)return false;
    *n=fread(b,1,SAVE_BYTES+1,f);bool ok=!ferror(f);return fclose(f)==0 && ok;
}
static bool readProgress(const char*path,Progress*out) {
    uint8_t b[SAVE_BYTES+1];size_t n=0;
    return readBytes(path,b,&n) && decodeProgress(b,n,out);
}
static bool inMap(const World*w,const Progress*p) {
    return w && w->player && w->tileMm>0 && w->tnx>0 && w->tnz>0 &&
           (int64_t)p->x>=w->mox && (int64_t)p->x<(int64_t)w->mox+(int64_t)w->tnx*w->tileMm &&
           (int64_t)p->z>=w->moz && (int64_t)p->z<(int64_t)w->moz+(int64_t)w->tnz*w->tileMm;
}
static bool nativePath(char*out,size_t cap) {
    int n=snprintf(out,cap,"%s/te_%s.rms",platform_data_dir(),SAVE_NAME);
    return n>0 && (size_t)n<cap;
}
bool Save_writePath(World*w,const char*path) {
    if(!w || !w->player || w->health<=0 || w->busted || !path) return false;
    Progress v={.money=w->money,.x=w->inCar?w->inCar->x:w->player->x,
                .z=w->inCar?w->inCar->z:w->player->z,.y=w->inCar?w->inCar->y:w->player->y,
                .yaw=w->player->yaw,.minutes=w->minutes,.ammo=w->ammo,
                .bat=w->hasBat,.pistol=w->hasPistol,.mission=w->missionDone};
    if(!inMap(w,&v) || v.minutes<0 || v.minutes>=1440 || v.ammo<0) return false;
    uint8_t b[SAVE_BYTES]={0x53,0x52,0x45,0x54,1,0,0,0,38,0,0,0};uint8_t*p=b+12;
    putbe(p,2);putbe(p+4,v.money);putbe(p+8,v.x);putbe(p+12,v.z);putbe(p+16,v.y);
    putbe(p+20,v.yaw);putbe(p+24,v.minutes);p[28]=v.bat;p[29]=v.pistol;
    putbe(p+30,v.ammo);putbe(p+34,v.mission?1:0);
    /* The caller supplies a staging path: native .tmp or Retro-Go's .new.
       Verify actual bytes: this checkout's shared atomic flag is a TODO. */
    if(!rg_storage_write_file(path,b,sizeof(b),0)) return false;
    uint8_t check[SAVE_BYTES+1];size_t n=0;
    return readBytes(path,check,&n) && n==sizeof(b) && !memcmp(check,b,n);
}
bool Save_loadPath(World*w,const char*path) {
    Progress v;
    if(!path || !readProgress(path,&v) || !inMap(w,&v)) return false;
    /* Complete validation precedes any mutation. Progress loads restart transient
       state (health/wanted/actors), just like the native Continue operation. */
    World_reset(w);
    w->money=v.money;w->minutes=v.minutes;w->ammo=v.ammo;
    w->hasBat=v.bat;w->hasPistol=v.pistol;w->missionDone=v.mission;
    World_continueAt(w,v.x,v.z,v.y,v.yaw);
    return true;
}
bool Save_exists(void) {
    char path[600],backup[608];Progress p;
    if(!nativePath(path,sizeof(path)))return false;
    snprintf(backup,sizeof(backup),"%s.bak",path);
    return readProgress(path,&p) || readProgress(backup,&p);
}
bool Save_load(World*w) {
    char path[600],backup[608];
    if(!nativePath(path,sizeof(path)))return false;
    snprintf(backup,sizeof(backup),"%s.bak",path);
    if(Save_loadPath(w,path)) return true;
    if(!Save_loadPath(w,backup)) return false;
    RG_LOGW("Recovered native progress from previous checkpoint");
    return true;
}
bool Save_write(World*w) {
    char path[600],temp[608],backup[608];
    if(!nativePath(path,sizeof(path)))return false;
    snprintf(temp,sizeof(temp),"%s.tmp",path);snprintf(backup,sizeof(backup),"%s.bak",path);
    if(!Save_writePath(w,temp)) {remove(temp);return false;}
    struct stat st;bool exists=stat(path,&st)==0,moved=false;
    if(!exists && errno!=ENOENT) {remove(temp);return false;}
    if(exists) {
        Progress old,prior;
        if((!readProgress(path,&old) || !inMap(w,&old)) &&
           readProgress(backup,&prior) && inMap(w,&prior)) {
            /* Preserve the known-good backup rather than rotating corrupt data
               over it. Failed publication can still recover from that backup. */
            if(remove(path)!=0){remove(temp);return false;}
        } else {
            if(remove(backup)!=0 && errno!=ENOENT){remove(temp);return false;}
            if(rename(path,backup)!=0){remove(temp);return false;}
            moved=true;
        }
    }
    if(rename(temp,path)!=0) {
        if(moved) rename(backup,path);
        remove(temp);return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Display options: fixed-size legacy RMS codec, no RecordStore heap.   */
/* ------------------------------------------------------------------ */
typedef struct {
    int bright,draw,radar,density,res;
    bool limiter,trails,subs,wide,legend,hud,radio;
} Options;
static bool validOptions(const Options *o) {
    return o->bright>=0 && o->bright<=15 && o->draw>=0 && o->draw<=15 &&
           o->radar>=0 && o->radar<=2 && o->density>=0 && o->density<=3 && o->res>=0 && o->res<=3;
}
static bool decodeOptions(const uint8_t *b,size_t n,Options *out) {
    if(n!=43)return false;
    if(getle(b)!=0x54455253u || getle(b+4)!=1 || getle(b+8)!=n-12)return false;
    const uint8_t *p=b+12;uint32_t version=getbe(p);
    if(version!=2)return false;
    for(int i=12;i<=16;i++)if(p[i]>1)return false;
    if(p[21]>1 || p[30]>1)return false;
    Options o={.bright=(int32_t)getbe(p+4),.draw=(int32_t)getbe(p+8),
        .radar=(int32_t)getbe(p+17),.density=(int32_t)getbe(p+22),.res=(int32_t)getbe(p+26),
        .limiter=p[12]!=0,.trails=p[13]!=0,.subs=p[14]!=0,.wide=p[15]!=0,
        .legend=p[16]!=0,.hud=p[21]!=0,.radio=p[30]!=0};
    if(!validOptions(&o))return false;
    *out=o;return true;
}
static bool readOptions(const char *path,Options *out) {
    uint8_t b[SAVE_BYTES+1];size_t n=0;
    return readBytes(path,b,&n) && decodeOptions(b,n,out);
}
static bool optionsPath(char *out,size_t cap) {
    int n=snprintf(out,cap,"%s/te_%s.rms",platform_data_dir(),OPT_NAME);
    return n>0 && (size_t)n<cap;
}
static bool writeOptions(const Game *g) {
    if(!g)return false;
    Options o={.bright=g->oBright,.draw=g->oDraw,.radar=g->oRadar,.density=g->oDensity,.res=g->oRes,
        .limiter=g->oLimiter,.trails=g->oTrails,.subs=g->oSubs,.wide=g->oWide,
        .legend=g->oLegend,.hud=g->oHud,.radio=g->oRadio};
    if(!validOptions(&o))return false;
    uint8_t b[43]={0x53,0x52,0x45,0x54,1,0,0,0,31,0,0,0};uint8_t *p=b+12;
    putbe(p,2);putbe(p+4,o.bright);putbe(p+8,o.draw);
    p[12]=o.limiter;p[13]=o.trails;p[14]=o.subs;p[15]=o.wide;p[16]=o.legend;
    putbe(p+17,o.radar);p[21]=o.hud;putbe(p+22,o.density);putbe(p+26,o.res);p[30]=o.radio;
    char path[600],temp[608],backup[608];
    if(!optionsPath(path,sizeof(path)))return false;
    snprintf(temp,sizeof(temp),"%s.tmp",path);snprintf(backup,sizeof(backup),"%s.bak",path);
    uint8_t check[SAVE_BYTES+1];size_t n=0;
    if(!rg_storage_write_file(temp,b,sizeof(b),0) || !readBytes(temp,check,&n) ||
       n!=sizeof(b) || memcmp(check,b,n)) {remove(temp);return false;}
    struct stat st;bool exists=stat(path,&st)==0,moved=false;
    if(!exists && errno!=ENOENT){remove(temp);return false;}
    if(exists){
        Options old,prior;
        if(!readOptions(path,&old) && readOptions(backup,&prior)){
            /* Never rotate a corrupt primary over a valid recovery copy. */
            if(remove(path)!=0){remove(temp);return false;}
        }else{
            if(remove(backup)!=0 && errno!=ENOENT){remove(temp);return false;}
            if(rename(path,backup)!=0){remove(temp);return false;}
            moved=true;
        }
    }
    if(rename(temp,path)!=0){
        if(moved)rename(backup,path);
        remove(temp);return false;
    }
    return true;
}
bool Save_writeOptions(struct Game *g) {
    bool ok=writeOptions(g);
    if(!ok)RG_LOGW("Native options could not be saved; current session settings retained");
    return ok;
}
bool Save_loadOptions(struct Game *g) {
    if(!g)return false;
    char path[600],backup[608];Options o;
    if(!optionsPath(path,sizeof(path)))return false;
    snprintf(backup,sizeof(backup),"%s.bak",path);
    if(!readOptions(path,&o)){
        if(!readOptions(backup,&o))return false;
        RG_LOGW("Recovered native options from previous settings");
    }
    /* Apply only after the entire bounded record has been validated. */
    g->oBright=o.bright;g->oDraw=o.draw;g->oRadar=o.radar;g->oDensity=o.density;g->oRes=o.res;
    g->oLimiter=o.limiter;g->oTrails=o.trails;g->oSubs=o.subs;g->oWide=o.wide;
    g->oLegend=o.legend;g->oHud=o.hud;g->oRadio=o.radio;g->needResize=true;
    return true;
}
