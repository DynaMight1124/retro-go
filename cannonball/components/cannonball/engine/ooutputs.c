/***************************************************************************
    Process Outputs.

    - Cabinet Vibration & Hydraulic Movement
    - Brake & Start Lamps
    - Coin Chute Outputs
    
    The Deluxe Motor code is also used by the force-feedback haptic system.

    One thing to note is that this code was originally intended to drive
    a moving hydraulic cabinet, not to be mapped to a haptic device.

    Therefore, it's not perfect when used in this way, but the results
    aren't bad :)
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdlib.h> // abs
#include <stdint.h>
#include "utils.h"

#include "engine/ocrash.h"
#include "engine/oferrari.h"
#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/ooutputs.h"

uint8_t OOutputs_hw_motor_control;
uint8_t OOutputs_dig_out;
CoinChute OOutputs_chute1;
CoinChute OOutputs_chute2;


#define STATE_INIT   0
#define STATE_DELAY  1
#define STATE_LEFT   2
#define STATE_RIGHT  3
#define STATE_CENTRE 4
#define STATE_DONE   5
#define STATE_EXIT   6

// Calibration Counter
const static int COUNTER_RESET = 300;

const static uint8_t MOTOR_OFF    = 0;
const static uint8_t MOTOR_RIGHT  = 0x5;
const static uint8_t MOTOR_CENTRE = 0x8;
const static uint8_t MOTOR_LEFT   = 0xB;
    

// These are calculated during startup in the original game.
// Here we just hardcode them, as the motor init code isn't ported.
const static uint8_t CENTRE_POS    = 0x80;
const static uint8_t LEFT_LIMIT    = 0xC1;
const static uint8_t RIGHT_LIMIT   = 0x3C;

// Motor Limit Values. Calibrated during startup.
int16_t limit_left = 0;
int16_t limit_right = 0;

// Motor Centre Position. (We Fudge this for Force Feedback wheel mode.)
int16_t motor_centre_pos;

// Difference between input_motor and input_motor_old
int16_t motor_x_change;

uint16_t motor_state;
uint8_t motor_enabled = 1;

// 0x11: Motor Control Value
int8_t motor_control;
// 0x12: Movement (1 = Left, -1 = Right, 0 = None)
int8_t motor_movement;
// 0x14: Is Motor Centered
uint8_t is_centered;
// 0x16: Motor X Change Latch
int16_t motor_change_latch;
// 0x18: Speed
int16_t speed;
// 0x1A: Road Curve
int16_t curve;
// 0x1E: Increment counter to index motor table for off-road/crash
int16_t vibrate_counter;
// 0x20: Last Motor X_Change > 8. No need to adjust further.
uint8_t was_small_change;
// 0x22: Adjusted movement value based on steering 1
int16_t movement_adjust1;
// 0x24: Adjusted movement value based on steering 2
int16_t movement_adjust2;
// 0x26: Adjusted movement value based on steering 3
int16_t movement_adjust3;

// Counter control for motor tests
static int16_t counter;

// Columns for output
uint16_t col1 = 0;
uint16_t col2 = 0;


void OOutputs_diag_left(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_diag_right(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_diag_centre(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_diag_done();

void OOutputs_calibrate_left(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_calibrate_right(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_calibrate_centre(int16_t input_motor, uint8_t hw_motor_limit);
void OOutputs_calibrate_done();

void OOutputs_do_motors(int MODE, int16_t input_motor);
void OOutputs_car_moving(const int MODE);
void OOutputs_car_stationary();
void OOutputs_adjust_motor();
void OOutputs_do_motor_crash();
void OOutputs_do_motor_offroad();
void OOutputs_set_value(const uint8_t*, uint8_t);
void OOutputs_done();
void OOutputs_motor_output(uint8_t cmd);

void OOutputs_do_vibrate_upright();
void OOutputs_do_vibrate_mini();


// Initalize Moving Cabinet Motor
// Source: 0xECE8
void OOutputs_init()
{
    OOutputs_chute1.output_bit  = OUTPUTS_D_COIN1_SUCC;
    OOutputs_chute2.output_bit  = OUTPUTS_D_COIN2_SUCC;

    motor_state        = STATE_INIT;
    OOutputs_hw_motor_control   = MOTOR_OFF;
    OOutputs_dig_out   = 0;
    motor_control      = 0;
    motor_movement     = 0;
    is_centered        = 0;
    motor_change_latch = 0;
    speed              = 0;
    curve              = 0;
    counter            = 0;
    vibrate_counter    = 0;
    was_small_change   = 0;
    movement_adjust1   = 0;
    movement_adjust2   = 0;
    movement_adjust3   = 0;
    OOutputs_chute1.counter[0]  = 0;
    OOutputs_chute1.counter[1]  = 0;
    OOutputs_chute1.counter[2]  = 0;
    OOutputs_chute2.counter[0]  = 0;
    OOutputs_chute2.counter[1]  = 0;
    OOutputs_chute2.counter[2]  = 0;
}

void OOutputs_tick(int MODE, int16_t input_motor, int16_t cabinet_type)
{
    switch (MODE)
    {
        // Force Feedback Steering Wheels
        case OUTPUTS_MODE_FFEEDBACK:
            OOutputs_do_motors(MODE, input_motor);   // Use X-Position of wheel instead of motor position
            OOutputs_motor_output(OOutputs_hw_motor_control); // Force Feedback Handling
            break;

        // CannonBoard: Real Cabinet
/*
        case OUTPUTS_MODE_CABINET:
            if (cabinet_type == config.cannonboard.CABINET_MOVING)
            {
                OOutputs_do_motors(MODE, input_motor);
                do_vibrate_mini();
            }
            else if (cabinet_type == config.cannonboard.CABINET_UPRIGHT)
                do_vibrate_upright();
            else if (cabinet_type == config.cannonboard.CABINET_MINI)
                do_vibrate_mini();
            break;
  */
    }
}

// ------------------------------------------------------------------------------------------------
// Digital Outputs
// ------------------------------------------------------------------------------------------------

void OOutputs_set_digital(uint8_t output)
{
    OOutputs_dig_out |= output;   
}

void OOutputs_clear_digital(uint8_t output)
{
    OOutputs_dig_out &= ~output;
}

// ------------------------------------------------------------------------------------------------
// Motor Diagnostics
// Source: 0x1885E
// ------------------------------------------------------------------------------------------------

uint8_t OOutputs_diag_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets)
{
    switch (motor_state)
    {
        // Initalize
        case STATE_INIT:
            col1 = 10;
            col2 = 27;
            OHud_blit_text_new(col1, 9, "LEFT LIMIT", HUD_GREY);
            OHud_blit_text_new(col1, 11, "RIGHT LIMIT", HUD_GREY);
            OHud_blit_text_new(col1, 13, "CENTRE", HUD_GREY);
            OHud_blit_text_new(col1, 16, "MOTOR POSITION", HUD_GREY);
            OHud_blit_text_new(col1, 18, "LIMIT B3 LEFT", HUD_GREY);
            OHud_blit_text_new(col1, 19, "LIMIT B4 CENTRE", HUD_GREY);
            OHud_blit_text_new(col1, 20, "LIMIT B5 RIGHT", HUD_GREY);
            counter          = COUNTER_RESET;
            motor_centre_pos = 0;
            motor_enabled    = 1;
            motor_state = STATE_LEFT;
            break;

        case STATE_LEFT:
            OOutputs_diag_left(input_motor, hw_motor_limit);
            break;

        case STATE_RIGHT:
            OOutputs_diag_right(input_motor, hw_motor_limit);
            break;

        case STATE_CENTRE:
            OOutputs_diag_centre(input_motor, hw_motor_limit);
            break;

        case STATE_DONE:
            OOutputs_diag_done();
            break;
    }

    // Print Motor Position & Limit Switch
    OHud_blit_text_new(col2, 16, "  H", 0x80);
    OHud_blit_text_new(col2, 16, Utils_int_to_string(input_motor), 0x80);
    OHud_blit_text_new(col2, 18, (hw_motor_limit & BIT_3) ? "ON " : "OFF ", 0x80);
    OHud_blit_text_new(col2, 19, (hw_motor_limit & BIT_4) ? "ON " : "OFF ", 0x80);
    OHud_blit_text_new(col2, 20, (hw_motor_limit & BIT_5) ? "ON " : "OFF ", 0x80);
    return motor_state == STATE_DONE;
}

void OOutputs_diag_left(int16_t input_motor, uint8_t hw_motor_limit)
{
    // If Right Limit Set, Move Left
    if (hw_motor_limit & BIT_5)
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = MOTOR_LEFT;
            return;
        }
        // Counter Expired, Left Limit Still Not Reached
        else
            OHud_blit_text_new(col2, 9, "FAIL 1", 0x80);
    }
    // Left Limit Reached
    else if (hw_motor_limit & BIT_3)
    {
        OHud_blit_text_new(col2, 9, "  H", 0x80);
        OHud_blit_text_new(col2, 9, Utils_int_to_string(input_motor), 0x80);
    }
    else
        OHud_blit_text_new(col2, 9, "FAIL 2", 0x80);

    counter          = COUNTER_RESET;
    motor_state      = STATE_RIGHT;
}


void OOutputs_diag_right(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (motor_centre_pos == 0 && ((hw_motor_limit & BIT_4) == 0))
        motor_centre_pos = input_motor;
   
    // If Left Limit Set, Move Right
    if (hw_motor_limit & BIT_3)
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = MOTOR_RIGHT; // Move Right
            return;
        }
        // Counter Expired, Right Limit Still Not Reached
        else
            OHud_blit_text_new(col2, 11, "FAIL 1", 0x80);
    }
    // Right Limit Reached
    else if (hw_motor_limit & BIT_5)
    {
        OHud_blit_text_new(col2, 11, "  H", 0x80);
        OHud_blit_text_new(col2, 11, Utils_int_to_string(input_motor), 0x80);
    }
    else
    {
        OHud_blit_text_new(col2, 11, "FAIL 2", 0x80);
        motor_enabled = 0;
        motor_state   = STATE_DONE;
        return;
    }

    motor_state  = STATE_CENTRE;
    counter      = COUNTER_RESET;
}


void OOutputs_diag_centre(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (hw_motor_limit & BIT_4) 
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = (counter <= COUNTER_RESET/2) ? MOTOR_RIGHT : MOTOR_LEFT; // Move Right
            return;
        }
        else
        {
            OHud_blit_text_new(col2, 13, "FAIL", 0x80);
        }  
    }
    else
    {
        OHud_blit_text_new(col2, 13, "  H", 0x80);
        OHud_blit_text_new(col2, 13, Utils_int_to_string((input_motor + motor_centre_pos) >> 1), 0x86);
        OOutputs_hw_motor_control = MOTOR_OFF; // switch off
        counter          = 32;
        motor_state      = STATE_DONE;
    }
}

void OOutputs_diag_done()
{
    if (counter > 0)
        counter--;

    if (counter == 0)
        OOutputs_hw_motor_control = MOTOR_CENTRE;
}

// ------------------------------------------------------------------------------------------------
// Calibrate Motors
// ------------------------------------------------------------------------------------------------

uint8_t OOutputs_calibrate_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets)
{
    switch (motor_state)
    {
        // Initalize
        case STATE_INIT:
            col1 = 11;
            col2 = 25;
            OHud_blit_text_big(      2,  "MOTOR CALIBRATION", HUD_GREY);
            OHud_blit_text_new(col1, 10, "MOVE LEFT   -", HUD_GREY);
            OHud_blit_text_new(col1, 12, "MOVE RIGHT  -", HUD_GREY);
            OHud_blit_text_new(col1, 14, "MOVE CENTRE -", HUD_GREY);
            counter          = 25;
            motor_centre_pos = 0;
            motor_enabled    = 1;
            motor_state++;
            break;

        // Just a delay to wait for the serial for safety
        case STATE_DELAY:
            if (--counter == 0 || packets > 60)
            {
                counter = COUNTER_RESET;
                motor_state++;
            }
            break;

        // Calibrate Left Limit
        case STATE_LEFT:
            OOutputs_calibrate_left(input_motor, hw_motor_limit);
            break;

        // Calibrate Right Limit
        case STATE_RIGHT:
            OOutputs_calibrate_right(input_motor, hw_motor_limit);
            break;

        // Return to Centre
        case STATE_CENTRE:
            OOutputs_calibrate_centre(input_motor, hw_motor_limit);
            break;

        // Clear Screen & Exit Calibration
        case STATE_DONE:
            OOutputs_calibrate_done();
            break;

        case STATE_EXIT:
            return 1;
    }

    return 0;
}

void OOutputs_calibrate_left(int16_t input_motor, uint8_t hw_motor_limit)
{
    // If Right Limit Set, Move Left
    if (hw_motor_limit & BIT_5)
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = MOTOR_LEFT;
            return;
        }
        // Counter Expired, Left Limit Still Not Reached
        else
        {
            OHud_blit_text_new(col2, 10, "FAIL 1", HUD_GREY);
            motor_centre_pos = 0;
            limit_left       = input_motor; // Set Left Limit
            OOutputs_hw_motor_control = MOTOR_LEFT;  // Move Left
            counter          = COUNTER_RESET;
            motor_state      = STATE_RIGHT;
        }
    }
    // Left Limit Reached
    else if (hw_motor_limit & BIT_3)
    {
        OHud_blit_text_new(col2, 10, Utils_int_to_hex_string(input_motor), 0x80);
        motor_centre_pos = 0;
        limit_left       = input_motor; // Set Left Limit
        OOutputs_hw_motor_control = MOTOR_LEFT;  // Move Left
        counter          = COUNTER_RESET; 
        motor_state      = STATE_RIGHT;
    }
    else
    {
        OHud_blit_text_new(col2, 10, "FAIL 2", HUD_GREY);
        OHud_blit_text_new(col2, 12, "FAIL 2", HUD_GREY);
        motor_enabled = 0; 
        counter       = COUNTER_RESET;
        motor_state   = STATE_CENTRE;
    }
}

void OOutputs_calibrate_right(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (motor_centre_pos == 0 && ((hw_motor_limit & BIT_4) == 0))
    {
        motor_centre_pos = input_motor;
    }
   
    // If Left Limit Set, Move Right
    if (hw_motor_limit & BIT_3)
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = MOTOR_RIGHT; // Move Right
            return;
        }
        // Counter Expired, Right Limit Still Not Reached
        else
        {
            OHud_blit_text_new(col2, 12, "FAIL 1", HUD_GREY);
            limit_right  = input_motor;
            motor_state  = STATE_CENTRE;
            counter      = COUNTER_RESET;
        }
    }
    // Right Limit Reached
    else if (hw_motor_limit & BIT_5)
    {
        OHud_blit_text_new(col2, 12, Utils_int_to_hex_string(input_motor), 0x80);
        limit_right   = input_motor; // Set Right Limit
        motor_state   = STATE_CENTRE;
        counter       = COUNTER_RESET;
    }
    else
    {
        OHud_blit_text_new(col2, 12, "FAIL 2", HUD_GREY);
        motor_enabled = 0;
        motor_state   = STATE_CENTRE;
        counter       = COUNTER_RESET;
    }
}

void OOutputs_calibrate_centre(int16_t input_motor, uint8_t hw_motor_limit)
{
    uint8_t fail = 0;

    if (hw_motor_limit & BIT_4) 
    {
        if (--counter >= 0)
        {
            OOutputs_hw_motor_control = (counter <= COUNTER_RESET/2) ? MOTOR_RIGHT : MOTOR_LEFT; // Move Right
            return;
        }
        else
        {
            OHud_blit_text_new(col2, 14, "FAIL SW", HUD_GREY);
            fail = 1;
            // Fall through to EEB6
        }  
    }
  
    // 0xEEB6:
    motor_centre_pos = (input_motor + motor_centre_pos) >> 1;
  
    int16_t d0 = limit_right - motor_centre_pos;
    int16_t d1 = motor_centre_pos  - limit_left;
  
    // set both to left limit
    if (d0 > d1)
        d1 = d0;

    d0 = d1;
  
    limit_left  = d0 - 6;
    limit_right = -d1 + 6;
    
    if (abs(motor_centre_pos - 0x80) > 0x20)
    {
        OHud_blit_text_new(col2, 14, "FAIL DIST", HUD_GREY);
        motor_enabled = 0;
    }
    else if (!fail)
    {
        OHud_blit_text_new(col2, 14, Utils_int_to_hex_string(motor_centre_pos), 0x80);
    }

    OHud_blit_text_new(13, 17, "TESTS COMPLETE!", 0x82);

    OOutputs_hw_motor_control = MOTOR_OFF; // switch off
    counter          = 90;
    motor_state      = STATE_DONE;
}

void OOutputs_calibrate_done()
{
    if (counter > 0)
        counter--;
    else
        motor_state = STATE_EXIT;
}

// ------------------------------------------------------------------------------------------------
// Moving Cabinet Code
// ------------------------------------------------------------------------------------------------

const static uint8_t MOTOR_VALUES[] = 
{
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 3, 3, 4, 4, 5, 5, 2, 3, 4, 5, 6, 7, 7, 7
};

const static uint8_t MOTOR_VALUES_STATIONARY[] = 
{
    2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

const static uint8_t MOTOR_VALUES_OFFROAD1[] = 
{
    0x8, 0x8, 0x5, 0x5, 0x8, 0x8, 0xB, 0xB, 0x8, 0x8, 0x4, 0x4, 0x8, 0x8, 0xC, 0xC, 
    0x8, 0x8, 0x3, 0x3, 0x8, 0x8, 0xD, 0xD, 0x8, 0x8, 0x2, 0x2, 0x8, 0x8, 0xE, 0xE,
};

const static uint8_t MOTOR_VALUES_OFFROAD2[] = 
{
    0x8, 0x5, 0x8, 0xB, 0x8, 0x5, 0x8, 0xB, 0x8, 0x4, 0x8, 0xC, 0x8, 0x4, 0x8, 0xC,
    0x8, 0x3, 0x8, 0xD, 0x8, 0x3, 0x8, 0xD, 0x8, 0x2, 0x8, 0xE, 0x8, 0x2, 0x8, 0xE,
};

const static uint8_t MOTOR_VALUES_OFFROAD3[] = 
{
    0x8, 0x5, 0x5, 0x8, 0xB, 0x8, 0x0, 0x8, 0x8, 0x4, 0x4, 0x8, 0xC, 0x8, 0x0, 0x8,
    0x8, 0x3, 0x3, 0x8, 0xD, 0x8, 0x0, 0x8, 0x8, 0x2, 0x2, 0x8, 0xE, 0x8, 0x0, 0x8,
};

const static uint8_t MOTOR_VALUES_OFFROAD4[] = 
{
    0x8, 0xB, 0xB, 0x8, 0x5, 0x8, 0x0, 0x8, 0x8, 0xC, 0xC, 0x8, 0x4, 0x8, 0x0, 0x8,
    0x8, 0xD, 0xD, 0x8, 0x3, 0x8, 0x0, 0x8, 0x8, 0xE, 0xE, 0x8, 0x2, 0x8, 0x0, 0x8,
};


// Process Motor Code.
// Note, that only the Deluxe Moving Motor Code is ported for now.
// Source: 0xE644
void OOutputs_do_motors(int MODE, int16_t input_motor)
{
    motor_x_change = -(input_motor - (MODE == OUTPUTS_MODE_FFEEDBACK ? CENTRE_POS : motor_centre_pos));

    if (!motor_enabled)
    {
        OOutputs_done();
        return;
    }

    // In-Game: Test for crash, skidding, whether car is moving
    if (Outrun_game_state == GS_INGAME)
    {
        if (OCrash_crash_counter)
        {
            if ((OInitEngine_car_increment >> 16) <= 0x14)
                OOutputs_car_stationary();
            else
                OOutputs_do_motor_crash();
        }
        else if (OCrash_skid_counter)
        {
            OOutputs_do_motor_crash();
        }
        else
        {
            if ((OInitEngine_car_increment >> 16) <= 0x14)
            {
                if (!was_small_change)
                    OOutputs_done();
                else
                    OOutputs_car_stationary();
            }
            else
                OOutputs_car_moving(MODE);
        }
    }
    // Not In-Game: Act as though car is stationary / moving slow
    else
    {
        OOutputs_car_stationary();
    }
}

// Source: 0xE6DA
void OOutputs_car_moving(const int MODE)
{
    // Motor is currently moving
    if (motor_movement)
    {
        OOutputs_hw_motor_control = motor_control;
        OOutputs_adjust_motor();
        return;
    }
    
    // Motor is not currently moving. Setup new movement as necessary.
    if (OFerrari_wheel_state != WHEELS_ON)
    {
        OOutputs_do_motor_offroad();
        return;
    }

    const uint16_t car_inc = OInitEngine_car_increment >> 16;
    if (car_inc <= 0x64)                    speed = 0;
    else if (car_inc <= 0xA0)               speed = 1 << 3;
    else if (car_inc <= 0xDC)               speed = 2 << 3;
    else                                    speed = 3 << 3;

    if (OInitEngine_road_curve == 0)         curve = 0;
    else if (OInitEngine_road_curve <= 0x3C) curve = 2; // sharp curve
    else if (OInitEngine_road_curve <= 0x5A) curve = 1; // gentle curve
    else                                     curve = 0;

    int16_t steering = OInputs_steering_adjust;
    steering += (movement_adjust1 + movement_adjust2 + movement_adjust3);
    steering >>= 2;
    movement_adjust3 = movement_adjust2;                   // Trickle down values
    movement_adjust2 = movement_adjust1;
    movement_adjust1 = OInputs_steering_adjust;

    // Veer Left
    if (steering >= 0)
    {
        steering = (steering >> 4) - 1;
        if (steering < 0)
        {
            OOutputs_car_stationary();
            return;
        }
                
        if (steering > 0)
            steering--;

        uint8_t motor_value = MOTOR_VALUES[speed + curve];

        if (MODE == OUTPUTS_MODE_FFEEDBACK)
        {
            OOutputs_hw_motor_control = motor_value + 8;
        }
        else
        {
            int16_t change = motor_x_change + (motor_value << 1);
            // Latch left movement
            if (change >= limit_left)
            {
                OOutputs_hw_motor_control   = MOTOR_CENTRE;
                motor_movement     = 1;
                motor_control      = 7;
                motor_change_latch = motor_x_change;
            }
            else
            {
                OOutputs_hw_motor_control = motor_value + 8;
            }
        }
        
        OOutputs_done();
    }
    // Veer Right
    else
    {
        steering = -steering;
        steering = (steering >> 4) - 1;
        if (steering < 0)
        {
            OOutputs_car_stationary();
            return;
        }

        if (steering > 0)
            steering--;

        uint8_t motor_value = MOTOR_VALUES[speed + curve];

        if (MODE == OUTPUTS_MODE_FFEEDBACK)
        {
            OOutputs_hw_motor_control = -motor_value + 8;
        }
        else
        {
            int16_t change = motor_x_change - (motor_value << 1);
            // Latch right movement
            if (change <= limit_right)
            {
                OOutputs_hw_motor_control   = MOTOR_CENTRE;
                motor_movement     = -1;
                motor_control      = 9;
                motor_change_latch = motor_x_change;
            }
            else
            {
                OOutputs_hw_motor_control = -motor_value + 8;
            }
        }
        
        OOutputs_done();
    }
}

// Source: 0xE822
void OOutputs_car_stationary()
{
    int16_t change = abs(motor_x_change);

    if (change <= 8)
    {
        if (!is_centered)
        {
            OOutputs_hw_motor_control = MOTOR_CENTRE;
            is_centered      = 1;
        }
        else
        {
            OOutputs_hw_motor_control = MOTOR_OFF;
            is_centered      = 0;
            OOutputs_done();
        }
    }
    else
    {
        int8_t motor_value = MOTOR_VALUES_STATIONARY[change >> 3];

        if (motor_x_change >= 0)
            motor_value = -motor_value;

        OOutputs_hw_motor_control = motor_value + 8;

        OOutputs_done();
    }
}

// Source: 0xE8DA
void OOutputs_adjust_motor()
{
    int16_t change = motor_change_latch; // d1
    motor_change_latch = motor_x_change;
    change -= motor_x_change;
    if (change < 0) 
        change = -change;

    // no movement
    if (change <= 2)
    {
        motor_movement = 0;
        is_centered    = 1;
    }
    // moving right
    else if (motor_movement < 0)
    {
        if (++motor_control > 10)
            motor_control = 10;
    }
    // moving left
    else 
    {
        if (--motor_control < 6)
            motor_control = 6;
    }

    OOutputs_done();
}

// Adjust motor during crash/skid state
// Source: 0xE994
void OOutputs_do_motor_crash()
{
    if (OFerrari_car_x_diff == 0)
        OOutputs_set_value(MOTOR_VALUES_OFFROAD1, 3);
    else if (OFerrari_car_x_diff < 0)
        OOutputs_set_value(MOTOR_VALUES_OFFROAD4, 3);
    else
        OOutputs_set_value(MOTOR_VALUES_OFFROAD3, 3);
}

// Adjust motor when wheels are off-road
// Source: 0xE9BE
void OOutputs_do_motor_offroad()
{
    const uint8_t* table = (OFerrari_wheel_state != WHEELS_OFF) ? MOTOR_VALUES_OFFROAD2 : MOTOR_VALUES_OFFROAD1;

    const uint16_t car_inc = OInitEngine_car_increment >> 16;
    uint8_t index;
    if (car_inc <= 0x32)      index = 0;
    else if (car_inc <= 0x50) index = 1;
    else if (car_inc <= 0x6E) index = 2;
    else                      index = 3;

    OOutputs_set_value(table, index);
}

void OOutputs_set_value(const uint8_t* table, uint8_t index)
{
    OOutputs_hw_motor_control = table[(index << 3) + (counter & 7)];
    counter++;
    OOutputs_done();
}

// Source: 0xE94E
void OOutputs_done()
{
    if (abs(motor_x_change) <= 8)
    {
        was_small_change = 1;
        motor_control    = MOTOR_CENTRE;
    }
    else
    {
        was_small_change = 0;
    }
}

// Send output commands to motor hardware
// This is the equivalent to writing to register 0x140003
void OOutputs_motor_output(uint8_t cmd)
{
    if (cmd == MOTOR_OFF || cmd == MOTOR_CENTRE)
        return;

    int8_t force = 0;

    if (cmd < MOTOR_CENTRE)      // left
        force = cmd - 1;
    else if (cmd > MOTOR_CENTRE) // right
        force = 15 - cmd;

//  No DirectX availble in C.
//    forcefeedback::set(cmd, force);
}

// ------------------------------------------------------------------------------------------------
// Deluxe Upright: Steering Wheel Movement
// ------------------------------------------------------------------------------------------------

// Deluxe Upright: Vibration Enable Table. 4 Groups of vibration values.
const static uint8_t VIBRATE_LOOKUP[] = 
{
    // SLOW SPEED --------   // MEDIUM SPEED ------
    1, 0, 0, 0, 1, 0, 0, 0,  1, 1, 0, 0, 1, 1, 0, 0,
    // FAST SPEED --------   // VERY FAST SPEED ---
    1, 1, 1, 0, 1, 1, 1, 0,  1, 1, 1, 1, 1, 1, 1, 1,
};

// Source: 0xEAAA
void OOutputs_do_vibrate_upright()
{
    if (Outrun_game_state != GS_INGAME)
    {
        OOutputs_clear_digital(OUTPUTS_D_MOTOR);
        return;
    }

    const uint16_t speed = OInitEngine_car_increment >> 16;
    uint16_t index = 0;

    // Car Crashing: Diable Motor once speed below 10
    if (OCrash_crash_counter)
    {
        if (speed <= 10)
        {
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);
            return;
        }
    }
    // Car Normal
    else if (!OCrash_skid_counter)
    {
        // 0xEAE2: Disable Vibration once speed below 30 or wheels on-road
        if (speed < 30 || OFerrari_wheel_state == WHEELS_ON)
        {
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);
            return;
        }

        // 0xEAFC: Both wheels off-road. Faster the car speed, greater the chance of vibrating
        if (OFerrari_wheel_state == WHEELS_OFF)
        {
            if (speed > 220)      index = 3;
            else if (speed > 170) index = 2;
            else if (speed > 120) index = 1;
        }
        // 0xEB38: One wheel off-road. Faster the car speed, greater the chance of vibrating
        else
        {
            if (speed > 270)      index = 3;
            else if (speed > 210) index = 2;
            else if (speed > 150) index = 1;
        }

        if (VIBRATE_LOOKUP[ (vibrate_counter & 7) + (index << 3) ])
            OOutputs_set_digital(OUTPUTS_D_MOTOR);
        else
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);

        vibrate_counter++;
        return;
    }
    // 0xEB68: Car Crashing or Skidding
    if (speed > 140)      index = 3;
    else if (speed > 100) index = 2;
    else if (speed > 60)  index = 1;

    if (VIBRATE_LOOKUP[ (vibrate_counter & 7) + (index << 3) ])
        OOutputs_set_digital(OUTPUTS_D_MOTOR);
    else
        OOutputs_clear_digital(OUTPUTS_D_MOTOR);

    vibrate_counter++;
}

// ------------------------------------------------------------------------------------------------
// Mini Upright: Steering Wheel Movement
// ------------------------------------------------------------------------------------------------

void OOutputs_do_vibrate_mini()
{
    if (Outrun_game_state != GS_INGAME)
    {
        OOutputs_clear_digital(OUTPUTS_D_MOTOR);
        return;
    }

    const uint16_t speed = OInitEngine_car_increment >> 16;
    uint16_t index = 0;

    // Car Crashing: Diable Motor once speed below 10
    if (OCrash_crash_counter)
    {
        if (speed <= 10)
        {
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);
            return;
        }
    }
    // Car Normal
    else if (!OCrash_skid_counter)
    {
        if (speed < 10 || OFerrari_wheel_state == WHEELS_ON)
        {
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);
            return;
        }  

        if (speed > 140)      index = 5;
        else if (speed > 100) index = 4;
        else if (speed > 60)  index = 3;
        else if (speed > 20)  index = 2;
        else                  index = 1;

        if (index > vibrate_counter)
        {
            vibrate_counter = 0;
            OOutputs_clear_digital(OUTPUTS_D_MOTOR);
        }
        else
        {
            vibrate_counter++;
            OOutputs_set_digital(OUTPUTS_D_MOTOR);
        }
        return;
    }

    // 0xEC7A calc_crash_skid:
    if (speed > 90)      index = 4;
    else if (speed > 70) index = 3;
    else if (speed > 50) index = 2;
    else if (speed > 30) index = 1;
    if (index > vibrate_counter)
    {
        vibrate_counter = 0;
        OOutputs_clear_digital(OUTPUTS_D_MOTOR);
    }
    else
    {
        vibrate_counter++;
        OOutputs_set_digital(OUTPUTS_D_MOTOR);
    }
}

// ------------------------------------------------------------------------------------------------
// Coin Chute Output
// Source: 0x6F8C
// ------------------------------------------------------------------------------------------------

void OOutputs_coin_chute_out(CoinChute* chute, uint8_t insert)
{
    // Initalize counter if coin inserted
    chute->counter[2] = insert ? 1 : 0;

    if (chute->counter[0])
    {
        if (--chute->counter[0] != 0)
            return;
        chute->counter[1] = 6;
        OOutputs_clear_digital(chute->output_bit);
    }
    else if (chute->counter[1])
    {
        chute->counter[1]--;
    }
    // Coin first inserted. Called Once. 
    else if (chute->counter[2])
    {
        chute->counter[2]--;
        chute->counter[0] = 6;
        OOutputs_set_digital(chute->output_bit);
    }
}
