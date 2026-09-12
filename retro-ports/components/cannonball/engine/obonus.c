/***************************************************************************
    Bonus Points Code.
    
    This is the code that displays your bonus points on completing the game.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/ostats.h"
#include "engine/ohud.h"
#include "engine/outils.h"
#include "engine/obonus.h"


int8_t OBonus_bonus_control;
int8_t OBonus_bonus_state;
int16_t OBonus_bonus_timer;


// Bonus seconds (for seconds countdown at bonus stage)
//
// Stored as hex, but should be converted to decimal for true value.
//
// So 0x314 would be 78.8 seconds
int16_t bonus_secs;

// Timing counter used in bonus code logic
int16_t bonus_counter;

void OBonus_init_bonus_text();
void OBonus_blit_bonus_secs();
void OBonus_decrement_bonus_secs();


void OBonus_init()
{
    OBonus_bonus_control = BONUS_DISABLE;
    OBonus_bonus_state   = BONUS_TEXT_INIT;

    bonus_counter = 0;
}

// Display Text and countdown time for bonus mode
// Source: 0x99E0
void OBonus_do_bonus_text()
{
    switch (OBonus_bonus_state)
    {
        case BONUS_TEXT_INIT:
            OBonus_init_bonus_text();
            break;

        case BONUS_TEXT_SECONDS:
            OBonus_decrement_bonus_secs();
            break;

        case BONUS_TEXT_CLEAR:
        case BONUS_TEXT_DONE:
            if (bonus_counter < 60)
                bonus_counter++;
            else
            {
                OHud_blit_text2(TEXT2_BONUS_CLEAR1);
                OHud_blit_text2(TEXT2_BONUS_CLEAR2);
                OHud_blit_text2(TEXT2_BONUS_CLEAR3);
            }
            break;
    }
}

// Calculate Bonus Seconds. This uses the seconds remaining for every lap raced.
// Print stuff to text layer for bonus mode.
//
// Source: 0x9A9C
void OBonus_init_bonus_text()
{
    int i = 0;
    OBonus_bonus_state = BONUS_TEXT_SECONDS;
    
    int16_t time_counter_bak = OStats_time_counter << 8;
    OStats_time_counter = 0x30;

    uint16_t total_time = 0;

    if (Outrun_cannonball_mode == OUTRUN_MODE_ORIGINAL)
    {
        // Add milliseconds remaining from previous stage times
        for (i = 0; i < 5; i++)
        {
            total_time = outils_bcd_add(DEC_TO_HEX[OStats_stage_times[i][2]], total_time);
        }
    }

    // Mask on top digit of lap milliseconds
    total_time &= 0xF0;

    if (total_time)
    {
        time_counter_bak |= (10 - (total_time >> 4));
    }
    // So 60 seconds remaining on the clock and 3 from lap_seconds would be 0x0603

    // Extract individual 3 digits
    uint16_t digit_mid = (time_counter_bak >> 8  & 0x0F) * 10;
    uint16_t digit_top = (time_counter_bak >> 12 & 0x0F) * 100;
    uint16_t digit_bot = (time_counter_bak & 0x0F);

    // Write them back to final bonus seconds value
    bonus_secs = digit_bot + digit_mid + digit_top;

    // Write to text layer
    OHud_blit_text2(TEXT2_BONUS_POINTS); // Print "BONUS POINTS"
    OHud_blit_text1(TEXT1_BONUS_STOP);   // Print full stop after Bonus Points text
    OHud_blit_text1(TEXT1_BONUS_SEC);    // Print "SEC"
    OHud_blit_text1(TEXT1_BONUS_X);      // Print 'X' symbol after SEC
    OHud_blit_text1(TEXT1_BONUS_PTS);    // Print "PTS"

    // Blit big 100K number
    uint32_t src_addr = TEXT1_BONUS_100K;
    uint32_t dst_addr = 0x11065A;
    int8_t count = RomLoader_read8IncP(&Roms_rom0, &src_addr);

    for (i = 0; i <= count; i++)
        OHud_blit_large_digit(&dst_addr, (RomLoader_read8IncP(&Roms_rom0, &src_addr) - 0x30) << 1);

    OBonus_blit_bonus_secs();
}

// Decrement bonus seconds. Blit seconds remaining.
// Source: 9A08
void OBonus_decrement_bonus_secs()
{
    if (bonus_counter < 60)
    {
        bonus_counter++;
        return;
    }

    // Play Signal 1 Sound In A Steady Fashion
    if ((((bonus_counter - 1) ^ bonus_counter) & BIT_2) == 0)
        OSoundInt_queue_sound(sound_SIGNAL1);

    // Increment Score by 100K points
    OStats_update_score(0x100000);
    
    // Blit bonus seconds remaining
    OBonus_blit_bonus_secs();
    
    if (--bonus_secs < 0)
    {
        bonus_counter = -1;
        OBonus_bonus_state = BONUS_TEXT_CLEAR;
    }
    else
        bonus_counter++;

}

// Blit large yellow second remaining value e.g.: 23.3
// Source: 0x9B7C
void OBonus_blit_bonus_secs()
{
    const uint8_t COL2 = 0x80;
    const uint16_t TILE_DOT = 0x8C2E;
    const uint16_t TILE_ZERO = 0x8420;

    uint32_t d1 = (bonus_secs / 100) << 8;
    uint32_t d4 = (bonus_secs / 100) * 100;

    uint32_t d2 = (bonus_secs - d4) / 10;
    uint32_t d3 = bonus_secs - d4;

    d4 = d2;
    d2 <<= 4;
    d4 *= 10;
    d3 -= d4;
    d1 += d2;
    d1 += d3;

    d3 = (d1 & 0xF)   << 1;
    d2 = (d1 & 0xF0)  >> 3;
    d1 = (d1 & 0xF00) >> 7;

    uint32_t text_addr = 0x110644;

    // Blit Digit 1
    if (d1)
    {
        OHud_blit_large_digit(&text_addr, d1);
    }
    else
    {
        Video_write_text16(text_addr, TILE_ZERO);
        Video_write_text16(text_addr | COL2, TILE_ZERO);
        text_addr += 2;        
    }

    // Blit Digit 2
    OHud_blit_large_digit(&text_addr, d2);
    // Blit Dot
    Video_write_text16(text_addr | COL2, TILE_DOT);
    text_addr += 2;
    // Blit Digit 3
    OHud_blit_large_digit(&text_addr, d3);
}
