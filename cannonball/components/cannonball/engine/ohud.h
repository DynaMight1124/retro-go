/***************************************************************************
    Heads-Up Display (HUD) Code
    
    - Score Rendering
    - Timer Rendering
    - Rev Rendering
    - Minimap Rendering
    - Text Rendering
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "outrun.h"

// Colours for default text palette
enum 
{
    HUD_GREY  = 0x84,
    HUD_PINK  = 0x86,
    HUD_GREEN = 0x92,
};

// Base for digits, for fast digit drawing
const static uint16_t HUD_DIGIT_BASE = 0x30;

void OHud_draw_main_hud();
void OHud_draw_fps_counter(int16_t);
void OHud_clear_timetrial_text();
void OHud_do_mini_map();
void OHud_draw_timer1(uint16_t);
void OHud_draw_timer2(uint16_t, uint32_t, uint16_t);
void OHud_draw_lap_timer(uint32_t, uint8_t*, uint8_t);
void OHud_draw_score_ingame(uint32_t);
void OHud_draw_score(uint32_t, const uint32_t, const uint8_t);
void OHud_draw_score_tile(uint32_t, const uint32_t, const uint8_t);
void OHud_draw_stage_number(uint32_t, uint8_t, uint16_t col);
void OHud_draw_rev_counter();
void OHud_draw_debug_info(uint32_t pos, uint16_t height_pat, uint8_t sprite_pat);
void OHud_blit_text1(uint32_t);
void OHud_blit_text1XY(uint8_t x, uint8_t y, uint32_t src_addr);
void OHud_blit_text2(uint32_t);
void OHud_blit_text_big(const uint8_t Y, const char* text, uint8_t do_notes);
void OHud_blit_text_new(uint16_t, uint16_t, const char* text, uint16_t col);
void OHud_blit_speed(uint32_t, uint16_t);
void OHud_blit_large_digit(uint32_t*, uint8_t);
void OHud_draw_copyright_text();
void OHud_draw_insert_coin();
void OHud_draw_credits();
uint32_t OHud_setup_mini_map();
uint32_t OHud_translate(uint16_t x, uint16_t y, uint32_t BASE_POS);
