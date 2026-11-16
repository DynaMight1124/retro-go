#include "world.h"
#include "world_internal.h"
#include "assets.h"
#include "anim_data.h"
#include "fx.h"
#include "j2me.h"
#include "gtavc_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* SPORTS_STR = " banshee cheetah infernus comet stinger deluxo pheonix sabretur hotring hotrina hotrinb bloodra bloodrb vicechee cuban sentxs ";
static const char* HEAVY_STR  = " benson boxville mule packer linerun flatbed trash barracks firetruk bus coach securica enforcer ambulan yankee rumpo pony burrito spand topfun mrwhoop walton bobcat rancher landstal patriot sandking fbiranch moonbeam ";
static const char* BIKE_STR   = " angel freeway pcj600 sanchez faggio pizzaboy ";
static const char* SLOW_STR   = " caddy baggage ";

int World_pools_findStr(char** a, int count, const char* s) {
    if (!a || !s) return 0;
    for (int i = 0; i < count; i++) {
        if (a[i] && strcmp(a[i], s) == 0) return i;
    }
    return 0;
}

static int randomAmbient(World* w) {
    for (int k = 0; k < 40; k++) {
        int i = Fx_rndBound(w->pedNamesCount);
        if ((w->pedFlags[i] & 14) == 0) return i;
    }
    return 0;
}

static void loadPed(World* w, int slot, int src) {
    int64_t load_start = rg_system_timer();
    if (w->pool[slot] != NULL) {
        Assets_release(w->poolTex[slot]);
        PedModel_free(w->pool[slot]);
        w->pool[slot] = NULL;
    }
    if (w->poolLo[slot] != NULL) {
        PedModel_free(w->poolLo[slot]);
        w->poolLo[slot] = NULL;
    }
    const char* n = w->pedNames[src];
    char p1[64], p2[64], pt[64];
    snprintf(p1, sizeof(p1), "/p_%s.bin", n);
    snprintf(p2, sizeof(p2), "/p_%s_lo.bin", n);
    snprintf(pt, sizeof(pt), "/t_p_%s.png", n);

    w->pool[slot] = PedModel_load(p1);
    w->poolLo[slot] = PedModel_load(p2);
    w->poolTex[slot] = Assets_acquirePath(pt);
    w->poolSrc[slot] = src;
    gtavc_prof_stage(PROF_MODEL_LOAD, load_start);
}

static void handling(World* w, CarModel* m) {
    if (w->hdl == NULL) {
        w->hdl = Assets_readLines("/handling.txt", &w->hdlCount);
    }
    char k[128];
    snprintf(k, sizeof(k), " %s ", m->name);
    int mv = 160, ac = 22, lk = 32, mass = 1500, grip = 100, cls = 0;

    if (strstr(BIKE_STR, k) != NULL) {
        mv = (strcmp(m->name, "faggio") == 0 || strcmp(m->name, "pizzaboy") == 0) ? 100 : 180;
        ac = 30; lk = 38; mass = 350; grip = 130; cls = 3;
    } else if (strstr(SPORTS_STR, k) != NULL) {
        mv = 200; ac = 30; lk = 30; mass = 1300; grip = 125; cls = 1;
    } else if (strstr(HEAVY_STR, k) != NULL) {
        mv = 130; ac = 15; lk = 30; mass = 3500; grip = 85; cls = 2;
    } else if (strstr(SLOW_STR, k) != NULL) {
        mv = 60; ac = 12; lk = 40; mass = 500; grip = 110; cls = 4;
    }
    if (strcmp(m->name, "police") == 0 || strcmp(m->name, "fbicar") == 0) {
        mv = 190; ac = 26;
    }
    m->vmax = mv * 11 / 36; m->accel = ac * 30; m->brake = 1100;
    m->lockDeg = lk; m->mass = mass; m->grip = grip; m->cls = cls;

    if (w->hdl) {
        char prefix[128];
        snprintf(prefix, sizeof(prefix), "%s ", m->name);
        size_t prefLen = strlen(prefix);
        for (int i = 0; i < w->hdlCount; i++) {
            if (strncmp(w->hdl[i], prefix, prefLen) != 0) continue;
            int v[6];
            const char* p = w->hdl[i] + prefLen;
            for (int f = 0; f < 6; f++) {
                while (*p == ' ') p++;
                v[f] = atoi(p);
                while (*p && *p != ' ') p++;
            }
            m->mass = v[0];
            m->grip = (v[1] > 95 ? v[1] : 95);
            m->vmax = v[2] * 11 / 36;
            m->accel = v[3] * 3;
            m->brake = v[4] + 200;
            m->lockDeg = v[5];
            break;
        }
    }
    m->wheelbase = m->dimZ * 16 * 6 / 10;
    if (m->wheelbase < 900) m->wheelbase = 900;
}

void World_pools_loadCar(World* w, int slot, int src) {
    int64_t load_start = rg_system_timer();
    if (w->cpool[slot] != NULL) {
        CarModel_unload(w->cpool[slot]);
        CarModel_free(w->cpool[slot]);
        w->cpool[slot] = NULL;
    }
    CarModel* m = CarModel_load(w->carNames[src]);
    if (m) {
        handling(w, m);
        w->cpool[slot] = m;
        w->cpoolSrc[slot] = src;
    }
    gtavc_prof_stage(PROF_MODEL_LOAD, load_start);
}

static Mesh* loadWeapon(const char* n) {
    char path[64];
    snprintf(path, sizeof(path), "/w_%s.bin", n);
    int len = 0;
    uint8_t* b = Assets_readAll(path, &len);
    if (!b) return NULL;
    DataInputStream d;
    dis_init(&d, b, len);
    int nt = 0;
    int* t = (int*)malloc(32 * sizeof(int)); // readTex internally does allocate
    int count = dis_readUnsignedByte(&d);
    for (int i = 0; i < count; i++) {
        int l = dis_readUnsignedByte(&d);
        char buf[256];
        dis_readFully(&d, (uint8_t*)buf, l);
        buf[l] = '\0';
        char tpath[300];
        snprintf(tpath, sizeof(tpath), "/t_%s.png", buf);
        t[i] = Assets_acquirePath(tpath);
    }
    nt = count;

    int nv = dis_readUnsignedShort(&d);
    int* x = (int*)malloc((size_t)nv * sizeof(int));
    int* y = (int*)malloc((size_t)nv * sizeof(int));
    int* z = (int*)malloc((size_t)nv * sizeof(int));
    int* u = (int*)malloc((size_t)nv * sizeof(int));
    int* v = (int*)malloc((size_t)nv * sizeof(int));
    for (int i = 0; i < nv; i++) {
        x[i] = dis_readShort(&d); y[i] = dis_readShort(&d); z[i] = dis_readShort(&d);
        u[i] = dis_readShort(&d); v[i] = dis_readShort(&d);
    }
    int mnt = dis_readUnsignedShort(&d);
    int* ta = (int*)malloc((size_t)mnt * sizeof(int));
    int* tb = (int*)malloc((size_t)mnt * sizeof(int));
    int* tc = (int*)malloc((size_t)mnt * sizeof(int));
    int* tex = (int*)malloc((size_t)mnt * sizeof(int));
    int* fl = (int*)malloc((size_t)mnt * sizeof(int));
    int* col = (int*)malloc((size_t)mnt * sizeof(int));
    for (int i = 0; i < mnt; i++) {
        ta[i] = dis_readUnsignedShort(&d); tb[i] = dis_readUnsignedShort(&d); tc[i] = dis_readUnsignedShort(&d);
        tex[i] = dis_readByte(&d); fl[i] = dis_readUnsignedByte(&d);
        col[i] = (dis_readUnsignedByte(&d) << 16) | (dis_readUnsignedByte(&d) << 8) | dis_readUnsignedByte(&d);
    }
    Mesh* m = Mesh_new(nv, mnt);
    for (int i = 0; i < nv; i++) {
        m->x[i] = x[i]; m->y[i] = y[i]; m->z[i] = z[i];
        m->u[i] = u[i]; m->v[i] = v[i];
    }
    for (int i = 0; i < mnt; i++) {
        m->ti[i * 3]     = ta[i];
        m->ti[i * 3 + 1] = tb[i];
        m->ti[i * 3 + 2] = tc[i];
        m->tex[i] = (int16_t)(tex[i] >= 0 && tex[i] < nt ? t[tex[i]] : -1);
        m->col[i] = col[i];
        int f = 0;
        if ((fl[i] & 2) != 0) f |= MESH_F_EMISSIVE;
        if ((fl[i] & 4) != 0) f |= MESH_F_TINT;
        if ((fl[i] & 16) != 0) f |= MESH_F_TINT2;
        m->flag[i] = (int8_t)f;
        CarModel_normal(m, i, ta[i], tb[i], tc[i]);
    }
    Mesh_bounds(m);
    free(x); free(y); free(z); free(u); free(v);
    free(ta); free(tb); free(tc); free(tex); free(fl); free(col);
    free(t);
    dis_close(&d);
    free(b);
    return m;
}

void World_loadPools(World* w) {
    w->pedNames = Assets_readLines("/peds.lst", &w->pedNamesCount);
    w->pedFlags = (int*)malloc((size_t)w->pedNamesCount * sizeof(int));
    for (int i = 0; i < w->pedNamesCount; i++) {
        char* sp = strchr(w->pedNames[i], ' ');
        if (sp) {
            *sp = '\0';
            w->pedFlags[i] = atoi(sp + 1);
        } else {
            w->pedFlags[i] = 0;
        }
    }
    w->carNames = Assets_readLines("/cars.lst", &w->carNamesCount);
    w->pm0 = PedModel_load("/p_player.bin");
    w->playerTex = Assets_acquirePath("/t_p_player.png");

    w->copSlot = 0;
    loadPed(w, 0, World_pools_findStr(w->pedNames, w->pedNamesCount, "cop"));
    for (int i = 1; i < w->npool; i++) loadPed(w, i, randomAmbient(w));

    w->policeSlot = 0;
    World_pools_loadCar(w, 0, World_pools_findStr(w->carNames, w->carNamesCount, "police"));
    const char* startCars[9] = {
        "admiral", "taxi", "pcj600", "stallion", "cheetah",
        "sentinel", "faggio", "pony", "voodoo"
    };
    for (int i = 1; i < w->ncpool; i++) {
        World_pools_loadCar(w, i, World_pools_findStr(w->carNames, w->carNamesCount, startCars[i - 1]));
    }
    w->weaponMesh[1] = loadWeapon("bat");
    w->weaponMesh[2] = loadWeapon("colt45");
}

void World_pools_rotatePools(World* w, int dt) {
    /* The Java build rotates one ped and one car model every 40 s.  On this
       target that releases textures still referenced by existing pedestrians
       and reloads large models in the middle of gameplay (visible as peds
       disappearing and as multi-second hitches).  The pools are small enough
       to keep resident, so rotation is disabled. */
    (void)w;
    (void)dt;
    return;
    w->poolT += dt;
    if (w->poolT < 40000) return;
    w->poolT = 0;

    int s = 1 + Fx_rndBound(w->npool - 1);
    bool used = false;
    for (int i = 0; i < w->nped; i++) {
        if (w->peds[i].kind == s && (w->peds[i].state != PED_DEAD || w->peds[i].deadT >= 0)) used = true;
    }
    if (!used) loadPed(w, s, randomAmbient(w));

    int c = 1 + Fx_rndBound(w->ncpool - 1);
    used = false;
    for (int i = 0; i < w->ncars; i++) {
        if (w->cars[i].type == c) used = true;
    }
    if (!used) World_pools_loadCar(w, c, Fx_rndBound(w->carNamesCount));
}

void World_assignKind(World* w, Ped* p, int slot) {
    /* The legacy pool has only five models, while the spawner used to pick
       a slot from the full WORLD_NPOOL range.  Slots without a model made
       pedestrians invisible, which is why the density option appeared not
       to work. */
    if (slot < 0 || slot >= w->npool || w->pool[slot] == NULL) {
        int fallback = 0;
        for (int i = 1; i < w->npool; i++) {
            if (w->pool[i] != NULL) { fallback = i; break; }
        }
        slot = fallback;
    }
    p->kind = slot;
    p->tex = (slot >= 0 && slot < WORLD_NPOOL && w->poolTex[slot] >= 0) ? w->poolTex[slot] : -1;
    if (!w->pedFlags || w->pedNamesCount <= 0) {
        p->female = false;
        p->gang = false;
        return;
    }
    int src = w->poolSrc[slot];
    if (src < 0 || src >= w->pedNamesCount) src = 0;
    int f = w->pedFlags[src];
    p->female = (f & 1) != 0;
    p->gang = (f & 4) != 0;
}

void World_initClips(World* w) {
    w->cIdle = AnimData_find("IDLE_stance");
    w->cWalk = AnimData_find("WALK_player");
    w->cWalkB = AnimData_find("walk_back");
    w->cRun = AnimData_find("run_player");
    w->cCiviWalk = AnimData_find("WALK_civi");
    w->cCiviRun = AnimData_find("run_civi");
    w->cWomanWalk = AnimData_find("WOMAN_walknorm");
    w->cWomanRun = AnimData_find("woman_run");
    w->cWomanIdle = AnimData_find("woman_idlestance");
    /* FIGHTppunch in the shipped ped.ifp is a static side-lying pose (all
       nine exported frames are identical), which made Tommy horizontal during
       the bat attack.  Use the animated upright heavy punch instead. */
    w->cPunch = AnimData_find("FIGHTpunch");
    w->cJab = AnimData_find("FIGHTjab");
    w->cHeavy = AnimData_find("FIGHTpunch");
    w->cHitF = AnimData_find("HIT_front");
    w->cHitB = AnimData_find("HIT_back");
    w->cKoF = AnimData_find("KO_shot_front");
    w->cKoB = AnimData_find("KO_skid_back");
    w->cSprint = AnimData_find("sprint_civi");
    w->cGetup = AnimData_find("getup");
    w->cOpen = AnimData_find("CAR_open_LHS");
    w->cGetin = AnimData_find("CAR_getin_LHS");
    w->cGetout = AnimData_find("CAR_getout_LHS");
    w->cPull = AnimData_find("CAR_pullout_LHS");
    w->cJacked = AnimData_find("CAR_jackedLHS");
    w->cSit = AnimData_find("CAR_sit");
    w->cJumpL = AnimData_find("JUMP_launch");
    w->cJumpG = AnimData_find("JUMP_glide");
    w->cJumpLand = AnimData_find("JUMP_land");
}
