#ifndef PICO8_API_H
#define PICO8_API_H
#include "data.h"
#include "pico8_globals.h"  // brings in frontbuffer[], palette[], pal_map[], bootup_time as extern
static uint32_t cartdata[64];
static Spritesheet spritesheet;
static Spritesheet fontsheet;
static uint8_t map_data[64 * 128];
// bootup_time, frontbuffer[], palette[], pal_map[] live in pico8_globals.c
// (declared extern in pico8_globals.h, included above).
const static uint8_t orig_pal_map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
#endif
