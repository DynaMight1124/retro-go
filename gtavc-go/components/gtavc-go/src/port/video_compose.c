#include "video_compose.h"
#include <rg_system.h>
#include <string.h>

#define OUT_W 320
#define OUT_H 240
#define BLOCK 16
#define COLS (OUT_W/BLOCK)
#define ROWS (OUT_H/BLOCK)
static struct {
    int *overlay;
    const int *world;
    int world_width,world_height;
    uint8_t dirty[COLS*ROWS];
    unsigned count;
    bool tracking,partial;
} compose;
/* Maps survive beginOverlay's per-frame reset. Only rebuilt on size changes. */
static uint16_t source_x[OUT_W],source_y[OUT_H];
static int mapped_width,mapped_height;

void Video_beginOverlay(int *overlay,const int *world,int world_width,int world_height) {
    memset(&compose,0,sizeof(compose));
    if(!overlay||!world||overlay==world||world_width<=0||world_width>OUT_W||
       world_height<=0||world_height>OUT_H)return;
    if(mapped_width!=world_width||mapped_height!=world_height) {
        for(int x=0;x<OUT_W;x++)source_x[x]=(uint16_t)(x*world_width/OUT_W);
        for(int y=0;y<OUT_H;y++)source_y[y]=(uint16_t)(y*world_height/OUT_H);
        mapped_width=world_width;mapped_height=world_height;
    }
    compose.overlay=overlay;compose.world=world;
    compose.world_width=world_width;compose.world_height=world_height;
    compose.tracking=compose.partial=true;
}
void Video_pauseTracking(void) { compose.tracking=false; }
void Video_clearOverlay(void) { memset(&compose,0,sizeof(compose)); }
bool Video_hasOverlay(void) { return compose.partial; }
unsigned Video_dirtyBlocks(void) { return compose.count; }

void IRAM_ATTR Video_prepareRect(int *buffer,int width,int height,int x,int y,int w,int h) {
    if(!compose.tracking || buffer!=compose.overlay || width!=OUT_W || height!=OUT_H || w<=0 || h<=0) return;
    int right=x+w,bottom=y+h;
    if(x<0)x=0;
    if(y<0)y=0;
    if(right>OUT_W)right=OUT_W;
    if(bottom>OUT_H)bottom=OUT_H;
    if(x>=right||y>=bottom)return;
    for(int by=y/BLOCK;by<=(bottom-1)/BLOCK;by++) {
        for(int bx=x/BLOCK;bx<=(right-1)/BLOCK;bx++) {
            int index=by*COLS+bx;
            if(compose.dirty[index])continue;
            compose.dirty[index]=1;compose.count++;
            for(int yy=by*BLOCK;yy<(by+1)*BLOCK;yy++) {
                int *dst=buffer+yy*OUT_W+bx*BLOCK;
                if(compose.world_width==160&&compose.world_height==120) {
                    const int *src=compose.world+(yy/2)*160+bx*(BLOCK/2);
                    for(int xx=0;xx<BLOCK/2;xx++)dst[xx*2]=dst[xx*2+1]=src[xx];
                } else {
                    const int *src=compose.world+source_y[yy]*compose.world_width;
                    for(int xx=0;xx<BLOCK;xx++)dst[xx]=src[source_x[bx*BLOCK+xx]];
                }
            }
        }
    }
}
void Video_materialize(void) {
    if(!compose.partial)return;
    compose.tracking=true;
    Video_prepareRect(compose.overlay,OUT_W,OUT_H,0,0,OUT_W,OUT_H);
    compose.tracking=false;compose.partial=false;
}
static inline uint16_t IRAM_ATTR pack565(int c,bool be) {
    unsigned p=((c>>8)&0xf800)|((c>>5)&0x7e0)|((c>>3)&31);
    return be?(uint16_t)((p<<8)|(p>>8)):(uint16_t)p;
}
void IRAM_ATTR Video_compose565(uint16_t *dest,int stride_pixels,bool big_endian) {
    if(!compose.partial||!dest||stride_pixels<OUT_W)return;
    if(compose.world_width==160&&compose.world_height==120) {
        /* Retain the specialised 2x path: convert each world pixel only once. */
        for(int y=0;y<OUT_H/2;y++) {
            uint16_t *a=dest+(y*2)*stride_pixels,*b=a+stride_pixels;
            const int *src=compose.world+y*(OUT_W/2);
            for(int x=0;x<OUT_W/2;x++) {
                uint16_t p=pack565(src[x],big_endian);
                a[x*2]=a[x*2+1]=b[x*2]=b[x*2+1]=p;
            }
        }
    } else {
        for(int y=0;y<OUT_H;y++) {
            uint16_t *row=dest+y*stride_pixels;
            if(y>0&&source_y[y]==source_y[y-1]) {
                memcpy(row,row-stride_pixels,OUT_W*sizeof(*row));
                continue;
            }
            const int *src=compose.world+source_y[y]*compose.world_width;
            int previous=-1;uint16_t pixel=0;
            for(int x=0;x<OUT_W;x++) {
                int sx=source_x[x];
                if(sx!=previous){pixel=pack565(src[sx],big_endian);previous=sx;}
                row[x]=pixel;
            }
        }
    }
    /* HUD colours were blended in 24-bit against the exact same world pixels. */
    for(int by=0;by<ROWS;by++)for(int bx=0;bx<COLS;bx++) {
        if(!compose.dirty[by*COLS+bx])continue;
        for(int y=by*BLOCK;y<(by+1)*BLOCK;y++) {
            const int *src=compose.overlay+y*OUT_W+bx*BLOCK;
            uint16_t *dst=dest+y*stride_pixels+bx*BLOCK;
            for(int x=0;x<BLOCK;x++)dst[x]=pack565(src[x],big_endian);
        }
    }
}
