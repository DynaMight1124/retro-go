#ifndef GTAVC_PED_MODEL_H
#define GTAVC_PED_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>
#include "mesh.h"
#include "r3d.h"

#define PED_MODEL_TORSO 0
#define PED_MODEL_ARM_A 1
#define PED_MODEL_FORE_A 2
#define PED_MODEL_ARM_B 3
#define PED_MODEL_FORE_B 4
#define PED_MODEL_THIGH_A 5
#define PED_MODEL_SHIN_A 6
#define PED_MODEL_THIGH_B 7
#define PED_MODEL_SHIN_B 8

typedef struct PedModel {
    int n, rootH;
    int* jx;
    int* jy;
    int* jz;
    Mesh** part;

    /* scratch */
    int rYaw[9], rt[9], rb[9], rtor[9], rbase[9];
    int rp[9][9];
    int px[9], py[9], pz[9];
    int hx, hy, hz;
    int a[9], b[9], mm[9], tmp[9];
} PedModel;

PedModel* PedModel_load(const char* res);
void      PedModel_hand(PedModel* m, int o[3]);
void      PedModel_draw(PedModel* m, R3D* r, int fx, int fy, int fz, int yaw, int tilt, int lift, const int* ang, int tex);
void      PedModel_drawPose(PedModel* m, R3D* r, int fx, int fy, int fz, int yaw, const int* pose, int tex);
void      PedModel_free(PedModel* m);

#endif /* GTAVC_PED_MODEL_H */
