/***************************************************************************
    Collision & Crash Code. 
    
    There are two types of collision: Scenery & Traffic.
    
    1/ Traffic: The Ferrari will spin after a collision.
    2/ Scenery: There are three types of scenery collision:
       - Low speed bump. Car rises slightly in the air and stalls.
       - Mid speed spin. Car spins and slides after collision.
       - High speed flip. If slightly slower, car rolls into screen.
         Otherwise, grows towards screen and vanishes
         
    Known Issues With Original Code:
    - Passenger sprites flicker if they land moving in the water on Stage 1
    
    The Ferrari sprite is used differently by the crash code.
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"


// Reference to sprite used by crash code
extern oentry* OCrash_spr_ferrari;
extern oentry* OCrash_spr_shadow;
extern oentry* OCrash_spr_pass1;
extern oentry* OCrash_spr_pass1s;
extern oentry* OCrash_spr_pass2;
extern oentry* OCrash_spr_pass2s;

// Default value to reset skid counter to on collision
const static uint8_t SKID_RESET = 0x14;

// Maximum value to allow skid reset to be set to during collision
const static uint8_t SKID_MAX = 0x1E;

// Amount to adjust car x position by when skidding
const static uint8_t SKID_X_ADJ = 0x18;

//Crash State [Investigate Further]
//
// 0 = No crash
// 1 = Collision with object. Init Spin if medium speed collision.
// 2 = Flip Car.
// 3 = Slide Car. Trigger Smoke Cloud.
// 4 = Horizontally Flip Car. Trigger Smoke Cloud.
// 5 = SPIN: Remove Smoke From Spin. Girl Points Finger.
//     FLIP: Flip Animation Done.
// 6 = Pan Camera To Track Centre
// 7 = Camera Repositioned. Ready For Restart
//
// (Note that hitting another vehicle and skidding does not affect the crash state)
extern int8_t OCrash_crash_state;

// Skid Counter. Set On Collision With Another Vehicle Only.
//
// If positive, skid to the left.
// If negative, skid to the right.
extern int16_t OCrash_skid_counter;
extern int16_t OCrash_skid_counter_bak;

// Spin Control 1 - SPIN only
//
// 0 = No Spin
// 1 = Init Spin Car
// 2 = Spin In Progress
extern uint8_t OCrash_spin_control1;

extern uint8_t OCrash_spin_control2;

// Increments on a per collision basis (Follows on from collision_sprite being set)
//
// Used to cycle passenger animations after a crash
//
// coll_count1 != coll_count2 = Crash Subroutines Not Enabled
// coll_count1 == coll_count2 = Crash Subroutines Enabled
extern int16_t OCrash_coll_count1;
extern int16_t OCrash_coll_count2;

// Counter that increments per-frame during a crash scenario
extern int16_t OCrash_crash_counter;

// Denotes the spin/flip number following the crash.
//
// 0 = No crash has taken place yet
// 1 = Crash has taken place this level
// 2 = Crash. First Spin/Flip. 
// 3 = Crash. Second Spin/Flip.
// 4 = Crash. Third Spin/Flip.
extern int16_t OCrash_crash_spin_count;

// Crash Sprite Z Position
extern int16_t OCrash_crash_z;


void OCrash_init(oentry* f, oentry* s, oentry* p1, oentry* p1s, oentry* p2, oentry* p2s);
uint8_t OCrash_is_flip();
void OCrash_enable();
void OCrash_clear_crash_state();
void OCrash_tick();




