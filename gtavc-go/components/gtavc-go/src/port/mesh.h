#ifndef GTAVC_MESH_H
#define GTAVC_MESH_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_BODY 0x1000000
#define MESH_F_DOUBLE 1
#define MESH_F_EMISSIVE 2
#define MESH_F_WATER 4
#define MESH_F_TINT 16
#define MESH_F_GLASS 32
#define MESH_F_TINT2 64

typedef struct Mesh {
    int nv, nt;
    int *x, *y, *z, *u, *v;
    int *ti;
    int16_t *vertices16; /* cached terrain: interleaved x,y,z,u,v; wide arrays NULL */
    uint16_t *indices16;
    int16_t *tex;
    int *col;
    int8_t *flag;
    int8_t *nx, *ny, *nz;
    int bx, by, bz, br;
    void* block;      /* single allocation for all arrays (NULL for manual meshes) */
} Mesh;

Mesh* Mesh_new(int nv, int nt);
Mesh* Mesh_newCompact(int nv, int nt);
static inline int Mesh_vertex(const Mesh *m,int i,int axis) {
    if (m->vertices16) return m->vertices16[i*5+axis];
    const int *a = axis==0?m->x:axis==1?m->y:axis==2?m->z:axis==3?m->u:m->v;
    return a[i];
}
static inline int Mesh_index(const Mesh *m,int i) {
    return m->indices16?m->indices16[i]:m->ti[i];
}
void  Mesh_bounds(Mesh* m);
void  Mesh_free(Mesh* m);

#endif /* GTAVC_MESH_H */
