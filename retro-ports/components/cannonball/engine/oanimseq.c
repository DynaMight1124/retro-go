/***************************************************************************
    Animation Sequences.
    
    Used in three areas of the game:
    - The sequence at the start with the Ferrari driving in from the side
    - Flag Waving Man
    - 5 x End Sequences
    
    See "oanimsprite.h" for the specific format used by animated sprites.
    It is essentially a deviation from the normal sprites in the game.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/obonus.h"
#include "engine/oferrari.h"
#include "engine/oinputs.h"
#include "engine/oanimseq.h"

// ----------------------------------------------------------------------------
// Animation Data Format.
// Animation blocks are stored in groups of 8 bytes, and formatted as follows:
//
// +00 [Byte] Sprite Colour Palette
// +01 [Byte] Bit 7: Make X Position Negative
//            Bits 4-6: Sprite To Sprite Priority
//            Bits 0-3: Top Bits Of Sprite Data Address
// +02 [Word] Sprite Data Address
// +04 [Byte] Sprite X Position
// +05 [Byte] Sprite Y Position
// +06 [Byte] Sprite To Road Priority
// +07 [Byte] Bit 7: Set To Load Next Block Of Sprite Animation Data To 0x1E
//            Bit 6: Set For H-Flip
//            Bit 4:
//            Bits 0-3: Animation Frame Delay 
//                     (Before Incrementing To Next Block Of 8 Bytes)
// ----------------------------------------------------------------------------


// Man at line with start flag
oanimsprite OAnimSeq_anim_flag;

// Ferrari Animation Sequence
oanimsprite OAnimSeq_anim_ferrari;               // 1

// Passenger Animation Sequences
oanimsprite OAnimSeq_anim_pass1;                 // 2
oanimsprite OAnimSeq_anim_pass2;                 // 3

// End Sequence Stuff
oanimsprite OAnimSeq_anim_obj1;                  // 4
oanimsprite OAnimSeq_anim_obj2;                  // 5
oanimsprite OAnimSeq_anim_obj3;                  // 6
oanimsprite OAnimSeq_anim_obj4;                  // 7
oanimsprite OAnimSeq_anim_obj5;                  // 8
oanimsprite OAnimSeq_anim_obj6;                  // 9
oanimsprite OAnimSeq_anim_obj7;                  // 10
oanimsprite OAnimSeq_anim_obj8;                  // 10

// End sequence to display (0-4)
uint8_t OAnimSeq_end_seq;

// End Sequence Animation Position
int16_t seq_pos;

// End Sequence State (0 = Init, 1 = Tick)
uint8_t end_seq_state;

// Used for Ferrari End Animation Sequence
uint8_t ferrari_stopped;

void OAnimSeq_init_end_sprites();
void OAnimSeq_tick_ferrari();
void OAnimSeq_anim_seq_outro(oanimsprite*);
void OAnimSeq_anim_seq_shadow(oanimsprite*, oanimsprite*);
void OAnimSeq_anim_seq_outro_ferrari();
uint8_t OAnimSeq_read_anim_data(oanimsprite*);


void OAnimSeq_init_oanimsprite(oanimsprite* animsprite, oentry* s)
{
    animsprite->sprite = s;
    animsprite->sprite->function_holder = -1;
    animsprite->anim_addr_curr = 0;
    animsprite->anim_addr_next = 0;
    animsprite->anim_frame = 0;
    animsprite->frame_delay = 0;
    animsprite->anim_props = 0;
    animsprite->anim_state = 0;
}

void OAnimSeq_init(oentry* jump_table)
{
    // --------------------------------------------------------------------------------------------
    // Flag Animation Setup
    // --------------------------------------------------------------------------------------------
    oentry* sprite_flag = &jump_table[SPRITE_FLAG];
    OAnimSeq_anim_flag.sprite = sprite_flag;
    OAnimSeq_anim_flag.anim_state = 0;

    // Jump table initalisations
    sprite_flag->shadow = 7;
    sprite_flag->draw_props = DRAW_BOTTOM;

    // Routine initalisations
    sprite_flag->control |= SPRITES_ENABLE;
    sprite_flag->z = 400 << 16;

    // --------------------------------------------------------------------------------------------
    // Ferrari & Passenger Animation Setup For Intro
    // --------------------------------------------------------------------------------------------
    oentry* sprite_ferrari = &jump_table[SPRITE_FERRARI];
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_ferrari, sprite_ferrari);
    OAnimSeq_anim_ferrari.anim_addr_curr = Outrun_adr.anim_ferrari_curr;
    OAnimSeq_anim_ferrari.anim_addr_next = Outrun_adr.anim_ferrari_next;
    sprite_ferrari->control |= SPRITES_ENABLE;
    sprite_ferrari->draw_props = DRAW_BOTTOM;

    oentry* sprite_pass1 = &jump_table[SPRITE_PASS1];
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_pass1, sprite_pass1);
    OAnimSeq_anim_pass1.anim_addr_curr = Outrun_adr.anim_pass1_curr;
    OAnimSeq_anim_pass1.anim_addr_next = Outrun_adr.anim_pass1_next;
    sprite_pass1->draw_props = DRAW_BOTTOM;

    oentry* sprite_pass2 = &jump_table[SPRITE_PASS2];
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_pass2, sprite_pass2);
    OAnimSeq_anim_pass2.anim_addr_curr = Outrun_adr.anim_pass2_curr;
    OAnimSeq_anim_pass2.anim_addr_next = Outrun_adr.anim_pass2_next;
    sprite_pass2->draw_props = DRAW_BOTTOM;

    // --------------------------------------------------------------------------------------------
    // End Sequence Animation
    // --------------------------------------------------------------------------------------------
    end_seq_state = 0; // init
    seq_pos = 0;
    ferrari_stopped = 0;

    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj1, &jump_table[SPRITE_CRASH]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj2, &jump_table[SPRITE_CRASH_SPRITES_SHADOW]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj3, &jump_table[SPRITE_SPRITES_SHADOW]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj4, &jump_table[SPRITE_CRASH_PASS1]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj5, &jump_table[SPRITE_CRASH_PASS1_S]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj6, &jump_table[SPRITE_CRASH_PASS2]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj7, &jump_table[SPRITE_CRASH_PASS2_S]);
    OAnimSeq_init_oanimsprite(&OAnimSeq_anim_obj8, &jump_table[SPRITE_FLAG]);
}

// ------------------------------------------------------------------------------------------------
// START ANIMATION SEQUENCES (FLAG, FERRARI DRIVING IN)
// ------------------------------------------------------------------------------------------------

void OAnimSeq_flag_seq()
{
    if (!(OAnimSeq_anim_flag.sprite->control & SPRITES_ENABLE))
        return;

    if (Outrun_tick_frame)
    {
        if (Outrun_game_state < GS_START1 || Outrun_game_state > GS_GAMEOVER)
        {
            OAnimSeq_anim_flag.sprite->control &= ~SPRITES_ENABLE;
            return;
        }

        // Init Flag Sequence
        if (Outrun_game_state < GS_INGAME && OAnimSeq_anim_flag.anim_state != Outrun_game_state)
        {
            OAnimSeq_anim_flag.anim_state = Outrun_game_state;

            // Index of animation sequences
            uint32_t index = Outrun_adr.anim_seq_flag + ((Outrun_game_state - 9) << 3);

            OAnimSeq_anim_flag.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &index);
            OAnimSeq_anim_flag.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &index);

            OAnimSeq_anim_flag.frame_delay = RomLoader_read8(Roms_rom0p, 7 + OAnimSeq_anim_flag.anim_addr_curr) & 0x3F;
            OAnimSeq_anim_flag.anim_frame  = 0;
        }

        // Wave Flag 
        if (Outrun_game_state <= GS_INGAME)
        {
            uint32_t index = OAnimSeq_anim_flag.anim_addr_curr + (OAnimSeq_anim_flag.anim_frame << 3);

            OAnimSeq_anim_flag.sprite->addr    = RomLoader_read32(Roms_rom0p, index) & 0xFFFFF;
            OAnimSeq_anim_flag.sprite->pal_src = RomLoader_read8(Roms_rom0p, index);

            uint32_t addr = SPRITE_ZOOM_LOOKUP + (((OAnimSeq_anim_flag.sprite->z >> 16) << 2) | OSprites_sprite_scroll_speed);
            uint32_t value = RomLoader_read32(Roms_rom0p, addr);
            OAnimSeq_anim_flag.sprite->z += value;
            uint16_t z16 = OAnimSeq_anim_flag.sprite->z >> 16;
        
            if (z16 >= 0x200)
            {
                OAnimSeq_anim_flag.sprite->control &= ~SPRITES_ENABLE;
                return;
            }
            OAnimSeq_anim_flag.sprite->priority = z16;
            OAnimSeq_anim_flag.sprite->zoom     = z16 >> 2;

            // Set X Position
            int16_t sprite_x = (int8_t) RomLoader_read8(Roms_rom0p, 4 + index);
            sprite_x -= ORoad_road0_h[z16];
            int32_t final_x = (sprite_x * z16) >> 9;

            if (RomLoader_read8(Roms_rom0p, 1 + index) & BIT_7)
                final_x = -final_x;

            OAnimSeq_anim_flag.sprite->x = final_x;

            // Set Y Position
            int16_t sprite_y      = (int8_t) RomLoader_read8(Roms_rom0p, 5 + index);
            int16_t final_y       = (sprite_y * z16) >> 9;
            OAnimSeq_anim_flag.sprite->y   = ORoad_get_road_y(z16) - final_y;

            // Set H-Flip
            if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_6)
                OAnimSeq_anim_flag.sprite->control |= SPRITES_HFLIP;
            else
                OAnimSeq_anim_flag.sprite->control &= ~SPRITES_HFLIP;

            // Ready for next frame
            if (--OAnimSeq_anim_flag.frame_delay == 0)
            {
                // Load Next Block Of Animation Data
                if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_7)
                {
                    OAnimSeq_anim_flag.anim_addr_curr = OAnimSeq_anim_flag.anim_addr_next;
                    OAnimSeq_anim_flag.frame_delay    = RomLoader_read8(Roms_rom0p, 7 + OAnimSeq_anim_flag.anim_addr_curr) & 0x3F;
                    OAnimSeq_anim_flag.anim_frame     = 0;
                }
                // Last Block
                else
                {
                    OAnimSeq_anim_flag.frame_delay = RomLoader_read8(Roms_rom0p, 0x0F + index) & 0x3F;
                    OAnimSeq_anim_flag.anim_frame++;
                }
            }
        }
    }

    // Order sprites
    OSprites_map_palette(OAnimSeq_anim_flag.sprite);
    OSprites_do_spr_order_shadows(OAnimSeq_anim_flag.sprite);
}

// Setup Ferrari Animation Sequence
// 
// Source: 0x6036
void OAnimSeq_ferrari_seq()
{
    if (!(OAnimSeq_anim_ferrari.sprite->control & SPRITES_ENABLE))
        return;

    if (Outrun_game_state == GS_MUSIC) return;

    OAnimSeq_anim_pass1.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_pass2.sprite->control |= SPRITES_ENABLE;

    if (Outrun_game_state <= GS_LOGO)
    {
        OFerrari_init_ingame();
        return;
    }

    OAnimSeq_anim_ferrari.frame_delay = RomLoader_read8(Roms_rom0p, 7 + OAnimSeq_anim_ferrari.anim_addr_curr) & 0x3F;
    OAnimSeq_anim_pass1.frame_delay   = RomLoader_read8(Roms_rom0p, 7 + OAnimSeq_anim_pass1.anim_addr_curr) & 0x3F;
    OAnimSeq_anim_pass2.frame_delay   = RomLoader_read8(Roms_rom0p, 7 + OAnimSeq_anim_pass2.anim_addr_curr) & 0x3F;

    OFerrari_car_state = CAR_NORMAL;
    OFerrari_state     = FERRARI_SEQ2;

    OAnimSeq_anim_seq_intro(&OAnimSeq_anim_ferrari);
}

// Process Animations for Ferrari and Passengers on intro
//
// Source: 60AE
void OAnimSeq_anim_seq_intro(oanimsprite* anim)
{
    if (Outrun_game_state <= GS_LOGO)
    {
        OFerrari_init_ingame();
        return;
    }

    if (Outrun_tick_frame)
    {
        if (anim->anim_frame >= 1)
            OFerrari_car_state = CAR_ANIM_SEQ;

        uint32_t index              = anim->anim_addr_curr + (anim->anim_frame << 3);

        anim->sprite->addr          = RomLoader_read32(Roms_rom0p, index) & 0xFFFFF;
        anim->sprite->pal_src       = RomLoader_read8(Roms_rom0p, index);
        anim->sprite->zoom          = 0x7F;
        anim->sprite->road_priority = 0x1FE;
        anim->sprite->priority      = 0x1FE - ((RomLoader_read16(Roms_rom0p, index) & 0x70) >> 4);

        // Set X
        int16_t sprite_x = (int8_t) RomLoader_read8(Roms_rom0p, 4 + index);
        int32_t final_x = (sprite_x * anim->sprite->priority) >> 9;
        if (RomLoader_read8(Roms_rom0p, 1 + index) & BIT_7)
            final_x = -final_x;
        anim->sprite->x = final_x;

        // Set Y
        anim->sprite->y = 221 - ((int8_t) RomLoader_read8(Roms_rom0p, 5 + index));

        // Set H-Flip
        if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_6)
            anim->sprite->control |= SPRITES_HFLIP;
        else
            anim->sprite->control &= ~SPRITES_HFLIP;

        // Ready for next frame
        if (--anim->frame_delay == 0)
        {
            // Load Next Block Of Animation Data
            if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_7)
            {
                // Yeah the usual OutRun code hacks to do really odd stuff!
                // In this case, to exit the routine and setup the Ferrari on the last entry for passenger 2
                if (anim == &OAnimSeq_anim_pass2)
                {
                    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR)
                    {
                        OSprites_map_palette(anim->sprite);
                        OSprites_do_spr_order_shadows(anim->sprite);
                    }
                    OFerrari_init_ingame();
                    return;
                }

                anim->anim_addr_curr = anim->anim_addr_next;
                anim->frame_delay    = RomLoader_read8(Roms_rom0p, 7 + anim->anim_addr_curr) & 0x3F;
                anim->anim_frame     = 0;
            }
            // Last Block
            else
            {
                anim->frame_delay = RomLoader_read8(Roms_rom0p, 0x0F + index) & 0x3F;
                anim->anim_frame++;
            }
        }
    }

    // Order sprites
    if (ORoad_get_view_mode() != ROAD_VIEW_INCAR)
    {
        OSprites_map_palette(anim->sprite);
        OSprites_do_spr_order_shadows(anim->sprite);
    }
}

// ------------------------------------------------------------------------------------------------
// END ANIMATION SEQUENCES
// ------------------------------------------------------------------------------------------------

// Initialize end sequence animations on game complete
// Source: 0x9978
void OAnimSeq_init_end_seq()
{
    // Process animation sprites instead of normal routine
    OFerrari_state = FERRARI_END_SEQ;

    // Setup Ferrari Sprite
    OAnimSeq_anim_ferrari.sprite->control |= SPRITES_ENABLE; 
    OAnimSeq_anim_ferrari.sprite->id = 0;
    OAnimSeq_anim_ferrari.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_ferrari.anim_frame = 0;
    OAnimSeq_anim_ferrari.frame_delay = 0;

    seq_pos = 0;

    // Disable Passenger Sprites. These are replaced with new versions by the animation sequence.
    OFerrari_spr_pass1->control &= ~SPRITES_ENABLE;
    OFerrari_spr_pass2->control &= ~SPRITES_ENABLE;

    OBonus_bonus_control += 4;
}

void OAnimSeq_tick_end_seq()
{
    switch (end_seq_state)
    {
        case 0: // init
            if (Outrun_tick_frame) OAnimSeq_init_end_sprites();
            else return;
            
        case 1: // tick & blit
            OAnimSeq_anim_seq_outro_ferrari();                   // Ferrari Sprite
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_obj1);                 // Car Door Opening Animation
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_obj2);                 // Interior of Ferrari
            OAnimSeq_anim_seq_shadow(&OAnimSeq_anim_ferrari, &OAnimSeq_anim_obj3); // Car Shadow
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_pass1);                // Man Sprite
            OAnimSeq_anim_seq_shadow(&OAnimSeq_anim_pass1, &OAnimSeq_anim_obj4);   // Man Shadow
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_pass2);                // Female Sprite
            OAnimSeq_anim_seq_shadow(&OAnimSeq_anim_pass2, &OAnimSeq_anim_obj5);   // Female Shadow
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_obj6);                 // Man Presenting Trophy
            if (OAnimSeq_end_seq == 4)
                OAnimSeq_anim_seq_outro(&OAnimSeq_anim_obj7);             // Varies
            else
                OAnimSeq_anim_seq_shadow(&OAnimSeq_anim_obj6, &OAnimSeq_anim_obj7);
            OAnimSeq_anim_seq_outro(&OAnimSeq_anim_obj8);                 // Effects
            break;
    }
}

// Initalize Sprites For End Sequence.
// Source: 0x588A
void OAnimSeq_init_end_sprites()
{
    // Ferrari Object [0x5B12 entry point]
    uint32_t addr = Outrun_adr.anim_endseq_obj1 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_ferrari.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_ferrari.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);
    ferrari_stopped = 0;
    
    // 0x58A4: Car Door Opening Animation [seq_sprite_entry]
    OAnimSeq_anim_obj1.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj1.sprite->id = 1;
    OAnimSeq_anim_obj1.sprite->shadow = 3;
    OAnimSeq_anim_obj1.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj1.anim_frame = 0;
    OAnimSeq_anim_obj1.frame_delay = 0;
    OAnimSeq_anim_obj1.anim_props = 0;
    addr = Outrun_adr.anim_endseq_obj2 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_obj1.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_obj1.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);
    
    // 0x58EC: Interior of Ferrari (Note this wobbles a little when passengers exit) [seq_sprite_entry]
    OAnimSeq_anim_obj2.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj2.sprite->id = 2;
    OAnimSeq_anim_obj2.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj2.anim_frame = 0;
    OAnimSeq_anim_obj2.frame_delay = 0;
    OAnimSeq_anim_obj2.anim_props = 0;
    addr = Outrun_adr.anim_endseq_obj3 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_obj2.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_obj2.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);

    // 0x592A: Car Shadow [SeqSpriteShadow]
    OAnimSeq_anim_obj3.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj3.sprite->id = 3;
    OAnimSeq_anim_obj3.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj3.anim_frame = 0;
    OAnimSeq_anim_obj3.frame_delay = 0;
    OAnimSeq_anim_obj3.anim_props = 0;
    OAnimSeq_anim_obj3.sprite->addr = Outrun_adr.shadow_data;

    // 0x5960: Man Sprite [seq_sprite_entry]
    OAnimSeq_anim_pass1.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_pass1.sprite->id = 4;
    OAnimSeq_anim_pass1.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_pass1.anim_frame = 0;
    OAnimSeq_anim_pass1.frame_delay = 0;
    OAnimSeq_anim_pass1.anim_props = 0;
    addr = Outrun_adr.anim_endseq_obj4 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_pass1.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_pass1.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);

    // 0x5998: Man Shadow [SeqSpriteShadow]
    OAnimSeq_anim_obj4.sprite->control = SPRITES_ENABLE;
    OAnimSeq_anim_obj4.sprite->id = 5;
    OAnimSeq_anim_obj4.sprite->shadow = 7;
    OAnimSeq_anim_obj4.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj4.anim_frame = 0;
    OAnimSeq_anim_obj4.frame_delay = 0;
    OAnimSeq_anim_obj4.anim_props = 0;
    OAnimSeq_anim_obj4.sprite->addr = Outrun_adr.shadow_data;

    // 0x59BE: Female Sprite [seq_sprite_entry]
    OAnimSeq_anim_pass2.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_pass2.sprite->id = 6;
    OAnimSeq_anim_pass2.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_pass2.anim_frame = 0;
    OAnimSeq_anim_pass2.frame_delay = 0;
    OAnimSeq_anim_pass2.anim_props = 0;
    addr = Outrun_adr.anim_endseq_obj5 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_pass2.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_pass2.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);

    // 0x59F6: Female Shadow [SeqSpriteShadow]
    OAnimSeq_anim_obj5.sprite->control = SPRITES_ENABLE;
    OAnimSeq_anim_obj5.sprite->id = 7;
    OAnimSeq_anim_obj5.sprite->shadow = 7;
    OAnimSeq_anim_obj5.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj5.anim_frame = 0;
    OAnimSeq_anim_obj5.frame_delay = 0;
    OAnimSeq_anim_obj5.anim_props = 0;
    OAnimSeq_anim_obj5.sprite->addr = Outrun_adr.shadow_data;

    // 0x5A2C: Person Presenting Trophy [seq_sprite_entry]
    OAnimSeq_anim_obj6.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj6.sprite->id = 8;
    OAnimSeq_anim_obj6.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj6.anim_frame = 0;
    OAnimSeq_anim_obj6.frame_delay = 0;
    OAnimSeq_anim_obj6.anim_props = 0;
    addr = Outrun_adr.anim_endseq_obj6 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_obj6.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_obj6.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);

    // Alternate Use Based On End Sequence
    OAnimSeq_anim_obj7.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj7.sprite->id = 9;
    OAnimSeq_anim_obj7.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj7.anim_frame = 0;
    OAnimSeq_anim_obj7.frame_delay = 0;
    OAnimSeq_anim_obj7.anim_props = 0;

    // [seq_sprite_entry]
    if (OAnimSeq_end_seq == 4)
    {
        addr = Outrun_adr.anim_endseq_objB + (OAnimSeq_end_seq << 3);
        OAnimSeq_anim_obj7.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
        OAnimSeq_anim_obj7.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);
    }
    // Trophy Shadow
    else
    {
        OAnimSeq_anim_obj7.sprite->shadow = 7;
        OAnimSeq_anim_obj7.sprite->addr = Outrun_adr.shadow_data;
    }

    // 0x5AD0: Enable After Effects (e.g. cloud of smoke for genie) [seq_sprite_entry]
    OAnimSeq_anim_obj8.sprite->control |= SPRITES_ENABLE;
    OAnimSeq_anim_obj8.sprite->id = 10;
    OAnimSeq_anim_obj8.sprite->draw_props = DRAW_BOTTOM;
    OAnimSeq_anim_obj8.anim_frame = 0;
    OAnimSeq_anim_obj8.frame_delay = 0;
    OAnimSeq_anim_obj8.anim_props = 0xFF00;
    addr = Outrun_adr.anim_endseq_obj7 + (OAnimSeq_end_seq << 3);
    OAnimSeq_anim_obj8.anim_addr_curr = RomLoader_read32IncP(Roms_rom0p, &addr);
    OAnimSeq_anim_obj8.anim_addr_next = RomLoader_read32IncP(Roms_rom0p, &addr);
    
    end_seq_state = 1;
}

// Ferrari Outro Animation Sequence
// Source: 0x5B12
void OAnimSeq_anim_seq_outro_ferrari()
{
    if (/*Outrun_tick_frame && */!ferrari_stopped)
    {
        // Car is moving. Turn Brake On.
        if (OInitEngine_car_increment >> 16)
        {
            OFerrari_auto_brake  = 1;
            OInputs_brake_adjust = 0xFF;
        }
        else
        {
            OSoundInt_queue_sound(sound_VOICE_CONGRATS);
            ferrari_stopped = 1;
        }
    }
    OAnimSeq_anim_seq_outro(&OAnimSeq_anim_ferrari);
}

// End Sequence: Setup Animated Sprites 
// Source: 0x5B42
void OAnimSeq_anim_seq_outro(oanimsprite* anim)
{
    OInputs_steering_adjust = 0;

    // Return if no animation data to process
    if (!OAnimSeq_read_anim_data(anim)) 
        return;

    if (Outrun_tick_frame)
    {    
        // Process Animation Data
        uint32_t index = anim->anim_addr_curr + (anim->anim_frame << 3);

        anim->sprite->addr          = RomLoader_read32(Roms_rom0p, index) & 0xFFFFF;
        anim->sprite->pal_src       = RomLoader_read8(Roms_rom0p, index);
        anim->sprite->zoom          = RomLoader_read8(Roms_rom0p, 6 + index) >> 1;
        anim->sprite->road_priority = RomLoader_read8(Roms_rom0p, 6 + index) << 1;
        anim->sprite->priority      = anim->sprite->road_priority - ((RomLoader_read16(Roms_rom0p, index) & 0x70) >> 4); // (bits 4-6)
        anim->sprite->x             = (RomLoader_read8(Roms_rom0p, 4 + index) * anim->sprite->priority) >> 9;
    
        if (RomLoader_read8(Roms_rom0p, 1 + index) & BIT_7)
            anim->sprite->x = -anim->sprite->x;

        // set_sprite_xy: (similar to flag code again)

        // Set Y Position
        int16_t sprite_y = (int8_t) RomLoader_read8(Roms_rom0p, 5 + index);
        int16_t final_y  = (sprite_y * anim->sprite->priority) >> 9;
        anim->sprite->y  = ORoad_get_road_y(anim->sprite->priority) - final_y;

        // Set H-Flip
        if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_6)
            anim->sprite->control |= SPRITES_HFLIP;
        else
            anim->sprite->control &= ~SPRITES_HFLIP;

        // Ready for next frame
        if (--anim->frame_delay == 0)
        {
            // Load Next Block Of Animation Data
            if (RomLoader_read8(Roms_rom0p, 7 + index) & BIT_7)
            {
                anim->anim_props    |= 0xFF;
                anim->anim_addr_curr = anim->anim_addr_next;
                anim->frame_delay    = RomLoader_read8(Roms_rom0p, 7 + anim->anim_addr_curr) & 0x3F;
                anim->anim_frame     = 0;
            }
            // Last Block
            else
            {
                anim->frame_delay = RomLoader_read8(Roms_rom0p, 0x0F + index) & 0x3F;
                anim->anim_frame++;
            }
        } 
        OSprites_map_palette(anim->sprite);
    }

    // Order sprites
    OSprites_do_spr_order_shadows(anim->sprite);
}

// Render Sprite Shadow For End Sequence
// Use parent sprite as basis to set this up
// Source: 0x5C48
void OAnimSeq_anim_seq_shadow(oanimsprite* parent, oanimsprite* anim)
{
    // Return if no animation data to process
    if (!OAnimSeq_read_anim_data(anim)) 
        return;

    if (Outrun_tick_frame)
    {  
        uint8_t zoom_shift = 3;

        // Car Shadow
        if (anim->sprite->id == 3)
        {
            zoom_shift = 1;
            if ((parent->anim_props & 0xFF) == 0 && OFerrari_sprite_ai_x <= 5)
                zoom_shift++;
        }
        // 5C88 set_sprite_xy:
        anim->sprite->x    = parent->sprite->x;
        uint16_t priority  = parent->sprite->road_priority >> zoom_shift;
        anim->sprite->zoom = priority - (priority >> 2);
        anim->sprite->y    = ORoad_get_road_y(parent->sprite->road_priority);
    
        // Chris - The following extra line seems necessary due to the way I set the sprites up.
        // Actually, I think it's a bug in the original game, relying on this being setup by 
        // the crash code previously. But anyway!
        anim->sprite->road_priority = parent->sprite->road_priority;
    }

    OSprites_do_spr_order_shadows(anim->sprite);
}

// Read Animation Data for End Sequence
// Source: 0x5CC4
uint8_t OAnimSeq_read_anim_data(oanimsprite* anim)
{
    uint32_t addr = Outrun_adr.anim_end_table + (OAnimSeq_end_seq << 2) + (anim->sprite->id << 2) +  (anim->sprite->id << 4); // a0 + d1

    // Read start & end position in animation timeline for this object
    int16_t start_pos = RomLoader_read16(Roms_rom0p, addr);     // d0
    int16_t end_pos =   RomLoader_read16(Roms_rom0p, 2 + addr); // d3

    int16_t pos = seq_pos; // d1
    
    // Global Sequence Position: Advance to next position
    // Not particularly clean embedding this here obviously!
    if (Outrun_tick_frame && anim->anim_props & 0xFF00)
        seq_pos++;

    // Test Whether Animation Sequence Is Over & Initalize Course Map
    if (OBonus_bonus_control != BONUS_DISABLE)
    {
        const uint16_t END_SEQ_LENGTHS[] = {0x244, 0x244, 0x244, 0x190, 0x258};

        if (seq_pos == END_SEQ_LENGTHS[OAnimSeq_end_seq])
        {
            OBonus_bonus_control = BONUS_DISABLE;
            // we're missing all the code here to disable the animsprites, but probably not necessary?

            if (Outrun_cannonball_mode == OUTRUN_MODE_ORIGINAL)
                Outrun_game_state = GS_INIT_MAP;
            else
                Outrun_init_best_outrunners();
        }
    }

    // --------------------------------------------------------------------------------------------
    // Process Animation Sequence
    // --------------------------------------------------------------------------------------------

    const uint8_t DO_NOTHING = 0;
    const uint8_t PROCESS    = 1;

    // check_seq_pos:
    // Sequence: Start Position
    // ret_set_frame_delay: 
    if (pos == start_pos)
    {
        // If current animation block is set, extract frame delay
        if (anim->anim_addr_curr)
            anim->frame_delay = RomLoader_read8(Roms_rom0p, 7 + anim->anim_addr_curr) & 0x3F;

        return PROCESS;
    }
    // Sequence: Invalid Position
    else if (pos < start_pos || pos > end_pos) // d1 < d0 || d1 > d3
        return DO_NOTHING;

    // Sequence: In Progress
    else if (pos < end_pos)
        return PROCESS;
    
    // Sequence: End Position
    else
    {
        // End Of Animation Data
        if (anim->sprite->id == 8) // Trophy person
        {
            anim->sprite->id = 11;
            if (OAnimSeq_end_seq >= 2)
                anim->sprite->shadow = 7;

            anim->anim_addr_curr = RomLoader_read32(Roms_rom0p, Outrun_adr.anim_endseq_obj8 + (OAnimSeq_end_seq << 3));
            anim->anim_addr_next = RomLoader_read32(Roms_rom0p, Outrun_adr.anim_endseq_obj8 + (OAnimSeq_end_seq << 3) + 4);
            anim->anim_frame = 0;
            return DO_NOTHING;
        }
        // 5e14
        else if (anim->sprite->id == 10)
        {
            anim->sprite->id = 12;
            if (OAnimSeq_end_seq >= 2)
                anim->sprite->shadow = 7;

            anim->anim_addr_curr = RomLoader_read32(Roms_rom0p, Outrun_adr.anim_endseq_objA + (OAnimSeq_end_seq << 3));
            anim->anim_addr_next = RomLoader_read32(Roms_rom0p, Outrun_adr.anim_endseq_objA + (OAnimSeq_end_seq << 3) + 4);
            anim->anim_frame = 0;
            return DO_NOTHING;
        }
    }
    return PROCESS;
}
