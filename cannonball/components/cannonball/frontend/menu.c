/***************************************************************************
    Front End Menu System.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "main.h"
#include <stdint.h>
#include "menu.h"
#include "setup.h"

#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/osprites.h"
#include "engine/ologo.h"
#include "engine/opalette.h"
#include "engine/otiles.h"

#include "frontend/ttrial.h"

// Menu state
uint8_t Menu_state;

enum
{
    MENU_STATE_MENU,
    MENU_STATE_REDEFINE_KEYS,
    MENU_STATE_REDEFINE_JOY,
    MENU_STATE_TTRIAL,
};

uint8_t redef_state;
static uint32_t frame;
int32_t message_counter;
const static int32_t MESSAGE_TIME = 5;
const char* msg;
int16_t cursor;
uint8_t is_text_menu;
uint16_t horizon_pos;

// ------------------------------------------------------------------------------------------------
// Text Labels for menus
// ------------------------------------------------------------------------------------------------

// Back Labels
#define ENTRY_BACK        "BACK"

// Main Menu
#define ENTRY_PLAYGAME    "PLAY GAME"
#define ENTRY_GAMEMODES   "GAME MODES"
#define ENTRY_SETTINGS    "SETTINGS"
#define ENTRY_ABOUT       "ABOUT"
#define ENTRY_EXIT        "EXIT"

// Game Modes Menu
#define ENTRY_ENHANCED    "SET ENHANCED MODE"
#define ENTRY_ORIGINAL    "SET ORIGINAL MODE"
#define ENTRY_CONT        "CONTINUOUS MODE"
#define ENTRY_TIMETRIAL   "TIME TRIAL MODE"

// Time Trial Menu
#define ENTRY_START        "START TIME TRIAL"
#define ENTRY_LAPS         "NO OF LAPS "

// Continuous Menu
#define ENTRY_START_CONT  "START CONTINUOUS MODE"

// Settings Menu
#define ENTRY_VIDEO       "VIDEO"
#define ENTRY_SOUND       "SOUND"
#define ENTRY_CONTROLS    "CONTROLS"
#define ENTRY_CANNONBOARD "CANNONBOARD"
#define ENTRY_ENGINE      "GAME ENGINE"
#define ENTRY_SCORES      "CLEAR HISCORES"
#define ENTRY_SAVE        "SAVE AND RETURN"

// CannonBoard Menu
#define ENTRY_C_INTERFACE "INTERFACE DIAGNOSTICS"
#define ENTRY_C_INPUTS    "INPUT TEST"
#define ENTRY_C_OUTPUTS   "OUTPUT TEST"
#define ENTRY_C_CRT       "CRT TEST"

// Video Menu
#define ENTRY_FPS         "FRAME RATE "
#define ENTRY_FULLSCREEN  "FULL SCREEN "
#define ENTRY_WIDESCREEN  "WIDESCREEN "
#define ENTRY_HIRES       "HIRES "
#define ENTRY_SCALE       "WINDOW SCALE "
#define ENTRY_SCANLINES   "SCANLINES "

// Sound Menu
#define ENTRY_MUTE        "SOUND "
#define ENTRY_BGM         "BGM VOL "
#define ENTRY_SFX         "SFX VOL "
#define ENTRY_ADVERTISE   "ADVERTISE SOUND "
#define ENTRY_PREVIEWSND  "PREVIEW MUSIC "
#define ENTRY_FIXSAMPLES  "FIX SAMPLES "
#define ENTRY_MUSICTEST   "MUSIC TEST"
#define ENTRY_CAMDMIDI    "AMIGA CAMD MIDI "
#define ENTRY_MODMUSIC    "AMIGA MOD MUSIC "
#define ENTRY_FX          "SOUND FX "

// Controls Menu
#define ENTRY_GEAR        "GEAR "
#define ENTRY_ANALOG      "ANALOG "
#define ENTRY_REDEFJOY    "REDEFINE GAMEPAD"
#define ENTRY_REDEFKEY    "REDEFINE KEYS"
#define ENTRY_DSTEER      "DIGITAL STEER SPEED "
#define ENTRY_DPEDAL      "DIGITAL PEDAL SPEED "

// Game Engine Menu
#define ENTRY_TRACKS      "TRACKS "
#define ENTRY_TIME        "TIME "
#define ENTRY_TRAFFIC     "TRAFFIC "
#define ENTRY_OBJECTS     "OBJECTS "
#define ENTRY_PROTOTYPE   "PROTOTYPE STAGE 1 "
#define ENTRY_ATTRACT     "NEW ATTRACT "

// Music Test Menu
#define ENTRY_MUSIC1      "MAGICAL SOUND SHOWER"
#define ENTRY_MUSIC2      "PASSING BREEZE"
#define ENTRY_MUSIC3      "SPLASH WAVE"
#define ENTRY_MUSIC4      "LAST WAVE"

#define MAX_STRING 64

typedef const char* String;
typedef char FixedString[MAX_STRING];

FixedString menu_selected_todraw[12];
String* menu_selected;
uint16_t  menu_seleted_itemcount;

String menu_main[] = { ENTRY_PLAYGAME, ENTRY_GAMEMODES, ENTRY_SETTINGS, ENTRY_ABOUT, ENTRY_EXIT };
String menu_gamemodes[] = { ENTRY_ENHANCED, ENTRY_ORIGINAL, ENTRY_CONT, ENTRY_TIMETRIAL, ENTRY_BACK };
String menu_cont[] = { ENTRY_START_CONT, ENTRY_TRAFFIC, ENTRY_BACK };
String menu_timetrial[] = { ENTRY_START, ENTRY_LAPS, ENTRY_TRAFFIC, ENTRY_BACK };
String menu_about[] = { "CANNONBALL 0.3 © CHRIS WHITE 2014", "REASSEMBLER.BLOGSPOT.COM", " ", "CANNONBALL IS FREE AND MAY NOT BE SOLD." };
String menu_video[] = { ENTRY_FPS, ENTRY_FULLSCREEN, ENTRY_WIDESCREEN, ENTRY_HIRES, ENTRY_SCALE, ENTRY_SCANLINES, ENTRY_BACK, };
#if !defined (_AMIGA_)
String menu_sound[] = { ENTRY_MUTE, ENTRY_ADVERTISE, ENTRY_PREVIEWSND, ENTRY_FIXSAMPLES, ENTRY_MUSICTEST, ENTRY_CAMDMIDI, ENTRY_BACK };
#else
String menu_sound[] = { ENTRY_CAMDMIDI, ENTRY_MUSICTEST, ENTRY_BACK };
#endif
String menu_controls[] = { ENTRY_GEAR, ENTRY_ANALOG, ENTRY_REDEFKEY, ENTRY_REDEFJOY, ENTRY_DSTEER, ENTRY_DPEDAL, ENTRY_BACK };
String menu_engine[] = { ENTRY_TRACKS, ENTRY_TIME, ENTRY_TRAFFIC, ENTRY_OBJECTS, ENTRY_PROTOTYPE, ENTRY_ATTRACT, ENTRY_BACK };
String menu_musictest[] = { ENTRY_MUSIC1, ENTRY_MUSIC2, ENTRY_MUSIC3, ENTRY_MUSIC4, ENTRY_BACK };
String text_redefine[] = { "PRESS UP", "PRESS DOWN", "PRESS LEFT", "PRESS RIGHT", "PRESS ACCELERATE", "PRESS BRAKE", "PRESS GEAR", "PRESS GEAR HIGH", "PRESS START", "PRESS COIN IN", "PRESS MENU", "PRESS VIEW CHANGE" };

String menu_settings[] = { ENTRY_VIDEO, ENTRY_SOUND
ENTRY_CONTROLS, ENTRY_ENGINE, ENTRY_SCORES, ENTRY_SAVE };

uint16_t menu_main_itemcount = 5;
uint16_t menu_gamemodes_itemcount = 5;
uint16_t menu_cont_itemcount = 3;
uint16_t menu_timetrial_itemcount = 4;
uint16_t menu_about_itemcount = 4;
uint16_t menu_video_itemcount = 7;
#if !defined (_AMIGA_)
uint16_t menu_sound_itemcount = 7;
#else
uint16_t menu_sound_itemcount = 3;
#endif
uint16_t menu_controls_itemcount = 7;
uint16_t menu_engine_itemcount = 7;
uint16_t menu_musictest_itemcount = 5;
uint16_t text_redefine_itemcount = 12;

uint16_t menu_settings_itemcount = 5;

void Menu_tick_ui();
void Menu_draw_menu_options();
void Menu_draw_text(const char* s);
void Menu_tick_menu();
void Menu_set_menu(String* menu, uint16_t menu_items_count);
void Menu_refresh_menu();
void Menu_set_menu_text(const char* s1, const char* s2);
void Menu_redefine_keyboard();
void Menu_redefine_joystick();
void Menu_display_message(const char* message);
uint8_t Menu_check_jap_roms();
void Menu_restart_video();
void Menu_start_game(int mode, int settings);

// Logo Y Position
const static int16_t LOGO_Y = -60;

// Columns and rows available
const static uint16_t COLS = 40;
const static uint16_t ROWS = 28;

// Horizon Destination Position
const static uint16_t HORIZON_DEST = 0x3A0;


void Menu_init()
{   
    // If we got a new high score on previous time trial, then save it!
    if (Outrun_ttrial.new_high_score)
    {
        Outrun_ttrial.new_high_score = 0;
        TTrial_update_best_time();
    }

    Outrun_select_course(0, Config_engine.prototype != 0);
    Video_enabled = 1;
    HWSprites_set_x_clip(0); // Stop clipping in wide-screen mode.
    HWSprites_reset();
    Video_clear_text_ram();
    HWTiles_restore_tiles();
    OLogo_enable(LOGO_Y);

    // Setup palette, road and colours for background
    ORoad_stage_lookup_off = 9;
    OInitEngine_init_road_seg_master();
    OPalette_setup_sky_palette();
    OPalette_setup_ground_color();
    OPalette_setup_road_centre();
    OPalette_setup_road_stripes();
    OPalette_setup_road_side();
    OPalette_setup_road_colour();
    OTiles_setup_palette_hud();

    ORoad_init();
    ORoad_road_ctrl = ROAD_R0;
    ORoad_horizon_set = 1;
    ORoad_horizon_base = HORIZON_DEST + 0x100;
    OInitEngine_rd_split_state = INITENGINE_SPLIT_NONE;
    OInitEngine_car_increment = 0;
    OInitEngine_change_width = 0;

    Outrun_game_state = GS_INIT;

    Menu_set_menu(menu_main, menu_main_itemcount);
    Menu_refresh_menu();

    // Reset audio, so we can play tones
    OSoundInt_has_booted = 1;
    OSoundInt_init();

#ifdef COMPILE_SOUND_CODE
    Audio_clear_wav();
#endif

    frame = 0;
    message_counter = 0;

    Menu_state = MENU_STATE_MENU;
}

void Menu_tick(Packet* packet)
{
    switch (Menu_state)
    {
        case MENU_STATE_MENU:
        case MENU_STATE_REDEFINE_KEYS:
        case MENU_STATE_REDEFINE_JOY:
            Menu_tick_ui();
            break;

        case MENU_STATE_TTRIAL:
            {
                int ttrial_state = TTrial_tick();

                if (ttrial_state == TTRIAL_INIT_GAME)
                {
                    cannonball_state = STATE_INIT_GAME;
                    OSoundInt_queue_clear();
                }
                else if (ttrial_state == TTRIAL_BACK_TO_MENU)
                {
                    Menu_init();
                }
            }
            break;
    }
}

void Menu_tick_ui()
{
    // Skip odd frames at 60fps
    frame++;

    Video_clear_text_ram();

    if (Menu_state == MENU_STATE_MENU)
    {
        Menu_tick_menu();
        Menu_draw_menu_options();
    }
    else if (Menu_state == MENU_STATE_REDEFINE_KEYS)
    {
        Menu_redefine_keyboard();
    }
    else if (Menu_state == MENU_STATE_REDEFINE_JOY)
    {
        Menu_redefine_joystick();
    }

    // Show messages
    if (message_counter > 0)
    {
        message_counter--;
        OHud_blit_text_new(0, 1, msg, HUD_GREY);
    }
     
    // Shift horizon
    if (ORoad_horizon_base > HORIZON_DEST)
    {
        ORoad_horizon_base -= 60 / Config_fps;
        if (ORoad_horizon_base < HORIZON_DEST)
            ORoad_horizon_base = HORIZON_DEST;
    }
    // Advance road
    else
    {
        uint32_t scroll_speed = (Config_fps == 60) ? Config_menu.road_scroll_speed : Config_menu.road_scroll_speed << 1;

        if (OInitEngine_car_increment < scroll_speed << 16)
            OInitEngine_car_increment += (1 << 14);
        if (OInitEngine_car_increment > scroll_speed << 16)
            OInitEngine_car_increment = scroll_speed << 16;
        uint32_t result = 0x12F * (OInitEngine_car_increment >> 16);
        ORoad_road_pos_change = result;
        ORoad_road_pos += result;
        if (ORoad_road_pos >> 16 > ROAD_END) // loop to beginning of track data
            ORoad_road_pos = 0;
        OInitEngine_update_road();
        OInitEngine_set_granular_position();
        ORoad_road_width_bak = ORoad_road_width >> 16; 
        ORoad_car_x_bak = -ORoad_road_width_bak; 
        OInitEngine_car_x_pos = ORoad_car_x_bak;
    }

    // Do Animations at 30 fps
    if (Config_fps != 60 || (frame & 1) == 0)
    {
        OLogo_tick();
        OSprites_sprite_copy();
        OSprites_update_sprites();
    }

    // Draw FPS
    if (Config_video.fps_count)
        OHud_draw_fps_counter(cannonball_fps_counter);

    ORoad_tick();
}

void Menu_draw_menu_options()
{
    int i;
    int8_t x = 0;

    // Find central column in screen. 
    int8_t y = 13 + ((ROWS - 13) >> 1) - ((menu_seleted_itemcount * 2) >> 1);

    for (i = 0; i < (int) menu_seleted_itemcount; i++)
    {
        FixedString* s = &menu_selected_todraw[i];

        // Centre the menu option
        x = 20 - (strlen(*s) >> 1);
        OHud_blit_text_new(x, y, *s, HUD_GREEN);

        if (!is_text_menu)
        {
            // Draw minicar
            if (i == cursor)
                Video_write_text32(OHud_translate(x - 3, y, 0x110030), RomLoader_read32(&Roms_rom0, TILES_MINICARS1));
            // Erase minicar from this position
            else
                Video_write_text32(OHud_translate(x - 3, y, 0x110030), 0x20202020);
        }
        y += 2;
    }
}

// Draw a single line of text
void Menu_draw_text(const char* s)
{
    // Centre text
    int8_t x = 20 - (strlen(s) >> 1);

    // Find central column in screen. 
    int8_t y = 13 + ((ROWS - 13) >> 1) - 1;

    OHud_blit_text_new(x, y, s, HUD_GREEN);
}

uint8_t startsWith(const char* base, const char* str)
{
    return (strstr(base, str) - base) == 0;
}

#define SELECTED(string) startsWith(OPTION, string)

void Menu_tick_menu()
{
    // Tick Controls
    if (Input_has_pressed(INPUT_DOWN) || OInputs_is_analog_r())
    {
        OSoundInt_queue_sound(sound_BEEP1);

        if (++cursor >= (int16_t) menu_seleted_itemcount)
            cursor = 0;
    }
    else if (Input_has_pressed(INPUT_UP) || OInputs_is_analog_l())
    {
        OSoundInt_queue_sound(sound_BEEP1);

        if (--cursor < 0)
            cursor = menu_seleted_itemcount - 1;
    }
    else if (Input_has_pressed(INPUT_ACCEL) || Input_has_pressed(INPUT_START) || OInputs_is_analog_select())
    {
        // Get option that was selected
        const char* OPTION = menu_selected[cursor];

        if (menu_selected == menu_main)
        {
            if (SELECTED(ENTRY_PLAYGAME))
            {
                Menu_start_game(OUTRUN_MODE_ORIGINAL, 0);
                //cabdiag->set(CabDiag::STATE_MOTORT);
                //Menu_state = STATE_DIAGNOSTICS;
                return;
            }
            else if (SELECTED(ENTRY_GAMEMODES))
                Menu_set_menu(menu_gamemodes, menu_gamemodes_itemcount);
            else if (SELECTED(ENTRY_SETTINGS))
                Menu_set_menu(menu_settings, menu_settings_itemcount);
            else if (SELECTED(ENTRY_ABOUT))
                Menu_set_menu(menu_about, menu_about_itemcount);
            else if (SELECTED(ENTRY_EXIT))
                cannonball_state = STATE_QUIT;
        }
        else if (menu_selected == menu_gamemodes)
        {
            if (SELECTED(ENTRY_ENHANCED))
                Menu_start_game(OUTRUN_MODE_ORIGINAL, 1);
            else if (SELECTED(ENTRY_ORIGINAL))
                Menu_start_game(OUTRUN_MODE_ORIGINAL, 2);
            else if (SELECTED(ENTRY_CONT))
                Menu_set_menu(menu_cont, menu_cont_itemcount);
            else if (SELECTED(ENTRY_TIMETRIAL))
                Menu_set_menu(menu_timetrial, menu_timetrial_itemcount);
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_main, menu_main_itemcount);
        }
        else if (menu_selected == menu_cont)
        {
            if (SELECTED(ENTRY_START_CONT))
            {
                Config_save(FILENAME_CONFIG);
                Outrun_custom_traffic = Config_cont_traffic;
                Menu_start_game(OUTRUN_MODE_CONT, 0);
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (++Config_cont_traffic > TTRIAL_MAX_TRAFFIC)
                    Config_cont_traffic = 0;
            }
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_gamemodes, menu_gamemodes_itemcount);
        }
        else if (menu_selected == menu_timetrial)
        {
            if (SELECTED(ENTRY_START))
            {
                if (Menu_check_jap_roms())
                {
                    Config_save(FILENAME_CONFIG);
                    Menu_state = MENU_STATE_TTRIAL;
                    TTrial_init();
                }
            }
            else if (SELECTED(ENTRY_LAPS))
            {
                if (++Config_ttrial.laps > TTRIAL_MAX_LAPS)
                    Config_ttrial.laps = 1;
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (++Config_ttrial.traffic > TTRIAL_MAX_TRAFFIC)
                    Config_ttrial.traffic = 0;
            }
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_gamemodes, menu_gamemodes_itemcount);
        }
        else if (menu_selected == menu_settings)
        {
            if (SELECTED(ENTRY_VIDEO))
                Menu_set_menu(menu_video, menu_video_itemcount);
            else if (SELECTED(ENTRY_SOUND))
                Menu_set_menu(menu_sound, menu_sound_itemcount);
            else if (SELECTED(ENTRY_CONTROLS))
            {
                if (Input_gamepad)
                    Menu_display_message("GAMEPAD FOUND");
                Menu_set_menu(menu_controls, menu_controls_itemcount);
            }
            else if (SELECTED(ENTRY_ENGINE))
                Menu_set_menu(menu_engine, menu_engine_itemcount);
            else if (SELECTED(ENTRY_SCORES))
            {
                if (Config_clear_scores())
                    Menu_display_message("SCORES CLEARED");
                else
                    Menu_display_message("NO SAVED SCORES FOUND!");
            }
            else if (SELECTED(ENTRY_SAVE))
            {
                if (Config_save(FILENAME_CONFIG))
                    Menu_display_message("SETTINGS SAVED");
                else
                    Menu_display_message("ERROR SAVING SETTINGS!");
                Menu_set_menu(menu_main, menu_main_itemcount);
            }
        }
        else if (menu_selected == menu_about)
        {
            Menu_set_menu(menu_main, menu_main_itemcount);
        }
        else if (menu_selected == menu_video)
        {
            if (SELECTED(ENTRY_FULLSCREEN))
            {
                if (++Config_video.mode > VIDEO_MODE_STRETCH)
                    Config_video.mode = VIDEO_MODE_WINDOW;
                Menu_restart_video();
            }
            else if (SELECTED(ENTRY_WIDESCREEN))
            {
                Config_video.widescreen = !Config_video.widescreen;
                Menu_restart_video();
            }
            else if (SELECTED(ENTRY_HIRES))
            {
                Config_video.hires = !Config_video.hires;
                if (Config_video.hires)
                {
                    if (Config_video.scale > 1)
                        Config_video.scale >>= 1;
                }
                else
                {
                    Config_video.scale <<= 1;
                }

                Menu_restart_video();
                HWSprites_set_x_clip(0);
            }
            else if (SELECTED(ENTRY_SCALE))
            {
                if (++Config_video.scale > (Config_video.hires ? 2 : 4))
                    Config_video.scale = 1;
                Menu_restart_video();
            }
            else if (SELECTED(ENTRY_SCANLINES))
            {
                Config_video.scanlines += 10;
                if (Config_video.scanlines > 100)
                    Config_video.scanlines = 0;
                Menu_restart_video();
            }
            else if (SELECTED(ENTRY_FPS))
            {
                if (++Config_video.fps > 2)
                    Config_video.fps = 0;
                Config_set_fps(Config_video.fps);
            }
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_settings, menu_settings_itemcount);
        }
        else if (menu_selected == menu_sound)
        {
            
#if !defined (_AMIGA_)            
            if (SELECTED(ENTRY_MUTE))
            {
                Config_sound.enabled = !Config_sound.enabled;
                #ifdef COMPILE_SOUND_CODE
                if (Config_sound.enabled)
                    Audio_start_audio();
                else
                    Audio_stop_audio();              
                #endif
            }
            else if (SELECTED(ENTRY_ADVERTISE))
                Config_sound.advertise = !Config_sound.advertise;
            else if (SELECTED(ENTRY_PREVIEWSND))
                Config_sound.preview = !Config_sound.preview;
            else if (SELECTED(ENTRY_FIXSAMPLES))
            {
                int rom_type = !Config_sound.fix_samples;
                
                if (Roms_load_pcm_rom(rom_type == 1))
                {
                    Config_sound.fix_samples = rom_type;
                    Menu_display_message(rom_type == 1 ? "FIXED SAMPLES LOADED" : "ORIGINAL SAMPLES LOADED");
                }
                else
                {
                    Menu_display_message(rom_type == 1 ? "CANT LOAD FIXED SAMPLES" : "CANT LOAD ORIGINAL SAMPLES");
                }
            }
            else if (SELECTED(ENTRY_MUSICTEST))
                Menu_set_menu(menu_musictest, menu_musictest_itemcount);
#else                
            if (SELECTED(ENTRY_CAMDMIDI))
            {
                Config_sound.amiga_midi = !Config_sound.amiga_midi; 
                
            }
            else if (SELECTED(ENTRY_MUSICTEST))
                Menu_set_menu(menu_musictest, menu_musictest_itemcount);            
#endif            
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_settings, menu_settings_itemcount);
        }
        else if (menu_selected == menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (++Config_controls.gear > CONTROLS_GEAR_AUTO)
                    Config_controls.gear = CONTROLS_GEAR_BUTTON;
            }
            else if (SELECTED(ENTRY_ANALOG))
            {
                if (++Config_controls.analog == 3)
                    Config_controls.analog = 0;
                Input_analog = Config_controls.analog;
            }
            else if (SELECTED(ENTRY_REDEFKEY))
            {
                Menu_display_message("PRESS MENU TO END AT ANY STAGE");
                Menu_state = MENU_STATE_REDEFINE_KEYS;
                redef_state = 0;
                Input_key_press = -1;
            }
            else if (SELECTED(ENTRY_REDEFJOY))
            {
                Menu_display_message("PRESS MENU TO END AT ANY STAGE");
                Menu_state = MENU_STATE_REDEFINE_JOY;
                redef_state = Config_controls.analog == 1 ? 2 : 0; // Ignore pedals when redefining analog
                Input_joy_button = -1;
            }
            else if (SELECTED(ENTRY_DSTEER))
            {
                if (++Config_controls.steer_speed > 9)
                    Config_controls.steer_speed = 1;
            }
            else if (SELECTED(ENTRY_DPEDAL))
            {
                if (++Config_controls.pedal_speed > 9)
                    Config_controls.pedal_speed = 1;
            }
            else if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_settings, menu_settings_itemcount);
        }
        else if (menu_selected == menu_engine)
        {
            if (SELECTED(ENTRY_TRACKS))
            {
                Config_engine.jap = !Config_engine.jap;
            }
            else if (SELECTED(ENTRY_TIME))
            {
                if (Config_engine.dip_time == 3)
                {
                    if (!Config_engine.freeze_timer)
                        Config_engine.freeze_timer = 1;
                    else
                    {
                        Config_engine.dip_time = 0;
                        Config_engine.freeze_timer = 0;
                    }
                }
                else
                    Config_engine.dip_time++;
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (Config_engine.dip_traffic == 3)
                {
                    if (!Config_engine.disable_traffic)
                        Config_engine.disable_traffic = 1;
                    else
                    {
                        Config_engine.dip_traffic = 0;
                        Config_engine.disable_traffic = 0;
                    }
                }
                else
                    Config_engine.dip_traffic++;
            }
            else if (SELECTED(ENTRY_OBJECTS))
                Config_engine.level_objects = !Config_engine.level_objects;
            else if (SELECTED(ENTRY_PROTOTYPE))
                Config_engine.prototype = !Config_engine.prototype;
            else if (SELECTED(ENTRY_ATTRACT))
                Config_engine.new_attract ^= 1;
            if (SELECTED(ENTRY_BACK))
                Menu_set_menu(menu_settings, menu_settings_itemcount);
        }
        else if (menu_selected == menu_musictest)
        {
            if (SELECTED(ENTRY_MUSIC1))
            {
                if (Config_sound.amiga_midi)   
                { 
                    I_CAMD_StopSong();                
                    I_CAMD_PlaySong("data/Magical.mid");    
                }
                else
                    OSoundInt_queue_sound(SOUND_MUSIC_MAGICAL);
            }
            else if (SELECTED(ENTRY_MUSIC2))
            {
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();
                    I_CAMD_PlaySong("data/Pass-Brz.mid");                    
                }
                else            
                    OSoundInt_queue_sound(SOUND_MUSIC_BREEZE);
            }
            else if (SELECTED(ENTRY_MUSIC3))
            {
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();                
                    I_CAMD_PlaySong("data/Splash.mid");                    
                }
                else            
                    OSoundInt_queue_sound(SOUND_MUSIC_SPLASH);
            }
            else if (SELECTED(ENTRY_MUSIC4))
            {
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();                
                    I_CAMD_PlaySong("data/Lastwave.mid");                    
                }
                else
                    OSoundInt_queue_sound(SOUND_MUSIC_LASTWAVE);
            }
            else if (SELECTED(ENTRY_BACK))
            {
                OSoundInt_queue_sound(sound_FM_RESET);
                Menu_set_menu(menu_sound, menu_sound_itemcount);
            }
        }
        else
            Menu_set_menu(menu_main, menu_main_itemcount);

        OSoundInt_queue_sound(sound_BEEP1);
        Menu_refresh_menu();
    }
}

// Set Current Menu
void Menu_set_menu(String* menu, uint16_t menu_items_count)
{
    menu_seleted_itemcount = menu_items_count;
    menu_selected = menu;
    cursor = 0;

    int loop;
    for (loop = 0; loop < menu_items_count; loop++)
    {
		snprintf(menu_selected_todraw[loop], MAX_STRING, "%s", menu[loop]);
    }

    is_text_menu = (menu == menu_about);
}

// Refresh menu options with latest config data
void Menu_refresh_menu()
{
    int16_t cursor_backup = cursor;
    const char* s;

    const char* trafficStrings[TTRIAL_MAX_TRAFFIC + 1] = { "DISABLED", "1", "2", "3", "4", "5", "6", "7", "8"};
    const char* videoScaleStrings[4] = { "1x", "2x", "3x", "4x" };
    const char* scanLineStrings[11] = { "OFF", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%" };
    const char* lapsStrings[5] = { "1", "2", "3", "4", "5" };
    const char* speedStrings[9] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

    for (cursor = 0; cursor < (int) menu_seleted_itemcount; cursor++)
    {
        // Get option that was selected
        const char* OPTION = menu_selected[cursor];

        if (menu_selected == menu_timetrial)
        {
            if (SELECTED(ENTRY_LAPS))
                Menu_set_menu_text(ENTRY_LAPS, lapsStrings[Config_ttrial.laps - 1]);
            else if (SELECTED(ENTRY_TRAFFIC))
                Menu_set_menu_text(ENTRY_TRAFFIC, trafficStrings[Config_ttrial.traffic]);
        }
        else if (menu_selected == menu_cont)
        {
            if (SELECTED(ENTRY_TRAFFIC))
                Menu_set_menu_text(ENTRY_TRAFFIC, trafficStrings[Config_cont_traffic]);
        }
        else if (menu_selected == menu_video)
        {
            if (SELECTED(ENTRY_FULLSCREEN))
            {
                if (Config_video.mode == VIDEO_MODE_WINDOW)       s = "OFF";
                else if (Config_video.mode == VIDEO_MODE_FULL)    s = "ON";
                else if (Config_video.mode == VIDEO_MODE_STRETCH) s = "STRETCH";
                Menu_set_menu_text(ENTRY_FULLSCREEN, s);
            }
            else if (SELECTED(ENTRY_WIDESCREEN))
                Menu_set_menu_text(ENTRY_WIDESCREEN, Config_video.widescreen ? "ON" : "OFF");
            else if (SELECTED(ENTRY_SCALE))
                Menu_set_menu_text(ENTRY_SCALE, videoScaleStrings[Config_video.scale - 1]);
            else if (SELECTED(ENTRY_HIRES))
                Menu_set_menu_text(ENTRY_HIRES, Config_video.hires ? "ON" : "OFF");
            else if (SELECTED(ENTRY_FPS))
            {
                if (Config_video.fps == 0)      s = "30 FPS";
                else if (Config_video.fps == 1) s = "ORIGINAL";
                else if (Config_video.fps == 2) s = "60 FPS";
                Menu_set_menu_text(ENTRY_FPS, s);
            }
            else if (SELECTED(ENTRY_SCANLINES))
                Menu_set_menu_text(ENTRY_SCANLINES, scanLineStrings[Config_video.scanlines / 10]);
        }
        else if (menu_selected == menu_sound)
        {
            
#if !defined (_AMIGA_)
            if (SELECTED(ENTRY_MUTE))
                Menu_set_menu_text(ENTRY_MUTE, Config_sound.enabled ? "ON" : "OFF");
            else if (SELECTED(ENTRY_ADVERTISE))
                Menu_set_menu_text(ENTRY_ADVERTISE, Config_sound.advertise ? "ON" : "OFF");
            else if (SELECTED(ENTRY_PREVIEWSND))
                Menu_set_menu_text(ENTRY_PREVIEWSND, Config_sound.preview ? "ON" : "OFF");
            else if (SELECTED(ENTRY_FIXSAMPLES))
                Menu_set_menu_text(ENTRY_FIXSAMPLES, Config_sound.fix_samples ? "ON" : "OFF");
#else
            if (SELECTED(ENTRY_CAMDMIDI))
                Menu_set_menu_text(ENTRY_CAMDMIDI, Config_sound.amiga_midi ? "ON" : "OFF");

#endif                
        }
        else if (menu_selected == menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (Config_controls.gear == CONTROLS_GEAR_BUTTON)        s = "MANUAL";
                else if (Config_controls.gear == CONTROLS_GEAR_PRESS)    s = "MANUAL CABINET";
                else if (Config_controls.gear == CONTROLS_GEAR_SEPARATE) s = "MANUAL 2 BUTTONS";
                else if (Config_controls.gear == CONTROLS_GEAR_AUTO)     s = "AUTOMATIC";
                Menu_set_menu_text(ENTRY_GEAR, s);
            }
            else if (SELECTED(ENTRY_ANALOG))
            {
                if (Config_controls.analog == 0)      s = "OFF";
                else if (Config_controls.analog == 1) s = "ON";
                else if (Config_controls.analog == 2) s = "ON WHEEL ONLY";
                Menu_set_menu_text(ENTRY_ANALOG, s);
            }
            else if (SELECTED(ENTRY_DSTEER)) 
                Menu_set_menu_text(ENTRY_DSTEER, speedStrings[Config_controls.steer_speed - 1]);
            else if (SELECTED(ENTRY_DPEDAL))
                Menu_set_menu_text(ENTRY_DPEDAL, speedStrings[Config_controls.pedal_speed - 1]);
        }
        else if (menu_selected == menu_engine)
        {
            if (SELECTED(ENTRY_TRACKS))
                Menu_set_menu_text(ENTRY_TRACKS, Config_engine.jap ? "JAPAN" : "WORLD");
            else if (SELECTED(ENTRY_TIME))
            {
                if (Config_engine.freeze_timer)       s = "INFINITE";
                else if (Config_engine.dip_time == 0) s = "EASY";
                else if (Config_engine.dip_time == 1) s = "NORMAL";
                else if (Config_engine.dip_time == 2) s = "HARD";
                else if (Config_engine.dip_time == 3) s = "HARDEST";          
                Menu_set_menu_text(ENTRY_TIME, s);
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (Config_engine.disable_traffic)       s = "DISABLED";
                else if (Config_engine.dip_traffic == 0) s = "EASY";
                else if (Config_engine.dip_traffic == 1) s = "NORMAL";
                else if (Config_engine.dip_traffic == 2) s = "HARD";
                else if (Config_engine.dip_traffic == 3) s = "HARDEST";          
                Menu_set_menu_text(ENTRY_TRAFFIC, s);
            }
            else if (SELECTED(ENTRY_OBJECTS))
                Menu_set_menu_text(ENTRY_OBJECTS, Config_engine.level_objects ? "ENHANCED" : "ORIGINAL");
            else if (SELECTED(ENTRY_PROTOTYPE))
                Menu_set_menu_text(ENTRY_PROTOTYPE, Config_engine.prototype ? "ON" : "OFF");
            else if (SELECTED(ENTRY_ATTRACT))
                Menu_set_menu_text(ENTRY_ATTRACT, Config_engine.new_attract ? "ON" : "OFF");

        }
    }
    cursor = cursor_backup;
}

// Append Menu Text For A Particular Menu Entry
void Menu_set_menu_text(const char* s1, const char* s2)
{
    strcpy(menu_selected_todraw[cursor], s1);
    strcat(menu_selected_todraw[cursor], s2);
}

void Menu_redefine_keyboard()
{
    if (redef_state == 7 && Config_controls.gear != CONTROLS_GEAR_SEPARATE) // Skip redefine of second gear press
        redef_state++;

    switch (redef_state)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            if (Input_has_pressed(INPUT_MENU))
            {
                message_counter = 0;
                Menu_state = MENU_STATE_MENU;
            }
            else
            {
                Menu_draw_text(text_redefine[redef_state]);
                if (Input_key_press != -1)
                {
                    Config_controls.keyconfig[redef_state] = Input_key_press;
                    redef_state++;
                    Input_key_press = -1;
                }
            }
            break;

        case 12:
            Menu_state = MENU_STATE_MENU;
            break;
    }
}

void Menu_redefine_joystick()
{
    if (redef_state == 3 && Config_controls.gear != CONTROLS_GEAR_SEPARATE) // Skip redefine of second gear press
        redef_state++;

    switch (redef_state)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            if (Input_has_pressed(INPUT_MENU))
            {
                message_counter = 0;
                Menu_state = MENU_STATE_MENU;
            }
            else
            {
                Menu_draw_text(text_redefine[redef_state + 4]);
                if (Input_joy_button != -1)
                {
                    Config_controls.padconfig[redef_state] = Input_joy_button;
                    redef_state++;
                    Input_joy_button = -1;
                }
            }
            break;

        case 8:
            Menu_state = MENU_STATE_MENU;
            break;
    }
}

// Display a contextual message in the top left of the screen
void Menu_display_message(const char* message)
{
    msg = message;
    message_counter = MESSAGE_TIME * Config_fps;
}

uint8_t Menu_check_jap_roms()
{
    if (Config_engine.jap && !Roms_load_japanese_roms())
    {
        Menu_display_message("JAPANESE ROMSET NOT FOUND");
        return 0;
    }
    return 1;
}

// Reinitalize Video, and stop audio to avoid crackles
void Menu_restart_video()
{
    #ifdef COMPILE_SOUND_CODE
    if (Config_sound.enabled)
        Audio_stop_audio();
    #endif
    Video_disable();
    Video_init(&Config_video);
    #ifdef COMPILE_SOUND_CODE
    OSoundInt_init();
    if (Config_sound.enabled)
        Audio_start_audio();
    #endif
}

void Menu_start_game(int mode, int settings)
{
    // Enhanced Settings
    if (settings == 1)
    {
        if (!Config_video.hires)
        {
            if (Config_video.scale > 1)
                Config_video.scale >>= 1;
        }

        if (!Config_sound.fix_samples)
        {
            if (Roms_load_pcm_rom(1))
                Config_sound.fix_samples = 1;
        }

        Config_set_fps(Config_video.fps = 2);
        Config_video.widescreen     = 1;
        Config_video.hires          = 1;
        Config_engine.level_objects = 1;
        Config_engine.new_attract   = 1;
        Config_engine.fix_bugs      = 1;
        Config_sound.preview        = 1;

        Menu_restart_video();
    }
    // Original Settings
    else if (settings == 2)
    {
        if (Config_video.hires)
        {
            Config_video.scale <<= 1;
        }

        if (Config_sound.fix_samples)
        {
            if (Roms_load_pcm_rom(0))
                Config_sound.fix_samples = 0;
        }

        Config_set_fps(Config_video.fps = 1);
        Config_video.widescreen     = 0;
        Config_video.hires          = 0;
        Config_engine.level_objects = 0;
        Config_engine.new_attract   = 0;
        Config_engine.fix_bugs      = 0;
        Config_sound.preview        = 0;

        Menu_restart_video();
    }
    // Otherwise, use whatever is already setup...
    else
    {
        Config_engine.fix_bugs = Config_engine.fix_bugs_backup;
    }

    if (Menu_check_jap_roms())
    {
        Outrun_cannonball_mode = mode;
        cannonball_state = STATE_INIT_GAME;
        OSoundInt_queue_clear();
    }
}
