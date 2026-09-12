/***************************************************************************
    OutRun Utility Functions & Assembler Helper Functions. 

    Common OutRun library functions.
    Helper functions used to facilitate 68K to C++ porting process.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.h"


extern const uint8_t DEC_TO_HEX[];

void outils_reset_random_seed();
uint32_t outils_random();
int32_t outils_isqrt(int32_t);
uint16_t outils_convert16_dechex(uint16_t);
uint32_t outils_bcd_add(uint32_t, uint32_t);
uint32_t outils_bcd_sub(uint32_t, uint32_t);

// Inline functions
void outils_move16(uint32_t src, uint32_t* dst);
void outils_add16(uint32_t src, uint32_t* dst);
void outils_sub16(int32_t src, int32_t* dst);
void outils_swap32(int32_t* v);
void outils_swapU32(uint32_t* v);
void outils_convert_counter_to_time(uint16_t counter, uint8_t* converted);

