/***************************************************************************
    Music Selection Screen.

    This is a combination of a tilemap and overlayed sprites.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"


// Music Track Selected By Player
extern uint8_t OMusic_music_selected;

uint8_t OMusic_load_widescreen_map();
void OMusic_enable();
void OMusic_disable();
void OMusic_tick();
void OMusic_blit();
void OMusic_check_start();




