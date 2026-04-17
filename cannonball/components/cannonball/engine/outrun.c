/***************************************************************************
    OutRun Engine Entry Point.

    This is the hub of the ported OutRun code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "setup.h"
#include "main.h"
#include "trackloader.h"
#include "utils.h"
#include "engine/oattractai.h"
#include "engine/oanimseq.h"
#include "engine/obonus.h"
#include "engine/ocrash.h"
#include "engine/oferrari.h"
#include "engine/ohiscore.h"
#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/olevelobjs.h"
#include "engine/ologo.h"
#include "engine/omap.h"
#include "engine/omusic.h"
#include "engine/ooutputs.h"
#include "engine/osmoke.h"
#include "globals.h"
#include "engine/outrun.h"
#include "engine/opalette.h"
#include "engine/ostats.h"
#include "engine/otiles.h"
#include "engine/otraffic.h"
#include "engine/outils.h"

/*
    Known Core Engine Issues:

    - Road split. Minor bug on positioning traffic on correct side of screen for one frame or so at the point of split.
      Most noticeable in 60fps mode. 
      The Dreamcast version exhibits a bug where the road renders on the wrong side of the screen for one frame at this point.
      The original version (and Cannonball) has a problem where the cars face the wrong direction for one frame. 

    Bugs Present In Original 1986 Release:

    - Millisecond displays incorrectly on Extend Time screen [fixed]
    - Erroneous values in sprite zooming table [fixed]
    - Shadow popping into position randomly. Try setting car x position to 0x1E2. (0x260050) [fixed]
    - Stage 2a: Incomplete arches due to lack of sprite slots [fixed]
    - Best OutRunners screen looks odd after Stage 2 Gateway
    - Stage 3c: Clouds overlapping trees [unable to fix easily]
    - Sometimes the Ferrari stalls on the start-line on game restart. Happens in Attract Mode too.
    - On completion screen, some of the side crowd graphics are misplaced. Japanese version only [fixed]

*/

uint8_t Outrun_freeze_timer;
uint8_t Outrun_cannonball_mode;
uint8_t Outrun_custom_traffic;
time_trial_t Outrun_ttrial;
uint8_t Outrun_service_mode;
uint8_t Outrun_tick_frame;
uint32_t Outrun_tick_counter;
int8_t Outrun_game_state;
adr_t Outrun_adr;


uint8_t attract_view;
int16_t attract_counter;

// Car Increment Backup for attract mode
uint32_t car_inc_bak;

// Debug to denote when fork has been chosen
int8_t fork_chosen;

void Outrun_jump_table(Packet* packet);
void Outrun_init_jump_table();
void Outrun_main_switch();
void Outrun_controls();
uint8_t Outrun_decrement_timers();
void Outrun_init_motor_calibration();
void Outrun_init_attract();
void Outrun_tick_attract();
void Outrun_check_freeplay_start();

void Outrun_init()
{
    Outrun_freeze_timer = Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL ? 1 : Config_engine.freeze_timer;
    Video_enabled = 0;
    Outrun_select_course(Config_engine.jap != 0, Config_engine.prototype != 0);
    Video_clear_text_ram();

    Outrun_tick_counter = 0;

    Outrun_boot();
}

void Outrun_boot()
{
    Outrun_game_state = Config_engine.layout_debug ? GS_INIT_GAME : GS_INIT;
    // Initialize default hi-score entries
    OHiScore_init_def_scores();
    // Load saved hi-score entries
    Config_load_scores(getScoresFilename());
    OStats_init(Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL);
    Outrun_init_jump_table();
    OInitEngine_init(Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL ? Outrun_ttrial.level : 0);
    OSoundInt_init();
    outils_reset_random_seed(); // Ensure we match the genuine boot up of the original game each time
}

void Outrun_tick(Packet* packet, uint8_t tick_frame)
{
    Outrun_tick_frame = tick_frame;
    
    if (cannonball_tick_frame)
        Outrun_tick_counter++;

    if (Outrun_game_state >= GS_START1 && Outrun_game_state <= GS_INGAME)
    {
        if (Input_has_pressed(INPUT_VIEWPOINT))
        {
            int mode = ORoad_get_view_mode() + 1;
            if (mode > ROAD_VIEW_INCAR)
                mode = ROAD_VIEW_ORIGINAL;

            ORoad_set_view_mode(mode, 0);
        }
    }

    // Only tick the road cpu twice for every time we tick the main cpu
    // The timing here isn't perfect, as normally the road CPU would run in parallel with the main CPU.
    // We can potentially hack this by calling the road CPU twice.
    // Most noticeable with clipping sprites on hills.

    // 30 FPS
    // Updates Game Logic 1/2 frames
    // Updates V-Blank 1/2 frames
    if (Config_fps == 30 && Config_tick_fps == 30)
    {
        Outrun_jump_table(packet);
        ORoad_tick();
        Outrun_vint();
        Outrun_vint();
    }
    // 30/60 FPS Hybrid. (This is the same as the original game)
    // Updates Game Logic 1/2 frames
    // Updates V-Blank 1/1 frames
    else if (Config_fps == 60 && Config_tick_fps == 30)
    {
        if (cannonball_tick_frame)
        {
            Outrun_jump_table(packet);
            ORoad_tick();
        }
        Outrun_vint();
    }
    // 60 FPS. Smooth Mode.
    // Updates Game Logic 1/1 frames
    // Updates V-Blank 1/1 frames
    else
    {
        Outrun_jump_table(packet);
        ORoad_tick();
        Outrun_vint();
    }

    // Draw FPS
    if (Config_video.fps_count)
        OHud_draw_fps_counter(cannonball_fps_counter);
}

// Vertical Interrupt
void Outrun_vint()
{
    OTiles_write_tilemap_hw();
    OSprites_update_sprites();
    OTiles_update_tilemaps(Outrun_cannonball_mode == OUTRUN_MODE_ORIGINAL ? OStats_cur_stage : 0);

    if (Config_fps < 120 || (cannonball_frame & 1))
    {
        OPalette_cycle_sky_palette();
        OPalette_fade_palette();
        // ... 
        OStats_do_timers();
        if (Outrun_cannonball_mode != OUTRUN_MODE_TTRIAL) OHud_draw_timer1(OStats_time_counter);
        uint8_t coin = OInputs_do_credits();
        OOutputs_coin_chute_out(&OOutputs_chute1, coin == 1);
        OOutputs_coin_chute_out(&OOutputs_chute2, coin == 2);
        OInitEngine_set_granular_position();
    }
}

void Outrun_jump_table(Packet* packet)
{
    if (Outrun_tick_frame && Outrun_game_state != GS_CALIBRATE_MOTOR)
    {
        Outrun_main_switch();                  // Address #1 (0xB128) - Main Switch
        OInputs_adjust_inputs();        // Address #2 (0x74D8) - Adjust Analogue Inputs
    }

    switch (Outrun_game_state)
    {
        case GS_REINIT:
        case GS_CALIBRATE_MOTOR:
            break;
 
        // ----------------------------------------------------------------------------------------
        // Couse Map Specific Code
        // ----------------------------------------------------------------------------------------
        case GS_MAP:
            OMap_tick();
            break;


        case GS_MUSIC:
            OSprites_tick();
            OLevelObjs_do_sprite_routine();

            if (!Outrun_tick_frame)
            {
                OMusic_blit();
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Best OutRunners Entry (EditJumpTable3 Entries)
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BEST2:
        case GS_BEST2:
            OSprites_tick();
            OLevelObjs_do_sprite_routine();

            if (!Outrun_tick_frame)
            {
                // Check for start button if credits are remaining and set state to Music Selection
                if (OStats_credits && Input_is_pressed_clear(INPUT_START))
                    Outrun_game_state = GS_INIT_MUSIC;
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Core Game Engine Routines
        // ----------------------------------------------------------------------------------------
        case GS_LOGO:
            if (!cannonball_tick_frame)
                OLogo_blit();

        case GS_ATTRACT:
        case GS_BEST1:
            if (Outrun_tick_frame)
                Outrun_check_freeplay_start();
        
        default:
            if (Outrun_tick_frame) OSprites_tick();                // Address #3 Jump_SetupSprites
            OLevelObjs_do_sprite_routine();                 // replaces calling each sprite individually
            if (!Config_engine.disable_traffic)
                OTraffic_tick();                            // Spawn & Tick Traffic
            if (Outrun_tick_frame) OInitEngine_init_crash_bonus(); // Initalize crash sequence or bonus code
            OFerrari_tick();
            if (OFerrari_state != FERRARI_END_SEQ)
            {
                OAnimSeq_flag_seq();
                OCrash_tick();
                OSmoke_draw_ferrari_smoke(&OSprites_jump_table[SPRITE_SMOKE1]); // Do Left Hand Smoke
                OFerrari_draw_shadow();                                                   // (0xF1A2) - Draw Ferrari Shadow
                OSmoke_draw_ferrari_smoke(&OSprites_jump_table[SPRITE_SMOKE2]); // Do Right Hand Smoke
            }
            else
            {
                OSmoke_draw_ferrari_smoke(&OSprites_jump_table[SPRITE_SMOKE1]); // Do Left Hand Smoke
                OSmoke_draw_ferrari_smoke(&OSprites_jump_table[SPRITE_SMOKE2]); // Do Right Hand Smoke
            }
            break;
    }

    OSprites_sprite_copy();

    // Motor Code
    if (Outrun_tick_frame)
    {
        if (Outrun_game_state == GS_CALIBRATE_MOTOR)
        {
            if (OOutputs_calibrate_motor(packet->ai1, packet->mci, 0))
            {
                Video_enabled     = 0;
                Video_clear_text_ram();
                ORoad_horizon_set = 0;
                Outrun_boot();
            }
        }
        else
        {
            if (Config_controls.haptic && Config_controls.analog)
                OOutputs_tick(OUTPUTS_MODE_FFEEDBACK, OInputs_input_steering, -1);
            else if (Config_cannonboard.enabled)
                OOutputs_tick(OUTPUTS_MODE_CABINET, packet->ai1, Config_cannonboard.cabinet);
        }
    }


    if (Config_cannonboard.enabled && Config_cannonboard.debug)
    {
        uint16_t x = 1;
        uint16_t y = 5;
        OHud_blit_text_new(x, y, "AI0 ACCEL", HUD_GREY);   OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->ai0), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "AI2 WHEEL", HUD_GREY);   OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->ai2), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "AI3 BRAKE", HUD_GREY);   OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->ai3), HUD_PINK); x += 13;
      
        x = 1;
        y = 6;
        OHud_blit_text_new(x, y, "AI1 MOTOR", HUD_GREY); OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->ai1), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "MC OUT", HUD_GREY);    OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(OOutputs_hw_motor_control), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "MC IN", HUD_GREY);     OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->mci), HUD_PINK);

        x = 1;
        y = 7;
        OHud_blit_text_new(x, y, "DI1", HUD_GREY);     OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->di1), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "DI2", HUD_GREY);     OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(packet->di2), HUD_PINK); x += 13;
        OHud_blit_text_new(x, y, "DIG OUT", HUD_GREY); OHud_blit_text_new(x + 10, y, Utils_int_to_hex_string(OOutputs_dig_out), HUD_PINK); x += 13;
    }
}

// Source: 0xB15E
void Outrun_main_switch()
{
    switch (Outrun_game_state)
    {
        case GS_INIT:  
            Outrun_init_attract();
            // fall through
            
        // ----------------------------------------------------------------------------------------
        // Attract Mode
        // ----------------------------------------------------------------------------------------
        case GS_ATTRACT:
            Outrun_tick_attract();
            break;

        case GS_INIT_BEST1:
            OFerrari_car_ctrl_active = 0;
            OInitEngine_car_increment = 0;
            OFerrari_car_inc_old = 0;
            OStats_time_counter = 5;
            OStats_frame_counter = OStats_frame_reset;
            OHiScore_init();
            OSoundInt_queue_sound(sound_FM_RESET);
            #ifdef COMPILE_SOUND_CODE
            Audio_clear_wav();
            #endif
            Outrun_game_state = GS_BEST1;

        case GS_BEST1:
            OHud_draw_copyright_text();
            OHiScore_display_scores();
            OHud_draw_credits();
            OHud_draw_insert_coin();
            if (OStats_credits)
                Outrun_game_state = GS_INIT_MUSIC;
            else if (Outrun_decrement_timers())
                Outrun_game_state = GS_INIT_LOGO;
            break;

        case GS_INIT_LOGO:
            Video_clear_text_ram();
            OFerrari_car_ctrl_active = 0;
            OInitEngine_car_increment = 0;
            OFerrari_car_inc_old = 0;
            OStats_time_counter = 5;
            OStats_frame_counter = OStats_frame_reset;
            OSoundInt_queue_sound(0);
            OLogo_enable(sound_FM_RESET);
            Outrun_game_state = GS_LOGO;

        case GS_LOGO:
            OHud_draw_credits();
            OHud_draw_copyright_text();
            OHud_draw_insert_coin();
            OLogo_tick();

            if (OStats_credits)
                Outrun_game_state = GS_INIT_MUSIC;
            else if (Outrun_decrement_timers())
            {
                OLogo_disable();
                Outrun_game_state = GS_INIT; // Resume attract mode
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Music Select Screen
        // ----------------------------------------------------------------------------------------

        case GS_INIT_MUSIC:
            OMusic_enable();
            Outrun_game_state = GS_MUSIC;

        case GS_MUSIC:
            OHud_draw_credits();
            OHud_draw_insert_coin();
            OMusic_check_start(); // Check for start button
            OMusic_tick();
            if (Outrun_decrement_timers())
            {
                OMusic_disable();
                Outrun_game_state = GS_INIT_GAME;
            }
            break;
        // ----------------------------------------------------------------------------------------
        // In-Game
        // ----------------------------------------------------------------------------------------

        case GS_INIT_GAME:
            //ROM:0000B3E8                 move.w  #-1,(ingame_active1).l              ; Denote in-game engine is active
            //ROM:0000B3F0                 clr.l   (prev_game_time).l                  ; Reset overall game time
            //ROM:0000B3F6                 move.w  #-1,(ingame_active2).l
            Video_clear_text_ram();
            OFerrari_car_ctrl_active = 1;
            Outrun_init_jump_table();
            OInitEngine_init(Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL ? Outrun_ttrial.level : 0);
            // Timing Hack to ensure horizon is correct
            // Note that the original code disables the screen, and waits for the second CPU's interrupt instead
            ORoad_tick();
            ORoad_tick();
            ORoad_tick();
            OSoundInt_queue_sound(sound_STOP_CHEERS);
            OSoundInt_queue_sound(sound_VOICE_GETREADY);

            #ifdef COMPILE_SOUND_CODE
            if (OMusic_music_selected >= 0 && OMusic_music_selected <= 2)
            {
                Audio_load_wav(Config_sound.custom_music[OMusic_music_selected].filename);
                OSoundInt_queue_sound(sound_REVS); // queue revs sound manually
            }
            else
            #endif
            OSoundInt_queue_sound(OMusic_music_selected);
            
            if (!Outrun_freeze_timer)
                OStats_time_counter = STATS_TIME[Config_engine.dip_time * 40]; // Set time to begin level with
            else
                OStats_time_counter = 0x30;

            OStats_frame_counter = OStats_frame_reset + 50;
            OStats_credits--;                                   // Update Credits
            OHud_blit_text1(TEXT1_CLEAR_START);
            OHud_blit_text1(TEXT1_CLEAR_CREDITS);
            OSoundInt_queue_sound(sound_INIT_CHEERS);
            Video_enabled = 1;
            Outrun_game_state = GS_START1;
            OHud_draw_main_hud();
            // fall through

        //  Start Game - Car Driving In
        case GS_START1:
        case GS_START2:
            if (--OStats_frame_counter < 0)
            {
                OSoundInt_queue_sound(sound_SIGNAL1);
                OStats_frame_counter = OStats_frame_reset;
                Outrun_game_state++;
            }
            break;

        case GS_START3:
            if (--OStats_frame_counter < 0)
            {
                if (Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL)
                {
                    OHud_clear_timetrial_text();
                }

                OSoundInt_queue_sound(sound_SIGNAL2);
                OSoundInt_queue_sound(sound_STOP_CHEERS);
                OStats_frame_counter = OStats_frame_reset;
                Outrun_game_state++;
            }
            break;

        case GS_INGAME:
            if (Outrun_decrement_timers())
                Outrun_game_state = GS_INIT_GAMEOVER;
            break;

        // ----------------------------------------------------------------------------------------
        // Bonus Mode
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BONUS:
            OStats_frame_counter = OStats_frame_reset;
            OBonus_bonus_control = BONUS_INIT;  // Initialize Bonus Mode Logic
            ORoad_road_load_end   |= BIT_0;             // Instruct CPU 1 to load end road section
            OStats_game_completed |= BIT_0;             // Denote game completed
            OBonus_bonus_timer = 3600;                  // Safety Timer Added in Rev. A Roms
            Outrun_game_state = GS_BONUS;

        case GS_BONUS:
            if (--OBonus_bonus_timer < 0)
            {
                OBonus_bonus_control = BONUS_DISABLE;
                Outrun_game_state = GS_INIT_GAMEOVER;
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Display Game Over Text
        // ----------------------------------------------------------------------------------------
        case GS_INIT_GAMEOVER:
            if (Outrun_cannonball_mode != OUTRUN_MODE_TTRIAL)
            {
                OFerrari_car_ctrl_active = 0; // -1
                OInitEngine_car_increment = 0;
                OFerrari_car_inc_old = 0;
                OStats_time_counter = 3;
                OStats_frame_counter = OStats_frame_reset;
                OHud_blit_text2(TEXT2_GAMEOVER);
            }
            else
            {
                OHud_blit_text_big(7, Outrun_ttrial.new_high_score ? "NEW RECORD" : "BAD LUCK", 0);

                OHud_blit_text1(TEXT1_LAPTIME1);
                OHud_blit_text1(TEXT1_LAPTIME2);
                OHud_draw_lap_timer(0x110554, Outrun_ttrial.best_lap, Outrun_ttrial.best_lap[2]);

                OHud_blit_text_new(9,  14, "OVERTAKES          - ", HUD_GREY);
                OHud_blit_text_new(31, 14, Utils_int_to_string((int) Outrun_ttrial.overtakes), HUD_GREEN);
                OHud_blit_text_new(9,  16, "VEHICLE COLLISIONS - ", HUD_GREY);
                OHud_blit_text_new(31, 16, Utils_int_to_string((int) Outrun_ttrial.vehicle_cols), HUD_GREEN);
                OHud_blit_text_new(9,  18, "CRASHES            - ", HUD_GREY);
                OHud_blit_text_new(31, 18, Utils_int_to_string((int) Outrun_ttrial.crashes), HUD_GREEN);
            }
            OSoundInt_queue_sound(sound_NEW_COMMAND);
            Outrun_game_state = GS_GAMEOVER;

        case GS_GAMEOVER:
            if (Outrun_cannonball_mode == OUTRUN_MODE_ORIGINAL)
            {
                if (Outrun_decrement_timers())
                    Outrun_game_state = GS_INIT_MAP;
            }
            else if (Outrun_cannonball_mode == OUTRUN_MODE_CONT)
            {
                if (Outrun_decrement_timers())
                    Outrun_init_best_outrunners();
            }
            else if (Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL)
            {
                if (Outrun_tick_counter & BIT_4)
                    OHud_blit_text1XY(10, 20, TEXT1_PRESS_START);
                else
                    OHud_blit_text1XY(10, 20, TEXT1_CLEAR_START);

                if (Input_is_pressed(INPUT_START))
                    cannonball_state = STATE_INIT_MENU;
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Display Course Map
        // ----------------------------------------------------------------------------------------
        case GS_INIT_MAP:
            OMap_init();
            OHud_blit_text2(TEXT2_COURSEMAP);
            Outrun_game_state = GS_MAP;
            // fall through

        case GS_MAP:
            break;

        // ----------------------------------------------------------------------------------------
        // Best OutRunners / Score Entry
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BEST2:
            ORoad_set_view_mode(ROAD_VIEW_ORIGINAL, 1);
            // bsr.w   EndGame
            OSprites_disable_sprites();
            OTraffic_disable_traffic();
            // bsr.w   EditJumpTable3
            OSprites_clear_palette_data();
            OLevelObjs_init_hiscore_sprites();
            OCrash_coll_count1   = 0;
            OCrash_coll_count2   = 0;
            OCrash_crash_counter = 0;
            OCrash_skid_counter  = 0;
            OCrash_spin_control1 = 0;
            OFerrari_car_ctrl_active = 0; // -1
            OInitEngine_car_increment = 0;
            OFerrari_car_inc_old = 0;
            OStats_time_counter = 0x30;
            OStats_frame_counter = OStats_frame_reset;
            OHiScore_init();
            OSoundInt_queue_sound(sound_NEW_COMMAND);
            OSoundInt_queue_sound(sound_FM_RESET);
            #ifdef COMPILE_SOUND_CODE
            Audio_clear_wav();
            #endif
            Outrun_game_state = GS_BEST2;
            // fall through

        case GS_BEST2:
            OHiScore_tick(); // Do High Score Logic
            OHud_draw_credits();

            // If countdown has expired
            if (Outrun_decrement_timers())
            {
                //ROM:0000B700                 bclr    #5,(ppi1_value).l                   ; Turn screen off (not activated until PPI written to)
                OFerrari_car_ctrl_active = 1; // 0 : Allow road updates
                Outrun_init_jump_table();
                OInitEngine_init(Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL ? Outrun_ttrial.level : 0);
                //ROM:0000B716                 bclr    #0,(byte_260550).l
                Outrun_game_state = GS_REINIT;          // Reinit game to attract mode
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Reinitialize Game After High Score Entry
        // ----------------------------------------------------------------------------------------
        case GS_REINIT:
            Video_clear_text_ram();
            Outrun_game_state = GS_INIT;
            break;
    }

    OInitEngine_update_road();
    OInitEngine_update_engine();

    // --------------------------------------------------------------------------------------------
    // Debugging Only
    // --------------------------------------------------------------------------------------------
    if (DEBUG_LEVEL)
    {
        if (OInitEngine_rd_split_state != 0)
        {
            if (!fork_chosen)
            {
                if (OInitEngine_camera_x_off < 0)
                    fork_chosen = -1;
                else
                    fork_chosen = 1;        
            }
        }
        else if (fork_chosen)
            fork_chosen = 0;

        // Hack to allow user to choose road fork with left/right
        if (fork_chosen == -1)
        {
            ORoad_road_width_bak = ORoad_road_width >> 16; 
            ORoad_car_x_bak = -ORoad_road_width_bak; 
            OInitEngine_car_x_pos = ORoad_car_x_bak;
        }
        else
        {
            ORoad_road_width_bak = ORoad_road_width >> 16; 
            ORoad_car_x_bak = ORoad_road_width_bak; 
            OInitEngine_car_x_pos = ORoad_car_x_bak;
        } 
    }

}

// Setup Jump Table. Move from ROM to RAM.
//
// Source Address: 0x7E1C
// Input:          Sprite To Copy
// Output:         None
//
// ROM Format [0xF000 - 0xF1F5]
//
// Word 1: Number of entries [7D]
// Long 1: Address 1 (address of jump information)
// ...
// Long x: Address x
//
// Each address in the jump table is a pointer into ROM containing 0x1F words 
// of info (so info is at 0x40 boundary in bytes)
//
// RAM Format[0x61800]
//
// 0x00 byte: If high byte set, take jump
// 0x01 byte: Index number
// 0x02 long: Address to jump to
void Outrun_init_jump_table()
{
    // Reset value to restore car increment to during attract mode
    car_inc_bak = 0;

    OSprites_init();
    if (Outrun_cannonball_mode != OUTRUN_MODE_TTRIAL) 
    {
        OTraffic_init_stage1_traffic();      // Hard coded traffic in right hand lane
        if (TrackLoader_display_start_line)
            OLevelObjs_init_startline_sprites(); // Hard coded start line sprites (not part of level data)
    }
    else if (TrackLoader_display_start_line)
        OLevelObjs_init_timetrial_sprites();

    OTraffic_init();
    OSmoke_init();
    ORoad_init();
    OTiles_init();
    OPalette_init();
    OInputs_init();
    OBonus_init();
    OOutputs_init();

    HWTiles_set_x_clamp(HWTILES_RIGHT);
    HWSprites_set_x_clip(0);
}

// -------------------------------------------------------------------------------
// Decrement Game Time
// 
// Decrements Frame Count, and Overall Time Counter
//
// Returns true if timer expired.
// Source: 0xB736
// -------------------------------------------------------------------------------
uint8_t Outrun_decrement_timers()
{
    // Cheat
    if (Outrun_freeze_timer && Outrun_game_state == GS_INGAME)
        return 0;

    // Correct count-down timer running fast at 1/29th (3%)
    // Fix timer counting extra second
    if (Config_engine.fix_timer)
    {
        if (--OStats_frame_counter > 0)
            return 0;

        OStats_frame_counter = OStats_frame_reset;
        OStats_time_counter  = outils_bcd_sub(1, OStats_time_counter);

        // We need to manually refresh the HUD here to display '0' seconds
        if (OStats_time_counter == 0)
            OHud_draw_timer1(0);

        return (OStats_time_counter == 0);
    }
    else
    {
        if (--OStats_frame_counter >= 0)
            return 0;

        OStats_frame_counter = OStats_frame_reset;
        OStats_time_counter  = outils_bcd_sub(1, OStats_time_counter);
        return (OStats_time_counter < 0);
    }
}

// -------------------------------------------------------------------------------
// CannonBoard: Motor Calibration
// -------------------------------------------------------------------------------

void Outrun_init_motor_calibration()
{
    OTiles_init();
    OPalette_init();
    OInputs_init();
    OOutputs_init();

    HWTiles_set_x_clamp(HWTILES_RIGHT);
    HWSprites_set_x_clip(0);

    OTiles_fill_tilemap_color(0x4F60); // Fill Tilemap Light Blue

    Video_enabled        = 1;
    OSoundInt_has_booted = 1;

    ORoad_init();
    ORoad_horizon_set    = 1;
    ORoad_horizon_base   = -0x3FF;
    Outrun_game_state           = GS_CALIBRATE_MOTOR;


    // Write Palette To RAM
    uint32_t dst = 0x120000;
    const static uint32_t PAL_SERVICE[] = {0xFF, 0xFF00FF, 0xFF00FF, 0xFF0000};
    Video_write_pal32IncP(&dst, PAL_SERVICE[0]);
    Video_write_pal32IncP(&dst, PAL_SERVICE[1]);
    Video_write_pal32IncP(&dst, PAL_SERVICE[2]);
    Video_write_pal32IncP(&dst, PAL_SERVICE[3]);
}

// -------------------------------------------------------------------------------
// Attract Mode Control
// -------------------------------------------------------------------------------

void Outrun_init_attract()
{
    Video_enabled             = 1;
    OSoundInt_has_booted      = 1;
    OFerrari_car_ctrl_active  = 1;
    OFerrari_car_inc_old      = car_inc_bak >> 16;
    OInitEngine_car_increment = car_inc_bak;
    OStats_time_counter       = Config_engine.new_attract ? 0x80 : 0x15;
    OStats_frame_counter      = OStats_frame_reset;
    attract_counter           = 0;
    attract_view              = 0;
    OAttractAI_init();
    Outrun_game_state = Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL ? GS_INIT_MUSIC : GS_ATTRACT;
}

void Outrun_tick_attract()
{
    OHud_draw_credits();
    OHud_draw_copyright_text();
    OHud_draw_insert_coin();

    // Enhanced Attract Mode (Switch Between Views)
    if (Config_engine.new_attract)
    {
        if (++attract_counter > 240)
        {
            uint8_t VIEWS[] = {ROAD_VIEW_ORIGINAL, ROAD_VIEW_ELEVATED, ROAD_VIEW_INCAR};

            attract_counter = 0;
            if (++attract_view > 2)
                attract_view = 0;
            uint8_t snap = VIEWS[attract_view] == ROAD_VIEW_INCAR;
            ORoad_set_view_mode(VIEWS[attract_view], snap);
        }
    }

    if (OStats_credits)
        Outrun_game_state = GS_INIT_MUSIC;

    else if (Outrun_decrement_timers())
    {
        car_inc_bak = OInitEngine_car_increment;
        Outrun_game_state = GS_INIT_BEST1;
    }
}

void Outrun_check_freeplay_start()
{
    if (Config_engine.freeplay)
    {
        if (Input_is_pressed_clear(INPUT_START))
        {
            if (!OStats_credits)
                OStats_credits = 1;
        }
    }
}

// -------------------------------------------------------------------------------
// Best OutRunners Initialization
// -------------------------------------------------------------------------------

void Outrun_init_best_outrunners()
{
    Video_enabled = 0;
    HWSprites_set_x_clip(0); // Stop clipping in wide-screen mode.
    OTiles_fill_tilemap_color(0); // Fill Tilemap Black
    OSprites_disable_sprites();
    ORoad_horizon_base = 0x154;
    OHiScore_setup_pal_best();    // Setup Palettes
    OHiScore_setup_road_best();
    Outrun_game_state = GS_INIT_BEST2;
}

// -------------------------------------------------------------------------------
// Remap ROM addresses and select course.
// -------------------------------------------------------------------------------

void Outrun_select_course(uint8_t jap, uint8_t prototype)
{
    if (jap)
    {
        Roms_rom0p = &Roms_j_rom0;
        Roms_rom1p = &Roms_j_rom1;

        // Main CPU
        Outrun_adr.tiles_def_lookup      = TILES_DEF_LOOKUP_J;
        Outrun_adr.tiles_table           = TILES_TABLE_J;
        Outrun_adr.sprite_master_table   = SPRITE_MASTER_TABLE_J;
        Outrun_adr.sprite_type_table     = SPRITE_TYPE_TABLE_J;
        Outrun_adr.sprite_def_props1     = SPRITE_DEF_PROPS1_J;
        Outrun_adr.sprite_def_props2     = SPRITE_DEF_PROPS2_J;
        Outrun_adr.sprite_cloud          = SPRITE_CLOUD_FRAMES_J;
        Outrun_adr.sprite_minitree       = SPRITE_MINITREE_FRAMES_J;
        Outrun_adr.sprite_grass          = SPRITE_GRASS_FRAMES_J;
        Outrun_adr.sprite_sand           = SPRITE_SAND_FRAMES_J;
        Outrun_adr.sprite_stone          = SPRITE_STONE_FRAMES_J;
        Outrun_adr.sprite_water          = SPRITE_WATER_FRAMES_J;
        Outrun_adr.sprite_ferrari_frames = SPRITE_FERRARI_FRAMES_J;
        Outrun_adr.sprite_skid_frames    = SPRITE_SKID_FRAMES_J;
        Outrun_adr.sprite_pass_frames    = SPRITE_PASS_FRAMES_J;
        Outrun_adr.sprite_pass1_skidl    = SPRITE_PASS1_SKIDL_J;
        Outrun_adr.sprite_pass1_skidr    = SPRITE_PASS1_SKIDR_J;
        Outrun_adr.sprite_pass2_skidl    = SPRITE_PASS2_SKIDL_J;
        Outrun_adr.sprite_pass2_skidr    = SPRITE_PASS2_SKIDR_J;
        Outrun_adr.sprite_crash_spin1    = SPRITE_CRASH_SPIN1_J;
        Outrun_adr.sprite_crash_spin2    = SPRITE_CRASH_SPIN2_J;
        Outrun_adr.sprite_bump_data1     = SPRITE_BUMP_DATA1_J;
        Outrun_adr.sprite_bump_data2     = SPRITE_BUMP_DATA2_J;
        Outrun_adr.sprite_crash_man1     = SPRITE_CRASH_MAN1_J;
        Outrun_adr.sprite_crash_girl1    = SPRITE_CRASH_GIRL1_J;
        Outrun_adr.sprite_crash_flip     = SPRITE_CRASH_FLIP_J;
        Outrun_adr.sprite_crash_flip_m1  = SPRITE_CRASH_FLIP_MAN1_J;
        Outrun_adr.sprite_crash_flip_g1  = SPRITE_CRASH_FLIP_GIRL1_J;
        Outrun_adr.sprite_crash_flip_m2  = SPRITE_CRASH_FLIP_MAN2_J;
        Outrun_adr.sprite_crash_flip_g2  = SPRITE_CRASH_FLIP_GIRL2_J;
        Outrun_adr.sprite_crash_man2     = SPRITE_CRASH_MAN2_J;
        Outrun_adr.sprite_crash_girl2    = SPRITE_CRASH_GIRL2_J;
        Outrun_adr.smoke_data            = SMOKE_DATA_J;
        Outrun_adr.spray_data            = SPRAY_DATA_J;
        Outrun_adr.anim_ferrari_frames   = ANIM_FERRARI_FRAMES_J;
        Outrun_adr.anim_endseq_obj1      = ANIM_ENDSEQ_OBJ1_J;
        Outrun_adr.anim_endseq_obj2      = ANIM_ENDSEQ_OBJ2_J;
        Outrun_adr.anim_endseq_obj3      = ANIM_ENDSEQ_OBJ3_J;
        Outrun_adr.anim_endseq_obj4      = ANIM_ENDSEQ_OBJ4_J;
        Outrun_adr.anim_endseq_obj5      = ANIM_ENDSEQ_OBJ5_J;
        Outrun_adr.anim_endseq_obj6      = ANIM_ENDSEQ_OBJ6_J;
        Outrun_adr.anim_endseq_obj7      = ANIM_ENDSEQ_OBJ7_J;
        Outrun_adr.anim_endseq_obj8      = ANIM_ENDSEQ_OBJ8_J;
        Outrun_adr.anim_endseq_objA      = ANIM_ENDSEQ_OBJA_J;
        Outrun_adr.anim_endseq_objB      = ANIM_ENDSEQ_OBJB_J;
        Outrun_adr.anim_end_table        = ANIM_END_TABLE_J;
        Outrun_adr.shadow_data           = SPRITE_SPRITES_SHADOW_DATA_J;
        Outrun_adr.shadow_frames         = SPRITE_SHDW_FRAMES_J;
        Outrun_adr.sprite_shadow_small   = SPRITE_SHDW_SMALL_J;
        Outrun_adr.sprite_logo_bg        = SPRITE_LOGO_BG_J;
        Outrun_adr.sprite_logo_car       = SPRITE_LOGO_CAR_J;
        Outrun_adr.sprite_logo_bird1     = SPRITE_LOGO_BIRD1_J;
        Outrun_adr.sprite_logo_bird2     = SPRITE_LOGO_BIRD2_J;
        Outrun_adr.sprite_logo_base      = SPRITE_LOGO_BASE_J;
        Outrun_adr.sprite_logo_text      = SPRITE_LOGO_TEXT_J;
        Outrun_adr.sprite_logo_palm1     = SPRITE_LOGO_PALM1_J;
        Outrun_adr.sprite_logo_palm2     = SPRITE_LOGO_PALM2_J;
        Outrun_adr.sprite_logo_palm3     = SPRITE_LOGO_PALM3_J;
        Outrun_adr.sprite_fm_left        = SPRITE_FM_LEFT_J;
        Outrun_adr.sprite_fm_centre      = SPRITE_FM_CENTRE_J;
        Outrun_adr.sprite_fm_right       = SPRITE_FM_RIGHT_J;
        Outrun_adr.sprite_dial_left      = SPRITE_DIAL_LEFT_J;
        Outrun_adr.sprite_dial_centre    = SPRITE_DIAL_CENTRE_J;
        Outrun_adr.sprite_dial_right     = SPRITE_DIAL_RIGHT_J;
        Outrun_adr.sprite_eq             = SPRITE_EQ_J;
        Outrun_adr.sprite_radio          = SPRITE_RADIO_J;
        Outrun_adr.sprite_hand_left      = SPRITE_HAND_LEFT_J;
        Outrun_adr.sprite_hand_centre    = SPRITE_HAND_CENTRE_J;
        Outrun_adr.sprite_hand_right     = SPRITE_HAND_RIGHT_J;
        Outrun_adr.sprite_coursemap_top  = SPRITE_COURSEMAP_TOP_J;
        Outrun_adr.sprite_coursemap_bot  = SPRITE_COURSEMAP_BOT_J;
        Outrun_adr.sprite_coursemap_end  = SPRITE_COURSEMAP_END_J;
        Outrun_adr.sprite_minicar_right  = SPRITE_MINICAR_RIGHT_J;
        Outrun_adr.sprite_minicar_up     = SPRITE_MINICAR_UP_J;
        Outrun_adr.sprite_minicar_down   = SPRITE_MINICAR_DOWN_J;
        Outrun_adr.anim_seq_flag         = ANIM_SEQ_FLAG_J;
        Outrun_adr.anim_ferrari_curr     = ANIM_FERRARI_CURR_J;
        Outrun_adr.anim_ferrari_next     = ANIM_FERRARI_NEXT_J;
        Outrun_adr.anim_pass1_curr       = ANIM_PASS1_CURR_J;
        Outrun_adr.anim_pass1_next       = ANIM_PASS1_NEXT_J;
        Outrun_adr.anim_pass2_curr       = ANIM_PASS2_CURR_J;
        Outrun_adr.anim_pass2_next       = ANIM_PASS2_NEXT_J;
        Outrun_adr.traffic_props         = TRAFFIC_PROPS_J;
        Outrun_adr.traffic_data          = TRAFFIC_DATA_J;
        Outrun_adr.sprite_porsche        = SPRITE_PORSCHE_J;
        Outrun_adr.sprite_coursemap      = SPRITE_COURSEMAP_J;
        Outrun_adr.road_seg_table        = ROAD_SEG_TABLE_J;
        Outrun_adr.road_seg_end          = ROAD_SEG_TABLE_END_J;
        Outrun_adr.road_seg_split        = ROAD_SEG_TABLE_SPLIT_J;
        
        // Sub CPU
        Outrun_adr.road_height_lookup    = ROAD_HEIGHT_LOOKUP_J;
    }
    else
    {
        Roms_rom0p = &Roms_rom0;
        Roms_rom1p = &Roms_rom1;

        // Main CPU
        Outrun_adr.tiles_def_lookup      = TILES_DEF_LOOKUP;
        Outrun_adr.tiles_table           = TILES_TABLE;
        Outrun_adr.sprite_master_table   = SPRITE_MASTER_TABLE;
        Outrun_adr.sprite_type_table     = SPRITE_TYPE_TABLE;
        Outrun_adr.sprite_def_props1     = SPRITE_DEF_PROPS1;
        Outrun_adr.sprite_def_props2     = SPRITE_DEF_PROPS2;
        Outrun_adr.sprite_cloud          = SPRITE_CLOUD_FRAMES;
        Outrun_adr.sprite_minitree       = SPRITE_MINITREE_FRAMES;
        Outrun_adr.sprite_grass          = SPRITE_GRASS_FRAMES;
        Outrun_adr.sprite_sand           = SPRITE_SAND_FRAMES;
        Outrun_adr.sprite_stone          = SPRITE_STONE_FRAMES;
        Outrun_adr.sprite_water          = SPRITE_WATER_FRAMES;
        Outrun_adr.sprite_ferrari_frames = SPRITE_FERRARI_FRAMES;
        Outrun_adr.sprite_skid_frames    = SPRITE_SKID_FRAMES;
        Outrun_adr.sprite_pass_frames    = SPRITE_PASS_FRAMES;
        Outrun_adr.sprite_pass1_skidl    = SPRITE_PASS1_SKIDL;
        Outrun_adr.sprite_pass1_skidr    = SPRITE_PASS1_SKIDR;
        Outrun_adr.sprite_pass2_skidl    = SPRITE_PASS2_SKIDL;
        Outrun_adr.sprite_pass2_skidr    = SPRITE_PASS2_SKIDR;
        Outrun_adr.sprite_crash_spin1    = SPRITE_CRASH_SPIN1;
        Outrun_adr.sprite_crash_spin2    = SPRITE_CRASH_SPIN2;
        Outrun_adr.sprite_bump_data1     = SPRITE_BUMP_DATA1;
        Outrun_adr.sprite_bump_data2     = SPRITE_BUMP_DATA2;
        Outrun_adr.sprite_crash_man1     = SPRITE_CRASH_MAN1;
        Outrun_adr.sprite_crash_girl1    = SPRITE_CRASH_GIRL1;
        Outrun_adr.sprite_crash_flip     = SPRITE_CRASH_FLIP;
        Outrun_adr.sprite_crash_flip_m1  = SPRITE_CRASH_FLIP_MAN1;
        Outrun_adr.sprite_crash_flip_g1  = SPRITE_CRASH_FLIP_GIRL1;
        Outrun_adr.sprite_crash_flip_m2  = SPRITE_CRASH_FLIP_MAN2;
        Outrun_adr.sprite_crash_flip_g2  = SPRITE_CRASH_FLIP_GIRL2;
        Outrun_adr.sprite_crash_man2     = SPRITE_CRASH_MAN2;
        Outrun_adr.sprite_crash_girl2    = SPRITE_CRASH_GIRL2;
        Outrun_adr.smoke_data            = SMOKE_DATA;
        Outrun_adr.spray_data            = SPRAY_DATA;
        Outrun_adr.shadow_data           = SPRITE_SPRITES_SHADOW_DATA;
        Outrun_adr.shadow_frames         = SPRITE_SHDW_FRAMES;
        Outrun_adr.sprite_shadow_small   = SPRITE_SHDW_SMALL;
        Outrun_adr.sprite_logo_bg        = SPRITE_LOGO_BG;
        Outrun_adr.sprite_logo_car       = SPRITE_LOGO_CAR;
        Outrun_adr.sprite_logo_bird1     = SPRITE_LOGO_BIRD1;
        Outrun_adr.sprite_logo_bird2     = SPRITE_LOGO_BIRD2;
        Outrun_adr.sprite_logo_base      = SPRITE_LOGO_BASE;
        Outrun_adr.sprite_logo_text      = SPRITE_LOGO_TEXT;
        Outrun_adr.sprite_logo_palm1     = SPRITE_LOGO_PALM1;
        Outrun_adr.sprite_logo_palm2     = SPRITE_LOGO_PALM2;
        Outrun_adr.sprite_logo_palm3     = SPRITE_LOGO_PALM3;
        Outrun_adr.sprite_fm_left        = SPRITE_FM_LEFT;
        Outrun_adr.sprite_fm_centre      = SPRITE_FM_CENTRE;
        Outrun_adr.sprite_fm_right       = SPRITE_FM_RIGHT;
        Outrun_adr.sprite_dial_left      = SPRITE_DIAL_LEFT;
        Outrun_adr.sprite_dial_centre    = SPRITE_DIAL_CENTRE;
        Outrun_adr.sprite_dial_right     = SPRITE_DIAL_RIGHT;
        Outrun_adr.sprite_eq             = SPRITE_EQ;
        Outrun_adr.sprite_radio          = SPRITE_RADIO;
        Outrun_adr.sprite_hand_left      = SPRITE_HAND_LEFT;
        Outrun_adr.sprite_hand_centre    = SPRITE_HAND_CENTRE;
        Outrun_adr.sprite_hand_right     = SPRITE_HAND_RIGHT;
        Outrun_adr.sprite_coursemap_top  = SPRITE_COURSEMAP_TOP;
        Outrun_adr.sprite_coursemap_bot  = SPRITE_COURSEMAP_BOT;
        Outrun_adr.sprite_coursemap_end  = SPRITE_COURSEMAP_END;
        Outrun_adr.sprite_minicar_right  = SPRITE_MINICAR_RIGHT;
        Outrun_adr.sprite_minicar_up     = SPRITE_MINICAR_UP;
        Outrun_adr.sprite_minicar_down   = SPRITE_MINICAR_DOWN;
        Outrun_adr.anim_seq_flag         = ANIM_SEQ_FLAG;
        Outrun_adr.anim_ferrari_curr     = ANIM_FERRARI_CURR;
        Outrun_adr.anim_ferrari_next     = ANIM_FERRARI_NEXT;
        Outrun_adr.anim_pass1_curr       = ANIM_PASS1_CURR;
        Outrun_adr.anim_pass1_next       = ANIM_PASS1_NEXT;
        Outrun_adr.anim_pass2_curr       = ANIM_PASS2_CURR;
        Outrun_adr.anim_pass2_next       = ANIM_PASS2_NEXT;
        Outrun_adr.anim_ferrari_frames   = ANIM_FERRARI_FRAMES;
        Outrun_adr.anim_endseq_obj1      = ANIM_ENDSEQ_OBJ1;
        Outrun_adr.anim_endseq_obj2      = ANIM_ENDSEQ_OBJ2;
        Outrun_adr.anim_endseq_obj3      = ANIM_ENDSEQ_OBJ3;
        Outrun_adr.anim_endseq_obj4      = ANIM_ENDSEQ_OBJ4;
        Outrun_adr.anim_endseq_obj5      = ANIM_ENDSEQ_OBJ5;
        Outrun_adr.anim_endseq_obj6      = ANIM_ENDSEQ_OBJ6;
        Outrun_adr.anim_endseq_obj7      = ANIM_ENDSEQ_OBJ7;
        Outrun_adr.anim_endseq_obj8      = ANIM_ENDSEQ_OBJ8;
        Outrun_adr.anim_endseq_objA      = ANIM_ENDSEQ_OBJA;
        Outrun_adr.anim_endseq_objB      = ANIM_ENDSEQ_OBJB;
        Outrun_adr.anim_end_table        = ANIM_END_TABLE;
        Outrun_adr.shadow_data           = SPRITE_SPRITES_SHADOW_DATA;
        Outrun_adr.shadow_frames         = SPRITE_SHDW_FRAMES;
        Outrun_adr.sprite_shadow_small   = SPRITE_SHDW_SMALL;
        Outrun_adr.traffic_props         = TRAFFIC_PROPS;
        Outrun_adr.traffic_data          = TRAFFIC_DATA;
        Outrun_adr.sprite_porsche        = SPRITE_PORSCHE;
        Outrun_adr.sprite_coursemap      = SPRITE_COURSEMAP;
        Outrun_adr.road_seg_table        = ROAD_SEG_TABLE;
        Outrun_adr.road_seg_end          = ROAD_SEG_TABLE_END;
        Outrun_adr.road_seg_split        = ROAD_SEG_TABLE_SPLIT;

        // Sub CPU
        Outrun_adr.road_height_lookup    = ROAD_HEIGHT_LOOKUP;
    }

    // Initialize tracks AFTER setting up addresses
    TrackLoader_init(jap);

    // Use Prototype Coconut Beach Track
    TrackLoader_stage_data[0] = prototype ? 0x3A : 0x3C;
}
