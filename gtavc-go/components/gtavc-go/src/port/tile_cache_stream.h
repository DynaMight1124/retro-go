#ifndef GTAVC_TILE_CACHE_STREAM_H
#define GTAVC_TILE_CACHE_STREAM_H
#include "tile.h"
typedef struct TileCacheStream TileCacheStream;
TileCacheStream *Tile_cacheStreamCreate(size_t length,int i,int j,int ox,int oz,int tileMm);
/* -1 error, 0 more input, 1 complete; unconsumed bytes belong to the caller. */
int Tile_cacheStreamFeed(TileCacheStream *s,const uint8_t *data,size_t length,size_t *used);
Tile *Tile_cacheStreamTake(TileCacheStream *s);
void Tile_cacheStreamFree(TileCacheStream *s);
Tile *Tile_loadCacheCompact(const uint8_t *data,int length,int i,int j,int ox,int oz,int tileMm);
#endif
