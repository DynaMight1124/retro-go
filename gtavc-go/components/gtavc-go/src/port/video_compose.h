#ifndef GTAVC_VIDEO_COMPOSE_H
#define GTAVC_VIDEO_COMPOSE_H
#include <stdint.h>
#include <stdbool.h>
/* Overlay is always 320x240; world uses legacy nearest-neighbour coordinates.
   Buffers and dimensions remain stable until composition/materialization ends. */
void Video_beginOverlay(int *overlay,const int *world,int world_width,int world_height);
void Video_pauseTracking(void);
void Video_clearOverlay(void);
void Video_materialize(void);
void Video_prepareRect(int *buffer,int width,int height,int x,int y,int w,int h);
bool Video_hasOverlay(void);
unsigned Video_dirtyBlocks(void);
void Video_compose565(uint16_t *dest,int stride_pixels,bool big_endian);
#endif
