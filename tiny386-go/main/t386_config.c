#include "t386_config.h"
#include "t386_input.h"
#include "t386_floppy.h"
#include "t386_firmware.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef struct {T386Config *c; const char *dir;} Parse;
static char *resolve(const char *dir,const char *path) {
    if(!*path) return NULL;
    if(path[0]=='/' || path[0]=='\\' || (strlen(path)>1 && path[1]==':')) return strdup(path);
    size_t len=strlen(dir)+strlen(path)+2;
    char *s=malloc(len); if(s) snprintf(s,len,"%s/%s",dir,path); return s;
}
static bool number(const char *s,long *out) {
    char *end; errno=0; long n=strtol(s,&end,10);
    if(errno||end==s||n<0) return false;
    long mul=1;
    if(*end) {if(end[1]) return false; mul=*end=='M'?1048576:*end=='K'?1024:0;}
    if(!mul||n>LONG_MAX/mul) return false;
    *out=n*mul; return true;
}
static int parse(void *opaque,const char *section,const char *key,const char *value) {
    Parse *p=opaque; PCConfig *c=&p->c->pc; const char **dest=NULL; long n;
    if(!strcmp(section,"controls")) {
        const char *names[]={"up","right","down","left","a","b","start","select"};
        for(int i=0;i<8;i++) if(!strcmp(key,names[i])) {int k=t386_keycode(value); if(k<0)return 0; p->c->keycodes[i]=k; return 1;}
        return 0;
    }
    if(!strcmp(section,"pc")) {
        if(!strcmp(key,"bios")) dest=&c->bios;
        else if(!strcmp(key,"vga_bios")) dest=&c->vga_bios;
        else if(strlen(key)==3 && !strncmp(key,"hd",2) && key[2]>='a' && key[2]<='d') {dest=&c->disks[key[2]-'a']; c->iscd[key[2]-'a']=0;}
        else if(strlen(key)==3 && !strncmp(key,"cd",2) && key[2]>='a' && key[2]<='d') {dest=&c->disks[key[2]-'a']; c->iscd[key[2]-'a']=1;}
        else if(strlen(key)==3 && !strncmp(key,"fd",2) && key[2]>='a' && key[2]<='b') dest=&c->fdd[key[2]-'a'];
        if(dest) {char *s=resolve(p->dir,value); if(!s)return 0; free((void*)*dest); *dest=s; return 1;}
        if(!number(value,&n)) return 0;
        if(!strcmp(key,"mem_size")) {if(n<1048576||n>64*1048576L)return 0; c->mem_size=n;}
        else if(!strcmp(key,"vga_mem_size")) {if(n<262144||n>2097152||(n&65535))return 0; c->vga_mem_size=n;}
        else if(!strcmp(key,"fill_cmos")) {if(n>1)return 0; c->fill_cmos=(int)n;}
        else if(!strcmp(key,"vga_force_8dm")) {if(n>1)return 0; c->vga_force_8dm=(int)n;}
        else return 0;
        return 1;
    }
    if(!number(value,&n)) return 0;
    if(!strcmp(section,"display")) return (!strcmp(key,"width")&&n==640)||(!strcmp(key,"height")&&n==480);
    if(!strcmp(section,"cpu")) {
        if(!strcmp(key,"gen") && n>=3&&n<=5) {c->cpu_gen=(int)n; return 1;}
        if(!strcmp(key,"fpu")&&n==0) return 1;
    }
    return 0;
}
void t386_config_free(T386Config *config) {
    PCConfig *c=&config->pc;
    free((void*)c->bios); free((void*)c->vga_bios);
    for(int i=0;i<4;i++) free((void*)c->disks[i]);
    for(int i=0;i<2;i++) free((void*)c->fdd[i]);
    memset(config,0,sizeof(*config));
}
bool t386_config_load(const char *path,T386Config *out,char *error,size_t cap) {
    memset(out,0,sizeof(*out)); if(cap) error[0]=0;
    out->pc=(PCConfig){.mem_size=2097152,.vga_mem_size=262144,.width=640,.height=480,.cpu_gen=3,.vga_force_8dm=1};
    const int keys[]={103,106,108,105,29,56,28,1}; memcpy(out->keycodes,keys,sizeof(keys));
    const char *reason="Invalid or unsupported descriptor";
    if(!path||strlen(path)<4||strcmp(path+strlen(path)-4,".ini")) goto fail;
    char *dir=strdup(path); if(!dir) {reason="Out of memory"; goto fail;}
    char *slash=strrchr(dir,'/'), *back=strrchr(dir,'\\'); if(back && (!slash||back>slash))slash=back;
    if(slash) {if(slash==dir)slash[1]=0;else *slash=0;} else strcpy(dir,".");
    Parse p={out,dir}; int ret=ini_parse(path,parse,&p); free(dir);
    if(ret) goto fail;
    if(!out->pc.bios) out->pc.bios=strdup(T386_BIOS_BUILTIN);
    if(!out->pc.vga_bios) out->pc.vga_bios=strdup(T386_VGA_BUILTIN);
    bool boot=false;
    for(int i=0;i<4;i++) boot|=out->pc.disks[i]!=NULL && !out->pc.iscd[i];
    for(int i=0;i<2;i++) boot|=out->pc.fdd[i]!=NULL;
    if(!boot) {reason="A DOS HDD or floppy image is required";goto fail;}
    const char *files[8]={out->pc.bios,out->pc.vga_bios,out->pc.disks[0],out->pc.disks[1],out->pc.disks[2],out->pc.disks[3],out->pc.fdd[0],out->pc.fdd[1]};
    if(!files[0]||!files[1]) {reason="Out of memory";goto fail;}
    for(int i=0;i<8;i++) if(files[i]) {
        if((i==0 && !strcmp(files[i],T386_BIOS_BUILTIN)) ||
           (i==1 && !strcmp(files[i],T386_VGA_BUILTIN))) continue;
        FILE *f=fopen(files[i], i>=2 && !(i<6&&out->pc.iscd[i-2])?"r+b":"rb");
        if(!f) {if(cap)snprintf(error,cap,"Cannot open %s",files[i]); t386_config_free(out);return false;}
        if(fseek(f,0,SEEK_END)) {fclose(f);reason="Cannot seek content";goto fail;}
        long len=ftell(f); fclose(f);
        if(len<=0||(i==0 && len>0x40000)||(i==1 && len>0x40000)||(i>=2&&(len%512))) {reason="Invalid content size";goto fail;}
        if(i>=6) {
            bool supported=false;
            for(unsigned j=0;fmt[j].sectors;j++)
                supported|=len==(long)fmt[j].sectors*fmt[j].tracks*fmt[j].heads*512;
            if(!supported) {reason="Unsupported floppy image geometry";goto fail;}
        }
    }
    return true;
fail:
    if(cap)snprintf(error,cap,"%s",reason);
    t386_config_free(out);
    return false;
}
