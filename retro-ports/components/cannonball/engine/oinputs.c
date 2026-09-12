/***************************************************************************
    Process Inputs.
    
    - Read & Process inputs and controls.
    - Note, this class does not contain platform specific code.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/ocrash.h"
#include "engine/oinputs.h"
#include "engine/ostats.h"

int8_t OInputs_crash_input;

// Acceleration Input
int16_t OInputs_input_acc;

// Steering Input
int16_t OInputs_input_steering;

// Processed / Adjusted Values
int16_t OInputs_steering_adjust;
int16_t OInputs_acc_adjust;
int16_t OInputs_brake_adjust;
    
// True = High Gear. False = Low Gear.
uint8_t OInputs_gear;



// ------------------------------------------------------------------------
// Variables for port
// ------------------------------------------------------------------------

// Amount to adjust steering per tick. (0x3 is a good test value)
uint8_t steering_inc;

// Amount to adjust acceleration per tick. (0x10 is a good test value)
uint8_t acc_inc;

// Amount to adjust brake per tick. (0x10 is a good test value)
uint8_t brake_inc;

static const int DELAY_RESET = 60;
int delay1, delay2, delay3;

// Coin Inputs (Only used by CannonBoard)
uint8_t coin1, coin2;

// ------------------------------------------------------------------------
// Variables from original code
// ------------------------------------------------------------------------

const static uint8_t STEERING_MIN = 0x48;
const static uint8_t STEERING_MAX = 0xB8;
const static uint8_t STEERING_CENTRE = 0x80;
    
// Current steering value
int16_t steering_old;
int16_t steering_change;

const static uint8_t PEDAL_MIN = 0x30;
const static uint8_t PEDAL_MAX = 0x90;

// Brake Input
int16_t input_brake;

void OInputs_digital_steering();
void OInputs_digital_pedals();

void OInputs_init()
{
    OInputs_input_steering  = STEERING_CENTRE;
    steering_old    = STEERING_CENTRE;
    OInputs_steering_adjust = 0;
    OInputs_acc_adjust      = 0;
    OInputs_brake_adjust    = 0;
    steering_change = 0;

    steering_inc = Config_controls.steer_speed;
    acc_inc      = Config_controls.pedal_speed * 4;
    brake_inc    = Config_controls.pedal_speed * 4;

    OInputs_input_acc   = 0;
    input_brake = 0;
    OInputs_gear        = 0;
    OInputs_crash_input = 0;
    delay1      = 0;
    delay2      = 0;
    delay3      = 0;
    coin1       = 0;
    coin2       = 0;
}

void OInputs_tick(Packet* packet)
{
// CannonBoard Input
    if (packet != NULL)
    {
        OInputs_input_steering = packet->ai2;
        OInputs_input_acc      = packet->ai0;
        input_brake    = packet->ai3;
            
        if (Config_controls.gear != CONTROLS_GEAR_AUTO)
            OInputs_gear       = (packet->di1 & 0x10) == 0;

        // Coin Chutes
        coin1 = (packet->di1 & 0x40) != 0;
        coin2 = (packet->di1 & 0x80) != 0;

        // Service
        Input_keys[INPUT_COIN]  = (packet->di1 & 0x04) != 0;
        // Start
        Input_keys[INPUT_START] = (packet->di1 & 0x08) != 0;
    }

    // Standard PC Keyboard/Joypad/Wheel Input
    else
    {
        // Digital Controls: Simulate Analog
        if (!Input_analog || !Input_gamepad)
        {
            OInputs_digital_steering();
            OInputs_digital_pedals();
        }
        // Analog Controls
        else
        {
            OInputs_input_steering = Input_a_wheel;

            // Analog Pedals
            if (Input_analog == 1)
            {
                OInputs_input_acc = Input_a_accel;
                input_brake = Input_a_brake;
            }
            // Digital Pedals
            else
            {
                OInputs_digital_pedals();
            }
        }
    }
}
// DIGITAL CONTROLS: Digital Simulation of analog steering
void OInputs_digital_steering()
{
    // ------------------------------------------------------------------------
    // STEERING
    // ------------------------------------------------------------------------
    if (Input_is_pressed(INPUT_LEFT))
    {
        // Recentre wheel immediately if facing other way
        if (OInputs_input_steering > STEERING_CENTRE) OInputs_input_steering = STEERING_CENTRE;

        OInputs_input_steering -= steering_inc;
        if (OInputs_input_steering < STEERING_MIN) OInputs_input_steering = STEERING_MIN;
    }
    else if (Input_is_pressed(INPUT_RIGHT))
    {
        // Recentre wheel immediately if facing other way
        if (OInputs_input_steering < STEERING_CENTRE) OInputs_input_steering = STEERING_CENTRE;

        OInputs_input_steering += steering_inc;
        if (OInputs_input_steering > STEERING_MAX) OInputs_input_steering = STEERING_MAX;
    }
    // Return steering to centre if nothing pressed
    else
    {
        if (OInputs_input_steering < STEERING_CENTRE)
        {
            OInputs_input_steering += steering_inc;
            if (OInputs_input_steering > STEERING_CENTRE)
                OInputs_input_steering = STEERING_CENTRE;
        }
        else if (OInputs_input_steering > STEERING_CENTRE)
        {
            OInputs_input_steering -= steering_inc;
            if (OInputs_input_steering < STEERING_CENTRE)
                OInputs_input_steering = STEERING_CENTRE;
        }
    }
}

// DIGITAL CONTROLS: Digital Simulation of analog pedals
void OInputs_digital_pedals()
{
    // ------------------------------------------------------------------------
    // ACCELERATION
    // ------------------------------------------------------------------------

    if (Input_is_pressed(INPUT_ACCEL))
    {
        OInputs_input_acc += acc_inc;
        if (OInputs_input_acc > 0xFF) OInputs_input_acc = 0xFF;
    }
    else
    {
        OInputs_input_acc -= acc_inc;
        if (OInputs_input_acc < 0) OInputs_input_acc = 0;
    }

    // ------------------------------------------------------------------------
    // BRAKE
    // ------------------------------------------------------------------------

    if (Input_is_pressed(INPUT_BRAKE))
    {
        input_brake += brake_inc;
        if (input_brake > 0xFF) input_brake = 0xFF;
    }
    else
    {
        input_brake -= brake_inc;
        if (input_brake < 0) input_brake = 0;
    }
}

void OInputs_do_gear()
{
    // ------------------------------------------------------------------------
    // GEAR SHIFT
    // ------------------------------------------------------------------------

    // Automatic Gears: Don't do anything
    if (Config_controls.gear == CONTROLS_GEAR_AUTO)
        return;

    else
    {
        // Manual: Cabinet Shifter
        if (Config_controls.gear == CONTROLS_GEAR_PRESS)
            OInputs_gear = !Input_is_pressed(INPUT_GEAR1);

        // Manual: Two Separate Buttons for gears
        else if (Config_controls.gear == CONTROLS_GEAR_SEPARATE)
        {
            if (Input_has_pressed(INPUT_GEAR1))
                OInputs_gear = 0;
            else if (Input_has_pressed(INPUT_GEAR2))
                OInputs_gear = 1;
        }

        // Manual: Keyboard/Digital Button
        else
        {
            if (Input_has_pressed(INPUT_GEAR1))
                OInputs_gear = !OInputs_gear;
        }
    }
}

// Adjust Analogue Inputs
//
// Read, Adjust & Write Analogue Inputs
// In the original, these values are set during a H-Blank routine
//
// Source: 74E2

void OInputs_adjust_inputs()
{
    // Cap Steering Value
    if (OInputs_input_steering < STEERING_MIN) OInputs_input_steering = STEERING_MIN;
    else if (OInputs_input_steering > STEERING_MAX) OInputs_input_steering = STEERING_MAX;

    if (OInputs_crash_input)
    {
        OInputs_crash_input--;
        int16_t d0 = ((OInputs_input_steering - 0x80) * 0x100) / 0x70;
        if (d0 > 0x7F) d0 = 0x7F;
        else if (d0 < -0x7F) d0 = -0x7F;
        OInputs_steering_adjust = OCrash_crash_counter ? 0 : d0;
    }
    else
    {
        // no_crash1:
        int16_t d0 = OInputs_input_steering - steering_old;
        steering_old = OInputs_input_steering;
        steering_change += d0;
        d0 = steering_change < 0 ? -steering_change : steering_change;

        // Note the below line "if (d0 > 2)" causes a bug in the original game
        // whereby if you hold the wheel to the left whilst stationary, then accelerate the car will veer left even
        // when the wheel has been centered
        if (Config_engine.fix_bugs || d0 > 2)
        {
            steering_change = 0;
            // Convert input steering value to internal value
            d0 = ((OInputs_input_steering - 0x80) * 0x100) / 0x70;
            if (d0 > 0x7F) d0 = 0x7F;
            else if (d0 < -0x7F) d0 = -0x7F;
            OInputs_steering_adjust = OCrash_crash_counter ? 0 : d0;
        }
    }

    // Cap & Adjust Acceleration Value
    int16_t acc = OInputs_input_acc;
    if (acc > PEDAL_MAX) acc = PEDAL_MAX;
    else if (acc < PEDAL_MIN) acc = PEDAL_MIN;
    OInputs_acc_adjust = ((acc - 0x30) * 0x100) / 0x61;

    // Cap & Adjust Brake Value
    int16_t brake = input_brake;
    if (brake > PEDAL_MAX) brake = PEDAL_MAX;
    else if (brake < PEDAL_MIN) brake = PEDAL_MIN;
    OInputs_brake_adjust = ((brake - 0x30) * 0x100) / 0x61;
}

// Simplified version of do credits routine. 
// I have not ported the coin chute handling code, or dip switch routines.
//
// Returns: 0 (No Coin Inserted)
//          1 (Coin Chute 1 Used)
//          2 (Coin Chute 2 Used)
//          3 (Key Pressed / Service Button)
//
// Source: 0x6DE0
uint8_t OInputs_do_credits()
{
    if (Input_has_pressed(INPUT_COIN))
    {
        Input_keys[INPUT_COIN] = 0; // immediately clear due to this routine being in vertical interrupt
        if (!Config_engine.freeplay && OStats_credits < 9)
        {
            OStats_credits++;
            // todo: Increment credits total for bookkeeping
            OSoundInt_queue_sound(sound_COIN_IN);
        }
        return 3;
    }
    else if (coin1)
    {
        coin1 = 0;
        if (!Config_engine.freeplay && OStats_credits < 9)
        {
            OStats_credits++;
            OSoundInt_queue_sound(sound_COIN_IN);
        }
        return 1;
    }
    else if (coin2)
    {
        coin2 = 0;
        if (!Config_engine.freeplay && OStats_credits < 9)
        {
            OStats_credits++;
            OSoundInt_queue_sound(sound_COIN_IN);
        }
        return 2;
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------
// Menu Selection Controls
// ------------------------------------------------------------------------------------------------

uint8_t OInputs_is_analog_l()
{
    if (OInputs_input_steering < STEERING_CENTRE - 0x10)
    {
        if (--delay1 < 0)
        {
            delay1 = DELAY_RESET;
            return 1;
        }
    }
    else
        delay1 = DELAY_RESET;
    return 0;
}

uint8_t OInputs_is_analog_r()
{
    if (OInputs_input_steering > STEERING_CENTRE + 0x10)
    {
        if (--delay2 < 0)
        {
            delay2 = DELAY_RESET;
            return 1;
        }
    }
    else
        delay2 = DELAY_RESET;
    return 0;
}

uint8_t OInputs_is_analog_select()
{
    if (OInputs_input_acc > 0x90)
    {
        if (--delay3 < 0)
        {
            delay3 = DELAY_RESET;
            return 1;
        }
    }
    else
        delay3 = DELAY_RESET;
    return 0;
}
