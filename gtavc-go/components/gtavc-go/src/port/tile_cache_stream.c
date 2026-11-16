/* Incremental GTC1 decoder. Only final tile allocations live here; the caller
 * owns a bounded input buffer and retains any incomplete trailing record. */
#include "tile_cache_stream.h"
#include <string.h>

enum { TC_HEADER, TC_TEXTURES, TC_TOP, TC_LOW, TC_SURFACE, TC_SOLID, TC_MESH_HEADER, TC_VERTICES, TC_TRIANGLES, TC_DONE, TC_FAILED };
struct TileCacheStream {
    Tile *tile;
    Mesh *mesh; /* owned by tile as soon as allocated */
    size_t length, position;
    int phase, index, nearCount, farCount, meshIndex;
};
static unsigned be16(const uint8_t *p) { return ((unsigned)p[0] << 8) | p[1]; }
static int signed16(const uint8_t *p) { return (int16_t)be16(p); }

TileCacheStream *Tile_cacheStreamCreate(size_t length,int i,int j,int ox,int oz,int tileMm) {
    if (length < 12 || length > 4u*1024*1024) return NULL;
    TileCacheStream *s = calloc(1,sizeof(*s));
    if (!s) return NULL;
    s->tile = calloc(1,sizeof(Tile));
    if (!s->tile) { free(s); return NULL; }
    s->length = length;
    s->tile->i=i; s->tile->j=j;
    s->tile->cx=(ox+i*tileMm+tileMm/2)>>4;
    s->tile->cz=(oz+j*tileMm+tileMm/2)>>4;
    return s;
}
static bool allocateHeader(TileCacheStream *s,const uint8_t *p) {
    if (memcmp(p,"GTC1",4) || be16(p+4)!=1) return false;
    s->nearCount=be16(p+6); s->farCount=be16(p+8);
    Tile *t=s->tile; t->texCount=(int)be16(p+10);
    if(s->nearCount>512 || s->farCount>512 || t->texCount>512) {
        t->texCount=0; return false;
    }
    size_t minimum=12+(size_t)t->texCount*2+TILE_NH*TILE_NH*5+TILE_SROW*TILE_NS+
                   (size_t)(s->nearCount+s->farCount)*4;
    if(minimum>s->length) return false;
    if(t->texCount) {
        t->texPath=calloc(t->texCount,sizeof(char*));
        t->texPix=calloc(t->texCount,sizeof(int*));
        t->texW=calloc(t->texCount,sizeof(int)); t->texH=calloc(t->texCount,sizeof(int));
        t->slot=calloc(t->texCount,sizeof(int));
        if(!t->texPath||!t->texPix||!t->texW||!t->texH||!t->slot) return false;
    }
    t->top=malloc(TILE_NH*TILE_NH*sizeof(int16_t));
    t->low=malloc(TILE_NH*TILE_NH*sizeof(int16_t));
    t->surf=malloc(TILE_NH*TILE_NH); t->solid=malloc(TILE_SROW*TILE_NS);
    if(s->nearCount) t->nearChunks=calloc(s->nearCount,sizeof(Mesh*));
    if(s->farCount) t->farChunks=calloc(s->farCount,sizeof(Mesh*));
    return t->top&&t->low&&t->surf&&t->solid&&
           (!s->nearCount||t->nearChunks)&&(!s->farCount||t->farChunks);
}
int Tile_cacheStreamFeed(TileCacheStream *s,const uint8_t *data,size_t length,size_t *used) {
    *used=0;
    if(!s || s->phase==TC_FAILED || !s->tile) return -1;
    while(s->phase!=TC_FAILED) {
        Tile *t=s->tile;
        /* Advance empty sections without consuming a byte. */
        if(s->phase==TC_TEXTURES && s->index==t->texCount) {s->phase=TC_TOP;s->index=0;}
        if(s->phase==TC_MESH_HEADER && s->meshIndex==s->nearCount+s->farCount) s->phase=TC_DONE;
        if(s->phase==TC_DONE) return 1;
        size_t n=s->phase==TC_HEADER?12:s->phase==TC_MESH_HEADER?4:
                 s->phase==TC_VERTICES?10:s->phase==TC_TRIANGLES?15:
                 (s->phase==TC_SURFACE||s->phase==TC_SOLID)?1:2;
        if(n>s->length-s->position) {s->phase=TC_FAILED;break;}
        if(n>length-*used) return 0;
        const uint8_t *p=data+*used;
        switch(s->phase) {
        case TC_HEADER:
            if(!allocateHeader(s,p)) {s->phase=TC_FAILED;break;}
            s->phase=TC_TEXTURES;break;
        case TC_TEXTURES: {
            char path[32]; snprintf(path,sizeof(path),"/x/%u.png",be16(p));
            t->texPath[s->index]=strdup(path);
            if(!t->texPath[s->index]) {s->phase=TC_FAILED;break;}
            s->index++; break;
        }
        case TC_TOP: case TC_LOW:
            (s->phase==TC_TOP?t->top:t->low)[s->index++]=(int16_t)be16(p);
            if(s->index==TILE_NH*TILE_NH) {s->phase++;s->index=0;} break;
        case TC_SURFACE: case TC_SOLID:
            (s->phase==TC_SURFACE?t->surf:t->solid)[s->index++]=*p;
            if(s->index==(s->phase==TC_SURFACE?TILE_NH*TILE_NH:TILE_SROW*TILE_NS)) {
                s->phase++;s->index=0;
            } break;
        case TC_MESH_HEADER: {
            int nv=(int)be16(p),nt=(int)be16(p+2);
            if(!nv||!nt || (size_t)nv*10+(size_t)nt*15>s->length-s->position-4) {
                s->phase=TC_FAILED;break;
            }
            s->mesh=Mesh_newCompact(nv,nt);
            if(!s->mesh) {s->phase=TC_FAILED;break;}
            if(s->meshIndex<s->nearCount) t->nearChunks[t->nearChunkCount++]=s->mesh;
            else t->farChunks[t->farChunkCount++]=s->mesh;
            s->phase=TC_VERTICES;s->index=0;break;
        }
        case TC_VERTICES: {
            Mesh *m=s->mesh;int i=s->index++;
            int16_t *v = m->vertices16 + i*5;
            v[0]=signed16(p);v[1]=signed16(p+2);v[2]=signed16(p+4);
            v[3]=signed16(p+6);v[4]=signed16(p+8);
            if(s->index==m->nv) {s->phase=TC_TRIANGLES;s->index=0;}break;
        }
        case TC_TRIANGLES: {
            Mesh *m=s->mesh;int i=s->index++;
            unsigned a=be16(p),b=be16(p+2),c=be16(p+4),tex=be16(p+6);
            if(tex!=65535 && tex>=(unsigned)t->texCount) {s->phase=TC_FAILED;break;}
            m->indices16[i*3]=a;m->indices16[i*3+1]=b;m->indices16[i*3+2]=c;
            m->tex[i]=(int16_t)tex;m->flag[i]=(int8_t)p[8];
            m->col[i]=((int)p[9]<<16)|((int)p[10]<<8)|p[11];
            m->nx[i]=(int8_t)p[12];m->ny[i]=(int8_t)p[13];m->nz[i]=(int8_t)p[14];
            if(a>=(unsigned)m->nv||b>=(unsigned)m->nv||c>=(unsigned)m->nv) {
                m->indices16[i*3]=m->indices16[i*3+1]=m->indices16[i*3+2]=0;m->tex[i]=-1;
            }
            if(s->index==m->nt) {
                Mesh_bounds(m);s->mesh=NULL;s->meshIndex++;s->index=0;s->phase=TC_MESH_HEADER;
            }break;
        }
        default: s->phase=TC_FAILED;break;
        }
        if(s->phase==TC_FAILED) break;
        s->position+=n;*used+=n;
    }
    return -1;
}
Tile *Tile_cacheStreamTake(TileCacheStream *s) {
    if(!s||s->phase!=TC_DONE) return NULL;
    Tile *t=s->tile;s->tile=NULL;return t;
}
void Tile_cacheStreamFree(TileCacheStream *s) {
    if(!s) return;
    Tile_free(s->tile);free(s);
}

Tile *Tile_loadCacheCompact(const uint8_t *data,int length,int i,int j,int ox,int oz,int tileMm) {
    TileCacheStream *s = Tile_cacheStreamCreate((size_t)length,i,j,ox,oz,tileMm);
    if (!s) return NULL;
    size_t used;
    Tile *tile = Tile_cacheStreamFeed(s,data,(size_t)length,&used)==1 ? Tile_cacheStreamTake(s) : NULL;
    Tile_cacheStreamFree(s);
    return tile;
}
