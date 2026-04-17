/***************************************************************************
    Core Game Engine Routines.
    
    - The main loop which advances the level onto the next segment.
    - Code to directly control the road hardware. For example, the road
      split and bonus points routines.
    - Code to determine whether to initialize certain game modes
      (Crash state, Bonus points, road split state) 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"


// Debug: Camera X Offset
extern int16_t OInitEngine_camera_x_off;

// Is the in-game engine active?
extern uint8_t OInitEngine_ingame_engine;

// Time to wait before enabling ingame_engine after crash
extern int16_t OInitEngine_ingame_counter;

// Road Split State
// 0 = No Road Split
// 1 = Init Road Split
// 2 = Road Split
// 3 = Beginning of split. User must choose.
// 4 = Road physically splits into two individual roads
// 5 = Road fully split. Init remove other road
// 6 = Road split. Only one road drawn.
// 7 = Unknown
// 8 = Init Road Merge before checkpoint sign
// 9 = Road Merge before checkpoint sign
// A = Unknown
// B = Checkpoint sign 
// C = Unused
// D = Unused
// E = Unused
// F = Unused
// 10 = Init Bonus Points Sequence
// 11 = Bonus Points Sequence
extern uint16_t OInitEngine_rd_split_state;
enum {INITENGINE_SPLIT_NONE, INITENGINE_SPLIT_INIT, INITENGINE_SPLIT_CHOICE1, INITENGINE_SPLIT_CHOICE2};

// Upcoming Road Type:
// 0 = No change
// 1 = Straight road
// 2 = Right Bend
// 3 = Left Bend
extern int16_t OInitEngine_road_type;
extern int16_t OInitEngine_road_type_next;
enum {INITENGINE_ROAD_NOCHANGE, INITENGINE_ROAD_STRAIGHT, INITENGINE_ROAD_RIGHT, INITENGINE_ROAD_LEFT};

// End Of Stage Properties
//
// Bit 0: Fade Sky & Ground Palette To New Entry
// Bit 1: Use Current Palette (Don't Bump To Next One)
// Bit 2: Use Current Sky Palette For Fade (Don't Bump To Next One)
// Bit 3: Loop back to stage 1
extern uint8_t OInitEngine_end_stage_props;

extern uint32_t OInitEngine_car_increment; // NEEDS REPLACING. Implementing here as a quick hack so routine works

// Car X Position              
// 0000 = Centre of two road generators
//
// Turning Right REDUCES value, Turning Left INCREASES value
//    
// 0xxx [pos] = Road Generator 1 Position (0 - xxx from centre)
// Fxxx [neg] = Road Generator 2 Position (0 + xxx from centre)
extern int16_t OInitEngine_car_x_pos;
extern int16_t OInitEngine_car_x_old;

// Checkpoint Marker

// 0  = Checkpoint Not Past
// -1 = Checkpoint Past
extern int8_t OInitEngine_checkpoint_marker;

// Something to do with the increment / curve of the road
extern int16_t OInitEngine_road_curve;
extern int16_t OInitEngine_road_curve_next;

// Road split logic handling to remove split
// 0 = Normal Road Rendering
// 1 = Road Has Split, Old Road now removed
extern int8_t OInitEngine_road_remove_split;

// Route Selected
// -1 = Left
// 0  = Right
// But confusingly, these values get swapped by a not instruction
extern int8_t OInitEngine_route_selected;

// Road Width Change 
// 0 = No
// -1 = In Progress
extern int16_t OInitEngine_change_width;

void OInitEngine_init(int8_t debug_level);

void OInitEngine_init_road_seg_master();
void OInitEngine_init_crash_bonus();
void OInitEngine_update_road();
void OInitEngine_update_engine();
void OInitEngine_update_shadow_offset();
void OInitEngine_set_granular_position();
void OInitEngine_set_fine_position();

void OInitEngine_init_bonus(int16_t); // moved here for debugging purposes

