/***************************************************************************
    Level Object Logic
    
    This class handles rendering most of the objects that comprise a typical
    level. 
    
    - Configures rendering properties (co-ordinates, zoom etc.) 
    - Object specific logic, including collision checks & start lights etc.

    The original codebase contains a large amount of code duplication,
    much of which is duplicated here.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.h"



// Spray Counter (Going Through Water).
extern uint16_t OLevelObjs_spray_counter;

// Wheel Spray Type
// 00 = Water
// 04 = Yellow Stuff
// 08 = Green Stuff
// 0c = Pink stuff
// 10 = Smoke
extern uint16_t OLevelObjs_spray_type;

//	Collision With Sprite Has Ocurred
//
// 0 = No Collision
// 1 = Collision (and increments for every additional collision in this crash cycle)
extern uint8_t OLevelObjs_collision_sprite;

// Sprite Collision Counter (Hitting Scenery)
extern int16_t OLevelObjs_sprite_collision_counter;

void OLevelObjs_init_startline_sprites();
void OLevelObjs_init_timetrial_sprites();
void OLevelObjs_init_hiscore_sprites();
void OLevelObjs_setup_sprites(uint32_t);
void OLevelObjs_do_sprite_routine();
void OLevelObjs_hide_sprite(oentry*);
