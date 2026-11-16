#include "tile.h"
#include "gtavc_profile.h"
#include "tile_cache_stream.h"
#include "assets.h"
#include "car_model.h"
#include "j2me.h"
#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void tile_split(const Mesh* src, Mesh*** outChunks, int* outCount);

/* Last tile load metrics, consumed by the libretro wrapper for gtavc.log. */
int Tile_lastSeq = 0;
int Tile_lastMs = 0;
int Tile_lastCache = 0;
int Tile_lastI = 0;
int Tile_lastJ = 0;
int Tile_installSeq = 0;
int Tile_installMs = 0;
int Tile_installTex = 0;
int Tile_installI = 0;
int Tile_installJ = 0;

Mesh* Tile_readMesh(DataInputStream* d) {
    int nv = dis_readUnsignedShort(d);
    int vertStart = d->pos;
    d->pos += nv * 10;                 /* x,y,z,u,v shorts */
    int nt = dis_readUnsignedShort(d);
    int triStart = d->pos;
    Mesh* m = Mesh_new(nv, nt);
    if (!m) return NULL;

    d->pos = vertStart;
    for (int i = 0; i < nv; i++) {
        m->x[i] = dis_readShort(d);
        m->y[i] = dis_readShort(d);
        m->z[i] = dis_readShort(d);
        m->u[i] = dis_readShort(d);
        m->v[i] = dis_readShort(d);
    }
    d->pos = triStart;
    for (int i = 0; i < nt; i++) {
        int a = dis_readUnsignedShort(d);
        int bb = dis_readUnsignedShort(d);
        int c = dis_readUnsignedShort(d);
        m->ti[i * 3]     = a;
        m->ti[i * 3 + 1] = bb;
        m->ti[i * 3 + 2] = c;
        int t = dis_readUnsignedShort(d);
        m->tex[i] = (int16_t)(t == 0xFFFF ? -1 : t);
        m->flag[i] = (int8_t)dis_readUnsignedByte(d);
        m->col[i] = (dis_readUnsignedByte(d) << 16) | (dis_readUnsignedByte(d) << 8) | dis_readUnsignedByte(d);
        if (a >= nv || bb >= nv || c >= nv) {
            m->ti[i * 3] = 0; m->ti[i * 3 + 1] = 0; m->ti[i * 3 + 2] = 0;
            m->tex[i] = -1;
        }
        CarModel_normal(m, i, m->ti[i * 3], m->ti[i * 3 + 1], m->ti[i * 3 + 2]);
    }
    Mesh_bounds(m);
    return m;
}

static Mesh* cacheReadMesh(DataInputStream* d) {
    int nv = dis_readUnsignedShort(d);
    int nt = dis_readUnsignedShort(d);
    if (nv <= 0 || nt <= 0 || dis_available(d) < 0) return NULL;
    Mesh* m = Mesh_new(nv, nt);
    if (!m) return NULL;
    for (int i = 0; i < nv; i++) {
        m->x[i] = dis_readShort(d);
        m->y[i] = dis_readShort(d);
        m->z[i] = dis_readShort(d);
        m->u[i] = dis_readShort(d);
        m->v[i] = dis_readShort(d);
    }
    for (int i = 0; i < nt; i++) {
        m->ti[i * 3]     = dis_readUnsignedShort(d);
        m->ti[i * 3 + 1] = dis_readUnsignedShort(d);
        m->ti[i * 3 + 2] = dis_readUnsignedShort(d);
        int tex = dis_readUnsignedShort(d);
        m->tex[i] = (int16_t)(tex == 0xFFFF ? -1 : tex);
        m->flag[i] = (int8_t)dis_readByte(d);
        int r = dis_readUnsignedByte(d), g = dis_readUnsignedByte(d), b = dis_readUnsignedByte(d);
        m->col[i] = (r << 16) | (g << 8) | b;
        m->nx[i] = (int8_t)dis_readByte(d);
        m->ny[i] = (int8_t)dis_readByte(d);
        m->nz[i] = (int8_t)dis_readByte(d);
        if (m->ti[i * 3] >= nv || m->ti[i * 3 + 1] >= nv || m->ti[i * 3 + 2] >= nv) {
            m->ti[i * 3] = m->ti[i * 3 + 1] = m->ti[i * 3 + 2] = 0;
            m->tex[i] = -1;
        }
    }
    Mesh_bounds(m);
    return m;
}

/* Pre-split tile cache.  Splitting a dense tile on the 87 MHz target costs
   more than a frame; the host installer writes the chunks once so streaming
   only has to read and copy them. */
Tile* Tile_loadCache(const uint8_t* data, int len, int i, int j, int ox, int oz, int tileMm) {
    DataInputStream d;
    dis_init(&d, data, len);
    if (dis_readInt(&d) != 0x47544331) { dis_close(&d); return NULL; } /* GTC1 */
    if (dis_readUnsignedShort(&d) != 1) { dis_close(&d); return NULL; }

    int nearCount = dis_readUnsignedShort(&d);
    int farCount  = dis_readUnsignedShort(&d);
    int texCount  = dis_readUnsignedShort(&d);
    if (nearCount < 0 || nearCount > 512 || farCount < 0 || farCount > 512 ||
        texCount < 0 || texCount > 512) {
        dis_close(&d);
        return NULL;
    }

    Tile* t = (Tile*)calloc(1, sizeof(Tile));
    if (!t) { dis_close(&d); return NULL; }
    t->i = i;
    t->j = j;
    t->cx = (ox + i * tileMm + tileMm / 2) >> 4;
    t->cz = (oz + j * tileMm + tileMm / 2) >> 4;
    t->texCount = texCount;
    t->texPath = (char**)calloc((size_t)texCount, sizeof(char*));
    t->texPix  = (int**)calloc((size_t)texCount, sizeof(int*));
    t->texW    = (int*)calloc((size_t)texCount, sizeof(int));
    t->texH    = (int*)calloc((size_t)texCount, sizeof(int));
    t->slot    = (int*)calloc((size_t)texCount, sizeof(int));
    if ((texCount > 0 && (!t->texPath || !t->texPix || !t->texW || !t->texH || !t->slot))) {
        dis_close(&d);
        Tile_free(t);
        return NULL;
    }
    for (int k = 0; k < texCount; k++) {
        int id = dis_readUnsignedShort(&d);
        char pbuf[64];
        snprintf(pbuf, sizeof(pbuf), "/x/%d.png", id);
        t->texPath[k] = strdup(pbuf);
    }

    int nH = TILE_NH * TILE_NH;
    t->top = (int16_t*)malloc((size_t)nH * sizeof(int16_t));
    t->low = (int16_t*)malloc((size_t)nH * sizeof(int16_t));
    t->surf = (uint8_t*)malloc((size_t)nH);
    t->solid = (uint8_t*)malloc((size_t)(TILE_SROW * TILE_NS));
    if (!t->top || !t->low || !t->surf || !t->solid) {
        dis_close(&d);
        Tile_free(t);
        return NULL;
    }
    for (int k = 0; k < nH; k++) t->top[k] = (int16_t)dis_readShort(&d);
    for (int k = 0; k < nH; k++) t->low[k] = (int16_t)dis_readShort(&d);
    dis_readFully(&d, t->surf, nH);
    dis_readFully(&d, t->solid, TILE_SROW * TILE_NS);

    t->nearChunks = (Mesh**)calloc((size_t)nearCount, sizeof(Mesh*));
    t->farChunks  = (Mesh**)calloc((size_t)farCount, sizeof(Mesh*));
    if ((nearCount > 0 && !t->nearChunks) || (farCount > 0 && !t->farChunks)) {
        dis_close(&d);
        Tile_free(t);
        return NULL;
    }
    for (int c = 0; c < nearCount; c++) {
        t->nearChunks[c] = cacheReadMesh(&d);
        if (!t->nearChunks[c]) { dis_close(&d); Tile_free(t); return NULL; }
        t->nearChunkCount++;
    }
    for (int c = 0; c < farCount; c++) {
        t->farChunks[c] = cacheReadMesh(&d);
        if (!t->farChunks[c]) { dis_close(&d); Tile_free(t); return NULL; }
        t->farChunkCount++;
    }
    if (dis_available(&d) < 0) {
        dis_close(&d);
        Tile_free(t);
        return NULL;
    }
    dis_close(&d);
    return t;
}

static bool cacheWriteMesh(FILE* f, Mesh* m) {
    if (!m || m->nv <= 0 || m->nt <= 0) return false;
    uint8_t hdr[4];
    hdr[0] = (uint8_t)(m->nv >> 8); hdr[1] = (uint8_t)m->nv;
    hdr[2] = (uint8_t)(m->nt >> 8); hdr[3] = (uint8_t)m->nt;
    if (fwrite(hdr, 1, 4, f) != 4) return false;
    for (int i = 0; i < m->nv; i++) {
        int16_t v[5] = { (int16_t)Mesh_vertex(m,i,0), (int16_t)Mesh_vertex(m,i,1), (int16_t)Mesh_vertex(m,i,2),
                         (int16_t)Mesh_vertex(m,i,3), (int16_t)Mesh_vertex(m,i,4) };
        uint8_t b[10];
        for (int k = 0; k < 5; k++) { b[k * 2] = (uint8_t)((uint16_t)v[k] >> 8); b[k * 2 + 1] = (uint8_t)v[k]; }
        if (fwrite(b, 1, 10, f) != 10) return false;
    }
    for (int i = 0; i < m->nt; i++) {
        uint8_t b[15];
        int idx[3] = { Mesh_index(m,i * 3), Mesh_index(m,i * 3 + 1), Mesh_index(m,i * 3 + 2) };
        int tex = m->tex[i];
        for (int k = 0; k < 3; k++) { b[k * 2] = (uint8_t)((uint16_t)idx[k] >> 8); b[k * 2 + 1] = (uint8_t)idx[k]; }
        b[6] = (uint8_t)((uint16_t)(tex < 0 ? 0xFFFF : tex) >> 8);
        b[7] = (uint8_t)(tex < 0 ? 0xFF : tex);
        b[8] = (uint8_t)m->flag[i];
        b[9] = (uint8_t)((m->col[i] >> 16) & 255);
        b[10] = (uint8_t)((m->col[i] >> 8) & 255);
        b[11] = (uint8_t)(m->col[i] & 255);
        b[12] = (uint8_t)m->nx[i];
        b[13] = (uint8_t)m->ny[i];
        b[14] = (uint8_t)m->nz[i];
        if (fwrite(b, 1, 15, f) != 15) return false;
    }
    return true;
}

bool Tile_saveCache(Tile* t, const char* path) {
    if (!t || !path || t->nearChunkCount <= 0) return false;
    FILE* f = fopen(path, "wb");
    if (!f) return false;

    uint8_t hdr[6];
    hdr[0] = 'G'; hdr[1] = 'T'; hdr[2] = 'C'; hdr[3] = '1';
    hdr[4] = 0; hdr[5] = 1;
    fwrite(hdr, 1, 6, f);
    uint8_t c[6];
    c[0] = (uint8_t)(t->nearChunkCount >> 8); c[1] = (uint8_t)t->nearChunkCount;
    c[2] = (uint8_t)(t->farChunkCount >> 8);  c[3] = (uint8_t)t->farChunkCount;
    c[4] = (uint8_t)(t->texCount >> 8);       c[5] = (uint8_t)t->texCount;
    fwrite(c, 1, 6, f);
    for (int k = 0; k < t->texCount; k++) {
        int id = 0;
        if (t->texPath[k]) {
            const char* s = strrchr(t->texPath[k], '/');
            if (s) id = atoi(s + 1);
        }
        uint8_t b[2] = { (uint8_t)(id >> 8), (uint8_t)id };
        fwrite(b, 1, 2, f);
    }
    for (int k = 0; k < TILE_NH * TILE_NH; k++) {
        uint16_t v = (uint16_t)t->top[k];
        uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
        fwrite(b, 1, 2, f);
    }
    for (int k = 0; k < TILE_NH * TILE_NH; k++) {
        uint16_t v = (uint16_t)t->low[k];
        uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
        fwrite(b, 1, 2, f);
    }
    fwrite(t->surf, 1, TILE_NH * TILE_NH, f);
    fwrite(t->solid, 1, TILE_SROW * TILE_NS, f);
    for (int i = 0; i < t->nearChunkCount; i++) if (!cacheWriteMesh(f, t->nearChunks[i])) { fclose(f); return false; }
    for (int i = 0; i < t->farChunkCount; i++)  if (!cacheWriteMesh(f, t->farChunks[i]))  { fclose(f); return false; }
    fclose(f);
    return true;
}

Tile* Tile_load(int i, int j, int ox, int oz, int tileMm) {
    long long loadStart = System_currentTimeMillis();
    char cachePath[64];
    snprintf(cachePath, sizeof(cachePath), "/tc/%d_%d.bin", i, j);
    int cacheLen = 0;
    uint8_t* cacheData = Assets_readAll(cachePath, &cacheLen);
    if (cacheData) {
        Tile* cached = Tile_loadCacheCompact(cacheData, cacheLen, i, j, ox, oz, tileMm);
        free(cacheData);
        if (cached) {
            Tile_lastSeq++;
            Tile_lastMs = (int)(System_currentTimeMillis() - loadStart);
            Tile_lastCache = 1;
            Tile_lastI = i;
            Tile_lastJ = j;
            return cached;
        }
    }

    static int logTiles = -1;
    if (logTiles < 0) { const char* e = getenv("GTAVC_TILELOG"); logTiles = GTAVC_DETAILED_PROFILE && e && *e; }
    long long t0 = logTiles ? System_currentTimeMillis() : 0;
    char path[256];
    snprintf(path, sizeof(path), "/t/%d_%d.bin", i, j);
    int len = 0;
    uint8_t* b = Assets_readAll(path, &len);
    if (!b) return NULL;

    DataInputStream d;
    dis_init(&d, b, len);

    Tile* t = (Tile*)calloc(1, sizeof(Tile));
    if (!t) { dis_close(&d); free(b); return NULL; }
    t->i = i;
    t->j = j;
    t->cx = (ox + i * tileMm + tileMm / 2) >> 4;
    t->cz = (oz + j * tileMm + tileMm / 2) >> 4;

    dis_readByte(&d);
    dis_readByte(&d);
    int nt = dis_readUnsignedShort(&d);
    t->texCount = nt;
    t->texPath = (char**)calloc((size_t)nt, sizeof(char*));
    t->texPix  = (int**)calloc((size_t)nt, sizeof(int*));
    t->texW    = (int*)calloc((size_t)nt, sizeof(int));
    t->texH    = (int*)calloc((size_t)nt, sizeof(int));
    t->slot    = (int*)calloc((size_t)nt, sizeof(int));
    if ((nt > 0 && (!t->texPath || !t->texPix || !t->texW || !t->texH || !t->slot))) {
        dis_close(&d);
        free(b);
        Tile_free(t);
        return NULL;
    }

    for (int k = 0; k < nt; k++) {
        int id = dis_readUnsignedShort(&d);
        char pbuf[64];
        snprintf(pbuf, sizeof(pbuf), "/x/%d.png", id);
        t->texPath[k] = strdup(pbuf);
    }

    long long t1 = logTiles ? System_currentTimeMillis() : 0;
    t->nearMesh = Tile_readMesh(&d);
    long long t2 = logTiles ? System_currentTimeMillis() : 0;
    t->farMesh  = Tile_readMesh(&d);
    long long t3 = logTiles ? System_currentTimeMillis() : 0;
    if (!t->nearMesh || !t->farMesh) {
        dis_close(&d);
        free(b);
        Tile_free(t);
        return NULL;
    }
    tile_split(t->nearMesh, &t->nearChunks, &t->nearChunkCount);
    long long t4 = logTiles ? System_currentTimeMillis() : 0;
    tile_split(t->farMesh,  &t->farChunks,  &t->farChunkCount);
    long long t5 = logTiles ? System_currentTimeMillis() : 0;
    if (t->nearChunks) { Mesh_free(t->nearMesh); t->nearMesh = NULL; }
    if (t->farChunks)  { Mesh_free(t->farMesh);  t->farMesh = NULL; }

    int nH = TILE_NH * TILE_NH;
    t->top = (int16_t*)malloc((size_t)nH * sizeof(int16_t));
    t->low = (int16_t*)malloc((size_t)nH * sizeof(int16_t));
    if (!t->top || !t->low) {
        dis_close(&d); free(b); Tile_free(t); return NULL;
    }
    for (int k = 0; k < nH; k++) t->top[k] = (int16_t)dis_readShort(&d);
    for (int k = 0; k < nH; k++) t->low[k] = (int16_t)dis_readShort(&d);

    t->surf = (uint8_t*)malloc((size_t)nH);
    if (!t->surf) { dis_close(&d); free(b); Tile_free(t); return NULL; }
    dis_readFully(&d, t->surf, nH);

    int nSolid = TILE_SROW * TILE_NS;
    t->solid = (uint8_t*)malloc((size_t)nSolid);
    if (!t->solid) { dis_close(&d); free(b); Tile_free(t); return NULL; }
    dis_readFully(&d, t->solid, nSolid);
    dis_close(&d);
    free(b);

    int dim[2];
    for (int k = 0; k < nt; k++) {
        if (Assets_find(t->texPath[k]) >= 0) continue;
        t->texPix[k] = Assets_decode(t->texPath[k], dim);
        t->texW[k] = dim[0];
        t->texH[k] = dim[1];
    }
    Tile_lastSeq++;
    Tile_lastMs = (int)(System_currentTimeMillis() - loadStart);
    Tile_lastCache = 0;
    Tile_lastI = i;
    Tile_lastJ = j;
    if (logTiles) {
        long long t6 = System_currentTimeMillis();
        RG_LOGD("TILE %d_%d bytes=%d read=%lld near=%lld far=%lld splitN=%lld splitF=%lld tex=%lld chunks=%d/%d\n",
                i, j, len, t1-t0, t2-t1, t3-t2, t4-t3, t5-t4, t6-t5,
                t->nearChunkCount, t->farChunkCount);
    }
    return t;
}

static void remapMesh(Mesh* m, const int* slot) {
    if (!m) return;
    for (int i = 0; i < m->nt; i++) {
        int t = m->tex[i];
        if (t >= 0) m->tex[i] = (int16_t)slot[t];
    }
}

/*
 * Split a tile mesh into a grid of small meshes so off-screen parts of a
 * 100 m tile are not transformed every frame.  The upstream Java build
 * transforms all vertices of every visible tile; on the 87 MHz MIPS target a
 * dense downtown tile has ~13k vertices, almost all outside the camera
 * frustum.  Chunks keep the same geometry, only the culling granularity
 * changes.
 */
#ifndef TILE_SPLIT
#define TILE_SPLIT 8
#endif
#ifndef TILE_YSPLIT
#define TILE_YSPLIT 4
#endif
#define TILE_HALF 3200
#define TILE_YMIN (-1024)
#define TILE_YSPAN 5120
#define TILE_BUCKETS (TILE_SPLIT * TILE_SPLIT * TILE_YSPLIT)

static Mesh* tile_make_chunk(const Mesh* src, const int* triList, int triCount, int* map, int* used, int* usedCount) {
    Mesh* m = Mesh_new(triCount * 3, triCount);
    if (!m) return NULL;
    int nv = 0;
    *usedCount = 0;
    for (int i = 0; i < triCount; i++) {
        int ti = triList[i];
        int idx[3] = { src->ti[ti * 3], src->ti[ti * 3 + 1], src->ti[ti * 3 + 2] };
        for (int k = 0; k < 3; k++) {
            int v = idx[k];
            if (map[v] < 0) {
                map[v] = nv;
                used[(*usedCount)++] = v;
                m->x[nv] = src->x[v];
                m->y[nv] = src->y[v];
                m->z[nv] = src->z[v];
                m->u[nv] = src->u[v];
                m->v[nv] = src->v[v];
                nv++;
            }
            m->ti[i * 3 + k] = map[v];
        }
        m->tex[i] = src->tex[ti];
        m->col[i] = src->col[ti];
        m->flag[i] = src->flag[ti];
        m->nx[i] = src->nx[ti];
        m->ny[i] = src->ny[ti];
        m->nz[i] = src->nz[ti];
    }
    m->nv = nv;
    Mesh_bounds(m);
    return m;
}

static void tile_split(const Mesh* src, Mesh*** outChunks, int* outCount) {
    *outChunks = NULL;
    *outCount = 0;
    if (!src || src->nt == 0) return;

    int counts[TILE_BUCKETS];
    int starts[TILE_BUCKETS + 1];
    memset(counts, 0, sizeof(counts));
    for (int i = 0; i < src->nt; i++) {
        int i0 = src->ti[i * 3], i1 = src->ti[i * 3 + 1], i2 = src->ti[i * 3 + 2];
        int cx = (src->x[i0] + src->x[i1] + src->x[i2]) / 3;
        int cy = (src->y[i0] + src->y[i1] + src->y[i2]) / 3;
        int cz = (src->z[i0] + src->z[i1] + src->z[i2]) / 3;
        int gx = (cx + TILE_HALF) * TILE_SPLIT / (TILE_HALF * 2);
        int gz = (cz + TILE_HALF) * TILE_SPLIT / (TILE_HALF * 2);
        int gy = (cy - TILE_YMIN) * TILE_YSPLIT / TILE_YSPAN;
        if (gx < 0) gx = 0; else if (gx >= TILE_SPLIT) gx = TILE_SPLIT - 1;
        if (gz < 0) gz = 0; else if (gz >= TILE_SPLIT) gz = TILE_SPLIT - 1;
        if (gy < 0) gy = 0; else if (gy >= TILE_YSPLIT) gy = TILE_YSPLIT - 1;
        counts[(gy * TILE_SPLIT + gz) * TILE_SPLIT + gx]++;
    }
    starts[0] = 0;
    int nonEmpty = 0;
    for (int i = 0; i < TILE_BUCKETS; i++) {
        starts[i + 1] = starts[i] + counts[i];
        if (counts[i]) nonEmpty++;
    }
    if (nonEmpty == 0) return;

    int* list = (int*)malloc((size_t)src->nt * sizeof(int));
    int* fill = (int*)malloc((size_t)TILE_BUCKETS * sizeof(int));
    int* map = (int*)malloc((size_t)src->nv * sizeof(int));
    int* used = (int*)malloc((size_t)src->nt * 3 * sizeof(int));
    if (!list || !fill || !map || !used) {
        free(list); free(fill); free(map); free(used);
        return;
    }
    for (int i = 0; i < src->nv; i++) map[i] = -1;
    memcpy(fill, starts, (size_t)TILE_BUCKETS * sizeof(int));
    for (int i = 0; i < src->nt; i++) {
        int i0 = src->ti[i * 3], i1 = src->ti[i * 3 + 1], i2 = src->ti[i * 3 + 2];
        int cx = (src->x[i0] + src->x[i1] + src->x[i2]) / 3;
        int cy = (src->y[i0] + src->y[i1] + src->y[i2]) / 3;
        int cz = (src->z[i0] + src->z[i1] + src->z[i2]) / 3;
        int gx = (cx + TILE_HALF) * TILE_SPLIT / (TILE_HALF * 2);
        int gz = (cz + TILE_HALF) * TILE_SPLIT / (TILE_HALF * 2);
        int gy = (cy - TILE_YMIN) * TILE_YSPLIT / TILE_YSPAN;
        if (gx < 0) gx = 0; else if (gx >= TILE_SPLIT) gx = TILE_SPLIT - 1;
        if (gz < 0) gz = 0; else if (gz >= TILE_SPLIT) gz = TILE_SPLIT - 1;
        if (gy < 0) gy = 0; else if (gy >= TILE_YSPLIT) gy = TILE_YSPLIT - 1;
        int b = (gy * TILE_SPLIT + gz) * TILE_SPLIT + gx;
        list[fill[b]++] = i;
    }
    free(fill);

    Mesh** chunks = (Mesh**)calloc((size_t)nonEmpty, sizeof(Mesh*));
    if (!chunks) {
        free(list); free(map); free(used);
        return;
    }
    int n = 0;
    for (int b = 0; b < TILE_BUCKETS; b++) {
        if (counts[b] == 0) continue;
        int usedCount = 0;
        Mesh* m = tile_make_chunk(src, list + starts[b], counts[b], map, used, &usedCount);
        for (int u = 0; u < usedCount; u++) map[used[u]] = -1;
        if (m) chunks[n++] = m;
    }
    free(list); free(map); free(used);
    *outChunks = chunks;
    *outCount = n;
}

static bool tileInstallStep(Tile* t, int budgetUs, bool preparedOnly) {
    if (!t || t->installed) return true;
    int64_t start = rg_system_timer();
    if (!t->installNextTexture && !t->installNextMesh) t->installStartedUs = start;
    while (t->installNextTexture < t->texCount) {
        int k = t->installNextTexture;
        int resident = (preparedOnly || t->texPix[k]) ? Assets_find(t->texPath[k]) : -1;
        if (preparedOnly && !t->texPix[k] && resident < 0) return false;
        /* Assets_acquire doesn't consume supplied pixels for resident names.
           Preparation can race with another game-thread acquisition. */
        if (t->texPix[k] && resident >= 0) {free(t->texPix[k]);t->texPix[k]=NULL;}
        t->slot[k] = Assets_acquire(t->texPath[k], t->texPix[k], t->texW[k], t->texH[k]);
        t->texPix[k] = NULL;
        if (t->slot[k] >= 0) t->installLoadedTextures++;
        t->installNextTexture++;
        if (budgetUs > 0 && rg_system_timer() - start >= budgetUs) return false;
    }
    int chunks = t->nearChunkCount + t->farChunkCount;
    while (t->installNextMesh < chunks) {
        int n = t->installNextMesh++;
        remapMesh(n < t->nearChunkCount ? t->nearChunks[n] :
                  t->farChunks[n - t->nearChunkCount], t->slot);
        if (t->installNextMesh < chunks && budgetUs > 0 &&
            rg_system_timer() - start >= budgetUs) return false;
    }
    t->installed = true;
    Tile_installSeq++;
    Tile_installMs = (int)((rg_system_timer() - t->installStartedUs) / 1000);
    Tile_installTex = t->installLoadedTextures;
    Tile_installI = t->i;
    Tile_installJ = t->j;
    return true;
}

bool Tile_installStep(Tile* t, int budgetUs) {return tileInstallStep(t,budgetUs,false);}
bool Tile_installPreparedStep(Tile* t, int budgetUs) {return tileInstallStep(t,budgetUs,true);}

void Tile_install(Tile* t) {
    Tile_installStep(t, 0);
}

void Tile_unload(Tile* t) {
    if (!t) return;
    if (t->installed || t->installNextTexture) {
        int count = t->installed ? t->texCount : t->installNextTexture;
        for (int k = 0; k < count; k++) {
            Assets_release(t->slot[k]);
        }
    }
    t->installed = false;
    t->installNextTexture = t->installNextMesh = t->installLoadedTextures = 0;
    if (t->nearMesh) { Mesh_free(t->nearMesh); t->nearMesh = NULL; }
    if (t->farMesh)  { Mesh_free(t->farMesh);  t->farMesh = NULL; }
    for (int i = 0; i < t->nearChunkCount; i++) if (t->nearChunks[i]) Mesh_free(t->nearChunks[i]);
    for (int i = 0; i < t->farChunkCount; i++) if (t->farChunks[i]) Mesh_free(t->farChunks[i]);
    free(t->nearChunks); t->nearChunks = NULL; t->nearChunkCount = 0;
    free(t->farChunks);  t->farChunks = NULL;  t->farChunkCount = 0;
    free(t->top);   t->top = NULL;
    free(t->low);   t->low = NULL;
    free(t->surf);  t->surf = NULL;
    free(t->solid); t->solid = NULL;
}

void Tile_free(Tile* t) {
    if (!t) return;
    Tile_unload(t);
    for (int k = 0; k < t->texCount; k++) {
        if (t->texPath) free(t->texPath[k]);
        if (t->texPix) free(t->texPix[k]);
    }
    free(t->texPath);
    free(t->texPix);
    free(t->texW);
    free(t->texH);
    free(t->slot);
    free(t);
}
