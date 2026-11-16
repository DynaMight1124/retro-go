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

void World_debugSpawn(World* w, int src) {
    int slot = -1;
    for (int s2 = w->ncpool - 1; s2 >= 1 && slot < 0; s2--) {
        bool used = false;
        for (int i = 0; i < w->ncars; i++) {
            if (w->cars[i].type == s2) used = true;
        }
        if (!used) slot = s2;
    }
    if (slot < 0) {
        for (int i = w->ncars - 1; i >= 0; i--) {
            if (&w->cars[i] != w->inCar && w->cars[i].type != 0) {
                slot = w->cars[i].type;
                break;
            }
        }
    }
    if (slot < 0) return;
    for (int i = w->ncars - 1; i >= 0; i--) {
        if (w->cars[i].type == slot && &w->cars[i] != w->inCar) World_removeCar(w, i);
    }
    World_pools_loadCar(w, slot, src);
    int fx = Fx_sin(w->player->yaw), fz = Fx_cos(w->player->yaw);
    Car* c = World_addCar(w, CAR_PARKED, slot, w->player->x + ((fx * 5000) >> 14), w->player->z + ((fz * 5000) >> 14), w->player->yaw + 256, Fx_rndBound(10));
    if (c) World_say(w, w->carNames[src]);
}

void World_lockOn(World* w) {
    Ped* p = w->player;
    if (w->target != NULL && (w->target->state == PED_DEAD || w->target->seat != NULL ||
        Fx_dist(w->target->x - p->x, w->target->z - p->z) > 40000)) {
        w->target = NULL;
    }
    if (w->target == NULL) {
        int fx = Fx_sin(p->yaw), fz = Fx_cos(p->yaw), bd = 36000;
        for (int i = 0; i < w->nped; i++) {
            Ped* o = &w->peds[i];
            if (o->state == PED_DEAD || o->seat != NULL) continue;
            int dx = o->x - p->x, dz = o->z - p->z;
            int d = Fx_dist(dx, dz);
            if (d > bd || d < 1) continue;
            if ((dx * fx + dz * fz) / 16384 < d * 70 / 100) continue;
            bd = d;
            w->target = o;
        }
    }
    if (w->target != NULL) {
        p->yaw = Fx_yawOf(w->target->x - p->x, w->target->z - p->z);
    }
}

void World_voice(World* w, int kind, Ped* p) {
    if (w->voiceCd > 0 || Fx_dist(p->x - w->player->x, p->z - w->player->z) > 25000) return;
    w->voiceCd = 1800;
    Snd_voice(kind, p->female);
}

void World_shoot(World* w) {
    if (w->shotCd > 0) return;
    if (w->ammo <= 0) {
        w->weapon = 0;
        World_say(w, "Out of ammo");
        return;
    }
    w->shotCd = 380;
    w->ammo--;
    Ped* p = w->player;
    Ped* best = w->target;
    Snd_play(SND_GUN);
    w->flashT = 70;
    if (best != NULL) {
        p->yaw = Fx_yawOf(best->x - p->x, best->z - p->z);
        World_damagePed(w, best, 26 + Fx_rndBound(10), true, p->x, p->z);
    }
    for (int i = 0; i < w->nped; i++) {
        Ped* o = &w->peds[i];
        if (o->cop || o->seat != NULL || o->state == PED_DEAD || o->state == PED_JACKED || o->angry) continue;
        if (Fx_dist(o->x - p->x, o->z - p->z) < 30000 && o->clip < 0) {
            if (o->state != PED_FLEE) World_voice(w, SND_V_PANIC, o);
            o->state = PED_FLEE;
            o->timer = 5000;
            o->yaw = Fx_yawOf(o->x - p->x, o->z - p->z);
        }
    }
    if (World_copsNear(w, 35 * 1024) > 0) {
        World_raiseWanted(w, (best != NULL && best->cop) ? 2 : 1);
    }
}

void World_doPunch(World* w, int dmgBase, int reach) {
    Ped* p = w->player;
    int fx = Fx_sin(p->yaw), fz = Fx_cos(p->yaw);
    for (int i = 0; i < w->nped; i++) {
        Ped* o = &w->peds[i];
        if (o->state == PED_DEAD || o->seat != NULL || o->state == PED_JACKED) continue;
        int dx = o->x - p->x, dz = o->z - p->z;
        int d = Fx_dist(dx, dz);
        if (d > reach || d == 0) continue;
        if ((dx * fx + dz * fz) / 16384 < d * 5 / 10) continue;
        Snd_play(w->weapon == 1 ? SND_BAT : SND_PUNCH);
        World_damagePed(w, o, dmgBase + Fx_rndBound(14), true, p->x, p->z);
        int push = (o->state == PED_DEAD) ? 0 : 250;
        w->rx = o->x + dx * push / d;
        w->rz = o->z + dz * push / d;
        World_resolve(w, 280);
        if (World_walkable(w, w->rx, w->rz, o->y)) {
            o->x = w->rx;
            o->z = w->rz;
        }
        break;
    }
}

bool World_isAttack(World* w, int c) {
    return (c == w->cJab || c == w->cPunch || c == w->cHeavy);
}

void World_handleEnter(World* w) {
    if (w->inCar != NULL) {
        Car* c = w->inCar;
        Ped* p = w->player;
        p->x = World_localX(c, World_seatLX(w, c), World_seatLZ(w, c));
        p->z = World_localZ(c, World_seatLX(w, c), World_seatLZ(w, c));
        p->yaw = c->yaw;
        p->y = c->y;
        p->seat = NULL;
        c->mode = CAR_PARKED;
        w->inCar = NULL;
        Snd_engine(false, 0);
        if (World_cm(w, c)->bike) {
            p->x = World_localX(c, -60, 0);
            p->z = World_localZ(c, -60, 0);
            p->clip = -1;
            p->y = World_groundH(w, p->x, p->z, c->y);
            return;
        }
        Snd_play(SND_DOOR_OPEN);
        World_startClip(p, w->cGetout, -1, 0);
        w->pAct = WORLD_PA_GETOUT;
        return;
    }
    Car* best = NULL;
    int bd = 4600;
    for (int i = 0; i < w->ncars; i++) {
        Car* c = &w->cars[i];
        if (c->wrecked) continue;
        int d = Fx_dist(c->x - w->player->x, c->z - w->player->z);
        if (d < bd && Fx_abs(c->speed) < 3500) {
            bd = d;
            best = c;
        }
    }
    if (best != NULL) {
        if (w->radioEnabled) Snd_radioPrepare();
        w->pActCar = best;
        w->pAct = WORLD_PA_WALKTO;
        w->player->clip = -1;
        w->camYawOff = 0;
    }
}

void World_updateAct(World* w, int dt) {
    Car* c = w->pActCar;
    Ped* p = w->player;
    bool bike = World_cm(w, c)->bike;
    if (w->pAct == WORLD_PA_WALKTO) {
        int tx = bike ? World_localX(c, -60, 0) : World_localX(c, World_seatLX(w, c) - 59, World_seatLZ(w, c) - 34);
        int tz = bike ? World_localZ(c, -60, 0) : World_localZ(c, World_seatLX(w, c) - 59, World_seatLZ(w, c) - 34);
        int dx = tx - p->x, dz = tz - p->z, d = Fx_dist(dx, dz);
        if (d < 450) {
            p->x = tx; p->z = tz; p->yaw = c->yaw; p->speed = 0;
            if (c->driver != NULL && c->driver->seat == c) {
                Ped* v = c->driver;
                v->seat = NULL; c->driver = NULL;
                v->x = World_localX(c, World_seatLX(w, c), World_seatLZ(w, c));
                v->z = World_localZ(c, World_seatLX(w, c), World_seatLZ(w, c));
                v->yaw = c->yaw; v->y = c->y;
                v->state = PED_JACKED; v->hp = 100;
                World_startClip(v, w->cJacked, w->cGetup, 0);
                if (v->cop) World_raiseWanted(w, 2);
                else if (World_copsNear(w, 30 * 1024) > 0) World_raiseWanted(w, 1);
                w->pedsHit++;
                if (bike) p->clip = -1;
                else World_startClip(p, w->cPull, w->cGetin, 0);
            } else if (bike) {
                p->clip = -1;
            } else {
                World_startClip(p, w->cOpen, w->cGetin, 0);
                Snd_play(SND_DOOR_OPEN);
            }
            w->pAct = WORLD_PA_GETIN;
        } else {
            int want = Fx_yawOf(dx, dz);
            p->yaw = (p->yaw + Fx_clamp(Fx_angDiff(p->yaw, want), -700 * dt / 1000, 700 * dt / 1000)) & 1023;
            int sp = 4200;
            p->x += ((Fx_sin(p->yaw) * sp) >> 14) * dt / 1000;
            p->z += ((Fx_cos(p->yaw) * sp) >> 14) * dt / 1000;
            p->y = World_groundH(w, p->x, p->z, p->y);
            p->speed = sp;
            World_selectLoco(w, p, dt, sp);
        }
    } else if (w->pAct == WORLD_PA_GETIN) {
        World_stepClip(w, p, dt);
        if (p->clip < 0) {
            w->inCar = c;
            c->mode = CAR_PLAYER;
            p->seat = c;
            p->speed = 0;
            w->pAct = 0;
            w->carsStolen++;
            p->x = c->x;
            p->z = c->z;
            if (!bike) Snd_play(SND_DOOR_CLOSE);
            if (strcmp(World_cm(w, c)->name, "police") == 0) World_raiseWanted(w, 1);
        }
    } else if (w->pAct == WORLD_PA_GETOUT) {
        World_stepClip(w, p, dt);
        if (p->clip < 0) {
            w->pAct = 0;
            p->y = World_groundH(w, p->x, p->z, p->y);
            Snd_play(SND_DOOR_CLOSE);
        }
    }
}

void World_updateOnFoot(World* w, int dt, int keys, bool fire) {
    Ped* p = w->player;
    if (p->clip >= 0 && World_isAttack(w, p->clip)) {
        World_stepClip(w, p, dt);
        if (p->clip >= 0 && !p->hitDone && p->clipT >= 170) {
            p->hitDone = true;
            World_doPunch(w, w->weapon == 1 ? 55 : (w->combo == 3 ? 45 : 28), w->weapon == 1 ? 2200 : 1700);
        }
        p->speed = 0;
        return;
    }
    if (p->clip >= 0) World_stepClip(w, p, dt);

    int turn = 0, move = 0;
    if ((keys & WORLD_K_LEFT) != 0)  turn -= 1;
    if ((keys & WORLD_K_RIGHT) != 0) turn += 1;
    if ((keys & WORLD_K_UP) != 0)    move += 1;
    if ((keys & WORLD_K_DOWN) != 0)  move -= 1;

    if ((keys & WORLD_K_SPRINT) != 0) w->sprintHeld += dt;
    else w->sprintHeld = 0;
    bool sprint = w->sprintHeld > 350;

    int tr = Fx_abs(p->speed) > 4000 ? 640 : 860;
    if (!p->air) p->yaw = (p->yaw + turn * tr * dt / 1000) & 1023;

    int want = 0;
    if (move > 0) want = sprint ? 8800 : (w->runMode ? 6200 : 2000);
    else if (move < 0) want = w->runMode ? -3000 : -2000;
    want = want * w->speedMul / 100;

    bool pistol = (w->weapon == 2 && w->hasPistol);
    if (pistol && (keys & WORLD_K_FIRE) != 0) {
        w->aimT = 900;
        World_lockOn(w);
        World_shoot(w);
        if (want > 2000) want = 2000;
    } else if (!pistol && fire && !p->air) {
        if (w->weapon == 1) {
            World_startClip(p, w->cPunch, -1, 0);
            Snd_play(SND_SWING);
            return;
        }
        w->combo = w->combo % 3 + 1;
        if (w->combo == 1) World_startClip(p, w->cJab, -1, 560);
        else if (w->combo == 2) World_startClip(p, w->cPunch, -1, 0);
        else World_startClip(p, w->cJab, -1, 700);
        if (Fx_rndBound(4) == 0) World_voice(w, SND_V_TOMMY, p);
        return;
    }
    if ((keys & WORLD_K_FIRE) == 0 && w->aimT <= 0) w->target = NULL;

    if (!p->air) {
        int accel = (want == 0 || (want > 0) != (p->speed > 0)) ? 18000 : 11000;
        int d = want - p->speed, stp = accel * dt / 1000;
        p->speed += (d > stp ? stp : (d < -stp ? -stp : d));
        if (w->jumpReq && p->clip < 0) {
            p->air = true;
            p->vy = 300;
            World_startClip(p, w->cJumpL, -1, 0);
        }
    }

    int sp = p->speed;
    w->rx = p->x + ((Fx_sin(p->yaw) * sp) >> 14) * dt / 1000;
    w->rz = p->z + ((Fx_cos(p->yaw) * sp) >> 14) * dt / 1000;
    World_resolve(w, 300);

    int gCur = p->air ? p->y : World_groundH(w, p->x, p->z, p->y);
    /* The console map represents kerbs and stair steps more coarsely than
       the phone tiles; allow a small climb instead of treating every step as
       a wall. */
    int ox = p->x, oz = p->z, lim = p->air ? 20 : 72;
    if (World_groundH(w, w->rx, w->rz, gCur + 40) - gCur > lim) {
        int nz = w->rz;
        w->rz = oz;
        if (World_groundH(w, w->rx, w->rz, gCur + 40) - gCur > lim) {
            w->rx = ox; w->rz = nz;
            if (World_groundH(w, w->rx, w->rz, gCur + 40) - gCur > lim) {
                w->rx = ox; w->rz = oz;
            }
        }
    }

    for (int i = 0; i < w->ncars; i++) {
        Car* c = &w->cars[i];
        CarModel* m = World_cm(w, c);
        int half = m->bike ? 600 : (m->dimZ * 16 / 2 - 900);
        int rr   = m->bike ? 750 : (m->dimX * 16 / 2 + 350);
        if (half < 0) half = 0;
        for (int k = -1; k <= 1; k++) {
            int cx = c->x + ((Fx_sin(c->yaw) * k * half) >> 14);
            int cz = c->z + ((Fx_cos(c->yaw) * k * half) >> 14);
            int dx = w->rx - cx, dz = w->rz - cz;
            if (dx > rr || dx < -rr || dz > rr || dz < -rr) continue;
            int dd = Fx_dist(dx, dz);
            if (dd < rr && dd > 0) {
                w->rx = cx + dx * rr / dd;
                w->rz = cz + dz * rr / dd;
            }
        }
    }

    int mv = Fx_dist(w->rx - p->x, w->rz - p->z);
    p->x = w->rx;
    p->z = w->rz;

    int gy = World_groundH(w, p->x, p->z, (p->air ? p->y : gCur) + 40);
    if (p->air) {
        p->vy -= 627 * dt / 1000;
        p->y += p->vy * dt / 1000;
        if (p->clip < 0) {
            p->loco = w->cJumpG;
            p->locoT += dt;
        }
        if (p->y <= gy && gy <= gCur + 80 && p->vy <= 0) {
            p->y = gy;
            p->air = false;
            p->vy = 0;
            World_startClip(p, w->cJumpLand, -1, 250);
        }
        return;
    }
    if (mv == 0 && sp != 0) p->speed = sp / 2;
    if (p->clip < 0) World_selectLoco(w, p, dt, (mv == 0 && sp != 0) ? (sp / 6) : sp);
    if (gy < p->y - 40) {
        p->air = true;
        p->vy = 0;
    } else if (gy <= p->y + 72) {
        p->y = gy;
    }
}
