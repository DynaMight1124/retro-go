#pragma once

#include "stdint.h"

#ifdef RETRO_GO
#include <rg_system.h>
#include <rg_utils.h>
#include <esp_heap_caps.h>
// Use standard Retro-Go allocator for stability
#define malloc(x) rg_alloc(x, MEM_SLOW)
#define free(x) free(x)

// Single source of truth for music toggle
extern int enable_ym2151_synth;

// Core 1 Audio Task
extern rg_task_t *audio_task_handle;
#endif

#define REAL_AUDIO_FREQUENCY 22050

// ------------------------------------------------------------------------------------------------
// Debug Settings
// ------------------------------------------------------------------------------------------------

#define DEBUG_LEVEL 0

// Force AI to play the levels
#define FORCE_AI 0

// ------------------------------------------------------------------------------------------------
// Cannonball Specific Constants
// ------------------------------------------------------------------------------------------------

// Original Screen Width and Height
#define S16_WIDTH      320
#define S16_HEIGHT     224

// Widescreen width: (320 / 4) * 5. 
#define S16_WIDTH_WIDE 398

// Palette Address in Memory
#define S16_PALETTE_BASE    0x120000

// Number of Palette Entries
#define S16_PALETTE_ENTRIES 4096

// Number of stages
#define STAGES 15

// Hard Coded End Point of every level
#define ROAD_END      0x79C

// End Point of level for CPU1, including horizon
#define ROAD_END_CPU1 0x904

// Common Bits
enum
{
    BIT_0 = 0x01,
    BIT_1 = 0x02,
    BIT_2 = 0x04,
    BIT_3 = 0x08,
    BIT_4 = 0x10,
    BIT_5 = 0x20,
    BIT_6 = 0x40,
    BIT_7 = 0x80,
    BIT_8 = 0x100,
    BIT_9 = 0x200,
    BIT_A = 0x400
};
