/***************************************************************************
    OutRun Engine Entry Point.

    This is the hub of the ported OutRun code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <stdint.h>
#include "roms.h"
#include "globals.h"

#include "video.h"

#include "frontend/config.h"

// Main include for Ported OutRun Code
#include "oaddresses.h"
#include "osprites.h"
#include "oroad.h"
#include "oinitengine.h"
#include "audio/osoundint.h"
#include "cannonboard/interface.h"

// Globals
enum 
{
	GS_INIT = 0,				// Initalize Game
	GS_ATTRACT = 1,				// Attract Mode
	GS_INIT_BEST1 = 2,			// Load Best Outrunners
	GS_BEST1 = 3,				// Best Outrunners (Attract Mode)
	GS_INIT_LOGO = 4,			// Load Outrun Logo
	GS_LOGO = 5,				// Outrun Logo (Attract Mode)
	GS_INIT_MUSIC = 6,			// Load Music Selection Screen
	GS_MUSIC = 7,				// Music Selection Screen
	GS_INIT_GAME = 8,			// Loading In-Game
	GS_START1 = 9,				// Start Game, Car Driving In
	GS_START2 = 10,				// Start Game, Countdown
	GS_START3 = 11,				// Start Game, Countdown 2
	GS_INGAME = 12,				// Start Game, User in control
	GS_INIT_BONUS = 13,			// Load Bonus Points
	GS_BONUS = 14,				// Display Bonus Points
	GS_INIT_GAMEOVER = 15,	    // Load Game Over
	GS_GAMEOVER = 16,           // Game Over Text
	GS_INIT_MAP = 17,			// Load Course Map
	GS_MAP = 18,				// Course Map
	GS_INIT_BEST2 = 19,			// Load Best Outrunners
	GS_BEST2 = 20,			    // Best Outrunners
	GS_REINIT = 21,				// Reinitalize Game (after outrunners screen)
    GS_CALIBRATE_MOTOR = 100,   // Calibrate Motors
};

typedef struct 
{
    uint8_t enabled;             // Time Trial Mode Enabled
    uint8_t  level;           // Time Trial Level
    uint8_t  traffic;         // Max Traffic Level
    uint8_t  laps;            // Total laps (maximum of 5 laps total allowed)
    uint8_t  current_lap;     // Which lap are we currently on
    uint16_t overtakes;       // Number of overtakes
    uint16_t vehicle_cols;    // Number of vehicle collisions
    uint16_t crashes;         // Number of crashes
    uint8_t  laptimes[5][3];  // Stored lap times
    int16_t  best_lap_counter;// Counter representing best laptime
    uint8_t  best_lap[3];     // Stored best lap time
    uint8_t new_high_score;      // Has player achieved a new high score?
} time_trial_t;

// Addresses (Used to swap between original and Japanese roms)
typedef struct 
{
    // CPU 0
    uint32_t tiles_def_lookup;
    uint32_t tiles_table;

    uint32_t sprite_master_table;
    uint32_t sprite_type_table;
    uint32_t sprite_def_props1;
    uint32_t sprite_def_props2;
    uint32_t sprite_cloud;
    uint32_t sprite_minitree;
    uint32_t sprite_grass;
    uint32_t sprite_sand;
    uint32_t sprite_stone;
    uint32_t sprite_water;
    uint32_t sprite_ferrari_frames;
    uint32_t sprite_skid_frames;
    uint32_t sprite_pass_frames;
    uint32_t sprite_pass1_skidl;
    uint32_t sprite_pass1_skidr;
    uint32_t sprite_pass2_skidl;
    uint32_t sprite_pass2_skidr;
    uint32_t sprite_crash_spin1;
    uint32_t sprite_crash_spin2;
    uint32_t sprite_bump_data1;
    uint32_t sprite_bump_data2;
    uint32_t sprite_crash_man1;
    uint32_t sprite_crash_girl1;
    uint32_t sprite_crash_flip;
    uint32_t sprite_crash_flip_m1;
    uint32_t sprite_crash_flip_g1;
    uint32_t sprite_crash_flip_m2;
    uint32_t sprite_crash_flip_g2;
    uint32_t sprite_crash_man2;
    uint32_t sprite_crash_girl2;
    uint32_t smoke_data;
    uint32_t spray_data;
    uint32_t shadow_data;
    uint32_t shadow_frames;
    uint32_t sprite_shadow_small;

    uint32_t sprite_logo_bg;
    uint32_t sprite_logo_car;
    uint32_t sprite_logo_bird1;
    uint32_t sprite_logo_bird2;
    uint32_t sprite_logo_base;
    uint32_t sprite_logo_text;
    uint32_t sprite_logo_palm1;
    uint32_t sprite_logo_palm2;
    uint32_t sprite_logo_palm3;

    uint32_t sprite_fm_left;
    uint32_t sprite_fm_centre;
    uint32_t sprite_fm_right;
    uint32_t sprite_dial_left;
    uint32_t sprite_dial_centre;
    uint32_t sprite_dial_right;
    uint32_t sprite_eq;
    uint32_t sprite_radio;
    uint32_t sprite_hand_left;
    uint32_t sprite_hand_centre;
    uint32_t sprite_hand_right;

    uint32_t sprite_coursemap_top;
    uint32_t sprite_coursemap_bot;
    uint32_t sprite_coursemap_end;
    uint32_t sprite_minicar_right;
    uint32_t sprite_minicar_up;
    uint32_t sprite_minicar_down;

    uint32_t anim_seq_flag;
    uint32_t anim_ferrari_curr;
    uint32_t anim_ferrari_next;
    uint32_t anim_pass1_curr;
    uint32_t anim_pass1_next;
    uint32_t anim_pass2_curr;
    uint32_t anim_pass2_next;

    uint32_t anim_ferrari_frames;
    uint32_t anim_endseq_obj1;
    uint32_t anim_endseq_obj2;
    uint32_t anim_endseq_obj3;
    uint32_t anim_endseq_obj4;
    uint32_t anim_endseq_obj5;
    uint32_t anim_endseq_obj6;
    uint32_t anim_endseq_obj7;
    uint32_t anim_endseq_obj8;
    uint32_t anim_endseq_objA;
    uint32_t anim_endseq_objB;
    uint32_t anim_end_table;

    uint32_t traffic_props;
    uint32_t traffic_data;
    uint32_t sprite_porsche;

    uint32_t sprite_coursemap;
    uint32_t road_seg_table;
    uint32_t road_seg_end;
    uint32_t road_seg_split;
    
    // CPU 1
    uint32_t road_height_lookup;
} adr_t;



extern uint8_t Outrun_freeze_timer;

// CannonBall Game Mode
extern uint8_t Outrun_cannonball_mode;

const static uint8_t OUTRUN_MODE_ORIGINAL = 0; // Original OutRun Mode
const static uint8_t OUTRUN_MODE_TTRIAL   = 1; // Enhanced Time Trial Mode
const static uint8_t OUTRUN_MODE_CONT     = 2; // Enhanced Continuous Mode

// Max traffic level for custom modes 
extern uint8_t Outrun_custom_traffic;

// Time trial data
extern time_trial_t Outrun_ttrial;

// Service Mode Toggle: Not implemented yet.
extern uint8_t Outrun_service_mode;

// Tick Logic. Used when running at non-standard > 30 fps
extern uint8_t Outrun_tick_frame;

// Tick Counter (always syncd to 30 fps to flash text and other stuff)
extern uint32_t Outrun_tick_counter;

// Main game state
extern int8_t Outrun_game_state;

// Address structures
extern adr_t Outrun_adr;


void Outrun_init();
void Outrun_boot();
void Outrun_tick(Packet* packet, uint8_t tick_frame);
void Outrun_vint();
void Outrun_init_best_outrunners();
void Outrun_select_course(uint8_t jap, uint8_t prototype);

