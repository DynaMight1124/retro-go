/***************************************************************************
    Interface to Ported Z80 Code.
    Handles the interface between 68000 program code and Z80.

    Also abstracted here, so the more complex OSound class isn't exposed
    to the main code directly
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "hwaudio/segapcm.h"
#include "hwaudio/ym2151.h"
#include "engine/audio/commands.h"

#define OSoundInt_PCM_RAM_SIZE 0x100

// Note whether the game has booted
extern uint8_t OSoundInt_has_booted;

// [+0] Unused
// [+1] Engine pitch high
// [+2] Engine pitch low
// [+3] Engine pitch vol
// [+4] Traffic data #1 
// [+5] Traffic data #2 
// [+6] Traffic data #3 
// [+7] Traffic data #4
extern uint8_t OSoundInt_engine_data[8];

void OSoundInt_init();
void OSoundInt_reset();
void OSoundInt_tick();

void OSoundInt_play_queued_sound();
void OSoundInt_queue_sound_service(uint8_t snd);
void OSoundInt_queue_sound(uint8_t snd);
void OSoundInt_queue_clear();


