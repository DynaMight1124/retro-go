#ifndef GTAVC_SKY_H
#define GTAVC_SKY_H

#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>
#include "r3d.h"

struct World;

extern int* Sky_flash; extern int* Sky_smoke; extern int* Sky_fire; extern int* Sky_explo; extern int* Sky_tgt;
extern int Sky_sFlash, Sky_sSmoke, Sky_sFire, Sky_sExplo, Sky_sTgt;
extern int Sky_sunX, Sky_sunY, Sky_sunZ;

bool Sky_load(void);
void Sky_sunDir(struct World* w);
int  Sky_mix(int a, int b, int t);
void Sky_spr(int* fb, int W, int H, const int* tex, int ts, int cx, int cy, int w, int h, int col, int mode, int mul);
void Sky_flashEffect(R3D* r, int x, int y, int z);
void Sky_particle(R3D* r, int type, int x, int y, int z, int size, int fade);
void Sky_target(R3D* r, int x, int y, int z, int hp, int maxHp, int time);
void Sky_drawSky(struct World* w, R3D* r);
void Sky_drawPost(struct World* w, R3D* r);
void Sky_free(void);

#endif /* GTAVC_SKY_H */
