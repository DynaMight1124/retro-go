#pragma once

#include <stdint.h>
#include "romloader.h"

enum
{
    HWTILES_LEFT,
    HWTILES_RIGHT,
    HWTILES_CENTRE,
};

extern uint8_t HWTiles_text_ram[0x1000]; // Text RAM
extern uint8_t HWTiles_tile_ram[0x10000]; // Tile RAM

void HWTiles_Create(void);
void HWTiles_Destroy(void);

void HWTiles_init(uint8_t* src_tiles, const uint8_t hires);
void HWTiles_patch_tiles(RomLoader* patch);
void HWTiles_restore_tiles();
void HWTiles_set_x_clamp(const uint16_t);
void HWTiles_update_tile_values();
void HWTiles_render_tile_layer(uint16_t*, uint8_t, uint8_t);
void HWTiles_render_text_layer(uint16_t*, uint8_t);
void HWTiles_render_all_tiles(uint16_t*);
