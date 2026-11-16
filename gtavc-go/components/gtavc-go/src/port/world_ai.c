#include "world.h"
#include "world_internal.h"
#include "assets.h"
#include "anim_data.h"
#include "fx.h"
#include "sky.h"
#include "snd.h"
#include "save.h"
#include "ui.h"
#include "j2me.h"
#include "gtavc_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void World_trafficAI(World* w, Car* c, int dt) {
    if (c->driver == NULL || c->driver->seat != c) {
        c->mode = CAR_PARKED;
        return;
    }
    int fx = Fx_sin(c->yaw), fz = Fx_cos(c->yaw);
    int rxv = fz, rzv = -fx;
    int look = 6000 + Fx_abs(c->speed) / 2;
    int ax = c->x + ((fx * (look >> 4)) >> 10);
    int az = c->z + ((fz * (look >> 4)) >> 10);
    int eR = 0, eL = 0;

    for (int d = 1; d <= 12; d++) {
        if (World_surfAt(w, ax + rxv * d / 16, az + rzv * d / 16) != WORLD_S_ROAD) break;
        eR = d;
    }
    for (int d = 1; d <= 12; d++) {
        if (World_surfAt(w, ax - rxv * d / 16, az - rzv * d / 16) != WORLD_S_ROAD) break;
        eL = d;
    }

    int st = 0;
    bool aheadRoad = World_surfAt(w, ax, az) == WORLD_S_ROAD;
    if (aheadRoad) {
        int width = eL + eR + 1, target = width / 4;
        if (eR > target + 1) st = 1;
        else if (eR < target - 1 || eR == 0) st = -1;
    } else {
        int bestA = 0, bestScore = -1;
        for (int a = -192; a <= 192; a += 64) {
            if (a == 0) continue;
            int yy = (c->yaw + a) & 1023, sc2 = 0;
            for (int s = 4; s <= 16; s += 4) {
                if (World_surfAt(w, c->x + ((Fx_sin(yy) * s) >> 4), c->z + ((Fx_cos(yy) * s) >> 4)) == WORLD_S_ROAD) sc2++;
            }
            if (a > 0) sc2 = sc2 * 2 + 1; else sc2 *= 2;
            if (sc2 > bestScore) {
                bestScore = sc2;
                bestA = a;
            }
        }
        st = bestScore <= 1 ? 1 : (bestA > 0 ? 1 : -1);
    }

    bool block = false;
    for (int i = 0; i < w->ncars && !block; i++) {
        Car* o = &w->cars[i];
        if (o == c) continue;
        int dx = o->x - c->x, dz = o->z - c->z;
        int fwd = (dx * fx + dz * fz) >> 14;
        int lat = (dx * rxv + dz * rzv) >> 14;
        if (fwd > 0 && fwd < look && lat < 2200 && lat > -2200) block = true;
    }
    if (!block) {
        int dx = w->player->x - c->x, dz = w->player->z - c->z;
        int fwd = (dx * fx + dz * fz) >> 14;
        int lat = (dx * rxv + dz * rzv) >> 14;
        if (w->inCar == NULL && fwd > 0 && fwd < look && lat < 2000 && lat > -2000) block = true;
        for (int i = 0; i < w->nped && !block; i++) {
            Ped* p = &w->peds[i];
            if (p->seat != NULL || p->state == PED_DEAD) continue;
            dx = p->x - c->x;
            dz = p->z - c->z;
            fwd = (dx * fx + dz * fz) >> 14;
            lat = (dx * rxv + dz * rzv) >> 14;
            if (fwd > 0 && fwd < look && lat < 1600 && lat > -1600) block = true;
        }
    }

    int target = aheadRoad ? (World_vmaxOf(w, c) < 11 * 1024 ? World_vmaxOf(w, c) : 11 * 1024) : 4 * 1024;
    int acc = block ? -1 : (c->speed < target ? 1 : 0);
    if (block && c->speed < 600) {
        acc = 0;
        c->vx = c->vz = 0;
        c->speed = 0;
        c->blocked += dt;
    } else {
        c->blocked = 0;
    }
    if (c->blocked > 2500 && (w->time % 4000) < dt) {
        Snd_play3d(SND_HORN, c->x, c->z, w->player->x, w->player->z);
    }
    if (c->blocked > 7000) {
        c->blocked = 0;
        c->yaw = (c->yaw + 512) & 1023;
    }
    World_driveCar(w, c, dt, acc, st, false);
}

void World_policeAI(World* w, Car* c, int dt) {
    if (c->driver == NULL || c->driver->seat != c) {
        c->mode = CAR_PARKED;
        return;
    }
    int dx = w->player->x - c->x, dz = w->player->z - c->z;
    int d = Fx_dist(dx, dz);
    int diff = Fx_angDiff(c->yaw, Fx_yawOf(dx, dz));
    int st = diff > 20 ? 1 : (diff < -20 ? -1 : 0);

    if (d < 10000 && w->inCar == NULL) {
        World_driveCar(w, c, dt, -1, 0, true);
        if (Fx_abs(c->speed) < 800) {
            Ped* dr = c->driver;
            dr->seat = NULL;
            c->driver = NULL;
            c->mode = CAR_PARKED;
            dr->x = World_localX(c, -90, 0);
            dr->z = World_localZ(c, -90, 0);
            dr->state = PED_WALK;
            dr->y = World_groundH(w, dr->x, dr->z, c->y);
            dr->hp = 160;
            World_spawnCopAt(w, World_localX(c, 90, 0), World_localZ(c, 90, 0));
        }
        return;
    }

    bool hand = Fx_abs(diff) > 200 && c->speed > 8000;
    int acc = (Fx_abs(diff) > 300 && c->speed > 6000) ? 0 : 1;
    World_driveCar(w, c, dt, acc, st, hand);
}

void World_updatePed(World* w, Ped* p, int dt) {
    if (p->seat != NULL) {
        Car* c = p->seat;
        p->x = World_localX(c, World_seatLX(w, c), World_seatLZ(w, c));
        p->z = World_localZ(c, World_seatLX(w, c), World_seatLZ(w, c));
        p->yaw = c->yaw;
        p->y = c->y;
        return;
    }
    if (p->state == PED_DEAD && p->deadT >= 0) p->deadT += dt;

    bool finished = World_stepClip(w, p, dt);
    if (p->clip >= 0) {
        if (p->state == PED_DEAD) {
            if (p->timer > 0) p->timer -= dt;
        }
        if ((p->cop || p->angry) && (p->clip == w->cPunch || p->clip == w->cJab) &&
            p->clipT > 200 && !p->hitDone) {
            p->hitDone = true;
            if (w->inCar == NULL && w->pAct == 0 &&
                Fx_dist(w->player->x - p->x, w->player->z - p->z) < 1800) {
                World_hurt(w, 5 + Fx_rndBound(4));
                Snd_play(SND_PUNCH);
            }
        }
        return;
    }

    if (finished || p->state == PED_HIT || p->state == PED_JACKED || p->state == PED_GETUP) {
        if (p->state == PED_JACKED) {
            p->state = PED_GETUP;
            World_startClip(p, w->cGetup, -1, 0);
            return;
        }
        if (p->state == PED_HIT || p->state == PED_GETUP) {
            p->state = (p->cop || p->angry) ? PED_WALK : PED_FLEE;
            p->timer = 4000;
            p->yaw = Fx_yawOf(p->x - w->player->x, p->z - w->player->z);
        }
    }
    if (p->state == PED_DEAD) {
        if (p->timer > 0) p->timer -= dt;
        return;
    }

    int sp;
    int dxp = w->player->x - p->x, dzp = w->player->z - p->z;
    int dpl = Fx_dist(dxp, dzp);

    if ((p->cop && w->wanted > 0 && w->health > 0 && !w->busted) || (p->angry && w->health > 0)) {
        int want = Fx_yawOf(dxp, dzp);
        p->yaw = (p->yaw + Fx_clamp(Fx_angDiff(p->yaw, want), -500 * dt / 1000, 500 * dt / 1000)) & 1023;
        sp = dpl > 1500 ? ((p->cop && w->wanted == 1) ? 3600 : 4600) : 0;
        if (p->attackCd > 0) p->attackCd -= dt;
        if (p->angry && dpl > 30000) p->angry = false;

        if (p->cop && w->wanted == 1) {
            if (dpl < 1500 && w->inCar == NULL && w->pAct == 0) {
                p->closeT += dt;
                if (p->closeT > 2200) {
                    w->busted = true;
                    w->bustedT = 0;
                    p->closeT = 0;
                }
            } else {
                p->closeT = 0;
            }
            if ((w->player->clip == w->cPunch || w->player->clip == w->cJab ||
                 w->player->clip == w->cHeavy || w->aimT > 0) &&
                dpl < 1700 && p->attackCd <= 0) {
                World_startClip(p, w->cPunch, -1, 0);
                p->attackCd = 2200;
            }
        } else if (p->cop && w->wanted >= 2 && dpl < 16000 && dpl > 2500 &&
                   p->attackCd <= 0 && w->inCar == NULL) {
            p->attackCd = 1600 + Fx_rndBound(1200);
            p->aimT = 600;
            sp = 0;
            Snd_play3d(SND_GUN, p->x, p->z, w->player->x, w->player->z);
            if (Fx_rndBound(100) < 55) World_hurt(w, 4 + Fx_rndBound(4));
        } else if (dpl <= 1600 && p->attackCd <= 0 && w->inCar == NULL) {
            World_startClip(p, Fx_rndBound(2) == 0 ? w->cPunch : w->cJab, -1, 0);
            p->attackCd = 1600 + Fx_rndBound(900);
        }
        if (p->aimT > 0) {
            p->aimT -= dt;
            sp = 0;
        }
        p->speed = sp;
    } else {
        if (p->state == PED_FLEE) {
            p->timer -= dt;
            sp = 5200;
            if (p->timer <= 0) p->state = PED_WALK;
        } else {
            p->turnTimer -= dt;
            sp = p->cop ? 1300 : p->speed;
            if (p->state == PED_IDLE) {
                sp = 0;
                if (p->turnTimer <= 0) {
                    p->state = PED_WALK;
                    p->turnTimer = 2500;
                }
            } else if (p->turnTimer <= 0) {
                p->targetYaw = (p->yaw + Fx_rndBound(160) - 80) & 1023;
                p->turnTimer = 3000 + Fx_rndBound(6000);
                if (Fx_rndBound(6) == 0) {
                    p->state = PED_IDLE;
                    p->turnTimer = 1500 + Fx_rndBound(2500);
                }
            }
            p->yaw = (p->yaw + Fx_clamp(Fx_angDiff(p->yaw, p->targetYaw), -300 * dt / 1000, 300 * dt / 1000)) & 1023;
            if (dpl < 4000 && w->inCar == NULL && (w->time % 9000) < dt && Fx_rndBound(2) == 0) {
                World_voice(w, SND_V_CHAT, p);
            }
        }
        for (int i = 0; i < w->ncars; i++) {
            Car* c = &w->cars[i];
            if (Fx_abs(c->speed) < 2000) continue;
            int dx = p->x - c->x, dz = p->z - c->z;
            if (dx > 5000 || dx < -5000 || dz > 5000 || dz < -5000) continue;
            if (p->state != PED_FLEE && Fx_dist(dx, dz) < 4000) {
                p->state = PED_FLEE;
                p->timer = 2200;
                p->yaw = Fx_yawOf(dx, dz);
                if (Fx_rndBound(3) == 0) World_voice(w, SND_V_PANIC, p);
            }
        }
    }

    int ax = p->x + ((Fx_sin(p->yaw) * 1500) >> 14);
    int az = p->z + ((Fx_cos(p->yaw) * 1500) >> 14);
    bool calm = p->state == PED_WALK && !p->cop && !p->angry;
    if (sp > 0 && (!World_walkable(w, ax, az, p->y) ||
        (calm && World_surfAt(w, ax, az) == WORLD_S_ROAD &&
         World_surfAt(w, p->x, p->z) != WORLD_S_ROAD && Fx_rndBound(40) != 0))) {
        int l = (p->yaw - 256) & 1023, rr = (p->yaw + 256) & 1023;
        bool okL = World_walkable(w, p->x + ((Fx_sin(l) * 1500) >> 14), p->z + ((Fx_cos(l) * 1500) >> 14), p->y);
        bool okR = World_walkable(w, p->x + ((Fx_sin(rr) * 1500) >> 14), p->z + ((Fx_cos(rr) * 1500) >> 14), p->y);
        p->targetYaw = (okR && (!okL || Fx_rndBound(2) == 0)) ? rr : (okL ? l : ((p->yaw + 512) & 1023));
        p->yaw = (p->yaw + Fx_clamp(Fx_angDiff(p->yaw, p->targetYaw), -90, 90)) & 1023;
        if (!p->cop && !p->angry) sp = sp / 3;
    }

    w->rx = p->x + ((Fx_sin(p->yaw) * sp) >> 14) * dt / 1000;
    w->rz = p->z + ((Fx_cos(p->yaw) * sp) >> 14) * dt / 1000;
    World_resolve(w, 280);
    World_stepOk(w, p->x, p->z, p->y, 100);
    if (World_deep(w, w->rx, w->rz, p->y)) {
        w->rx = p->x;
        w->rz = p->z;
    }
    p->x = w->rx;
    p->z = w->rz;
    World_selectLoco(w, p, dt, sp);
    p->y = World_groundH(w, p->x, p->z, p->y);
}

void World_updateCamera(World* w, int dt, int keys) {
    int turnCam = 0;
    /* On foot L/R orbit the camera.  In a car L is the handbrake and R looks
       behind (Switch shoulder-button behaviour). */
    if (w->inCar == NULL) {
        if ((keys & WORLD_K_CAML) != 0) turnCam -= 1;
        if ((keys & WORLD_K_CAMR) != 0) turnCam += 1;
    }
    if (turnCam != 0) {
        w->camYawOff += turnCam * 520 * dt / 1000;
        w->camIdle = 0;
    } else {
        w->camIdle += dt;
        bool moving = (w->inCar != NULL) ? Fx_abs(w->inCar->speed) > 1500 : w->player->speed > 0;
        if (moving && w->camIdle > 1500) w->camYawOff -= w->camYawOff * 3 * dt / 1000;
    }
    w->camYawOff = Fx_clamp(w->camYawOff, -512, 512);
    if ((keys & WORLD_K_CAMU) != 0) { w->camPitch += 80 * dt / 1000 + 1; w->camIdle = 0; }
    if ((keys & WORLD_K_CAMD) != 0) { w->camPitch -= 80 * dt / 1000 + 1; w->camIdle = 0; }
    w->camPitch = Fx_clamp(w->camPitch, 4, 150);

    int hy;
    if (w->inCar != NULL) {
        int spd = Fx_dist(w->inCar->vx, w->inCar->vz);
        hy = spd > 2500 ? Fx_yawOf(w->inCar->vx, w->inCar->vz) : w->inCar->yaw;
        if (w->inCar->speed < -1500) hy = (w->inCar->yaw + 512) & 1023;
    } else {
        hy = w->player->yaw;
    }

    int want = (hy + w->camYawOff) & 1023;
    if (w->inCar != NULL && (keys & WORLD_K_CAMR) != 0) {
        /* look behind while R is held */
        want = (w->inCar->yaw + 512) & 1023;
        w->camYaw = want;
        w->camIdle = 0;
    } else {
        int df = Fx_angDiff(w->camYaw, want);
        int rate = w->inCar != NULL ? 3 : 7;
        w->camYaw = (w->camYaw + df * rate * dt / 1000 + (df > 0 ? 1 : (df < 0 ? -1 : 0))) & 1023;
        if (Fx_abs(Fx_angDiff(w->camYaw, want)) < 2) w->camYaw = want;
    }

    int tx = w->player->x, tz = w->player->z, ty = w->player->y + 100;
    int dist = (w->inCar != NULL) ? (World_cm(w, w->inCar)->bike ? 5200 : 6800 + Fx_abs(w->inCar->speed) / 25) : w->camDistMm;
    int cp = Fx_cos(w->camPitch), sp = Fx_sin(w->camPitch);
    int hd = (dist * cp) >> 14, vd = (dist * sp) >> 14;

    int bestD = 120;
    for (int s = 8; s >= 1; s--) {
        int f = s * 1000 / 8;
        int cx = tx - ((Fx_sin(w->camYaw) * (hd * f / 1000)) >> 14);
        int cz = tz - ((Fx_cos(w->camYaw) * (hd * f / 1000)) >> 14);
        int cyy = ty + (vd * f / 1000 >> 4);
        if (cyy > World_groundH(w, cx, cz, cyy) + 20) {
            bestD = f;
            break;
        }
    }
    int hdf = hd * bestD / 1000, vdf = vd * bestD / 1000;
    w->camXm = tx - ((Fx_sin(w->camYaw) * hdf) >> 14);
    w->camZm = tz - ((Fx_cos(w->camYaw) * hdf) >> 14);
    w->camYm = ty + (vdf >> 4);
    int g = World_groundH(w, w->camXm, w->camZm, w->camYm);
    if (w->camYm < g + 24) w->camYm = g + 24;
}

void World_cycleWeapon(World* w) {
    for (int k = 0; k < 3; k++) {
        w->weapon = (w->weapon + 1) % 3;
        if (w->weapon == 0 || (w->weapon == 1 && w->hasBat) ||
            (w->weapon == 2 && w->hasPistol && w->ammo > 0)) break;
    }
    char msg[96];
    if (w->weapon == 0) snprintf(msg, sizeof(msg), "Fists");
    else if (w->weapon == 1) snprintf(msg, sizeof(msg), "Baseball bat");
    else snprintf(msg, sizeof(msg), "Pistol  (%d)", w->ammo);
    World_say(w, msg);
    w->helpT = 1200;
}

void World_updatePickups(World* w, int dt) {
    for (int i = 0; i < w->npick; i++) {
        if (w->pkLife[i] < 0) continue;
        if (w->pkLife[i] > 0) {
            w->pkLife[i] -= dt;
            if (w->pkLife[i] <= 0) { w->pkLife[i] = -1; continue; }
        }
        if (w->pkT[i] > 0) { w->pkT[i] -= dt; continue; }
        if (w->inCar == NULL && Fx_dist(w->pkX[i] - w->player->x, w->pkZ[i] - w->player->z) < 1200) {
            if (w->pkType[i] == 3) {
                w->money += w->pkAmt[i];
                char m[64];
                snprintf(m, sizeof(m), "$%d", w->pkAmt[i]);
                World_say(w, m);
            } else if (w->pkType[i] == 2) {
                w->hasPistol = true;
                w->ammo += w->pkLife[i] > 0 ? 17 : 68;
                if (w->weapon == 0) w->weapon = 2;
                char m[64];
                snprintf(m, sizeof(m), "Pistol  (%d)", w->ammo);
                World_say(w, m);
            } else {
                w->hasBat = true;
                if (w->weapon == 0) w->weapon = 1;
                World_say(w, "Baseball bat");
            }
            w->helpT = 1500;
            Snd_play(SND_PICKUP);
            if (w->pkLife[i] > 0) w->pkLife[i] = -1;
            else w->pkT[i] = 30000;
        }
    }
}

void World_updateWanted(World* w, int dt) {
    if (w->wanted == 0) return;
    if (World_copsNear(w, 40 * 1024) == 0) {
        w->wantedCalmT += dt;
        if (w->wantedCalmT > 22000) {
            w->wanted--;
            w->wantedCalmT = 0;
            if (w->wanted == 0) World_say(w, "You lost the cops.");
        }
    } else {
        w->wantedCalmT = 0;
    }
    int foot = 0, carsP = 0;
    for (int i = 0; i < w->nped; i++) {
        if (w->peds[i].cop && w->peds[i].state != PED_DEAD && w->peds[i].seat == NULL) foot++;
    }
    for (int i = 0; i < w->ncars; i++) {
        if (w->cars[i].mode == CAR_POLICE) carsP++;
    }
    int maxFoot = w->wanted == 1 ? 1 : (w->wanted == 2 ? 2 : 3);
    if (foot < maxFoot && (w->time % 4000) < dt) World_spawnCop(w);
    if (w->wanted >= 2 && carsP < (w->wanted >= 4 ? 2 : 1) && (w->time % 9000) < dt) World_spawnPoliceCar(w);
}

void World_updateDriving(World* w, int dt, int keys) {
    Car* c = w->inCar;
    int acc = 0, st = 0;
    /* A / d-pad up = accelerate, B / d-pad down = brake+reverse. */
    if ((keys & (WORLD_K_UP | WORLD_K_SPRINT)) != 0) acc += 1;
    if ((keys & (WORLD_K_DOWN | WORLD_K_FIRE)) != 0) acc -= 1;
    if ((keys & WORLD_K_LEFT) != 0) st -= 1;
    if ((keys & WORLD_K_RIGHT) != 0) st += 1;
    if (c->wrecked) acc = 0;
    /* X or L = handbrake (Switch LB). */
    bool hand = (keys & (WORLD_K_HAND | WORLD_K_CAML)) != 0;
    World_driveCar(w, c, dt, acc, st, hand);
    w->player->x = c->x;
    w->player->z = c->z;
    w->player->yaw = c->yaw;
    w->player->y = c->y;
}

void World_updateWorld(World* w, int dt) {
    World_updateParticles(w, dt);

    for (int i = 0; i < w->ncars; i++) {
        Car* c = &w->cars[i];
        if (!c->wrecked) {
            c->fxT += dt;
            CarModel* m = World_cm(w, c);
            int ez = m->bike ? 0 : m->dimZ * 30 / 100;
            if (c->burning) {
                c->burnT += dt;
                if (c->fxT > 50) {
                    c->fxT = 0;
                    int lx1 = Fx_rndBound(60) - 30;
                    int lz1 = Fx_rndBound(60) - 30;
                    World_spawnFx(w, 1, World_localX(c, lx1, ez - 30), c->y + m->dimY * 6 / 10, World_localZ(c, lz1, ez - 30));
                    int lx2 = Fx_rndBound(40) - 20;
                    int lz2 = Fx_rndBound(40) - 20;
                    World_spawnFx(w, 1, World_localX(c, lx2, ez), c->y + m->dimY * 7 / 10, World_localZ(c, lz2, ez));
                    World_spawnFx(w, 0, World_localX(c, 0, ez), c->y + m->dimY, World_localZ(c, 0, ez));
                }
                if (c->burnT > 5000) World_explode(w, c);
            } else if (c->hp < 450 && c->fxT > (c->hp < 250 ? 90 : 220)) {
                c->fxT = 0;
                World_spawnFx(w, 0, World_localX(c, 0, ez), c->y + m->dimY * 7 / 10, World_localZ(c, 0, ez));
            }
        }
        if (c->mode == CAR_TRAFFIC) World_trafficAI(w, c, dt);
        else if (c->mode == CAR_POLICE) World_policeAI(w, c, dt);
        else if (c->mode == CAR_PARKED && (c->vx != 0 || c->vz != 0)) World_driveCar(w, c, dt, 0, 0, true);
    }

    int traffic = 0;
    for (int i = w->ncars - 1; i >= 0; i--) {
        Car* c = &w->cars[i];
        int d = Fx_dist(c->x - w->player->x, c->z - w->player->z);
        if (c != w->inCar && c != w->pActCar && d > 150 * 1024) {
            World_removeCar(w, i);
            continue;
        }
        if (c->mode == CAR_TRAFFIC) traffic++;
    }
    if (traffic < w->maxTraffic && (w->time % 1200) < dt) World_spawnTraffic(w, false);

    for (int i = 0; i < w->nped; i++) World_updatePed(w, &w->peds[i], dt);

    int alive = 0;
    for (int i = 0; i < w->nped; i++) {
        Ped* p = &w->peds[i];
        if (p->seat != NULL) continue;
        int d = Fx_dist(p->x - w->player->x, p->z - w->player->z);
        if (p->cop) {
            if ((w->wanted == 0 && d > 50000) || d > 120000) {
                p->state = PED_DEAD;
                p->timer = 0;
                p->deadT = -1;
                p->cop = false;
            }
            continue;
        }
        if (p->state != PED_DEAD && p->state != PED_JACKED && p->state != PED_GETUP && d > 100000) {
            p->state = PED_DEAD;
            p->timer = 0;
            p->deadT = -1;
        }
        if (p->state != PED_DEAD) alive++;
    }
    if (alive < w->maxPed && (w->time % 500) < dt) World_spawnPed(w, false);
}

void World_update(World* w, int dt, int keys, bool fire, bool enter, bool toggleRun, bool nextWeapon, bool jump) {
    w->jumpReq = jump;
    if (w->voiceCd > 0) w->voiceCd -= dt;
    if (dt > 100) dt = 100;
    w->time += dt;
    w->clockMs += dt;
    while (w->clockMs >= 1000) {
        w->clockMs -= 1000;
        w->minutes++;
        if (w->minutes >= 1440) {
            w->minutes = 0;
            w->dayCount++;
        }
    }
    if (w->helpT > 0) w->helpT -= dt;
    if (w->shake > 0) w->shake -= dt;
    if (w->shotCd > 0) w->shotCd -= dt;
    if (w->aimT > 0) w->aimT -= dt;
    if (w->flashT > 0) w->flashT -= dt;

    GTAVC_TIME(PROF_STREAM, World_streamTiles(w, w->player->x, w->player->z));
    GTAVC_TIME(PROF_POOLS, World_pools_rotatePools(w, dt));
    GTAVC_TIME(PROF_MISSION, World_missionUpdate(w, dt));

    int64_t radio_start = rg_system_timer();
    bool radioNow = (w->inCar != NULL) && w->radioEnabled;
    if (radioNow && !w->radioInCar) Snd_radioStart();
    else if (!radioNow && w->radioInCar) Snd_radioStop();
    w->radioInCar = radioNow;
    if (radioNow) Snd_radioTick(dt);
    gtavc_prof_stage(PROF_RADIO, radio_start);

    if (w->busted) {
        w->bustedT += dt;
        GTAVC_TIME(PROF_ACTORS, World_updateWorld(w, dt));
        GTAVC_TIME(PROF_CAMERA, World_updateCamera(w, dt, keys));
        if (w->bustedT > 3600) {
            w->busted = false;
            w->bustedT = 0;
            World_arrestRespawn(w);
        }
        return;
    }
    if (w->health <= 0) {
        if (w->wastedT == 0) {
            w->player->seat = NULL;
            if (w->inCar != NULL) {
                w->inCar->mode = CAR_PARKED;
                w->inCar = NULL;
            }
            w->pAct = 0;
            World_startClip(w->player, w->cKoF, -2, 0);
            Snd_engine(false, 0);
        }
        w->wastedT += dt;
        World_stepClip(w, w->player, dt);
        if (w->wastedT > 3600) World_respawn(w);
        GTAVC_TIME(PROF_ACTORS, World_updateWorld(w, dt));
        GTAVC_TIME(PROF_CAMERA, World_updateCamera(w, dt, keys));
        return;
    }

    /* Continuous autorun: the original tap-to-walk toggle is intentionally
       disabled on this target. */
    (void)toggleRun;
    if (nextWeapon && w->inCar == NULL) World_cycleWeapon(w);

    if (w->pAct != 0) {
        GTAVC_TIME(PROF_PLAYER, World_updateAct(w, dt));
    } else {
        if (enter) GTAVC_TIME(PROF_PLAYER, World_handleEnter(w));
        if (w->pAct == 0) {
            if (w->inCar != NULL) GTAVC_TIME(PROF_PLAYER, World_updateDriving(w, dt, keys));
            else GTAVC_TIME(PROF_PLAYER, World_updateOnFoot(w, dt, keys, fire));
        }
    }

    bool inWater = (w->inCar != NULL)
        ? World_deep(w, w->inCar->x, w->inCar->z, w->inCar->y)
        : (w->pAct == 0 && World_deep(w, w->player->x, w->player->z, w->player->y));
    if (inWater) {
        if (w->drownT == 0) Snd_play(SND_SPLASH);
        w->drownT += dt;
        if (w->inCar != NULL) {
            w->inCar->vx = w->inCar->vx * 9 / 10;
            w->inCar->vz = w->inCar->vz * 9 / 10;
        }
        if (w->drownT > 1500) {
            w->health = 0;
            w->wastedT = 0;
            World_say(w, "Tommy can't swim!");
        }
    } else {
        w->drownT = 0;
    }

    World_updatePickups(w, dt);
    w->saveT += dt;
    if (w->saveT > 20000 && w->pAct == 0) {
        w->saveT = 0;
        GTAVC_TIME(PROF_SAVE, Save_write(w));
    }
    GTAVC_TIME(PROF_ACTORS, World_updateWorld(w, dt));
    GTAVC_TIME(PROF_CAMERA, World_updateCamera(w, dt, keys));
    GTAVC_TIME(PROF_WANTED, World_updateWanted(w, dt));

    int64_t sound_start = rg_system_timer();
    Snd_engine(w->inCar != NULL && !w->inCar->wrecked,
               w->inCar != NULL ? Fx_abs(w->inCar->speed) * 100 / (World_vmaxOf(w, w->inCar) > 1 ? World_vmaxOf(w, w->inCar) : 1) : 0);

    bool siren = false;
    for (int i = 0; i < w->ncars; i++) {
        if (w->cars[i].mode == CAR_POLICE &&
            Fx_dist(w->cars[i].x - w->player->x, w->cars[i].z - w->player->z) < 90000) siren = true;
    }
    Snd_siren(siren);
    gtavc_prof_stage(PROF_SOUND, sound_start);
}
