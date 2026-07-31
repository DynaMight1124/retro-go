#pragma once

#include "stdint.h"

#ifdef RETRO_GO
#include <rg_system.h>
#include <rg_utils.h>
#include <esp_heap_caps.h>

// Render target-native RGB565 directly into Retro-Go's owned surface. Define
// this to 0 at build time to retain the original selector + conversion path.
#ifndef CANNONBALL_DIRECT_RGB565
#define CANNONBALL_DIRECT_RGB565 1
#endif

// Use standard Retro-Go allocator for stability
#define malloc(x) rg_alloc(x, MEM_SLOW)
#define free(x) free(x)

// Single source of truth for music toggle
extern int enable_ym2151_synth;

// Core 1 Audio Task
extern rg_task_t *audio_task_handle;
#endif

#define REAL_AUDIO_FREQUENCY 22050
#ifdef RETRO_GO
// Internal synthesis rate. 11040 is exactly divisible by Cannonball's 30 Hz
// tick rate; each 368-frame block is resampled to 735 sink frames.
#define SYNTH_AUDIO_FREQUENCY 11040
#else
#define SYNTH_AUDIO_FREQUENCY REAL_AUDIO_FREQUENCY
#endif

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

#if defined(RETRO_GO) && CANNONBALL_DIRECT_RGB565
extern uint16_t Render_rgb[S16_PALETTE_ENTRIES * 3];
static inline uint16_t Video_output_color(uint32_t selector)
{
    return Render_rgb[selector & ((S16_PALETTE_ENTRIES * 3) - 1)];
}
uint16_t Render_shadow_color(uint16_t color);
#else
static inline uint16_t Video_output_color(uint32_t selector)
{
    return (uint16_t)selector;
}
#endif

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
