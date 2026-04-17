/***************************************************************************
    Tilemap Handling Code. 

    Logic for the foreground and background tilemap layers.

    - Read and render tilemaps
    - H-Scroll & V-Scroll
    - Palette Initialization
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.h"

// + 0x21: Tilemap Control
// 0 = Clear Tile Table 1 & Init Default Tilemap (Stage 1)
// 1 = Scroll Tilemap
// 2 = Init Tilemap
// 3 = New Tilemap Initialized - Scroll both tilemaps during tilesplit
extern uint8_t OTiles_tilemap_ctrl;
enum { TILEMAP_CLEAR, TILEMAP_SCROLL, TILEMAP_INIT, TILEMAP_SPLIT };

void OTiles_init();
void OTiles_set_vertical_swap();
void OTiles_setup_palette_tilemap();
void OTiles_setup_palette_widescreen();
void OTiles_setup_palette_hud();
void OTiles_reset_tiles_pal();
void OTiles_update_tilemaps(int8_t);
void OTiles_init_tilemap_palette(uint16_t);
void OTiles_fill_tilemap_color(uint16_t);
void OTiles_write_tilemap_hw();
void OTiles_set_scroll(int16_t h_scroll, int16_t v_scroll);


