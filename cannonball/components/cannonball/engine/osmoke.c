/***************************************************************************
    Smoke & Spray Control.
    
    Animate the smoke and spray below the Ferrari's wheels.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/ocrash.h"
#include "engine/oferrari.h"
#include "engine/olevelobjs.h"
#include "engine/osmoke.h"


int8_t OSmoke_load_smoke_data;

// Ferrari wheel smoke type on road
uint16_t smoke_type_onroad;

// Ferrari wheel smoke type off road
uint16_t smoke_type_offroad;

// Ferrari wheel smoke type after car collision
uint16_t smoke_type_slip;

void OSmoke_tick_smoke_anim(oentry*, int8_t, uint32_t);


void OSmoke_init()
{
    OSmoke_load_smoke_data = 0;
}

// Called twice, once for each plume of smoke from the car
//
// -------------------------------------
// Animation Data Format For Smoke/Spray
// -------------------------------------
//
// Format:
//
// [+0] Long: Sprite Data Address
// [+4] Byte: Sprite Z value of smoke (bigger value means in front of car, and zoomed further)
// [+5] Byte: Sprite Palette
// [+6] Byte: Sprite X (Bottom 4 bits) 
//            Sprite Y (Top 4 bits)
// [+7] Byte: Bit 0: H-Flip sprite
//            Bit 1: Zoom shift value
//            Bits 4-7: Priority Change Per Frame


// Source: 0xA816
void OSmoke_draw_ferrari_smoke(oentry *sprite)
{
    OSmoke_setup_smoke_sprite(0);

    if (Outrun_game_state != GS_ATTRACT)
    {
        if (Outrun_game_state < GS_START1 || Outrun_game_state >= GS_INIT_GAMEOVER) return; 
    }

    if (OCrash_crash_counter && !OCrash_crash_z) return;

    // ------------------------------------------------------------------------
    // Spray from water. More violent than the offroad wheel stuff
    // ------------------------------------------------------------------------

    if (OLevelObjs_spray_counter)
    {
        OSmoke_tick_smoke_anim(sprite, 1, RomLoader_read32(Roms_rom0p, Outrun_adr.spray_data + OLevelObjs_spray_type));
        return;
    }

    // Enhancement: When not displaying car, don't draw smoke effects
    if (ORoad_get_view_mode() == ROAD_VIEW_INCAR && !OCrash_is_flip())
        return;
    
    // ------------------------------------------------------------------------
    // Car Slipping/Skidding
    // ------------------------------------------------------------------------

    if (OFerrari_is_slipping && OFerrari_wheel_state == WHEELS_ON)
    {
        OSmoke_tick_smoke_anim(sprite, 0, RomLoader_read32(Roms_rom0p, Outrun_adr.smoke_data + smoke_type_slip));
        return;
    }

    // ------------------------------------------------------------------------
    // Wheels Offroad
    // ------------------------------------------------------------------------

    if (OFerrari_wheel_state != WHEELS_ON)
    {
        uint32_t smoke_adr = RomLoader_read32(Roms_rom0p, Outrun_adr.smoke_data + smoke_type_offroad);

        // Left Wheel Only
        if (sprite == &OSprites_jump_table[SPRITE_SMOKE2] && OFerrari_wheel_state == WHEELS_LEFT_OFF)
            OSmoke_tick_smoke_anim(sprite, 1, smoke_adr);

        // Right Wheel Only
        else if (sprite == &OSprites_jump_table[SPRITE_SMOKE1] && OFerrari_wheel_state == WHEELS_RIGHT_OFF)
            OSmoke_tick_smoke_anim(sprite, 1, smoke_adr);
        
        // Both Wheels
        else if (OFerrari_wheel_state == WHEELS_OFF)
            OSmoke_tick_smoke_anim(sprite, 1, smoke_adr);

        return;
    }

    // test_crash_intro:
    
    // Normal
    if (OFerrari_car_state == CAR_NORMAL)
    {
        sprite->type = sprite->xw1; // Copy frame number to type
    }
    // Smoke from wheels
    else if (OFerrari_car_state == CAR_SMOKE)
    {
        OSmoke_tick_smoke_anim(sprite, 1, RomLoader_read32(Roms_rom0p, Outrun_adr.smoke_data + smoke_type_onroad));
    }
    // Animation Sequence
    else
    {
        if (OFerrari_wheel_state != WHEELS_ON)
            sprite->type = sprite->xw1; // Copy frame number to type
        else
        {
            OSmoke_tick_smoke_anim(sprite, 1, RomLoader_read32(Roms_rom0p, Outrun_adr.smoke_data + smoke_type_onroad));
        }
    }
}

// - Set wheel spray sprite data dependent on upcoming stage
// - Use Main entry point when we know for a fact road isn't splitting
// - Use SetSmokeSprite1 entry point when road could potentially be splitting
//   Source: 0xA94C
void OSmoke_setup_smoke_sprite(uint8_t force_load)
{
    uint16_t stage_lookup = Outrun_cannonball_mode != OUTRUN_MODE_ORIGINAL ? ORoad_stage_lookup_off : 0;

    // Check whether we should load new sprite data when transitioning between stages
    if (!force_load)
    {
        uint8_t set = OSmoke_load_smoke_data & BIT_0;
        OSmoke_load_smoke_data &= ~BIT_0;
        if (!set) return; // Don't load new smoke data
        stage_lookup = ORoad_stage_lookup_off + 8;
    }

    // Set Smoke Colour When On Road
    const static uint8_t ONROAD_SMOKE[] = 
    { 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 2
        0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 3
        0x00, 0x00, 0x0A, 0x07, 0x00, 0x00, 0x00, 0x00, // Stage 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Stage 5
    };

    smoke_type_onroad = ONROAD_SMOKE[stage_lookup] << 2;
    smoke_type_slip = smoke_type_onroad;

    // Set Smoke Colour When Off Road
    const static uint8_t OFFROAD_SMOKE[] = 
    { 
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 1
        0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 2
        0x02, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 3
        0x08, 0x05, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, // Stage 4
        0x08, 0x02, 0x08, 0x06, 0x09, 0x00, 0x00, 0x00  // Stage 5
    };

    smoke_type_offroad = OFFROAD_SMOKE[stage_lookup] << 2;
}

// Set the smoke x,y and z co-ordinates
// Also sets the speed at which the animation repeats
//
// Inputs:
//
// d0 = 0: Use car speed to determine animation speed
//    = 1: Use revs to determine animation speed
//
// a5 = Smoke Sprite Plume
// Source: 0xA9B6
void OSmoke_tick_smoke_anim(oentry* sprite, int8_t anim_ctrl, uint32_t addr)
{
    sprite->x = OFerrari_spr_ferrari->x;
    sprite->y = OFerrari_spr_ferrari->y;

    // ------------------------------------------------------------------------
    // Use revs to set sprite counter reload value
    // ------------------------------------------------------------------------
    if (anim_ctrl == 1)
    {
        int16_t revs = 0;

        // Force smoke during animation sequence
        if (OFerrari_car_state == CAR_ANIM_SEQ)
            revs = 0x80;
        else
        {
            revs = OFerrari_revs >> 16;
            if (revs > 0xFF) revs = 0xFF;
        }

        sprite->reload = 3 - (revs >> 6);
        sprite->z = (revs >> 1);           // More revs = smoke is emitted further

        // Crash Occuring
        if (OCrash_crash_counter)
        {
            sprite->y = -(ORoad_road_y[ORoad_road_p0 + OCrash_crash_z] >> 4) + 223;

            // Trigger Smoke Cloud.
            // Car is slid to the side, so we need to offset the smoke accordingly
            if (OCrash_crash_state == 4)
            {
                if (OCrash_spr_ferrari->control & SPRITES_HFLIP)
                {
                    if (sprite == &OSprites_jump_table[SPRITE_SMOKE2])
                    {
                        sprite->y -= 10;
                    }
                    else
                    {
                        sprite->x -= 64;
                        sprite->y -= 4;
                    }
                }
                else
                {
                    if (sprite == &OSprites_jump_table[SPRITE_SMOKE2])
                    {
                        sprite->x += 64;
                        sprite->y -= 4;
                    }
                    else
                    {
                        sprite->y -= 10;    
                    }
                }
            } // End trigger smoke cloud

            int16_t z_shift = OCrash_crash_spin_count - 1;
            if (z_shift == 0) z_shift = 1;
            sprite->z = 0xFF >> z_shift;
        } 
    }
    // ------------------------------------------------------------------------
    // Use car speed to set sprite counter reload value
    // ------------------------------------------------------------------------
    else
    {
        uint16_t car_inc = (OInitEngine_car_increment >> 16);
        if (car_inc > 0xFF) car_inc = 0xFF;
        sprite->reload = 7 - (car_inc >> 5);
        sprite->z = 0;
    }

    // Return if stationary and not in animation sequence
    if (OFerrari_car_state != CAR_ANIM_SEQ && OInitEngine_car_increment >> 16 == 0)
        return; 

    if (Outrun_tick_frame)
    {
        if (sprite->counter > 0)
        {
            sprite->counter--;
        }
        else
        {
            // One Wheel Off Road
            if (OFerrari_wheel_state != WHEELS_ON && OFerrari_wheel_state != WHEELS_OFF)
            {
                sprite->counter = sprite->reload;
                sprite->xw1++; // Increment Frame
            }
            // Two Wheels On Road
            else if (sprite == &OSprites_jump_table[SPRITE_SMOKE1])
            {
                sprite->counter = sprite->reload;
                sprite->xw1++; // Increment Frame

                // Copy to second smoke sprite (yes this is crap, but it's directly ported code)
                OSprites_jump_table[SPRITE_SMOKE2].counter = sprite->reload;
                OSprites_jump_table[SPRITE_SMOKE2].xw1 = sprite->xw1;
            }
        }
    }
    // setup_smoke:
    uint16_t frame   = (sprite->xw1 & 7) << 3;
    sprite->addr     = RomLoader_read32(Roms_rom0p, addr + frame);
    sprite->pal_src  = RomLoader_read8(Roms_rom0p, addr + frame + 5);
    uint16_t smoke_z = RomLoader_read8(Roms_rom0p, addr + frame + 4) + sprite->z;
    if (smoke_z > 0xFF) smoke_z = 0xFF;

    // inc_crash_z:
    // When setting up smoke, include crash_z value into zoom if necessary
    if (OCrash_crash_counter && OCrash_crash_z)
    {
        smoke_z = (smoke_z * OCrash_crash_z) >> 9;
    }

    // Set Sprite Zoom
    if (smoke_z <= 0x40) smoke_z = 0x40;
    uint8_t shift = (RomLoader_read8(Roms_rom0p, addr + frame + 7) & 2) >> 1;
    uint8_t zoom = smoke_z >> shift;
    if (zoom <= 0x40) zoom = 0x40;
    sprite->zoom = zoom;

    // Set Sprite Y
    sprite->y += ((RomLoader_read8(Roms_rom0p, addr + frame + 6) & 0xF) * zoom) >> 8;

    // Set Sprite Priority
    sprite->priority = OFerrari_spr_ferrari->priority + ((RomLoader_read8(Roms_rom0p, addr + frame + 7) >> 4) & 0xF);
    sprite->road_priority = sprite->priority;

    // Set Sprite X
    uint8_t hflip = (RomLoader_read8(Roms_rom0p, addr + frame + 7) & 1);
    int8_t x = ((RomLoader_read8(Roms_rom0p, addr + frame + 6) >> 3) & 0x1E);

    // Enhancement: When viewing in-car, spread the spray out
    if (ORoad_get_view_mode() == ROAD_VIEW_INCAR)
        x += 10;

    if (sprite == &OSprites_jump_table[SPRITE_SMOKE1])
    {
        sprite->draw_props = DRAW_BOTTOM | DRAW_LEFT; // Anchor bottom left
        hflip++;
    }
    else
    {
        sprite->draw_props = DRAW_BOTTOM | DRAW_RIGHT; // Anchor bottom right
        x = -x;        
    }
    sprite->x += (x * zoom) >> 8;

    // Set H-Flip
    if (hflip & 1) sprite->control |= SPRITES_HFLIP;
    else sprite->control &= ~SPRITES_HFLIP;

    OSprites_map_palette(sprite);
    OSprites_do_spr_order_shadows(sprite);
}

// Draw only helper routine.
// Useful for framerate changes.
void OSmoke_draw(oentry* sprite)
{
    if (Outrun_game_state != GS_ATTRACT)
    {
        if (Outrun_game_state < GS_START1 || Outrun_game_state >= GS_INIT_GAMEOVER) return; 
    }

    if (OCrash_crash_counter && !OCrash_crash_z) return;

    // Return if stationary and not in animation sequence
    if (OFerrari_car_state != CAR_ANIM_SEQ && OInitEngine_car_increment >> 16 == 0)
        return; 
    
    OSprites_map_palette(sprite);
    OSprites_do_spr_order_shadows(sprite);
}
