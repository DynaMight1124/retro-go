#include "game.h"
#include "world_internal.h"
#include "assets.h"
#include "anim_data.h"
#include "fx.h"
#include "sky.h"
#include "snd.h"
#include "save.h"
#include "intro.h"
#include "j2me.h"
#include "gtavc_profile.h"
#include "video_compose.h"
#include "port_defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game* GTA_game = NULL;

static const char* MAIN_ITEMS[] = { "start game", "options", "controls", "quit game" };
static const char* MAIN_C_ITEMS[] = { "continue", "start new game", "options", "controls", "quit game" };
static const char* PAUSE_ITEMS[] = { "resume", "start new game", "map", "stats", "brief", "options", "quit game" };
static const char* DBG_ITEMS[] = { "spawn car", "time of day", "tommy speed", "game speed", "god mode", "clear wanted level", "give weapons + $5000", "teleport to spawn", "back" };
static const char* OPT_NAMES[] = { "brightness", "draw distance", "frame limiter", "ps2 filter", "subtitles", "wide screen",
                                   "map legend", "radar mode", "hud mode", "traffic density", "screen resolution",
                                   "radio", "restore defaults", "back" };

static const char* CREDIT = "A Port made by AidenCantCode";
static void applyOptions(Game* g, bool fill);
static void fillPopulation(World* w);

Game* Game_new(void) {
    Game* g = (Game*)calloc(1, sizeof(Game));
    GTA_game = g;
    if (!g) return NULL;
    R3D_init(&g->r);
    g->state = GAME_S_SPLASH;
    g->fadeMs = 9999;
    g->playFade = 9999;
    g->gameSpeed = 100;
    g->oBright = 10;
    g->oDraw = 2;
    g->oRadar = 0;
    g->oRes = GTAVC_DEFAULT_RESOLUTION;
    g->oDensity = 1;
    g->oLimiter = true;
    g->oTrails = false;
    g->oSubs = true;
    g->oWide = false;
    g->oLegend = false;
    g->oHud = true;
    g->oRadio = true;
    g->rs = 1;
    g->needResize = true;
    g->running = true;
    g->tbX = (int*)calloc(12, sizeof(int));
    g->tbY = (int*)calloc(12, sizeof(int));
    if (!g->tbX || !g->tbY) {
        free(g->tbX); free(g->tbY); free(g);
        GTA_game = NULL;
        return NULL;
    }

    Fx_init();
    Assets_init(&g->r);
    Assets_r = &g->r;
    g->legacy = Assets_legacy;
    if (g->legacy) {
        g->oRes = GTAVC_DEFAULT_RESOLUTION;
        g->oDraw = 2;
    }
    /* Restore the display options chosen on the previous run.  Environment
       overrides below stay useful for the host/MIPS test harness. */
    Save_loadOptions(g);
    const char* ores = getenv("GTAVC_ORES");
    if (ores) g->oRes = atoi(ores);
    const char* den = getenv("GTAVC_DENSITY");
    if (den) g->oDensity = atoi(den);
    const char* od = getenv("GTAVC_ODRAW");
    if (od) g->oDraw = atoi(od);
    g->total = UI_STEPS + 7;
    g->W = 320;
    g->H = 240;
    return g;
}

void Game_free(Game* g) {
    if (!g) return;
    Video_clearOverlay();
    if (g->world) {
        World_free(g->world);
        g->world = NULL;
    }
    AnimData_free();
    Sky_free();
    Ui_cleanup();
    Snd_cleanup();
    Assets_cleanup();
    if (g->scr && g->scr != g->r.fb) free(g->scr);
    R3D_free(&g->r);
    free(g->trail);
    free(g->blm);
    free(g->blm2);
    free(g->tbX);
    free(g->tbY);
    if (GTA_game == g) GTA_game = NULL;
    free(g);
}

void Game_start(Game* g) {
    g->running = true;
}

void Game_stop(Game* g) {
    if (!g) return;
    Save_writeOptions(g);
    if (g->started && g->world) Save_write(g->world);
    g->running = false;
    if (g->world) g->world->alive = false;
    Snd_stopAll();
}

int Game_getWidth(Game* g) { return g->W; }
int Game_getHeight(Game* g) { return g->H; }

void Game_resize(Game* g) {
    Video_clearOverlay();
    g->needResize = false;
    if (g->W < 64 || g->H < 48) return;
    g->u = g->W / 320;
    int u2 = g->H / 240;
    if (u2 < g->u) g->u = u2;
    if (g->u < 1) g->u = 1;
    int rw, rh;
    if (g->oRes == 0) { rw = g->W; rh = g->H; }
    else if (g->oRes == 1) { rw = g->W * 3 / 4; rh = g->H * 3 / 4; }
    else if (g->oRes == 2) { rw = g->W / 2; rh = g->H / 2; }
    else { rw = g->W / 3; rh = g->H / 3; }
    g->rs = (rw == g->W) ? 1 : 2;
    int focal = g->oWide ? rw * 4 / 8 : rw * 5 / 8;
    int* oldScr = g->scr;
    if (oldScr && oldScr != g->r.fb) free(oldScr);
    g->scr = NULL;
    R3D_resize(&g->r, rw, rh, focal);
    if (!g->r.fb || !g->r.zb) {
        snprintf(g->loadMsg, sizeof(g->loadMsg), "Not enough memory for world buffers");
        g->running = false;
        return;
    }
    if (g->rs == 1) {
        g->scr = g->r.fb;
    } else {
        g->scr = (int*)malloc((size_t)g->W * g->H * sizeof(int));
        if (!g->scr) {
            snprintf(g->loadMsg, sizeof(g->loadMsg), "Not enough memory for HUD buffer");
            g->running = false;
        }
    }
    free(g->trail);
    g->trail = NULL;
}

static int mapKey(Game* g, int k) {
    (void)g;
    switch (k) {
        case KEY_UP: return GAME_A_UP;
        case KEY_DOWN: return GAME_A_DOWN;
        case KEY_LEFT: return GAME_A_LEFT;
        case KEY_RIGHT: return GAME_A_RIGHT;
        case KEY_FIRE: return GAME_A_FIRE;
        case KEY_NUM2: return GAME_A_CAMU;
        case KEY_NUM8: return GAME_A_CAMD;
        case KEY_NUM4: return GAME_A_CAML;
        case KEY_NUM6: return GAME_A_CAMR;
        case KEY_NUM5: case 'f': case 'F': return GAME_A_FIRE;
        case KEY_STAR: case 'e': case 'E': return GAME_A_ENTER;
        case KEY_NUM1: case 'q': case 'Q': return GAME_A_SPRINT;
        case KEY_NUM0: case ' ': case 'b': case 'B': return GAME_A_HAND;
        case KEY_POUND: case 'p': case 'P': case 27: return GAME_A_PAUSE;
        case KEY_NUM3: case 'r': case 'R': return GAME_A_WEAP;
        case KEY_NUM7: return GAME_A_HAND;
        case KEY_NUM9: return GAME_A_ENTER;
        case 'w': case 'W': return GAME_A_UP;
        case 's': case 'S': return GAME_A_DOWN;
        case 'a': case 'A': return GAME_A_LEFT;
        case 'd': case 'D': return GAME_A_RIGHT;
        case 'i': case 'I': return GAME_A_CAMU;
        case 'k': case 'K': return GAME_A_CAMD;
        case 'j': case 'J': return GAME_A_CAML;
        case 'l': case 'L': return GAME_A_CAMR;
        case -6: case -7: case -21: case -22: return GAME_A_PAUSE;
        default: break;
    }
    return 0;
}

void Game_press(Game* g, int a) {
    g->down |= a;
    g->edges |= a;
}

void Game_keyPressed(Game* g, int k) {
    g->anyKey++;
    int a = mapKey(g, k);
    if (a == GAME_A_SPRINT) g->sprintPress = (int)System_currentTimeMillis();
    g->down |= a;
    g->edges |= a;
}

void Game_keyReleased(Game* g, int k) {
    int a = mapKey(g, k);
    if (a == GAME_A_SPRINT && g->sprintPress > 0 &&
        (int)System_currentTimeMillis() - g->sprintPress < 350) {
        g->edges |= GAME_A_TAPRUN;
    }
    if (a == GAME_A_SPRINT) g->sprintPress = 0;
    g->down &= ~a;
}

static void dim(Game* g, int f) {
    if (f >= 256) return;
    int count = g->rs > 1 ? g->W * g->H : g->r.W * g->r.H;
    for (int q = 0; q < count; q++) {
        int c = g->scr[q];
        g->scr[q] = (int)(((((uint32_t)(c & 0xFF00FF) * (uint32_t)f) >> 8) & 0xFF00FF) |
                          ((((uint32_t)(c & 0xFF00) * (uint32_t)f) >> 8) & 0xFF00));
    }
}

static void scrClearFull(Game* g) {
    (void)g;
}

static void drawMenu(Game* g, const char* title, const char* const* items, int n) {
    scrClearFull(g);
    Ui_menuBg(g->scr, g->W, g->H, g->u);
    Ui_menuTitle(g->scr, g->W, g->H, g->u, title);
    int y0 = (g->H <= 120 && n > 5) ? 18 : g->H * 36 / 100;
    int step = (g->H <= 120 && n > 5) ? 12 : 15 * g->u;
    for (int i = 0; i < n; i++) {
        Ui_menuItem(g->scr, g->W, g->H, g->u, g->W / 2, y0 + i * step, items[i], i == g->sel);
    }
}

static void doSplash(Game* g, int dt) {
    if (g->splashMs == 0) {
        Ui_loadFont();
        g->splashKeys = g->anyKey;
    }
    g->splashMs += dt;
    int count = g->rs > 1 ? g->W * g->H : g->r.W * g->r.H;
    for (int i = 0; i < count; i++) g->scr[i] = 0;
    int a = g->splashMs < 600 ? g->splashMs * 255 / 600 :
            (g->splashMs > 2600 ? (3200 - g->splashMs) * 255 / 600 : 255);
    if (a < 0) a = 0;
    if (Ui_faMain != NULL) {
        int c = (a << 16) | (a << 8) | a;
        Ui_textC(g->scr, g->W, g->H, g->W / 2, g->H / 2 - 8 * g->u, CREDIT, c, g->u);
    }
    if (g->splashMs > 3200 || (g->anyKey != g->splashKeys && g->splashMs > 300)) {
        g->state = GAME_S_LOAD;
        g->loadMs = 0;
        g->edges = 0;
    }
}

static void doVideo(Game* g) {
    (void)g;
    int count = g->rs > 1 ? g->W * g->H : g->r.W * g->r.H;
    for (int i = 0; i < count; i++) g->scr[i] = 0;
    g->state = GAME_S_MENU;
    g->sel = 0;
    g->edges = 0;
    g->down = 0;
    g->fadeMs = 0;
}

static void loadOne(Game* g, int i) {
    if (i < UI_STEPS) {
        Ui_loadStep(i);
        if (i == UI_STEPS - 1) g->uiReady = true;
        return;
    }
    i -= UI_STEPS;
    if (i == 0) {
        Assets_r = &g->r;
        g->world = World_new(&g->r);
        if (!g->world) {
            g->loadErr = 1;
            snprintf(g->loadMsg, sizeof(g->loadMsg), "Not enough memory for world");
            return;
        }
        World_loadIndex(g->world);
        if (g->world->tnx <= 0) {
            g->loadErr = 1;
            snprintf(g->loadMsg, sizeof(g->loadMsg), "mapidx.bin unavailable, incomplete or out of memory");
        }
    } else if (i == 1) {
        AnimData_load();
        World_loadTimecyc(g->world);
        Sky_load();
        Snd_init();
    } else if (i == 2) {
        World_loadPools(g->world);
    } else if (i == 3) {
        World_preload(g->world, g->world->spX, g->world->spZ);
    } else if (i == 4) {
        World_reset(g->world);
        g->world->brightness = g->oBright * 14 / 15;
        g->world->drawDist = 60 + g->oDraw * 12;
        World_startLoader(g->world);
    }
}

static void doLoad(Game* g, int dt, int e) {
    g->loadMs += dt;
    if (g->loadI < g->total && g->loadErr == 0) {
        int t0 = (int)System_currentTimeMillis();
        int budget = g->loadMs < 30 ? 1 : 3;
        for (int k = 0; k < budget && g->loadI < g->total && !g->loadErr; k++) {
            loadOne(g, g->loadI);
            g->loadI++;
            if ((int)System_currentTimeMillis() - t0 > 60) break;
        }
    }
    int count = g->rs > 1 ? g->W * g->H : g->r.W * g->r.H;
    for (int i = 0; i < count; i++) g->scr[i] = 0;

    bool intro = g->loadMs < 2600;
    if (e != 0 && Ui_scr[1] != NULL && g->loadMs > 400) intro = false;
    if (intro && Ui_scr[0] != NULL) {
        int f = 256;
        if (g->loadMs < 500) f = g->loadMs * 256 / 500;
        else if (g->loadMs > 2100) f = (2600 - g->loadMs) * 256 / 500;
        Ui_stretch(Ui_scr[0], 320, 240, g->scr, g->W, g->H, f < 0 ? 0 : f);
    } else {
        int idx = 1 + ((g->loadMs / 2300) % 4);
        if (Ui_scr[idx] == NULL) idx = 1;
        if (Ui_scr[idx] != NULL) Ui_stretch(Ui_scr[idx], 320, 240, g->scr, g->W, g->H, 256);
        int bw = g->W * 6 / 10, bx = (g->W - bw) / 2, by = g->H - 14 * g->u;
        Ui_fill(g->scr, g->W, g->H, bx - g->u, by - g->u, bw + 2 * g->u, 5 * g->u, 0x101018);
        Ui_fill(g->scr, g->W, g->H, bx, by, bw * g->loadI / g->total, 3 * g->u, 0xFF6EC7);
        if (g->loadErr != 0) {
            char msg[192];
            snprintf(msg, sizeof(msg), "error %s", g->loadMsg);
            Ui_text(g->scr, g->W, g->H, 6 * g->u, 6 * g->u, msg, 0xFF4040, g->u);
        }
        if (g->loadI >= g->total && g->loadMs > 3200) {
            g->state = GAME_S_VIDEO;
            g->sel = 0;
            g->edges = 0;
            for (int q = 0; q < 5; q++) {
                free(Ui_scr[q]);
                Ui_scr[q] = NULL;
            }
        }
    }
}

static void menuNav(Game* g, int e, int n) {
    if ((e & GAME_A_UP) != 0) g->sel = (g->sel + n - 1) % n;
    if ((e & GAME_A_DOWN) != 0) g->sel = (g->sel + 1) % n;
}

static void doMenu(Game* g, int e) {
    if (!g->saveChecked) {
        g->hasSave = Save_exists();
        g->saveChecked = true;
    }
    int n = g->hasSave ? 5 : 4;
    const char* const* items = g->hasSave ? MAIN_C_ITEMS : MAIN_ITEMS;
    menuNav(g, e, n);
    if ((e & GAME_A_FIRE) != 0) {
        int s = g->hasSave ? g->sel - 1 : g->sel;
        if (g->hasSave && g->sel == 0) {
            g->state = GAME_S_START;
            g->startMs = 0;
            g->startKind = 1;
        } else if (s == 0) {
            g->state = GAME_S_START;
            g->startMs = 0;
            g->startKind = 0;
        } else if (s == 1) {
            g->prevState = GAME_S_MENU;
            g->state = GAME_S_OPT;
            g->optSel = 0;
        } else if (s == 2) {
            g->prevState = GAME_S_MENU;
            g->state = GAME_S_CTRL;
        } else {
            g->running = false;
        }
    }
    drawMenu(g, "grand theft auto", items, n);
    Ui_textC(g->scr, g->W, g->H, g->W / 2, g->H - 14 * g->u, "a port made by aidencantcode", 0xB0B0C0, 1);
}

static void finishSessionStart(Game* g) {
    g->pedDataOk = (g->world->pm0 != NULL && g->world->playerTex >= 0 && AnimData_n > 0);
    fillPopulation(g->world);
    g->state = GAME_S_PLAY;
    g->down = 0;
    g->edges = 0;
    g->titleMs = 0;
    g->started = true;
    g->playFade = 0;
}

bool Game_loadProgress(Game* g, const char* path) {
    if (!g || !g->world || g->loadErr || g->loadI < g->total) return false;
    if (!Save_loadPath(g->world, path)) return false;
    applyOptions(g, false);
    finishSessionStart(g);
    return true;
}

static void doStart(Game* g, int dt) {
    g->startMs += dt;
    drawMenu(g, "grand theft auto", g->hasSave ? MAIN_C_ITEMS : MAIN_ITEMS, g->hasSave ? 5 : 4);
    int f = 256 - g->startMs * 256 / 500;
    if (f < 0) f = 0;
    dim(g, f);
    if (g->startMs >= 550) {
        applyOptions(g, false);
        if (g->startKind == 1) {
            if (!Save_load(g->world)) {
                g->state = GAME_S_MENU;
                g->saveChecked = false;
                g->edges = g->down = 0;
                RG_LOGW("Progress load failed; returning to menu");
                return;
            }
        } else World_reset(g->world);
        finishSessionStart(g);
    }
}

static void doPause(Game* g, int e) {
    menuNav(g, e, 7);
    if ((e & GAME_A_PAUSE) != 0) {
        g->state = GAME_S_PLAY;
        g->down = 0;
        return;
    }
    if ((e & GAME_A_FIRE) != 0) {
        switch (g->sel) {
            case 0: g->state = GAME_S_PLAY; g->down = 0; break;
            case 1: applyOptions(g, false); World_reset(g->world); fillPopulation(g->world); g->state = GAME_S_PLAY; g->down = 0; break;
            case 2: g->prevState = GAME_S_PAUSE; g->state = GAME_S_MAP; break;
            case 3: g->prevState = GAME_S_PAUSE; g->state = GAME_S_STATS; break;
            case 4: g->prevState = GAME_S_PAUSE; g->state = GAME_S_CTRL; break;
            case 5: g->prevState = GAME_S_PAUSE; g->state = GAME_S_OPT; g->optSel = 0; break;
            default: Save_write(g->world); g->saveChecked = false; g->state = GAME_S_MENU; g->sel = 0; break;
        }
    }
    if (g->state == GAME_S_PAUSE) drawMenu(g, "pause menu", PAUSE_ITEMS, 7);
}

static int countAlivePeds(World* w) {
    int alive = 0;
    for (int i = 0; i < w->nped; i++) {
        Ped* p = &w->peds[i];
        if (p->seat != NULL || p->cop || p->state == PED_DEAD) continue;
        alive++;
    }
    return alive;
}

static int countTraffic(World* w) {
    int n = 0;
    for (int i = 0; i < w->ncars; i++) {
        if (w->cars[i].mode == CAR_TRAFFIC) n++;
    }
    return n;
}

/* Increasing the traffic-density option must be visible immediately, not
   only after the next ambient-spawn tick. */
static void fillPopulation(World* w) {
    if (!w) return;
    int alive = countAlivePeds(w);
    int fails = 0;
    for (int tries = 0; tries < w->maxPed * 8 && alive < w->maxPed; tries++) {
        int before = alive;
        World_spawnPed(w, false);
        alive = countAlivePeds(w);
        if (alive == before) {
            if (++fails >= 8) break;
        } else {
            fails = 0;
        }
    }
    int traffic = countTraffic(w);
    fails = 0;
    for (int tries = 0; tries < w->maxTraffic * 8 && traffic < w->maxTraffic; tries++) {
        int before = traffic;
        World_spawnTraffic(w, false);
        traffic = countTraffic(w);
        if (traffic == before) {
            if (++fails >= 8) break;
        } else {
            fails = 0;
        }
    }
}

static void applyOptions(Game* g, bool fill) {
    if (!g->world) return;
    World* w = g->world;
    w->radioEnabled = g->oRadio;
    if (!g->oRadio) {
        Snd_radioStop();
        w->radioInCar = false;
    }
    w->brightness = g->oBright * 14 / 15;
    w->drawDist = 60 + g->oDraw * 12;
    if (g->oDensity == 0) {
        w->maxPed = g->legacy ? 6 : 12;
        w->maxTraffic = g->legacy ? 2 : 3;
    } else if (g->oDensity == 1) {
        w->maxPed = g->legacy ? 10 : 22;
        w->maxTraffic = g->legacy ? 3 : 8;
    } else if (g->oDensity == 2) {
        w->maxPed = g->legacy ? 16 : 30;
        w->maxTraffic = g->legacy ? 5 : 10;
    } else {
        /* Very high: same pedestrian cap, but much more road traffic. */
        w->maxPed = g->legacy ? 16 : 30;
        w->maxTraffic = g->legacy ? 8 : 12;
    }
    /* trim an existing population down to the new cap */
    int alive = countAlivePeds(w);
    for (int i = w->nped - 1; i >= 0 && alive > w->maxPed; i--) {
        Ped* p = &w->peds[i];
        if (p->seat == NULL && !p->cop && p->state != PED_DEAD) {
            p->state = PED_DEAD;
            p->deadT = -1;
            p->timer = 0;
            alive--;
        }
    }
    for (;;) {
        int traffic = countTraffic(w);
        int victim = -1;
        for (int i = 0; i < w->ncars; i++) {
            if (w->cars[i].mode == CAR_TRAFFIC && &w->cars[i] != w->inCar &&
                &w->cars[i] != w->pActCar) {
                victim = i;
            }
        }
        if (traffic <= w->maxTraffic || victim < 0) break;
        World_removeCar(w, victim);
    }
    if (fill) fillPopulation(w);
}

void Game_refreshOptions(Game* g) {
    if (!g) return;
    applyOptions(g, true);
}

void Game_logOptions(const Game *g) {
    RG_LOGI("Options: brightness=%d draw=%d density=%d resolution=%d wide=%s radar=%d HUD=%s",
            g->oBright, g->oDraw, g->oDensity, g->oRes, g->oWide ? "on" : "off", g->oRadar, g->oHud ? "on" : "off");
    RG_LOGI("Options: limiter=%s filter=%s subtitles=%s legend=%s radio=%s",
            g->oLimiter ? "on" : "off", g->oTrails ? "on" : "off", g->oSubs ? "on" : "off",
            g->oLegend ? "on" : "off", g->oRadio ? "on" : "off");
}

static void doOptions(Game* g, int e) {
    int n = 14;
    bool changed = false;
    if ((e & GAME_A_UP) != 0) g->optSel = (g->optSel + n - 1) % n;
    if ((e & GAME_A_DOWN) != 0) g->optSel = (g->optSel + 1) % n;
    int dlt = ((e & GAME_A_RIGHT) != 0 ? 1 : 0) - ((e & GAME_A_LEFT) != 0 ? 1 : 0);
    if ((e & GAME_A_FIRE) != 0 && g->optSel < 12) dlt = 1;
    if (dlt != 0) {
        changed = true;
        switch (g->optSel) {
            case 0: g->oBright = Fx_clamp(g->oBright + dlt, 0, 15); break;
            case 1: g->oDraw = Fx_clamp(g->oDraw + dlt, 0, 15); break;
            case 2: g->oLimiter = !g->oLimiter; break;
            case 3: g->oTrails = !g->oTrails; break;
            case 4: g->oSubs = !g->oSubs; break;
            case 5: g->oWide = !g->oWide; g->needResize = true; break;
            case 6: g->oLegend = !g->oLegend; break;
            case 7: g->oRadar = (g->oRadar + 3 + dlt) % 3; break;
            case 8: g->oHud = !g->oHud; break;
            case 9: g->oDensity = (g->oDensity + 4 + dlt) % 4; break;
            case 10: g->oRes = (g->oRes + 1) % 4; g->needResize = true; break;
            case 11: g->oRadio = !g->oRadio; break;
            default: break;
        }
    }
    if ((e & GAME_A_FIRE) != 0 && g->optSel == 12) {
        g->oBright = 10; g->oDraw = 2; g->oLimiter = true; g->oTrails = false;
        g->oSubs = true; g->oWide = false; g->oLegend = false; g->oRadar = 0;
        g->oHud = true; g->oRadio = true; g->oRes = GTAVC_DEFAULT_RESOLUTION; g->oDensity = 1; g->needResize = true;
        changed = true;
    }
    bool leaving = ((e & GAME_A_FIRE) != 0 && g->optSel == 13) || (e & GAME_A_PAUSE) != 0;
    if (leaving) {
        g->state = (g->prevState == GAME_S_PAUSE) ? GAME_S_PAUSE : GAME_S_MENU;
    }
    applyOptions(g, changed);
    if (changed) Game_logOptions(g);
    if (changed || leaving) Save_writeOptions(g);
    scrClearFull(g);
    Ui_menuBg(g->scr, g->W, g->H, g->u);
    Ui_menuTitle(g->scr, g->W, g->H, g->u, "display");
    int x0 = g->W * 15 / 100, xr = g->W * 87 / 100;
    int step = (g->H <= 120) ? 10 * g->u : 11 * g->u;
    int y0 = (g->H <= 120) ? 16 : g->H * 20 / 100;
    int max_vis = (g->H <= 120) ? 8 : n;
    int start_i = 0;
    if (g->H <= 120 && g->optSel >= max_vis) {
        start_i = g->optSel - max_vis + 1;
    }
    for (int idx = 0; idx < max_vis && (start_i + idx) < n; idx++) {
        int i = start_i + idx;
        int y = y0 + idx * step;
        if (i == g->optSel) Ui_fill(g->scr, g->W, g->H, x0 - 4 * g->u, y - g->u, xr - x0 + 8 * g->u, (step > 10 ? 11 : 10) * g->u, UI_HILITE);
        if (i >= 12) {
            Ui_textC(g->scr, g->W, g->H, g->W / 2, y, OPT_NAMES[i], UI_PINK, g->u);
            continue;
        }
        Ui_text(g->scr, g->W, g->H, x0, y, OPT_NAMES[i], UI_PINK, g->u);
        switch (i) {
            case 0: Ui_bars(g->scr, g->W, g->H, g->u, xr, y, g->oBright, 15); break;
            case 1: Ui_bars(g->scr, g->W, g->H, g->u, xr, y, g->oDraw, 15); break;
            case 2: Ui_textR(g->scr, g->W, g->H, xr, y, g->oLimiter ? "on" : "off", UI_PINK, g->u); break;
            case 3: Ui_textR(g->scr, g->W, g->H, xr, y, g->oTrails ? "on" : "off", UI_PINK, g->u); break;
            case 4: Ui_textR(g->scr, g->W, g->H, xr, y, g->oSubs ? "on" : "off", UI_PINK, g->u); break;
            case 5: Ui_textR(g->scr, g->W, g->H, xr, y, g->oWide ? "on" : "off", UI_PINK, g->u); break;
            case 6: Ui_textR(g->scr, g->W, g->H, xr, y, g->oLegend ? "on" : "off", UI_PINK, g->u); break;
            case 7: Ui_textR(g->scr, g->W, g->H, xr, y, g->oRadar == 0 ? "map & blips" : (g->oRadar == 1 ? "blips only" : "off"), UI_PINK, g->u); break;
            case 8: Ui_textR(g->scr, g->W, g->H, xr, y, g->oHud ? "on" : "off", UI_PINK, g->u); break;
            case 9: Ui_textR(g->scr, g->W, g->H, xr, y,
                             g->oDensity == 0 ? "low" : (g->oDensity == 1 ? "medium" : (g->oDensity == 2 ? "high" : "very high")), UI_PINK, g->u); break;
            case 10: {
                char buf[32];
                int rw = g->W, rh = g->H;
                if (g->oRes == 1) { rw = g->W * 3 / 4; rh = g->H * 3 / 4; }
                else if (g->oRes == 2) { rw = g->W / 2; rh = g->H / 2; }
                else if (g->oRes == 3) { rw = g->W / 3; rh = g->H / 3; }
                snprintf(buf, sizeof(buf), "%dx%dx32", rw, rh);
                Ui_textR(g->scr, g->W, g->H, xr, y, buf, UI_PINK, g->u);
                break;
            }
            case 11: Ui_textR(g->scr, g->W, g->H, xr, y, g->oRadio ? "on" : "off", UI_PINK, g->u); break;
            default: break;
        }
    }
    Ui_textR(g->scr, g->W, g->H, g->W - 6 * g->u, g->H - 11 * g->u, "left, right to adjust", 0xE0E0E0, 1);
}

static void draw2(Game* g) {
    scrClearFull(g);
    Ui_menuBg(g->scr, g->W, g->H, g->u);
    int x = (g->W <= 160) ? 6 : g->W * 14 / 100;
    int y = (g->H <= 120) ? 18 : g->H * 24 / 100;
    int st = (g->H <= 120) ? 10 * g->u : 12 * g->u;
    if (g->state == GAME_S_CTRL) {
        Ui_menuTitle(g->scr, g->W, g->H, g->u, "controls");
        const char* t[8] = {
            "d-pad: move / steer", "A: sprint / accelerate", "B: attack / brake+rev",
            "X: jump / handbrake", "Y: enter / exit vehicle",
            "L: cam left / handbrake", "R: cam right / look back", "Start: pause  Select: weap"
        };
        for (int i = 0; i < 8; i++) Ui_text(g->scr, g->W, g->H, x, y + i * st, t[i], UI_PINK, g->u);
    } else if (g->state == GAME_S_STATS) {
        Ui_menuTitle(g->scr, g->W, g->H, g->u, "stats");
        World* w = g->world;
        char clk[16];
        World_clockStr(w, clk, sizeof(clk));
        char t[6][96];
        snprintf(t[0], sizeof(t[0]), "money: $%d", w->money);
        snprintf(t[1], sizeof(t[1]), "time: %s  day %d", clk, w->dayCount + 1);
        snprintf(t[2], sizeof(t[2]), "people hurt: %d", w->pedsHit);
        snprintf(t[3], sizeof(t[3]), "cars stolen: %d", w->carsStolen);
        snprintf(t[4], sizeof(t[4]), "wanted level: %d", w->wanted);
        snprintf(t[5], sizeof(t[5]), "health: %d", w->health);
        for (int i = 0; i < 6; i++) Ui_text(g->scr, g->W, g->H, x, y + i * st, t[i], UI_PINK, g->u);
    } else {
        Ui_menuTitle(g->scr, g->W, g->H, g->u, "map");
        World* w = g->world;
        if (Ui_radImg == NULL) return;
        int mh = g->H * 74 / 100;
        int mw = mh * Ui_radW / Ui_radH;
        if (mw > g->W - 20 * g->u) {
            mw = g->W - 20 * g->u;
            mh = mw * Ui_radH / Ui_radW;
        }
        int ox = g->W / 2 - mw / 2, oy = g->H * 20 / 100;
        for (int j = 0; j < mh; j++) {
            int yy = oy + j;
            if (yy >= g->H) break;
            int sj = j * Ui_radH / mh;
            for (int i = 0; i < mw; i++) {
                g->scr[yy * g->W + ox + i] = Ui_radImg[sj * Ui_radW + i * Ui_radW / mw] & 0xFFFFFF;
            }
        }
        int pxm = (w->player->x - Ui_radX0) / Ui_radCell * mw / Ui_radW;
        int pym = (Ui_radZ0 - w->player->z) / Ui_radCell * mh / Ui_radH;
        Ui_fill(g->scr, g->W, g->H, ox + pxm - 3 * g->u, oy + pym - 3 * g->u, 6 * g->u, 6 * g->u, 0x000000);
        Ui_fill(g->scr, g->W, g->H, ox + pxm - 2 * g->u, oy + pym - 2 * g->u, 4 * g->u, 4 * g->u,
                (g->blink / 300) % 2 == 0 ? 0xFF40D0 : 0xFFFFFF);
    }
}

static void doDebug(Game* g, int e) {
    World* w = g->world;
    int n = 9;
    if ((e & GAME_A_UP) != 0) g->dbgSel = (g->dbgSel + n - 1) % n;
    if ((e & GAME_A_DOWN) != 0) g->dbgSel = (g->dbgSel + 1) % n;
    int d = ((e & GAME_A_RIGHT) != 0 ? 1 : 0) - ((e & GAME_A_LEFT) != 0 ? 1 : 0);
    bool ok = (e & GAME_A_FIRE) != 0;
    switch (g->dbgSel) {
        case 0:
            if (w->carNamesCount > 0) {
                g->dbgCar = (g->dbgCar + d + w->carNamesCount) % w->carNamesCount;
                if (ok) { World_debugSpawn(w, g->dbgCar); g->state = GAME_S_PLAY; }
            }
            break;
        case 1: if (d != 0) w->minutes = (w->minutes + d * 30 + 1440) % 1440; break;
        case 2: if (d != 0) w->speedMul = Fx_clamp(w->speedMul + d * 25, 50, 300); break;
        case 3: if (d != 0) g->gameSpeed = Fx_clamp(g->gameSpeed + d * 25, 25, 300); break;
        case 4: if (ok || d != 0) w->god = !w->god; break;
        case 5: if (ok) { w->wanted = 0; World_clearCops(w); } break;
        case 6: if (ok) { w->hasBat = true; w->hasPistol = true; w->ammo += 200; w->money += 5000; } break;
        case 7: if (ok) { w->inCar = NULL; w->player->seat = NULL; w->pAct = 0; World_placePlayerAtSpawn(w); World_preload(w, w->player->x, w->player->z); g->state = GAME_S_PLAY; } break;
        default: if (ok) g->state = GAME_S_PLAY; break;
    }
    if ((e & GAME_A_PAUSE) != 0) g->state = GAME_S_PLAY;
    if (g->state == GAME_S_PLAY) { g->down = 0; return; }

    World_render(w);
    Ui_fillA(g->scr, g->W, g->H, 0, 0, g->W, g->H, 0x000000, 150);
    Ui_textC(g->scr, g->W, g->H, g->W / 2, 6 * g->u, "debug menu", UI_PINK, g->u);
    int y0 = 24 * g->u, st = 13 * g->u;
    for (int i = 0; i < n; i++) {
        char v[128];
        v[0] = '\0';
        char clk[16];
        switch (i) {
            case 0: if (w->carNamesCount > 0) snprintf(v, sizeof(v), "< %s >", w->carNames[g->dbgCar]); break;
            case 1: World_clockStr(w, clk, sizeof(clk)); snprintf(v, sizeof(v), "< %s >", clk); break;
            case 2: snprintf(v, sizeof(v), "< x%d.%d >", w->speedMul / 100, (w->speedMul % 100) / 10); break;
            case 3: snprintf(v, sizeof(v), "< x%d.%d >", g->gameSpeed / 100, (g->gameSpeed % 100) / 10); break;
            case 4: snprintf(v, sizeof(v), "%s", w->god ? "on" : "off"); break;
            default: break;
        }
        int y = y0 + i * st;
        if (i == g->dbgSel) Ui_fill(g->scr, g->W, g->H, 6 * g->u, y - g->u, g->W - 12 * g->u, 12 * g->u, UI_HILITE);
        Ui_text(g->scr, g->W, g->H, 10 * g->u, y, DBG_ITEMS[i], UI_WHITE, g->u);
        if (v[0]) Ui_textR(g->scr, g->W, g->H, g->W - 10 * g->u, y, v, UI_BLUE, g->u);
    }
}

static void upscale(Game* g) {
    int rw = g->r.W, rh = g->r.H;
    int* src = g->r.fb;

    /* Fast path for 2x upscale (160x120 -> 320x240): linear write + block memcpy */
    if (rw * 2 == g->W && rh * 2 == g->H) {
        int W = g->W;
        for (int sy = 0; sy < rh; sy++) {
            const int* sline = &src[sy * rw];
            uint32_t* dline0 = (uint32_t*)&g->scr[(sy * 2) * W];
            uint32_t* dline1 = (uint32_t*)&g->scr[(sy * 2 + 1) * W];
            for (int sx = 0; sx < rw; sx++) {
                uint32_t px = (uint32_t)sline[sx];
                dline0[sx * 2]     = px;
                dline0[sx * 2 + 1] = px;
            }
            memcpy(dline1, dline0, (size_t)W * sizeof(uint32_t));
        }
        return;
    }

    for (int y = 0; y < g->H; y++) {
        int sy = y * rh / g->H;
        if (sy >= rh) sy = rh - 1;
        int so = sy * rw, d0 = y * g->W;
        for (int x = 0; x < g->W; x++) {
            int sx = x * rw / g->W;
            if (sx >= rw) sx = rw - 1;
            g->scr[d0 + x] = src[so + sx];
        }
    }
}

static void ps2Filter(Game* g) {
    int W = g->W, H = g->H;
    int bw = W >> 2, bh = H >> 2;
    if (g->blm == NULL || g->blm2 == NULL) {
        g->blm = (int*)malloc((size_t)bw * bh * sizeof(int));
        g->blm2 = (int*)malloc((size_t)bw * bh * sizeof(int));
    }
    for (int y = 0; y < bh; y++) {
        for (int x = 0; x < bw; x++) {
            int c = g->scr[(y << 2) * W + (x << 2)];
            int r0 = (c >> 16) & 255, g0 = (c >> 8) & 255, b0 = c & 255;
            int l = (r0 * 77 + g0 * 150 + b0 * 29) >> 8;
            int k = l > 150 ? (l - 150) * 2 : 0;
            g->blm[y * bw + x] = ((r0 * k >> 8) << 16) | ((g0 * k >> 8) << 8) | (b0 * k >> 8);
        }
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int y = 0; y < bh; y++) {
            for (int x = 0; x < bw; x++) {
                int rr = 0, gg = 0, bb = 0, n = 0;
                for (int j = -1; j <= 1; j++) {
                    int yy = y + j;
                    if (yy < 0 || yy >= bh) continue;
                    for (int i = -1; i <= 1; i++) {
                        int xx = x + i;
                        if (xx < 0 || xx >= bw) continue;
                        int c = g->blm[yy * bw + xx];
                        rr += (c >> 16) & 255;
                        gg += (c >> 8) & 255;
                        bb += c & 255;
                        n++;
                    }
                }
                g->blm2[y * bw + x] = ((rr / n) << 16) | ((gg / n) << 8) | (bb / n);
            }
        }
        int* t = g->blm;
        g->blm = g->blm2;
        g->blm2 = t;
    }
    bool trails = g->trail != NULL;
    if (!trails) g->trail = (int*)calloc((size_t)W * H, sizeof(int));
    for (int y = 0; y < H; y++) {
        int by = (y >> 2) < bh ? (y >> 2) : bh - 1;
        int o = y * W;
        for (int x = 0; x < W; x++) {
            int c = g->scr[o + x];
            int b = g->blm[by * bw + ((x >> 2) < bw ? (x >> 2) : bw - 1)];
            int r0 = ((c >> 16) & 255) * 270 >> 8;
            int g0 = ((c >> 8) & 255) * 252 >> 8;
            int b0 = (c & 255) * 222 >> 8;
            r0 += (b >> 16) & 255;
            g0 += (b >> 8) & 255;
            b0 += b & 255;
            if (trails) {
                int t = g->trail[o + x];
                r0 = (r0 * 5 + ((t >> 16) & 255) * 3) >> 3;
                g0 = (g0 * 5 + ((t >> 8) & 255) * 3) >> 3;
                b0 = (b0 * 5 + (t & 255) * 3) >> 3;
            }
            if (r0 > 255) r0 = 255;
            if (g0 > 255) g0 = 255;
            if (b0 > 255) b0 = 255;
            int out = (r0 << 16) | (g0 << 8) | b0;
            g->scr[o + x] = out;
            if (g->trail) g->trail[o + x] = out;
        }
    }
}

static void overlay(Game* g) {
    World* w = g->world;
    if (!g->pedDataOk) {
        Ui_fillA(g->scr, g->W, g->H, 0, 0, g->W, 14 * g->u, 0x300000, 200);
        Ui_textC(g->scr, g->W, g->H, g->W / 2, 4 * g->u,
                 "PED DATA MISSING (anim.bin / p_*.bin)", 0xFF6060, g->u);
    }
    if (g->oHud) Ui_hudDraw(g->scr, g->W, g->H, w, g->u, true, g->oRadar);
    if (w->missionState == WORLD_MISSION_ACTIVE && w->helpT <= 0) {
        int px = 4 * g->u, py = 18 * g->u;
        int sec = (w->missionTimer + 999) / 1000;
        if (sec < 0) sec = 0;
        char tbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%d:%02d", sec / 60, sec % 60);
        int mw = (g->W <= 160) ? (g->W - 8 * g->u) : 220 * g->u;
        Ui_fillA(g->scr, g->W, g->H, px, py - 2 * g->u, mw, 28 * g->u, 0x000000, 160);
        Ui_text(g->scr, g->W, g->H, px + 2 * g->u, py, "An Old Friend", 0xFFD040, g->u);
        Ui_text(g->scr, g->W, g->H, px + 2 * g->u, py + 10 * g->u, w->missionText, 0xFFFFFF, g->u);
        Ui_textR(g->scr, g->W, g->H, px + mw - 2 * g->u, py, tbuf, 0xFFFF80, g->u);
    }
    if (w->helpT > 0 && g->oSubs && w->help[0]) Ui_helpBox(g->scr, g->W, g->H, g->u, w->help);
    if (g->titleMs < 4200) {
        int a = g->titleMs < 600 ? g->titleMs * 255 / 600 :
                (g->titleMs > 3400 ? (4200 - g->titleMs) * 255 / 800 : 255);
        const char* z = g->legacy ? "Ocean Beach" : "Washington Beach";
        int tw = Ui_textW(z, g->u * 2);
        if (a > 120) Ui_text(g->scr, g->W, g->H, g->W - 8 * g->u - tw, g->H - 28 * g->u, z, 0x28D890, g->u * 2);
    }
    if (w->busted) {
        Ui_fillA(g->scr, g->W, g->H, 0, 0, g->W, g->H, 0x000000, (w->bustedT / 14) < 150 ? (w->bustedT / 14) : 150);
        const char* s = "busted";
        int sc = (g->W <= 160) ? 2 : g->u * 3;
        Ui_text(g->scr, g->W, g->H, g->W / 2 - Ui_textW(s, sc) / 2, g->H / 2 - 8 * sc, s, UI_PINK, sc);
    }
    if (w->health <= 0) {
        Ui_fillA(g->scr, g->W, g->H, 0, 0, g->W, g->H, 0x000000, (w->wastedT / 14) < 150 ? (w->wastedT / 14) : 150);
        const char* s = "wasted";
        int sc = (g->W <= 160) ? 2 : g->u * 3;
        Ui_text(g->scr, g->W, g->H, g->W / 2 - Ui_textW(s, sc) / 2, g->H / 2 - 8 * sc, s, UI_PINK, sc);
    }
    if (g->oLegend) {
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "fps %d", g->fps);
        Ui_text(g->scr, g->W, g->H, 4 * g->u, g->H - 11 * g->u, fpsText, 0xFFFF80, 1);
    }
}

static void doPlay(Game* g, int dt, int e, bool draw_frame) {
    if ((g->down & GAME_A_HAND) != 0 && (e & GAME_A_ENTER) != 0) {
        Video_materialize();
        g->state = GAME_S_DEBUG;
        g->dbgSel = 0;
        g->edges = 0;
        return;
    }
    dt = dt * g->gameSpeed / 100;
    if ((e & GAME_A_PAUSE) != 0) {
        Video_materialize();
        g->state = GAME_S_PAUSE;
        g->sel = 0;
        g->prevState = GAME_S_PLAY;
        Save_write(g->world);
        Snd_stopAll();
        return;
    }
    int k = 0, d = g->down;
    if ((d & GAME_A_UP) != 0) k |= WORLD_K_UP;
    if ((d & GAME_A_DOWN) != 0) k |= WORLD_K_DOWN;
    if ((d & GAME_A_LEFT) != 0) k |= WORLD_K_LEFT;
    if ((d & GAME_A_RIGHT) != 0) k |= WORLD_K_RIGHT;
    if ((d & GAME_A_FIRE) != 0) k |= WORLD_K_FIRE;
    if ((d & GAME_A_CAML) != 0) k |= WORLD_K_CAML;
    if ((d & GAME_A_CAMR) != 0) k |= WORLD_K_CAMR;
    if ((d & GAME_A_CAMU) != 0) k |= WORLD_K_CAMU;
    if ((d & GAME_A_CAMD) != 0) k |= WORLD_K_CAMD;
    if ((d & GAME_A_SPRINT) != 0) k |= WORLD_K_SPRINT;
    if ((d & GAME_A_HAND) != 0) k |= WORLD_K_HAND;

    g->titleMs += dt;
    int64_t t_up0 = GTAVC_PROFILE_TIMER();
    World_update(g->world, dt, k, (e & GAME_A_FIRE) != 0, (e & GAME_A_ENTER) != 0,
                 (e & GAME_A_TAPRUN) != 0, (e & GAME_A_WEAP) != 0,
                 (e & GAME_A_HAND) != 0 && g->world->inCar == NULL);
    uint32_t update_us = (uint32_t)(GTAVC_PROFILE_TIMER() - t_up0);
    GTAVC_PROF(g_prof.t_update_us += update_us);
    if (update_us > g_prof.update_peak_us) GTAVC_PROF(g_prof.update_peak_us = update_us);


    World* w = g->world;
    if (w->inCar == NULL && w->helpT <= 0 && w->helpShown == 0) {
        for (int i = 0; i < w->ncars; i++) {
            if (Fx_dist(w->cars[i].x - w->player->x, w->cars[i].z - w->player->z) < 3800) {
                World_say(w, "Press * to enter or exit a vehicle.");
                w->helpShown = 1;
                break;
            }
        }
    }
    if (draw_frame) {
        int64_t t_ren0 = GTAVC_PROFILE_TIMER();
        World_render(w);
        GTAVC_PROF(g_prof.t_render_us += (uint64_t)(GTAVC_PROFILE_TIMER() - t_ren0));
    }
}

void Game_frame(Game* g, int dt, bool draw_frame) {
    if (g->state != GAME_S_PLAY) Video_materialize();
    Video_pauseTracking();
    if (g->needResize) Game_resize(g);
    if (!g->running || !g->scr || !g->r.fb || !g->r.zb) return;
    int e = g->edges;
    g->edges = 0;
    int stateBefore = g->state;
    /* A is the confirm button in menus (Switch layout). */
    if (g->state != GAME_S_PLAY && (e & GAME_A_SPRINT)) e |= GAME_A_FIRE;
    g->blink += dt;
    g->fpsMs += dt;
    g->fpsCount++;
    if (g->fpsMs >= 1000) {
        g->fps = g->fpsCount;
        g->fpsCount = 0;
        g->fpsMs = 0;
    }

    switch (g->state) {
        case GAME_S_SPLASH: doSplash(g, dt); break;
        case GAME_S_VIDEO: doVideo(g); break;
        case GAME_S_LOAD: doLoad(g, dt, e); break;
        case GAME_S_MENU: doMenu(g, e); break;
        case GAME_S_START: doStart(g, dt); break;
        case GAME_S_DEBUG: doDebug(g, e); break;
        case GAME_S_OPT: doOptions(g, e); break;
        case GAME_S_CTRL:
        case GAME_S_MAP:
        case GAME_S_STATS:
            if ((e & (GAME_A_FIRE | GAME_A_PAUSE | GAME_A_ENTER)) != 0) {
                g->state = (g->prevState == GAME_S_PLAY || g->prevState == GAME_S_PAUSE) ? GAME_S_PAUSE : GAME_S_MENU;
            }
            draw2(g);
            break;
        case GAME_S_PAUSE: doPause(g, e); break;
        case GAME_S_PLAY: doPlay(g, dt, e, draw_frame); break;
        default: break;
    }
    if (stateBefore == GAME_S_PLAY && g->state != GAME_S_PLAY) {
        /* Do not leave an engine/siren/radio loop running behind the pause menu. */
        Snd_stopAll();
        Snd_radioStop();
        if (g->world) g->world->radioInCar = false;
    }

    if (g->state == GAME_S_MENU && g->fadeMs < 900) {
        g->fadeMs += dt;
        int f = g->fadeMs < 150 ? 0 : (g->fadeMs - 150) * 256 / 750;
        if (f < 256) {
            int count = g->rs > 1 ? g->W * g->H : g->r.W * g->r.H;
            for (int q = 0; q < count; q++) {
                int c = g->scr[q];
                g->scr[q] = (int)(((((uint32_t)(c & 0xFF00FF) * (uint32_t)f) >> 8) & 0xFF00FF) |
                                  ((((uint32_t)(c & 0xFF00) * (uint32_t)f) >> 8) & 0xFF00));
            }
        }
    }
    if (draw_frame) {
        bool native = g->nativeOutput && stateBefore == GAME_S_PLAY &&
            g->state == GAME_S_PLAY && g->W == 320 && g->H == 240 &&
            g->r.W > 0 && g->r.W < 320 && g->r.H > 0 && g->r.H < 240 && !g->oTrails &&
            g->playFade >= 800 && g->world && !g->world->busted && g->world->health > 0;
        if (native) Video_beginOverlay(g->scr, g->r.fb, g->r.W, g->r.H);
        else Video_clearOverlay();
        if (!native && g->rs > 1 && g->state == GAME_S_PLAY) {
            int64_t t_up0 = GTAVC_PROFILE_TIMER();
            upscale(g);
            GTAVC_PROF(g_prof.t_upscale_us += (uint64_t)(GTAVC_PROFILE_TIMER() - t_up0));
        }
        if (g->oTrails && g->state == GAME_S_PLAY) ps2Filter(g);
        if (g->state == GAME_S_PLAY) {
            int64_t t_ov0 = GTAVC_PROFILE_TIMER();
            overlay(g);
            GTAVC_PROF(g_prof.t_overlay_us += (uint64_t)(GTAVC_PROFILE_TIMER() - t_ov0));
        }
    }
    if (g->state == GAME_S_PLAY && g->playFade < 800) {
        g->playFade += dt;
        if (draw_frame) dim(g, g->playFade < 100 ? 0 : (g->playFade - 100) * 256 / 700);
    }
}

void Game_pointerPressed(Game* g, int x, int y) {
    (void)x; (void)y;
    g->anyKey++;
    if (g->state != GAME_S_PLAY) {
        g->down |= GAME_A_FIRE;
        g->edges |= GAME_A_FIRE;
        g->touchAct = GAME_A_FIRE;
        return;
    }
}

void Game_pointerReleased(Game* g, int x, int y) {
    (void)x; (void)y;
    g->down &= ~g->touchAct;
    g->touchAct = 0;
}
