/*
 * Host runner for the GTA VC J2ME -> C port.
 *
 * This is a 1:1 replay of the upstream Java harness
 * (reference/tools/test/Shots.java): same frame counts, same key presses,
 * same screenshots.  Set GTAVC_SEED to the value patched into the Java
 * reference to compare the two runs pixel by pixel.
 */
#include "game.h"
#include "world.h"
#include "world_internal.h"
#include "fx.h"
#include "ped.h"
#include "car.h"
#include "car_model.h"
#include "sky.h"
#include "save.h"
#include "snd.h"
#include "fx_math.h"
#include "j2me.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

static Game* g;
static World* w;
static const char* shotDir = "screenshots";
static int frameNo = 0;

static void ensureDir(const char* p) {
    mkdir(p, 0777);
}

static void writePPM(const char* path, const uint32_t* argb, int W, int H) {
    FILE* f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) {
        uint32_t c = argb[i];
        fputc((c >> 16) & 255, f);
        fputc((c >> 8) & 255, f);
        fputc(c & 255, f);
    }
    fclose(f);
}

static int shotIndex = 0;
static void shot(const char* name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%02d_%s.ppm", shotDir, shotIndex++, name);
    writePPM(path, (const uint32_t*)g->scr, g->W, g->H);
    printf("shot %s\n", path);
}

static void diagState(const char* tag) {
    if (!getenv("GTAVC_DIAG") || !w) return;
    int64_t ph = 0, ch = 0;
    for (int i = 0; i < w->nped; i++) ph += (int64_t)w->peds[i].x * 31 + w->peds[i].z * 17 + w->peds[i].yaw * 7 + w->peds[i].state;
    for (int i = 0; i < w->ncars; i++) ch += (int64_t)w->cars[i].x * 31 + w->cars[i].z * 17 + w->cars[i].yaw * 7 + w->cars[i].speed;
    for (int i = 0; i < w->ncars; i++) {
        Car* c = &w->cars[i];
        if (c->driver && c->driver->seat != c)
            printf("BROKEN[%s] car=%d drv=%d drvseat-index=%d\n", tag, i, (int)(c->driver - w->peds),
                   c->driver->seat ? (int)(c->driver->seat - w->cars) : -1);
    }
    printf("STATE %s frames=%d time=%d py=%d ammo=%d weapon=%d seed=%d ncars=%d nped=%d money=%d health=%d wanted=%d px=%d pz=%d yaw=%d ph=%lld ch=%lld\n",
           tag, frameNo, w->time, w->player->y, w->ammo, w->weapon, Fx_seed, w->ncars, w->nped, w->money, w->health, w->wanted,
           w->player->x, w->player->z, w->player->yaw, (long long)ph, (long long)ch);
}

static void frames(int n) {
    for (int i = 0; i < n; i++) {
        Game_frame(g, 33);
        frameNo++;
    }
}

/* Java: g.keyPressed(code); frames(n); g.keyReleased(code); */
static void key(int code, int n) {
    Game_keyPressed(g, code);
    frames(n);
    Game_keyReleased(g, code);
}

/* Java: g.press(action); ... g.down = 0; */
static void press(int action) {
    Game_press(g, action);
}

static uint16_t argb565(uint32_t c) {
    return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
}

static void passBench(void) {
    static uint32_t src[320 * 240];
    static uint16_t dst[320 * 240];
    for (int i = 0; i < 320 * 240; i++) src[i] = 0xFF102030u + (uint32_t)i;
    int n = 400;
    clock_t t0 = clock();
    for (int r = 0; r < n; r++)
        for (int i = 0; i < 320 * 240; i++) dst[i] = argb565(src[i]);
    double ms1 = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC / n;

    t0 = clock();
    for (int r = 0; r < n; r++) {
        uint16_t* d = dst;
        const uint32_t* s = src;
        for (int i = 0; i < 320 * 240; i += 2) {
            uint32_t a = s[i], b = s[i + 1];
            uint32_t p = ((a >> 8) & 0xF800) | ((a >> 5) & 0x07E0) | ((a >> 3) & 0x001F);
            uint32_t q = ((b >> 8) & 0xF800) | ((b >> 5) & 0x07E0) | ((b >> 3) & 0x001F);
            *(uint32_t*)(d + i) = p | (q << 16);
        }
    }
    double ms2 = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC / n;

    /* nearest-neighbour 160x120 -> 320x240 doubling, ARGB */
    static uint32_t small[160 * 120];
    static uint32_t big[320 * 240];
    for (int i = 0; i < 160 * 120; i++) small[i] = src[i];
    t0 = clock();
    for (int r = 0; r < n; r++) {
        for (int y = 0; y < 240; y++) {
            const uint32_t* s = small + (y >> 1) * 160;
            uint32_t* d = big + y * 320;
            for (int x = 0; x < 320; x += 2) {
                uint32_t c = s[x >> 1];
                d[x] = c; d[x + 1] = c;
            }
        }
    }
    double ms3 = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC / n;
    printf("PASSBENCH convert=%.3f ms packed=%.3f ms upscale2x=%.3f ms\n", ms1, ms2, ms3);
}

int main(int argc, char** argv) {
    if (getenv("GTAVC_PASSBENCH")) { passBench(); return 0; }
    const char* assets = argc > 1 ? argv[1] : "assets";
    if (argc > 2) shotDir = argv[2];

    platform_set_assets_dir(assets);
    platform_set_data_dir("/tmp/gtavc_port_test");
    RecordStore_deleteRecordStore("gtavc_save");
    RecordStore_deleteRecordStore("gtavc_opts");
    platform_fast_mode = 1;
    ensureDir("/tmp/gtavc_port_test");
    ensureDir(shotDir);

    printf("=== GTA VC J2ME C port - host runner ===\n");
    g = Game_new();

    if (getenv("GTAVC_AUDIOTEST")) {
        Snd_init();
        static int16_t abuf[735 * 2];
        FILE* f = fopen("/tmp/gtavc_audio.raw", "wb");
        int maxv = 0;
        int64_t sum = 0;
        int ns = 0;
        Snd_play(SND_GUN);
        for (int i = 0; i < 40; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_stopAll();
        Snd_play(SND_BAT);
        for (int i = 0; i < 20; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_stopAll();
        Snd_siren(true);
        for (int i = 0; i < 20; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_stopAll();
        Snd_engine(true, 60);
        for (int i = 0; i < 20; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_engine(false, 0);
        Snd_stopAll();
        Snd_voice(SND_V_TOMMY, false);
        for (int i = 0; i < 20; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_stopAll();
        Snd_radioStart();
        for (int i = 0; i < 20; i++) {
            Snd_mix(abuf, 735, 44100);
            if (f) fwrite(abuf, sizeof(int16_t), 735 * 2, f);
            for (int k = 0; k < 735 * 2; k++) {
                int v = abuf[k] < 0 ? -abuf[k] : abuf[k];
                if (v > maxv) maxv = v;
                sum += (int64_t)abuf[k] * abuf[k];
                ns++;
            }
        }
        Snd_radioStop();
        Snd_stopAll();
        if (f) fclose(f);
        int rms = ns ? (int)Fx_isqrt((uint64_t)(sum / (ns ? ns : 1))) : 0;
        printf("AUDIOTEST clips max=%d rms=%d samples=%d -> %s\n", maxv, rms, ns, maxv > 100 ? "PASS" : "FAIL");
        Snd_cleanup();
        return maxv > 100 ? 0 : 1;
    }

    if (getenv("GTAVC_OPTSTEST")) {
        g->oBright = 3;
        g->oDraw = 7;
        g->oLimiter = false;
        g->oTrails = true;
        g->oSubs = false;
        g->oWide = true;
        g->oLegend = true;
        g->oRadar = 2;
        g->oHud = false;
        g->oRadio = false;
        g->oDensity = 2;
        g->oRes = 3;
        Save_writeOptions(g);
        g->oBright = 10; g->oDraw = 4; g->oLimiter = true; g->oTrails = false;
        g->oSubs = true; g->oWide = false; g->oLegend = false; g->oRadar = 0;
        g->oHud = true; g->oRadio = true; g->oDensity = 0; g->oRes = 1;
        bool ok = Save_loadOptions(g);
        bool pass = ok && g->oBright == 3 && g->oDraw == 7 && !g->oLimiter &&
                    g->oTrails && !g->oSubs && g->oWide && g->oLegend &&
                    g->oRadar == 2 && !g->oHud && !g->oRadio && g->oDensity == 2 && g->oRes == 3;
        printf("OPTSTEST load=%d bright=%d draw=%d limiter=%d trails=%d subs=%d wide=%d legend=%d radar=%d hud=%d radio=%d density=%d res=%d -> %s\n",
               ok, g->oBright, g->oDraw, g->oLimiter, g->oTrails, g->oSubs, g->oWide,
               g->oLegend, g->oRadar, g->oHud, g->oRadio, g->oDensity, g->oRes, pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }

    frames(5);
    shot("splash");

    int guard = 0;
    while (g->state != GAME_S_MENU && guard++ < 9000) {
        Game_frame(g, 33);
        frameNo++;
    }
    printf("menu after %d frames\n", guard);
    frames(10);
    shot("menu");

    press(GAME_A_FIRE);
    Game_frame(g, 33);
    g->down = 0;
    w = g->world;
    frames(40);
    shot("spawn");
    diagState("spawn");

    press(GAME_A_PAUSE);
    Game_frame(g, 33);
    g->down = 0;
    g->sel = 2;
    press(GAME_A_FIRE);
    Game_frame(g, 33);
    g->down = 0;
    frames(2);
    shot("map");
    diagState("map");
    press(GAME_A_FIRE);
    Game_frame(g, 33);
    g->down = 0;
    g->sel = 0;
    press(GAME_A_FIRE);
    Game_frame(g, 33);
    g->down = 0;

    if (!w) { printf("FAIL: no world\n"); return 1; }
    printf("spawn ground %d y %d surf %d peds %d cars %d\n",
           World_groundH(w, w->player->x, w->player->z, w->player->y),
           w->player->y, World_surfAt(w, w->player->x, w->player->z),
           w->nped, w->ncars);

    if (getenv("GTAVC_SPAWNCHECK") && w) {
        int badP = 0, badC = 0;
        for (int i = 0; i < w->nped; i++) {
            Ped* p = &w->peds[i];
            if (p->seat != NULL || p->state == PED_DEAD) continue;
            if (!World_spawnClear(w, p->x, p->z, 500)) {
                badP++;
                printf("FAIL PED %d at (%d, %d, %d)\n", i, p->x, p->z, p->y);
            }
        }
        for (int i = 0; i < w->ncars; i++) {
            if (!World_spawnClear(w, w->cars[i].x, w->cars[i].z, 1300)) badC++;
        }
        printf("SPAWNCHECK peds=%d badPeds=%d cars=%d badCars=%d\n",
               w->nped, badP, w->ncars, badC);
        shot("spawncheck");
        return (badP || badC) ? 1 : 0;
    }

    if (getenv("GTAVC_MISSIONTEST") && w) {
        frames(220);
        printf("MISSION start state=%d timer=%d text=%s marker=%d,%d,%d player=%d,%d\n",
               w->missionState, w->missionTimer, w->missionText,
               w->missionX, w->missionY, w->missionZ, w->player->x, w->player->z);
        shot("mission_start");
        if (w->missionState == WORLD_MISSION_ACTIVE) {
            int mx = w->missionX, mz = w->missionZ;
            w->player->x = mx;
            w->player->z = mz;
            w->player->y = World_groundH(w, mx, mz, w->missionY);
            World_preload(w, mx, mz);
            frames(5);
            printf("MISSION pass state=%d done=%d money=%d text=%s\n",
                   w->missionState, w->missionDone, w->money, w->missionText);
            shot("mission_pass");
            w->missionDone = false;
            w->missionState = WORLD_MISSION_AVAILABLE;
            bool loaded = Save_load(w);
            printf("MISSION save load=%d done=%d state=%d money=%d\n",
                   loaded, w->missionDone, w->missionState, w->money);
            if (!loaded || !w->missionDone || w->missionState != WORLD_MISSION_PASSED) return 1;
        }
        return (w->missionState == WORLD_MISSION_PASSED && w->missionDone) ? 0 : 1;
    }

    if (getenv("GTAVC_RADIOTEST") && w) {
        /* Spawn a car at the player's feet, use the real enter/exit path and
           verify that the radio follows radioEnabled and the vehicle state. */
        g->oRadio = true;
        Game_refreshOptions(g);
        World_placePlayerAtSpawn(w);
        w->inCar = NULL; w->player->seat = NULL; w->pAct = 0;
        w->radioInCar = false;
        World_preload(w, w->player->x, w->player->z);
        World_findSpot(w, 4, 18, WORLD_S_ROAD);
        Car* rc = World_addCar(w, CAR_PARKED, 1, w->spot[0], w->spot[1], 0, 0);
        if (!rc) { printf("RADIOTEST spawn -> FAIL\n"); return 1; }
        w->player->x = rc->x; w->player->z = rc->z;
        w->player->y = World_groundH(w, rc->x, rc->z, rc->y);
        Game_press(g, GAME_A_ENTER);
        Game_frame(g, 33); g->down = 0;
        bool quietWhileWalking = !Snd_radioActive();
        frames(120);
        bool inCar = w->inCar != NULL;
        bool on = inCar && w->radioInCar && Snd_radioActive();
        printf("RADIOTEST enter car=%d flag=%d active=%d quietWalk=%d\n",
               inCar, w->radioInCar, Snd_radioActive(), quietWhileWalking);
        shot("radio_car");

        g->oRadio = false;
        Game_refreshOptions(g);
        frames(3);
        bool off = !w->radioEnabled && !w->radioInCar && !Snd_radioActive();
        printf("RADIOTEST option off enabled=%d flag=%d active=%d\n", w->radioEnabled, w->radioInCar, Snd_radioActive());

        g->oRadio = true;
        Game_refreshOptions(g);
        frames(8);
        bool resumed = w->radioEnabled && w->radioInCar && Snd_radioActive();
        printf("RADIOTEST option on enabled=%d flag=%d active=%d\n", w->radioEnabled, w->radioInCar, Snd_radioActive());

        Game_press(g, GAME_A_ENTER);
        Game_frame(g, 33); g->down = 0;
        frames(120);
        bool exited = w->inCar == NULL && !w->radioInCar && !Snd_radioActive();
        printf("RADIOTEST exit car=%d flag=%d active=%d\n", w->inCar != NULL, w->radioInCar, Snd_radioActive());

        g->state = GAME_S_OPT;
        g->prevState = GAME_S_PAUSE;
        g->optSel = 11;
        Game_frame(g, 33);
        shot("options_radio");
        g->state = GAME_S_PLAY;
        bool pass = quietWhileWalking && on && off && resumed && exited;
        printf("RADIOTEST enter/off/on/exit -> %s\n", pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }

    if (getenv("GTAVC_BRIDGETEST") && w) {
        /* 1. High bridge ascent: spawn car at foot of high bridge ramp
              and drive onto the high bridge deck over water. */
        int bx = -52224, bz = 99328;
        World_preload(w, bx, bz);
        int by = World_groundH(w, bx, bz, 4000);
        w->player->x = bx; w->player->z = bz; w->player->y = by;
        Car* bc = World_addCar(w, CAR_PARKED, 1, bx, bz, 716, 0); 
        if (!bc) { printf("BRIDGETEST addCar failed\n"); return 1; }
        bc->y = by;
        w->inCar = bc;
        bc->mode = CAR_PLAYER;
        w->player->seat = bc;
        w->pAct = 0;
        frames(5);
        if (!w->inCar) { printf("BRIDGETEST car entry failed\n"); return 1; }
        Car* car = w->inCar;
        printf("BRIDGETEST start: x=%d z=%d y=%d (expect ~660)\n", car->x, car->z, car->y);

        /* Drive up the ramp towards the bridge deck */
        for (int i = 0; i < 200; i++) {
            g->down = GAME_A_UP;
            Game_frame(g, 33);
            car = w->inCar;
            if (!car) break;
        }
        g->down = 0;
        car = w->inCar;
        if (!car) { printf("BRIDGETEST lost car on bridge!\n"); return 1; }
        printf("BRIDGETEST on bridge: x=%d z=%d y=%d speed=%d drown=%d health=%d\n",
               car->x, car->z, car->y, car->speed, w->drownT, w->health);
        shot("bridge_high");
        bool bridgePass = (car->x < bx - 10000) && (car->y > by + 20) && (w->drownT == 0) && (w->health > 80);

        /* 2. Road over parking garage (Tile 20_14 col 33):
              Drive along the elevated road over the parking lot. */
        int px = 164864, pz = -683008;
        World_preload(w, px, pz);
        int py = World_groundH(w, px, pz, 4000);
        car->x = px; car->z = pz; car->y = py; car->yaw = 0; car->speed = 0; car->vx = car->vz = 0;
        w->player->x = px; w->player->z = pz; w->player->y = py;
        printf("BRIDGETEST parking road start: x=%d z=%d y=%d (expect ~440)\n", car->x, car->z, car->y);

        for (int i = 0; i < 150; i++) {
            g->down = GAME_A_UP;
            Game_frame(g, 33);
            car = w->inCar;
            if (!car) break;
        }
        g->down = 0;
        car = w->inCar;
        if (!car) { printf("BRIDGETEST lost car on parking road!\n"); return 1; }
        printf("BRIDGETEST parking road end: x=%d z=%d y=%d speed=%d drown=%d health=%d\n",
               car->x, car->z, car->y, car->speed, w->drownT, w->health);
        shot("parking_road");
        bool parkingPass = (car->y >= 350) && (w->drownT == 0) && (w->health > 80);

        bool allPass = bridgePass && parkingPass;
        printf("BRIDGETEST bridge=%d parking=%d -> %s\n", bridgePass, parkingPass, allPass ? "PASS" : "FAIL");
        return allPass ? 0 : 1;
    }

    if (getenv("GTAVC_RAMPTEST") && w) {
        /* 1. Beach stunt ramp ascent:
              Start in clear approach (z=-891000, y=329) and accelerate up the ramp. */
        w->ncars = 0;
        int rx = 177152, rz = -891000;
        World_preload(w, rx, rz);
        int ry = World_groundH(w, rx, rz, 4000);
        w->player->x = rx; w->player->z = rz; w->player->y = ry;
        Car* rc = World_addCar(w, CAR_PLAYER, 1, rx, rz, 0, 0);
        if (!rc) { printf("RAMPTEST addCar failed\n"); return 1; }
        rc->y = ry;
        w->inCar = rc;
        w->player->seat = rc;
        w->pAct = 0;
        frames(5);
        printf("RAMPTEST start: x=%d z=%d y=%d (expect ~329)\n", w->inCar->x, w->inCar->z, w->inCar->y);

        int maxY = w->inCar->y;
        for (int i = 0; i < 140; i++) {
            g->down = GAME_A_UP;
            Game_frame(g, 33);
            if (w->inCar && w->inCar->y > maxY) maxY = w->inCar->y;
        }
        g->down = 0;
        Car* car = w->inCar;
        if (!car) { printf("RAMPTEST lost car!\n"); return 1; }
        printf("RAMPTEST climbed: x=%d z=%d y=%d maxY=%d speed=%d health=%d\n",
               car->x, car->z, car->y, maxY, car->speed, w->health);
        shot("ramp_beach");
        bool rampPass = (maxY >= 450) && (car->z > rz + 3000) && (w->health > 80);

        /* 2. Building wall test: drive East from Ken's alley into the building wall
              at x=141312 and assert the car is stopped and does not penetrate. */
        int wx = 137000, wz = -837200;
        World_preload(w, wx, wz);
        int wy = World_groundH(w, wx, wz, 4000);
        car->x = wx; car->z = wz; car->y = wy; car->yaw = 256; car->speed = 0; car->vx = car->vz = 0;
        w->player->x = wx; w->player->z = wz; w->player->y = wy;
        printf("RAMPTEST wall approach: x=%d z=%d y=%d\n", car->x, car->z, car->y);

        for (int i = 0; i < 60; i++) {
            g->down = GAME_A_UP;
            Game_frame(g, 33);
            car = w->inCar;
            if (!car) break;
        }
        g->down = 0;
        if (!car) { printf("RAMPTEST lost car at wall!\n"); return 1; }
        printf("RAMPTEST wall result: x=%d z=%d y=%d (blocked: %d, no roof: %d)\n",
               car->x, car->z, car->y, car->x <= 140500, car->y < 350);
        shot("wall_blocked");
        bool wallPass = (car->x <= 140500) && (car->y < 350);

        bool allPass = rampPass && wallPass;
        printf("RAMPTEST ramp=%d wall=%d -> %s\n", rampPass, wallPass, allPass ? "PASS" : "FAIL");
        return allPass ? 0 : 1;
    }

    if (getenv("GTAVC_TILECACHE") && w) {
        const char* dir = getenv("GTAVC_TILECACHE_DIR");
        if (!dir || !*dir) dir = "assets/tc";
        mkdir(dir, 0777);
        int made = 0, failed = 0, skipped = 0;
        long long t0 = System_currentTimeMillis();
        for (int j = 0; j < w->tnz; j++) {
            for (int i = 0; i < w->tnx; i++) {
                int k = j * w->tnx + i;
                if (!w->tflag || !w->tflag[k]) continue;
                char path[512];
                snprintf(path, sizeof(path), "%s/%d_%d.bin", dir, i, j);
                if (access(path, F_OK) == 0) { skipped++; continue; }
                Tile* tc = Tile_load(i, j, w->mox, w->moz, w->tileMm);
                if (tc && Tile_saveCache(tc, path)) made++;
                else { failed++; if (tc) remove(path); }
                if (tc) Tile_free(tc);
            }
        }
        long long t1 = System_currentTimeMillis();
        printf("TILECACHE made=%d skipped=%d failed=%d ms=%lld dir=%s\n",
               made, skipped, failed, t1 - t0, dir);
        return failed ? 1 : 0;
    }

    if (getenv("GTAVC_TILEBENCH")) {
        int i = 22, j = 37;
        long long t0 = System_currentTimeMillis();
        Tile* tb = Tile_load(i, j, w->mox, w->moz, w->tileMm);
        long long t1 = System_currentTimeMillis();
        printf("TILEBENCH %d_%d ms=%lld near=%d far=%d ok=%d\n",
               i, j, t1 - t0, tb ? tb->nearChunkCount : -1,
               tb ? tb->farChunkCount : -1, tb != NULL);
        if (tb) Tile_free(tb);
        return 0;
    }

    if (getenv("GTAVC_CONTROLS")) {
        /* on-foot: continuous autorun + A sprint */
        Game_keyPressed(g, KEY_UP);
        frames(60);
        printf("CTRL run speed=%d (expect ~6200)\n", w->player->speed);
        Game_keyPressed(g, KEY_NUM1);
        frames(30);
        printf("CTRL sprint speed=%d (expect ~8800)\n", w->player->speed);
        Game_keyReleased(g, KEY_NUM1);
        Game_keyReleased(g, KEY_UP);
        frames(10);
        /* B = attack */
        Game_keyPressed(g, KEY_NUM5);
        Game_frame(g, 33); g->down = 0; frameNo++;
        printf("CTRL attack clip=%d\n", w->player->clip);
        Game_keyReleased(g, KEY_NUM5);
        frames(40);
        /* X = jump */
        Game_keyPressed(g, KEY_NUM0);
        Game_frame(g, 33); g->down = 0; frameNo++;
        printf("CTRL jump air=%d\n", w->player->air);
        Game_keyReleased(g, KEY_NUM0);
        frames(40);
        /* Y = enter the parked car (stand next to the Admiral) */
        if (w->ncars > 0) {
            Car* parked = &w->cars[0];
            w->player->x = parked->x - 1200;
            w->player->z = parked->z;
            w->player->y = World_groundH(w, w->player->x, w->player->z, parked->y);
        }
        Game_keyPressed(g, KEY_STAR);
        frames(1);
        Game_keyReleased(g, KEY_STAR);
        frames(200);
        printf("CTRL inCar=%d\n", w->inCar != NULL);
        Car* cc = w->inCar;
        if (cc) {
            Game_keyPressed(g, KEY_NUM1);
            frames(60);
            printf("CTRL A throttle speed=%d\n", cc->speed);
            Game_keyReleased(g, KEY_NUM1);
            Game_keyPressed(g, KEY_NUM5);
            frames(60);
            printf("CTRL B brake speed=%d\n", cc->speed);
            Game_keyReleased(g, KEY_NUM5);
            Game_keyPressed(g, KEY_NUM1);
            frames(40);
            Game_keyPressed(g, KEY_NUM4);   /* L handbrake */
            frames(30);
            printf("CTRL L handbrake slide=%d braking=%d\n", cc->slide, cc->braking);
            Game_keyReleased(g, KEY_NUM4);
            Game_keyPressed(g, KEY_NUM6);   /* R look behind */
            Game_frame(g, 33); frameNo++;
            printf("CTRL R lookbehind camYaw=%d carYaw=%d want=%d\n", w->camYaw, cc->yaw, (cc->yaw + 512) & 1023);
            Game_keyReleased(g, KEY_NUM6);
            Game_keyReleased(g, KEY_NUM1);
        }
        Game_stop(g);
        Game_free(g);
        return 0;
    }

    if (getenv("GTAVC_BENCH")) {
        int bn = atoi(getenv("GTAVC_BENCH"));
        if (getenv("GTAVC_NOHUD")) g->oHud = false;
        if (getenv("GTAVC_NORADAR")) g->oRadar = 2;
        Game_press(g, GAME_A_UP);
        frames(30);
        clock_t tb = clock();
        frames(bn);
        double bms = (double)(clock() - tb) * 1000.0 / (double)CLOCKS_PER_SEC / (double)bn;
        g->down = 0;
        printf("BENCH frames=%d ms/frame=%.2f tris=%d verts=%d seed=%d\n", bn, bms, g->r.tris, g->r.verts, Fx_seed);
        Game_stop(g);
        Game_free(g);
        return 0;
    }

    for (int k = 1; k < 4; k++) {
        w->camYawOff = k * 256;
        frames(25);
        char n[32];
        snprintf(n, sizeof(n), "look%d", k);
        shot(n);
    }
    w->camYawOff = 0;
    diagState("looks");

    for (int i = 0; i < w->npick; i++) {
        w->player->x = w->pkX[i];
        w->player->z = w->pkZ[i];
        frames(3);
    }
    diagState("pickups");
    if (getenv("GTAVC_DIAG")) {
        printf("PK npick=%d", w->npick);
        for (int i = 0; i < w->npick; i++) printf(" [%d]=%d,%d", i, w->pkX[i], w->pkZ[i]);
        printf("\n");
        for (int i = 0; i < w->nped; i++)
            printf("PED %d x=%d z=%d y=%d yaw=%d st=%d clip=%d timer=%d sp=%d hp=%d tt=%d loco=%d locoT=%d\n", i,
                   w->peds[i].x, w->peds[i].z, w->peds[i].y, w->peds[i].yaw, w->peds[i].state,
                   w->peds[i].clip, w->peds[i].timer, w->peds[i].speed, w->peds[i].hp,
                   w->peds[i].turnTimer, w->peds[i].loco, w->peds[i].locoT);
        for (int i = 0; i < w->ncars; i++)
        {
            int di = -1;
            if (w->cars[i].driver) di = (int)(w->cars[i].driver - w->peds);
            printf("CAR %d x=%d z=%d yaw=%d sp=%d mode=%d type=%d drv=%d\n", i,
                   w->cars[i].x, w->cars[i].z, w->cars[i].yaw, w->cars[i].speed, w->cars[i].mode, w->cars[i].type, di);
        }
    }
    printf("weapons: bat %d pistol %d ammo %d current %d\n",
           w->hasBat, w->hasPistol, w->ammo, w->weapon);

    w->weapon = 2;
    Game_keyPressed(g, KEY_NUM5);
    if (getenv("GTAVC_DIAG")) {
        for (int i = 0; i < 8; i++) { Game_frame(g, 33); frameNo++; {
            int ti = -1;
            if (w->target) ti = (int)(w->target - w->peds);
            printf("FRAME aim%d seed=%d target=%d aimT=%d shotCd=%d yaw=%d ammo=%d\n", i, Fx_seed, ti, w->aimT, w->shotCd, w->player->yaw, w->ammo);
        } }
        shot("aim");
        for (int i = 8; i < 28; i++) { Game_frame(g, 33); frameNo++; {
            int ti = -1;
            if (w->target) ti = (int)(w->target - w->peds);
            printf("FRAME aim%d seed=%d target=%d aimT=%d shotCd=%d yaw=%d ammo=%d\n", i, Fx_seed, ti, w->aimT, w->shotCd, w->player->yaw, w->ammo);
        } }
    } else {
        frames(8);
        shot("aim");
        frames(20);
    }
    Game_keyReleased(g, KEY_NUM5);
    printf("after shooting: ammo %d wanted %d\n", w->ammo, w->wanted);

    diagState("aim");
    w->weapon = 1;
    frames(5);
    shot("bat");
    if (getenv("GTAVC_BATTEST")) {
        w->camYawOff = 512;
        w->camPitch = 30;
        w->camIdle = 0;
        printf("BATCLIPS punch=%d jab=%d heavy=%d idle=%d\n", w->cPunch, w->cJab, w->cHeavy, w->cIdle);
        Game_keyPressed(g, KEY_NUM5);
        for (int i = 0; i < 12; i++) {
            Game_frame(g, 33);
            printf("BATFRAME %d clip=%d t=%d spine=%d,%d,%d,%d,%d,%d,%d,%d,%d pelvis=%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                   i, w->player->clip, w->player->clipT,
                   w->pose[0],w->pose[1],w->pose[2],w->pose[3],w->pose[4],w->pose[5],w->pose[6],w->pose[7],w->pose[8],
                   w->pose[81],w->pose[82],w->pose[83],w->pose[84],w->pose[85],w->pose[86],w->pose[87],w->pose[88],w->pose[89]);
            char n[32];
            snprintf(n, sizeof(n), "batswing_%02d", i);
            shot(n);
        }
        Game_keyReleased(g, KEY_NUM5);
        printf("BATTEST clip=%d clipT=%d\n", w->player->clip, w->player->clipT);
        return 0;
    }

    Ped* v = NULL;
    for (int i = 0; i < w->nped; i++) {
        if (!w->peds[i].cop && w->peds[i].seat == NULL && w->peds[i].state != PED_DEAD) {
            v = &w->peds[i];
            break;
        }
    }
    if (v) {
        World_damagePed(w, v, 500, true, w->player->x, w->player->z);
        w->player->x = v->x + 2600;
        w->player->z = v->z;
        w->player->y = World_groundH(w, w->player->x, w->player->z, v->y);
        w->player->yaw = Fx_yawOf(v->x - w->player->x, v->z - w->player->z);
        w->camYaw = w->player->yaw;
        frames(70);
        shot("dead");
        diagState("dead");
    }

    int times[3] = { 9 * 60, 18 * 60 + 40, 23 * 60 };
    for (int i = 0; i < 3; i++) {
        w->minutes = times[i];
        frames(3);
        char n[32];
        snprintf(n, sizeof(n), "time_%d", times[i] / 60);
        shot(n);
    }
    w->minutes = 13 * 60;
    diagState("times");

    Car* c = NULL;
    int bd = 1 << 30;
    for (int i = 0; i < w->ncars; i++) {
        if (World_cm(w, &w->cars[i])->bike) continue;
        int d = Fx_dist(w->cars[i].x - w->player->x, w->cars[i].z - w->player->z);
        if (d < bd) { bd = d; c = &w->cars[i]; }
    }
    if (c) {
        c->mode = CAR_PARKED;
        c->vx = c->vz = 0;
        c->speed = 0;
        w->player->x = c->x - 2500;
        w->player->z = c->z;
        w->player->y = c->y;
        diagState("preenter");
        key(KEY_STAR, 1);
        frames(150);
        diagState("entered");
        printf("in car: %d model %s\n", w->inCar != NULL, World_cm(w, c)->name);
        if (World_findSpot(w, 5, 40, WORLD_S_ROAD)) {
            c->x = w->spot[0];
            c->z = w->spot[1];
            c->yaw = World_roadYaw(w, c->x, c->z);
            c->y = World_groundH(w, c->x, c->z, c->y);
            w->player->x = c->x;
            w->player->z = c->z;
        }
        Game_keyPressed(g, KEY_UP);
        for (int t = 0; t < 8; t++) {
            frames(30);
            printf("t=%ds speed %d km/h hp %d\n", t + 1, c->speed * 36 / 10240, c->hp);
        }
        shot("drive");
        if (getenv("GTAVC_DIAG")) {
            int fx2 = Fx_sin(c->yaw), fz2 = Fx_cos(c->yaw);
            CarModel* mm = World_cm(w, c);
            int half2 = mm->dimZ * 16 / 2 - 900;
            int px2 = c->x + ((fx2 * half2) >> 14);
            int pz2 = c->z + ((fz2 * half2) >> 14);
            printf("FRONT m %d %d y %d\n", px2 / 1024, pz2 / 1024, c->y / 64);
            for (int dz = -2; dz <= 2; dz++) {
                printf("  ");
                for (int dx = -2; dx <= 2; dx++)
                    printf("%s", World_solidCell(w, World_scell(px2 + dx * 1024, w->mox), World_scell(pz2 + dz * 1024, w->moz)) ? "#" : ".");
                printf("\n");
            }
            for (int q = 0; q < 6; q++) {
                int ox = c->x, oz = c->z;
                w->dbgHit = 0;
                Game_frame(g, 33); frameNo++;
                printf("hit %d inCar %d pAct %d speed %d v %d,%d moved %d down %d ncars %d\n",
                       w->dbgHit, w->inCar == c, w->pAct, c->speed, c->vx, c->vz,
                       Fx_dist(c->x - ox, c->z - oz), g->down, w->ncars);
            }
            {
                int half3 = mm->dimZ * 16 / 2 - 900;
                int rad3 = mm->dimX * 16 / 2 + 60;
                int gH = c->y > World_groundH(w, c->x, c->z, c->y) ? c->y : World_groundH(w, c->x, c->z, c->y);
                printf("dims %d,%d,%d half %d rad %d gHere %d\n", mm->dimX, mm->dimY, mm->dimZ, half3, rad3, gH);
                for (int k2 = -1; k2 <= 1; k2++) {
                    int ox2 = (Fx_sin(c->yaw) * k2 * half3) >> 14;
                    int oz2 = (Fx_cos(c->yaw) * k2 * half3) >> 14;
                    w->rx = c->x + ox2; w->rz = c->z + oz2;
                    bool rs = World_resolve(w, rad3);
                    printf("  k %d resolve %d g %d g2 %d\n", k2, rs,
                           World_groundH(w, c->x + ox2, c->z + oz2, gH),
                           World_groundH(w, c->x + ox2 * 12 / 10, c->z + oz2 * 12 / 10, gH));
                }
            }
            for (int i = 0; i < w->ncars; i++) {
                if (&w->cars[i] != c) printf("  other car dist %d\n", Fx_dist(w->cars[i].x - c->x, w->cars[i].z - c->z));
            }
            {
                int fxa = Fx_sin(c->yaw), fza = Fx_cos(c->yaw);
                printf("car y %d ground %d surf %d\n", c->y, World_groundH(w, c->x, c->z, c->y), World_surfAt(w, c->x, c->z));
                for (int d = 500; d <= 5000; d += 500) {
                    int x = c->x + ((fxa * d) >> 14), z = c->z + ((fza * d) >> 14);
                    printf("  ahead %dmm ground %d solid %d surf %d\n", d,
                           World_groundH(w, x, z, c->y), World_solidCell(w, World_scell(x, w->mox), World_scell(z, w->moz)),
                           World_surfAt(w, x, z));
                }
                for (int sd = -2000; sd <= 2000; sd += 500) {
                    int x = c->x + ((fza * sd) >> 14), z = c->z - ((fxa * sd) >> 14);
                    printf("  side %d solid %d g %d\n", sd,
                           World_solidCell(w, World_scell(x, w->mox), World_scell(z, w->moz)),
                           World_groundH(w, x, z, c->y));
                }
            }
            printf("speed %d m/s, vx %d vz %d hp %d\n", c->speed / 1024, c->vx, c->vz, c->hp);
        }
        Game_keyPressed(g, KEY_LEFT);
        frames(40);
        shot("turn");
        Game_keyReleased(g, KEY_LEFT);
        Game_keyPressed(g, KEY_NUM0);
        Game_keyPressed(g, KEY_RIGHT);
        frames(30);
        shot("handbrake");
        Game_keyReleased(g, KEY_RIGHT);
        Game_keyReleased(g, KEY_NUM0);
        Game_keyReleased(g, KEY_UP);
        printf("slide %d yawVel %d\n", c->slide, c->yawVel);
        Game_keyPressed(g, KEY_STAR);
        frames(2);
        Game_keyReleased(g, KEY_STAR);
        frames(60);
        w->camYawOff = 300;
        World_damageCar(w, c, 0, 2000);
        frames(50);
        shot("fire");
        printf("burning %d\n", c->burning);
        frames(120);
        shot("explode");
        printf("wrecked %d health %d\n", c->wrecked, w->health);
    }

    if (getenv("GTAVC_DENSITYTEST") && w) {
        /* Exercise the real options-menu path: select "traffic density"
           and press right.  This also checks that the change is applied and
           persisted immediately. */
        g->state = GAME_S_OPT;
        g->prevState = GAME_S_PAUSE;
        g->optSel = 9;
        int orders[2][4] = { { 0, 1, 2, 3 }, { 3, 0, 1, 2 } };
        for (int pass = 0; pass < 2; pass++) {
            for (int q = 0; q < 4; q++) {
                int d = orders[pass][q];
                for (int guard = 0; guard < 5 && g->oDensity != d; guard++) {
                    Game_keyPressed(g, KEY_RIGHT);
                    Game_frame(g, 33);
                    Game_keyReleased(g, KEY_RIGHT);
                }
                g->edges = 0;
                int alive = 0, visible = 0, traffic = 0;
                for (int i = 0; i < w->nped; i++) {
                    Ped* p = &w->peds[i];
                    if (p->seat != NULL || p->cop || p->state == PED_DEAD) continue;
                    alive++;
                    if (w->pool[p->kind] || w->poolLo[p->kind]) visible++;
                }
                for (int i = 0; i < w->ncars; i++) if (w->cars[i].mode == CAR_TRAFFIC) traffic++;
                printf("DENSITY d=%d alive=%d visible=%d traffic=%d caps=%d/%d\n",
                       d, alive, visible, traffic, w->maxPed, w->maxTraffic);
            }
        }
        return 0;
    }

    long long t0 = System_currentTimeMillis();
    frames(60);
    double ms = (double)(System_currentTimeMillis() - t0) / 60.0;
    int loaded = 0;
    for (int k = 0; k < w->tnx * w->tnz; k++) if (w->tiles[k] != NULL) loaded++;
    printf("ms/frame %.2f tris %d tiles %d\n", ms, g->r.tris, loaded);

    if (getenv("GTAVC_STRESS") && w) {
        int n = atoi(getenv("GTAVC_STRESS"));
        printf("stress: %d frames\n", n);
        w->health = 100; w->busted = false; w->wastedT = 0;
        World_placePlayerAtSpawn(w);
        w->inCar = NULL; w->player->seat = NULL; w->pAct = 0;
        World_preload(w, w->player->x, w->player->z);
        World_findSpot(w, 5, 20, WORLD_S_ROAD);
        Car* sc = World_addCar(w, CAR_PARKED, 1, w->spot[0], w->spot[1], 0, 0);
        if (sc) {
            w->player->x = sc->x; w->player->z = sc->z;
            w->player->y = sc->y;
            Game_press(g, GAME_A_ENTER);
            Game_frame(g, 33); g->down = 0;
            frames(120);
        }
        unsigned st = 12345;
        for (int i = 0; i < n; i++) {
            st = st * 1103515245u + 12345u;
            int k = (st >> 16) & 1023;
            g->down = GAME_A_UP;
            if (k < 120) g->down |= GAME_A_LEFT;
            else if (k < 240) g->down |= GAME_A_RIGHT;
            if ((k & 255) == 7) g->down |= GAME_A_HAND;
            if ((i % 300) == 0 && w->inCar) {
                if (World_findSpot(w, 60, 110, WORLD_S_ROAD)) {
                    w->inCar->x = w->spot[0];
                    w->inCar->z = w->spot[1];
                    w->inCar->y = World_groundH(w, w->spot[0], w->spot[1], w->inCar->y);
                    w->player->x = w->inCar->x;
                    w->player->z = w->inCar->z;
                    w->player->y = w->inCar->y;
                    w->health = 100;
                }
            }
            Game_frame(g, 33);
            frameNo++;
            if ((i % 500) == 0) printf("stress %d inCar=%d pos=%d,%d tiles=%d fps=%d\n",
                                       i, w->inCar != NULL, w->player->x, w->player->z,
                                       g->r.tris, g->fps);
        }
        g->down = 0;
        shot("stress_end");
        printf("stress done inCar=%d health=%d wanted=%d\n", w->inCar != NULL, w->health, w->wanted);
    }

    Game_stop(g);
    Game_free(g);
    printf("=== done ===\n");
    return 0;
}
