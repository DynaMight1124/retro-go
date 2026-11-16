#ifndef GTAVC_ASSETS_H
#define GTAVC_ASSETS_H

#include <stdint.h>
#include <stdbool.h>

struct R3D;

#define ASSETS_MAX_SLOTS 4096

extern int Assets_SLOTS;
extern char* Assets_slotName[ASSETS_MAX_SLOTS];
extern int Assets_slotRef[ASSETS_MAX_SLOTS];
extern int Assets_slotUse[ASSETS_MAX_SLOTS];
extern bool Assets_legacy;
extern int Assets_useClock;
extern struct R3D* Assets_r;

void Assets_init(struct R3D* r);
int* Assets_decode(const char* path, int dim[2]);
int  Assets_find(const char* path);
int  Assets_acquire(const char* path, int* pix, int w, int h);
int  Assets_acquirePath(const char* path);
void Assets_release(int s);

int* Assets_imgPix(const char* res, int dim[2]);
int* Assets_intensity(const char* res, int dim[2]);
uint8_t* Assets_readAll(const char* res, int* outLen);
char** Assets_readLines(const char* res, int* outCount);
void   Assets_freeLines(char** lines, int count);
void   Assets_cleanup(void);

#endif /* GTAVC_ASSETS_H */
