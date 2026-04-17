/***************************************************************************
    Course Map Logic & Rendering. 
    
    This is the full-screen map that is displayed at the end of the game. 
    
    The logo is built from multiple sprite components.
    
    The course map itself is made up of sprites and pieced together. 
    It's not a tilemap.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"


// Load Sprites Needed for Course Map
extern uint8_t OMap_init_sprites;

void OMap_init();
void OMap_tick();
void OMap_blit();
void OMap_load_sprites();
void OMap_draw_course_map();
void OMap_position_ferrari(uint8_t index);
