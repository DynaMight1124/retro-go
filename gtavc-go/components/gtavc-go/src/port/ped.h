#ifndef GTAVC_PED_H
#define GTAVC_PED_H

#include <stdint.h>
#include <stdbool.h>

#define PED_WALK 0
#define PED_IDLE 1
#define PED_FLEE 2
#define PED_DEAD 3
#define PED_HIT 4
#define PED_JACKED 5
#define PED_GETUP 6

struct Car;

typedef struct Ped {
    int x, z, y, yaw;
    int state, hp, tex, timer, cash;
    int speed;
    int turnTimer, targetYaw;
    bool cop, female;
    int kind;
    int clip, clipT, clipNext, clipEnd;
    int loco, locoT;
    bool hitDone;
    struct Car* seat;
    int attackCd;
    int deadT;
    int bloodX, bloodZ, bloodY;
    int closeT;
    bool angry, brave, gang;
    int aimT;
    bool air;
    int vy;
} Ped;

void Ped_init(Ped* p);

#endif /* GTAVC_PED_H */
