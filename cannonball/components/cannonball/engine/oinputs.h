/***************************************************************************
    Process Inputs.
    
    - Read & Process inputs and controls.
    - Note, this class does not contain platform specific code.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "sdl/input.h"
#include "outrun.h"

const static uint8_t INPUTS_BRAKE_THRESHOLD1 = 0x80;
const static uint8_t INPUTS_BRAKE_THRESHOLD2 = 0xA0;
const static uint8_t INPUTS_BRAKE_THRESHOLD3 = 0xC0;
const static uint8_t INPUTS_BRAKE_THRESHOLD4 = 0xE0;

extern int8_t OInputs_crash_input;

// Acceleration Input
extern int16_t OInputs_input_acc;

// Steering Input
extern int16_t OInputs_input_steering;

// Processed / Adjusted Values
extern int16_t OInputs_steering_adjust;
extern int16_t OInputs_acc_adjust;
extern int16_t OInputs_brake_adjust;
    
// True = High Gear. False = Low Gear.
extern uint8_t OInputs_gear;

void OInputs_init();
void OInputs_tick(Packet* packet);
void OInputs_adjust_inputs();
void OInputs_do_gear();
uint8_t OInputs_do_credits();

uint8_t OInputs_is_analog_l();
uint8_t OInputs_is_analog_r();
uint8_t OInputs_is_analog_select();

