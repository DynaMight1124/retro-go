/* Bounded-workspace PNG: one uncompressed DEFLATE block per RGB scanline.
 * No image-sized allocation or encoder heap is required. Source is read-only. */
#include "screenshot.h"
#include <stdint.h>

static void be32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint32_t crc_bytes(uint32_t crc,const uint8_t *p,size_t n){
 while(n--){crc^=*p++;for(int i=0;i<8;i++)crc=(crc>>1)^(0xedb88320u & (0u-(crc&1)));}
 return crc;
}
static bool chunk(Screenshot_sink sink,void *ctx,const char *type,const uint8_t *p,size_t n){
 uint8_t header[8],tail[4];be32(header,(uint32_t)n);
 for(int i=0;i<4;i++)header[4+i]=(uint8_t)type[i];
 uint32_t crc=crc_bytes(0xffffffffu,header+4,4);
 if(n)crc=crc_bytes(crc,p,n);
 be32(tail,~crc);
 return sink(ctx,header,8)&&(!n||sink(ctx,p,n))&&sink(ctx,tail,4);
}
bool Screenshot_write(const rg_surface_t *s,int w,int h,Screenshot_sink sink,void *ctx){
 if(!s||!s->data||!sink||s->width<=0||s->width>320||s->height<=0||s->height>240||
    s->offset<0||s->stride<s->width*2||
    (s->format!=RG_PIXEL_565_LE&&s->format!=RG_PIXEL_565_BE))return false;
 if(w<=0&&h<=0){w=s->width;h=s->height;}
 else if(w<=0){if(h>240)return false;w=h*s->width/s->height;}
 else if(h<=0){if(w>320)return false;h=w*s->height/s->width;}
 if(w<=0||w>320||h<=0||h>240)return false;
 const uint8_t signature[]={137,80,78,71,13,10,26,10};
 uint8_t ihdr[13]={0};be32(ihdr,(uint32_t)w);be32(ihdr+4,(uint32_t)h);ihdr[8]=8;ihdr[9]=2;
 const uint8_t zheader[]={0x78,0x01};
 if(!sink(ctx,signature,8)||!chunk(sink,ctx,"IHDR",ihdr,13)||!chunk(sink,ctx,"IDAT",zheader,2))return false;
 uint32_t a=1,b=0;
 /* 5-byte stored-block header + filter byte + at most 320 RGB pixels. */
 uint8_t row[5+1+320*3];
 int n=1+w*3;
 row[1]=(uint8_t)n;row[2]=(uint8_t)(n>>8);row[3]=(uint8_t)~n;row[4]=(uint8_t)(~n>>8);row[5]=0;
 float step_x=(float)s->width/w,step_y=(float)s->height/h;
 for(int y=0;y<h;y++){
  row[0]=(y==h-1)?1:0; /* BFINAL; stored blocks start at byte boundaries. */
  const uint8_t *src=(const uint8_t*)s->data+s->offset+(int)(y*step_y)*s->stride;
  for(int x=0;x<w;x++){
   int i=(int)(x*step_x)*2;uint16_t p;
   if(s->format==RG_PIXEL_565_LE)p=(uint16_t)(src[i]|(src[i+1]<<8));
   else p=(uint16_t)((src[i]<<8)|src[i+1]);
   row[6+x*3]=(p>>8)&248;row[7+x*3]=(p>>3)&252;row[8+x*3]=(p&31)<<3;
  }
  for(int i=5;i<5+n;i++){a=(a+row[i])%65521;b=(b+a)%65521;}
  if(!chunk(sink,ctx,"IDAT",row,(size_t)n+5))return false;
 }
 uint8_t adler[4];be32(adler,(b<<16)|a);
 return chunk(sink,ctx,"IDAT",adler,4)&&chunk(sink,ctx,"IEND",NULL,0);
}

/* File ownership stays in the application. Failed writes are never reported as
 * successful screenshots; remove incomplete PNGs rather than leave corrupt ones. */
#include <stdio.h>
static bool file_sink(void *ctx,const void *data,size_t n){return fwrite(data,1,n,(FILE*)ctx)==n;}
bool Screenshot_save(const rg_surface_t *s,const char *filename,int w,int h){
 if(!filename||!*filename)return false;
 FILE *f=fopen(filename,"wb");if(!f)return false;
 bool ok=Screenshot_write(s,w,h,file_sink,f);
 if(fclose(f)!=0)ok=false;
 if(!ok)remove(filename);
 return ok;
}
