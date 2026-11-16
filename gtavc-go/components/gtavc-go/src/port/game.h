#ifndef GTAVC_GAME_H
#define GTAVC_GAME_H

#include <stdint.h>
#include <stdbool.h>
#include "r3d.h"
#include "world.h"
#include "ui.h"

#define GAME_S_LOAD    0
#define GAME_S_MENU    1
#define GAME_S_OPT     2
#define GAME_S_PLAY    3
#define GAME_S_PAUSE   4
#define GAME_S_CTRL    5
#define GAME_S_MAP     6
#define GAME_S_STATS   7
#define GAME_S_SPLASH  8
#define GAME_S_VIDEO   9
#define GAME_S_START   10
#define GAME_S_DEBUG   11

#define GAME_A_UP     1
#define GAME_A_DOWN   2
#define GAME_A_LEFT   4
#define GAME_A_RIGHT  8
#define GAME_A_FIRE   16
#define GAME_A_ENTER  32
#define GAME_A_CAML   64
#define GAME_A_CAMR   128
#define GAME_A_CAMU   256
#define GAME_A_CAMD   512
#define GAME_A_SPRINT 1024
#define GAME_A_HAND   2048
#define GAME_A_PAUSE  4096
#define GAME_A_WEAP   8192
#define GAME_A_TAPRUN 16384

typedef struct Game {
    int startMs, startKind, playFade, dbgSel, dbgCar, gameSpeed;
    bool legacy;

    void* midlet;
    int W, H, u, rs;
    int* scr;
    bool nativeOutput; /* Retro-Go adapter opts into exact native composition. */
    int* trail;
    R3D r;
    World* world;
    int state, prevState;
    int splashMs, fadeMs;
    bool running, paused, needResize;

    int down, edges;
    int loadI, loadMs, loadErr;
    char loadMsg[128];
    int total;
    bool uiReady;
    int sel, menuSel;
    int optSel;
    int oBright, oDraw, oRadar, oRes, oDensity;
    bool oLimiter, oTrails, oSubs, oWide, oLegend, oHud, oRadio;
    int fps, fpsCount, fpsMs;
    int blink;
    int titleMs;
    bool started;
    int splashKeys;
    int videoKeys;
    bool introDone;
    int anyKey;
    int sprintPress;
    bool hasSave, saveChecked;
    bool pedDataOk;
    int* blm;
    int* blm2;
    int* tbX;
    int* tbY;
    int tbS, touchAct;
} Game;

Game* Game_new(void);
void Game_logOptions(const Game* g);
void  Game_free(Game* g);
void  Game_start(Game* g);
void  Game_stop(Game* g);
void  Game_resize(Game* g);
void  Game_refreshOptions(Game* g);
bool  Game_loadProgress(Game* g, const char* path);
void  Game_frame(Game* g, int dt, bool draw_frame);
void  Game_press(Game* g, int a);
void  Game_keyPressed(Game* g, int k);
void  Game_keyReleased(Game* g, int k);
void  Game_pointerPressed(Game* g, int x, int y);
void  Game_pointerReleased(Game* g, int x, int y);
int   Game_getWidth(Game* g);
int   Game_getHeight(Game* g);

extern Game* GTA_game;

#endif /* GTAVC_GAME_H */
