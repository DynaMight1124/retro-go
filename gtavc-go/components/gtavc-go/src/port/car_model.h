#ifndef GTAVC_CAR_MODEL_H
#define GTAVC_CAR_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include "mesh.h"

#define CAR_MODEL_P_CHASSIS 0
#define CAR_MODEL_P_BONNET 1
#define CAR_MODEL_P_BOOT 2
#define CAR_MODEL_P_DOOR_LF 3
#define CAR_MODEL_P_DOOR_RF 4
#define CAR_MODEL_P_DOOR_LR 5
#define CAR_MODEL_P_DOOR_RR 6
#define CAR_MODEL_P_WING_LF 7
#define CAR_MODEL_P_WING_RF 8
#define CAR_MODEL_P_BUMP_F 9
#define CAR_MODEL_P_BUMP_R 10
#define CAR_MODEL_P_WINDSCREEN 11

typedef struct CarModel {
    int np;
    int* partId;
    Mesh*** ok;      /* [part][0..1] -> body / glass */
    Mesh*** dam;     /* [part][0..1] -> body / glass */
    Mesh* farMesh;
    Mesh* farGlass;
    int wx[4], wy[4], wz[4], wr[4];
    int nw, dimX, dimY, dimZ, seatX, seatY, seatZ;
    bool bike;
    char* name;
    int* tex;
    int texCount;
    int mass, grip, cls;
    int vmax, accel, brake, lockDeg, wheelbase;
} CarModel;

extern Mesh* CarModel_wheel;

CarModel* CarModel_load(const char* name);
void      CarModel_unload(CarModel* c);
void      CarModel_free(CarModel* c);
void      CarModel_normal(Mesh* m, int k, int a, int b, int c);

#endif /* GTAVC_CAR_MODEL_H */
