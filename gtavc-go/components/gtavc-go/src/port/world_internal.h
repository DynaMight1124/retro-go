#ifndef GTAVC_WORLD_INTERNAL_H
#define GTAVC_WORLD_INTERNAL_H

#include "world.h"

/* Map / grid helpers (world.c) */
int  World_idx(World* w, int i, int j);
Tile* World_tileFor(World* w, int gi, int gj, int per);
int  World_hCell(World* w, int gi, int gj, int y);
int  World_groundH2(World* w, int xm, int zm);
bool World_walkable(World* w, int xm, int zm, int y);
bool World_deep(World* w, int xm, int zm, int y);
bool World_stepOk(World* w, int ox, int oz, int oy, int maxStep);
void World_buildFxMeshes(World* w);

/* Entity helpers (world_entities.c) */
Car* World_addCar(World* w, int mode, int slot, int xm, int zm, int yaw, int colorIdx);
void World_removeCar(World* w, int i);
void World_addDriver(World* w, Car* c, bool cop);
void World_addPickup(World* w, int type, int fwd, int side);
void World_dropPickup(World* w, int type, int amt, int x, int z, int y);
void World_spawnTraffic(World* w, bool initial);
void World_spawnPed(World* w, bool initial);
Ped* World_spawnCopAt(World* w, int x, int z);
void World_spawnCop(World* w);
void World_spawnPoliceCar(World* w);
int  World_copsNear(World* w, int rangeMm);
void World_raiseWanted(World* w, int lvl);
void World_startClip(Ped* p, int clip, int next, int endMs);
bool World_stepClip(World* w, Ped* p, int dt);
void World_selectLoco(World* w, Ped* p, int dt, int speed);
int  World_localX(Car* c, int lx, int lz);
int  World_localZ(Car* c, int lx, int lz);
int  World_seatLX(World* w, Car* c);
int  World_seatLZ(World* w, Car* c);
void World_assignKind(World* w, Ped* p, int slot);
void World_initClips(World* w);
void World_respawn(World* w);
void World_arrestRespawn(World* w);

/* Pools (world_pools.c) */
void World_pools_loadCar(World* w, int slot, int src);
void World_pools_rotatePools(World* w, int dt);
int  World_pools_findStr(char** a, int count, const char* s);

/* Gameplay (world_gameplay.c) */
void World_hurt(World* w, int dmg);
void World_moveCar(World* w, Car* c, int dt);
void World_driveCar(World* w, Car* c, int dt, int acc, int st, bool hand);
void World_explode(World* w, Car* c);
void World_spawnFx(World* w, int type, int x, int y, int z);
void World_updateParticles(World* w, int dt);
int  World_regionOf(Car* c, int x, int z);

/* Combat / interaction (world_combat.c) */
void World_lockOn(World* w);
void World_voice(World* w, int kind, Ped* p);
void World_shoot(World* w);
void World_doPunch(World* w, int dmgBase, int reach);
void World_handleEnter(World* w);
void World_updateAct(World* w, int dt);
void World_updateOnFoot(World* w, int dt, int keys, bool fire);
void World_updateDriving(World* w, int dt, int keys);
void World_updatePed(World* w, Ped* p, int dt);
void World_trafficAI(World* w, Car* c, int dt);
void World_policeAI(World* w, Car* c, int dt);
void World_updateCamera(World* w, int dt, int keys);
void World_updateWorld(World* w, int dt);
void World_cycleWeapon(World* w);
void World_updatePickups(World* w, int dt);
void World_updateWanted(World* w, int dt);

/* Rendering (world_render.c) */
void World_loadTimecyc(World* w);
void World_lighting(World* w);
void World_drawPickups(World* w);
Mesh* World_cashMesh(World* w);
void World_drawBlood(World* w, Ped* p);
void World_drawBeam(World* w);
void World_drawCar(World* w, Car* c, bool glassPass);
void World_drawPed(World* w, Ped* p);
void World_drawHeld(World* w, PedModel* m, Mesh* mesh);

#endif /* GTAVC_WORLD_INTERNAL_H */
