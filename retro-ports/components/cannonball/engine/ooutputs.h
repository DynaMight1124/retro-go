/***************************************************************************
    Process Outputs.
    
    - Only the Deluxe Moving Motor Code is ported for now.
    - This is used by the force-feedback haptic system.

    One thing to note is that this code was originally intended to drive
    a moving hydraulic cabinet, not to be mapped to a haptic device.

    Therefore, it's not perfect when used in this way, but the results
    aren't bad :)
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <stdint.h>

typedef struct
{
    // Coin Chute Counters
    uint8_t counter[3];
    // Output bit
    uint8_t output_bit;
} CoinChute;


#define OUTPUTS_MODE_FFEEDBACK 0
#define OUTPUTS_MODE_CABINET   1

// Hardware Motor Control:
// 0 = Switch off
// 5 = Left
// 8 = Centre
// B = Right
extern uint8_t OOutputs_hw_motor_control;

// Digital Outputs
enum
{
    OUTPUTS_D_EXT_MUTE   = 0x01, // bit 0 = External Amplifier Mute Control
    OUTPUTS_D_BRAKE_LAMP = 0x02, // bit 1 = brake lamp
    OUTPUTS_D_START_LAMP = 0x04, // bit 2 = start lamp
    OUTPUTS_D_COIN1_SUCC = 0x08, // bit 3 = Coin successfully inserted - Chute 2
    OUTPUTS_D_COIN2_SUCC = 0x10, // bit 4 = Coin successfully inserted - Chute 1
    OUTPUTS_D_MOTOR      = 0x20, // bit 5 = steering wheel central vibration
    OUTPUTS_D_UNUSED     = 0x40, // bit 6 = ?
    OUTPUTS_D_SOUND      = 0x80, // bit 7 = sound enable
};

extern uint8_t OOutputs_dig_out;

extern CoinChute OOutputs_chute1;
extern CoinChute OOutputs_chute2;


void OOutputs_init();
uint8_t OOutputs_diag_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets);
uint8_t OOutputs_calibrate_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets);
void OOutputs_tick(int MODE, int16_t input_motor, int16_t cabinet_type); 
void OOutputs_set_digital(uint8_t);
void OOutputs_clear_digital(uint8_t);
void OOutputs_coin_chute_out(CoinChute* chute, uint8_t insert);
