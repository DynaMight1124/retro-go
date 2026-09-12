/***************************************************************************
    Road Rendering & Control

    This is a complete port of the 68000 SUB CPU Program ROM.
    
    The original code consists of a shared Sega library and some routines
    which are OutRun specific.
    
    Some of the original code is not used and is therefore not ported.
    
    This is the most complex area of the game code, and an area of the code
    in need of refactoring.

    Useful background reading on road rendering:
    http://www.extentofthejam.com/pseudo/

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/


#pragma once
#include <stdint.h>


extern uint32_t ORoad_road_pos;        // 0x6: Current Road Position (addressed as long and word)
extern int16_t ORoad_tilemap_h_target; // 0xA: Tilemap H target

// Stage Lookup Offset - Used to retrieve various data from in-game tables
//
// Stage Map:
//
// 24  23  22  21  20 
//   1B  1A  19  18
//     12  11  10
//       09  08 
//         00 
//
// Increments by 8 each stage.
// Also increments by +1 during the road split section from the values shown above.
//
// 0x38: Set to -8 during bonus mode section
extern int16_t ORoad_stage_lookup_off;

// These pointers rotate and select the current chunk of road data to blit
extern uint16_t ORoad_road_p0; // 0x3A: Road Pointer 0
extern uint16_t ORoad_road_p1; // 0x3C: Road Pointer 1 (Working road data)
extern uint16_t ORoad_road_p2; // 0x3E: Road Pointer 2 (Chunk of road to be blitted)
extern uint16_t ORoad_road_p3; // Ox40: Road Pointer 3 (Horizon Y Position)

// 0x4C: Road Width Backup
extern int16_t ORoad_road_width_bak;

// 0x4E: Car X Backup
extern int16_t ORoad_car_x_bak;

// 0x66: Road Height Lookup 
extern uint16_t ORoad_height_lookup;

// 0x722 - [word] Road Height Index. Working copy of 60066.
extern uint16_t ORoad_height_lookup_wrk;

// 0x6C: Change in road position
extern int32_t ORoad_road_pos_change; 
	
// 0x5E: Instruct CPU 1 to load end section road. Set Bit 1.
extern uint8_t ORoad_road_load_end;

// 0x306: Road Control
extern uint8_t ORoad_road_ctrl;
enum 
{
	ROAD_OFF = 0,         // Both Roads Off
	ROAD_R0 = 1,          // Road 0
	ROAD_R1 = 2,          // Road 1
	ROAD_BOTH_P0 = 3,     // Both Roads (Road 0 Priority) [DEFAULT]
	ROAD_BOTH_P1 = 4,     // Both Roads (Road 1 Priority) 
	ROAD_BOTH_P0_INV = 5, // Both Roads (Road 0 Priority) (Road Split. Invert Road 1)
	ROAD_BOTH_P1_INV = 6, // Both Roads (Road 1 Priority) (Road Split. Invert Road 1)
	ROAD_R0_SPLIT = 7,    // Road 0 (Road Split.)
	ROAD_R1_SPLIT = 8,    // Road 1 (Road Split. Invert Road 1)
};

// 0x30B: Road Load Split.
// This should be set to tell CPU 1 to init the road splitting code
// 0    = Do Not Load
// 0xFF = Load
extern int8_t ORoad_road_load_split;

// 0x314: Road Width
// There are two road generators.
//
// The second road is drawn at an offset from the first, so that it either appears as one solid road, or as two separate roads.
//
// 00 = 3 lanes
// D4 = 6 lanes
//
// Once the distance is greater than F0 or so, it's obvious there are two independent roads.
extern int32_t ORoad_road_width;// DANGER! USED AS LONG AND WORD

// 0x420: Offset Into Road Data [Current Road Position * 4]
// Moved from private for tracked
extern uint16_t ORoad_road_data_offset;

// 0x4F0: Start Address of Road Data For Current Stage In ROM
// TODO - move back to being private at some stage
//uint32_t stage_addr;

// 0x510: Horizon Y Position
extern int16_t ORoad_horizon_y2;
extern int16_t ORoad_horizon_y_bak;

// 0x53C: Granular Position. More fine than other positioning info. Used to choose road background colour.
extern uint16_t ORoad_pos_fine;

// 0x732 - [long] Base Horizon Y-Offset. Adjusting this almost has the effect of raising the camera. 
// Stage 1 is 0x240
// Higher values = higher horizon.
// Note: This is adjusted mid-stage for Stage 2, but remains constant for Stage 1.
extern int32_t ORoad_horizon_base;

// 0x736: 0 = Base Horizon Value Not Set. 1 = Value Set.
extern uint8_t ORoad_horizon_set;

#define ROAD_ARRAY_LENGTH 0x200

// 60800 - 60BFF: Road X-Positions [Before H-Scroll Is Applied] - Same Data For Both Roads
extern int16_t ORoad_road_x[ROAD_ARRAY_LENGTH];

// 60C00 - 60FFF: Road 0 H-Scroll Adjusted Positions
extern int16_t ORoad_road0_h[ROAD_ARRAY_LENGTH];
	
// 61000 - 613FF: Road 1 H-Scroll Adjusted Positions
extern int16_t ORoad_road1_h[ROAD_ARRAY_LENGTH];

// 61400 - 617FF: Not sure what this is yet
extern int16_t ORoad_road_unk[ROAD_ARRAY_LENGTH];

// 61800 - 637FF: Road Y-Positions	
//
// Consists of three separate blocks of data:
//
// Offset 0x000: Source Data. List of sequential numbers to indicate when to read next road value from source rom.
//               Numbers iterate down sequentially, until a hill, when they will rise again
// Offset 0x280: Priority information for road (during elevated sections). Used to hide sprites
// Offset 0x400: Destination Data. Final converted data to be output to road hardware
//
// This format is repeated four times, due to the way values rotate through road ram
extern int16_t *ORoad_road_y;

const static uint8_t ROAD_VIEW_ORIGINAL = 0;
const static uint8_t ROAD_VIEW_ELEVATED = 1;
const static uint8_t ROAD_VIEW_INCAR    = 2;

void ORoad_init();
void ORoad_tick();
uint8_t ORoad_get_view_mode();
int16_t ORoad_get_road_y(uint16_t);
void ORoad_set_view_mode(uint8_t, uint8_t);

