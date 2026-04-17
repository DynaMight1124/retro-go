/***************************************************************************
    SDL Based Input Handling.

    Populates keys array with user Input_
    If porting to a non-SDL platform, you would need to replace this class.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <stdint.h>
#include <SDL/SDL.h>

enum presses
{
    INPUT_LEFT  = 0,
    INPUT_RIGHT = 1,
    INPUT_UP    = 2,
    INPUT_DOWN  = 3,
    INPUT_ACCEL = 4,
    INPUT_BRAKE = 5,
    INPUT_GEAR1 = 6,
    INPUT_GEAR2 = 7,
    INPUT_START = 8,
    INPUT_COIN  = 9,
    INPUT_VIEWPOINT = 10,        
    INPUT_PAUSE = 11,
    INPUT_STEP  = 12,
    INPUT_TIMER = 13,
    INPUT_MENU = 14     
};

extern uint8_t Input_keys[15];
extern uint8_t Input_keys_old[15];

// Has gamepad been found?
extern uint8_t Input_gamepad;

// Use analog controls
extern int Input_analog;

// Latch last key press for redefines
extern int Input_key_press;

// Latch last joystick button press for redefines
extern int16_t Input_joy_button;

// Analog Controls
extern int Input_a_wheel;
extern int Input_a_accel;
extern int Input_a_brake;

void Input_init(int, int*, int*, int, int*, int*);
void Input_close();
void Input_handle_key_up(SDL_keysym*);
void Input_handle_key_down(SDL_keysym*);
void Input_handle_joy_axis(SDL_JoyAxisEvent*);
void Input_handle_joy_down(SDL_JoyButtonEvent*);
void Input_handle_joy_up(SDL_JoyButtonEvent*);
void Input_frame_done();
uint8_t Input_is_pressed(enum presses p);
uint8_t Input_is_pressed_clear(enum presses p);
uint8_t Input_has_pressed(enum presses p);
