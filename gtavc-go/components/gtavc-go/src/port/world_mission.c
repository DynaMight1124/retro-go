#include "world.h"
#include "world_internal.h"
#include "save.h"
#include "fx.h"
#include "j2me.h"

#include <stdio.h>
#include <string.h>

#define MISSION_TIME_MS 180000

static void makeMarker(World* w) {
    if (w->missionMarker) return;
    Mesh* m = Mesh_new(6, 8);
    if (!m) return;
    m->x[0] = 0;    m->y[0] = 120;  m->z[0] = 0;
    m->x[1] = 64;   m->y[1] = 0;    m->z[1] = 0;
    m->x[2] = 0;    m->y[2] = 0;    m->z[2] = 64;
    m->x[3] = -64;  m->y[3] = 0;    m->z[3] = 0;
    m->x[4] = 0;    m->y[4] = 0;    m->z[4] = -64;
    m->x[5] = 0;    m->y[5] = -40;  m->z[5] = 0;
    static const int ti[24] = {
        0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 1,
        5, 2, 1,  5, 3, 2,  5, 4, 3,  5, 1, 4
    };
    memcpy(m->ti, ti, sizeof(ti));
    for (int i = 0; i < 8; i++) {
        m->tex[i] = -1;
        m->col[i] = 0xFFD040;
        m->flag[i] = (int8_t)(MESH_F_DOUBLE | MESH_F_EMISSIVE);
    }
    Mesh_bounds(m);
    w->missionMarker = m;
}

static void pickDestination(World* w) {
    /* Find a street-level point roughly 60-110 m from the office.  Use the
       spawn height as the reference so a roof is not selected; only the five
       preloaded tiles are searched (no RNG, so the simulation stays
       reproducible). */
    int bx = w->player->x + 40 * 1024, bz = w->player->z;
    int by = World_groundH(w, bx, bz, w->player->y);
    bool found = false;
    for (int dz = -120; dz <= 120 && !found; dz += 8) {
        for (int dx = -120; dx <= 120 && !found; dx += 8) {
            int adx = dx < 0 ? -dx : dx, adz = dz < 0 ? -dz : dz;
            int ad = adx > adz ? adx : adz;
            if (ad < 24 || ad > 40) continue;
            int x = w->player->x + dx * 1024;
            int z = w->player->z + dz * 1024;
            int g = World_groundH(w, x, z, w->player->y);
            if (g < w->player->y - 250 || g > w->player->y + 300) continue;
            int surf = World_surfAt(w, x, z);
            if (surf == WORLD_S_WATER) continue;
            if (!World_walkable(w, x, z, w->player->y)) continue;
            if (surf == WORLD_S_ROAD || surf == WORLD_S_PAVE) {
                bx = x; bz = z; by = g; found = true;
            } else if (!found) {
                bx = x; bz = z; by = g;
            }
        }
    }
    if (by < w->player->y - 250 || by > w->player->y + 300) by = w->player->y;
    w->missionX = bx;
    w->missionY = by;
    w->missionZ = bz;
}

void World_missionReset(World* w) {
    if (!w) return;
    makeMarker(w);
    w->missionTimer = 0;
    w->missionDelay = 1500;
    w->missionX = w->player->x;
    w->missionY = w->player->y;
    w->missionZ = w->player->z;
    if (w->missionDone) {
        w->missionState = WORLD_MISSION_PASSED;
        w->missionText[0] = 0;
    } else {
        w->missionState = WORLD_MISSION_AVAILABLE;
        snprintf(w->missionText, sizeof(w->missionText), "An Old Friend");
    }
}

static void startMission(World* w) {
    w->missionState = WORLD_MISSION_ACTIVE;
    w->missionTimer = MISSION_TIME_MS;
    pickDestination(w);
    snprintf(w->missionText, sizeof(w->missionText), "Go to the Ocean View Hotel");
    World_say(w, "Ken: Tommy, take a car and meet me at the Ocean View Hotel.");
}

static void failMission(World* w, const char* why) {
    w->missionState = WORLD_MISSION_AVAILABLE;
    w->missionTimer = 0;
    w->missionDelay = 3000;
    w->missionX = w->player->x;
    w->missionY = w->player->y;
    w->missionZ = w->player->z;
    snprintf(w->missionText, sizeof(w->missionText), "An Old Friend");
    World_say(w, why);
}

static void passMission(World* w) {
    w->missionDone = true;
    w->missionState = WORLD_MISSION_PASSED;
    w->missionTimer = 0;
    w->missionText[0] = 0;
    w->money += 100;
    World_say(w, "Mission passed! $100");
    Save_write(w);
}

void World_missionUpdate(World* w, int dt) {
    if (!w) return;
    if (w->missionState == WORLD_MISSION_PASSED) return;

    if (w->missionState == WORLD_MISSION_AVAILABLE) {
        if (w->missionDelay > 0) {
            w->missionDelay -= dt;
            return;
        }
        if (w->health > 0 && !w->busted) {
            startMission(w);
        }
        return;
    }

    if (w->missionState != WORLD_MISSION_ACTIVE) return;
    if (w->health <= 0 || w->busted) {
        failMission(w, "Mission failed. You were wasted or busted.");
        return;
    }
    w->missionTimer -= dt;
    if (w->missionTimer <= 0) {
        w->missionTimer = 0;
        failMission(w, "Mission failed. You ran out of time.");
        return;
    }
    if (Fx_dist(w->player->x - w->missionX, w->player->z - w->missionZ) < 5000) {
        passMission(w);
    }
}

void World_drawMissionMarker(World* w) {
    if (!w || !w->missionMarker || w->missionState == WORLD_MISSION_PASSED) return;
    if (w->missionState == WORLD_MISSION_AVAILABLE &&
        Fx_dist(w->player->x - w->missionX, w->player->z - w->missionZ) < 4000) {
        /* The player is standing on the office marker; a huge near-camera
           diamond would cover the screen.  The radar blip is enough until
           the mission starts. */
        return;
    }
    R3D* r = w->r;
    int dx = (w->missionX >> 4) - r->camX;
    int dy = w->missionY + 90 - r->camY;
    int dz = (w->missionZ >> 4) - r->camZ;
    if (dx > 8000 || dx < -8000 || dz > 8000 || dz < -8000 || dy > 8000 || dy < -8000) return;

    Fx_rotY(w->rYaw, w->time / 3);
    Fx_mul(r->mv, w->rYaw, w->mmA);
    int* mv = r->mv;
    int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
    int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
    int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
    bool oldAdd = r->additive;
    r->additive = true;
    R3D_drawMesh(r, w->missionMarker, w->mmA, tx, ty, tz, 0, 16384, 0, 0);
    r->additive = oldAdd;
}
