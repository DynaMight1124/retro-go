/***************************************************************************
    Collision & Crash Code. 
    
    There are two types of collision: Scenery & Traffic.
    
    1/ Traffic: The Ferrari will spin after a collision.
    2/ Scenery: There are three types of scenery collision:
       - Low speed bump. Car rises slightly in the air and stalls.
       - Mid speed spin. Car spins and slides after collision.
       - High speed flip. If slightly slower, car rolls into screen.
         Otherwise, grows towards screen and vanishes
         
    Known Issues With Original Code:
    - Passenger sprites flicker if they land moving in the water on Stage 1
    
    The Ferrari sprite is used differently by the crash code.
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/oferrari.h"
#include "engine/oinputs.h"
#include "engine/olevelobjs.h"
#include "engine/outils.h"
#include "engine/ocrash.h"


oentry* OCrash_spr_ferrari;
oentry* OCrash_spr_shadow;
oentry* OCrash_spr_pass1;
oentry* OCrash_spr_pass1s;
oentry* OCrash_spr_pass2;
oentry* OCrash_spr_pass2s;
int8_t OCrash_crash_state;
int16_t OCrash_skid_counter;
int16_t OCrash_skid_counter_bak;
uint8_t OCrash_spin_control1;
uint8_t OCrash_spin_control2;
int16_t OCrash_coll_count1;
int16_t OCrash_coll_count2;
int16_t OCrash_crash_counter;
int16_t OCrash_crash_spin_count;
int16_t OCrash_crash_z;


// This is the rolled Ferrari sprite, which is configured differently for
// the crash code. 
// The offsets indicate the original offsets in memory.

//+1E [Word] Spins/Flips Remaining
int16_t spinflipcount1;
//+22 [Word] Crash Spin/Flip Count Copy
int16_t spinflipcount2;
//+24 [Word] Crash Slide Value (After Spin/Flip etc.)
int16_t slide;
//+26 [Word] Frame (actually an index into the Sprite format below)
static int16_t frame;
//+28 [Long] Address of animation sequence (frame address, palette info etc.)
uint32_t addr;
//+2C [Word] Camera Pan X Target (for repositioning after crash)
int16_t camera_x_target;
//+2E [Word] Camera Pan Increment
int16_t camera_xinc;
//+30 [Word] Index into movement lookup table (to set y position of car during low speed bump)
int16_t lookup_index;
//+32 [Word] Frame to restore car to after bump routine
int16_t frame_restore;
//+34 [Word] Used as a shift value to change y position during shunt
int16_t shift;
//+36 [Word] Flip Only: 0 = Fast Crash. 1 = Slow Crash.
int16_t crash_speed;
//+38 [Word] Crash Z Increment (How much to change Crash Z Per Tick)
int16_t OCrash_crash_zinc;
//+3A [Word] 0 = RHS, 1 = LHS?
int16_t crash_side;

// Passenger Frame to use during spin
int16_t spin_pass_frame;

int8_t crash_type;
enum { CRASH_BUMP = 0, CRASH_SPIN = 1, CRASH_FLIP = 2 };

// Delay counter after crash. 
// Show animation (e.g. girl pointing finger, before car is repositioned)
int16_t crash_delay;

void OCrash_do_crash();
void OCrash_spin_switch(const uint16_t);
void OCrash_crash_switch();

void OCrash_init_collision();
void OCrash_do_collision();
void OCrash_end_collision();

void OCrash_do_bump();
void OCrash_do_car_flip();
void OCrash_init_finger(uint32_t);
void OCrash_trigger_smoke();
void OCrash_post_flip_anim();
void OCrash_pan_camera();

void OCrash_init_spin1();
void OCrash_init_spin2();
void OCrash_collide_slow();
void OCrash_collide_med();
void OCrash_collide_fast();

void OCrash_done(oentry*);

void OCrash_do_shadow(oentry*, oentry*);

// Pointers to functions for crash code
void (*OCrash_function_pass1)(oentry*);
void (*OCrash_function_pass2)(oentry*);

void OCrash_do_crash_passengers(oentry*);
void OCrash_flip_start(oentry*);

void OCrash_crash_pass1(oentry*);
void OCrash_crash_pass2(oentry*);
void OCrash_crash_pass_flip(oentry*);

void OCrash_pass_flip(oentry*);
void OCrash_pass_situp(oentry*);
void OCrash_pass_turnhead(oentry*);

void OCrash_init(oentry* f, oentry* s, oentry* p1, oentry* p1s, oentry* p2, oentry* p2s)
{
    OCrash_spr_ferrari = f;
    OCrash_spr_shadow  = s;
    OCrash_spr_pass1   = p1;
    OCrash_spr_pass1s  = p1s;
    OCrash_spr_pass2   = p2;
    OCrash_spr_pass2s  = p2s;

    // Setup function pointers for passenger sprites
    OCrash_function_pass1 = &OCrash_do_crash_passengers;
    OCrash_function_pass2 = &OCrash_do_crash_passengers;
}

uint8_t OCrash_is_flip()
{
    return OCrash_crash_counter && crash_type == CRASH_FLIP;
}

void OCrash_enable()
{
    // This is called multiple times, so need this check in place
    if (OCrash_spr_ferrari->control & SPRITES_ENABLE) 
        return;

    OCrash_spr_ferrari->control |= SPRITES_ENABLE;
    
    // Reset all corresponding variables
    spinflipcount1 = 0;
    spinflipcount2 = 0;
    slide = 0;
    frame = 0;
    addr = 0;
    camera_x_target = 0;
    camera_xinc = 0;
    lookup_index = 0;
    frame_restore = 0;
    shift = 0;
    crash_speed = 0;
    OCrash_crash_zinc = 0;
    crash_side = 0;

    OCrash_spr_ferrari->counter = 0;

    Outrun_ttrial.crashes++;
}

// Source: 0x1128
void OCrash_clear_crash_state()
{
    OCrash_spin_control1 = 0;
    OCrash_coll_count1 = 0;
    OCrash_coll_count2 = 0;
    OLevelObjs_collision_sprite = 0;
    OCrash_crash_counter = 0;
    OCrash_crash_state = 0;
    OCrash_crash_z = 0;
    spin_pass_frame = 0;
    OCrash_crash_spin_count = 0;
    crash_delay = 0;
    crash_type  = 0;
    OCrash_skid_counter = 0;
}

void OCrash_tick()
{
    if (!Outrun_tick_frame && 
        ORoad_get_view_mode() == ROAD_VIEW_INCAR &&
        crash_type != CRASH_FLIP)
        return;

    // Do Ferrari
    if (OCrash_spr_ferrari->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_do_crash();
        else OSprites_do_spr_order_shadows(OCrash_spr_ferrari);

    // Do Car Shadow
    if (OCrash_spr_shadow->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_do_shadow(OCrash_spr_ferrari, OCrash_spr_shadow);
        else OSprites_do_spr_order_shadows(OCrash_spr_shadow);

    // Do Passenger 1
    if (OCrash_spr_pass1->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_function_pass1(OCrash_spr_pass1);
        else OSprites_do_spr_order_shadows(OCrash_spr_pass1);

    // Do Passenger 1 Shadow
    if (OCrash_spr_pass1s->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_do_shadow(OCrash_spr_pass1, OCrash_spr_pass1s);
        else OSprites_do_spr_order_shadows(OCrash_spr_pass1s);

    // Do Passenger 2
    if (OCrash_spr_pass2->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_function_pass2(OCrash_spr_pass2);
        else OSprites_do_spr_order_shadows(OCrash_spr_pass2);

    // Do Passenger 2 Shadow
    if (OCrash_spr_pass2s->control & SPRITES_ENABLE)
        if (Outrun_tick_frame) OCrash_do_shadow(OCrash_spr_pass2, OCrash_spr_pass2s);
        else OSprites_do_spr_order_shadows(OCrash_spr_pass2s);
}

// Source: 0x1162
void OCrash_do_crash()
{
    switch (Outrun_game_state)
    {
        case GS_INIT_MUSIC:
        case GS_MUSIC:
            OCrash_end_collision();
            return;

        // Fall through to continue code below
        case GS_ATTRACT:
        case GS_INGAME:
        case GS_BONUS:
            break;

        default:
            // In other modes render crashing ferrari if crash counter is set
            if (OCrash_crash_counter)
            {
                if (ORoad_get_view_mode() != ROAD_VIEW_INCAR || crash_type == CRASH_FLIP)
                    OSprites_do_spr_order_shadows(OCrash_spr_ferrari);
            }
            // Set Distance Into Screen from crash counter
            OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
            return;
    }

    // ------------------------------------------------------------------------
    // cont1: Adjust steering
    // ------------------------------------------------------------------------
    int16_t steering_adjust = OInputs_steering_adjust;
    OInputs_steering_adjust = 0;

    if (!OFerrari_car_ctrl_active)
    {
        if (OCrash_spin_control1 == 0)
        {
            uint16_t inc = ((OInitEngine_car_increment >> 16) * 31) >> 5;
            OInitEngine_car_increment = (OInitEngine_car_increment & 0xFFFF) | (inc << 16);
            OFerrari_car_inc_old = inc;
        }
        else
        {
            OInputs_steering_adjust = steering_adjust >> 1;
        }
    }

    // ------------------------------------------------------------------------
    // Determine whether to init spin or crash code
    // ------------------------------------------------------------------------
    int16_t spin2_copy = OCrash_spin_control2;

    // dec_spin2:
    if (spin2_copy != 0)
    {
        spin2_copy -= 2;
        if (spin2_copy < 0)
            OCrash_spin_control2 = 0;
        else
            OCrash_spin_switch(spin2_copy);
    }

    // dec_spin1:
    int16_t spin1_copy = OCrash_spin_control1;

    if (spin1_copy != 0)
    {
        spin1_copy -= 2;
        if (spin1_copy < 0)
            OCrash_spin_control1 = 0;
        else
            OCrash_spin_switch(spin1_copy);
    }
    // Not spinning, init crash code
    else
        OCrash_crash_switch();
}

// Source: 0x1224
void OCrash_spin_switch(const uint16_t ctrl)
{
    OCrash_crash_counter++;
    OCrash_crash_z = 0;
    
    switch (ctrl & 3)
    {
        // No Spin - Need to init crash/spin routines
        case 0:
            OCrash_init_collision();
            break;
        // Init Spin
        case 1:
            OCrash_do_collision();
            break;
        // Spin In Progress - End Collision Routine
        case 2:
        case 3:
            OCrash_end_collision();
            break;
    }
}

// Init Crash.
//
// Source: 0x1252
void OCrash_crash_switch()
{
    OCrash_crash_counter++;
    OCrash_crash_z = 0;

    switch (OCrash_crash_state & 7)
    {
        // No Crash. Need to setup crash routines.
        case 0:
            OCrash_init_collision();
            break;
        // Initial Collision
        case 1:
            if ((crash_type & 3) == 0) OCrash_do_bump();
            else OCrash_do_collision();
            break;
        // Flip Car
        case 2:
            OCrash_do_car_flip();
            break;
        // Horizontally Flip Car, Trigger Smoke Cloud
        case 3:
        case 4:
            OCrash_trigger_smoke();
            break;
        // Do Girl Pointing Finger Animation/Delay Before Pan
        case 5:
            OCrash_post_flip_anim();
            break;
        // Pan Camera To Track Centre
        case 6:
            OCrash_pan_camera();
            break;
        // Camera Repositioned. Prepare For Restart.
        case 7:
            OCrash_end_collision();
            break;
    }
}

// Init Collision. Used For Spin & Flip
//
// Source: 0x1962
void OCrash_init_collision()
{
    OFerrari_car_state = CAR_ANIM_SEQ; // Denote car animation sequence

    // Enable crash sprites
    OCrash_spr_shadow->control |= SPRITES_ENABLE;
    OCrash_spr_pass1->control  |= SPRITES_ENABLE;
    OCrash_spr_pass2->control  |= SPRITES_ENABLE;

    // Disable normal sprites
    OFerrari_spr_ferrari->control &= ~SPRITES_ENABLE;
    OFerrari_spr_shadow->control  &= ~SPRITES_ENABLE;
    OFerrari_spr_pass1->control   &= ~SPRITES_ENABLE;
    OFerrari_spr_pass2->control   &= ~SPRITES_ENABLE;

    OCrash_spr_ferrari->x = OFerrari_spr_ferrari->x;
    OCrash_spr_ferrari->y = 221;
    OCrash_spr_ferrari->counter = 0x1FC;
    OCrash_spr_ferrari->draw_props = DRAW_BOTTOM;

    // Collided with another vechicle
    if (OCrash_spin_control2)
        OCrash_init_spin2();
    else if (OCrash_spin_control1)
        OCrash_init_spin1();
    // Crash into scenery
    else
    {
        OCrash_skid_counter = 0;
        uint16_t car_inc = OInitEngine_car_increment >> 16;
        if (car_inc < 0x64)
            OCrash_collide_slow();
        else if (car_inc < 0xC8)
            OCrash_collide_med();
        else
            OCrash_collide_fast();
    }
}

// This code also triggers a flip, if the crash_type is set correctly.
// Source: 0x138C
void OCrash_do_collision()
{
    if (OLevelObjs_collision_sprite)
    {
        OLevelObjs_collision_sprite = 0;
        if (OCrash_spin_control1 || OCrash_spin_control2)
        {
            OCrash_spin_control2 = 0;
            OCrash_spin_control1 = 0;
            OCrash_init_collision(); // Init collision with another sprite
            return;
        }

        // Road generator 1
        if (OInitEngine_car_x_pos - (ORoad_road_width >> 16) >= 0)
        {
            if (slide < 0)
            {
                slide = -slide;
                OInitEngine_car_x_pos -= slide;
                OSoundInt_queue_sound(sound_CRASH2);
            }
        }
        // Road generator 2
        else
        {
            if (slide >= 0)
            {
                slide = -slide;
                OInitEngine_car_x_pos -= slide;
                OSoundInt_queue_sound(sound_CRASH2);
            }
        }
    }
    // 0x13F8
    uint32_t property_table = addr + (frame << 3);
    OCrash_crash_z = OCrash_spr_ferrari->counter;
    OCrash_spr_ferrari->zoom = 0x80;
    OCrash_spr_ferrari->priority = 0x1FD;
    OInitEngine_car_x_pos -= slide;
    OCrash_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, property_table);

    if (RomLoader_read8(Roms_rom0p, 4 + property_table))
        OCrash_spr_ferrari->control |= SPRITES_HFLIP;
    else
        OCrash_spr_ferrari->control &= ~SPRITES_HFLIP;

    OCrash_spr_ferrari->pal_src = RomLoader_read8(Roms_rom0p, 5 + property_table);
    spin_pass_frame = (int8_t) RomLoader_read8(Roms_rom0p, 6 + property_table);

    if (--spinflipcount2 > 0)
    {
        OCrash_done(OCrash_spr_ferrari);
        return;
    }

    spinflipcount2 = OCrash_crash_spin_count; // Expired: spinflipcount

    if (spinflipcount1)
    {
        frame++;

        // 0x1470
        // Initialize Car Flip
        if (!OCrash_spin_control1 && !OCrash_spin_control2 && frame == 2 && crash_type != CRASH_SPIN)
        {
            OCrash_crash_state = 2; // flip
            addr = Outrun_adr.sprite_crash_flip;
            spinflipcount1 = 3; // 3 flips remaining
            spinflipcount2 = OCrash_crash_spin_count;
            frame = 0;

            // Enable passenger shadows
            OCrash_spr_pass1s->control |= SPRITES_ENABLE;
            OCrash_spr_pass2s->control |= SPRITES_ENABLE;
            OCrash_done(OCrash_spr_ferrari);
            return;
        }
        // Do Spin
        else
        {
            if (slide > 0)
                slide -= 2;
            else if (slide < -2)
                slide += 2;

            // End of frame sequence
            if (!RomLoader_read8(Roms_rom0p, 7 + property_table))
            {
                OCrash_done(OCrash_spr_ferrari);
                return;
            }
        }
    }
    // 0x14F4
    frame = 0;
    
    // Last spin
    if (--spinflipcount1 <= 0)
    {
        OSoundInt_queue_sound(sound_STOP_SLIP);
        if (OCrash_spin_control2)
        {
            OCrash_spin_control2++;
        }
        else if (OCrash_spin_control1)
        {
            OCrash_spin_control1++;
        }
        // Init smoke
        else
        {
            OCrash_crash_state = 4; // trigger smoke
            OCrash_crash_spin_count = 1;
            OCrash_spr_ferrari->x += slide; // inc ferrari x based on slide value
        }
    }
    else
        OCrash_crash_spin_count++;

    OCrash_done(OCrash_spr_ferrari);
}

// Source: 0x1D0C
void OCrash_end_collision()
{
    // Enable 'normal' Ferrari object
    OFerrari_spr_ferrari->control |= SPRITES_ENABLE;
    OFerrari_spr_shadow->control  |= SPRITES_ENABLE;
    OFerrari_spr_pass1->control   |= SPRITES_ENABLE;
    OFerrari_spr_pass2->control   |= SPRITES_ENABLE;

    OCrash_coll_count2 = OCrash_coll_count1;
    if (!OCrash_coll_count2)
        OCrash_coll_count2 = OCrash_coll_count1 = 1;

    OCrash_crash_counter = 0;
    OCrash_crash_state = 0;
    OLevelObjs_collision_sprite = 0;
    
    OFerrari_spr_ferrari->x = 0;
    OFerrari_spr_ferrari->y = 221;
    OFerrari_car_ctrl_active = 1;
    OFerrari_car_state = CAR_NORMAL;
    OLevelObjs_spray_counter = 0;
    OCrash_crash_z = 0;

    if (OCrash_spin_control1)
        OFerrari_car_inc_old = OInitEngine_car_increment >> 16;
    else
        OFerrari_reset_car();

    OCrash_spin_control2 = 0;
    OCrash_spin_control1 = 0;

    OCrash_spr_ferrari->control &= ~SPRITES_ENABLE;
    OCrash_spr_shadow->control  &= ~SPRITES_ENABLE;
    OCrash_spr_pass1->control   &= ~SPRITES_ENABLE;
    OCrash_spr_pass1s->control  &= ~SPRITES_ENABLE;
    OCrash_spr_pass2->control   &= ~SPRITES_ENABLE;
    OCrash_spr_pass2s->control  &= ~SPRITES_ENABLE;

    OCrash_function_pass1 = &OCrash_do_crash_passengers;
    OCrash_function_pass2 = &OCrash_do_crash_passengers;
    OInputs_crash_input = 0x10; // Set delay in processing steering
}

// Low Speed Bump - Car Rises in air and sinks
// Source: 0x12BE
void OCrash_do_bump()
{
    OFerrari_car_ctrl_active = 0;   // Disable user control of car
    OCrash_spr_ferrari->zoom = 0x80;           // Set Entry Number For Zoom Lookup Table
    OCrash_spr_ferrari->priority = 0x1FD;
    
    int16_t new_position = (int8_t) RomLoader_read8(&Roms_rom0, DATA_MOVEMENT + (lookup_index << 3));

    if (new_position)
        OCrash_crash_z = OCrash_spr_ferrari->counter;

    OCrash_spr_ferrari->y = 221 - (new_position >> shift);

    uint32_t frames = addr + (frame << 3);
    OCrash_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, frames);
    
    if (RomLoader_read8(Roms_rom0p, frames + 4))
        OCrash_spr_ferrari->control |= SPRITES_HFLIP;
    else
        OCrash_spr_ferrari->control &= ~SPRITES_HFLIP;
    
    OCrash_spr_ferrari->pal_src = RomLoader_read8(Roms_rom0p, frames + 5);
    spin_pass_frame = (int8_t) RomLoader_read8(Roms_rom0p, frames + 6);

    if (++lookup_index >= 0x10)
    {
        addr += (frame_restore << 3);
        OCrash_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, addr);
        spin_pass_frame = (int8_t) RomLoader_read8(Roms_rom0p, addr + 6);
        OCrash_crash_state = 4;      // Trigger smoke cloud
        OCrash_crash_spin_count = 1; // Denote Crash
    }

    OCrash_done(OCrash_spr_ferrari);
}

// Source: 0x1562
void OCrash_do_car_flip()
{
    // Do this if during the flip, the car has recollided with a new sprite + slow crash (similar to spin_collide)
    if (OLevelObjs_collision_sprite && crash_speed == 1)
    {
        uint16_t car_inc16 = OInitEngine_car_increment >> 16;

        // Road generator 1
        if (OInitEngine_car_x_pos - (ORoad_road_width >> 16) >= 0)
        {
            // swap_slide_dir2
            if (slide < 0)
            {
                slide = -slide;
                OInitEngine_car_increment = ((car_inc16 >> 1) << 16) | (OInitEngine_car_increment & 0xFFFF);
                OSoundInt_queue_sound(sound_CRASH2);
                if (OInitEngine_car_increment >> 16 > 0x14)
                {
                    int16_t z = OCrash_spr_ferrari->counter > 0x1FD ? 0x1FD : OCrash_spr_ferrari->counter; // d3
                    int16_t x_adjust = (0x50 * z) >> 9; // d1
                    if (slide < 0) x_adjust = -x_adjust;
                    OInitEngine_car_x_pos -= x_adjust;
                }
            }
            // 0x15F6
            else
                slide += (slide >> 3);
        }
        // Road generator 2
        else
        {
            // swap_slide_dir2:
            if (slide >= 0)
            {
                slide = -slide;
                OInitEngine_car_increment = ((car_inc16 >> 1) << 16) | (OInitEngine_car_increment & 0xFFFF);
                OSoundInt_queue_sound(sound_CRASH2);
                if (OInitEngine_car_increment >> 16 > 0x14)
                {
                    int16_t z = OCrash_spr_ferrari->counter > 0x1FD ? 0x1FD : OCrash_spr_ferrari->counter; // d3
                    int16_t x_adjust = (0x50 * z) >> 9; // d1
                    if (slide < 0) x_adjust = -x_adjust;
                    OInitEngine_car_x_pos -= x_adjust;
                }
            }
            // 0x15F6
            else
                slide += (slide >> 3);
        }
    }
    
    // flip_cont
    OLevelObjs_collision_sprite = 0; // Moved this for clarity
    uint32_t frames = addr + (frame << 3);
    OCrash_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, frames);

    // ------------------------------------------------------------------------
    // Fast Crash: Car Heads towards camera in sky, before vanishing (0x161E)
    // ------------------------------------------------------------------------
    if (crash_speed == 0)
    {
        OCrash_spr_shadow->control &= ~SPRITES_ENABLE; // Disable Shadow
        OCrash_spr_ferrari->counter += OCrash_crash_zinc;       // Increment Crash Z
        if (OCrash_spr_ferrari->counter > 0x3FF)
        {
            OCrash_spr_ferrari->zoom = 0;
            OCrash_spr_ferrari->counter = 0;
            OCrash_init_finger(frames);
            OCrash_done(OCrash_spr_ferrari);
            return;
        }
        else
            OCrash_crash_zinc++;
    }
    // ------------------------------------------------------------------------
    // Slow Crash (0x1648 flip_slower)
    // ------------------------------------------------------------------------
    else
    {
        OCrash_spr_ferrari->counter -= OCrash_crash_zinc;       // Decrement Crash Z
        if (OCrash_crash_zinc > 2)
            OCrash_crash_zinc--;
    }

    // set_OCrash_crash_z_inc
    // Note that we've set OCrash_crash_zinc previously now for clarity

    // use ferrari_OCrash_crash_z to set priority
    OCrash_spr_ferrari->priority = OCrash_spr_ferrari->counter > 0x1FD ? 0x1FD : OCrash_spr_ferrari->counter;

    int16_t x_diff = (slide * OCrash_spr_ferrari->priority) >> 9;
    OInitEngine_car_x_pos -= x_diff;

    int16_t passenger_frame = (int8_t) RomLoader_read8(Roms_rom0p, 6 + frames);

    // Start of sequence
    if (passenger_frame == 0)
    {
        slide >>= 1;
        OSoundInt_queue_sound(sound_CRASH2);
    }

    // Set Z during lower frames
    if (passenger_frame <= 0x10 && OCrash_spr_ferrari->counter <= 0x1FE)
        OCrash_crash_z = OCrash_spr_ferrari->counter;

    // 0x16CC
    passenger_frame = (passenger_frame * OCrash_spr_ferrari->priority) >> 9;

    // Set Ferrari Y
    int16_t y = -(ORoad_road_y[ORoad_road_p0 + OCrash_spr_ferrari->priority] >> 4) + 223;
    y -= passenger_frame;
    OCrash_spr_ferrari->y = y;

    // Set Ferrari Zoom from Z
    OCrash_spr_ferrari->zoom = (OCrash_spr_ferrari->counter >> 2);
    if (OCrash_spr_ferrari->zoom < 0x40) OCrash_spr_ferrari->zoom = 0x40;

    // Set Ferrari H-Flip
    if (crash_side) 
        OCrash_spr_ferrari->control |= SPRITES_HFLIP;
    else 
        OCrash_spr_ferrari->control &= ~SPRITES_HFLIP;

    OCrash_spr_ferrari->pal_src = RomLoader_read8(Roms_rom0p, 4 + frames);

    if (--spinflipcount2 > 0)
    {
        OCrash_done(OCrash_spr_ferrari);
        return;
    }

    spinflipcount2 = OCrash_crash_spin_count;

    // 0x1736
    // Advance to next frame in sequence
    if (spinflipcount1)
    {
        frame++;
        // End of frame sequence
        if ((RomLoader_read8(Roms_rom0p, 7 + frames) & BIT_7) == 0)
        {
            OCrash_done(OCrash_spr_ferrari);
            return;
        }
    }

    frame = 0;
    
    if (--spinflipcount1 > 0)
    {
        OCrash_crash_spin_count++;
    }
    else
    {
        OCrash_init_finger(frames);
    }

    OCrash_done(OCrash_spr_ferrari);
}

// Init Delay/Girl Pointing Finger
// Source: 0x175C
void OCrash_init_finger(uint32_t frames)
{
    OCrash_crash_spin_count = 1;           // Denote Crash has taken place
    
    // Do Delay whilst girl points finger
    if (crash_type == CRASH_FLIP)
    {
        OFerrari_wheel_state = WHEELS_ON;
        OFerrari_car_state   = CAR_NORMAL;
        slide = 0;
        addr += frames;
        crash_delay = 30;
        OCrash_crash_state = 5; 
    }
    // Slide Car and Trigger Smoke Cloud
    else
    {
        OCrash_crash_state = 3;
        frame = 0;
        addr = Outrun_adr.sprite_crash_man1;
        OCrash_crash_spin_count = 4;   // Denote third flip
        spinflipcount2   = 4;
    }
}

// Post Crash: Slide Car Slightly, then trigger smoke
// Source: 0x17D2
void OCrash_trigger_smoke()
{
    OCrash_crash_z = OCrash_spr_ferrari->counter;
    int16_t slide_copy = slide;

    if (slide < 0)
        slide++;
    else if (slide > 0)
        slide--;

    // Slide Car
    OInitEngine_car_x_pos -= slide_copy;

    OCrash_spr_ferrari->addr = RomLoader_read32(Roms_rom0p, addr);

    // Set Ferrari H-Flip
    if (RomLoader_read8(Roms_rom0p, 4 + addr))
        OCrash_spr_ferrari->control |= SPRITES_HFLIP;
    else 
        OCrash_spr_ferrari->control &= ~SPRITES_HFLIP;

    OCrash_spr_ferrari->pal_src =     RomLoader_read8(Roms_rom0p, 5 + addr);
    spin_pass_frame = (int8_t) RomLoader_read8(Roms_rom0p, 6 + addr);

    // Slow Car
    OInitEngine_car_increment = 
        (OInitEngine_car_increment - ((OInitEngine_car_increment >> 2) & 0xFFFF0000)) 
        | (OInitEngine_car_increment & 0xFFFF);

    // Car stationary
    if (OInitEngine_car_increment >> 16 <= 0)
    {
        OInitEngine_car_increment = 0;
        OFerrari_wheel_state = WHEELS_ON;
        OFerrari_car_state   = CAR_NORMAL;
        slide = 0;
        crash_delay = 30;
        OCrash_crash_state = 5; // post crash animation delay state
    }

    OCrash_done(OCrash_spr_ferrari);
}

// Source: 0x1870
void OCrash_post_flip_anim()
{
    OFerrari_car_ctrl_active = 0;  // Car and road updates disabled
    if (--crash_delay > 0)
    {
        OCrash_done(OCrash_spr_ferrari);
        return;
    }

    OFerrari_car_ctrl_active = 1;
    OCrash_crash_state = 6; // Denote pan camera to track centre
    
    int16_t road_width = ORoad_road_width >> 16;
    int16_t car_x_pos  = OInitEngine_car_x_pos;
    camera_xinc = 8;
    
    // Double Road
    if (road_width >= 0xD7)
    {
        if (car_x_pos < 0)
            road_width = -road_width;
    }
    else
    {
        road_width = 0;
    }

    camera_x_target = road_width;

    // 1/ Car on road generator 1 (1 road enabled)
    // 2/ Car on road generator 1 (2 roads enabled)
    if (road_width < car_x_pos)
    {
        camera_xinc += (car_x_pos - road_width) >> 6;
        camera_xinc = -camera_xinc;
    }
    else
    {
        camera_xinc += (road_width - car_x_pos) >> 6;
    }

    OCrash_done(OCrash_spr_ferrari);
}

// Pan Camera Back To Centre After Flip
// Source: 0x18EC
void OCrash_pan_camera()
{
    OFerrari_car_ctrl_active = 1;

    OInitEngine_car_x_pos += camera_xinc;

    int16_t x_diff = (OFerrari_car_x_diff * OCrash_spr_ferrari->counter) >> 9;
    OCrash_spr_ferrari->x += x_diff;

    // Pan Right
    if (camera_xinc >= 0)
    {
        if (camera_x_target <= OInitEngine_car_x_pos)
            OCrash_crash_state = 7; // Denote camera position and ready for restart
    }
    // Pan Left
    else
    {
        if (camera_x_target >= OInitEngine_car_x_pos)
            OCrash_crash_state = 7;
    }

    OCrash_done(OCrash_spr_ferrari);
}

// Source: 0x1C7E
void OCrash_init_spin1()
{
    OSoundInt_queue_sound(sound_INIT_SLIP);
    uint16_t car_inc = OInitEngine_car_increment >> 16;
    uint16_t spins = 1;
    if (car_inc > 0xB4)
        spins += outils_random() & 1;

    spinflipcount1 = spins;
    OCrash_crash_spin_count = 2;
    spinflipcount2 = 2;

    slide = ((spins + 1) << 2) + (car_inc > 0xFF) ? 0xFF >> 3 : car_inc >> 3;

    if (OCrash_skid_counter_bak < 0)
        addr = Outrun_adr.sprite_crash_spin1;
    else
    {
        addr = Outrun_adr.sprite_crash_spin1;
        slide = -slide;
    }
    
    OCrash_spin_control1++;
    frame = 0;
    OCrash_skid_counter = 0;
    OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
}

// Source: 0x1C10
void OCrash_init_spin2()
{
    OSoundInt_queue_sound(sound_INIT_SLIP);
    uint16_t car_inc = OInitEngine_car_increment >> 16;
    spinflipcount1 = 1;
    OCrash_crash_spin_count = 2;
    spinflipcount2 = 8;

    slide = (car_inc > 0xFF) ? 0xFF >> 3 : car_inc >> 3;

    if (OInitEngine_road_type != INITENGINE_ROAD_RIGHT)
    {
        addr = Outrun_adr.sprite_crash_spin1;  
    }
    else
    {
        addr = Outrun_adr.sprite_crash_spin1;
        slide = -slide;
    }

    OCrash_spin_control2++;
    frame = 0;
    OCrash_skid_counter = 0;
    OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
}

// Collision: Slow 
// Rebound and bounce car in air
// Source: 0x19EE
void OCrash_collide_slow()
{
    OSoundInt_queue_sound(sound_REBOUND);
    
    // Setup shift value for car bump, based on current speed, which ultimately determines how much car rises in air
    uint16_t car_inc = OInitEngine_car_increment >> 16;

    if (car_inc <= 0x28)
        shift = 6;
    else if (car_inc <= 0x46)
        shift = 5;
    else
        shift = 4;

    lookup_index = 0;

    // Calculate change in road y, so we can determine incline frame for ferrari
    int16_t y = ORoad_road_y[ORoad_road_p0 + (0x3E0 / 2)] - ORoad_road_y[ORoad_road_p0 + (0x3F0 / 2)];
    frame_restore = 0;
    if (y >= 0x12) frame_restore++;
    if (y >= 0x13) frame_restore++;
    
    // Right Hand Side: Increment Frame Entry By 3
    if (OInitEngine_car_x_pos < 0)
        addr = Outrun_adr.sprite_bump_data2;
    else
        addr = Outrun_adr.sprite_bump_data1;

    crash_type = CRASH_BUMP; // low speed bump
    OInitEngine_car_increment &= 0xFFFF;

    // set_collision:
    frame = 0;
    OCrash_crash_state = 1; // collision with object
    OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
}

// Collision: Medium
// Spin car
// Source: 0x1A98
void OCrash_collide_med()
{
    OSoundInt_queue_sound(sound_INIT_SLIP);
    
    // Set number of spins based on car speed
    uint16_t car_inc = OInitEngine_car_increment >> 16;    
    spinflipcount1 = car_inc <= 0x96 ? 1 : 2;
    spinflipcount2 = OCrash_crash_spin_count = 2;
    slide = ((spinflipcount1 + 1) << 2) + ((car_inc > 0xFF ? 0xFF : car_inc) >> 3);

    // Right Hand Side: Increment Frame Entry By 3
    if (OInitEngine_car_x_pos < 0)
    {
        addr = Outrun_adr.sprite_crash_spin1;
        slide = -slide;
    }
    else
        addr = Outrun_adr.sprite_crash_spin1;

    crash_type = CRASH_SPIN;

    // set_collision:
    frame = 0;
    OCrash_crash_state = 1; // collision with object
    OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
}

// Collision: Fast
// Spin, Then Flip Car
//
// Source: 0x1B12
void OCrash_collide_fast()
{
    OSoundInt_queue_sound(sound_CRASH1);

    uint16_t car_inc = OInitEngine_car_increment >> 16;
    if (car_inc > 0xFA)
    {
        OCrash_crash_zinc = 1;
        crash_speed = 0;
    }
    else
    {
        OCrash_crash_zinc = 0x10;
        crash_speed = 1;
    }

    spinflipcount1 = 1;
    spinflipcount2 = OCrash_crash_spin_count = 2;
    
    slide = (car_inc > 0xFF ? 0xFF : car_inc) >> 2;
    slide += (slide >> 1);

    if (OInitEngine_road_type != INITENGINE_ROAD_STRAIGHT)
    {
        int16_t d2 = (0x78 - (OInitEngine_road_curve <= 0x78 ? OInitEngine_road_curve : 0x78)) >> 1;

        // collide_fast_curve:
        slide += d2;
        if (OInitEngine_road_type == INITENGINE_ROAD_RIGHT)
            slide = -slide;
    }
    else
    {
        if (OInitEngine_car_x_pos < 0) slide = -slide; // rhs
    }
    
    // set_fast_slide:
    if (slide > 0x78) 
        slide = 0x78;

    if (OInitEngine_car_x_pos < 0)
    {
        addr = Outrun_adr.sprite_crash_spin2;
        crash_side = 0; // RHS
    }
    else
    {
        addr = Outrun_adr.sprite_crash_spin1;
        crash_side = 1; // LHS
    }

    crash_type = CRASH_FLIP; // Flip

    // set_collision:
    frame = 0;
    OCrash_crash_state = 1; // collision with object
    OCrash_spr_ferrari->road_priority = OCrash_spr_ferrari->counter;
}

// Source: 0x1556
void OCrash_done(oentry* sprite)
{
    OSprites_map_palette(sprite);

    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR || crash_type == CRASH_FLIP)
        OSprites_do_spr_order_shadows(sprite);

    sprite->road_priority = sprite->counter;
}

// ------------------------------------------------------------------------------------------------
//                                      SPRITES_SHADOW CRASH ROUTINES
// ------------------------------------------------------------------------------------------------

// Render Shadow
//
// Disabled during fast car flip, when car rapidly heading towards screen
//
// Source: 0x1DF2
void OCrash_do_shadow(oentry* src_sprite, oentry* dst_sprite)
{
    uint8_t shadow_shift;

    // Ferrari Shadow
    if (src_sprite == OCrash_spr_ferrari)
    {
        dst_sprite->draw_props = DRAW_BOTTOM;
        shadow_shift = 1;
    }
    else
    {
        shadow_shift = 3;
    }

    dst_sprite->x = src_sprite->x;
    dst_sprite->road_priority = src_sprite->road_priority;

    // Get Z from source sprite (stored in counter)
    uint16_t counter = (src_sprite->counter) >> shadow_shift;
    counter = counter - (counter >> 2);
    dst_sprite->zoom = (uint8_t) counter;

    // Set shadow y
    uint16_t offset = src_sprite->counter > 0x1FF ? 0x1FF : src_sprite->counter;
    dst_sprite->y = -(ORoad_road_y[ORoad_road_p0 + offset] >> 4) + 223;

    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR || crash_type == CRASH_FLIP)
        OSprites_do_spr_order_shadows(dst_sprite);
}

// ------------------------------------------------------------------------------------------------
//                                     PASSENGER CRASH ROUTINES
// ------------------------------------------------------------------------------------------------

// Flips & Spins Only
//
// Process passengers during crash scenario.
//
// Source: 0x1E66
void OCrash_flip_start(oentry* sprite); // forward declaration

void OCrash_do_crash_passengers(oentry* sprite)
{
    // --------------------------------------------------------------------------------------------
    // Flip car
    // --------------------------------------------------------------------------------------------
    if (OCrash_crash_state == 2)
    {
        // Update pointer to functions
        if (sprite == OCrash_spr_pass1) 
            OCrash_function_pass1 = &OCrash_flip_start;
        else if (sprite == OCrash_spr_pass2) 
            OCrash_function_pass2 = &OCrash_flip_start;

        // Crash Passenger Flip
        OCrash_crash_pass_flip(sprite);
        return;
    }

    // --------------------------------------------------------------------------------------------
    // Non-Flip
    // --------------------------------------------------------------------------------------------
    if (OCrash_crash_state < 5)
        OCrash_crash_pass1(sprite);
    else
        OCrash_crash_pass2(sprite);

    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR || crash_type == CRASH_FLIP)
    {
        OSprites_map_palette(sprite);
        OSprites_do_spr_order_shadows(sprite);
    }
}

// Position Passenger Sprites During Crash (But Not Flip)
//
// - Process passenger sprites in crash scenario
// - Called separately for man and girl
//
// Source: 0x1EA6
void OCrash_crash_pass1(oentry* sprite)
{
    uint32_t frames = (sprite == OCrash_spr_pass1 ? Outrun_adr.sprite_crash_man1 : Outrun_adr.sprite_crash_girl1) + (spin_pass_frame << 3);
    
    sprite->addr    = RomLoader_read32(Roms_rom0p, frames);
    uint8_t props   = RomLoader_read8(Roms_rom0p, 4 + frames);
    sprite->pal_src = RomLoader_read8(Roms_rom0p, 5 + frames);
    sprite->x       = OCrash_spr_ferrari->x + (int8_t) RomLoader_read8(Roms_rom0p, 6 + frames);
    sprite->y       = OCrash_spr_ferrari->y + (int8_t) RomLoader_read8(Roms_rom0p, 7 + frames);

    // Check H-Flip
    if (props & BIT_7)
        sprite->control |= SPRITES_HFLIP;
    else
        sprite->control &= ~SPRITES_HFLIP;

    // Test whether we should set priority higher (unused on passenger sprites I think)
    if (props & BIT_0)
        sprite->priority = sprite->road_priority = 0x1FE;
    else
        sprite->priority = sprite->road_priority = 0x1FD;

    sprite->zoom = 0x7E;
}

// Passenger animations following the crash sequence i.e. when car is stationary
//
// 3 = hands battering,
// 2 = man scratches head, girl taps car
// 1 = man scratch head & girl points
// 0 = man subdued & girl points
//
// - Process passenger sprites in crash scenario
// - Called separately for man and girl
// - Selects which passenger animation to play
//
// Source: 0x1F26
void OCrash_crash_pass2(oentry* sprite)
{
    uint32_t frames = (sprite == OCrash_spr_pass1 ? Outrun_adr.sprite_crash_man2 : Outrun_adr.sprite_crash_girl2);

    // Use OCrash_coll_count2 to select one of the three animations that can be played
    // Use crash_delay to toggle between two distinct frames
    frames += ((OCrash_coll_count2 & 3) << 4) + (crash_delay & 8);
    
    sprite->addr    = RomLoader_read32(Roms_rom0p, frames);
    uint8_t props   = RomLoader_read8(Roms_rom0p, 4 + frames);
    sprite->pal_src = RomLoader_read8(Roms_rom0p, 5 + frames);
    sprite->x       = OCrash_spr_ferrari->x + (int8_t) RomLoader_read8(Roms_rom0p, 6 + frames);
    sprite->y       = OCrash_spr_ferrari->y + (int8_t) RomLoader_read8(Roms_rom0p, 7 + frames);

    // Check H-Flip
    if (props & BIT_7)
        sprite->control |= SPRITES_HFLIP;
    else
        sprite->control &= ~SPRITES_HFLIP;

    // Test whether we should set priority higher (unused on passenger sprites I think)
    if (props & BIT_0)
        sprite->priority = sprite->road_priority = 0x1FF;
    else
        sprite->priority = sprite->road_priority = 0x1FE;

    sprite->zoom = 0x7E;

    // Man
    if (sprite == OCrash_spr_pass1)
    {
        const int8_t XY_OFF[] =
        {
            -0xC, -0x1E, 
            0x2,  -0x1B, 
            0x4,  -0x1A, 
            0x5,  -0x1E,
            0x11, -0x1B,
            0x0,  -0x1A,
            -0x1, -0x1B,
            -0xC, -0x1C,
            -0xE, -0x1B,
            -0xE, -0x1C,
            -0xE, -0x1D,
            -0xC, -0x1B,
            -0xC, -0x1C,
            -0xC, -0x1D,
        };

        sprite->x += XY_OFF[spin_pass_frame << 1];
        sprite->y += XY_OFF[(spin_pass_frame << 1) + 1];
    }
    // Woman
    else
    {
        const int8_t XY_OFF[] =
        {
            0xA,   -0x1A,
            0x0,   -0x1B,
            -0xF,  -0x1B,
            -0x15, -0x1B,
            -0x2,  -0x1E,
            0x7,   -0x1A,
            0x13,  -0x1D,
            0x9,   -0x1B,
            0x3,   -0x1B,
            0x3,   -0x1C,
            0x3,   -0x1D,
            0x7,   -0x1B,
            0x7,   -0x1C,
            0x7,   -0x1D,
        };

        sprite->x += XY_OFF[spin_pass_frame << 1];
        sprite->y += XY_OFF[(spin_pass_frame << 1) + 1];
    }
}

// Handle passenger animation sequence during car flip
//
// 3 main stages:
// 1/ Flip passengers out of car
// 2/ Passengers sit up on road after crash
// 3/ Passengers turn head and look at car (only if camera pan)
//
// Source: 0x1FDE

void OCrash_crash_pass_flip(oentry* sprite)
{
    // Some of these variable names really need refactoring
    sprite->reload        = 0;                      // clear passenger flip control
    sprite->xw1           = 0;
    sprite->x             = OCrash_spr_ferrari->x;
    sprite->traffic_speed = OCrash_crash_spin_count;
    sprite->counter       = 0x1FE;                  // sprite zoom

    // Set address of animation sequence based on whether male/female
    sprite->z = sprite == OCrash_spr_pass1 ? Outrun_adr.sprite_crash_flip_m1 : Outrun_adr.sprite_crash_flip_g1;

    OCrash_flip_start(sprite);
}

// Source: 0x201A
void OCrash_flip_start(oentry* sprite)
{
    if (Outrun_game_state != GS_ATTRACT && Outrun_game_state != GS_INGAME)
    {
        OSprites_do_spr_order_shadows(sprite);
        OCrash_done(sprite);
        return;
    }

    switch (sprite->reload & 3) // check passenger flip control
    {
        // Flip passengers out of car
        case 0:
            OCrash_pass_flip(sprite);
            break;

        // Passengers sit up on road after crash
        case 1:
            OCrash_pass_situp(sprite);
            break;

        // Passengers turn head and look at car (only if camera pan)
        case 2:
        case 3:
            OCrash_pass_turnhead(sprite);
            break;
    }
}

// Flip passengers out of car
// Source: 0x2066
void OCrash_pass_flip(oentry* sprite)
{
    // Fast crash
    if (crash_speed == 0)
    {
        sprite->counter += (OCrash_crash_zinc << 2);

        if (sprite->counter > 0x3FF)
        {
            sprite->reload = 1; // // Passenger Control: Passengers sit up on road after crash

            // Disable sprite and shadow
            sprite->control &= ~SPRITES_ENABLE;
            if (sprite == OCrash_spr_pass1)
                OCrash_spr_pass1s->control &= ~SPRITES_ENABLE;
            else
                OCrash_spr_pass2s->control &= ~SPRITES_ENABLE;
            return;
        }
    }
    // Slow crash
    else
    {
        int16_t zinc = OCrash_crash_zinc >> 2;
    
        // Adjust the z position of the female more than the man
        if (sprite == OCrash_spr_pass2)
        {
            zinc += (zinc >> 1);
        }

        sprite->counter -= zinc;
    }

    // set_z_lookup
    int16_t zoom = sprite->counter >> 2;
    if (zoom < 0x40) zoom = 0x40;
    sprite->zoom = (uint8_t) zoom;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr = RomLoader_read32(Roms_rom0p, frames);

    uint16_t offset = sprite->counter > 0x1FF ? 0x1FF : sprite->counter;
    int16_t y_change = (((int8_t) RomLoader_read8(Roms_rom0p, 6 + frames)) * offset) >> 9; // d1

    sprite->y = -(ORoad_road_y[ORoad_road_p0 + offset] >> 4) + 223;
    sprite->y -= y_change;

    // 2138

    sprite->priority = offset;
    if (crash_side) 
        sprite->control |= SPRITES_HFLIP;
    else 
        sprite->control &= ~SPRITES_HFLIP;

    sprite->pal_src = RomLoader_read8(Roms_rom0p, 4 + frames);
    
    // Decrement spin count
    // Increment frame of passengers for first spins
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = OCrash_crash_spin_count;
        sprite->xw1++; // Increase passenger frame

        // End of animation sequence. Progress to next sequnce of animations.
        if (RomLoader_read8(Roms_rom0p, 7 + frames) & BIT_7)
        {
            sprite->reload = 1; // Passenger Control: Passengers sit up on road after crash
            sprite->xw1    = 0; // Reset passenger frame
            
            // Update address of animation sequence to be used
            sprite->z      = sprite == OCrash_spr_pass1 ? Outrun_adr.sprite_crash_flip_m2 : Outrun_adr.sprite_crash_flip_g2;
            frames         = sprite->z;

            // Set Frame Delay for this animation sequence from lower bytes
            sprite->traffic_speed = RomLoader_read8(Roms_rom0p, 7 + frames) & 0x7F;

            OCrash_done(sprite);
            return;
        }
    }

    // set_passenger_x
    sprite->x =  OCrash_spr_ferrari->x;
    sprite->x += ((int8_t) RomLoader_read8(Roms_rom0p, 5 + frames));
    OCrash_done(sprite);
}

// Passengers sit up on road after crash
// Source: 0x205A
void OCrash_pass_situp(oentry* sprite)
{
    // Update passenger x position
    int16_t x_diff = (OFerrari_car_x_diff * sprite->counter) >> 9;
    sprite->x += x_diff;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr    = RomLoader_read32(Roms_rom0p, frames);
    sprite->pal_src = RomLoader_read8(Roms_rom0p, 4 + frames);

    // Decrement frame delay counter
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = RomLoader_read8(Roms_rom0p, 0xF + frames) & 0x7F;

        // End of animation sequence. Progress to next sequnce of animations.
        if (RomLoader_read8(Roms_rom0p, 7 + frames) & BIT_7)
        {
            sprite->reload = 2; // Passenger Control: Passengers turn head and look at car
        }
        else
        {
            sprite->xw1++; // Increase passenger frame

            // If camera pan: Make passengers turn heads!
            if (OCrash_crash_state == 6)
                    sprite->reload = 2;
        }
    }
    OCrash_done(sprite);
}

// Passengers turn head and look at car (only if camera pan)
// Source: 0x222C
void OCrash_pass_turnhead(oentry* sprite)
{
    // Update passenger x position
    int16_t x_diff = (OFerrari_car_x_diff * sprite->counter) >> 9;
    sprite->x += x_diff;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr    = RomLoader_read32(Roms_rom0p, frames);
    sprite->pal_src = RomLoader_read8(Roms_rom0p, 4 + frames);

    // End of animation sequence.
    if (RomLoader_read8(Roms_rom0p, 7 + frames) & BIT_7)
    {
        OCrash_done(sprite);
        return;
    }

    // Decrement frame delay counter
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = RomLoader_read8(Roms_rom0p, 0xF + frames) & 0x7F;
        sprite->xw1++; // Increase passenger frame
    }

    OCrash_done(sprite);
}
