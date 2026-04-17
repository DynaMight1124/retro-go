/***************************************************************************
    Track Loading Code.

    Abstracts the level format, so that the original ROMs as well as
    in conjunction with track data from the LayOut editor.

    - Handles levels (path, width, height, scenery)
    - Handles additional level sections (road split, end section)
    - Handles road/level related palettes
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "globals.h"

// Road Generator Palette Representation
typedef struct
{
    uint32_t stripe_centre;   // Centre Stripe Colour
    uint32_t stripe;          // Stripe Colour
    uint32_t side;            // Side Colour
    uint32_t road;            // Main Road Colour
} RoadPalette;

// OutRun Level Representation
typedef struct 
{
    uint8_t* path;            // CPU 1 Path Data
    uint8_t* curve;           // Track Curve Information (Derived From Path)
    uint8_t* width_height;    // Track Width & Height Lookups
    uint8_t* scenery;         // Track Scenery Lookups

    uint16_t pal_sky;         // Index into Sky Palettes
    uint16_t pal_gnd;         // Index into Ground Palettes

    RoadPalette palr1;        // Road 1 Generator Palette
    RoadPalette palr2;        // Road 2 Generator Palette
} Level;

// LayOut Binary Header Format
#define LAYOUT_EXPECTED_VERSION 1

#define LAYOUT_HEADER      0
#define LAYOUT_PATH        (LAYOUT_HEADER      + sizeof(uint32_t) + sizeof(uint8_t))
#define LAYOUT_LEVELS      (LAYOUT_PATH        + sizeof(uint32_t))
#define LAYOUT_END_PATH    (LAYOUT_LEVELS      + (STAGES * sizeof(uint32_t)))
#define LAYOUT_END_LEVELS  (LAYOUT_END_PATH    + sizeof(uint32_t))
#define LAYOUT_SPLIT_PATH  (LAYOUT_END_LEVELS  + (5 * sizeof(uint32_t)))
#define LAYOUT_SPLIT_LEVEL (LAYOUT_SPLIT_PATH  + sizeof(uint32_t))
#define LAYOUT_PAL_SKY     (LAYOUT_SPLIT_LEVEL + sizeof(uint32_t))
#define LAYOUT_PAL_GND     (LAYOUT_PAL_SKY     + sizeof(uint32_t))
#define LAYOUT_SPRITE_MAPS (LAYOUT_PAL_GND     + sizeof(uint32_t))
#define LAYOUT_HEIGHT_MAPS (LAYOUT_SPRITE_MAPS + sizeof(uint32_t))



extern uint8_t* TrackLoader_stage_data;

extern Level* TrackLoader_levels;
extern Level* TrackLoader_levels_end;

extern Level* TrackLoader_current_level;

const static int MODE_ORIGINAL = 0;
const static int MODE_LAYOUT   = 1;

// Display start line on Stage 1
extern uint8_t TrackLoader_display_start_line;

extern uint32_t TrackLoader_curve_offset;
extern uint32_t TrackLoader_wh_offset;
extern uint32_t TrackLoader_scenery_offset;

// Shared Structures
extern uint8_t* TrackLoader_pal_sky_data;
extern uint8_t* TrackLoader_pal_gnd_data;
extern uint8_t* TrackLoader_heightmap_data;
extern uint8_t* TrackLoader_scenerymap_data;

extern uint32_t TrackLoader_pal_sky_offset;
extern uint32_t TrackLoader_pal_gnd_offset;
extern uint32_t TrackLoader_heightmap_offset;
extern uint32_t TrackLoader_scenerymap_offset;


void TrackLoader_Create();
void TrackLoader_Destroy();

void TrackLoader_init(uint8_t jap);
uint8_t TrackLoader_set_layout_track(const char* filename);
void TrackLoader_init_original_tracks(uint8_t jap);
void TrackLoader_init_layout_tracks(uint8_t jap);
void TrackLoader_init_track(const uint32_t);
void TrackLoader_init_track_split();
void TrackLoader_init_track_bonus(const uint32_t);

void TrackLoader_init_path(const uint32_t);
void TrackLoader_init_path_split();
void TrackLoader_init_path_end();

uint32_t TrackLoader_read_pal_sky_table(uint16_t entry);
uint32_t TrackLoader_read_pal_gnd_table(uint16_t entry);    
uint32_t TrackLoader_read_heightmap_table(uint16_t entry);
uint32_t TrackLoader_read_scenerymap_table(uint16_t entry);

int16_t TrackLoader_readPath(uint32_t addr);
int16_t TrackLoader_readPathIncP(uint32_t* addr);
int16_t TrackLoader_read_width_height(uint32_t* addr);
int16_t TrackLoader_read_curve(uint32_t addr);
uint16_t TrackLoader_read_scenery_pos();
uint8_t TrackLoader_read_total_sprites();
uint8_t TrackLoader_read_sprite_pattern_index();

int8_t TrackLoader_stage_offset_to_level(uint32_t);
Level* TrackLoader_get_level(uint32_t);

int32_t TrackLoader_read32IncP(uint8_t* data, uint32_t* addr);
int16_t TrackLoader_read16IncP(uint8_t* data, uint32_t* addr);
int8_t TrackLoader_read8IncP(uint8_t* data, uint32_t* addr);
int32_t TrackLoader_read32(uint8_t* data, uint32_t addr);
int16_t TrackLoader_read16(uint8_t* data, uint32_t addr);
int8_t TrackLoader_read8(uint8_t* data, uint32_t addr);


