/***************************************************************************
    Core Game Engine Routines.
    
    - The main loop which advances the level onto the next segment.
    - Code to directly control the road hardware. For example, the road
      split and bonus points routines.
    - Code to determine whether to initialize certain game modes
      (Crash state, Bonus points, road split state) 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "trackloader.h"
#include <stdint.h>
#include "engine/oanimseq.h"
#include "engine/obonus.h"
#include "engine/ocrash.h"
#include "engine/oferrari.h"
#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/olevelobjs.h"
#include "engine/omusic.h"
#include "engine/ooutputs.h"
#include "engine/ostats.h"
#include "engine/outils.h"
#include "engine/opalette.h"
#include "engine/osmoke.h"
#include "engine/otiles.h"
#include "engine/otraffic.h"
#include "globals.h"
#include "engine/oinitengine.h"


int16_t OInitEngine_camera_x_off;
uint8_t OInitEngine_ingame_engine;
int16_t OInitEngine_ingame_counter;
uint16_t OInitEngine_rd_split_state;
int16_t OInitEngine_road_type;
int16_t OInitEngine_road_type_next;
uint8_t OInitEngine_end_stage_props;
uint32_t OInitEngine_car_increment; // NEEDS REPLACING. Implementing here as a quick hack so routine works
int16_t OInitEngine_car_x_pos;
int16_t OInitEngine_car_x_old;
int8_t OInitEngine_checkpoint_marker;
int16_t OInitEngine_road_curve;
int16_t OInitEngine_road_curve_next;
int8_t OInitEngine_road_remove_split;
int8_t OInitEngine_route_selected;
int16_t OInitEngine_change_width;

// Road width at merge point
const static uint16_t RD_WIDTH_MERGE = 0xD4;

// Road width of next section
int16_t road_width_next;

// Speed at which adjustment to road section occurs
int16_t road_width_adj;

int16_t granular_rem;

uint16_t pos_fine_old;

// Road Original Width. Used when adjustments are being made during road split, and end sequence initialisation
int16_t road_width_orig;

// Used by road merge logic, to control width of road
int16_t road_width_merge;
    
// ------------------------------------------------------------------------
// Route Information
// ------------------------------------------------------------------------

// Used as part of road split code.
// 0 = Route Info Not Updated
// 1 = Route Info Updated
int8_t route_updated;
    
void OInitEngine_setup_stage1();
void OInitEngine_check_road_split();
void OInitEngine_check_stage();
void OInitEngine_init_split1();
void OInitEngine_init_split2();
void OInitEngine_init_split3();
void OInitEngine_init_split4();
void OInitEngine_init_split5();
void OInitEngine_init_split6();
void OInitEngine_init_split7();
void OInitEngine_init_split9();
void OInitEngine_init_split10();
void OInitEngine_bonus1();
void OInitEngine_bonus2();
void OInitEngine_bonus3();
void OInitEngine_bonus4();
void OInitEngine_bonus5();
void OInitEngine_bonus6();
void OInitEngine_reload_stage1();
void OInitEngine_init_split_next_level();
void OInitEngine_test_bonus_mode(uint8_t);

// Continuous Mode Level Ordering
const static uint8_t CONTINUOUS_LEVELS[] = {0, 0x8, 0x9, 0x10, 0x11, 0x12, 0x18, 0x19, 0x1A, 0x1B, 0x20, 0x21, 0x22, 0x23, 0x24};


// Source: 0x8360
void OInitEngine_init(int8_t level)
{
    OStats_game_completed  = 0;

    OInitEngine_ingame_engine          = 0;
    OInitEngine_ingame_counter         = 0;
    OStats_cur_stage       = 0;
    ORoad_stage_lookup_off = level ? level : 0;
    OInitEngine_rd_split_state         = INITENGINE_SPLIT_NONE;
    OInitEngine_road_type              = INITENGINE_ROAD_NOCHANGE;
    OInitEngine_road_type_next         = INITENGINE_ROAD_NOCHANGE;
    OInitEngine_end_stage_props        = 0;
    OInitEngine_car_increment          = 0;
    OInitEngine_car_x_pos              = 0;
    OInitEngine_car_x_old              = 0;
    OInitEngine_checkpoint_marker      = 0;
    OInitEngine_road_curve             = 0;
    OInitEngine_road_curve_next        = 0;
    OInitEngine_road_remove_split      = 0;
    OInitEngine_route_selected         = 0;
    
    road_width_next        = 0;
    road_width_adj         = 0;
    OInitEngine_change_width           = 0;
    granular_rem           = 0;
    pos_fine_old           = 0;
    road_width_orig        = 0;
    road_width_merge       = 0;
    route_updated          = 0;

	OInitEngine_init_road_seg_master();

    // Road Renderer: Setup correct stage address 
    if (level)
        TrackLoader_init_path(ORoad_stage_lookup_off);

	OPalette_setup_sky_palette();
	OPalette_setup_ground_color();
	OPalette_setup_road_centre();
	OPalette_setup_road_stripes();
	OPalette_setup_road_side();
	OPalette_setup_road_colour();   
    OTiles_setup_palette_hud();                     // Init Default Palette for HUD
    OSprites_copy_palette_data();                   // Copy Palette Data to RAM
    OTiles_setup_palette_tilemap();                 // Setup Palette For Tilemap
    OInitEngine_setup_stage1();                                 // Setup Misc stuff relating to Stage 1
    OTiles_reset_tiles_pal();                       // Reset Tiles, Palette And Road Split Data
    OCrash_clear_crash_state();

    // The following is set up specifically for time trial mode
    if (level)
    {  
        OTiles_init_tilemap_palette(level);
        ORoad_road_ctrl  = ROAD_BOTH_P0;
        ORoad_road_width = RD_WIDTH_MERGE;        // Setup a default road width
    }

    OSoundInt_reset();
}

// Source: 0x8402
void OInitEngine_setup_stage1()
{
    ORoad_road_width = 0x1C2 << 16;     // Force display of two roads at start
    OStats_score = 0;
    OStats_clear_stage_times();
    OFerrari_reset_car();               // Reset Car Speed/Rev Values
    OOutputs_set_digital(OUTPUTS_D_EXT_MUTE);
    OOutputs_set_digital(OUTPUTS_D_SOUND);
    OSoundInt_engine_data[sound_ENGINE_VOL] = 0x3F;
    OStats_extend_play_timer = 0;
    OInitEngine_checkpoint_marker = 0;              // Denote not past checkpoint marker
    OTraffic_set_max_traffic();         // Set Number Of Enemy Cars Based On Dip Switches
    OStats_clear_route_info();
    OSmoke_setup_smoke_sprite(1);
}

// Initialise Master Segment Address For Stage
//
// 1. Read Internal Stage Number from Stage Data Table (Using the lookup offset)
// 2. Load the master address, using the stage number as an index.
//
// Source: 0x8C80

void OInitEngine_init_road_seg_master()
{
    TrackLoader_init_track(ORoad_stage_lookup_off);
}

//
// Check Road Width
// Source: B85A
//
// Potentially Update Width Of Road
//
// ADDRESS 2 - Road Segment Data [8 byte boundaries]:
//
// Word 1 [+0]: Segment Position
// Word 2 [+2]: Set = Denotes Road Height Info. Unset = Denotes Road Width
// Word 3 [+4]: Segment Road Width / Segment Road Height Index
// Word 4 [+6]: Segment Width Adjustment SIGNED (Speed at which width is adjusted)

void OInitEngine_update_road()
{
    OInitEngine_check_road_split(); // Check/Process road split if necessary
    uint32_t addr = 0;
    uint16_t d0 = TrackLoader_read_width_height(&addr);
    // Update next road section
    if (d0 <= ORoad_road_pos >> 16)
    {
        // Skip road width adjustment if set and adjust height
        if (TrackLoader_read_width_height(&addr) == 0)
        {
            // ROM:0000B8A6 skip_next_width
            if (ORoad_height_lookup == 0)
                 ORoad_height_lookup = TrackLoader_read_width_height(&addr); // Set new height lookup section
        }
        else
        {
            // ROM:0000B87A
            int16_t width  = TrackLoader_read_width_height(&addr); // Segment road width
            int16_t change = TrackLoader_read_width_height(&addr); // Segment adjustment speed

            if (width != (int16_t) (ORoad_road_width >> 16))
            {
                if (width <= (int16_t) (ORoad_road_width >> 16))
                    change = -change;

                road_width_next = width;
                road_width_adj  = change;
                OInitEngine_change_width = -1; // Denote road width is changing
            }
        }
        TrackLoader_wh_offset += 8;
    }

    // ROM:0000B8BC set_road_width    
    // Width of road is changing & car is moving
    if (OInitEngine_change_width != 0 && OInitEngine_car_increment >> 16 != 0)
    {
        int32_t d0 = ((OInitEngine_car_increment >> 16) * road_width_adj) << 4;
        ORoad_road_width += d0; // add long here
        if (d0 > 0)
        {    
            if (road_width_next < (int16_t) (ORoad_road_width >> 16))
            {
                OInitEngine_change_width = 0;
                ORoad_road_width = road_width_next << 16;
            }
        }
        else
        {
            if (road_width_next >= (int16_t) (ORoad_road_width >> 16))
            {
                OInitEngine_change_width = 0;
                ORoad_road_width = road_width_next << 16;
            }
        }
    }
    // ------------------------------------------------------------------------------------------------
    // ROAD SEGMENT FORMAT
    //
    // Each segment of road is 6 bytes in memory, consisting of 3 words
    // Each road segment is a significant length of road btw :)
    //
    // ADDRESS 3 - Road Segment Data [6 byte boundaries]
    //
    // Word 1 [+0]: Segment Position (used with 0x260006 car position)
    // Word 2 [+2]: Segment Road Curve
    // Word 3 [+4]: Segment Road type (1 = Straight, 2 = Right Bend, 3 = Left Bend)
    //
    // 60a08 = address of next road segment? (e.g. A0 = 0x0001DD86)
    // ------------------------------------------------------------------------------------------------

    // ROM:0000B91C set_road_type: 

    int16_t segment_pos = TrackLoader_read_curve(0);

    if (segment_pos != -1)
    {
        int16_t d1 = segment_pos - 0x3C;

        if (d1 <= (int16_t) (ORoad_road_pos >> 16))
        {
            OInitEngine_road_curve_next = TrackLoader_read_curve(2);
            OInitEngine_road_type_next  = TrackLoader_read_curve(4);
        }

        if (segment_pos <= (int16_t) (ORoad_road_pos >> 16))
        {
            OInitEngine_road_curve = TrackLoader_read_curve(2);
            OInitEngine_road_type  = TrackLoader_read_curve(4);
            TrackLoader_curve_offset += 6;
            OInitEngine_road_type_next = 0;
            OInitEngine_road_curve_next = 0;
        }
    }
}

// Carries on from the above in the original code
void OInitEngine_update_engine()
{   
    // ------------------------------------------------------------------------
    // TILE MAP OFFSETS
    // ROM:0000B986 setup_shadow_offset:
    // Setup the shadow offset based on how much we've scrolled left/right. Lovely and subtle!
    // ------------------------------------------------------------------------

    OInitEngine_update_shadow_offset();

    // ------------------------------------------------------------------------
    // Main Car Logic Block
    // ------------------------------------------------------------------------

    OFerrari_move();

    if (OFerrari_car_ctrl_active)
    {
        OFerrari_set_curve_adjust();
        OFerrari_set_ferrari_x();
        OFerrari_do_skid();
        OFerrari_check_wheels();
        OFerrari_set_ferrari_bounds();
    }

    OFerrari_do_sound_score_slip();

    // ------------------------------------------------------------------------
    // Setup New Sprite Scroll Speed. Based On Granular Difference.
    // ------------------------------------------------------------------------
    OInitEngine_set_granular_position();
    OInitEngine_set_fine_position();

    // Draw Speed & Hud Stuff
    if (Outrun_game_state >= GS_START1 && Outrun_game_state <= GS_BONUS)
    {
        // Convert & Blit Car Speed
        OHud_blit_speed(0x110CB6, OInitEngine_car_increment >> 16);
        OHud_blit_text1(HUD_KPH1);
        OHud_blit_text1(HUD_KPH2);

        // Blit High/Low Gear
        if (Config_controls.gear == CONTROLS_GEAR_BUTTON )
        {
            if (OInputs_gear)
                OHud_blit_text_new(9, 26, "H", HUD_GREY);
            else
                OHud_blit_text_new(9, 26, "L", HUD_GREY);
        }

        if (Config_engine.layout_debug)
            OHud_draw_debug_info(ORoad_road_pos, ORoad_height_lookup_wrk, TrackLoader_read_sprite_pattern_index());
    }

    if (OLevelObjs_spray_counter > 0)
        OLevelObjs_spray_counter--;

    if (OLevelObjs_sprite_collision_counter > 0)
        OLevelObjs_sprite_collision_counter--;

    OPalette_setup_sky_cycle();
}

void OInitEngine_update_shadow_offset()
{
    int16_t shadow_off = ORoad_tilemap_h_target & 0x3FF;
    if (shadow_off > 0x1FF)
        shadow_off = -shadow_off + 0x3FF;
    shadow_off >>= 2;
    if (ORoad_tilemap_h_target & BIT_A)
        shadow_off = -shadow_off; // reverse direction of shadow
    OSprites_shadow_offset = shadow_off;
}

// Check for Road Split
//
// - Checks position in level and determine whether to init road split
// - Processes road split if initialized
//
// Source: 868E
void OInitEngine_check_road_split()
{
    // Check whether to initialize the next level
    OStats_init_next_level();

    switch (OInitEngine_rd_split_state)
    {
        // State 0: No road split. Check current road position with ROAD_END.
        case INITENGINE_SPLIT_NONE:
            if (ORoad_road_pos >> 16 <= ROAD_END) return; 
            OInitEngine_check_stage(); // Do Split - Split behaviour depends on stage
            break;

        // State 1: (Does this ever need to be called directly?)
        case INITENGINE_SPLIT_INIT:
            OInitEngine_init_split1();
            break;
        
        // State 2: Road Split
        case INITENGINE_SPLIT_CHOICE1:
            if (ORoad_road_pos >> 16 >= 0x3F)  
                OInitEngine_init_split2();            
            break;

        // State 3: Beginning of split. User must choose.
        case INITENGINE_SPLIT_CHOICE2:
            OInitEngine_init_split2();
            break;

        // State 4: Road physically splits into two individual roads
        case 4:
            OInitEngine_init_split3();
            break;

        // State 5: Road fully split. Init remove other road
        case 5:
            if (!OInitEngine_road_curve)
                OInitEngine_rd_split_state = 6; // and fall through
            else
                break;
        
        // State 6: Road split. Only one road drawn.
        case 6:
            OInitEngine_init_split5();
            break;

        // Stage 7
        case 7:
            OInitEngine_init_split6();
            break;

        // State 8: Init Road Merge before checkpoint sign
        case 8:
            OTraffic_traffic_split = -1;
            OInitEngine_rd_split_state = 9;
            break;

        // State 9: Road Merge before checkpoint sign
        case 9:
            OTraffic_traffic_split = 0;
        case 0x0A:
            OInitEngine_init_split9();
            break;

        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            OInitEngine_init_split10();
            break;
        
        // Init Bonus Sequence
        case 0x10:
            OInitEngine_init_bonus(ORoad_stage_lookup_off - 0x20);
            break;

        case 0x11:
            OInitEngine_bonus1();
            break;

        case 0x12:
            OInitEngine_bonus2();
            break;

        case 0x13:
            OInitEngine_bonus3();
            break;

        case 0x14:
            OInitEngine_bonus4();
            break;

        case 0x15:
            OInitEngine_bonus5();
            break;

        case 0x16:
        case 0x17:
        case 0x18:
            OInitEngine_bonus6();
            break;
    }
}

// ------------------------------------------------------------------------------------------------
// Check Stage Info To Determine What To Do With Road
//
// Stage 1-4: Init Road Split
// Stage 5: Init Bonus
// Stage 5 ATTRACT: Loop to Stage 1
// ------------------------------------------------------------------------------------------------
void OInitEngine_check_stage()
{
    // Time Trial Mode
    if (Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL)
    {
        // Store laptime and reset
        uint8_t* laptimes = Outrun_ttrial.laptimes[Outrun_ttrial.current_lap];

        laptimes[0] = OStats_stage_times[0][0];
        laptimes[1] = OStats_stage_times[0][1];
        laptimes[2] = OStats_stage_times[0][2];
        OStats_stage_times[0][0] = 
        OStats_stage_times[0][1] = 
        OStats_stage_times[0][2] = 0;

        // Check for new best laptime
        int16_t counter = OStats_stage_counters[Outrun_ttrial.current_lap];
        if (counter < Outrun_ttrial.best_lap_counter)
        {
            Outrun_ttrial.best_lap_counter = counter;
            Outrun_ttrial.best_lap[0] = laptimes[0];
            Outrun_ttrial.best_lap[1] = laptimes[1];
            Outrun_ttrial.best_lap[2] = OStats_lap_ms[laptimes[2]];

            // Draw best laptime
            OStats_extend_play_timer = 0x80;
            OHud_blit_text1(TEXT1_LAPTIME1);
            OHud_blit_text1(TEXT1_LAPTIME2);
            OHud_draw_lap_timer(0x110554, laptimes, OStats_lap_ms[laptimes[2]]);

            Outrun_ttrial.new_high_score = 1;
        }

        if (Outrun_game_state == GS_INGAME)
        {
            // More laps to go, loop the course
            if (++Outrun_ttrial.current_lap < Outrun_ttrial.laps)
            {
                // Update lap number
                ORoad_road_pos = 0;
                ORoad_tilemap_h_target = 0;
                OInitEngine_init_road_seg_master();
            }
            else 
            {
                // Set correct finish segment for final 5 stages, otherwise just default to first one.
                ORoad_stage_lookup_off = ORoad_stage_lookup_off < 0x20 ? 0x20 : ORoad_stage_lookup_off;
                OStats_time_counter = 1;
                OInitEngine_init_bonus(ORoad_stage_lookup_off - 0x20);
            }
        }
    }
    else if (Outrun_cannonball_mode == OUTRUN_MODE_CONT)
    {
        ORoad_road_pos         = 0;
        ORoad_tilemap_h_target = 0;
        
        if ((OStats_cur_stage + 1) == 15)
        {
            if (Outrun_game_state == GS_INGAME)
                OInitEngine_init_bonus(outils_random() % 5);
            else
                OInitEngine_reload_stage1();
        }
        else
        {
            ORoad_stage_lookup_off = CONTINUOUS_LEVELS[++OStats_cur_stage];
            OInitEngine_init_road_seg_master();
            OSprites_clear_palette_data();

            // Init next tilemap
            OTiles_set_vertical_swap(); // Tell tilemap to v-scroll off/on

            // Reload smoke data
            OSmoke_setup_smoke_sprite(1);

            // Update palette
            OInitEngine_end_stage_props |= BIT_1; // Don't bump stage offset when fetching next palette
            OInitEngine_end_stage_props |= BIT_2;
            OPalette_pal_manip_ctrl = 1;
            OPalette_setup_sky_change();
            
            // Denote Checkpoint Passed
            OInitEngine_checkpoint_marker = -1;

            // Cycle Music every 5 stages
            if (Outrun_game_state == GS_INGAME)
            {
                if (OStats_cur_stage == 5 || OStats_cur_stage == 10)
                {
                    switch (OMusic_music_selected)
                    {
                        // Cycle in-built sounds
                        case SOUND_MUSIC_BREEZE:
                            OMusic_music_selected = SOUND_MUSIC_SPLASH;
                            OSoundInt_queue_sound(SOUND_MUSIC_SPLASH2); // Play without rev effect
                            break;
                        case SOUND_MUSIC_SPLASH:
                            OMusic_music_selected = SOUND_MUSIC_MAGICAL;
                            OSoundInt_queue_sound(SOUND_MUSIC_MAGICAL2); // Play without rev effect
                            break;
                        case SOUND_MUSIC_MAGICAL:
                            OMusic_music_selected = SOUND_MUSIC_BREEZE;
                            OSoundInt_queue_sound(SOUND_MUSIC_BREEZE2); // Play without rev effect
                            break;
                    }                 
                }
            }              
        }
    }

    // Stages 0-4, do road split
    else if (OStats_cur_stage <= 3)
    {
        OInitEngine_rd_split_state = INITENGINE_SPLIT_INIT;
        OInitEngine_init_split1();
    }
    // Stage 5: Init Bonus
    else if (Outrun_game_state == GS_INGAME)
    {
        OInitEngine_init_bonus(ORoad_stage_lookup_off - 0x20);
    }
    // Stage 5 Attract Mode: Reload Stage 1
    else
    {
        OInitEngine_reload_stage1();
    }
}

void OInitEngine_reload_stage1()
{
    ORoad_road_pos         = 0;
    ORoad_tilemap_h_target = 0;
    OStats_cur_stage       = -1;
    ORoad_stage_lookup_off = -8;

    OStats_clear_route_info();

    OInitEngine_end_stage_props |= BIT_1; // Loop back to stage 1 (Used by tilemap code)
    OInitEngine_end_stage_props |= BIT_2;
    OInitEngine_end_stage_props |= BIT_3;
    OSmoke_setup_smoke_sprite(1);
    OInitEngine_init_split_next_level();
}

// ------------------------------------------------------------------------------------------------
// Road Split 1
// Init Road Split & Begin Road Split
// Called When We're Not On The Final Stage
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split1()
{
    OInitEngine_rd_split_state = INITENGINE_SPLIT_CHOICE1;

    ORoad_road_load_split  = -1;
    ORoad_road_ctrl        = ROAD_BOTH_P0_INV; // Both Roads (Road 0 Priority) (Road Split. Invert Road 0)
    road_width_orig        = ORoad_road_width >> 16;
    ORoad_road_pos         = 0;
    ORoad_tilemap_h_target = 0;
    TrackLoader_init_track_split();
}

// ------------------------------------------------------------------------------------------------
// Road Split 2: Beginning of split. User must choose.
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split2()
{
    OInitEngine_rd_split_state = INITENGINE_SPLIT_CHOICE2;

    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((ORoad_road_pos >> 16) - 0x3F) << 3) + road_width_orig;
    ORoad_road_width = (pos << 16) | (ORoad_road_width & 0xFFFF);
    if (pos > 0xFF)
    {
        route_updated &= ~BIT_0;
        OInitEngine_init_split3();
    }
}

// ------------------------------------------------------------------------------------------------
// Road Split 3: Road physically splits into two individual roads
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split3()
{
    OInitEngine_rd_split_state = 4;

    int16_t pos = (((ORoad_road_pos >> 16) - 0x3F) << 3) + road_width_orig;
    ORoad_road_width = (pos << 16) | (ORoad_road_width & 0xFFFF);

    if (route_updated & BIT_0 || pos <= 0x168)
    {
        if (ORoad_road_width >> 16 > 0x300)
            OInitEngine_init_split4();
        return;
    }

    route_updated |= BIT_0; // Denote route info updated
    OInitEngine_route_selected = 0;

    // Go Left
    if (OInitEngine_car_x_pos > 0)
    {
        OInitEngine_route_selected = -1;
        uint8_t inc = 1 << (3 - OStats_cur_stage);

        // One of the following increment values

        // Stage 1 = +8 (1 << 3 - 0)
        // Stage 2 = +4 (1 << 3 - 1)
        // Stage 3 = +2 (1 << 3 - 2)
        // Stage 4 = +1 (1 << 3 - 3)
        // Stage 5 = Road doesn't split on this stage

        OStats_route_info += inc;
        ORoad_stage_lookup_off++;
    }
    // Go Right / Continue

    OInitEngine_end_stage_props |= BIT_0;                                 // Set end of stage property (road splitting)
    OSmoke_load_smoke_data |= BIT_0;                          // Denote we should load smoke sprite data
    OStats_routes[0]++;                                       // Set upcoming stage number to store route info
    OStats_routes[OStats_routes[0]] = OStats_route_info;      // Store route info for course map screen

    if (ORoad_road_width >> 16 > 0x300)
        OInitEngine_init_split4();
}

// ------------------------------------------------------------------------------------------------
// Road Split 4: Road Fully Split, Remove Other Road
// ------------------------------------------------------------------------------------------------

void OInitEngine_init_split4()
{
    OInitEngine_rd_split_state = 5; // init_split4

     // Set Appropriate Road Control Value, Dependent On Route Chosen
    if (OInitEngine_route_selected != 0)
        ORoad_road_ctrl = ROAD_R0_SPLIT;
    else
        ORoad_road_ctrl = ROAD_R1_SPLIT;

    // Denote road split has been removed (for enemy traFfic logic)
    OInitEngine_road_remove_split |= BIT_0;
       
    if (!OInitEngine_road_curve)
        OInitEngine_init_split5();
}

// ------------------------------------------------------------------------------------------------
// Road Split 5: Only Draw One Section Of Road - Wait For Final Curve
// ------------------------------------------------------------------------------------------------

void OInitEngine_init_split5()
{
    OInitEngine_rd_split_state = 6;
    if (OInitEngine_road_curve)
        OInitEngine_init_split6();
}

// ------------------------------------------------------------------------------------------------
// Road Split 6 - Car On Final Curve Of Split
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split6()
{
    OInitEngine_rd_split_state = 7;
    if (!OInitEngine_road_curve)
        OInitEngine_init_split7();
}

// ------------------------------------------------------------------------------------------------
// Road Split 7: Init Road Merge Before Checkpoint (From Normal Section Of Road)
// ------------------------------------------------------------------------------------------------

void OInitEngine_init_split7()
{
    OInitEngine_rd_split_state = 8;

    ORoad_road_ctrl = ROAD_BOTH_P0;
    OInitEngine_route_selected = ~OInitEngine_route_selected; // invert bits
    int16_t width2 = (ORoad_road_width >> 16) << 1;
    if (OInitEngine_route_selected == 0) 
        width2 = -width2;
    OInitEngine_car_x_pos += width2;
    OInitEngine_car_x_old += width2;
    road_width_orig = ORoad_road_pos >> 16;
    road_width_merge = ORoad_road_width >> 19; // (>> 16 and then >> 3)
    OInitEngine_road_remove_split &= ~BIT_0; // Denote we're back to normal road handling for enemy traffic logic
}

// ------------------------------------------------------------------------------------------------
// Road Split 9 - Do Road Merger. Road Gets Narrower Again.
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split9()
{
    OInitEngine_rd_split_state = 10;

    // Calculate narrower road width to merge roads
    uint16_t d0 = (road_width_merge - ((ORoad_road_pos >> 16) - road_width_orig)) << 3;

    if (d0 <= RD_WIDTH_MERGE)
    {
        ORoad_road_width = (RD_WIDTH_MERGE << 16) | (ORoad_road_width & 0xFFFF);
        OInitEngine_init_split10();
    }
    else
        ORoad_road_width = (d0 << 16) | (ORoad_road_width & 0xFFFF);
}


// ------------------------------------------------------------------------------------------------
// Road Split A: Checkpoint Sign
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split10()
{
    OInitEngine_rd_split_state = 11;

    if (ORoad_road_pos >> 16 > 0x180)
    {
        OInitEngine_rd_split_state = 0;
        OInitEngine_init_split_next_level();
    }
}

// ------------------------------------------------------------------------------------------------
// Road Split B: Init Next Level
// ------------------------------------------------------------------------------------------------
void OInitEngine_init_split_next_level()
{
    ORoad_road_pos = 0;
    ORoad_tilemap_h_target = 0;
    OStats_cur_stage++;
    ORoad_stage_lookup_off += 8;    // Increment lookup to next block of stages
    OStats_route_info += 0x10;      // Route Info increments by 10 at each stage
    OHud_do_mini_map();
    OInitEngine_init_road_seg_master();

    // Clear sprite palette lookup
    if (OStats_cur_stage)
        OSprites_clear_palette_data();
}

// ------------------------------------------------------------------------------------------------
// Bonus Road Mode Control
// ------------------------------------------------------------------------------------------------

// Initialize new segment of road data for bonus sequence
// Source: 0x8A04
void OInitEngine_init_bonus(int16_t seq)
{
    ORoad_road_ctrl = ROAD_BOTH_P0_INV;
    ORoad_road_pos  = 0;
    ORoad_tilemap_h_target = 0;
    OAnimSeq_end_seq = (uint8_t) seq; // Set End Sequence (0 - 4)
    TrackLoader_init_track_bonus(OAnimSeq_end_seq);
    Outrun_game_state = GS_INIT_BONUS;
    OInitEngine_rd_split_state = 0x11;
    OInitEngine_bonus1();
}

void OInitEngine_bonus1()
{
    if (ORoad_road_pos >> 16 >= 0x5B)
    {
        OTraffic_bonus_lhs = 1; // Force traffic spawn on LHS during bonus mode
        OInitEngine_rd_split_state = 0x12;
        OInitEngine_bonus2();
    }
}

void OInitEngine_bonus2()
{
    if (ORoad_road_pos >> 16 >= 0xB6)
    {
        OTraffic_bonus_lhs = 0; // Disable forced traffic spawn
        road_width_orig = ORoad_road_width >> 16;
        OInitEngine_rd_split_state = 0x13;
        OInitEngine_bonus3();
    }
}

// Stretch the road to a wider width. It does this based on the car's current position.
void OInitEngine_bonus3()
{
    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((ORoad_road_pos >> 16) - 0xB6) << 3) + road_width_orig;
    ORoad_road_width = (pos << 16) | (ORoad_road_width & 0xFFFF);
    if (pos > 0xFF)
    {
        OInitEngine_route_selected = 0;
        if (OInitEngine_car_x_pos > 0)
            OInitEngine_route_selected = ~OInitEngine_route_selected; // invert bits
        OInitEngine_rd_split_state = 0x14;
        OInitEngine_bonus4();
    }
}

void OInitEngine_bonus4()
{
    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((ORoad_road_pos >> 16) - 0xB6) << 3) + road_width_orig;
    ORoad_road_width = (pos << 16) | (ORoad_road_width & 0xFFFF);
    if (pos > 0x300)
    {
         // Set Appropriate Road Control Value, Dependent On Route Chosen
        if (OInitEngine_route_selected != 0)
            ORoad_road_ctrl = ROAD_R0_SPLIT;
        else
            ORoad_road_ctrl = ROAD_R1_SPLIT;

        // Denote road split has been removed (for enemy traFfic logic)
        OInitEngine_road_remove_split |= BIT_0;
        OInitEngine_rd_split_state = 0x15;
        OInitEngine_bonus5();
    }
}

// Check for end of curve. Init next state when ended.
void OInitEngine_bonus5()
{
    if (!OInitEngine_road_curve)
    {
        OInitEngine_rd_split_state = 0x16;
        OInitEngine_bonus6();
    }
}

// This state simply checks for the end of bonus mode
void OInitEngine_bonus6()
{
    if (OBonus_bonus_control >= BONUS_END)
        OInitEngine_rd_split_state = 0;
}

// SetGranularPosition
//
// Source: BD3E
//
// Uses the car increment value to set the granular position.
// The granular position is used to finely scroll the road by CPU 1 and smooth zooming of scenery.
//
// pos_fine is the (road_pos >> 16) * 10
//
// Notes:
// Disable with - bpset bd3e,1,{pc = bd76; g}

void OInitEngine_set_granular_position()
{
    uint16_t car_inc16 = OInitEngine_car_increment >> 16;

    uint16_t result = car_inc16 / 0x40;
    uint16_t rem = car_inc16 % 0x40;

    granular_rem += rem;
    // When the overall counter overflows past 0x40, we must carry a 1 to the unsigned divide :)
    if (granular_rem >= 0x40)
    {
        granular_rem -= 0x40;
        result++;
    }
    ORoad_pos_fine += result;
}

void OInitEngine_set_fine_position()
{
    uint16_t d0 = ORoad_pos_fine - pos_fine_old;
    if (d0 > 0xF)
        d0 = 0xF;

    d0 <<= 0xB;
    OSprites_sprite_scroll_speed = d0;

    pos_fine_old = ORoad_pos_fine;
}

// Check whether to initalize crash or bonus sequence code
//
// Source: 0x984E
void OInitEngine_init_crash_bonus()
{
    if (Outrun_game_state == GS_MUSIC) return;

    if (OCrash_skid_counter > 6 || OCrash_skid_counter < -6)
    {
        // do_skid:
        if (OTraffic_collision_traffic == 1)
        {   
            OTraffic_collision_traffic = 2;
            uint8_t rnd = outils_random() & OTraffic_collision_mask;
            if (rnd == OTraffic_collision_mask)
            {
                // Try to launch crash code and perform a spin
                if (OCrash_coll_count1 == OCrash_coll_count2)
                {
                    if (!OCrash_spin_control1)
                    {
                        OCrash_spin_control1 = 1;
                        OCrash_skid_counter_bak = OCrash_skid_counter;
                        OInitEngine_test_bonus_mode(1); // 9924 fall through
                        return;
                    }
                    OInitEngine_test_bonus_mode(0); // finalise skid               
                    return;
                }
                else
                {
                    OInitEngine_test_bonus_mode(1); // test_bonus_mode
                    return;
                }
            }
        }
    }
    else if (OCrash_spin_control2 == 1)
    {
        // 9894
        OCrash_spin_control2 = 2;
        if (OCrash_coll_count1 == OCrash_coll_count2)
        {
            OCrash_enable();
        }
        OInitEngine_test_bonus_mode(0); // finalise skid
        return;
    }
    else if (OCrash_spin_control1 == 1)
    {
        // 98c0
        OCrash_spin_control1 = 2;
        OCrash_enable();
        OInitEngine_test_bonus_mode(0); // finalise skid
        return;
    }

    // 0x9924: Section Of Code
    if (OCrash_coll_count1 == OCrash_coll_count2) 
    {
        OInitEngine_test_bonus_mode(1);  // test_bonus_mode
    }
    else
    {
        OCrash_enable();
        OInitEngine_test_bonus_mode(0); // finalise skid
    }
}

// Source: 0x993C
void OInitEngine_test_bonus_mode(uint8_t do_bonus_check)
{
    // Bonus checking code 
    if (do_bonus_check && OBonus_bonus_control)
    {
        // Do Bonus Text Display
        if (Outrun_cannonball_mode != OUTRUN_MODE_TTRIAL && OBonus_bonus_state < 3)
            OBonus_do_bonus_text();

        // End Seq Animation Stage #0
        if (OBonus_bonus_control == BONUS_SEQ0)
        {
            if (Outrun_cannonball_mode == OUTRUN_MODE_TTRIAL)
                Outrun_game_state = GS_INIT_GAMEOVER;
            else
                OAnimSeq_init_end_seq();
        }
    }

   // finalise_skid:
   if (!OCrash_skid_counter)
       OTraffic_collision_traffic = 0;
}
