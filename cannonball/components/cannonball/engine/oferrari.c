/***************************************************************************
    Ferrari Rendering & Handling Code.
       
    Much of the handling code is very messy. As such, the translated code 
    isn't great as I tried to focus on accuracy rather than refactoring.
    
    A good example of the randomness is a routine I've named
      do_sound_score_slip()
    which performs everything from updating the score, setting the audio
    engine tone, triggering smoke effects etc. in an interwoven fashion.
    
    The Ferrari sprite has different properties to other game objects
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/oanimseq.h"
#include "engine/oattractai.h"
#include "engine/obonus.h"
#include "engine/ocrash.h"
#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/olevelobjs.h"
#include "engine/ooutputs.h"
#include "engine/ostats.h"
#include "engine/outils.h"
#include "globals.h"
#include "engine/oferrari.h"


oentry *OFerrari_spr_ferrari;
oentry *OFerrari_spr_pass1;
oentry *OFerrari_spr_pass2;
oentry *OFerrari_spr_shadow;
uint8_t OFerrari_state; 
uint16_t OFerrari_counter;
int16_t OFerrari_steering_old;
uint8_t OFerrari_car_ctrl_active;
int8_t OFerrari_car_state;
uint8_t OFerrari_auto_brake;
uint8_t OFerrari_torque_index; 
int16_t OFerrari_torque;
int32_t OFerrari_revs;
uint8_t OFerrari_rev_shift;
uint8_t OFerrari_wheel_state;
uint8_t OFerrari_wheel_traction;
uint16_t OFerrari_is_slipping;
uint8_t OFerrari_slip_sound;
uint16_t OFerrari_car_inc_old;
int16_t OFerrari_car_x_diff;
int16_t OFerrari_rev_stop_flag;
int16_t OFerrari_revs_post_stop;
int16_t OFerrari_acc_post_stop;
uint16_t OFerrari_rev_pitch1;
uint16_t OFerrari_rev_pitch2;
int16_t OFerrari_sprite_ai_counter;
int16_t OFerrari_sprite_ai_curve;
int16_t OFerrari_sprite_ai_x;
int16_t OFerrari_sprite_ai_steer;
int16_t OFerrari_sprite_car_x_bak;
int16_t OFerrari_sprite_wheel_state;
int16_t OFerrari_sprite_slip_copy;
int8_t OFerrari_wheel_pal;
int16_t OFerrari_sprite_pass_y;
int16_t OFerrari_wheel_frame_reset;
int16_t OFerrari_wheel_counter;



// Max speed of car
#define MAX_SPEED 0x1260000

// Car Base Increment, For Movement
const static uint32_t CAR_BASE_INC = 0x12F;

// Maximum distance to allow car to stray from road
const static uint16_t OFFROAD_BOUNDS = 0x1F4;

// Used by set_car_x
int16_t road_width_old;

// -------------------------------------------------------------------------
// Controls
// -------------------------------------------------------------------------
    
int16_t accel_value;
int16_t accel_value_bak;
int16_t brake_value;
uint8_t gear_value;
uint8_t gear_bak;

// Trickle down adjusted acceleration values
int16_t acc_adjust1;
int16_t acc_adjust2;
int16_t acc_adjust3;

// Trickle down brake values
int16_t brake_adjust1;
int16_t brake_adjust2;
int16_t brake_adjust3;

// Calculated brake value to subtract from acc_burst.
int32_t brake_subtract;

// Counter. When enabled, acceleration disabled
int8_t gear_counter;

// Previous rev adjustment (stored)
int32_t rev_adjust;
    
// -------------------------------------------------------------------------
// Smoke
// -------------------------------------------------------------------------

// Counter for smoke after changing gear. Values over 0 result in smoke
int16_t gear_smoke;

// Similar to above
int16_t gfx_smoke;

// Set to -1 when car sharply corners and player is steering into direction of corner
int8_t cornering;
int8_t cornering_old;

// Rev Lookup Table. 255 Values.
// Used to provide rev adjustment. Note that values tail off at higher speeds.
const uint8_t OFerrari_rev_inc_lookup[] = 
{
    0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17, 
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
    0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 
    0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 
    0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 
    0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 
    0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 
    0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x20, 0x20, 0x20, 0x20, 
    0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 
    0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2E, 0x2E, 0x2E, 
    0x2F, 0x2F, 0x2F, 0x2F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2F, 0x2F, 0x2F, 0x2F, 
    0x2E, 0x2E, 0x2E, 0x2D, 0x2D, 0x2D, 0x2C, 0x2C, 0x2B, 0x2B, 0x2A, 0x2A, 0x29, 0x29, 0x28, 0x28, 
    0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18, 
    0x17, 0x15, 0x13, 0x11, 0x0F, 0x0D, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x06, 0x06, 
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01
};


const uint16_t OFerrari_torque_lookup[] = 
{
    0x2600, // Offset 0x00 - Start line                                                           
    0x243C, // ..
    0x2278, // values only used when pulling away from start line
    0x20B4,
    0x1EF0,
    0x1D2C,
    0x1B68,
    0x19A4,
    0x17E0,
    0x161C,
    0x1458,
    0x1294,
    0x10D0,
    0xF0C,
    0xD48,
    0xB84,
    0x9BB, // Offset 0x10 - Low Gear
    0x983, // ..
    0x94B, // .. in between these values
    0x913, // .. is the lookup as we shift between
    0x8DB, // .. the two gears
    0x8A3, // ..
    0x86B,
    0x833,
    0x7FB,
    0x7C2,
    0x789,
    0x750,
    0x717,
    0x6DE,
    0x6A5,
    0x66C, // Offset 0x1F - High Gear
};

void OFerrari_logic();
void OFerrari_ferrari_normal();
void OFerrari_setup_ferrari_sprite();
void OFerrari_setup_ferrari_bonus_sprite();
void OFerrari_init_end_seq();
void OFerrari_do_end_seq();
void OFerrari_tick_engine_disabled(int32_t*);
void OFerrari_set_ferrari_palette();
void OFerrari_set_passenger_sprite(oentry*);
void OFerrari_set_passenger_frame(oentry*);
void OFerrari_car_acc_brake();
void OFerrari_do_gear_torque(int16_t*);
void OFerrari_do_gear_low(int16_t*);
void OFerrari_do_gear_high(int16_t*);
int32_t OFerrari_tick_gear_change(int16_t);
int32_t OFerrari_get_speed_inc_value(uint16_t, uint32_t);
int32_t OFerrari_get_speed_dec_value(uint16_t);
void OFerrari_set_brake_subtract();
void OFerrari_finalise_revs(int32_t*, int32_t);
void OFerrari_convert_revs_speed(int32_t, int32_t*);
void OFerrari_update_road_pos();
int32_t OFerrari_tick_smoke();
void OFerrari_set_wheels(uint8_t);
 void OFerrari_draw_sprite(oentry*);


void OFerrari_init(oentry *f, oentry *p1, oentry *p2, oentry *s)
{
    OFerrari_state       = FERRARI_SEQ1;
    OFerrari_spr_ferrari = f;
    OFerrari_spr_pass1   = p1;
    OFerrari_spr_pass2   = p2;
    OFerrari_spr_shadow  = s;

    OFerrari_spr_ferrari->control |= SPRITES_ENABLE;
    OFerrari_spr_pass1->control   |= SPRITES_ENABLE;
    OFerrari_spr_pass2->control   |= SPRITES_ENABLE;
    OFerrari_spr_shadow->control  |= SPRITES_ENABLE;

    OFerrari_state             = 0;
    OFerrari_counter           = 0;
    OFerrari_steering_old      = 0;
    road_width_old             = 0;
    OFerrari_car_state         = CAR_NORMAL;
    OFerrari_auto_brake        = 0;
    OFerrari_torque_index      = 0;
    OFerrari_torque            = 0;
    OFerrari_revs              = 0;
    OFerrari_rev_shift         = 0;
    OFerrari_wheel_state       = WHEELS_ON;
    OFerrari_wheel_traction    = TRACTION_ON;
    OFerrari_is_slipping       = 0;
    OFerrari_slip_sound        = 0;
    OFerrari_car_inc_old       = 0;
    OFerrari_car_x_diff        = 0;
    OFerrari_rev_stop_flag     = 0;
    OFerrari_revs_post_stop    = 0;
    OFerrari_acc_post_stop     = 0;
    OFerrari_rev_pitch1        = 0;
    OFerrari_rev_pitch2        = 0;
    OFerrari_sprite_ai_counter = 0;
    OFerrari_sprite_ai_curve   = 0;
    OFerrari_sprite_ai_x       = 0;
    OFerrari_sprite_ai_steer   = 0;
    OFerrari_sprite_car_x_bak  = 0;
    OFerrari_sprite_wheel_state= 0;
    OFerrari_sprite_slip_copy  = 0;
    OFerrari_wheel_pal         = 0;
    OFerrari_sprite_pass_y     = 0;
    OFerrari_wheel_frame_reset = 0;
    OFerrari_wheel_counter     = 0;
    
    road_width_old    = 0;
    accel_value       = 0;
    accel_value_bak   = 0;
    brake_value       = 0;
    gear_value        = 0;
    gear_bak          = 0;
    brake_subtract    = 0;
    gear_counter      = 0;
    rev_adjust        = 0;
    gear_smoke        = 0;
    gfx_smoke         = 0;
    cornering         = 0;
    cornering_old     = 0;
    OFerrari_car_ctrl_active   = 1;
}

// Reset all values relating to car speed, revs etc.
// Source: 0x61F2
void OFerrari_reset_car()
{
    OFerrari_rev_shift            = 1;  // Set normal rev shift value
    OCrash_spin_control2 = 0;
    OFerrari_revs                 = 0;
    OInitEngine_car_increment = 0;
    gear_value           = 0;
    gear_bak             = 0;
    rev_adjust           = 0;
    OFerrari_car_inc_old          = 0;
    OFerrari_torque               = 0x1000;
    OFerrari_torque_index         = 0x1F;
    OFerrari_rev_stop_flag        = 0;
    OInitEngine_ingame_engine = 0;
    OInitEngine_ingame_counter = 0x1E; // Set ingame counter (time until we hand control to user)
    OFerrari_slip_sound           = sound_STOP_SLIP;
    acc_adjust1          = 
    acc_adjust2          = 
    acc_adjust3          = 0;
    brake_adjust1        = 
    brake_adjust2        = 
    brake_adjust3        = 0;
    OFerrari_auto_brake           = 0;
    OFerrari_counter              = 0;
    OFerrari_is_slipping          = 0;    // Denote not slipping/skidding
}

void OFerrari_tick()
{
    switch (OFerrari_state)
    {
        case FERRARI_SEQ1:
            OAnimSeq_ferrari_seq();
            OAnimSeq_anim_seq_intro(&OAnimSeq_anim_pass1);
            OAnimSeq_anim_seq_intro(&OAnimSeq_anim_pass2);
            break;

        case FERRARI_SEQ2:
            OAnimSeq_anim_seq_intro(&OAnimSeq_anim_ferrari);
            OAnimSeq_anim_seq_intro(&OAnimSeq_anim_pass1);
            OAnimSeq_anim_seq_intro(&OAnimSeq_anim_pass2);
            break;

        case FERRARI_INIT:
            if (OFerrari_spr_ferrari->control & SPRITES_ENABLE) 
                if (Outrun_tick_frame)
                    OFerrari_init_ingame();
            break;

        case FERRARI_LOGIC:
            if (OFerrari_spr_ferrari->control & SPRITES_ENABLE) 
            {
                if (Outrun_tick_frame)
                    OFerrari_logic();
                else
                    OFerrari_draw_sprite(OFerrari_spr_ferrari);
            }
            if (OFerrari_spr_pass1->control & SPRITES_ENABLE) 
            {
                if (Outrun_tick_frame)
                    OFerrari_set_passenger_sprite(OFerrari_spr_pass1);
                else
                    OFerrari_draw_sprite(OFerrari_spr_pass1);
            }

            if (OFerrari_spr_pass2->control & SPRITES_ENABLE)
            {
                if (Outrun_tick_frame)
                    OFerrari_set_passenger_sprite(OFerrari_spr_pass2);
                else
                    OFerrari_draw_sprite(OFerrari_spr_pass2);
            }
            break;

        // Ferrari End Sequence Logic
        case FERRARI_END_SEQ:
                OAnimSeq_tick_end_seq();
            break;
    }
}

// Initalize Ferrari Start Sequence
//
// Source: 6036
//
// Note the remainder of this block is handled in OAnimSeq_ferrari_seq
void OFerrari_init_ingame()
{
    // turn_off:
    OFerrari_car_state = CAR_NORMAL;
    OFerrari_state = FERRARI_LOGIC;
    OFerrari_spr_ferrari->reload = 0;
    OFerrari_spr_ferrari->counter = 0;
    OFerrari_sprite_ai_counter = 0;
    OFerrari_sprite_ai_curve = 0;
    OFerrari_sprite_ai_x = 0;
    OFerrari_sprite_ai_steer = 0;
    OFerrari_sprite_car_x_bak = 0;
    OFerrari_sprite_wheel_state = 0;

    // Passengers
    // move.l  #SetPassengerSprite,$42(a5)
    OFerrari_spr_pass1->reload = 0;
    OFerrari_spr_pass1->counter = 0;
    OFerrari_spr_pass1->xw1 = 0;
    // move.l  #SetPassengerSprite,$82(a5)
    OFerrari_spr_pass2->reload = 0;
    OFerrari_spr_pass2->counter = 0;
    OFerrari_spr_pass2->xw1 = 0;
}

// Source: 9C84
void OFerrari_logic()
{
    switch (OBonus_bonus_control)
    {
        // Not Bonus Mode
        case BONUS_DISABLE:
            OFerrari_ferrari_normal();
            break;

        case BONUS_INIT:
            OFerrari_rev_shift = 2;          // Set Double Rev Shift Value
            OBonus_bonus_control = BONUS_TICK;

        case BONUS_TICK:
            OAttractAI_check_road_bonus();
            OAttractAI_set_steering_bonus();

            // Accelerate Car
            if (OInitEngine_rd_split_state == 0 || (ORoad_road_pos >> 16) <= 0x163)
            {
                OInputs_acc_adjust = 0xFF;
                OInputs_brake_adjust = 0;
                OFerrari_setup_ferrari_bonus_sprite();
                return;
            }
            else // init_end_anim
            {
                OFerrari_rev_shift = 1;
                OBonus_bonus_control = BONUS_SEQ0;
                // note fall through!
            }
            
            case BONUS_SEQ0:
                if ((ORoad_road_pos >> 16) < 0x18E)
                {
                    OFerrari_init_end_seq();
                    return;
                }
                OBonus_bonus_control = BONUS_SEQ1;
                // fall through
            
            case BONUS_SEQ1:
                if ((ORoad_road_pos >> 16) < 0x18F)
                {
                    OFerrari_init_end_seq();
                    return;
                }
                OBonus_bonus_control = BONUS_SEQ2;

            case BONUS_SEQ2:
                if ((ORoad_road_pos >> 16) < 0x190)
                {
                    OFerrari_init_end_seq();
                    return;
                }
                OBonus_bonus_control = BONUS_SEQ3;
            
            case BONUS_SEQ3:
                if ((ORoad_road_pos >> 16) < 0x191)
                {
                    OFerrari_init_end_seq();
                    return;
                }
                else
                {
                    OFerrari_car_ctrl_active = 0; // -1
                    OInitEngine_car_increment = 0;
                    OBonus_bonus_control = BONUS_END;
                }

            case BONUS_END:
                OInputs_acc_adjust = 0;
                OInputs_brake_adjust = 0xFF;
                OFerrari_do_end_seq();
                break;

        /*default:
            std::cout << "Need to finish OFerrari:logic()" << std::endl;
            break;*/
    }
}

// Ferrari - Normal Game Logic (Non-Bonus Mode Code)
//
// Source: 0x9CB4
void OFerrari_ferrari_normal()
{
    if (FORCE_AI && Outrun_game_state == GS_INGAME)
    {
        OAttractAI_tick_ai_enhanced();
        OFerrari_setup_ferrari_sprite();
        return;
    }

    switch (Outrun_game_state)
    {    
        // Attract Mode: Process AI Code and fall through
        case GS_INIT:
        case GS_ATTRACT:
            if (Config_engine.new_attract)
                OAttractAI_tick_ai_enhanced();
            else
                OAttractAI_tick_ai();
            OFerrari_setup_ferrari_sprite();
            break;

        case GS_INIT_BEST1:
        case GS_BEST1:
        case GS_INIT_LOGO:
        case GS_LOGO:
        case GS_INIT_GAME:
        case GS_INIT_GAMEOVER:
        case GS_GAMEOVER:
            OInputs_brake_adjust = 0;
        case GS_START1:
        case GS_START2:
        case GS_START3:
            OInputs_steering_adjust = 0;
        case GS_INGAME:
        case GS_INIT_BONUS:
        case GS_BONUS:
        case GS_INIT_MAP:
        case GS_MAP:
            OFerrari_setup_ferrari_sprite();
            break;

        case GS_INIT_MUSIC:
        case GS_MUSIC:
            return;

        /*default:
            std::cout << "Need to finish OFerrari:ferrari_normal()" << std::endl;
            break;*/
    }
}

// Setup Ferrari Sprite Object
//
// Source: 0x9D30
void OFerrari_setup_ferrari_sprite()
{
    OFerrari_spr_ferrari->y = 221; // Set Default Ferrari Y
    
    // Test Collision With Other Sprite Object
    if (OLevelObjs_collision_sprite)
    {
        if (OCrash_coll_count1 == OCrash_coll_count2)
        {
            OCrash_coll_count1++;
            OLevelObjs_collision_sprite = 0;
            OCrash_crash_state = 0;
        }
    }

    // Setup Default Ferrari Properties
    OFerrari_spr_ferrari->x = 0;
    OFerrari_spr_ferrari->zoom = 0x7F;
    OFerrari_spr_ferrari->draw_props = DRAW_BOTTOM    ; // Anchor Bottom
    OFerrari_spr_ferrari->shadow = 3;
    OFerrari_spr_ferrari->width = 0;
    OFerrari_spr_ferrari->priority = OFerrari_spr_ferrari->road_priority = 0x1FD;

    // Set Ferrari H-Flip Based On Steering & Speed
    int16_t d4 = OInputs_steering_adjust;

    // If steering close to centre clear d4 to ignore h-flip of Ferrari
    if (d4 >= -8 && d4 <= 7)
        d4 = 0;
    // If speed to slow clear d4 to ignore h-flip of Ferrari
    if (OInitEngine_car_increment >> 16 < 0x14)
        d4 = 0;

    // cont2:
    d4 >>= 2; // increase change of being close to zero and no h-flip occurring
    
    int16_t x_off = 0;

    // ------------------------------------------------------------------------
    // Not Skidding
    // ------------------------------------------------------------------------
    if (!OCrash_skid_counter)
    {
        if (d4 >= 0)
            OFerrari_spr_ferrari->control &= ~SPRITES_HFLIP;
        else
            OFerrari_spr_ferrari->control |= SPRITES_HFLIP;

        // 0x9E4E not_skidding:

        // Calculate change in road y, so we can determine incline frame for ferrari
        int16_t y = ORoad_road_y[ORoad_road_p0 + (0x3D0 / 2)] - ORoad_road_y[ORoad_road_p0 + (0x3E0 / 2)];

        // Converts y difference to a frame value (this is for when on an incline)
        int16_t incline_frame_offset = 0;
        if (y >= 0x12) incline_frame_offset += 8;
        if (y >= 0x13) incline_frame_offset += 8;

        // Get abs version of ferrari turn
        int16_t turn_frame_offset = 0;
        int16_t d2 = d4;
        if (d2 < 0) d2 = -d2;
        if (d2 >= 0x12) turn_frame_offset += 0x18;
        if (d2 >= 0x1E) turn_frame_offset += 0x18;

        // Set Ferrari Sprite Properties
        uint32_t offset = Outrun_adr.sprite_ferrari_frames + turn_frame_offset + incline_frame_offset;
        OFerrari_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, offset);     // Set Ferrari Frame Address
        OFerrari_sprite_pass_y = RomLoader_read16(Roms_rom0p, offset + 4); // Set Passenger Y Offset
        x_off = RomLoader_read16(Roms_rom0p, offset + 6); // Set Ferrari Sprite X Offset

        if (d4 < 0) x_off = -x_off;
    }
    // ------------------------------------------------------------------------
    // Skidding
    // ------------------------------------------------------------------------
    else
    {
        int16_t skid_counter = OCrash_skid_counter;

        if (skid_counter < 0)
        {
            OFerrari_spr_ferrari->control |= SPRITES_HFLIP;
            skid_counter = -skid_counter; // Needs to be positive
        }
        else
            OFerrari_spr_ferrari->control &= ~SPRITES_HFLIP;

        int16_t frame = 0;

        if (skid_counter >= 3)  frame += 8;
        if (skid_counter >= 6)  frame += 8;
        if (skid_counter >= 12) frame += 8;

        // Calculate incline
        int16_t y = ORoad_road_y[ORoad_road_p0 + (0x3D0 / 2)] - ORoad_road_y[ORoad_road_p0 + (0x3E0 / 2)];

        int16_t incline_frame_offset = 0;
        if (y >= 0x12) incline_frame_offset += 0x20;
        if (y >= 0x13) incline_frame_offset += 0x20;

        uint32_t offset = Outrun_adr.sprite_skid_frames + frame + incline_frame_offset;
        OFerrari_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, offset); // Set Ferrari Frame Address
        OFerrari_sprite_pass_y = RomLoader_read16(Roms_rom0p, offset + 4); // Set Passenger Y Offset
        x_off = RomLoader_read16(Roms_rom0p, offset + 6);         // Set Ferrari Sprite X Offset
        OFerrari_wheel_traction = TRACTION_OFF;                  // Both wheels have lost traction

        if (OCrash_skid_counter >= 0) x_off = -x_off;
    }

    OFerrari_spr_ferrari->x += x_off;

    OFerrari_shake();
    OFerrari_set_ferrari_palette();
    OFerrari_draw_sprite(OFerrari_spr_ferrari);
}

// Bonus Mode: Setup Ferrari Sprite Details
//
// Source: 0xA212
void OFerrari_setup_ferrari_bonus_sprite()
{
    // Setup Default Ferrari Properties
    OFerrari_spr_ferrari->y = 221;
    //spr_ferrari->x = 0; not really needed as set below
    OFerrari_spr_ferrari->priority = OFerrari_spr_ferrari->road_priority = 0x1FD;

    if (OInputs_steering_adjust > 0)
        OFerrari_spr_ferrari->control &= ~SPRITES_HFLIP;
    else
        OFerrari_spr_ferrari->control |= SPRITES_HFLIP;

    // Get abs version of ferrari turn
    int16_t turn_frame_offset = 0;
    int16_t d2 = OInputs_steering_adjust >> 2;
    if (d2 < 0) d2 = -d2;
    if (d2 >= 0x4) turn_frame_offset += 0x18;
    if (d2 >= 0x8) turn_frame_offset += 0x18;

    // Set Ferrari Sprite Properties
    uint32_t offset   = Outrun_adr.sprite_ferrari_frames + turn_frame_offset + 8; // 8 denotes the 'level' frames, no slope.
    OFerrari_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, offset);     // Set Ferrari Frame Address
    OFerrari_sprite_pass_y     = RomLoader_read16(Roms_rom0p, offset + 4); // Set Passenger Y Offset
    int16_t x_off     = RomLoader_read16(Roms_rom0p, offset + 6); // Set Ferrari Sprite X Offset

    if (OInputs_steering_adjust < 0) x_off = -x_off;
    OFerrari_spr_ferrari->x = x_off;

    OFerrari_set_ferrari_palette();
    //OSprites_map_palette(spr_ferrari);
    //OSprites_do_spr_order_shadows(spr_ferrari);
    OFerrari_draw_sprite(OFerrari_spr_ferrari);
}

// Source: 0xA1CE
void OFerrari_init_end_seq()
{
    // AI for remainder
    OAttractAI_check_road_bonus();
    OAttractAI_set_steering_bonus();
    // Brake Car!
    OInputs_acc_adjust = 0;
    OInputs_brake_adjust = 0xFF;
    OFerrari_do_end_seq();
}

// Drive Ferrari to the side during end sequence
//
// Source: 0xA298
void OFerrari_do_end_seq()
{
    OFerrari_spr_ferrari->y = 221;
    OFerrari_spr_ferrari->priority = OFerrari_spr_ferrari->road_priority = 0x1FD;

    // Set Ferrari Frame
    // +0 [Long]: Address of frame
    // +4 [Byte]: Passenger Offset (always 0!)
    // +5 [Byte]: Ferrari X Change
    // +6 [Byte]: Sprite Colour Palette
    // +7 [Byte]: H-Flip

    uint32_t addr = Outrun_adr.anim_ferrari_frames + ((OBonus_bonus_control - 0xC) << 1);

    OFerrari_spr_ferrari->addr    = RomLoader_read32(Roms_rom0p, addr);
    OFerrari_sprite_pass_y        = RomLoader_read8(Roms_rom0p, 4 + addr);  // Set Passenger Y Offset
    OFerrari_spr_ferrari->x       = RomLoader_read8(Roms_rom0p, 5 + addr);
    OFerrari_spr_ferrari->pal_src = RomLoader_read8(Roms_rom0p, 6 + addr);
    OFerrari_spr_ferrari->control = RomLoader_read8(Roms_rom0p, 7 + addr) | (OFerrari_spr_ferrari->control & 0xFE); // HFlip

    OSprites_map_palette(OFerrari_spr_ferrari);
    OSprites_do_spr_order_shadows(OFerrari_spr_ferrari);
}

// - Update Car Palette To Adjust Brake Lamp
// - Also Control Speed at which wheels spin through palette adjustment
//
// Source: 0x9F7C
void OFerrari_set_ferrari_palette()
{
    uint8_t pal;

    // Denote palette for brake light
    if (OInputs_brake_adjust >= INPUTS_BRAKE_THRESHOLD1)
    {
        OOutputs_set_digital(OUTPUTS_D_BRAKE_LAMP);
        pal = 2;
    }
    else
    {
        OOutputs_clear_digital(OUTPUTS_D_BRAKE_LAMP);
        pal = 0;
    }

    // Car Moving
    if (OInitEngine_car_increment >> 16 != 0)
    {
        int16_t car_increment = 5 - (OInitEngine_car_increment >> 21);
        if (car_increment < 0) car_increment = 0;
        OFerrari_wheel_frame_reset = car_increment;

        // Increment wheel palette offset (to move wheels)
        if (OFerrari_wheel_counter <= 0)
        {
            OFerrari_wheel_counter = OFerrari_wheel_frame_reset;
            OFerrari_wheel_pal++; 
        }
        else
            OFerrari_wheel_counter--;
    }
    OFerrari_spr_ferrari->pal_src = pal + 2 + (OFerrari_wheel_pal & 1);
}

// Set Ferrari X Position
//
// - Reads steering value
// - Converts to a practical change in x position
// - There are a number of special cases, as you will see by looking at the code
//
// In use:
//
// d0 = Amount to adjust car x position by
//
// Source: 0xC1B2

void OFerrari_set_ferrari_x()
{
    int16_t steering = OInputs_steering_adjust;

    // Hack to reduce the amount you can steer left and right at the start of Stage 1
    // The amount you can steer increases as you approach road position 0x7F
    if (OStats_cur_stage == 0 && OInitEngine_rd_split_state == 0 && (ORoad_road_pos >> 16) <= 0x7F)
    {
        steering = (steering * (ORoad_road_pos >> 16)) >> 7;
    }

    steering -= OFerrari_steering_old;
    if (steering > 0x40) steering = 0x40;
    else if (steering < -0x40) steering = -0x40;
    OFerrari_steering_old += steering;
    steering = OFerrari_steering_old;

    // This block of code reduces the amount the player
    // can steer left and right, when below a particular speed
    if (OFerrari_wheel_state == WHEELS_ON && (OInitEngine_car_increment >> 16) <= 0x7F)
    {
        steering = (steering * (OInitEngine_car_increment >> 16)) >> 7;
    }

    // Check Road Curve And Adjust X Value Accordingly
    // This effectively makes it harder to steer into sharp corners
    int16_t road_curve = OInitEngine_road_curve;
    if (road_curve)
    {
        road_curve -= 0x40;
        if (road_curve < 0)
        {
            int16_t diff_from_max = (MAX_SPEED >> 17) - (OInitEngine_car_increment >> 16);
            if (diff_from_max < 0)
            {
                int16_t curve = diff_from_max * road_curve;
                int32_t result = (int32_t) steering * (0x24C0 - curve);
                steering = result / 0x24C0;
            }
        }
    }

    steering >>= 3;
    int16_t steering2 = steering;
    steering >>= 2;
    steering += steering2;
    steering = -steering;

    // Return if car is not moving
    if (Outrun_game_state == GS_INGAME && OInitEngine_car_increment >> 16 == 0)
    {
        // Hack so car is centered if we want to bypass countdown sequence
        int16_t road_width_change = (ORoad_road_width >> 16) - road_width_old;
        road_width_old = (ORoad_road_width >> 16);
        if (OInitEngine_car_x_pos < 0)
            road_width_change = -road_width_change;
        OInitEngine_car_x_pos += road_width_change;
        // End of Hack
        return;
    }
    
    OInitEngine_car_x_pos += steering;
    
    int16_t road_width_change = (ORoad_road_width >> 16) - road_width_old;
    road_width_old = (ORoad_road_width >> 16);
    if (OInitEngine_car_x_pos < 0)
        road_width_change = -road_width_change;
    OInitEngine_car_x_pos += road_width_change;
}

// Ensure car does not stray too far from sides of road
// There are three parts to this function:
// a. Normal Road
// b. Road Split
// c. Dual Lanes
//
// Source: 0xC2B0
void OFerrari_set_ferrari_bounds()
{
    // d0 = road_width
    // d2 = car_x_pos

    int16_t road_width16 = ORoad_road_width >> 16;
    int16_t d1 = 0;

    // Road Split Both Lanes
    if (OInitEngine_rd_split_state == 4)
    {
        if (OInitEngine_car_x_pos < 0)
            road_width16 = -road_width16;
        d1 = road_width16 + 0x140;
        road_width16 -= 0x140;
    }
    // One Lane
    else if (road_width16 <= 0xFF)
    {
        road_width16 += OFFROAD_BOUNDS;
        d1 = road_width16;
        road_width16 = -road_width16;
    }
    // Two Lanes - road_two_lanes:
    else
    {
        if (OInitEngine_car_x_pos < 0)
            road_width16 = -road_width16;
        d1 = road_width16 + OFFROAD_BOUNDS;
        road_width16 -= OFFROAD_BOUNDS;
    }

    // Set Bounds
    if (OInitEngine_car_x_pos < road_width16)
        OInitEngine_car_x_pos = road_width16;
    else if (OInitEngine_car_x_pos > d1)
        OInitEngine_car_x_pos = d1;

    ORoad_car_x_bak = OInitEngine_car_x_pos;
    ORoad_road_width_bak = ORoad_road_width >> 16; 
}

// Check Car Is Still On Road
//
// Source: 0xBFDC
void OFerrari_check_wheels()
{
    OFerrari_wheel_state = WHEELS_ON;
    OFerrari_wheel_traction = TRACTION_ON;
    uint16_t road_width = ORoad_road_width >> 16;

    switch (ORoad_road_ctrl)
    {
        case ROAD_OFF:         // Both Roads Off
            return;

        // Single Road
        case ROAD_R0:          // Road 0
        case ROAD_R1:          // Road 1
        case ROAD_R0_SPLIT:    // Road 0 (Road Split.)
        case ROAD_R1_SPLIT:    // Road 1 (Road Split. Invert Road 1)
        {
            int16_t x = OInitEngine_car_x_pos;
            
            if (ORoad_road_ctrl == ROAD_R0_SPLIT)
                x -= road_width;
            else    
                x += road_width;

            if (ORoad_road_ctrl == ROAD_R0_SPLIT || ORoad_road_ctrl == ROAD_R1_SPLIT)
            {
                if (x > -0xD4 && x <= 0xD4) return;
                else if (x < -0x104 || x > 0x104) OFerrari_set_wheels(WHEELS_OFF);
                else if (x > 0xD4 && x <= 0x104) OFerrari_set_wheels(WHEELS_LEFT_OFF);
                else if (x <= 0xD4 && x >= -0x104) OFerrari_set_wheels(WHEELS_RIGHT_OFF);
            }
            else
            {
                if (x > -0xD4 && x <= 0xD4) return;
                else if (x < -0x104 || x > 0x104) OFerrari_set_wheels(WHEELS_OFF);
                else if (x > 0xD4 && x <= 0x104) OFerrari_set_wheels(WHEELS_RIGHT_OFF);
                else if (x <= 0xD4 && x >= -0x104) OFerrari_set_wheels(WHEELS_LEFT_OFF);
            }
        }
        break;
        
        // Both Roads
        case ROAD_BOTH_P0:     // Both Roads (Road 0 Priority) [DEFAULT]
        case ROAD_BOTH_P1:     // Both Roads (Road 1 Priority) 
        case ROAD_BOTH_P0_INV: // Both Roads (Road 0 Priority) (Road Split. Invert Road 1)
        case ROAD_BOTH_P1_INV: // Both Roads (Road 1 Priority) (Road Split. Invert Road 1)
        {
            int16_t x = OInitEngine_car_x_pos;

            if (road_width > 0xFF)
            {
                if (x < 0)
                    x += road_width;
                else
                    x -= road_width;

                if (x < -0x104 || x > 0x104) OFerrari_set_wheels(WHEELS_OFF);
                else if (x < -0xD4) OFerrari_set_wheels(WHEELS_RIGHT_OFF);
                else if (x > 0xD4) OFerrari_set_wheels(WHEELS_LEFT_OFF);
            }
            else
            {
                road_width += 0x104;

                if (x >= 0)
                {
                    if (x > road_width) OFerrari_set_wheels(WHEELS_OFF);
                    else if (x > road_width - 0x30) OFerrari_set_wheels(WHEELS_LEFT_OFF);
                }
                else
                {
                    x = -x;
                    if (x > road_width) OFerrari_set_wheels(WHEELS_OFF);
                    else if (x > road_width - 0x30) OFerrari_set_wheels(WHEELS_RIGHT_OFF);
                }
            }
        }
        break;
    }   
}

void OFerrari_set_wheels(uint8_t new_state) 
{
    OFerrari_wheel_state = new_state;
    OFerrari_wheel_traction = (new_state == WHEELS_OFF) ? 2 : 1;
}

// Adjusts the x-position of the car based on the curve.
// This effectively stops the user driving through the courses in a straight line.
// (sticks the car to the track).
//
// Source: 0xBF6E
void OFerrari_set_curve_adjust()
{
    int16_t x_diff = ORoad_road_x[170] - ORoad_road_x[511];

    // Invert x diff when taking roadsplit
    if (OInitEngine_rd_split_state && OInitEngine_car_x_pos < 0)
        x_diff = -x_diff;

    x_diff >>= 6;
    
    if (x_diff)
    {
        x_diff *= (OInitEngine_car_increment >> 16);
        x_diff /= 0xDC;
        x_diff <<= 1;
        OInitEngine_car_x_pos += x_diff;
    }
}

// Draw Shadow below Ferrari
//
// Source: 0xA7BC
void OFerrari_draw_shadow()
{
    if (OFerrari_spr_shadow->control & SPRITES_ENABLE)
    {
        if (Outrun_game_state == GS_MUSIC) return;

        if (Outrun_tick_frame)
        {
            OFerrari_spr_shadow->road_priority = OFerrari_spr_ferrari->road_priority - 1;
            OFerrari_spr_shadow->x = OFerrari_spr_ferrari->x;
            OFerrari_spr_shadow->y = 222;
            OFerrari_spr_shadow->zoom = 0x99;
            OFerrari_spr_shadow->draw_props = 8;
            OFerrari_spr_shadow->addr = Outrun_adr.shadow_data;
        }

        if (ORoad_get_view_mode() != ROAD_VIEW_INCAR)
            OSprites_do_spr_order_shadows(OFerrari_spr_shadow);
    }
}

// Set Passenger Sprite X/Y Position.
//
// Routine is used separately for both male and female passengers.
//
// In use:
// a1 = Passenger XY offset table
// a5 = Passenger 1 / Passenger 2 Sprite
// a6 = Car Sprite
//
// Memory locations:
// 62F00 = Car
// 62F40 = Passenger 1 (Man)
// 62F80 = Passenger 2 (Girl)
//
// Source: A568
void OFerrari_set_passenger_sprite(oentry* sprite)
{
    sprite->road_priority = OFerrari_spr_ferrari->road_priority;
    sprite->priority = OFerrari_spr_ferrari->priority + 1;
    uint16_t frame = OFerrari_sprite_pass_y << 3;

    // Is this a bug in the original? Note that by negating SPRITES_HFLIP check the passengers
    // shift right a few pixels on acceleration.
    if ((OInitEngine_car_increment >> 16 >= 0x14) && !(OFerrari_spr_ferrari->control & SPRITES_HFLIP))
        frame += 4;

    // --------------------------------------------------------------------------------------------
    // Set Palette
    // --------------------------------------------------------------------------------------------
    uint8_t pal = 0;

    // Test for car collision frame
    if (OFerrari_sprite_pass_y == 9)
    {
        // Set Brown/Blonde Palette depending on whether man or woman
        if (sprite == OFerrari_spr_pass1) pal = 0xA;
        else pal = 0x8;
    }
    else
    {
        if (sprite == OFerrari_spr_pass1) pal = 0x2D;
        else pal = 0x2E;
    }

    // --------------------------------------------------------------------------------------------
    // Set X/Y Position
    // --------------------------------------------------------------------------------------------

    sprite->pal_src = pal;
    uint32_t offset_table = ((sprite == OFerrari_spr_pass1) ? PASS1_OFFSET : PASS2_OFFSET) + frame;
    sprite->x = OFerrari_spr_ferrari->x + RomLoader_read16IncP(&Roms_rom0, &offset_table);
    sprite->y = OFerrari_spr_ferrari->y + RomLoader_read16(&Roms_rom0, offset_table);
    
    sprite->zoom = 0x7F;
    sprite->draw_props = 8;
    sprite->shadow = 3;
    sprite->width = 0;

    OFerrari_set_passenger_frame(sprite);
    OFerrari_draw_sprite(sprite);
}

// Set Passenger Sprite Frame
//
// - Set the appropriate male/female frame
// - Uses the car's speed to set the hair frame
// - Also handles skid case
//
// Source: 0xA632

void OFerrari_set_passenger_frame(oentry* sprite)
{
    uint32_t addr = Outrun_adr.sprite_pass_frames;
    if (sprite == OFerrari_spr_pass2) addr += 4; // Female frames
    uint16_t inc = OInitEngine_car_increment >> 16;

    // Car is moving
    // Use adjusted increment/speed of car as reload value for sprite counter (to ultimately set hair frame)
    if (inc != 0)
    {
        if (inc > 0xFF) inc = 0xFF;
        inc >>= 5;
        int16_t counter = 9 - inc;
        if (counter < 0) counter = 0;
        sprite->reload = counter;
        if (sprite->counter <= 0)
        {
            sprite->counter = sprite->reload;
            sprite->xw1++; // Reuse this as a personal tick counter to flick between hair frames
        }
        else
            sprite->counter--;
        
        inc = (sprite->xw1 & 1) << 3;
    }

    // Check Skid
    if (sprite->pass_props >= 9)
    {
        // skid left
        if (OCrash_skid_counter > 0)
        {
            sprite->addr = (sprite == OFerrari_spr_pass1) ? 
                Outrun_adr.sprite_pass1_skidl : Outrun_adr.sprite_pass2_skidl;
        }
        // skid right
        else
        {
            sprite->addr = (sprite == OFerrari_spr_pass1) ? 
                Outrun_adr.sprite_pass1_skidr : Outrun_adr.sprite_pass2_skidr;
        }
    }
    else
        sprite->addr = RomLoader_read32(Roms_rom0p, addr + inc);
}

// ------------------------------------------------------------------------------------------------
//                                       CAR HANDLING ROUTINES
// ------------------------------------------------------------------------------------------------

// Main routine handling car speed, revs, torque
//
// Source: 0x6288
void OFerrari_move()
{
    if (OFerrari_car_ctrl_active)
    {      
        // Auto braking if necessary
        if (Outrun_game_state != GS_ATTRACT && OFerrari_auto_brake)
            OInputs_acc_adjust = 0;   

        // Set Gear For Demo Mode
        if (FORCE_AI || 
            Outrun_game_state == GS_ATTRACT || Outrun_game_state == GS_BONUS || 
            Config_controls.gear == CONTROLS_GEAR_AUTO)
        {
            // demo_mode_gear
            OInputs_gear = (OInitEngine_car_increment >> 16 > 0xA0);
        }

        gfx_smoke = 0;

        // --------------------------------------------------------------------
        // CRASH CODE - Slow Car
        // --------------------------------------------------------------------
        if (OCrash_crash_counter && OCrash_spin_control1 <= 0)
        {
            OInitEngine_car_increment = (OInitEngine_car_increment & 0xFFFF) | ((((OInitEngine_car_increment >> 16) * 31) >> 5) << 16);
            OFerrari_revs = 0;
            gear_value = 0;
            gear_bak = 0;
            gear_smoke = 0;
            OFerrari_torque = 0x1000;
        }
        // --------------------------------------------------------------------
        // NO CRASH
        // --------------------------------------------------------------------
        else
        {    
            if (OFerrari_car_state >= 0) OFerrari_car_state = CAR_NORMAL; // No crash - Clear smoke from wheels

            // check_time_expired:
            // 631E: Clear acceleration value if time out
            if ((OStats_time_counter & 0xFF) == 0)
                OInputs_acc_adjust = 0;
        
            // --------------------------------------------------------------------
            // Do Car Acceleration / Revs / Torque
            // Note: Torque gets set based on gear car is in
            // --------------------------------------------------------------------
        
            // do_acceleration:
            OFerrari_car_acc_brake();

            int32_t d2 = OFerrari_revs / OFerrari_torque;

            if (!OInitEngine_ingame_engine)
            {
                OFerrari_tick_engine_disabled(&d2);
            }
            else
            {      
                int16_t d1 = OFerrari_torque_index;

                if (gear_counter == 0)
                    OFerrari_do_gear_torque(&d1);
            }
            // set_torque:
            int16_t new_torque = OFerrari_torque_lookup[OFerrari_torque_index];
            OFerrari_torque = new_torque;
            d2 = (int32_t) (d2 & 0xFFFF) * new_torque; // unsigned multiply
            int32_t accel_copy = accel_value << 16;
            int32_t rev_adjust_new = 0;

            if (gear_counter != 0)
                rev_adjust_new = OFerrari_tick_gear_change(d2 >> 16);

            // Compare Accelerator To Proposed New Speed
            //
            // d0 = Desired Accelerator Value [accel_copy]
            // d1 = Torque Value [new_torque]
            // d2 = Proposed New Rev Value [d2]
            // d4 = Rev Adjustment (due to braking / off road etc / revs being higher than desired) [rev_adjust]
            else if (d2 != accel_copy)
            {
                if (accel_copy >= d2)
                    rev_adjust_new = OFerrari_get_speed_inc_value(new_torque, d2);
                else
                    rev_adjust_new = OFerrari_get_speed_dec_value(new_torque);
            }

            // test_smoke:
            if (gear_smoke)
                rev_adjust_new = OFerrari_tick_smoke(); // Note this also changes the speed [stored in d4]

            // cont2:
            OFerrari_set_brake_subtract();               // Calculate brake value to subtract from revs
            OFerrari_finalise_revs(&d2, rev_adjust_new);  // Subtract calculated value from revs
            OFerrari_convert_revs_speed(new_torque, &d2); // d2 is converted from revs to speed

            // Ingame Control Not Active. Clear Car Speed
            if (!OInitEngine_ingame_engine)
            {
                OInitEngine_car_increment = 0;
                OFerrari_car_inc_old = 0;
            }
            // Set New Car Speed to car_increment
            else if (Outrun_game_state != GS_BONUS)
            {
                int16_t diff = OFerrari_car_inc_old - (d2 >> 16); // Old Speed - New Speed

                // Car is moving
                if (diff != 0)
                {
                    // Car Speeding Up (New Speed is faster)
                    if (diff < 0)
                    {
                        diff = -diff;
                        uint8_t adjust = 2;
                        if (OInitEngine_car_increment >> 16 <= 0x28)
                            adjust >>= 1;
                        if (diff > 2)
                            d2 = (OFerrari_car_inc_old + adjust) << 16;                      
                    }
                    // Car Slowing Down
                    else if (diff > 0)
                    {
                        uint8_t adjust = 2;
                        if (brake_subtract)
                            adjust <<= 2;
                        if (diff > adjust)
                            d2 = (OFerrari_car_inc_old - adjust) << 16;
                    }
                }
                OInitEngine_car_increment = d2;
            }
            else
                OInitEngine_car_increment = d2;
        } // end crash if/else block
        
        // move_car_rev:
        OFerrari_update_road_pos();
        OHud_draw_rev_counter();
    } // end car_ctrl_active

    // Check whether we want to play slip sound
    // check_slip
    if (gfx_smoke)
    {
        OFerrari_car_state = CAR_SMOKE; // Set smoke from car wheels
        if (OInitEngine_car_increment >> 16)
        {
            if (OFerrari_slip_sound == sound_STOP_SLIP)
                OSoundInt_queue_sound(OFerrari_slip_sound = sound_INIT_SLIP);
        }
        else
            OSoundInt_queue_sound(OFerrari_slip_sound = sound_STOP_SLIP);
    }
    // no_smoke:
    else
    {
        if (OFerrari_slip_sound != sound_STOP_SLIP)
            OSoundInt_queue_sound(OFerrari_slip_sound = sound_STOP_SLIP);        
    }
    // move_car
    OFerrari_car_inc_old = OInitEngine_car_increment >> 16;
    OFerrari_counter++;
    
    // During Countdown: Clear Car Speed
    if (Outrun_game_state == GS_START1 || Outrun_game_state == GS_START2 || Outrun_game_state == GS_START3)
    {
        OInitEngine_car_increment = 0;
        OFerrari_car_inc_old = 0;
    }
    
}

// Handle revs/torque when in-game engine disabled
//
// Source: 0x6694
void OFerrari_tick_engine_disabled(int32_t *d2)
{
    OFerrari_torque_index = 0;
    
    // Crash taking place - do counter and set game engine when expired
    if (OCrash_coll_count1)
    {
        OLevelObjs_spray_counter = 0;
        if (--OInitEngine_ingame_counter != 0)
            return;
    }
    else if (Outrun_game_state != GS_ATTRACT && Outrun_game_state != GS_INGAME) 
        return;

    // Switch back to in-game engine mode
    OInitEngine_ingame_engine = 1;

    OFerrari_torque = 0x1000;

    // Use top word of revs for lookup
    int16_t lookup = (OFerrari_revs >> 16);
    if (lookup > 0xFF) lookup = 0xFF;

    OFerrari_torque_index = (0x30 - OFerrari_rev_inc_lookup[lookup]) >> 2;

    int16_t acc = accel_value - 0x10;
    if (acc < 0) acc = 0;
    OFerrari_acc_post_stop = acc;
    OFerrari_revs_post_stop = lookup;

    // Clear bottom word of d2 and swap
    *d2 = *d2 << 16;

    //lookup -= 0x10;
    //if (lookup < 0) lookup = 0;
    OFerrari_rev_stop_flag = 14;
}

// - Convert the already processed accleration inputs into a meaningful value for both ACC and BRAKE
// - Adjust acceleration based on number of wheels on road
// - Adjust acceleration if car skidding or spinning
//
// Source: 0x6524
void OFerrari_car_acc_brake()
{
    // ------------------------------------------------------------------------
    // Acceleration
    // ------------------------------------------------------------------------
    int16_t acc1 = OInputs_acc_adjust;
    int16_t acc2 = OInputs_acc_adjust;

    acc1 += acc_adjust1 + acc_adjust2 + acc_adjust3;
    acc1 >>= 2;
    acc_adjust3 = acc_adjust2;
    acc_adjust2 = acc_adjust1;
    acc_adjust1 = acc2;

    if (!OInitEngine_ingame_engine)
    {
        acc2 -= accel_value_bak;
        if (acc2 < 0) acc2 = -acc2;
        if (acc2 < 8)
            acc1 = accel_value_bak;
    }
    // test_skid_spin:

    // Clear acceleration on skid
    if (OCrash_spin_control1 > 0 || OCrash_skid_counter)
        acc1 = 0;

    // Adjust speed when offroad
    else if (OFerrari_wheel_state != WHEELS_ON)
    {
        if (gear_value)
            acc1 = (acc1 * 3) / 10;
        else
            acc1 = (acc1 * 6) / 10;

        // If only one wheel off road, increase acceleration by a bit more than if both wheels off-road
        if (OFerrari_wheel_state != WHEELS_OFF)
            acc1 = (acc1 << 1) + (acc1 >> 1);
    }

    // finalise_acc_value:
    accel_value = acc1;
    accel_value_bak = acc1;

    // ------------------------------------------------------------------------
    // Brake
    // ------------------------------------------------------------------------
    int16_t brake1 = OInputs_brake_adjust;
    int16_t brake2 = OInputs_brake_adjust;
    brake1 += brake_adjust1 + brake_adjust2 + brake_adjust3;
    brake1 >>= 2;
    brake_adjust3 = brake_adjust2;
    brake_adjust2 = brake_adjust1;
    brake_adjust1 = brake2;
    brake_value = brake1;

    // ------------------------------------------------------------------------
    // Gears
    // ------------------------------------------------------------------------
    gear_bak = gear_value;
    gear_value = OInputs_gear;
}

// Do Gear Changes & Torque Index Settings
//
// Source: 0x6948
void OFerrari_do_gear_torque(int16_t *d1)
{
    if (OInitEngine_ingame_engine)
    {
        *d1 = OFerrari_torque_index;
        if (gear_value)
            OFerrari_do_gear_high(d1);
        else
            OFerrari_do_gear_low(d1);
    }
    OFerrari_torque_index = (uint8_t) *d1;
    // Backup gear value for next iteration (so we can tell when gear has recently changed)
    gear_bak = gear_value;
}

void OFerrari_do_gear_low(int16_t *d1)
{
    // Recent Shift from high to low
    if (gear_bak)
    {
        gear_value = 0;
        gear_counter = 4;
        return;
    }

    // Low Gear - Show Smoke When Accelerating From Standstill
    if (OInitEngine_car_increment >> 16 < 0x50 && accel_value - 0xE0 >= 0)
        gfx_smoke++;

    // Adjust Torque Index
    if (*d1 == 0x10) return;
    else if (*d1 < 0x10)
    {
        (*d1)++;
        return;
    }
    *d1 -= 4;
    if (*d1 < 0x10)
        *d1 = 0x10;
}

void OFerrari_do_gear_high(int16_t *d1)
{
    // Change from Low Gear to High Gear
    if (!gear_bak)
    {
        gear_value = 1;
        gear_counter = 4;
        return;
    }

    // Increment torque until it reaches 0x1F
    if (*d1 == 0x1F) return;
    (*d1)++;
}

// Source: 0x6752
int32_t OFerrari_tick_gear_change(int16_t rem)
{
    gear_counter--;
    rev_adjust = rev_adjust - (rev_adjust >> 4);

    // Setup smoke when gear counter hits zero
    if (gear_counter == 0)
    {
        rem -= 0xE0;
        if (rem < 0)  return rev_adjust;
        int16_t acc = accel_value - 0xE0;
        if (acc < 0)  return rev_adjust;
        gear_smoke = acc;
    }

    return rev_adjust;
}

// Set value to increment speed by, when revs lower than acceleration
//
// Inputs:
//
// d1 = Torque Value [new_torque]
// d2 = Proposed new increment value [new_rev]
//
// Outputs: d4 [Rev Adjustment]
//
// Source: 679C

int32_t OFerrari_get_speed_inc_value(uint16_t new_torque, uint32_t new_rev)
{
    // Use Top Word Of Revs For Table Lookup. Cap At 0xFF Max
    uint16_t lookup = (new_rev >> 16);
    if (lookup > 0xFF) lookup = 0xFF;

    uint32_t rev_adjust = OFerrari_rev_inc_lookup[lookup]; // d4

    // Double adjustment if car moving slowly
    if (OInitEngine_car_increment >> 16 <= 0x14)
        rev_adjust <<= 1;

    rev_adjust = ((new_torque * new_torque) >> 12) * rev_adjust;
    
    if (!OInitEngine_ingame_engine) return rev_adjust;
    return rev_adjust << OFerrari_rev_shift;
}

// Set value to decrement speed by, when revs higher than acceleration
//
// Inputs:
//
// d1 = Torque Value [new_torque]
//
// Outputs: d4 [Rev Adjustment]
//
// Source: 67E4
int32_t OFerrari_get_speed_dec_value(uint16_t new_torque)
{
    int32_t new_rev = -((0x440 * new_torque) >> 4);
    if (OFerrari_wheel_state == WHEELS_ON) return new_rev;
    return new_rev << 2;
}

// - Reads Brake Value
// - Translates Into A Value To Subtract From Car Speed
// - Also handles setting smoke on wheels during brake/skid
// Source: 0x6A10
void OFerrari_set_brake_subtract()
{
    int32_t d6 = 0;
    const int32_t DEC = -0x8800; // Base value to subtract from acceleration burst
    
    // Not skidding or spinning
    if (OCrash_skid_counter == 0 && OCrash_spin_control1 == 0)
    {
        if (brake_value < INPUTS_BRAKE_THRESHOLD1)
        {
            brake_subtract = d6;
            return;
        }
        else if (brake_value < INPUTS_BRAKE_THRESHOLD2)
        {
            brake_subtract = d6 + DEC;
            return;
        }
        else if (brake_value < INPUTS_BRAKE_THRESHOLD3)
        {
            brake_subtract = d6 + (DEC * 3);
            return;
        }
        else if (brake_value < INPUTS_BRAKE_THRESHOLD4)
        {
            brake_subtract = d6 + (DEC * 5);
            return;
        }
    }
    // check_smoke
    if (OInitEngine_car_increment >> 16 > 0x28)
        gfx_smoke++;

    brake_subtract = d6 + (DEC * 9);
}


// - Finalise rev value, taking adjustments into account
//
// Inputs:
//
// d2 = Current Revs
// d4 = Rev Adjustment
//
// Outputs:
//
// d2 = New rev value
//
// Source: 0x67FC

void OFerrari_finalise_revs(int32_t* d2, int32_t rev_adjust_new)
{
    rev_adjust_new += brake_subtract;
    if (rev_adjust_new < -0x44000) rev_adjust_new = -0x44000;
    *d2 += rev_adjust_new;
    rev_adjust = rev_adjust_new;
    
    if (*d2 > 0x13C0000) *d2 = 0x13C0000;
    else if (*d2 < 0) *d2 = 0;
}

// Convert revs to final speed value
// Set correct pitch for sound fx
//
// Note - main problem seems to be that the revs fed to this routine are wrong in the first place on restart
// rev_stop_flag, revs_post_stop, accel_value and acc_post_stop seem ok?
//
// Source: 0x6838
void OFerrari_convert_revs_speed(int32_t new_torque, int32_t* d2)
{
    OFerrari_revs = *d2;
    int32_t d3 = *d2;
    if (d3 < 0x1F0000) d3 = 0x1F0000;

    int16_t revs_top = d3 >> 16;
    
    // Check whether we're switching back to ingame engine (after disabling user control of car)
    if (OFerrari_rev_stop_flag)
    {
        if (revs_top >= OFerrari_revs_post_stop)
        {
            OFerrari_rev_stop_flag = 0;
        }
        else
        {
            if (accel_value < OFerrari_acc_post_stop)
                OFerrari_revs_post_stop -= OFerrari_rev_stop_flag;

            // cont1:

            int16_t d5 = OFerrari_revs_post_stop >> 1;
            int16_t d4 = OFerrari_rev_stop_flag;
            if (revs_top >= d5)
            {
                d5 >>= 1;
                d4 >>= 1;
                if (revs_top >= d5)
                    d4 >>= 1;
            }
            // 689c
            OFerrari_revs_post_stop -= d4;
            if (OFerrari_revs_post_stop < 0x1F) OFerrari_revs_post_stop = 0x1F;
            revs_top = OFerrari_revs_post_stop;
        }
    }

    // setup_pitch_fx:
    OFerrari_rev_pitch1 = (revs_top * 0x1A90) >> 8;

    // Setup New Car Increment Speed
    *d2 = ((*d2 >> 16) * 0x1A90) >> 8;
    *d2 = (*d2 << 16) >> 4;
    
    /*if (!new_torque)
    {
        std::cout << "convert_revs_speed error!" << std::endl;
    }*/

    *d2 = (*d2 / new_torque) * 0x480;
    
    if (*d2 < 0) *d2 = 0;
    else if (*d2 > MAX_SPEED) *d2 = MAX_SPEED;
}

// Update road_pos, taking road_curve into account
//
// Source: 0x6ABA
void OFerrari_update_road_pos()
{
    uint32_t car_inc = CAR_BASE_INC;

    // Bendy Roads
    if (OInitEngine_road_type > INITENGINE_ROAD_STRAIGHT)
    {
        int32_t x = OInitEngine_car_x_pos;
        
        if (OInitEngine_road_type == INITENGINE_ROAD_RIGHT)
            x = -x;

        x = (x / 0x28) + OInitEngine_road_curve;

        car_inc = (car_inc * x) / OInitEngine_road_curve;
    }

    car_inc *= OInitEngine_car_increment >> 16;
    ORoad_road_pos_change = car_inc;
    ORoad_road_pos += car_inc;
}

// Decrement Smoke Counter
// Source: 0x666A
int32_t OFerrari_tick_smoke()
{
    gear_smoke--;
    int32_t r = rev_adjust - (rev_adjust >> 4);
    if (gear_smoke >= 8) gfx_smoke++; // Trigger smoke
    return r;
}

// Calculate car score and sound. Strange combination, but there you go!
//
// Source: 0xBD78
void OFerrari_do_sound_score_slip()
{
    // ------------------------------------------------------------------------
    // ENGINE PITCH SOUND
    // ------------------------------------------------------------------------
    uint16_t engine_pitch = 0;

    // Do Engine Rev
    if (Outrun_game_state >= GS_START1 && Outrun_game_state <= GS_INGAME)
    {
        engine_pitch = OFerrari_rev_pitch2 + (OFerrari_rev_pitch2 >> 1);
    }

    OSoundInt_engine_data[sound_ENGINE_PITCH_H] = engine_pitch >> 8;
    OSoundInt_engine_data[sound_ENGINE_PITCH_L] = engine_pitch & 0xFF;

    // Curved Road
    if (OInitEngine_road_type != INITENGINE_ROAD_STRAIGHT)
    {
        int16_t steering = OInputs_steering_adjust;
        if (steering < 0) steering = -steering;

        // Hard turn
        if (steering >= 0x70)
        {
            // Move Left
            if (OInitEngine_car_x_pos > OInitEngine_car_x_old)
                cornering = (OInitEngine_road_type == INITENGINE_ROAD_LEFT) ? 0 : -1;
            // Straight
            else if (OInitEngine_car_x_pos == OInitEngine_car_x_old)
                cornering = 0;
            // Move Right
            else
                cornering = (OInitEngine_road_type == INITENGINE_ROAD_RIGHT) ? 0 : -1;
        }
        else
            cornering = 0;
    }
    else
        cornering = 0;

    // update_score:
    OFerrari_car_x_diff = OInitEngine_car_x_pos - OInitEngine_car_x_old;
    OInitEngine_car_x_old = OInitEngine_car_x_pos;

    if (Outrun_game_state == GS_ATTRACT) 
        return;

    // Wheels onroad - Convert Speed To Score
    if (OFerrari_wheel_state == WHEELS_ON) 
    {
        OStats_convert_speed_score(OInitEngine_car_increment >> 16);
    }

    // 0xBE6E
    if (!OFerrari_sprite_slip_copy)
    {
        if (OCrash_skid_counter)
        {
            OFerrari_is_slipping = -1;
            OSoundInt_queue_sound(sound_INIT_SLIP);
        }
    }
    // 0xBE94
    else
    {
        if (!OCrash_skid_counter)
        {
            OFerrari_is_slipping = 0;
            OSoundInt_queue_sound(sound_STOP_SLIP);
        }
    }
    // 0xBEAC
    OFerrari_sprite_slip_copy = OCrash_skid_counter;

    // ------------------------------------------------------------------------
    // Switch to cornering. Play Slip Sound. Init Smoke
    // ------------------------------------------------------------------------
    if (!OCrash_skid_counter)
    {
        // 0xBEBE: Initalize Slip
        if (!cornering_old)
        {
            if (cornering)
            {
                OFerrari_is_slipping = -1;
                OSoundInt_queue_sound(sound_INIT_SLIP);
            }
        }
        // 0xBEE4: Stop Cornering   
        else
        {
            if (!cornering)
            {
                OFerrari_is_slipping = 0;
                OSoundInt_queue_sound(sound_STOP_SLIP);
            }
        }
    }
    // check_wheels:
    cornering_old = cornering;

    if (OFerrari_sprite_wheel_state)
    {
        // If previous wheels on-road & current wheels off-road - play safety zone sound
        if (!OFerrari_wheel_state)
            OSoundInt_queue_sound(sound_STOP_SAFETYZONE);
    }
    // Stop Safety Sound
    else
    {
        if (OFerrari_wheel_state)
            OSoundInt_queue_sound(sound_INIT_SAFETYZONE);
    }
    OFerrari_sprite_wheel_state = OFerrari_wheel_state;
}

// Shake Ferrari by altering XY Position when wheels are off-road
//
// Source: 0x9FEE
void OFerrari_shake()
{
    if (Outrun_game_state != GS_INGAME && Outrun_game_state != GS_ATTRACT) return;

    if (OFerrari_wheel_traction == TRACTION_ON) return; // Return if both wheels have traction

    int8_t traction = OFerrari_wheel_traction - 1;
    int16_t rnd = outils_random();
    OFerrari_spr_ferrari->counter++;

    uint16_t car_inc = OInitEngine_car_increment >> 16; // [d5]
    
    if (car_inc <= 0xA) return; // Do not shake car at low speeds

    if (car_inc <= (0x3C >> traction))
    {
        rnd &= 3;
        if (rnd != 0) return;
    }
    else if (car_inc <= (0x78 >> traction))
    {
        rnd &= 1;
        if (rnd != 0) return;
    }

    if (rnd < 0) OFerrari_spr_ferrari->y--;
    else OFerrari_spr_ferrari->y++;

    rnd &= 1;
    
    if (!(OFerrari_spr_ferrari->counter & BIT_1))
        rnd = -rnd;

    OFerrari_spr_ferrari->x += rnd; // Increment Ferrari X by 1 or -1
}

// Perform Skid (After Car Has Collided With Another Vehicle)
//
// Source: 0xBFB4
void OFerrari_do_skid()
{
    if (!OCrash_skid_counter) return;

    if (OCrash_skid_counter > 0)
    {
        OCrash_skid_counter--;
        OInitEngine_car_x_pos += SKID_X_ADJ;
    }
    else
    {
        OCrash_skid_counter++;
        OInitEngine_car_x_pos -= SKID_X_ADJ;
    }
}

void OFerrari_draw_sprite(oentry* sprite)
{
    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR)
    {
        OSprites_map_palette(sprite);
        OSprites_do_spr_order_shadows(sprite);
    }
}

