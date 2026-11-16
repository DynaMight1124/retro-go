#ifndef GTAVC_R3D_H
#define GTAVC_R3D_H

#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>
#include "mesh.h"

#define R3D_NEAR 64
#define R3D_MAX_TEX 4096

typedef struct R3D {
    int W, H, cx, cy, focal;
    int* fb;
    int* zb;

    /* textures (ARGB, 0 = transparent) */
    int* T[R3D_MAX_TEX];
    int  uvs[R3D_MAX_TEX];
    int  tws[R3D_MAX_TEX], twm[R3D_MAX_TEX], thm[R3D_MAX_TEX], tpu[R3D_MAX_TEX], tpv[R3D_MAX_TEX];

    /* camera */
    int camX, camY, camZ;
    int recipF;
    int mv[9];
    int horizon;

    /* lighting / fog */
    int lx, ly, lz;
    int amb, dif;
    int fogR, fogG, fogB, fogNear, fogFar;
    int farZ;
    int skyTop, skyBot, groundCol;
    int emis;
    int animU, animV;
    int forceTex;
    bool blend;
    bool additive;
    bool night;
    int tint2;
    int tintC;

    /* scratch buffers */
    int vxCap;
    int *vx, *vy, *vz, *vsx, *vsy, *viz;
    int px[3], py[3], piz[3], pu[3], pv[3];
    float qx[10], qy[10], qz[10], qu[10], qv[10];
    float rx[10], ry[10], rz[10], ru[10], rv[10];
    int radX, radY;
    int tris;
    int verts;

} R3D;

void R3D_init(R3D* r);
void R3D_resize(R3D* r, int w, int h, int fovFocal);
void R3D_setTexture(R3D* r, int id, int* pix, int w, int h);
void R3D_setCamera(R3D* r, int x, int y, int z, int yaw, int pitch);
void R3D_clear(R3D* r);
void R3D_drawMesh(R3D* r, Mesh* m, const int* mm, int tx, int ty, int tz, int lmx, int lmy, int lmz, int body);
int  R3D_shadePx(int c, int s, int add);
void R3D_free(R3D* r);

#endif /* GTAVC_R3D_H */
