/***************************************************************************
    Ferrari Rendering & Handling Code.
       
    Much of the handling code is very messy. As such, the translated code 
    isn't great as I tried to focus on accuracy rather than refactoring.
    
    A good example of the randomness is a routine I've named
      do_sound_score_slip()
    which performs everything from updating the score, setting the audio
    engine tone, triggering smoke effects etc. in an interwoven fashion.
    
    The Ferrari sprite has different properties to other game objects
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"


// Ferrari Sprite Object
extern oentry *OFerrari_spr_ferrari;

// Passenger 1 Sprite Object
extern oentry *OFerrari_spr_pass1;

// Passenger 2 Sprite Object
extern oentry *OFerrari_spr_pass2;

// Ferrari Shadow Sprite Object
extern oentry *OFerrari_spr_shadow;

// -------------------------------------------------------------------------
// Main Switch Variables
// -------------------------------------------------------------------------

enum
{
    // Initialise Intro Animation Sequences
    FERRARI_SEQ1 = 0,

    // Tick Intro Animation Sequences
    FERRARI_SEQ2 = 1,

    // Initialize In-Game Logic
    FERRARI_INIT = 2,

    // Tick In-Game Logic
    FERRARI_LOGIC = 3,
        
    // Ferrari End Sequence Logic
    FERRARI_END_SEQ = 4,
};

// Which routine is in use
extern uint8_t OFerrari_state; 

// Unused counter. Implemented on original game so could be useful for debug.
extern uint16_t OFerrari_counter;

extern int16_t OFerrari_steering_old;
extern uint8_t OFerrari_car_ctrl_active;
    
// Car State
//
// -1 = Animation Sequence (Crash / Drive In)
// 0  = Normal
// 1  = Smoke from wheels
extern int8_t OFerrari_car_state;

enum { CAR_ANIM_SEQ = -1, CAR_NORMAL = 0, CAR_SMOKE = 1};

// Auto breaking for end sequence
extern uint8_t OFerrari_auto_brake;

// Torque table index lookup
//
// 00 = Start line only
// 10 = Low gear
// 1F = High gear
//
// Increments between the values
//
// Gets set based on what gear we're in 
extern uint8_t OFerrari_torque_index; 
extern int16_t OFerrari_torque;
extern int32_t OFerrari_revs;

// Rev Shift Value. Normal = 1.
// Higher values result in reaching higher revs faster!
extern uint8_t OFerrari_rev_shift;

// State of car wheels
//
// 0 = On Road
// 1 = Left Wheel Off-Road
// 2 = Right Wheel Off-Road
// 3 = Both Wheels Off-Road
extern uint8_t OFerrari_wheel_state;

enum
{
    WHEELS_ON = 0,
    WHEELS_LEFT_OFF = 1,
    WHEELS_RIGHT_OFF = 2,
    WHEELS_OFF = 3
};

// Wheel Traction
//
// 0 = Both Wheels Have Traction
// 1 = One Wheel Has Traction
// 2 = No Wheels Have Traction
extern uint8_t OFerrari_wheel_traction;

enum
{
    TRACTION_ON = 0,
    TRACTION_HALF = 1,
    TRACTION_OFF = 2,
};

// Ferrari is slipping/skidding either after collision or round bend
extern uint16_t OFerrari_is_slipping;

// Slip Command Sent To Sound Hardware
extern uint8_t OFerrari_slip_sound;

// Stores previous value of car_increment
extern uint16_t OFerrari_car_inc_old;

// Difference between car_x_pos and car_x_old
extern int16_t OFerrari_car_x_diff;

// -------------------------------------------------------------------------
// Engine Stop Flag
// -------------------------------------------------------------------------

// Flag set when switching back to in-game engine, to be used with revs_post_stop
// This is used to adjust the rev boost when returning to game
extern int16_t OFerrari_rev_stop_flag;

// Rev boost when we switch back to ingame engine and hand user control. 
// Set by user being on revs before initialization.
extern int16_t OFerrari_revs_post_stop;

extern int16_t OFerrari_acc_post_stop;

// -------------------------------------------------------------------------
// Engine Sounds. Probably needs to be moved
// -------------------------------------------------------------------------

// Sound: Adjusted rev value (to be used to set pitch sound fx)
extern uint16_t OFerrari_rev_pitch1;

extern uint16_t OFerrari_rev_pitch2;

// -------------------------------------------------------------------------
// Ferrari Specific Values
// -------------------------------------------------------------------------

// *22 [Word] AI Curve Counter. Increments During Curve. Resets On Straight.
extern int16_t OFerrari_sprite_ai_counter;

// *24 [Word] AI Curve Value. 0x96 - curve_next.
extern int16_t OFerrari_sprite_ai_curve;

// *26 [Word] AI X Position Adjustment
extern int16_t OFerrari_sprite_ai_x;

// *28 [Word] AI Steering Adjustment
extern int16_t OFerrari_sprite_ai_steer;
    
// *2A [Word] Car X Position Backup
extern int16_t OFerrari_sprite_car_x_bak;

// *2C [Word] Wheel State
extern int16_t OFerrari_sprite_wheel_state;

// *2E [Word] Ferrari Slipping (Copy of slip counter)
extern int16_t OFerrari_sprite_slip_copy;

// *39 [Byte] Wheel Palette Offset
extern int8_t OFerrari_wheel_pal;

// *3A [Word] Passenger Y Offset
extern int16_t OFerrari_sprite_pass_y;

// *3C [Word] Wheel Frame Counter Reset
extern int16_t OFerrari_wheel_frame_reset;

// *3E [Word] Wheel Frame Counter Reset
extern int16_t OFerrari_wheel_counter;


void OFerrari_init(oentry*, oentry*, oentry*, oentry*);
void OFerrari_reset_car();
void OFerrari_init_ingame();
void OFerrari_tick();
void OFerrari_set_ferrari_x();
void OFerrari_set_ferrari_bounds();
void OFerrari_check_wheels();
void OFerrari_set_curve_adjust();
void OFerrari_draw_shadow();
void OFerrari_move();
void OFerrari_do_sound_score_slip();
void OFerrari_shake();
void OFerrari_do_skid();
    

