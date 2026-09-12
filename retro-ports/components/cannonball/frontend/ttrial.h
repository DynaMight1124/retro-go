/***************************************************************************
    Time Trial Mode Front End.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.h"

// Maximum number of laps to allow the player to race
#define TTRIAL_MAX_LAPS 5

// Maximum number of cars to spawn
#define TTRIAL_MAX_TRAFFIC 8

enum
{
    TTRIAL_BACK_TO_MENU = -1,
    TTRIAL_CONTINUE = 0,
    TTRIAL_INIT_GAME = 1,
};

extern uint8_t TTrial_state;
extern int8_t TTrial_level_selected;

enum
{
    TTRIAL_INIT_COURSEMAP,
    TTRIAL_TICK_COURSEMAP,
    TTRIAL_TICK_GAME_ENGINE,
};


void TTrial_init();
int  TTrial_tick();
void TTrial_update_best_time();

