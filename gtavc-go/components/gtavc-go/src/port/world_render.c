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

void World_loadTimecyc(World* w) {
    int len = 0;
    uint8_t* b = Assets_readAll("/timecyc.bin", &len);
    if (!b) return;
    int n = len < 360 ? len : 360;
    memset(w->tc, 0, 360);
    memcpy(w->tc, b, (size_t)n);
    free(b);
}

void World_clockStr(World* w, char* out, size_t outSize) {
    if (outSize < 6) { if (outSize) out[0] = 0; return; }
    int h = w->minutes / 60, m = w->minutes % 60;
    out[0] = (char)('0' + h / 10);
    out[1] = (char)('0' + h % 10);
    out[2] = ':';
    out[3] = (char)('0' + m / 10);
    out[4] = (char)('0' + m % 10);
    out[5] = 0;
}

IRAM_ATTR void World_lighting(World* w) {
    int h = w->minutes / 60;
    int n = (h + 1) % 24;
    int f = (w->minutes % 60) * 256 / 60;
    for (int k = 0; k < 15; k++) {
        int a = w->tc[h * 15 + k] & 255;
        int b = w->tc[n * 15 + k] & 255;
        w->lv[k] = a + (((b - a) * f) >> 8);
    }
    R3D* r = w->r;
    r->skyTop = (w->lv[0] << 16) | (w->lv[1] << 8) | w->lv[2];
    r->skyBot = (w->lv[3] << 16) | (w->lv[4] << 8) | w->lv[5];
    r->fogR = w->lv[3];
    r->fogG = w->lv[4];
    r->fogB = w->lv[5];
    r->groundCol = Sky_mix(r->skyBot, 0x5C5A50, 150);

    int ambObj = (w->lv[9] + w->lv[10] + w->lv[11]) / 3;
    int dir = (w->lv[12] + w->lv[13] + w->lv[14]) / 3;
    int fb = 200 + w->brightness * 4;
    int a = ((ambObj * 256 / 255 + 28) * fb) >> 8;
    int d = ((dir * 140 / 255) * fb) >> 8;
    r->amb = a;
    r->dif = d;
    r->emis = 256;
    w->night = w->minutes < 6 * 60 || w->minutes >= 20 * 60;
    Sky_sunDir(w);
    if (Sky_sunY > 1500) {
        r->lx = Sky_sunX;
        r->ly = Sky_sunY;
        r->lz = Sky_sunZ;
    } else {
        r->lx = 5000;
        r->ly = 12000;
        r->lz = -6000;
    }
    r->night = w->minutes < 6 * 60 + 30 || w->minutes >= 19 * 60 + 30;
    r->farZ = w->drawDist * 64;
    r->fogFar = r->farZ;
    r->fogNear = r->farZ * 35 / 100;
}

void World_render(World* w) {
    int64_t t0 = GTAVC_PROFILE_TIMER();
    World_lighting(w);
    int sx = 0, sy = 0;
    if (w->shake > 0) {
        sx = (Fx_rndBound(9) - 4) * 2;
        sy = (Fx_rndBound(7) - 3);
    }
    R3D* r = w->r;
    r->animU = w->time / 22;
    r->animV = w->time / 31;
    R3D_setCamera(r, (w->camXm >> 4) + sx, w->camYm + sy, w->camZm >> 4, w->camYaw, w->camPitch);
    R3D_clear(r);
    int64_t clear_end = GTAVC_PROFILE_TIMER();
    Sky_drawSky(w, r);
    int64_t t1 = GTAVC_PROFILE_TIMER();

    int* mv = r->mv;
    int nearLim = w->nearLod;
    int lim = r->farZ + 3300;
    int fx = Fx_sin(w->camYaw), fz = Fx_cos(w->camYaw);

    int totalTiles = w->tnx * w->tnz;
    for (int k = 0; k < totalTiles; k++) {
        Tile* t = w->tiles[k];
        if (!t || !t->installed) continue;
        int dx = t->cx - r->camX, dz = t->cz - r->camZ;
        if (dx > lim || dx < -lim || dz > lim || dz < -lim) continue;
        if (((dx * fx + dz * fz) >> 14) < -4600) continue;
        int dy = -r->camY;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
        int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
        int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        bool near = Fx_abs(dx) < nearLim && Fx_abs(dz) < nearLim;
        Mesh** chunks = near ? t->nearChunks : t->farChunks;
        int nch = near ? t->nearChunkCount : t->farChunkCount;
        if (chunks) {
            for (int ci = 0; ci < nch; ci++) {
                R3D_drawMesh(r, chunks[ci], mv, tx, ty, tz, r->lx, r->ly, r->lz, 0);
            }
        } else {
            R3D_drawMesh(r, near ? t->nearMesh : t->farMesh, mv, tx, ty, tz, r->lx, r->ly, r->lz, 0);
        }
    }
    int64_t t2 = GTAVC_PROFILE_TIMER();

    for (int i = 0; i < w->nped; i++) World_drawBlood(w, &w->peds[i]);
    for (int i = 0; i < w->ncars; i++) World_drawCar(w, &w->cars[i], false);

    for (int i = 0; i < w->nped; i++) {
        Ped* p = &w->peds[i];
        if (p->state == PED_DEAD && p->deadT < 0) continue;
        if (Fx_abs(p->x - r->camX * 16) > 90000 || Fx_abs(p->z - r->camZ * 16) > 90000) continue;
        World_drawPed(w, p);
    }
    World_drawPed(w, w->player);

    if (w->target != NULL && w->aimT > 0 && w->target->state != PED_DEAD) {
        Sky_target(r, w->target->x >> 4, w->target->y + 70, w->target->z >> 4,
                   w->target->hp, w->target->cop ? 160 : 100, w->time);
    }
    World_drawPickups(w);
    World_drawMissionMarker(w);
    for (int i = 0; i < WORLD_NFX; i++) {
        if (w->fxL[i] <= 0) continue;
        int age = 256 - w->fxL[i] * 256 / w->fxM[i];
        int size = w->fxT[i] == 0 ? 70 + age / 2 : (w->fxT[i] == 1 ? 130 - age / 6 : 160 + age);
        int fade = w->fxT[i] == 0 ? (age < 40 ? age * 4 : 300 - age) : 256 - age;
        if (fade <= 0) continue;
        Sky_particle(r, w->fxT[i], w->fxX[i], w->fxY[i], w->fxZ[i], size, fade > 256 ? 256 : fade);
    }
    for (int i = 0; i < w->ncars; i++) World_drawCar(w, &w->cars[i], true);
    if (w->night) World_drawBeam(w);
    int64_t post_start = GTAVC_PROFILE_TIMER();
    Sky_drawPost(w, r);
    int64_t t3 = GTAVC_PROFILE_TIMER();

    GTAVC_PROF(g_prof.t_clear_us += (uint64_t)(clear_end - t0));
    GTAVC_PROF(g_prof.t_sky_us += (uint64_t)(t1 - clear_end));
    GTAVC_PROF(g_prof.t_post_us += (uint64_t)(t3 - post_start));
    GTAVC_PROF(g_prof.t_tiles_us += (uint64_t)(t2 - t1));
    GTAVC_PROF(g_prof.t_ents_us  += (uint64_t)(post_start - t2));
    GTAVC_PROF(g_prof.tris       += (uint32_t)r->tris);
}

Mesh* World_cashMesh(World* w) {
    if (w->cashMesh != NULL) return w->cashMesh;
    Mesh* m = Mesh_new(8, 12);
    int a = 12, b = 4, c = 7;
    int mx[8] = { -a, a, a, -a, -a, a, a, -a };
    int my[8] = { -b, -b, b, b, -b, -b, b, b };
    int mz[8] = { -c, -c, -c, -c, c, c, c, c };
    for (int i = 0; i < 8; i++) { m->x[i] = mx[i]; m->y[i] = my[i]; m->z[i] = mz[i]; m->u[i] = 0; m->v[i] = 0; }
    int q[36] = { 0, 1, 2, 0, 2, 3, 5, 4, 7, 5, 7, 6, 4, 0, 3, 4, 3, 7, 1, 5, 6, 1, 6, 2, 3, 2, 6, 3, 6, 7, 4, 5, 1, 4, 1, 0 };
    for (int i = 0; i < 36; i++) m->ti[i] = q[i];
    for (int i = 0; i < 12; i++) {
        m->tex[i] = -1;
        m->col[i] = i < 4 ? 0x3FA53F : 0x2E8B2E;
        m->flag[i] = (int8_t)(MESH_F_DOUBLE | MESH_F_EMISSIVE);
    }
    Mesh_bounds(m);
    w->cashMesh = m;
    return m;
}

void World_drawPickups(World* w) {
    R3D* r = w->r;
    for (int i = 0; i < w->npick; i++) {
        if (w->pkT[i] > 0 || w->pkLife[i] < 0) continue;
        Mesh* m = w->pkType[i] == 3 ? World_cashMesh(w) : w->weaponMesh[w->pkType[i]];
        if (!m) continue;
        int dx = (w->pkX[i] >> 4) - r->camX;
        int dy = w->pkY[i] + 40 + ((Fx_sin(w->time / 3) * 6) >> 14) - r->camY;
        int dz = (w->pkZ[i] >> 4) - r->camZ;
        if (dx > 6000 || dx < -6000 || dz > 6000 || dz < -6000) continue;
        Fx_rotY(w->rYaw, w->time / 4);
        Fx_mul(r->mv, w->rYaw, w->mmA);
        int* mv = r->mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
        int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
        int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        R3D_drawMesh(r, m, w->mmA, tx, ty, tz, 0, 16384, 0, 0);
    }
}

void World_drawBlood(World* w, Ped* p) {
    if (p->state != PED_DEAD || p->deadT <= 400 || p->seat != NULL) return;
    R3D* r = w->r;
    int dx = (p->bloodX >> 4) - r->camX;
    int dz = (p->bloodZ >> 4) - r->camZ;
    int dy = p->bloodY + 2 - r->camY;
    if (dx > 4000 || dx < -4000 || dz > 4000 || dz < -4000) return;
    int grow = p->deadT - 400;
    if (grow > 3000) grow = 3000;
    int s = 4000 + grow * 9 / 2;
    Fx_rotY(w->rYaw, p->yaw);
    for (int k = 0; k < 9; k++) w->sc[k] = 0;
    w->sc[0] = s;
    w->sc[4] = 16384;
    w->sc[8] = s;
    Fx_mul(w->rYaw, w->sc, w->wA);
    Fx_mul(r->mv, w->wA, w->mmA);
    int* mv = r->mv;
    int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
    int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
    int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
    R3D_drawMesh(r, w->blood, w->mmA, tx, ty, tz, 0, 16384, 0, 0);
}

void World_drawBeam(World* w) {
    R3D* r = w->r;
    int dx = (w->lampX >> 4) - r->camX;
    int dy = w->lampY - r->camY;
    int dz = (w->lampZ >> 4) - r->camZ;
    if (w->lampY < -50 || dx > 48000 || dx < -48000 || dz > 48000 || dz < -48000) return;

    Fx_rotY(w->rYaw, (w->time / 8) & 1023);
    Fx_mul(r->mv, w->rYaw, w->mmA);
    int* mv = r->mv;
    int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
    int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
    int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
    int fn = r->fogNear, ff = r->fogFar, fz0 = r->farZ;
    r->fogNear = 1 << 28;
    r->fogFar = (1 << 28) + 1;
    r->farZ = 60000;
    r->additive = true;
    R3D_drawMesh(r, w->beam, w->mmA, tx, ty, tz, 0, 16384, 0, 0);
    r->additive = false;
    r->farZ = fz0;
    r->fogNear = fn;
    r->fogFar = ff;
}

IRAM_ATTR void World_drawCar(World* w, Car* c, bool glassPass) {
    R3D* r = w->r;
    int px = c->x >> 4, pz = c->z >> 4;
    int dx = px - r->camX, dz = pz - r->camZ, dy = c->y - r->camY;
    if (dx > r->farZ || dx < -r->farZ || dz > r->farZ || dz < -r->farZ) return;

    int dist = Fx_abs(dx) > Fx_abs(dz) ? Fx_abs(dx) : Fx_abs(dz);
    bool near = dist < 1500;
    int* mv = r->mv;
    CarModel* m = World_cm(w, c);
    int roll = m->bike ? -c->roll * 3 : c->roll;
    int pitch = c->pitch - c->slopePitch;

    Fx_rotY(w->wA, c->yaw);
    Fx_rotX(w->wB, pitch);
    Fx_mul(w->wA, w->wB, w->wC);
    Fx_rotZ(w->wA, roll);
    Fx_mul(w->wC, w->wA, w->rYaw);
    Fx_mul(mv, w->rYaw, w->mmA);

    int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
    int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
    int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
    int lmx = (w->rYaw[0] * r->lx + w->rYaw[3] * r->ly + w->rYaw[6] * r->lz) >> 14;
    int lmy = (w->rYaw[1] * r->lx + w->rYaw[4] * r->ly + w->rYaw[7] * r->lz) >> 14;
    int lmz = (w->rYaw[2] * r->lx + w->rYaw[5] * r->ly + w->rYaw[8] * r->lz) >> 14;
    r->tint2 = c->color2;

    if (!near) {
        if (glassPass) {
            if (m->farGlass != NULL && dist < 2600) {
                r->blend = true;
                R3D_drawMesh(r, m->farGlass, w->mmA, tx, ty, tz, lmx, lmy, lmz, 0);
                r->blend = false;
            }
        } else {
            R3D_drawMesh(r, m->farMesh, w->mmA, tx, ty, tz, lmx, lmy, lmz, c->color);
        }
        return;
    }

    for (int p = 0; p < m->np; p++) {
        int id = m->partId[p];
        if (id < 12 && (c->lostBits & (1 << id)) != 0) continue;
        Mesh** v = (id < 12 && (c->damBits & (1 << id)) != 0) ? m->dam[p] : m->ok[p];
        if (!v) continue;
        if (glassPass) {
            if (v[1] != NULL) {
                r->blend = true;
                R3D_drawMesh(r, v[1], w->mmA, tx, ty, tz, lmx, lmy, lmz, 0);
                r->blend = false;
            }
        } else {
            R3D_drawMesh(r, v[0], w->mmA, tx, ty, tz, lmx, lmy, lmz, c->color);
        }
    }
    if (glassPass || m->bike) return;

    int cs = Fx_cos(c->yaw), sn = Fx_sin(c->yaw);
    for (int k = 0; k < m->nw; k++) {
        int wxl = m->wx[k], wzl = m->wz[k];
        int wpx = px + ((cs * wxl + sn * wzl) >> 14);
        int wpz = pz + ((-sn * wxl + cs * wzl) >> 14);
        int wpy = c->y + m->wy[k];
        Fx_rotY(w->wA, c->yaw + (wzl > 0 ? c->steer : 0));
        Fx_rotX(w->wB, c->wheelRot);
        Fx_mul(w->wA, w->wB, w->wC);
        if (wxl < 0) {
            Fx_rotY(w->wA, 512);
            Fx_mul(w->wC, w->wA, w->wB);
            memcpy(w->wC, w->wB, 9 * sizeof(int));
        }
        int ex = wpx - r->camX;
        int ey = wpy - r->camY;
        int ez = wpz - r->camZ;
        int wtx = (mv[0] * ex + mv[1] * ey + mv[2] * ez) >> 14;
        int wty = (mv[3] * ex + mv[4] * ey + mv[5] * ez) >> 14;
        int wtz = (mv[6] * ex + mv[7] * ey + mv[8] * ez) >> 14;
        Fx_mul(mv, w->wC, w->wA);
        int wlx = (w->wC[0] * r->lx + w->wC[3] * r->ly + w->wC[6] * r->lz) >> 14;
        int wly = (w->wC[1] * r->lx + w->wC[4] * r->ly + w->wC[7] * r->lz) >> 14;
        int wlz = (w->wC[2] * r->lx + w->wC[5] * r->ly + w->wC[8] * r->lz) >> 14;
        R3D_drawMesh(r, CarModel_wheel, w->wA, wtx, wty, wtz, wlx, wly, wlz, 0);
    }
}

static int AIM[9];
static int AIM_init = 0;

IRAM_ATTR void World_drawPed(World* w, Ped* p) {
    R3D* r = w->r;
    int clip, t, fx = p->x >> 4, fz = p->z >> 4, fy = p->y;
    if (p->seat != NULL) {
        clip = w->cSit;
        t = 0;
        fy = p->seat->y - 18;
        if (World_cm(w, p->seat)->bike) fy = p->seat->y + 10;
    } else if (p->clip >= 0) {
        clip = p->clip;
        t = p->clipT;
    } else {
        clip = p->loco;
        t = p->locoT;
    }
    if (clip < 0) clip = w->cIdle;

    int dx = fx - r->camX, dz = fz - r->camZ;
    if (dx > r->farZ || dx < -r->farZ || dz > r->farZ || dz < -r->farZ) return;

    AnimData_sample(clip, t, w->pose);
    bool aiming = p->seat == NULL && p->clip < 0 &&
        ((p == w->player && w->weapon == 2 && w->aimT > 0) || (p != w->player && p->aimT > 0));
    if (aiming) {
        if (!AIM_init) {
            Fx_rotY(AIM, 768);
            AIM_init = 1;
        }
        memcpy(&w->pose[9], AIM, 9 * sizeof(int));
        memcpy(&w->pose[18], AIM, 9 * sizeof(int));
    }

    int distXZ = Fx_abs(dx) > Fx_abs(dz) ? Fx_abs(dx) : Fx_abs(dz);
    PedModel* m;
    if (p == w->player) {
        m = w->pm0;
    } else {
        m = distXZ < 1150 ? w->pool[p->kind] : w->poolLo[p->kind];
        if (!m) m = distXZ < 1150 ? w->poolLo[p->kind] : w->pool[p->kind];
    }
    if (!m) return;
    int ptex = (p == w->player) ? w->playerTex : p->tex;
    if (ptex < 0 || ptex >= R3D_MAX_TEX || r->T[ptex] == NULL) ptex = -1;
    PedModel_drawPose(m, r, fx, fy, fz, p->yaw, w->pose, ptex);
    if (p == w->player && w->weapon > 0 && p->seat == NULL && w->weaponMesh[w->weapon] != NULL) {
        World_drawHeld(w, m, w->weaponMesh[w->weapon]);
    }
    if (aiming && (p == w->player ? w->flashT > 0 : p->aimT > 450)) {
        PedModel_hand(m, w->handW);
        Sky_flashEffect(r, w->handW[0], w->handW[1], w->handW[2]);
    }
}

void World_drawHeld(World* w, PedModel* m, Mesh* mesh) {
    R3D* r = w->r;
    PedModel_hand(m, w->handW);
    int* R = m->rp[2];
    /* The bat mesh is exported with its long axis along +x, matching the
       forearm.  Keep that original orientation (the previous 180-degree
       correction made it point behind Tommy). */
    int dx = w->handW[0] - r->camX;
    int dy = w->handW[1] - r->camY;
    int dz = w->handW[2] - r->camZ;
    int* mv = r->mv;
    int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
    int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
    int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
    Fx_mul(mv, R, w->mmA);
    R3D_drawMesh(r, mesh, w->mmA, tx, ty, tz,
                 (R[0] * r->lx + R[3] * r->ly + R[6] * r->lz) >> 14,
                 (R[1] * r->lx + R[4] * r->ly + R[7] * r->lz) >> 14,
                 (R[2] * r->lx + R[5] * r->ly + R[8] * r->lz) >> 14, 0);
}
