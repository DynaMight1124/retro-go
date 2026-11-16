#ifndef GTAVC_TILE_H
#define GTAVC_TILE_H

#include <stdint.h>
#include <stdbool.h>
#include "mesh.h"
#include "j2me.h"

#define TILE_NH 50
#define TILE_NS 100
#define TILE_SROW 13

typedef struct Tile {
    int i, j;
    int cx, cz;
    Mesh *nearMesh, *farMesh;
    Mesh **nearChunks; int nearChunkCount;
    Mesh **farChunks;  int farChunkCount;
    int16_t *top, *low;
    uint8_t *surf, *solid;
    char** texPath;
    int** texPix;
    int* texW;
    int* texH;
    int* slot;
    int texCount;
    bool installed;
    /* Game-thread-only progress; acquired slots remain owned during cancellation. */
    int installNextTexture, installNextMesh, installLoadedTextures;
    int64_t installStartedUs;
    int lastUse;
} Tile;

Tile* Tile_load(int i, int j, int ox, int oz, int tileMm);
Tile* Tile_loadCache(const uint8_t* data, int len, int i, int j, int ox, int oz, int tileMm);
bool  Tile_saveCache(Tile* t, const char* path);
Mesh* Tile_readMesh(DataInputStream* d);
void  Tile_install(Tile* t);
/* Soft budget: a single texture acquisition/remap cannot be interrupted.
   Nonpositive budget completes synchronously. True means safe to publish. */
bool  Tile_installStep(Tile* t, int budgetUs);
/* Stops at an uncached texture without prepared pixels; never reads textures. */
bool  Tile_installPreparedStep(Tile* t, int budgetUs);
void  Tile_unload(Tile* t);
void  Tile_free(Tile* t);

#endif /* GTAVC_TILE_H */
