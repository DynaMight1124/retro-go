#include "world.h"
#include "assets.h"
#include "anim_data.h"
#include "fx.h"
#include "sky.h"
#include "snd.h"
#include "save.h"
#include "ui.h"
#include "j2me.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world_internal.h"

int World_regionOf(Car* c, int x, int z) {
    int dx = x - c->x, dz = z - c->z;
    int lx = (Fx_cos(c->yaw) * dx - Fx_sin(c->yaw) * dz) >> 14;
    int lz = (Fx_sin(c->yaw) * dx + Fx_cos(c->yaw) * dz) >> 14;
    if (Fx_abs(lz) > Fx_abs(lx) * 2) return lz > 0 ? 0 : 1;
    return lx < 0 ? 2 : 3;
}

void World_damageCar(World* w, Car* c, int region, int amount) {
    (void)w;
    if (amount <= 0) return;
    c->hp -= amount;
    int parts[5];
    int count = 0;
    switch (region) {
        case 0:
            parts[0] = CAR_MODEL_P_BUMP_F; parts[1] = CAR_MODEL_P_BONNET; parts[2] = CAR_MODEL_P_WING_LF;
            parts[3] = CAR_MODEL_P_WING_RF; parts[4] = CAR_MODEL_P_WINDSCREEN; count = 5;
            break;
        case 1:
            parts[0] = CAR_MODEL_P_BUMP_R; parts[1] = CAR_MODEL_P_BOOT; count = 2;
            break;
        case 2:
            parts[0] = CAR_MODEL_P_DOOR_LF; parts[1] = CAR_MODEL_P_DOOR_LR; parts[2] = CAR_MODEL_P_WING_LF; count = 3;
            break;
        default:
            parts[0] = CAR_MODEL_P_DOOR_RF; parts[1] = CAR_MODEL_P_DOOR_RR; parts[2] = CAR_MODEL_P_WING_RF; count = 3;
            break;
    }
    for (int i = 0; i < count; i++) {
        int p = parts[i];
        int a = (i == 0) ? amount : (amount * 2 / 3);
        if (p == CAR_MODEL_P_WINDSCREEN) a = amount / 2;
        c->partDmg[p] += a;
        if (c->partDmg[p] > 150) c->damBits |= (1 << p);
        bool canFall = (p == CAR_MODEL_P_BUMP_F || p == CAR_MODEL_P_BUMP_R ||
                        p == CAR_MODEL_P_BONNET || p == CAR_MODEL_P_BOOT ||
                        (p >= CAR_MODEL_P_DOOR_LF && p <= CAR_MODEL_P_DOOR_RR));
        if (canFall && c->partDmg[p] > 700) c->lostBits |= (1 << p);
    }
    if (c->hp <= 0 && !c->wrecked && !c->burning) {
        c->hp = 0;
        c->burning = true;
        c->burnT = 0;
        if (c == w->inCar) World_say(w, "Get out, it's going to blow!");
    }
}

void World_spawnFx(World* w, int type, int x, int y, int z) {
    int k = -1;
    for (int i = 0; i < WORLD_NFX; i++) {
        if (w->fxL[i] <= 0) { k = i; break; }
    }
    if (k < 0) k = Fx_rndBound(WORLD_NFX);
    w->fxX[k] = x;
    w->fxY[k] = y;
    w->fxZ[k] = z;
    w->fxT[k] = type;
    w->fxM[k] = w->fxL[k] = (type == 0) ? (1800 + Fx_rndBound(600)) : ((type == 1) ? (450 + Fx_rndBound(250)) : 900);
}

void World_updateParticles(World* w, int dt) {
    for (int i = 0; i < WORLD_NFX; i++) {
        if (w->fxL[i] <= 0) continue;
        w->fxL[i] -= dt;
        int up = (w->fxT[i] == 0) ? 90 : ((w->fxT[i] == 1) ? 130 : 30);
        w->fxY[i] += up * dt / 1000;
        if (w->fxT[i] == 0) w->fxX[i] += 20 * dt / 1000;
    }
}

void World_explode(World* w, Car* c) {
    c->wrecked = true;
    c->burning = false;
    c->color = 0x2A2622;
    c->color2 = 0x2A2622;
    c->vx = c->vz = 0;
    c->speed = 0;
    c->lostBits |= (1 << CAR_MODEL_P_BONNET) | (1 << CAR_MODEL_P_DOOR_LF) | (1 << CAR_MODEL_P_BOOT);
    c->damBits = -1;
    Snd_play3d(SND_EXPLODE, c->x, c->z, w->player->x, w->player->z);
    for (int k = 0; k < 6; k++) {
        int rx = Fx_rndBound(120) - 60;
        int ry = Fx_rndBound(60);
        int rz = Fx_rndBound(120) - 60;
        World_spawnFx(w, 2, (c->x >> 4) + rx, c->y + 40 + ry, (c->z >> 4) + rz);
    }
    int d = Fx_dist(c->x - w->player->x, c->z - w->player->z);
    if (d < 30000) w->shake = 600;
    if (c == w->inCar) {
        w->health = 0;
        w->wastedT = 0;
    } else if (d < 7000 && w->inCar == NULL) {
        World_hurt(w, 110 - d / 100);
    }
    for (int i = 0; i < w->nped; i++) {
        Ped* p = &w->peds[i];
        if (p->state != PED_DEAD && p->seat == NULL && Fx_dist(p->x - c->x, p->z - c->z) < 6000) {
            World_damagePed(w, p, 200, false, c->x, c->z);
        }
    }
    for (int i = 0; i < w->ncars; i++) {
        if (&w->cars[i] != c && Fx_dist(w->cars[i].x - c->x, w->cars[i].z - c->z) < 7000) {
            World_damageCar(w, &w->cars[i], World_regionOf(&w->cars[i], c->x, c->z), 300);
        }
    }
}

void World_moveCar(World* w, Car* c, int dt) {
    int ox = c->x, oz = c->z;
    int gHere = World_groundH(w, c->x, c->z, c->y);
    if (c->y > gHere) gHere = c->y;

    c->x += c->vx * dt / 1000;
    c->z += c->vz * dt / 1000;
    CarModel* m = World_cm(w, c);
    int half = m->bike ? 500 : (m->dimZ * 16 / 2 - 900);
    int rad  = m->bike ? 380 : (m->dimX * 16 * 4 / 10);
    if (half < 200) half = 200;

    bool hit = false;
    int nx = 0, nz = 0;
    for (int k = -1; k <= 1; k++) {
        int ox2 = (Fx_sin(c->yaw) * k * half) >> 14;
        int oz2 = (Fx_cos(c->yaw) * k * half) >> 14;
        w->rx = c->x + ox2;
        w->rz = c->z + oz2;
        int bx = w->rx, bz = w->rz;
        if (World_resolve(w, rad)) {
            w->dbgHit = 10 + k;
            hit = true;
            nx += w->rx - bx;
            nz += w->rz - bz;
            c->x = w->rx - ox2;
            c->z = w->rz - oz2;
        }
        int g = World_groundH(w, c->x + ox2, c->z + oz2, gHere);
        int diff = g - gHere;
        if (diff > 72 && g > -60) {
            int gAhead = World_groundH(w, c->x + ox2 * 12 / 10, c->z + oz2 * 12 / 10, gHere);
            int diffAhead = gAhead - gHere;
            if (diffAhead > 72) {
                int surf = World_surfAt(w, c->x + ox2, c->z + oz2);
                bool isRamp = (surf == WORLD_S_ROAD) ||
                              World_isBridgeAt(w, c->x + ox2, c->z + oz2) ||
                              (diff <= 280 && gAhead >= g - 24 && surf != WORLD_S_WATER);
                if (!isRamp) {
                    w->dbgHit = 20 + k + g * 100;
                    hit = true;
                    nx += ox - c->x;
                    nz += oz - c->z;
                    c->x = ox;
                    c->z = oz;
                }
            }
        }
    }

    if (hit) {
        int nl = Fx_dist(nx, nz);
        int vn;
        if (nl > 0) {
            vn = (int)(((int64_t)c->vx * nx + (int64_t)c->vz * nz) / nl);
            if (vn < 0) {
                c->vx -= (int)(((int64_t)vn * nx * 13 / 10) / nl);
                c->vz -= (int)(((int64_t)vn * nz * 13 / 10) / nl);
            }
        } else {
            vn = -Fx_abs(c->speed);
            c->vx = -c->vx / 3;
            c->vz = -c->vz / 3;
        }
        int imp = Fx_abs(vn);
        if (imp > 3500) {
            if (c->mode == CAR_PLAYER) {
                w->shake = 300;
                World_hurt(w, imp / 6000);
            }
            World_damageCar(w, c, c->speed > 0 ? 0 : 1, imp / 140);
            if (Fx_dist(c->x - w->player->x, c->z - w->player->z) < 40000) Snd_play(SND_CRASH);
        }
        c->vx = c->vx * 85 / 100;
        c->vz = c->vz * 85 / 100;
    }

    for (int i = 0; i < w->ncars; i++) {
        Car* o = &w->cars[i];
        if (o == c) continue;
        int ddx = o->x - c->x, ddz = o->z - c->z;
        if (ddx > 7000 || ddx < -7000 || ddz > 7000 || ddz < -7000) continue;
        CarModel* om = World_cm(w, o);
        int rr = (m->bike ? 900 : 1500) + (om->bike ? 900 : 1500);
        int d = Fx_dist(ddx, ddz);
        if (d < rr && d > 0) {
            int push = (rr - d) / 2;
            o->x += ddx * push / d; o->z += ddz * push / d;
            c->x -= ddx * push / d; c->z -= ddz * push / d;
            int rvx = c->vx - o->vx, rvz = c->vz - o->vz;
            int vn = (int)(((int64_t)rvx * ddx + (int64_t)rvz * ddz) / d);
            if (vn > 0) {
                int mc = m->mass, mo = om->mass;
                int jc = (int)(((int64_t)vn * mo * 16 / 10) / (mc + mo));
                int jo = (int)(((int64_t)vn * mc * 16 / 10) / (mc + mo));
                c->vx -= jc * ddx / d; c->vz -= jc * ddz / d;
                o->vx += jo * ddx / d; o->vz += jo * ddz / d;
                if (vn > 3000) {
                    if (c->mode == CAR_PLAYER || o->mode == CAR_PLAYER) {
                        w->shake = 250;
                        Snd_play(SND_CRASH);
                    }
                    World_damageCar(w, c, World_regionOf(c, o->x, o->z), vn / 200);
                    World_damageCar(w, o, World_regionOf(o, c->x, c->z), vn / 200);
                    if ((o->mode == CAR_POLICE || (o->driver != NULL && o->driver->cop)) && c->mode == CAR_PLAYER) {
                        World_raiseWanted(w, 1);
                    }
                }
            }
        }
    }

    int g = World_groundH(w, c->x, c->z, c->y);
    if (g < c->y - 8) {
        c->vy -= 9800 * 64 / 1024 * dt / 1000;
        c->y += c->vy * dt / 1000;
        if (c->y < g) { c->y = g; c->vy = 0; }
    } else {
        if (g > c->y) c->y = g;
        else {
            int stepDt = dt < 100 ? dt : 100;
            c->y += (g - c->y) * stepDt / 40;
        }
        c->vy = 0;
    }
    c->wheelRot = (c->wheelRot + c->speed * dt / 1000 * 1024 / 2200) & 1023;

    if (Fx_abs(c->speed) > 2500) {
        for (int i = 0; i < w->nped; i++) {
            Ped* p = &w->peds[i];
            if (p->state == PED_DEAD || p->seat != NULL || p->state == PED_JACKED) continue;
            int ddx = p->x - c->x, ddz = p->z - c->z;
            if (ddx > 2800 || ddx < -2800 || ddz > 2800 || ddz < -2800) continue;
            if (Fx_dist(ddx, ddz) < (m->bike ? 1300 : 2000)) {
                World_damagePed(w, p, Fx_abs(c->speed) / 140, c->mode == CAR_PLAYER, c->x, c->z);
                p->x += (Fx_sin(c->yaw) * 1500) >> 14;
                p->z += (Fx_cos(c->yaw) * 1500) >> 14;
                if (c->mode == CAR_PLAYER) {
                    w->shake = 150;
                    Snd_play(SND_HITPED);
                }
            }
        }
        if (c->mode != CAR_PLAYER && w->inCar == NULL && w->pAct == 0) {
            int ddx = w->player->x - c->x, ddz = w->player->z - c->z;
            if (Fx_dist(ddx, ddz) < 1900) {
                World_hurt(w, Fx_abs(c->speed) / 1024 * 3);
                c->vx /= 3;
                c->vz /= 3;
            }
        }
    }
}

void World_driveCar(World* w, Car* c, int dt, int acc, int st, bool hand) {
    CarModel* m = World_cm(w, c);
    int fx = Fx_sin(c->yaw), fz = Fx_cos(c->yaw);
    int vF = (int)(((int64_t)c->vx * fx + (int64_t)c->vz * fz) >> 14);
    int vR = (int)(((int64_t)c->vx * fz - (int64_t)c->vz * fx) >> 14);
    int vmax = m->vmax * 1024, ac = m->accel * 1024 / 100, br = m->brake * 1024 / 100;
    int a = 0;
    c->braking = false;
    if (acc > 0) {
        if (vF < -400) { a = br; c->braking = true; }
        else if (vF < vmax) a = (int)((int64_t)ac * (vmax - vF + vmax / 5) / vmax);
    } else if (acc < 0) {
        if (vF > 400) { a = -br; c->braking = true; }
        else if (vF > -vmax / 3) a = -ac / 2;
    }
    int drag = 150 + Fx_abs(vF) / 16 + (int)((int64_t)vF * vF / 2600000);
    if (acc == 0) drag += 800;
    if (hand) drag += br / 2;
    int stopA = (int)((int64_t)Fx_abs(vF) * 1000 / (dt > 1 ? dt : 1));
    if (drag > stopA) drag = stopA;
    a += vF > 0 ? -drag : (vF < 0 ? drag : 0);

    int hf = World_groundH(w, c->x + ((fx * 1600) >> 14), c->z + ((fz * 1600) >> 14), c->y);
    int hb = World_groundH(w, c->x - ((fx * 1600) >> 14), c->z - ((fz * 1600) >> 14), c->y);
    int slope = (hf - hb) * 16 * 1024 / 3200;
    if (Fx_abs(slope) < 700) a -= slope * 9800 / 1024;
    else slope = 0;

    int s = World_surfAt(w, c->x, c->z);
    if ((s == WORLD_S_SAND || s == WORLD_S_GRASS) && vF > 6000) a -= vF / 6;
    vF += a * dt / 1000;
    if (acc == 0 && Fx_abs(vF) < 150) vF = 0;

    int asp = Fx_abs(vF);
    int lock = m->lockDeg * 1024 / 360;
    int k = (asp < 45000 ? asp : 45000) >> 6, kv = 15000 >> 6;
    int lockEff = lock * 256 / (256 + k * k * 256 / (kv * kv));
    int target = st * lockEff;
    int rate = (st == 0 ? 900 : 480) * dt / 1000 + 1;
    c->steer += Fx_clamp(target - c->steer, -rate, rate);
    int cs = Fx_cos(c->steer);
    int tan14 = (Fx_sin(c->steer) << 14) / (cs < 1000 ? 1000 : cs);
    int want = (int)(((int64_t)vF * tan14 / m->wheelbase * 163) >> 14);
    if (hand && asp > 4000) want = want * 16 / 10;
    int yawGrip = (hand ? 3 : 9) * m->grip / 100;
    int stepGrip = (yawGrip * dt < 1000) ? (yawGrip * dt) : 1000;
    c->yawVel += (want - c->yawVel) * stepGrip / 1000;
    c->yaw = (c->yaw + c->yawVel * dt / 1000) & 1023;

    int grip = m->grip * (hand ? 3 : 12);
    if (Fx_abs(vR) > 7000 && !hand) grip = grip * 2 / 3;
    int dR = grip * dt;
    if (vR > dR) vR -= dR; else if (vR < -dR) vR += dR; else vR = 0;
    c->slide = Fx_abs(vR) > 2500 || (hand && asp > 6000);

    fx = Fx_sin(c->yaw); fz = Fx_cos(c->yaw);
    c->vx = (int)(((int64_t)vF * fx + (int64_t)vR * fz) >> 14);
    c->vz = (int)(((int64_t)vF * fz - (int64_t)vR * fx) >> 14);
    c->speed = vF;

    int pitchT = -a / 900;
    int rollT = (int)((int64_t)c->yawVel * vF / 260000);
    int stepDt = dt < 100 ? dt : 100;
    c->pitch += (Fx_clamp(pitchT, -9, 9) - c->pitch) * stepDt / 140;
    c->roll += (Fx_clamp(rollT, -10, 10) - c->roll) * stepDt / 140;
    c->slopePitch += ((slope * 163 >> 10) - c->slopePitch) * stepDt / 100;
    World_moveCar(w, c, dt);
}

void World_damagePed(World* w, Ped* o, int dmg, bool byPlayer, int ax, int az) {
    if (o->state == PED_DEAD) return;
    o->hp -= dmg;
    w->pedsHit++;
    bool front = ((ax - o->x) >> 4) * (Fx_sin(o->yaw) >> 4) + ((az - o->z) >> 4) * (Fx_cos(o->yaw) >> 4) > 0;
    bool witnessed = byPlayer && World_copsNear(w, 30 * 1024) > 0;
    if (o->hp <= 0) {
        o->state = PED_DEAD;
        o->timer = 14000;
        o->deadT = 0;
        o->angry = false;
        o->aimT = 0;
        World_voice(w, SND_V_DEATH, o);
        int back = front ? -700 : 700;
        o->bloodX = o->x + ((Fx_sin(o->yaw) * back) >> 14);
        o->bloodZ = o->z + ((Fx_cos(o->yaw) * back) >> 14);
        o->bloodY = World_groundH(w, o->bloodX, o->bloodZ, o->y);
        World_startClip(o, front ? w->cKoF : w->cKoB, -2, 0);
        o->cash = 0;
        if (o->cop) {
            World_dropPickup(w, 2, 0, o->bloodX, o->bloodZ, o->y);
        } else if (Fx_rndBound(100) < 55) {
            World_dropPickup(w, 3, 5 + Fx_rndBound(60) + (o->gang ? 40 : 0),
                       o->x + ((Fx_sin(o->yaw) * 400) >> 14), o->z + ((Fx_cos(o->yaw) * 400) >> 14), o->y);
        }
        if (byPlayer) {
            w->kills++;
            if (o->cop) World_raiseWanted(w, w->wanted >= 1 ? 2 : 1);
            else if (witnessed) World_raiseWanted(w, 1);
            else if (w->wanted == 0 && ++w->quietKills >= w->quietLimit) {
                World_raiseWanted(w, 1);
                w->quietKills = 0;
                w->quietLimit = 3 + Fx_rndBound(3);
            }
        }
    } else {
        o->state = PED_HIT;
        World_startClip(o, front ? w->cHitF : w->cHitB, -1, 0);
        World_voice(w, o->brave ? SND_V_FIGHT : SND_V_PANIC, o);
        if (byPlayer) {
            if (o->cop || witnessed) World_raiseWanted(w, 1);
            if (o->brave && !o->cop) o->angry = true;
        }
    }
}

void World_hurt(World* w, int dmg) {
    if (w->health <= 0 || dmg <= 0 || w->god) return;
    if (w->armor > 0) {
        w->armor -= dmg;
        if (w->armor < 0) {
            w->health += w->armor;
            w->armor = 0;
        }
    } else {
        w->health -= dmg;
    }
    if (w->health <= 0) {
        w->health = 0;
        w->wastedT = 0;
    }
    w->shake = 250;
}
