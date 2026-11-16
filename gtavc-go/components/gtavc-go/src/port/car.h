#ifndef GTAVC_CAR_H
#define GTAVC_CAR_H

#include <stdint.h>
#include <stdbool.h>

#define CAR_PARKED 0
#define CAR_TRAFFIC 1
#define CAR_PLAYER 2
#define CAR_POLICE 3

#define CAR_ADMIRAL 0
#define CAR_OCEANIC 1
#define CAR_TAXI 2
#define CAR_PCJ600 3
#define CAR_POLICE_CAR 4

extern const int Car_COLORS[10];

struct Ped;

typedef struct Car {
    int x, z, y, yaw, speed;
    int type, color, color2, mode, blocked;
    int steer;
    int hp;
    int partDmg[12];
    int damBits, lostBits;
    int wheelRot;
    int aiTimer, turnDir;
    int vx, vz, vy;
    int yawVel, pitch, roll, slopePitch;
    bool slide, burning;
    int burnT, fxT;
    bool braking, wrecked;
    struct Ped* driver;
} Car;

void Car_init(Car* c);

#endif /* GTAVC_CAR_H */
