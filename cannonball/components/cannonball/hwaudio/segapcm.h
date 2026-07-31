/***************************************************************************
    Sega 8-Bit PCM Driver
    
    This driver is based upon the MAME source code, with some minor 
    modifications to integrate it into the Cannonball framework.

    Note, that I've altered this driver to output at a fixed 44,100Hz.
    This is to avoid the need for downsampling.  
    
    See http://mamedev.org/source/docs/license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.h"
#include "romloader.h"
#include "globals.h"

extern const uint32_t AUDIO_FREQUENCY;

// Size of the buffer (including channel info)
extern uint32_t SegaPCM_buffer_size;

static const uint32_t SEGAPCM_BANK_256    = (11);
static const uint32_t SEGAPCM_BANK_512    = (12);
static const uint32_t SEGAPCM_BANK_12M    = (13);
static const uint32_t SEGAPCM_BANK_MASK7  = (0x70 << 16);
static const uint32_t SEGAPCM_BANK_MASKF  = (0xf0 << 16);
static const uint32_t SEGAPCM_BANK_MASKF8 = (0xf8 << 16);

void SegaPCM_Create(uint32_t clock, RomLoader* rom, uint8_t* ram, int32_t bank);
void SegaPCM_Destroy();

void SegaPCM_init(int32_t fps);
void SegaPCM_stream_update();
int16_t* SegaPCM_get_buffer();
void SegaPCM_set_volume(uint8_t);
