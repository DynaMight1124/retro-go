/***************************************************************************
    Time Trial Mode Front End.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "sdl/input.h"
#include "frontend/ttrial.h"

#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/outils.h"
#include "engine/omap.h"
#include "engine/ostats.h"
#include "engine/otiles.h"

uint8_t TTrial_state;
int8_t TTrial_level_selected;


// Best lap times for all 15 tracks.
uint16_t* TTrial_best_times = Config_ttrial.best_times;

// Counter converted to actual laptime
uint8_t best_converted[3];

// Track Selection: Ferrari Position Per Track
// This is a link to a sprite object that represents part of the course map.
static const uint8_t FERRARI_POS[] = 
{
    1,5,3,11,9,7,19,17,15,13,24,23,22,21,20
};

// Map Stage Number to Internal Lookup 
static const uint8_t STAGE_LOOKUP[] = 
{
    0x00,
    0x09, 0x08,
    0x12, 0x11, 0x10,
    0x1B, 0x1A, 0x19, 0x18,
    0x24, 0x23, 0x22, 0x21, 0x20
};



void TTrial_init()
{
    TTrial_state = TTRIAL_INIT_COURSEMAP;
}

int TTrial_tick()
{
    switch (TTrial_state)
    {
        case TTRIAL_INIT_COURSEMAP:
            Outrun_select_course(Config_engine.jap != 0, Config_engine.prototype != 0); // Need to setup correct course map graphics.
            Config_load_tiletrial_scores();
            OSprites_init();
            Video_enabled = 1;
            HWSprites_set_x_clip(0);
            OMap_init();
            OMap_load_sprites();              
            OMap_position_ferrari(FERRARI_POS[TTrial_level_selected = 0]);
            OHud_blit_text_big(1, "STEER TO SELECT TRACK", 0);
            OHud_blit_text1XY(2, 25, TEXT1_LAPTIME1);
            OHud_blit_text1XY(2, 26, TEXT1_LAPTIME2);
            OSoundInt_queue_sound(sound_PCM_WAVE);
            Outrun_ttrial.laps    = Config_ttrial.laps;
            Outrun_custom_traffic = Config_ttrial.traffic;
            TTrial_state = TTRIAL_TICK_COURSEMAP;

        case TTRIAL_TICK_COURSEMAP:
            {
                if (Input_has_pressed(INPUT_MENU))
                {
                    return TTRIAL_BACK_TO_MENU;
                }
                else if (Input_has_pressed(INPUT_LEFT) || OInputs_is_analog_l())
                {
                    if (--TTrial_level_selected < 0)
                        TTrial_level_selected = sizeof(FERRARI_POS) - 1;
                }
                else if (Input_has_pressed(INPUT_RIGHT)|| OInputs_is_analog_r())
                {
                    if (++TTrial_level_selected > sizeof(FERRARI_POS) - 1)
                        TTrial_level_selected = 0;
                }
                else if (Input_has_pressed(INPUT_START) || Input_has_pressed(INPUT_ACCEL) || OInputs_is_analog_select())
                {
                    outils_convert_counter_to_time(TTrial_best_times[TTrial_level_selected], best_converted);

                    Outrun_cannonball_mode         = OUTRUN_MODE_TTRIAL;
                    Outrun_ttrial.level            = STAGE_LOOKUP[TTrial_level_selected];
                    Outrun_ttrial.current_lap      = 0;
                    Outrun_ttrial.best_lap_counter = 10000;
                    Outrun_ttrial.best_lap[0]      = best_converted[0];
                    Outrun_ttrial.best_lap[1]      = best_converted[1];
                    Outrun_ttrial.best_lap[2]      = best_converted[2];
                    Outrun_ttrial.best_lap_counter = TTrial_best_times[TTrial_level_selected];
                    Outrun_ttrial.new_high_score   = 0;
                    Outrun_ttrial.overtakes        = 0;
                    Outrun_ttrial.crashes          = 0;
                    Outrun_ttrial.vehicle_cols     = 0;
                    OStats_credits = 1;
                    return TTRIAL_INIT_GAME;
                }
                OMap_position_ferrari(FERRARI_POS[TTrial_level_selected]);
                outils_convert_counter_to_time(TTrial_best_times[TTrial_level_selected], best_converted);
                OHud_draw_lap_timer(OHud_translate(7, 26, 0x110030), best_converted, best_converted[2]);
                OMap_blit();
                ORoad_tick();
                OSprites_sprite_copy();
                OSprites_update_sprites();
                OTiles_write_tilemap_hw();
                OTiles_update_tilemaps(0);
            }
            break;
    }

    return TTRIAL_CONTINUE;
}

void TTrial_update_best_time()
{
    TTrial_best_times[TTrial_level_selected] = Outrun_ttrial.best_lap_counter;
    Config_save_tiletrial_scores();
}
