/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>

typedef struct
{
    uint32_t enabled;
    char title[128];
    char filename[512];
} custom_music_t;

typedef struct
{
    int32_t laps;
    int32_t traffic;
    uint16_t best_times[15];
} ttrial_settings_t;

typedef struct 
{
    uint32_t enabled;
    int32_t road_scroll_speed;
} menu_settings_t;

enum
{
    VIDEO_MODE_WINDOW,
    VIDEO_MODE_FULL,
    VIDEO_MODE_STRETCH
};

typedef struct
{
    int32_t mode;
    int32_t scale;
    int32_t scanlines;
    int32_t widescreen;
    int32_t fps;
    int32_t fps_count;
    int32_t hires;
    int32_t filtering;
    int32_t detailLevel;
    int32_t clipPlane;
} video_settings_t;

typedef struct
{
    int32_t enabled;
    int32_t advertise;
    int32_t preview;
    int32_t fix_samples;
    custom_music_t custom_music[4];
    int32_t amiga_midi;
    int32_t amiga_mods;
    int32_t amiga_fx;
} sound_settings_t;

enum
{
    CONTROLS_GEAR_BUTTON,
    CONTROLS_GEAR_PRESS,     // For cabinets
    CONTROLS_GEAR_SEPARATE,  // Separate button presses
    CONTROLS_GEAR_AUTO
};

typedef struct
{
    int32_t gear;
    int32_t steer_speed;   // Steering Digital Speed
    int32_t pedal_speed;   // Pedal Digital Speed
    int32_t padconfig[8];  // Joypad Button Config
    int32_t keyconfig[12]; // Keyboard Button Config
    int32_t pad_id;        // Use the N'th joystick on the system.
    int32_t analog;        // Use analog controls
    int32_t axis[3];       // Analog Axis
    int32_t asettings[3];  // Analog Settings

    int32_t haptic;        // Force Feedback Enabled
    int32_t max_force;
    int32_t min_force;
    int32_t force_duration;
} controls_settings_t;

enum
{
    CABINET_MOVING  = 0,
    CABINET_UPRIGHT = 1,
    CABINET_MINI    = 2
};

typedef struct
{
    int32_t enabled;      // CannonBall used in conjunction with CannonBoard in arcade cabinet
    char port[64]; // Port Name
    int32_t baud;         // Baud Rate
    int32_t debug;        // Display Debug Information
    int32_t cabinet;      // Cabinet Type
} cannonboard_settings_t;

typedef struct
{
    int32_t dip_time;
    int32_t dip_traffic;
    uint8_t freeplay;
    uint8_t freeze_timer;
    uint8_t disable_traffic;
    int32_t jap;
    int32_t prototype;
    int32_t randomgen;
    int32_t level_objects;
    uint8_t fix_bugs;
    uint8_t fix_bugs_backup;
    uint8_t fix_timer;
    uint8_t layout_debug;
    int new_attract;
} engine_settings_t;

extern menu_settings_t        Config_menu;
extern video_settings_t       Config_video;
extern sound_settings_t       Config_sound;
extern controls_settings_t    Config_controls;
extern engine_settings_t      Config_engine;
extern ttrial_settings_t      Config_ttrial;
extern cannonboard_settings_t Config_cannonboard;

// Internal screen width and height
extern uint16_t Config_s16_width;
extern uint16_t Config_s16_height;

// Internal screen x offset
extern uint16_t Config_s16_x_off;

// 30 or 60 fps
extern uint32_t Config_fps;

// Original game ticks sprites at 30fps but background scroll at 60fps
extern uint32_t Config_tick_fps;

// Continuous Mode: Traffic Setting
extern uint32_t Config_cont_traffic;
    
void Config_init();
void Config_load(const char* filename);
uint8_t Config_save(const char* filename);
void Config_load_scores(const char* filename);
void Config_save_scores(const char* filename);
void Config_load_tiletrial_scores();
void Config_save_tiletrial_scores();
uint8_t Config_clear_scores();
void Config_set_fps(int fps);
   
