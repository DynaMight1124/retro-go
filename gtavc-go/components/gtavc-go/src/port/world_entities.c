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

CarModel* World_cm(World* w, Car* c) {
    return w->cpool[c->type];
}

int World_vmaxOf(World* w, Car* c) {
    return w->cpool[c->type]->vmax * 1024;
}

void World_addPickup(World* w, int type, int fwd, int side) {
    int fx = Fx_sin(w->player->yaw), fz = Fx_cos(w->player->yaw);
    w->rx = w->player->x + ((fx * fwd + fz * side) >> 14);
    w->rz = w->player->z + ((fz * fwd - fx * side) >> 14);
    if (!World_walkable(w, w->rx, w->rz, w->player->y)) {
        w->rx = w->player->x + ((fx * fwd) >> 14);
        w->rz = w->player->z + ((fz * fwd) >> 14);
    }
    w->pkX[w->npick] = w->rx;
    w->pkZ[w->npick] = w->rz;
    w->pkY[w->npick] = World_groundH(w, w->rx, w->rz, w->player->y);
    w->pkType[w->npick] = type;
    w->pkT[w->npick] = 0;
    w->pkLife[w->npick] = 0;
    w->npick++;
}

void World_dropPickup(World* w, int type, int amt, int x, int z, int y) {
    int k = -1;
    for (int i = 0; i < w->npick; i++) {
        if (w->pkLife[i] < 0) { k = i; break; }
    }
    if (k < 0) {
        if (w->npick >= WORLD_NPK) return;
        k = w->npick++;
    }
    w->pkX[k] = x;
    w->pkZ[k] = z;
    w->pkY[k] = World_groundH(w, x, z, y);
    w->pkType[k] = type;
    w->pkAmt[k] = amt;
    w->pkT[k] = 0;
    w->pkLife[k] = 30000;
}

void World_placePlayerAtSpawn(World* w) {
    w->player->x = w->spX;
    w->player->z = w->spZ;
    w->player->yaw = w->spYaw;
    w->player->y = w->spY;
    w->player->state = PED_WALK;
    w->player->hp = 100;
    w->player->clip = -1;
    w->player->seat = NULL;
    w->player->speed = 0;
}

Car* World_addCar(World* w, int mode, int slot, int xm, int zm, int yaw, int colorIdx) {
    if (w->ncars >= 16 || w->cpool[slot] == NULL) return NULL;
    Car* c = &w->cars[w->ncars++];
    Car_init(c);
    c->mode = mode;
    c->type = slot;
    c->x = xm;
    c->z = zm;
    c->yaw = yaw & 1023;
    c->y = World_groundH(w, xm, zm, w->player->y);
    c->color = Car_COLORS[colorIdx % 10];
    c->color2 = Car_COLORS[(colorIdx + 3) % 10];
    const char* n = w->cpool[slot]->name;
    if (strcmp(n, "taxi") == 0 || strcmp(n, "cabbie") == 0 || strcmp(n, "zebra") == 0) {
        c->color = 0xF0C820; c->color2 = 0xF0C820;
    }
    if (strcmp(n, "police") == 0) {
        c->color = 0xF4F4F4; c->color2 = 0x202430;
    }
    return c;
}

void World_removeCar(World* w, int i) {
    Car* c = &w->cars[i];
    if (c->driver != NULL) {
        c->driver->seat = NULL;
        c->driver->state = PED_DEAD;
        c->driver->timer = 0;
        c->driver->hp = 0;
        c->driver->deadT = -1;
        c->driver->cop = false;
        c->driver = NULL;
    }
    Car* last = &w->cars[w->ncars - 1];
    if (c != last) {
        /* Java moves the object reference; in C the struct is copied, so every
           reference to the last slot has to be retargeted. */
        *c = *last;
        for (int k = 0; k < w->nped; k++) {
            if (w->peds[k].seat == last) w->peds[k].seat = c;
        }
        if (w->inCar == last) w->inCar = c;
        if (w->pActCar == last) w->pActCar = c;
        if (c->driver != NULL && c->driver->seat != c) c->driver->seat = c;
    }
    w->ncars--;
}

void World_addDriver(World* w, Car* c, bool cop) {
    Ped* d = NULL;
    for (int i = 0; i < w->nped; i++) {
        if (w->peds[i].state == PED_DEAD && w->peds[i].deadT < 0 && w->peds[i].seat == NULL) {
            d = &w->peds[i];
            break;
        }
    }
    if (d == NULL) {
        if (w->nped >= 40) return;
        d = &w->peds[w->nped++];
        Ped_init(d);
    }
    d->seat = c;
    c->driver = d;
    d->cop = cop;
    World_assignKind(w, d, cop ? w->copSlot : (1 + Fx_rndBound(w->npool - 1)));
    d->state = PED_IDLE;
    d->loco = w->cIdle;
    d->hp = 100;
    d->clip = -1;
    d->deadT = 0;
    d->angry = false;
}

bool World_findSpot(World* w, int d0, int d1, int want) {
    int py = w->player->y;
    int clearRad = (want == WORLD_S_ROAD) ? 1300 : 500;
    for (int k = 0; k < 24; k++) {
        int a = Fx_rndBound(1024);
        int d = (d0 + Fx_rndBound(d1 - d0 + 1)) * 1024;
        int x = w->player->x + ((Fx_sin(a) * (d >> 4)) >> 10);
        int z = w->player->z + ((Fx_cos(a) * (d >> 4)) >> 10);
        int s = World_surfAt(w, x, z);
        bool ok;
        if (want == WORLD_S_ROAD) {
            ok = (s == WORLD_S_ROAD && !World_deep(w, x, z, py) &&
                  World_spawnClear(w, x, z, clearRad));
        } else {
            ok = (s != WORLD_S_WATER && s != WORLD_S_ROAD &&
                  World_walkable(w, x, z, py) &&
                  World_spawnClear(w, x, z, clearRad));
        }
        if (ok) {
            w->spot[0] = x;
            w->spot[1] = z;
            return true;
        }
    }
    return false;
}

int World_roadYaw(World* w, int x, int z) {
    int best = 0, bl = -1;
    for (int a = 0; a < 1024; a += 64) {
        int l = 0;
        for (int s = 2; s <= 24; s += 2) {
            if (World_surfAt(w, x + ((Fx_sin(a) * s) >> 4), z + ((Fx_cos(a) * s) >> 4)) != WORLD_S_ROAD) break;
            l++;
        }
        if (l > bl) {
            bl = l;
            best = a;
        }
    }
    return best;
}

void World_spawnTraffic(World* w, bool initial) {
    if (!World_findSpot(w, initial ? 25 : 80, initial ? 110 : 130, WORLD_S_ROAD)) return;
    int slot = 1 + Fx_rndBound(w->ncpool - 1);          /* Java argument order */
    int yaw = World_roadYaw(w, w->spot[0], w->spot[1]);
    int color = Fx_rndBound(10);
    Car* c = World_addCar(w, CAR_TRAFFIC, slot, w->spot[0], w->spot[1], yaw, color);
    if (!c) return;
    c->vx = (Fx_sin(c->yaw) * 6) >> 4;
    c->vz = (Fx_cos(c->yaw) * 6) >> 4;
    c->speed = 6 * 1024;
    World_addDriver(w, c, false);
}

void World_spawnPed(World* w, bool initial) {
    if (!World_findSpot(w, initial ? 8 : 50, initial ? 75 : 85, WORLD_S_PAVE)) return;
    Ped* p = NULL;
    for (int i = 0; i < w->nped; i++) {
        if (w->peds[i].state == PED_DEAD && w->peds[i].timer <= 0 && !w->peds[i].cop && w->peds[i].seat == NULL) {
            p = &w->peds[i];
            break;
        }
    }
    if (p == NULL) {
        if (w->nped >= 40) return;
        p = &w->peds[w->nped++];
        Ped_init(p);
    }
    p->x = w->spot[0];
    p->z = w->spot[1];
    p->state = PED_WALK;
    p->hp = 100;
    p->cash = 0;
    p->cop = false;
    p->clip = -1;
    p->clipNext = -1;
    p->seat = NULL;
    p->deadT = 0;
    p->angry = false;
    p->aimT = 0;
    p->yaw = Fx_rndBound(1024);
    p->targetYaw = p->yaw;
    p->speed = 1150 + Fx_rndBound(350);
    int kind = (w->npool > 1) ? 1 + Fx_rndBound(w->npool - 1) : 0;
    World_assignKind(w, p, kind);
    p->brave = p->gang || (!p->female && Fx_rndBound(10) < 3);
    p->turnTimer = 1500 + Fx_rndBound(4000);
    p->locoT = Fx_rndBound(1000);
    p->y = World_groundH(w, p->x, p->z, w->player->y);
    p->timer = 0;
}

Ped* World_spawnCopAt(World* w, int x, int z) {
    Ped* p = NULL;
    for (int i = 0; i < w->nped; i++) {
        if (w->peds[i].state == PED_DEAD && w->peds[i].timer <= 0 && w->peds[i].seat == NULL) {
            p = &w->peds[i];
            break;
        }
    }
    if (p == NULL) {
        if (w->nped >= 40) return NULL;
        p = &w->peds[w->nped++];
        Ped_init(p);
    }
    p->x = x; p->z = z; p->cop = true;
    World_assignKind(w, p, w->copSlot);
    p->hp = 160; p->state = PED_WALK; p->speed = 0; p->clip = -1; p->seat = NULL;
    p->deadT = 0; p->cash = 0; p->closeT = 0; p->attackCd = 1500;
    p->y = World_groundH(w, x, z, w->player->y);
    p->yaw = Fx_yawOf(w->player->x - x, w->player->z - z);
    p->angry = false; p->aimT = 0;
    return p;
}

void World_spawnCop(World* w) {
    if (World_findSpot(w, 28, 42, WORLD_S_PAVE)) {
        World_spawnCopAt(w, w->spot[0], w->spot[1]);
    }
}

void World_spawnPoliceCar(World* w) {
    if (!World_findSpot(w, 60, 90, WORLD_S_ROAD)) return;
    Car* c = World_addCar(w, CAR_POLICE, w->policeSlot, w->spot[0], w->spot[1], Fx_yawOf(w->player->x - w->spot[0], w->player->z - w->spot[1]), 0);
    if (c) World_addDriver(w, c, true);
}

int World_copsNear(World* w, int rangeMm) {
    int n = 0;
    for (int i = 0; i < w->nped; i++) {
        Ped* p = &w->peds[i];
        if (!p->cop || p->state == PED_DEAD) continue;
        int x = (p->seat != NULL) ? p->seat->x : p->x;
        int z = (p->seat != NULL) ? p->seat->z : p->z;
        if (Fx_dist(x - w->player->x, z - w->player->z) < rangeMm) n++;
    }
    return n;
}

void World_raiseWanted(World* w, int lvl) {
    if (w->wanted < lvl) {
        w->wanted = lvl;
        w->wantedCalmT = 0;
    }
}

void World_say(World* w, const char* s) {
    if (!s) return;
    snprintf(w->help, sizeof(w->help), "%s", s);
    w->helpT = 5000;
}

void World_startClip(Ped* p, int clip, int next, int endMs) {
    p->clip = clip;
    p->clipT = 0;
    p->clipNext = next;
    p->clipEnd = endMs;
    p->hitDone = false;
}

static int clipLen(Ped* p) {
    int l = AnimData_len(p->clip);
    return (p->clipEnd > 0 && p->clipEnd < l) ? p->clipEnd : l;
}

bool World_stepClip(World* w, Ped* p, int dt) {
    if (p->clip < 0) return false;
    p->clipT += dt;
    int len = clipLen(p);
    if (p->clipT < len) return false;
    if (p->clipNext == -2) {
        p->clipT = len;
        return false;
    }
    if (p->clip == w->cGetout || p->clip == w->cGetin || p->clip == w->cPull || p->clip == w->cJacked || p->clip == w->cOpen) {
        AnimData_sample(p->clip, len, w->pose2);
        int offR = w->pose2[90] * 16, offF = w->pose2[92] * 16;
        int c = Fx_cos(p->yaw), s = Fx_sin(p->yaw);
        p->x += (offR * c + offF * s) >> 14;
        p->z += (-offR * s + offF * c) >> 14;
    }
    p->clip = p->clipNext;
    p->clipNext = -1;
    p->clipT = 0;
    p->clipEnd = 0;
    p->hitDone = false;
    return true;
}

void World_selectLoco(World* w, Ped* p, int dt, int speed) {
    int sp = Fx_abs(speed);
    int c;
    bool f = p->female;
    if (sp < 150) {
        c = f ? w->cWomanIdle : w->cIdle;
        p->locoT += dt;
    } else {
        if (speed < 0) c = w->cWalkB;
        else if (sp < 3000) c = (p == w->player) ? w->cWalk : (f ? w->cWomanWalk : w->cCiviWalk);
        else if (sp > 6000 && p != w->player) c = w->cSprint;
        else c = (p == w->player || p->cop) ? w->cRun : (f ? w->cWomanRun : w->cCiviRun);
        int ratio = sp * 256 / AnimData_speed(c);
        if (ratio < 90) ratio = 90;
        if (ratio > 560) ratio = 560;
        p->locoT += (dt * ratio) >> 8;
    }
    p->loco = c;
}

void World_reset(World* w) {
    const char* seedEnv = getenv("GTAVC_SEED");
    Fx_seed = (seedEnv && *seedEnv) ? atoi(seedEnv) : (int)System_currentTimeMillis();
    World_initClips(w);
    Ped_init(w->player);
    World_placePlayerAtSpawn(w);
    w->player->tex = w->playerTex;
    w->player->loco = w->cIdle;

    w->inCar = NULL;
    w->pActCar = NULL;
    w->pAct = 0;
    w->ncars = 0;
    w->nped = 0;
    w->health = 100;
    w->armor = 0;
    w->money = 0;
    w->wanted = 0;
    w->pedsHit = 0;
    w->carsStolen = 0;
    w->kills = 0;
    w->quietKills = 0;
    w->quietLimit = 3 + Fx_rndBound(3);
    w->missionDone = false;
    w->minutes = 22 * 60;
    w->wastedT = 0;
    w->helpShown = 0;
    w->helpT = 0;
    w->busted = false;
    w->bustedT = 0;
    w->drownT = 0;
    w->runMode = true;   /* continuous autorun on the console */
    w->weapon = 0;
    w->hasBat = false;
    w->hasPistol = false;
    w->ammo = 0;
    w->camYawOff = 0;
    w->camYaw = w->player->yaw;
    w->camPitch = 55;

    World_preload(w, w->player->x, w->player->z);

    int adm = World_pools_findStr(w->carNames, w->carNamesCount, "admiral");
    int slot = -1;
    for (int i = 0; i < w->ncpool; i++) {
        if (w->cpoolSrc[i] == adm && w->cpool[i] != NULL) slot = i;
    }
    if (slot >= 0) {
        int fx = Fx_sin(w->player->yaw), fz = Fx_cos(w->player->yaw);
        int off[10] = { 3000, 2600, 3000, -2600, 6000, 2600, 6000, -2600, 4000, 0 };
        for (int k = 0; k < 10; k += 2) {
            int x = w->player->x + ((fx * off[k] + fz * off[k + 1]) >> 14);
            int z = w->player->z + ((fz * off[k] - fx * off[k + 1]) >> 14);
            if (!World_walkable(w, x, z, w->player->y) ||
                !World_spawnClear(w, x, z, 1300)) continue;
            Car* c = World_addCar(w, CAR_PARKED, slot, x, z, w->player->yaw, 4);
            if (c) {
                c->color = 0xF2F2F2;
                c->color2 = 0xF2F2F2;
                c->y = World_groundH(w, x, z, w->player->y);
            }
            break;
        }
    }

    w->npick = 0;
    World_addPickup(w, 2, 2600, 1200);
    World_addPickup(w, 1, -2200, 1800);

    for (int i = 0; i < w->maxTraffic - 1; i++) World_spawnTraffic(w, true);
    for (int i = 0; i < w->maxPed - 4; i++) World_spawnPed(w, true);

    World_say(w, "Welcome to Vice City.   A: run/sprint  B: attack  X: jump  Y: enter/exit");
    World_missionReset(w);
}

void World_continueAt(World* w, int x, int z, int y, int yaw) {
    World_tileJobCancel(w);
    int totalTiles = w->tnx * w->tnz;
    for (int k = 0; k < totalTiles; k++) {
        if (w->tiles[k] != NULL) {
            Tile_unload(w->tiles[k]);
            Tile_free(w->tiles[k]);
            w->tiles[k] = NULL;
        }
    }
    w->player->x = x;
    w->player->z = z;
    w->player->yaw = yaw & 1023;
    World_preload(w, x, z);
    int g = World_groundH(w, x, z, y);
    w->player->y = g > -70 ? g : y;
    w->camYaw = w->player->yaw;
    w->ncars = 0;
    w->nped = 0;
    w->npick = 0;
    for (int i = 0; i < w->maxTraffic - 1; i++) World_spawnTraffic(w, true);
    for (int i = 0; i < w->maxPed - 4; i++) World_spawnPed(w, true);
    w->weapon = (w->hasPistol && w->ammo > 0) ? 2 : (w->hasBat ? 1 : 0);
    World_say(w, "Welcome back.");
    World_missionReset(w);
}

void World_clearCops(World* w) {
    for (int i = w->nped - 1; i >= 0; i--) {
        if (w->peds[i].cop && w->peds[i].seat == NULL) {
            Ped* removed = &w->peds[i];
            Ped* last = &w->peds[w->nped - 1];
            if (removed != last) {
                *removed = *last;
                for (int k = 0; k < w->ncars; k++) {
                    if (w->cars[k].driver == last) w->cars[k].driver = removed;
                }
                if (w->target == last) w->target = removed;
            }
            w->nped--;
        }
    }
    for (int i = w->ncars - 1; i >= 0; i--) {
        if (w->cars[i].mode == CAR_POLICE) World_removeCar(w, i);
    }
}

void World_respawn(World* w) {
    w->wastedT = 0;
    w->health = 100;
    w->armor = 0;
    w->money = w->money > 100 ? w->money - 100 : 0;
    w->wanted = 0;
    w->quietKills = 0;
    w->weapon = 0;
    w->hasPistol = false;
    w->hasBat = false;
    w->ammo = 0;
    World_placePlayerAtSpawn(w);
    World_preload(w, w->player->x, w->player->z);
    World_clearCops(w);
    World_say(w, "You have been treated. $100 fee.");
}

void World_arrestRespawn(World* w) {
    w->money = w->money > 100 ? w->money - 100 : 0;
    w->wanted = 0;
    w->quietKills = 0;
    w->weapon = 0;
    w->hasPistol = false;
    w->hasBat = false;
    w->ammo = 0;
    World_placePlayerAtSpawn(w);
    World_preload(w, w->player->x, w->player->z);
    World_clearCops(w);
    World_say(w, "Busted! $100 bail. Weapons confiscated.");
}

int World_localX(Car* c, int lx, int lz) {
    return c->x + (((Fx_cos(c->yaw) * lx + Fx_sin(c->yaw) * lz) >> 14) * 16);
}

int World_localZ(Car* c, int lx, int lz) {
    return c->z + (((-Fx_sin(c->yaw) * lx + Fx_cos(c->yaw) * lz) >> 14) * 16);
}

int World_seatLX(World* w, Car* c) {
    CarModel* m = World_cm(w, c);
    return m->bike ? 0 : -Fx_abs(m->seatX);
}

int World_seatLZ(World* w, Car* c) {
    return World_cm(w, c)->seatZ;
}
