/***************************************************************************
    Best Outrunners Name Entry & Display.
    Used in attract mode, and at game end.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.h"

typedef struct
{
    uint32_t score;
    uint8_t initial1;
    uint8_t initial2;
    uint8_t initial3;
    uint32_t maptiles;
    uint16_t time;
} score_entry;

// Number of score entries in table
#define HISCORE_NUM_SCORES 20
    
// 20 Score Entries
extern score_entry OHiScore_scores[HISCORE_NUM_SCORES];

void OHiScore_init();
void OHiScore_init_def_scores();
void OHiScore_tick();
void OHiScore_setup_pal_best();
void OHiScore_setup_road_best();
void OHiScore_display_scores();

