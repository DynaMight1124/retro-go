#ifndef _MENU_H_
#define _MENU_H_


typedef enum{
    MENU_TYPE_VOLUME,
    MENU_TYPE_BRIGHTNESS,
    MENU_TYPE_SAVE,
    MENU_TYPE_LOAD,
//    MENU_TYPE_ASPECT_RATIO,
    MENU_TYPE_EXIT,
    MENU_TYPE_POWERDOWN,
    NB_MENU_TYPES,
} ENUM_MENU_TYPE;


///------ Definition of the different aspect ratios 
// #define ASPECT_RATIOS
//     X(ASPECT_RATIOS_TYPE_STRETCHED, "STRETCHED")
//     X(ASPECT_RATIOS_TYPE_SCALED, "SCALED")
//     X(ASPECT_RATIOS_TYPE_CROPPED, "CROPPED")
//     X(NB_ASPECT_RATIOS_TYPES, "")

// ////------ Enumeration of the different aspect ratios ------
// #undef X
// #define X(a, b) a,
// typedef enum {ASPECT_RATIOS} ENUM_ASPECT_RATIOS_TYPES;

////------ Defines to be shared -------
#define RES_HW_SCREEN_HORIZONTAL    320
#define RES_HW_SCREEN_VERTICAL      240
#define STEP_CHANGE_VOLUME          10
#define STEP_CHANGE_BRIGHTNESS      10
#define NOTIF_SECONDS_DISP			2

////------ Menu commands -------
#define SHELL_CMD_VOLUME_GET                "volume get"
#define SHELL_CMD_VOLUME_SET                "volume set"
#define SHELL_CMD_BRIGHTNESS_GET            "brightness get"
#define SHELL_CMD_BRIGHTNESS_SET            "brightness set"
#define SHELL_CMD_NOTIF_SET                 "notif set"
#define SHELL_CMD_AUDIO_AMP_ON              "audio_amp on"
#define SHELL_CMD_AUDIO_AMP_OFF             "audio_amp off"
#define SHELL_CMD_POWERDOWN                 "powerdown"
#define SHELL_CMD_POWERDOWN_HANDLE          "powerdown handle"
#define SHELL_CMD_INSTANT_PLAY              "instant_play"
#define SHELL_CMD_KEYMAP_DEFAULT            "keymap default"
#define SHELL_CMD_KEYMAP_RESUME             "keymap resume"

////------ Global variables -------
extern int volume_percentage;
extern int brightness_percentage;

// extern const char *aspect_ratio_name[];
// extern int aspect_ratio;
// extern int aspect_ratio_factor_percent;
// extern int aspect_ratio_factor_step;
extern int stop_menu_loop;

//####################################
//# Functions
//####################################
void init_menu_SDL();
void deinit_menu_SDL();
void init_menu_zones();
void init_menu_system_values();
void run_menu_loop();


#endif /* _MENU_H_ */
