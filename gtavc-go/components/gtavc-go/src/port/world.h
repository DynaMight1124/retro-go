#ifndef GTAVC_WORLD_H
#define GTAVC_WORLD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "r3d.h"
#include "tile.h"
#include "car.h"
#include "ped.h"
#include "car_model.h"
#include "ped_model.h"

#define WORLD_S_WATER 0
#define WORLD_S_SAND  1
#define WORLD_S_GRASS 2
#define WORLD_S_PAVE  3
#define WORLD_S_ROAD  4
#define WORLD_S_ROCK  5

#define WORLD_K_UP     1
#define WORLD_K_DOWN   2
#define WORLD_K_LEFT   4
#define WORLD_K_RIGHT  8
#define WORLD_K_FIRE   16
#define WORLD_K_CAML   32
#define WORLD_K_CAMR   64
#define WORLD_K_CAMU   128
#define WORLD_K_CAMD   256
#define WORLD_K_SPRINT 512
#define WORLD_K_HAND   1024

#define WORLD_PA_NONE   0
#define WORLD_PA_WALKTO 1
#define WORLD_PA_GETIN  2
#define WORLD_PA_GETOUT 3

#define WORLD_NPOOL   12
#define WORLD_NCPOOL  10
#define WORLD_MAXPED  22
#define WORLD_MAXTRAFFIC 8
#define WORLD_NPK     24
#define WORLD_NFX     72

#define WORLD_MISSION_AVAILABLE 0
#define WORLD_MISSION_ACTIVE    1
#define WORLD_MISSION_PASSED    2

typedef struct World {
    R3D* r;

    /* map streaming */
    int mox, moz, tileMm, tnx, tnz;
    uint8_t* tflag;
    Tile** tiles;
    int rx0, rz0, rcell, rw, rh;
    uint8_t* rad;
    int spX, spZ, spY, spYaw;
    int lampX, lampY, lampZ;
    Mesh *beam, *blood;

    int reqQ[64];
    int reqN;
    Tile* readyQ[16];
    int readyN;
    bool* requested;
    bool alive;

    /* cooperative cache read (one tile at a time, spread over frames) */
    bool tileJobActive;
    bool tileWorkerEnabled;
    int tileJobI, tileJobJ, tileJobK, tileJobLen, tileJobGot;
    void* tileJobFile;
    uint8_t* tileJobBuf;
    struct TileCacheStream *tileJobDecoder;
    int tileJobBuffered, tileJobCapacity;
    int64_t tileJobStartedUs;
    Tile *tileJobTile; /* decoded, private until installation completes */
    bool tileTexturePending, tileTextureFallback;
    uint32_t tileInstallSlices, tileInstallUrgent, tileInstallPeakUs;
    uint64_t tileInstallWorkUs;
    /* Bounded retry metadata: no per-frame allocation, keyed by map index. */
    struct { int k; int64_t until_us; } tileRetry[32];

    /* pools */
    char** pedNames;
    int* pedFlags;
    int pedNamesCount;
    PedModel* pm0;
    int playerTex;
    PedModel* pool[WORLD_NPOOL];
    PedModel* poolLo[WORLD_NPOOL];
    int poolTex[WORLD_NPOOL];
    int poolSrc[WORLD_NPOOL];
    int copSlot;

    char** carNames;
    int carNamesCount;
    CarModel* cpool[WORLD_NCPOOL];
    int cpoolSrc[WORLD_NCPOOL];
    int policeSlot;
    Mesh* weaponMesh[3];
    char** hdl;
    int hdlCount;
    int poolT;

    /* runtime pool sizes (the original LEGACY configuration on S40 phones) */
    int npool, ncpool, maxPed, maxTraffic, ring, nearLod;

    /* animation clips */
    uint8_t tc[360];
    int cIdle, cWalk, cWalkB, cRun, cCiviWalk, cCiviRun, cWomanWalk, cWomanRun, cWomanIdle;
    int cPunch, cJab, cHeavy, cHitF, cHitB, cKoF, cKoB, cGetup;
    int cOpen, cGetin, cGetout, cPull, cJacked, cSit, cSprint;
    int cJumpL, cJumpG, cJumpLand;

    /* entities */
    Ped playerInst;
    Ped* player;
    Car* inCar;
    Car* pActCar;
    int pAct, combo;
    Car cars[16];
    int ncars;
    Ped peds[40];
    int nped;

    /* player stats */
    int money, health, armor, wanted, pedsHit, carsStolen, kills;
    /* first story mission "An Old Friend" */
    int missionState, missionTimer, missionDelay;
    int missionX, missionY, missionZ;
    char missionText[96];
    bool missionDone;
    Mesh* missionMarker;
    bool radioEnabled, radioInCar;
    int quietKills, quietLimit;
    int wantedCalmT;
    int minutes, clockMs;
    int camYawOff, camYaw, camPitch, camIdle, camXm, camYm, camZm;
    int camDistMm;
    char help[256];
    int helpT, helpShown;
    int wastedT, dayCount, bustedT, drownT;
    bool busted;
    int drawDist;
    int brightness;
    int time;
    int rx, rz;
    int shake;
    bool runMode, god, jumpReq;
    int speedMul, sprintHeld, voiceCd;
    Ped* target;
    int weapon;
    bool hasBat, hasPistol;
    int ammo, shotCd, aimT, flashT;
    int npick, saveT;

    int pkX[WORLD_NPK], pkZ[WORLD_NPK], pkY[WORLD_NPK], pkType[WORLD_NPK];
    int pkT[WORLD_NPK], pkAmt[WORLD_NPK], pkLife[WORLD_NPK];
    Mesh* cashMesh;

    int spot[2];
    int dbgHit;

    /* particles */
    int fxX[WORLD_NFX], fxY[WORLD_NFX], fxZ[WORLD_NFX], fxT[WORLD_NFX], fxL[WORLD_NFX], fxM[WORLD_NFX];

    bool night;
    int lv[15];

    /* scratch */
    int pose[93], pose2[93];
    int rYaw[9], mmA[9], wA[9], wB[9], wC[9], sc[9], handW[3];
} World;

World* World_new(R3D* r);
void   World_loadIndex(World* w);
void   World_preload(World* w, int x, int z);
void   World_startLoader(World* w);
void   World_streamTiles(World* w, int x, int z);
void   World_loadPools(World* w);
void   World_loadTimecyc(World* w);
void   World_reset(World* w);
void   World_tileJobCancel(World* w);
void   World_update(World* w, int dt, int keys, bool fire, bool enter, bool toggleRun, bool nextWeapon, bool jump);
bool   World_isBridgeAt(World* w, int xm, int zm);
bool   World_spawnClear(World* w, int xm, int zm, int radius);
bool   World_stepAllowed(World* w, int xm, int zm, int y, int maxStep);
void   World_missionReset(World* w);
void   World_missionUpdate(World* w, int dt);
void   World_drawMissionMarker(World* w);
void   World_render(World* w);
void   World_continueAt(World* w, int x, int z, int y, int yaw);
void   World_placePlayerAtSpawn(World* w);
void   World_clockStr(World* w, char* out, size_t outSize);

int    World_groundH(World* w, int xm, int zm, int y);
int    World_surfAt(World* w, int xm, int zm);
bool   World_solidCell(World* w, int gi, int gj);
int    World_scell(int m, int o);
bool   World_resolve(World* w, int rad);
bool   World_findSpot(World* w, int d0, int d1, int want);
int    World_roadYaw(World* w, int x, int z);
void   World_damagePed(World* w, Ped* o, int dmg, bool byPlayer, int ax, int az);
void   World_damageCar(World* w, Car* c, int region, int amount);
void   World_debugSpawn(World* w, int src);
void   World_clearCops(World* w);
void   World_say(World* w, const char* s);
CarModel* World_cm(World* w, Car* c);
int    World_vmaxOf(World* w, Car* c);
void   World_free(World* w);

#endif /* GTAVC_WORLD_H */
