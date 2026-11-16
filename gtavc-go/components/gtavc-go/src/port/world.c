#include "world.h"
#include "tile_cache_stream.h"
#include "tile_loader.h"
#include "tile_install_policy.h"
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
#include <limits.h>
#include "fx_math.h"


int World_idx(World* w, int i, int j) {
    return (i < 0 || j < 0 || i >= w->tnx || j >= w->tnz) ? -1 : (j * w->tnx + i);
}

Tile* World_tileFor(World* w, int gi, int gj, int per) {
    if (gi < 0 || gj < 0) return NULL;
    int k = World_idx(w, gi / per, gj / per);
    return k < 0 ? NULL : w->tiles[k];
}

int World_hCell(World* w, int gi, int gj, int y) {
    Tile* t = World_tileFor(w, gi, gj, TILE_NH);
    if (!t || !t->top) return -640;
    int k = (gj % TILE_NH) * TILE_NH + (gi % TILE_NH);
    int a = t->top[k];
    if (a == -32768) return -640;
    int b = t->low ? t->low[k] : -32768;

    /* 1. Above water, the seabed (b < -70) is never a walkable floor.
          Bridges, piers, and docks over water always return the deck a. */
    if (b < -70 && y > -70) return a;

    /* 2. Two distinct layers: ceiling a and floor b. */
    if (b != -32768 && b != a && b > -70) {
        /* On the upper deck (driving on elevated highway, overpass, bridge, roof) */
        if (y > b + 90 && (a - y) < (y - b)) {
            return a;
        }
        /* Ceiling is overhead (inside underground parking, under archway/bridge) */
        if (a > y + 70) {
            return b;
        }
    }

    /* 3. Single-layer cell (palm tree crown, tree canopy, overhead prop):
          If a is far above the entity (> 2.5 m above y), the entity is
          underneath it on the ground. Never teleport or spawn onto treetops! */
    if (a > y + 160) {
        return y;
    }

    return a;
}

bool World_spawnClear(World* w, int xm, int zm, int radius) {
    static const int dir[9][2] = {
        { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
        { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 }
    };
    int py = w->player->y;
    for (int i = 0; i < 9; i++) {
        int x = xm + dir[i][0] * radius;
        int z = zm + dir[i][1] * radius;
        if (World_solidCell(w, World_scell(x, w->mox), World_scell(z, w->moz))) return false;
        /* Reject points under a building roof/overhang.  Bridge decks over
           water are valid; ordinary roofs are not. */
        if (!World_isBridgeAt(w, x, z)) {
            int fx = (x - w->mox - 1024) / 8;
            int fz = (z - w->moz - 1024) / 8;
            int gi = fx >> 8, gj = fz >> 8;
            Tile* t = World_tileFor(w, gi, gj, TILE_NH);
            if (t && t->top) {
                int k = (gj % TILE_NH) * TILE_NH + (gi % TILE_NH);
                if (t->top[k] > py + 160) return false;
            }
        }
    }
    return true;
}

bool World_isBridgeAt(World* w, int xm, int zm) {
    int fx = (xm - w->mox - 1024) / 8;
    int fz = (zm - w->moz - 1024) / 8;
    int gi = fx >> 8, gj = fz >> 8;
    Tile* t = World_tileFor(w, gi, gj, TILE_NH);
    if (!t || !t->top || !t->low || !t->surf) return false;
    int k = (gj % TILE_NH) * TILE_NH + (gi % TILE_NH);
    int top = t->top[k], low = t->low[k];
    return t->surf[k] == WORLD_S_ROAD && top > -70 && low < -70;
}

/* A bridge deck over water can be several metres above the approach; do not
   treat it as an overhead roof. */
static bool bridgeStep(World* w, int xm, int zm, int y, int maxStep) {
    if (!World_isBridgeAt(w, xm, zm)) return false;
    int g = World_groundH(w, xm, zm, y);
    if (g - y > maxStep && g - y <= 2000) return true;
    return false;
}

bool World_stepAllowed(World* w, int xm, int zm, int y, int maxStep) {
    int d = World_groundH(w, xm, zm, y + 40) - y;
    if (d <= maxStep) return true;
    return bridgeStep(w, xm, zm, y, maxStep);
}

int World_groundH(World* w, int xm, int zm, int y) {
    /* ((a << 8) / 2048) == a / 8, with the same truncation toward zero */
    int fx = (xm - w->mox - 1024) / 8;
    int fz = (zm - w->moz - 1024) / 8;
    int i = fx >> 8, j = fz >> 8;
    int ax = fx & 255, az = fz & 255;
    int h00 = World_hCell(w, i, j, y);
    int h10 = World_hCell(w, i + 1, j, y);
    int h01 = World_hCell(w, i, j + 1, y);
    int h11 = World_hCell(w, i + 1, j + 1, y);

    if (h00 == -640 || h10 == -640 || h01 == -640 || h11 == -640) {
        int v = h00;
        if (h10 > v) v = h10;
        if (h01 > v) v = h01;
        if (h11 > v) v = h11;
        if (v == -640) return -640;
        if (h00 == -640) h00 = v;
        if (h10 == -640) h10 = v;
        if (h01 == -640) h01 = v;
        if (h11 == -640) h11 = v;
    }

    int mn = h00, mx = h00;
    if (h10 < mn) mn = h10;
    if (h10 > mx) mx = h10;
    if (h01 < mn) mn = h01;
    if (h01 > mx) mx = h01;
    if (h11 < mn) mn = h11;
    if (h11 > mx) mx = h11;

    if (mx - mn > 128) {
        return ax < 128 ? (az < 128 ? h00 : h01) : (az < 128 ? h10 : h11);
    }
    int a = h00 + (((h10 - h00) * ax) >> 8);
    int b = h01 + (((h11 - h01) * ax) >> 8);
    return a + (((b - a) * az) >> 8);
}

int World_groundH2(World* w, int xm, int zm) {
    return World_groundH(w, xm, zm, 4000);
}

int World_surfAt(World* w, int xm, int zm) {
    if (xm < w->mox || zm < w->moz) return WORLD_S_WATER;
    int gi = (xm - w->mox) / 2048;
    int gj = (zm - w->moz) / 2048;
    Tile* t = World_tileFor(w, gi, gj, TILE_NH);
    if (!t || !t->surf) return WORLD_S_WATER;
    return t->surf[(gj % TILE_NH) * TILE_NH + (gi % TILE_NH)];
}

bool World_solidCell(World* w, int gi, int gj) {
    Tile* t = World_tileFor(w, gi, gj, TILE_NS);
    if (!t || !t->solid) return false;
    int li = gi % TILE_NS, lj = gj % TILE_NS;
    if ((t->solid[lj * TILE_SROW + (li >> 3)] & (0x80 >> (li & 7))) == 0) return false;

    /* A roadway, bridge, ramp, or parking deck is for driving: ignore geometry
       artifacts (bridge trusses, railings, ramp side flanks) on driveable surfaces. */
    int hi = gi / 2, hj = gj / 2;
    Tile* th = World_tileFor(w, hi, hj, TILE_NH);
    if (th && th->surf && th->top && th->low) {
        int k = (hj % TILE_NH) * TILE_NH + (hi % TILE_NH);
        int s = th->surf[k];
        if (s == WORLD_S_ROAD) return false;
        if (s != WORLD_S_WATER) {
            int diff = th->top[k] - th->low[k];
            if (diff >= 50 && diff <= 280) return false; /* elevated ramp / parking floor */
            if (diff < 50) {
                /* check 4 cardinal neighbors for a ramp slope leading to a ramp/deck */
                static const int nbr[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
                for (int n = 0; n < 4; n++) {
                    int ni = (hi % TILE_NH) + nbr[n][0], nj = (hj % TILE_NH) + nbr[n][1];
                    if (ni >= 0 && ni < TILE_NH && nj >= 0 && nj < TILE_NH) {
                        int nk = nj * TILE_NH + ni;
                        int ndiff = th->top[nk] - th->low[nk];
                        int d = th->top[nk] - th->top[k];
                        if (d > 35 && d <= 260 && ndiff <= 280) return false;
                    }
                }
            }
        }
    }
    return true;
}

int World_scell(int m, int o) {
    return m < o ? -1 : (m - o) / 1024;
}

bool World_deep(World* w, int xm, int zm, int y) {
    return World_groundH(w, xm, zm, y) < -70;
}

bool World_walkable(World* w, int xm, int zm, int y) {
    if (World_surfAt(w, xm, zm) == WORLD_S_WATER) return false;
    int g = World_groundH(w, xm, zm, y);
    if (g < -70 || g > y + 120) return false;
    return !World_solidCell(w, World_scell(xm, w->mox), World_scell(zm, w->moz));
}

bool World_resolve(World* w, int rad) {
    bool hit = false;
    int i0 = World_scell(w->rx - rad, w->mox), i1 = World_scell(w->rx + rad, w->mox);
    int j0 = World_scell(w->rz - rad, w->moz), j1 = World_scell(w->rz + rad, w->moz);

    for (int j = j0; j <= j1; j++) {
        for (int i = i0; i <= i1; i++) {
            if (!World_solidCell(w, i, j)) continue;
            int bx0 = w->mox + i * 1024, bz0 = w->moz + j * 1024;
            int bx1 = bx0 + 1024, bz1 = bz0 + 1024;
            int cx = w->rx < bx0 ? bx0 : (w->rx > bx1 ? bx1 : w->rx);
            int cz = w->rz < bz0 ? bz0 : (w->rz > bz1 ? bz1 : w->rz);
            int dx = w->rx - cx, dz = w->rz - cz;

            if (dx == 0 && dz == 0) {
                int l = w->rx - bx0, rr = bx1 - w->rx;
                int t = w->rz - bz0, bt = bz1 - w->rz;
                int m = l;
                if (rr < m) m = rr;
                if (t < m) m = t;
                if (bt < m) m = bt;
                if (m == l) w->rx = bx0 - rad;
                else if (m == rr) w->rx = bx1 + rad;
                else if (m == t) w->rz = bz0 - rad;
                else w->rz = bz1 + rad;
                hit = true;
            } else {
                int d2 = dx * dx + dz * dz;
                if (d2 < rad * rad) {
                    int d = (int)Fx_isqrtFast((uint64_t)d2);
                    if (d < 1) d = 1;
                    w->rx = cx + dx * rad / d;
                    w->rz = cz + dz * rad / d;
                    hit = true;
                }
            }
        }
    }
    return hit;
}

bool World_stepOk(World* w, int ox, int oz, int oy, int maxStep) {
    if (World_stepAllowed(w, w->rx, w->rz, oy, maxStep)) return true;
    int nx = w->rx, nz = w->rz;
    w->rx = nx; w->rz = oz;
    if (World_stepAllowed(w, w->rx, w->rz, oy, maxStep)) return true;
    w->rx = ox; w->rz = nz;
    if (World_stepAllowed(w, w->rx, w->rz, oy, maxStep)) return true;
    w->rx = ox; w->rz = oz;
    return false;
}

void World_buildFxMeshes(World* w) {
    int L = 70 * 64, bw = 7 * 64;
    w->beam = Mesh_new(5, 2);
    w->beam->x[0] = 0;   w->beam->y[0] = 0;   w->beam->z[0] = 0;
    w->beam->x[1] = -bw; w->beam->y[1] = 0;   w->beam->z[1] = L;
    w->beam->x[2] = bw;  w->beam->y[2] = 0;   w->beam->z[2] = L;
    w->beam->x[3] = 0;   w->beam->y[3] = -bw; w->beam->z[3] = L;
    w->beam->x[4] = 0;   w->beam->y[4] = bw;  w->beam->z[4] = L;

    int tiB[6] = { 0, 1, 2, 0, 3, 4 };
    memcpy(w->beam->ti, tiB, sizeof(tiB));
    w->beam->tex[0] = -1; w->beam->tex[1] = -1;
    w->beam->col[0] = 0x2A2618; w->beam->col[1] = 0x2A2618;
    w->beam->flag[0] = (int8_t)(MESH_F_DOUBLE | MESH_F_EMISSIVE);
    w->beam->flag[1] = (int8_t)(MESH_F_DOUBLE | MESH_F_EMISSIVE);
    Mesh_bounds(w->beam);

    int q = 64;
    w->blood = Mesh_new(4, 2);
    w->blood->x[0] = -q; w->blood->z[0] = -q;
    w->blood->x[1] = q;  w->blood->z[1] = -q;
    w->blood->x[2] = q;  w->blood->z[2] = q;
    w->blood->x[3] = -q; w->blood->z[3] = q;
    w->blood->u[0] = 0;   w->blood->v[0] = 0;
    w->blood->u[1] = 511; w->blood->v[1] = 0;
    w->blood->u[2] = 511; w->blood->v[2] = 511;
    w->blood->u[3] = 0;   w->blood->v[3] = 511;

    int tiBl[6] = { 0, 2, 1, 0, 3, 2 };
    memcpy(w->blood->ti, tiBl, sizeof(tiBl));
    int bt = Assets_acquirePath("/t_blood.png");
    w->blood->tex[0] = (int16_t)bt; w->blood->tex[1] = (int16_t)bt;
    w->blood->flag[0] = MESH_F_DOUBLE; w->blood->flag[1] = MESH_F_DOUBLE;
    w->blood->ny[0] = 127; w->blood->ny[1] = 127;
    Mesh_bounds(w->blood);
}

World* World_new(R3D* r) {
    World* w = (World*)calloc(1, sizeof(World));
    if (!w) return NULL;
    w->r = r;
    w->alive = true;
    w->player = &w->playerInst;
    Ped_init(w->player);
    w->camPitch = 55;
    w->camDistMm = 4300;
    w->minutes = 14 * 60 + 18;
    w->drawDist = 150;
    w->brightness = 10;
    w->speedMul = 100;
    w->npool = Assets_legacy ? 5 : WORLD_NPOOL;
    w->ncpool = Assets_legacy ? 4 : WORLD_NCPOOL;
    w->maxPed = Assets_legacy ? 10 : WORLD_MAXPED;
    w->maxTraffic = Assets_legacy ? 3 : WORLD_MAXTRAFFIC;
    w->ring = Assets_legacy ? 1 : 2;
    /* The original uses the high-detail tile mesh out to 200 m.  On the
       console the low-detail mesh saves a lot of triangles and is invisible
       at 160x120, so keep near detail around the player only. */
    w->nearLod = Assets_legacy ? 70 * 64 : (150 * 64 + 3200);
    const char* lod = getenv("GTAVC_LOD");
    if (lod) w->nearLod = atoi(lod) * 64;
    return w;
}

void World_loadIndex(World* w) {
    w->tileMm = 100 * 1024;
    w->tnx = 0;
    w->tnz = 0;
    int len = 0;
    uint8_t* b = Assets_readAll("/mapidx.bin", &len);
    if (!b || len < 18) {free(b);return;}
    DataInputStream d;
    dis_init(&d, b, len);
    uint8_t mg[4];
    dis_readFully(&d, mg, 4);
    w->mox = dis_readInt(&d);
    w->moz = dis_readInt(&d);
    w->tileMm = dis_readUnsignedShort(&d);
    int nx = dis_readUnsignedShort(&d);
    int nz = dis_readUnsignedShort(&d);
    w->tileMm = 100 * 1024;

    /* Bound sizes by available bytes and integer/addressable ranges, not by
       today's asset dimensions or magic/version. Trailing extensions are OK. */
    uint64_t tiles64 = (uint64_t)nx * nz;
    if (!nx || !nz || nx > INT_MAX / w->tileMm || nz > INT_MAX / w->tileMm ||
        tiles64 > INT_MAX / sizeof(Tile*)) goto fail;
    int totalTiles = (int)tiles64;
    int bitBytes = (totalTiles + 7) / 8;
    if (dis_available(&d) < 42 || bitBytes > dis_available(&d) - 42) goto fail;
    const uint8_t *bits = b + d.pos;
    d.pos += bitBytes;
    w->tflag = (uint8_t*)malloc((size_t)totalTiles);
    if (!w->tflag) goto fail;
    for (int k = 0; k < totalTiles; k++)
        w->tflag[k] = (bits[k >> 3] >> (7 - (k & 7))) & 1;

    dis_readUnsignedShort(&d);
    w->rx0 = dis_readInt(&d);
    w->rz0 = dis_readInt(&d);
    w->rcell = dis_readUnsignedShort(&d);
    w->rw = dis_readUnsignedShort(&d);
    w->rh = dis_readUnsignedShort(&d);

    uint64_t rad64 = (uint64_t)w->rw * w->rh;
    if (dis_available(&d) < 26 || rad64 > (uint64_t)(dis_available(&d) - 26)) goto fail;
    int radBytes = (int)rad64;
    w->rad = radBytes ? (uint8_t*)malloc((size_t)radBytes) : NULL;
    if (radBytes && !w->rad) goto fail;
    dis_readFully(&d, w->rad, radBytes);

    w->spX = dis_readInt(&d);
    w->spZ = dis_readInt(&d);
    w->spY = dis_readInt(&d);
    w->spYaw = dis_readUnsignedShort(&d);

    w->lampX = dis_readInt(&d);
    w->lampY = dis_readInt(&d);
    w->lampZ = dis_readInt(&d);
    dis_close(&d);
    free(b);

    w->tiles = (Tile**)calloc((size_t)totalTiles, sizeof(Tile*));
    w->requested = (bool*)calloc((size_t)totalTiles, sizeof(bool));
    if (!w->tiles || !w->requested) goto fail_arrays;
    w->tnx = nx;
    w->tnz = nz;
    World_buildFxMeshes(w);
    return;
fail:
    dis_close(&d);
    free(b);
fail_arrays:
    free(w->tflag); w->tflag = NULL;
    free(w->rad); w->rad = NULL;
    free(w->tiles); w->tiles = NULL;
    free(w->requested); w->requested = NULL;
    w->tnx = w->tnz = 0;

}

void World_preload(World* w, int x, int z) {
    if (!w->tiles || w->tileMm <= 0 || w->tnx <= 0 || w->tnz <= 0) return;
    World_tileJobCancel(w);
    if (w->tileWorkerEnabled) TileLoader_waitIdle();
    int ci = (x - w->mox) / w->tileMm;
    int cj = (z - w->moz) / w->tileMm;
    int ringLimit = w->ring;

    int totalTiles = w->tnx * w->tnz;
    for (int k = 0; k < totalTiles; k++) {
        Tile* t = w->tiles[k];
        if (!t) continue;
        if (abs(t->i - ci) > ringLimit || abs(t->j - cj) > ringLimit) {
            Tile_unload(t);
            Tile_free(t);
            w->tiles[k] = NULL;
        }
    }

    /* centre + four cardinal neighbours: enough for spawning and the first
       camera frame; the corners stream in after gameplay starts. */
    static const int off[5][2] = { {0,0}, {1,0}, {-1,0}, {0,1}, {0,-1} };
    for (int n = 0; n < 5; n++) {
        int i = ci + off[n][0], j = cj + off[n][1];
        int k = World_idx(w, i, j);
        if (k < 0 || w->tiles[k] != NULL || w->tflag[k] == 0) continue;
        Tile* t = Tile_load(i, j, w->mox, w->moz, w->tileMm);
        if (t) {
            Tile_install(t);
            w->tiles[k] = t;
        }
    }
}

void World_startLoader(World* w) {
    w->tileWorkerEnabled = TileLoader_start();
    if (!w->tileWorkerEnabled) RG_LOGW("Background loader unavailable; using cooperative loading");
    RG_LOGD("[STREAM] loader=%s", w->tileWorkerEnabled ? "background (one job, core 0, priority 2)" : "cooperative fallback");
}

static void tileJobReleaseInput(World* w) {
    if (w->tileJobFile) {
        fclose((FILE*)w->tileJobFile);
        w->tileJobFile = NULL;
    }
    Tile_cacheStreamFree(w->tileJobDecoder);
    w->tileJobDecoder = NULL;
    free(w->tileJobBuf);
    w->tileJobBuf = NULL;
    w->tileJobBuffered = w->tileJobCapacity = 0;
    w->tileJobLen = w->tileJobGot = 0;
}

void World_tileJobCancel(World* w) {
    if (!w) return;
    if (w->tileWorkerEnabled && w->tileJobActive && (!w->tileJobTile || w->tileTexturePending)) {
        TileLoader_cancel();
        GTAVC_PROF(g_prof.worker_cancelled++);
    }
    tileJobReleaseInput(w);
    Tile_free(w->tileJobTile);
    w->tileJobTile = NULL;
    w->tileTexturePending = w->tileTextureFallback = false;
    w->tileJobActive = false;
    w->tileInstallSlices = w->tileInstallUrgent = w->tileInstallPeakUs = 0;
    w->tileInstallWorkUs = 0;
}

#define TILE_READ_SLICE_BYTES 65536
#define TILE_RETRY_DELAY_US 2000000

static bool tileRetryReady(World* w, int k) {
    int64_t now = rg_system_timer();
    for (unsigned i = 0; i < sizeof(w->tileRetry) / sizeof(w->tileRetry[0]); i++) {
        if (w->tileRetry[i].k == k && w->tileRetry[i].until_us > now) {
            GTAVC_PROF(g_prof.tile_deferred++);
            return false;
        }
    }
    return true;
}

static void tileRetryLater(World* w, int k) {
    unsigned slot = 0;
    for (unsigned i = 0; i < sizeof(w->tileRetry) / sizeof(w->tileRetry[0]); i++) {
        if (w->tileRetry[i].k == k) { slot = i; break; }
        if (w->tileRetry[i].until_us < w->tileRetry[slot].until_us) slot = i;
    }
    w->tileRetry[slot].k = k;
    w->tileRetry[slot].until_us = rg_system_timer() + TILE_RETRY_DELAY_US;
}

static int tileJobStart(World* w, int i, int j, int k) {
    char path[600];
    const char* d = platform_assets_dir();
    snprintf(path, sizeof(path), "%s/tc/%d_%d.bin", (d && *d) ? d : "assets", i, j);
    if (w->tileWorkerEnabled) {
        TileLoadRequest request = {.i=i, .j=j, .k=k, .ox=w->mox, .oz=w->moz, .tileMm=w->tileMm};
        snprintf(request.path, sizeof(request.path), "%s", path);
        if (!TileLoader_submit(&request)) {GTAVC_PROF(g_prof.worker_busy_ticks++); return -1;}
        w->tileJobI=i; w->tileJobJ=j; w->tileJobK=k;
        w->tileJobActive=true; w->tileJobStartedUs=rg_system_timer();
        GTAVC_DETAIL_LOG("[STREAM] queued tile=%d,%d worker=1", i, j);
        return 1;
    }
    int64_t open_start = GTAVC_PROFILE_TIMER();
    FILE* f = fopen(path, "rb");
    if (!f) {
        gtavc_prof_stage(PROF_TILE_OPEN, open_start);
        GTAVC_PROF(g_prof.tile_open_failed++);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    gtavc_prof_stage(PROF_TILE_OPEN, open_start);
    if (len <= 0 || len > 4 * 1024 * 1024) { fclose(f); return 0; }
    int64_t alloc_start = GTAVC_PROFILE_TIMER();
    int capacity = len < TILE_READ_SLICE_BYTES ? (int)len : TILE_READ_SLICE_BYTES;
    uint8_t* buf = (uint8_t*)malloc((size_t)capacity);
    TileCacheStream *decoder = buf ? Tile_cacheStreamCreate((size_t)len, i, j,
                                    w->mox, w->moz, w->tileMm) : NULL;
    gtavc_prof_stage(PROF_TILE_ALLOC, alloc_start);
    if (!decoder) {
        free(buf);
        RG_LOGW("Not enough memory to load map tile %d,%d", i, j);
        GTAVC_PROF(g_prof.tile_oom++);
        fclose(f);
        tileRetryLater(w, k);
        return -1;
    }
    w->tileJobFile = f;
    w->tileJobBuf = buf;
    w->tileJobDecoder = decoder;
    w->tileJobCapacity = capacity;
    w->tileJobBuffered = 0;
    w->tileJobLen = (int)len;
    w->tileJobGot = 0;
    w->tileJobI = i;
    w->tileJobJ = j;
    w->tileJobK = k;
    w->tileJobActive = true;
    w->tileJobStartedUs = rg_system_timer();
    GTAVC_DETAIL_LOG("[STREAM] start tile=%d,%d bytes=%d staging=%d incremental=1", i, j, (int)len, capacity);
    return 1;
}

static void tileJobFinish(World* w, Tile* t) {
    int k = w->tileJobK;
    int tile_i = w->tileJobI, tile_j = w->tileJobJ;
    int64_t job_start = w->tileJobStartedUs;
    if (t) {
        GTAVC_DETAIL_LOG("[STREAM] install tile=%d,%d slices=%u urgent=%u work_ms=%.1f peak_ms=%.1f",
                tile_i, tile_j, (unsigned)w->tileInstallSlices,
                (unsigned)w->tileInstallUrgent, w->tileInstallWorkUs/1000.0,
                w->tileInstallPeakUs/1000.0);
    }
    w->tileJobTile = NULL; /* transfer to world below, not cancellation cleanup */
    w->tileJobActive = false; /* completion, not cancellation */
    World_tileJobCancel(w);
    if (t) {
        unsigned compact_meshes = 0, saved_bytes = 0;
        for (int far = 0; far < 2; far++) {
            Mesh **chunks = far ? t->farChunks : t->nearChunks;
            int count = far ? t->farChunkCount : t->nearChunkCount;
            for (int n = 0; n < count; n++) {
                Mesh *mesh = chunks[n];
                if (mesh->vertices16) {
                    compact_meshes++;
                    saved_bytes += (unsigned)mesh->nv * 10u + (unsigned)mesh->nt * 6u;
                }
            }
        }
        GTAVC_PROF(g_prof.tile_installed++);
        w->tiles[k] = t;
        GTAVC_DETAIL_LOG("[STREAM] compact_meshes=%u array_bytes_saved=%u", compact_meshes, saved_bytes);
    }
    GTAVC_DETAIL_LOG("[STREAM] %s tile=%d,%d elapsed_ms=%.0f player_tile=%d,%d",
            t ? "ready" : "failed", tile_i, tile_j,
            (double)(rg_system_timer() - job_start) / 1000.0,
            (w->player->x - w->mox) / w->tileMm,
            (w->player->z - w->moz) / w->tileMm);
}

static int tileJobInstallBudget(World* w, Tile* tile) {
    int budget = Tile_installBudgetUs(tile->i, tile->j, w->tileMm, w->mox, w->moz,
                                      w->player->x, w->player->z);
    if (!budget || w->tileMm <= 0) return 0;
    /* Don't let a distant active job hold up any missing terrain near the
       player. Flush this job so the existing priority queue can advance. */
    int pi = (w->player->x - w->mox) / w->tileMm;
    int pj = (w->player->z - w->moz) / w->tileMm;
    for (int j = pj - 1; j <= pj + 1; j++) {
        for (int i = pi - 1; i <= pi + 1; i++) {
            int k = World_idx(w, i, j);
            if (k >= 0 && w->tflag[k] && !w->tiles[k] &&
                !Tile_installBudgetUs(i, j, w->tileMm, w->mox, w->moz,
                                      w->player->x, w->player->z)) return 0;
        }
    }
    return budget;
}

static void tileJobQueueTextures(World* w) {
    Tile *tile=w->tileJobTile;
    TileTextureRequest request={0};
    int room=TILE_TEXTURE_BATCH;
    for(int k=tile->installNextTexture;k<tile->texCount;k++) if(tile->texPix[k]) room--;
    /* A resident entry skipped by a prior batch may have lost its last owner
       while we waited. Retained later pixels still count against the bound.
       Reserve one entry for forward progress in a completely full staging set. */
    for(int k=tile->texCount-1;room<=0 && k>=tile->installNextTexture;k--) {
        if(tile->texPix[k]) {free(tile->texPix[k]);tile->texPix[k]=NULL;room++;}
    }
    const char *dir=platform_assets_dir();
    snprintf(request.path,sizeof(request.path),"%s/x/xatlas.bin",(dir&&*dir)?dir:"assets");
    for(int k=tile->installNextTexture;k<tile->texCount && request.count<room;k++) {
        if(tile->texPix[k] || Assets_find(tile->texPath[k])>=0) continue;
        const char *path=tile->texPath[k];char *end=NULL;
        if(!path || strncmp(path,"/x/",3) || path[3]<'0' || path[3]>'9') {
            w->tileTextureFallback=true;return;
        }
        long id=strtol(path+3,&end,10);
        if(id<0 || id>=8192 || strcmp(end,".png")) {w->tileTextureFallback=true;return;}
        int n=request.count++;
        request.indices[n]=k;request.ids[n]=(int)id;
    }
    if(request.count && TileLoader_submitTextures(&request)) w->tileTexturePending=true;
}

static void tileJobInstallStep(World* w) {
    Tile *tile = w->tileJobTile;
    if(w->tileTexturePending) {
        TileTextureResult result;
        if(!TileLoader_pollTextures(&result)) return;
        w->tileTexturePending=false;
        GTAVC_DETAIL_LOG("[STREAM] textures tile=%d,%d count=%d status=%d bytes=%u elapsed_ms=%.1f open_ms=%.1f read_ms=%.1f peak_ms=%.1f (background)",
                tile->i,tile->j,result.count,result.status,(unsigned)result.bytes_read,
                result.elapsed_us/1000.0,result.open_us/1000.0,result.read_us/1000.0,result.read_peak_us/1000.0);
        if(result.status==TL_OK) {
            for(int n=0;n<result.count;n++) {
                int k=result.indices[n];
                if(k>=tile->installNextTexture && k<tile->texCount && !tile->texPix[k]) {
                    tile->texPix[k]=result.pixels[n];result.pixels[n]=NULL;
                    tile->texW[k]=tile->texH[k]=32;
                }
            }
        } else {
            /* Preserve alternate/missing data behavior and memory-pressure
               recovery using the previous game-thread installation path. */
            w->tileTextureFallback=true;
        }
        TileLoader_freeTextures(&result);
    }
    int budget = tileJobInstallBudget(w, tile);
    int64_t start = rg_system_timer();
    bool prepared = w->tileWorkerEnabled && !w->tileTextureFallback;
    bool done = prepared ? Tile_installPreparedStep(tile,budget) : Tile_installStep(tile,budget);
    gtavc_prof_stage(PROF_TILE_INSTALL, start);
    uint32_t elapsed = (uint32_t)(rg_system_timer() - start);
    w->tileInstallSlices++;
    if (!budget) w->tileInstallUrgent++;
    w->tileInstallWorkUs += elapsed;
    if (elapsed > w->tileInstallPeakUs) w->tileInstallPeakUs = elapsed;
    if (done) tileJobFinish(w, tile);
    else if(prepared && tile->installNextTexture<tile->texCount &&
            !tile->texPix[tile->installNextTexture] &&
            Assets_find(tile->texPath[tile->installNextTexture])<0) tileJobQueueTextures(w);
}

static void tileJobBeginInstall(World* w, Tile* tile) {
    /* The cache decoder has transferred ownership. Release its input memory
       before retaining the private tile across installation frames. */
    tileJobReleaseInput(w);
    if (!tile) RG_LOGW("Map tile %d,%d could not be loaded", w->tileJobI, w->tileJobJ);
    w->tileJobTile = tile;
    if (tile) tileJobInstallStep(w);
    else tileJobFinish(w, NULL);
}

/* One buffered read and incremental decode per tick. Incomplete records
   carry at most 14 bytes; the entire encoded cache is never retained. */
static void tileJobStep(World* w) {
    if (!w->tileJobActive) return;
    if (w->tileJobTile) {tileJobInstallStep(w); return;}
    if (w->tileWorkerEnabled) {
        TileLoadResult result;
        if (!TileLoader_poll(&result)) {GTAVC_PROF(g_prof.worker_wait_ticks++); return;}
        GTAVC_PROF(g_prof.worker_jobs++);
        GTAVC_PROF(g_prof.worker_open_us += result.stats.open_us);
        GTAVC_PROF(g_prof.worker_read_us += result.stats.read_us);
        GTAVC_PROF(g_prof.worker_decode_us += result.stats.decode_us);
        GTAVC_PROF(g_prof.tile_read_bytes += result.stats.bytes_read);
        if (result.i != w->tileJobI || result.j != w->tileJobJ || result.k != w->tileJobK) {
            Tile_free(result.tile);
            World_tileJobCancel(w);
            return;
        }
        Tile* tile = result.tile;
        if (result.status == TL_NO_CACHE) {
            GTAVC_PROF(g_prof.tile_open_failed++);
            w->tileTextureFallback=true; /* raw loader already prepares its pixels */
            /* Raw fallback uses shared asset state: keep it on the game task. */
            GTAVC_TIME(PROF_TILE_FALLBACK, tile = Tile_load(result.i, result.j,
                                       w->mox, w->moz, w->tileMm));
        } else if (result.status == TL_ALLOC_ERROR) GTAVC_PROF(g_prof.tile_oom++);
        else if (result.status == TL_IO_ERROR) GTAVC_PROF(g_prof.tile_read_failed++);
        else if (result.status != TL_OK) GTAVC_PROF(g_prof.tile_parse_failed++);
        if (!tile) tileRetryLater(w, w->tileJobK);
        GTAVC_DETAIL_LOG("[STREAM] worker tile=%d,%d status=%d bytes=%u open_ms=%.1f read_ms=%.1f decode_ms=%.1f",
                result.i, result.j, result.status, (unsigned)result.stats.bytes_read,
                result.stats.open_us/1000.0, result.stats.read_us/1000.0, result.stats.decode_us/1000.0);
        GTAVC_DETAIL_LOG("[STREAM] worker elapsed_ms=%.1f read_peak_ms=%.1f decode_peak_ms=%.1f",
                result.elapsed_us/1000.0, result.stats.read_peak_us/1000.0, result.stats.decode_peak_us/1000.0);
        tileJobBeginInstall(w, tile);
        return;
    }
    int left = w->tileJobLen - w->tileJobGot;
    int room = w->tileJobCapacity - w->tileJobBuffered;
    int chunk = left < room ? left : room;
    if (chunk > 0) {
        int64_t read_start = GTAVC_PROFILE_TIMER();
        size_t n = fread(w->tileJobBuf + w->tileJobBuffered, 1, (size_t)chunk,
                         (FILE*)w->tileJobFile);
        gtavc_prof_stage(PROF_TILE_READ, read_start);
        if (n == 0) {
            RG_LOGW("Read failed for map tile %d,%d", w->tileJobI, w->tileJobJ);
            GTAVC_PROF(g_prof.tile_read_failed++);
            tileRetryLater(w, w->tileJobK);
            World_tileJobCancel(w);
            return;
        }
        GTAVC_PROF(g_prof.tile_read_bytes += (uint32_t)n);
        w->tileJobGot += (int)n;
        w->tileJobBuffered += (int)n;
    }
    size_t used = 0;
    int64_t parse_start = GTAVC_PROFILE_TIMER();
    int status = Tile_cacheStreamFeed(w->tileJobDecoder, w->tileJobBuf,
                                     (size_t)w->tileJobBuffered, &used);
    gtavc_prof_stage(PROF_TILE_PARSE, parse_start);
    if (status == 0 && w->tileJobGot < w->tileJobLen && chunk > 0) {
        w->tileJobBuffered -= (int)used;
        memmove(w->tileJobBuf, w->tileJobBuf + used, (size_t)w->tileJobBuffered);
        return;
    }
    Tile *t = status == 1 ? Tile_cacheStreamTake(w->tileJobDecoder) : NULL;
    if (!t) {
        GTAVC_PROF(g_prof.tile_parse_failed++);
        tileRetryLater(w, w->tileJobK);
    }
    tileJobBeginInstall(w, t);
}

void World_streamTiles(World* w, int x, int z) {
    if (!w->tiles || w->tileMm <= 0 || w->tnx <= 0 || w->tnz <= 0) return;

    /* Compute forward-biased streaming center:
       Look ahead 38 meters along camera heading so the next tile starts
       streaming BEFORE the player reaches the tile boundary. */
    int fx = Fx_sin(w->camYaw);
    int fz = Fx_cos(w->camYaw);
    int lookX = x + ((fx * 38000) >> 14);
    int lookZ = z + ((fz * 38000) >> 14);

    int ci = (lookX - w->mox) / w->tileMm;
    int cj = (lookZ - w->moz) / w->tileMm;
    int ringLimit = w->ring;

    if (ci < 0) ci = 0;
    else if (ci >= w->tnx) ci = w->tnx - 1;
    if (cj < 0) cj = 0;
    else if (cj >= w->tnz) cj = w->tnz - 1;

    /* Player's actual tile (never unload the tile the player is physically on) */
    int playerI = (x - w->mox) / w->tileMm;
    int playerJ = (z - w->moz) / w->tileMm;

    /* 1. Unload out-of-range tiles FIRST before attempting to allocate new tiles.
          On memory-constrained targets (8MB PSRAM), old tiles must be freed to
          reclaim memory for newly streaming tiles. */
    int totalTiles = w->tnx * w->tnz;
    for (int k = 0; k < totalTiles; k++) {
        Tile* t = w->tiles[k];
        if (!t) continue;
        if (t->i == playerI && t->j == playerJ) continue;
        if (abs(t->i - ci) > ringLimit || abs(t->j - cj) > ringLimit) {
            Tile_unload(t);
            Tile_free(t);
            w->tiles[k] = NULL;
            /* Reclaimed memory may make a previously failed tile affordable. */
            memset(w->tileRetry, 0, sizeof(w->tileRetry));
        }
    }

    /* 2. Step active tile job if in progress */
    if (w->tileJobActive) {
        if (abs(w->tileJobI - ci) > ringLimit || abs(w->tileJobJ - cj) > ringLimit) {
            World_tileJobCancel(w);
        } else {
            tileJobStep(w);
            return;
        }
    }

    /* 3. Prioritized loading:
          - Ring 0: tile ahead (ci, cj)
          - Ring 1 Pass 1: cardinal neighbours (straight road ahead/behind/sides)
          - Ring 1 Pass 2: diagonal corners */
    int loadedOne = 0;
    for (int ring = 0; ring <= ringLimit && !loadedOne; ring++) {
        if (ring == 0) {
            int k = World_idx(w, ci, cj);
            if (k >= 0 && w->tflag[k] != 0 && w->tiles[k] == NULL && tileRetryReady(w, k)) {
                int res = tileJobStart(w, ci, cj, k);
                if (res != 0) return; /* 1: started, -1: OOM retry later */
                Tile* t;
                GTAVC_TIME(PROF_TILE_FALLBACK, t = Tile_load(ci, cj, w->mox, w->moz, w->tileMm));
                if (t) {
                    GTAVC_TIME(PROF_TILE_INSTALL, Tile_install(t));
                    GTAVC_PROF(g_prof.tile_installed++);
                    w->tiles[k] = t;
                } else {
                    tileRetryLater(w, k);
                }
                loadedOne = 1;
            }
        } else {
            for (int pass = 1; pass <= 2 && !loadedOne; pass++) {
                for (int j = cj - ring; j <= cj + ring && !loadedOne; j++) {
                    for (int i = ci - ring; i <= ci + ring && !loadedOne; i++) {
                        int di = abs(i - ci), dj = abs(j - cj);
                        if (di > 1 || dj > 1) continue;
                        if ((pass == 1 && di + dj != 1) || (pass == 2 && di + dj != 2)) continue;
                        int k = World_idx(w, i, j);
                        if (k < 0 || w->tflag[k] == 0 || w->tiles[k] != NULL) continue;
                        if (!tileRetryReady(w, k)) continue;
                        int res = tileJobStart(w, i, j, k);
                        if (res != 0) return; /* 1: started, -1: OOM retry later */
                        Tile* t;
                        GTAVC_TIME(PROF_TILE_FALLBACK, t = Tile_load(i, j, w->mox, w->moz, w->tileMm));
                        if (t) {
                            GTAVC_TIME(PROF_TILE_INSTALL, Tile_install(t));
                            GTAVC_PROF(g_prof.tile_installed++);
                            w->tiles[k] = t;
                        } else {
                            tileRetryLater(w, k);
                        }
                        loadedOne = 1;
                    }
                }
            }
        }
    }
}

void World_free(World* w) {
    if (!w) return;
    World_tileJobCancel(w);
    if (w->tileWorkerEnabled) TileLoader_waitIdle();
    int totalTiles = w->tnx * w->tnz;
    if (w->tiles) {
        for (int k = 0; k < totalTiles; k++) {
            if (w->tiles[k]) {
                Tile_unload(w->tiles[k]);
                Tile_free(w->tiles[k]);
            }
        }
        free(w->tiles);
    }
    free(w->tflag);
    free(w->requested);
    free(w->rad);
    if (w->pedNames) Assets_freeLines(w->pedNames, w->pedNamesCount);
    free(w->pedFlags);
    if (w->carNames) Assets_freeLines(w->carNames, w->carNamesCount);
    for (int i = 0; i < WORLD_NPOOL; i++) {
        if (w->pool[i]) PedModel_free(w->pool[i]);
        if (w->poolLo[i]) PedModel_free(w->poolLo[i]);
    }
    if (w->pm0) PedModel_free(w->pm0);
    for (int i = 0; i < WORLD_NCPOOL; i++) {
        if (w->cpool[i]) CarModel_free(w->cpool[i]);
    }
    for (int i = 1; i < 3; i++) if (w->weaponMesh[i]) Mesh_free(w->weaponMesh[i]);
    if (w->beam) Mesh_free(w->beam);
    if (w->blood) Mesh_free(w->blood);
    if (w->cashMesh) Mesh_free(w->cashMesh);
    if (w->missionMarker) Mesh_free(w->missionMarker);
    if (w->hdl) Assets_freeLines(w->hdl, w->hdlCount);
    free(w);
}
