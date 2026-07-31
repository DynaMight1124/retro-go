/***************************************************************************
    SDL Based Input Handling.

    Populates keys array with user Input_
    If porting to a non-SDL platform, you would need to replace this class.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include "sdl/input.h"

#include "engine/ohud.h"
#include "frontend/config.h"
 

uint8_t Input_keys[15];
uint8_t Input_keys_old[15];

// Has gamepad been found?
uint8_t Input_gamepad;

// Use analog controls
int Input_analog;

// Latch last key press for redefines
int Input_key_press;

// Latch last joystick button press for redefines
int16_t Input_joy_button;

// Analog Controls
int Input_a_wheel;
int Input_a_accel;
int Input_a_brake;

static const int CENTRE = 0x80;

// Digital Dead Zone
static const int DIGITAL_DEAD = 127;

// SDL Joystick / Keypad
SDL_Joystick *Input_stick;

// Configurations for keyboard and joypad
int* Input_pad_config;
int* Input_key_config;
int* Input_axis;

int Input_wheel_zone;
int Input_wheel_dead;
int Input_pedals_dead;

void Input_handle_key(const int, const uint8_t);
void Input_handle_joy(const uint8_t, const uint8_t);

void Input_init(int pad_id, int* key_config, int* pad_config, int analog, int* axis, int* analog_settings)
{
    Input_key_config  = key_config;
    Input_pad_config  = pad_config;
    Input_analog      = analog;
    Input_axis        = axis;
    Input_wheel_zone  = analog_settings[0];
    Input_wheel_dead  = analog_settings[1];
    Input_pedals_dead = analog_settings[2];

    Input_gamepad = SDL_NumJoysticks() > pad_id;

    if (Input_gamepad)
    {
       Input_stick = SDL_JoystickOpen(pad_id);
    }

    Input_a_wheel = CENTRE;
}

void Input_close()
{
    if (Input_gamepad && Input_stick != NULL)
		SDL_JoystickClose(Input_stick);
}

// Detect whether a key press change has occurred
uint8_t Input_has_pressed(enum presses p)
{
    return Input_keys[p] && !Input_keys_old[p];
}

// Detect whether key is still pressed
uint8_t Input_is_pressed(enum presses p)
{
    return Input_keys[p];
}

// Detect whether pressed and clear the press
uint8_t Input_is_pressed_clear(enum presses p)
{
    uint8_t pressed = Input_keys[p];
    Input_keys[p] = 0;
    return pressed;
}

// Denote that a frame has been done by copying key presses into previous array
void Input_frame_done()
{
    memcpy(&Input_keys_old, &Input_keys, sizeof(Input_keys));
}

void Input_handle_key_down(SDL_keysym* keysym)
{
    Input_key_press = keysym->sym;
    Input_handle_key(Input_key_press, 1);
}

void Input_handle_key_up(SDL_keysym* keysym)
{
    Input_handle_key(keysym->sym, 0);
}
void Input_handle_key(const int key, const uint8_t is_pressed)
{
    // Redefinable Key Input
    if (key == Input_key_config[0])
        Input_keys[INPUT_UP] = is_pressed;

    else if (key == Input_key_config[1])
        Input_keys[INPUT_DOWN] = is_pressed;

    else if (key == Input_key_config[2])
        Input_keys[INPUT_LEFT] = is_pressed;

    else if (key == Input_key_config[3])
        Input_keys[INPUT_RIGHT] = is_pressed;

    if (key == Input_key_config[4])
        Input_keys[INPUT_ACCEL] = is_pressed;

    if (key == Input_key_config[5])
        Input_keys[INPUT_BRAKE] = is_pressed;

    if (key == Input_key_config[6])
        Input_keys[INPUT_GEAR1] = is_pressed;

    if (key == Input_key_config[7])
        Input_keys[INPUT_GEAR2] = is_pressed;

    if (key == Input_key_config[8])
        Input_keys[INPUT_START] = is_pressed;

    if (key == Input_key_config[9])
        Input_keys[INPUT_COIN] = is_pressed;

    if (key == Input_key_config[10])
        Input_keys[INPUT_MENU] = is_pressed;

    if (key == Input_key_config[11])
        Input_keys[INPUT_VIEWPOINT] = is_pressed;

    // Function keys are not redefinable
    switch (key)
    {
        case SDLK_F1:
        case SDLK_k:
        case SDLK_ESCAPE:
            Input_keys[INPUT_PAUSE] = is_pressed;
            break;

        case SDLK_F2:
            Input_keys[INPUT_STEP] = is_pressed;
            break;

        case SDLK_F3:
            Input_keys[INPUT_TIMER] = is_pressed;
            break;

        case SDLK_F5:
            Input_keys[INPUT_MENU] = is_pressed;
            break;

    }
}

void Input_handle_joy_axis(SDL_JoyAxisEvent* evt)
{
    int16_t value = evt->value;

    // Digital Controls
    if (!Input_analog)
    {
        // X-Axis
        if (evt->axis == 0)
        {
            // Neural
            if ( (value > -DIGITAL_DEAD ) && (value < DIGITAL_DEAD ) )
            {
                Input_keys[INPUT_LEFT]  = 0;
                Input_keys[INPUT_RIGHT] = 0;
            }
            else if (value < 0)
            {
                Input_keys[INPUT_LEFT] = 1;
            }
            else if (value > 0)
            {
                Input_keys[INPUT_RIGHT] = 1;
            }
        }
        // Y-Axis
        else if (evt->axis == 1)
        {
            // Neural
            if ( (value > -DIGITAL_DEAD ) && (value < DIGITAL_DEAD ) )
            {
                Input_keys[INPUT_UP]  = 0;
                Input_keys[INPUT_DOWN] = 0;
            }
            else if (value < 0)
            {
                Input_keys[INPUT_UP] = 1;
            }
            else if (value > 0)
            {
                Input_keys[INPUT_DOWN] = 1;
            }
        }
    }
    // Analog Controls
    else
    {
        //std::cout << "Axis: " << (int) evt->axis << " Value: " << (int) evt->value << std::endl;

        // Steering
        // OutRun requires values between 0x48 and 0xb8.
        if (evt->axis == Input_axis[0])
        {
            int percentage_adjust = ((Input_wheel_zone) << 8) / 100;         
            int adjusted = value + ((value * percentage_adjust) >> 8);
            
            // Make 0 hard left, and 0x80 centre value.
            adjusted = ((adjusted + (1 << 15)) >> 9);
            adjusted += 0x40; // Centre

            if (adjusted < 0x40)
                adjusted = 0x40;
            else if (adjusted > 0xC0)
                adjusted = 0xC0;

            // Remove Dead Zone
            if (Input_wheel_dead)
            {
                if (abs(CENTRE - adjusted) <= Input_wheel_dead)
                    adjusted = CENTRE;
            }

            //std::cout << "wheel zone : " << wheel_zone << " : " << std::hex << " : " << (int) adjusted << std::endl;
            Input_a_wheel = adjusted;
        }
        // Accelerator and Brake [Combined Axis]
        else if (Input_axis[1] == Input_axis[2] && (evt->axis == Input_axis[1] || evt->axis == Input_axis[2]))
        {
            // Accelerator
            if (value < -Input_pedals_dead)
            {
                // Scale input to be in the range of 0 to 0x7F
                value = -value;
                int adjusted = value / 258;
                adjusted += (adjusted >> 2);
                Input_a_accel = adjusted;
            }
            // Brake
            else if (value > Input_pedals_dead)
            {
                // Scale input to be in the range of 0 to 0x7F
                int adjusted = value / 258;
                adjusted += (adjusted >> 2);
                Input_a_brake = adjusted;
            }
            else
            {
                Input_a_accel = 0;
                Input_a_brake = 0;
            }
        }
        // Accelerator [Single Axis]
        else if (evt->axis == Input_axis[1])
        {
            // Scale input to be in the range of 0 to 0x7F
            int adjusted = 0x7F - ((value + (1 << 15)) >> 9);           
            adjusted += (adjusted >> 2);
            Input_a_accel = adjusted;
        }
        // Brake [Single Axis]
        else if (evt->axis == Input_axis[2])
        {
            // Scale input to be in the range of 0 to 0x7F
            int adjusted = 0x7F - ((value + (1 << 15)) >> 9);
            adjusted += (adjusted >> 2);
            Input_a_brake = adjusted;
        }
    }
}

void Input_handle_joy_down(SDL_JoyButtonEvent* evt)
{
    // Latch joystick button presses for redefines
    Input_joy_button = evt->button;
    Input_handle_joy(evt->button, 1);
}

void Input_handle_joy_up(SDL_JoyButtonEvent* evt)
{
    Input_handle_joy(evt->button, 0);
}

void Input_handle_joy(const uint8_t button, const uint8_t is_pressed)
{
    if (button == Input_pad_config[0])
        Input_keys[INPUT_ACCEL] = is_pressed;

    if (button == Input_pad_config[1])
        Input_keys[INPUT_BRAKE] = is_pressed;

    if (button == Input_pad_config[2])
        Input_keys[INPUT_GEAR1] = is_pressed;

    if (button == Input_pad_config[3])
        Input_keys[INPUT_GEAR2] = is_pressed;

    if (button == Input_pad_config[4])
        Input_keys[INPUT_START] = is_pressed;

    if (button == Input_pad_config[5])
        Input_keys[INPUT_COIN] = is_pressed;

    if (button == Input_pad_config[6])
        Input_keys[INPUT_MENU] = is_pressed;

    if (button == Input_pad_config[7])
        Input_keys[INPUT_VIEWPOINT] = is_pressed;
}
