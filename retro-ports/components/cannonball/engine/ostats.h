/***************************************************************************
    In-Game Statistics.
    - Stage Timers
    - Route Info
    - Speed to Score Conversion
    - Bonus Time Increment
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"

// Current stage
// 0    = Stage 1
// 1    = Stage 2
// 2    = Stage 3
// 3    = Stage 4
// 4    = Stage 5
// -1   = Bonus Points Section
//                  
// A good way to quickly see the end sequence is to set this to '4' and play 
// through the first level.
extern int8_t OStats_cur_stage;

// Score (Outputs Hex values directly)
extern uint32_t OStats_score;

// Store info on the route taken by the player
//
// +10 For each stage. 
//
// Then increment by the following when Left Hand route selected at each stage.
//
// Stage 1 = +8 (1 << 3 - 0)
// Stage 2 = +4 (1 << 3 - 1)
// Stage 3 = +2 (1 << 3 - 2)
// Stage 4 = +1 (1 << 3 - 3)
// Stage 5 = Road doesn't split on this stage
//
// So if we reach Stage 2 (left route) we do 10 + 8 = 18
extern uint16_t OStats_route_info;

// Stores route_info for each stage. Used by course map screen
// First entry stores upcoming stage number
extern uint16_t OStats_routes[0x8]; 

// Frame Counter Reset/Load Value.
// Load frame counter with this value when the counter has decremented and expired.
// Note: Values stored and used in hex.
extern int16_t OStats_frame_counter;
const static int16_t OStats_frame_reset = 30; 

// Time Counter (Frames). Counts downwards from 30.
// Used in correspondence with 0x60860.
// Note: Values stored and used in hex.
extern int16_t OStats_time_counter;

// Extend Play Timer.
//
// Loaded to 0x80 when EXTEND PLAY! banner should flash
extern int16_t OStats_extend_play_timer;

// Time data array
static const uint8_t STATS_TIME[] =
{
    // Easy
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x57, 0x62, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x66, 0x63, 0x61, 0x65, 0x00, 0x00, 0x00, 0x00, 
    0x58, 0x55, 0x56, 0x58, 0x56, 0x00, 0x00, 0x00,

    // Normal 
    0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x55, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x60, 0x65, 0x00, 0x00, 0x00, 0x00,
    0x56, 0x56, 0x56, 0x56, 0x56, 0x00, 0x00, 0x00, 

    // Hard
    0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x57, 0x60, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x64, 0x60, 0x58, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x54, 0x54, 0x54, 0x54, 0x56, 0x00, 0x00, 0x00, 

    // Hardest
    0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x57, 0x60, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x62, 0x60, 0x58, 0x63, 0x00, 0x00, 0x00, 0x00, 
    0x54, 0x54, 0x54, 0x54, 0x56, 0x00, 0x00, 0x00,
};

// Counters that increment with each game tick.
// Each stage has an independent counter (increased to 15 from 5 to support continuous mode)
extern int16_t OStats_stage_counters[15];

// Set when game completed
extern uint8_t OStats_game_completed;

extern const uint8_t* OStats_lap_ms;

// Number of credits inserted
extern uint8_t OStats_credits;

// Each stage has an entry for minutes, seconds and MS. (Extended to 15 from 5 to support continuous mode)
extern uint8_t OStats_stage_times[15][3];


void OStats_init(uint8_t);

void OStats_clear_stage_times();
void OStats_clear_route_info();

void OStats_do_mini_map();
void OStats_do_timers();
void OStats_convert_speed_score(uint16_t);
void OStats_update_score(uint32_t);
void OStats_init_next_level();

