#include "mesh.h"
#include <stdlib.h>
#include "fx_math.h"

static Mesh* meshAlloc(int nv, int nt, bool compact) {
    if (compact && (nv <= 0 || nt <= 0 || nv > 65535 || nt > 65535)) return NULL;
    Mesh* m = (Mesh*)calloc(1, sizeof(Mesh));
    if (!m) return NULL;
    m->nv = nv;
    m->nt = nt;
    /* One allocation per mesh: this matters on the 32 MB device, where a
       dense tile is split into hundreds of small chunk meshes. */
    size_t bytes = (size_t)nv * 5 * (compact ? sizeof(int16_t) : sizeof(int)) +
                   (size_t)nt * 3 * (compact ? sizeof(uint16_t) : sizeof(int)) +
                   (size_t)nt * sizeof(int) + (size_t)nt * sizeof(int16_t) +
                   (size_t)nt * 4 * sizeof(int8_t) + 8;
    uint8_t* block = (uint8_t*)calloc(1, bytes);
    if (!block) { free(m); return NULL; }
    m->block = block;
    uint8_t* p = block;
    if (compact) {
        m->vertices16 = (int16_t*)p; p += (size_t)nv * 5 * sizeof(int16_t);
        m->indices16 = (uint16_t*)p; p += (size_t)nt * 3 * sizeof(uint16_t);
        p = (uint8_t*)(((uintptr_t)p + 3u) & ~(uintptr_t)3u);
    } else {
        m->x = (int*)p; p += (size_t)nv * sizeof(int);
        m->y = (int*)p; p += (size_t)nv * sizeof(int);
        m->z = (int*)p; p += (size_t)nv * sizeof(int);
        m->u = (int*)p; p += (size_t)nv * sizeof(int);
        m->v = (int*)p; p += (size_t)nv * sizeof(int);
        m->ti = (int*)p; p += (size_t)nt * 3 * sizeof(int);
    }
    m->col = (int*)p; p += (size_t)nt * sizeof(int);
    m->tex = (int16_t*)p; p += (size_t)nt * sizeof(int16_t);
    p = (uint8_t*)(((uintptr_t)p + 3u) & ~(uintptr_t)3u);
    m->flag = (int8_t*)p; p += (size_t)nt * sizeof(int8_t);
    m->nx = (int8_t*)p; p += (size_t)nt * sizeof(int8_t);
    m->ny = (int8_t*)p; p += (size_t)nt * sizeof(int8_t);
    m->nz = (int8_t*)p;
    return m;
}

void Mesh_bounds(Mesh* m) {
    if (!m || m->nv == 0) return;
    int x0 = Mesh_vertex(m,0,0), x1 = x0;
    int y0 = Mesh_vertex(m,0,1), y1 = y0;
    int z0 = Mesh_vertex(m,0,2), z1 = z0;
    for (int i = 1; i < m->nv; i++) {
        int x = Mesh_vertex(m,i,0), y = Mesh_vertex(m,i,1), z = Mesh_vertex(m,i,2);
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
        if (z < z0) z0 = z;
        if (z > z1) z1 = z;
    }
    m->bx = (x0 + x1) >> 1;
    m->by = (y0 + y1) >> 1;
    m->bz = (z0 + z1) >> 1;
    int dx = (x1 - x0) >> 1;
    int dy = (y1 - y0) >> 1;
    int dz = (z1 - z0) >> 1;
    uint64_t d2 = (uint64_t)((int64_t)dx * dx) + (uint64_t)((int64_t)dy * dy) + (uint64_t)((int64_t)dz * dz);
    m->br = (int)Fx_isqrt(d2) + 2;
}

void Mesh_free(Mesh* m) {
    if (!m) return;
    if (m->block) {
        free(m->block);
    } else {
        free(m->x); free(m->y); free(m->z);
        free(m->u); free(m->v);
        free(m->ti); free(m->tex); free(m->col);
        free(m->flag); free(m->nx); free(m->ny); free(m->nz);
    }
    free(m);
}

Mesh* Mesh_new(int nv,int nt) { return meshAlloc(nv,nt,false); }
Mesh* Mesh_newCompact(int nv,int nt) { return meshAlloc(nv,nt,true); }
