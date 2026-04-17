/***************************************************************************
    Sprite Handling Routines.
    
    - Initializing Sprites from level data.
    - Mapping palettes to sprites.
    - Ordering sprites by priority.
    - Adding shadows to sprites where appropriate.
    - Clipping sprites based on priority in relation to road hardware.
    - Conversion from internal format to output format required by hardware.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../trackloader.h"

#include "engine/oanimseq.h"
#include "engine/ocrash.h"
#include "engine/oferrari.h"
#include "engine/olevelobjs.h"
#include "engine/osprites.h"
#include "engine/otraffic.h"
#include "engine/ozoom_lookup.h"
#include <stdint.h>

uint8_t OSprites_no_sprites;
oentry *OSprites_jump_table = NULL;
osprite *OSprites_sprite_entries = NULL;

uint16_t OSprites_seg_pos;
uint8_t OSprites_seg_total_sprites;
uint16_t OSprites_seg_sprite_freq;
int16_t OSprites_seg_spr_offset2;
int16_t OSprites_seg_spr_offset1;
uint32_t OSprites_seg_spr_addr;
uint16_t OSprites_sprite_scroll_speed;
int16_t OSprites_shadow_offset;
uint16_t OSprites_sprite_count;
uint16_t OSprites_spr_cnt_main;
uint16_t OSprites_spr_cnt_shadow;


// Start of Sprite RAM
static const uint32_t SPRITE_RAM = 0x130000;

// Palette Ram: Sprite Entries Start Here
static const uint32_t PAL_SPRITES = 0x121000;

// Denote whether to swap sprite ram
uint8_t do_sprite_swap;

// Store the next available sprite colour palette (0 - 7F)
uint8_t spr_col_pal;

// Stores number of palette entries to copy from rom to palram
int16_t pal_copy_count; 

// Palette Addresses. Used in conjunction with palette lookup table.
// Originally stored between 0x61602 - 0x617FF in RAM
//
// Format:
// Word 1: ROM Source Offset
// Word 2: Palette RAM Destination Offset
//
// Word 3: ROM Source Offset
// Word 4: Palette RAM Destination Offset
//
//etc.
uint16_t pal_addresses[0x100]; // todo: rename to pal_mapping

// Palette Lookup Table
uint8_t pal_lookup[0x100];

// Converted sprite entries in RAM for hardware.
uint8_t *sprite_order = NULL;
uint8_t *sprite_order2 = NULL;

void OSprites_sprite_control();
void OSprites_hide_hwsprite(oentry*, osprite*);
void OSprites_finalise_sprites();

void OSprites_init()
{
    if (!sprite_order) sprite_order = malloc(0x2000);
    if (!sprite_order2) sprite_order2 = malloc(0x2000);
    if (!OSprites_jump_table) OSprites_jump_table = malloc(JUMP_ENTRIES_TOTAL * sizeof(oentry));
    if (!OSprites_sprite_entries) OSprites_sprite_entries = malloc(JUMP_ENTRIES_TOTAL * sizeof(osprite));

    uint16_t i;

    // Set activated number of sprites based on config
    OSprites_no_sprites = Config_engine.level_objects ? SPRITE_ENTRIES : 0x4F;

    // Also handled by clear_palette_data() now
    for (i = 0; i < 0x100; i++)
        pal_lookup[i] = 0;

    for (i = 0; i < 0x2000; i++)
    {
        sprite_order[i] = 0;
        sprite_order2[i] = 0;
    }

    // Reset hardware entries
    for (i = 0; i < JUMP_ENTRIES_TOTAL; i++)
        osprite_init(&OSprites_sprite_entries[i]);

    for (i = 0; i < SPRITE_ENTRIES; i++)
        oentry_init(&OSprites_jump_table[i], i);

    // Ferrari + Passenger Sprites
    oentry_init(&OSprites_jump_table[SPRITE_FERRARI], SPRITE_FERRARI);        // Ferrari
    oentry_init(&OSprites_jump_table[SPRITE_PASS1], SPRITE_PASS1);
    oentry_init(&OSprites_jump_table[SPRITE_PASS2], SPRITE_PASS2);
    oentry_init(&OSprites_jump_table[SPRITE_SPRITES_SHADOW], SPRITE_SPRITES_SHADOW);          // Shadow Settings
    OSprites_jump_table[SPRITE_SPRITES_SHADOW].shadow = 7;
    oentry_init(&OSprites_jump_table[SPRITE_SMOKE1], SPRITE_SMOKE1);
    oentry_init(&OSprites_jump_table[SPRITE_SMOKE2], SPRITE_SMOKE2);

    OFerrari_init(&OSprites_jump_table[SPRITE_FERRARI], &OSprites_jump_table[SPRITE_PASS1], &OSprites_jump_table[SPRITE_PASS2], &OSprites_jump_table[SPRITE_SPRITES_SHADOW]);
    
    // ------------------------------------------------------------------------
    // Traffic in Right Hand Lane At Start of Stage 1
    // ------------------------------------------------------------------------

    for (i = SPRITE_TRAFF1; i <= SPRITE_TRAFF8; i++)
    {
        oentry_init(&OSprites_jump_table[i], i);      
        OSprites_jump_table[i].control |= SPRITES_SHADOW;
        OSprites_jump_table[i].addr = Outrun_adr.sprite_porsche; // Initial offset of traffic sprites. Will be changed.
    }

    // ------------------------------------------------------------------------
    // Crash Sprites
    // ------------------------------------------------------------------------

    for (i = SPRITE_CRASH; i <= SPRITE_CRASH_PASS2_S; i++)
    {
        oentry_init(&OSprites_jump_table[i], i);
    }

    OSprites_jump_table[SPRITE_CRASH_PASS1].draw_props = DRAW_BOTTOM;
    OSprites_jump_table[SPRITE_CRASH_PASS2].draw_props = DRAW_BOTTOM;

    OSprites_jump_table[SPRITE_CRASH_PASS1_S].shadow     = 7;
    OSprites_jump_table[SPRITE_CRASH_PASS1_S].addr       = Outrun_adr.shadow_data;
    OSprites_jump_table[SPRITE_CRASH_PASS2_S].shadow     = 7;
    OSprites_jump_table[SPRITE_CRASH_PASS2_S].draw_props = DRAW_BOTTOM;
    OSprites_jump_table[SPRITE_CRASH_PASS2_S].addr       = Outrun_adr.shadow_data;
    
    OSprites_jump_table[SPRITE_CRASH_SPRITES_SHADOW].shadow     = 7;
    OSprites_jump_table[SPRITE_CRASH_SPRITES_SHADOW].zoom       = 0x80;
    OSprites_jump_table[SPRITE_CRASH_SPRITES_SHADOW].draw_props = DRAW_BOTTOM;
    OSprites_jump_table[SPRITE_CRASH_SPRITES_SHADOW].addr       = Outrun_adr.shadow_data;

    OCrash_init(
        &OSprites_jump_table[SPRITE_CRASH], 
        &OSprites_jump_table[SPRITE_CRASH_SPRITES_SHADOW], 
        &OSprites_jump_table[SPRITE_CRASH_PASS1],
        &OSprites_jump_table[SPRITE_CRASH_PASS1_S],
        &OSprites_jump_table[SPRITE_CRASH_PASS2],
        &OSprites_jump_table[SPRITE_CRASH_PASS2_S]
    );

    // ------------------------------------------------------------------------
    // Animation Sequence Sprites
    // ------------------------------------------------------------------------

    oentry_init(&OSprites_jump_table[SPRITE_FLAG], SPRITE_FLAG);
    OAnimSeq_init(OSprites_jump_table);
    
    OSprites_seg_pos             = 0;
    OSprites_seg_total_sprites   = 0;
    OSprites_seg_sprite_freq     = 0;
    OSprites_seg_spr_offset2     = 0;
    OSprites_seg_spr_offset1     = 0;
    OSprites_seg_spr_addr        = 0;

    do_sprite_swap      = 0;
    OSprites_sprite_scroll_speed = 0;
    OSprites_shadow_offset       = 0;
    OSprites_sprite_count        = 0;
    OSprites_spr_cnt_main        = 0;
    OSprites_spr_cnt_shadow      = 0;

    spr_col_pal         = 0;
    pal_copy_count      = 0;    
}

// Swap Sprite RAM And Update Palette Data
void OSprites_update_sprites()
{
	if (do_sprite_swap)
	{
        do_sprite_swap = 0;
        HWSprites_swap();
        OSprites_copy_palette_data();
	}
}

// Disable All Sprite Entries
// Source: 0x4A50
void OSprites_disable_sprites()
{
    uint8_t i;
    for (i = 0; i < SPRITE_ENTRIES; i++)
        OSprites_jump_table[i].control &= ~SPRITES_ENABLE;
}

void OSprites_tick()
{
    OSprites_sprite_control();
}

// Sprite Control
//
// Source: 3BEE
//
// Notes:
// - Setup Jump Table Entry #2, the sprite control. This in turn is used to control and setup all the sprites.
// - Read 4 Byte Entry From Road_Seg_Adr1 which indicates the upcoming block of sprite data for the level
// - This first block of data specifies the position, total number of sprites in the block we want to try rendering 
//   and appropriate lookup for the sprite number/frequency info.
//
// - This second table in memory specifies the frequency and number of sprites in the sequence. 
//  
// - The second table also contains the actual sprite info (x,y,palette,type). This can be multipled sprites.
//
// ----------------------------------
//
// road_seg_addr1 Format: [4 byte boundaries]
//
// [+0] Road Position [Word]
// [+2] Number Of Sprites In Segment [Byte]
// [+3] Sprite Data Entry Number From Lookup Table * 4 [Byte]
//
// Sprite Master Table Format @ 0x1A43C:
//
// A Table Containing a series of longs. Each address in this table represents:
//
// [+0] Sprite Frequency Value Bitmask [Word]
// [+2] Reload Value For Sprite Info Offset [Word]
// [+4] Start of table with x,y,type,palette etc.
//      This table can appear as many times as desired in each block and follows the format below, starting +4:
//
// -------------------------------------------------
// [+0] [Byte] Bit 0 = H-Flip Sprite
//             Bit 1 = Enable Shadows
//            
//             Bits 4-7 = Routine Draw Number
// [+1] [Byte] Sprite X World Position
// [+2] [Word] Sprite Y World Position
// [+5] [Byte] Sprite Type
// [+7] [Byte] Sprite Palette
//-------------------------------------------------
//
// The Frequency bitmask, for example 111000111 00111000
// rotates left, and whenever a bit is set, a sprite from the sequence is rendered.
//
// When a bit is unset no draw occurs on that call.
//
// The entire sequence can repeat, until the max sprites counter expires.
//
// So the above example would draw 3 sprites in succession, then break for three attempts, then three again etc.

void OSprites_sprite_control()
{
    uint16_t pos = TrackLoader_read_scenery_pos();

    // Populate next road segment
    if (pos <= ORoad_road_pos >> 16)
    {
        OSprites_seg_pos = pos;                                                          // Position In Level Data [Word]
        OSprites_seg_total_sprites = TrackLoader_read_total_sprites();                   // Number of Sprites In Segment
        uint8_t pattern_index = TrackLoader_read_sprite_pattern_index();        // Block Of Sprites
        TrackLoader_scenery_offset += 4;                                        // Advance to next scenery point
        
        uint32_t a0 = TrackLoader_read_scenerymap_table(pattern_index);         // Get Address of Scenery Pattern
        OSprites_seg_sprite_freq = TrackLoader_read16IncP(TrackLoader_scenerymap_data, &a0); // Scenery Frequency
        OSprites_seg_spr_offset2 = TrackLoader_read16IncP(TrackLoader_scenerymap_data, &a0); // Reload value for scenery pattern
        OSprites_seg_spr_addr = a0;                                                      // Set ROM address for sprite info lookup (x, y, type)
                                                                                // NOTE: Sets to value of a0 itself, not memory location
        OSprites_seg_spr_offset1 = 0;                                                    // And Clear the offset into the above table
    }

    // Process segment
    if (OSprites_seg_total_sprites == 0) return;
    if (OSprites_seg_pos > ORoad_road_pos >> 16) return;

    // ------------------------------------------------------------------------
    // Sprite 1
    // ------------------------------------------------------------------------
    // Rotate 16 bit value left. Stick top bit in low bit.
    uint16_t carry = OSprites_seg_sprite_freq & 0x8000;
    OSprites_seg_sprite_freq = ((OSprites_seg_sprite_freq << 1) | ((OSprites_seg_sprite_freq & 0x8000) >> 15)) & 0xFFFF;

    if (carry)
    {
        OSprites_seg_total_sprites--;
        OSprites_seg_spr_offset1 -= 8; //  Decrement rom address offset to point at next sprite [8 byte boundary]
        if (OSprites_seg_spr_offset1 < 0)
            OSprites_seg_spr_offset1 = OSprites_seg_spr_offset2; // Reload sprite info offset value
        OLevelObjs_setup_sprites(0x10400);
    }
 
    if (OSprites_seg_total_sprites == 0)
    {
        OSprites_seg_pos++;
        return;
    }

    // ------------------------------------------------------------------------
    // Sprite 2 - Second Sprite is slightly set back from the first.
    // ------------------------------------------------------------------------
    carry = OSprites_seg_sprite_freq & 0x8000;
    OSprites_seg_sprite_freq = ((OSprites_seg_sprite_freq << 1) | ((OSprites_seg_sprite_freq & 0x8000) >> 15)) & 0xFFFF;

    if (carry)
    {
        OSprites_seg_total_sprites--;
        OSprites_seg_spr_offset1 -= 8; //  Decrement rom address offset to point at next sprite [8 byte boundary]
        if (OSprites_seg_spr_offset1 < 0)
            OSprites_seg_spr_offset1 = OSprites_seg_spr_offset2; // Reload sprite info offset value
        OLevelObjs_setup_sprites(0x10000);
    }

    OSprites_seg_pos++;
}

// Source: 0x76F4
void OSprites_clear_palette_data()
{
    int16_t i;
    spr_col_pal = 0;
    for (i = 0; i < 0x100; i++)
        pal_lookup[i] = 0;
}


// Copy Sprite Palette Data To Palette RAM On Vertical Interrupt
// 
// Source Address: 0x858E
// Input:          Source address in rom of data format
// Output:         None
//

void OSprites_copy_palette_data()
{
    int16_t i;
    uint16_t j; 
    // Return if no palette entries to copy
    if (pal_copy_count <= 0) return;

    for (i = 0; i < pal_copy_count; i++)
    {
        // Palette Data Source Offset (aligned to start of 32 byte boundry, * 5)
        uint16_t src_offset = pal_addresses[(i * 2) + 0] << 5;
        uint32_t src_addr = 2 + PAL_DATA + src_offset; // Source address in ROM

        uint16_t dst_offset = pal_addresses[(i * 2) + 1] << 5;
        uint32_t dst_addr = 2 + PAL_SPRITES + dst_offset;

        // Move 28 Bytes from ROM to palette RAM
        for (j = 0; j < 7; j++)
        {
            Video_write_pal32IncP(&dst_addr, RomLoader_read32IncP(&Roms_rom0, &src_addr));
        }
    }
    pal_copy_count = 0; // All entries copied
}

// Map Palettes from ROM to Palette RAM for a particular sprite.
// 
// Source Address: 0x75EA
// Input:          Sprite
// Output:         None
//
// Prepares RAM for copy_palette_data routine on vint

// Notes:
// 1. Checks lookup table to determine whether relevant palette info is copied to ram. 
//    Return if already cached
// 2. Otherwise set the mapping between ROM and the HW Palette to be used
// 3. pal_copy_count contains the number of entries we need to copy
// 4. pal_addresses contains the address mapping

void OSprites_map_palette(oentry* spr)
{
    uint8_t pal = pal_lookup[spr->pal_src];

    // -----------------------------------
    // Entry is cached. Use entry.
    // -----------------------------------
    if (pal != 0)
    {
        spr->pal_dst = pal;
        return;
    }
    // -----------------------------------
    // Entry is not cached. Need to cache.
    // -----------------------------------

    // Increment hw colour palette entry to use
    if (++spr_col_pal > 0x7F) return;
    
    spr->pal_dst = spr_col_pal; // Set next available hw sprite colour palette
    pal_lookup[spr->pal_src] = spr_col_pal;

    pal_addresses[pal_copy_count * 2] = spr->pal_src;
    pal_addresses[(pal_copy_count * 2) + 1] = spr_col_pal;

    pal_copy_count++;
}

// Setup Sprite Ordering Table & Shadows
// 
// Source Address: 0x77A8
// Input:          Sprite To Copy
// Output:         None
//
// Notes:
// 1/ Reads Sprite-to-Sprite priority of individual sprite
// 2/ Creates ordered sprite table starting at 0x64000
// 3/ The format of this table is detailed in the comments at 0x78B0
// 4/ Optionally adds shadow to sprite if requires
//
// The end result is a table of sprite entries at 0x64000

void OSprites_do_spr_order_shadows(oentry* input)
{
    // LayOut specific fix to avoid memory crash on over populated scenery segments
    if (OSprites_spr_cnt_main + OSprites_spr_cnt_shadow >= JUMP_ENTRIES_TOTAL)
        return;

    // Use priority as lookup into table. Assume we're on boundaries of 0x10
    uint16_t priority = (input->priority & 0x1FF) << 4;
    uint8_t bytes_to_copy = sprite_order[priority];

    // Maximum number of bytes we want to copy is 0x10
    if (bytes_to_copy < 0xE)
    {
        bytes_to_copy++;
        sprite_order[priority] = bytes_to_copy;
        sprite_order[priority + bytes_to_copy + 1] = input->jump_index; // put at offset +2
        OSprites_spr_cnt_main++;
    }

#if !defined (_AMIGA_)
    // Code to handle shadows under sprites
    // test_shadow:
    if (!(input->control & SPRITES_SHADOW)) return;

    // LayOut specific fix to avoid memory crash on over populated scenery segments
    if (OSprites_spr_cnt_main + OSprites_spr_cnt_shadow >= JUMP_ENTRIES_TOTAL)
        return;

    input->dst_index = OSprites_spr_cnt_shadow;
    OSprites_spr_cnt_shadow++;                       // Increment total shadow count

    uint8_t pal_dst = input->pal_dst;       // Backup Sprite Colour Palette
    uint8_t shadow = input->shadow;         // and priority and shadow settings
    int16_t x = input->x;                   // and x position
    uint32_t addr = input->addr;            // and original sprite data address
    input->pal_dst = 0;                     // clear colour palette
    input->shadow = 7;                      // Set NEW priority & shadow settings

    input->x += (input->road_priority * OSprites_shadow_offset) >> 9; // d0 = sprite z / distance into screen

    if (input->control & SPRITES_TRAFFIC_SPRITE)
    {
        input->addr = Outrun_adr.sprite_shadow_small;
        input->x = x;
    }
    else
    {
        input->addr = RomLoader_read32(Roms_rom0p, Outrun_adr.shadow_frames + 0x3C);
    }

    OSprites_do_sprite(input);           // Create Shadowed Version Of Sprite For Hardware

    input->pal_dst = pal_dst;   // Restore Sprite Colour Palette
    input->shadow = shadow;     // ...and other values
    input->x = x;
    input->addr = addr;
    
#endif

}

// Sprite Copying Routine
// 
// Source Address: 0x78B0
// Input:          None
// Output:         None
//

void OSprites_sprite_copy()
{
    uint16_t i;
    if (OSprites_spr_cnt_main == 0)
    {
        OSprites_finalise_sprites();
        return;
    }

    if (OSprites_spr_cnt_main + OSprites_spr_cnt_shadow > 0x7F)
    {
        OSprites_spr_cnt_main = OSprites_spr_cnt_shadow = 0;
        OSprites_finalise_sprites();
        return;
    }

    uint32_t spr_cnt_main_copy = OSprites_spr_cnt_main;

    // look up in sprite_order
    int32_t src_addr = -0x10;

    // copy to sprite_entries
    uint32_t dst_index = 0;

    // Get next relevant entry (number of bytes to copy, followed by indexes of sprites)
    while (spr_cnt_main_copy != 0)
    {
        src_addr += 0x10;
        uint8_t bytes_to_copy = sprite_order[src_addr]; // warning: actually reads a word here normally, so this is wrong

        // Copy the required number of bytes
        if (bytes_to_copy != 0)
        {
            int32_t src_offset = src_addr + 2;

            do
            {
                // Sprite Index To Draw
                sprite_order2[dst_index++] = sprite_order[src_offset++];                
                if (--spr_cnt_main_copy == 0) break; // Loop continues until this is zero.
            }
            // Decrement number of bytes to copy
            while (--bytes_to_copy != 0);
        }

        // Clear number of bytes to copy
        sprite_order[src_addr] = 0; 
    }

    // cont2:
    uint16_t cnt_shadow_copy = OSprites_spr_cnt_shadow;

    // next_sprite
    for (i = 0; i < OSprites_spr_cnt_main; i++)
    {
        uint16_t jump_index = sprite_order2[i];
        oentry *entry = &OSprites_jump_table[jump_index];
        entry->dst_index = cnt_shadow_copy;
        cnt_shadow_copy++;
        OSprites_do_sprite(entry);
    }

    OSprites_finalise_sprites();
}

// Was originally labelled set_end_marker
// 
// Source Address: 0x7942
// Input:          None
// Output:         None
//

void OSprites_finalise_sprites()
{
    OSprites_sprite_count = OSprites_spr_cnt_main + OSprites_spr_cnt_shadow;
    
    // Set end sprite marker
    OSprites_sprite_entries[OSprites_sprite_count].data[0] = 0xFFFF;
    OSprites_sprite_entries[OSprites_sprite_count].data[1] = 0xFFFF;

    // TODO: Code to wait for interrupt goes here
    OSprites_blit_sprites();
    OTraffic_traffic_logic();
    OTraffic_traffic_sound();
    OSprites_spr_cnt_main = OSprites_spr_cnt_shadow = 0;

    // Ready to swap buffer and blit
    do_sprite_swap = 1;
}

// Copy Sprite Data to Sprite RAM 
// 
// Source Address: 0x97E4
// Input:          None
// Output:         None
//

void OSprites_blit_sprites()
{
    uint16_t i;
    uint32_t dst_addr = SPRITE_RAM;

    for (i = 0; i <= OSprites_sprite_count; i++)
    {
        uint16_t* data = OSprites_sprite_entries[i].data;

        // Write twelve bytes
        Video_write_sprite16(&dst_addr, data[0]);
        Video_write_sprite16(&dst_addr, data[1]);
        Video_write_sprite16(&dst_addr, data[2]);
        Video_write_sprite16(&dst_addr, data[3]);
        Video_write_sprite16(&dst_addr, data[4]);
        Video_write_sprite16(&dst_addr, data[5]);
        Video_write_sprite16(&dst_addr, data[6]);

        // Allign on correct boundary
        dst_addr += 2;
    }
}

// Convert Sprite From Internal Software Format To Hardware Format
// 
// Source Address: 0x94EC
// Input:          Sprite To Copy
// Output:         None
//
// 1. Copies Sprite Information From Jump Table Area To RAM
// 2. Stores In Similar Format To Sprite Hardware, but with 4 extra bytes of scratch data on end
// 3. Note: Mostly responsible for setting x,y,width,height,zoom,pitch,priorities etc.
//
// 0x11ED2: Table of Sprite Addresses for Hardware. Contains:
//
// 5 x 10 bytes. One block for each sprite size lookup. 
// The exact sprite is selected using the ozoom_lookup.h table.
//
// + 0 : [Byte] Unused
// + 1 : [Byte] Width Helper Lookup  [Offsets into 0x20000 (the width and height table)]
// + 2 : [Byte] Line Data Width
// + 3 : [Byte] Height Helper Lookup [Offsets into 0x20000 (the width and height table)]
// + 4 : [Byte] Line Data Height
// + 5 : [Byte] Sprite Pitch
// + 7 : [Byte] Sprite Bank
// + 8 : [Word] Offset Within Sprite Bank

void OSprites_do_sprite(oentry* input)
{
    input->control |= SPRITES_DRAW_SPRITE; // Display input sprite

    // Get Correct Output Entry
    osprite* output = &OSprites_sprite_entries[input->dst_index];

    // Copy address sprite was copied from.
    // todo: pass pointer?
    output->scratch = input->jump_index;

    // Hide Sprite if zoom lookup not set
    if (input->zoom == 0)
    {
        OSprites_hide_hwsprite(input, output);
        return;
    }

    // Sprite Width/Height Settings
    uint16_t width = 0;
    uint16_t height = 0;
    
    // Set real h/v zoom values
    uint32_t index = (input->zoom * 4);
    osprite_set_vzoom(output, ZOOM_LOOKUP[index]); // note we don't increment src_rom here
    osprite_set_hzoom(output, ZOOM_LOOKUP[index++]);

    // -------------------------------------------------------------------------
    // Set width & height values using lookup
    // -------------------------------------------------------------------------
    uint16_t lookup_mask = ZOOM_LOOKUP[index++]; // Width/Height lookup helper
    
    // This is the address of the frame required for the level of zoom we're using
    // There are 5 unique frames that are typically used for zoomed sprites.
    // which correspond to different screen sizes
    uint32_t src_offsets = input->addr + ZOOM_LOOKUP[index];

    uint16_t d0 = input->draw_props | (input->zoom << 8);
    uint16_t top_bit = d0 & 0x8000;
    d0 &= 0x7FFF; // Clear top bit
    
    if (top_bit == 0)
    {
        if (ZOOM_LOOKUP[index] != 0)
        {
            lookup_mask += 0x4000;
            d0 = lookup_mask;
        }

        d0 = (d0 & 0xFF00) + RomLoader_read8(Roms_rom0p, src_offsets + 1);
        width = RomLoader_read8(Roms_rom0p, WH_TABLE + d0);
        d0 = (d0 & 0xFF00) + RomLoader_read8(Roms_rom0p, src_offsets + 3);
        height = RomLoader_read8(Roms_rom0p, WH_TABLE + d0);
    }
    // loc_9560:
    else
    {
        d0 &= 0x7C00;
        uint16_t h = d0;

        d0 = (d0 & 0xFF00) + RomLoader_read8(Roms_rom0p, src_offsets + 1);
        width = RomLoader_read8(Roms_rom0p, WH_TABLE + d0);
        d0 &= 0xFF;
        width += d0;
        
        h |= RomLoader_read8(Roms_rom0p, src_offsets + 3);
        height = RomLoader_read8(Roms_rom0p, WH_TABLE + h);
        h &= 0xFF;
        height += h;

    }
    // loc 9582:
    input->width = width;
    
    // -------------------------------------------------------------------------
    // Set Sprite X & Y Values
    // -------------------------------------------------------------------------
    OSprites_set_sprite_xy(input, output, width, height);
    
    // Here we need the entire value set by above routine, not just top 0x1FF mask!
    int16_t sprite_x1 = osprite_get_x(output);
    int16_t sprite_x2 = sprite_x1 + width;
    int16_t sprite_y1 = osprite_get_y(output);
    int16_t sprite_y2 = sprite_y1 + height;

    const uint16_t x1_bounds = 512 + Config_s16_x_off;
    const uint16_t x2_bounds = 192 - Config_s16_x_off;

    // Hide Sprite if off screen (note bug fix to solve shadow wrapping issue on original game)
    // I think this bug might be permanently fixed with the introduction of widescreen mode
    // as I had to change the storage size of the x-cordinate. 
    // Unsetting fix_bugs may no longer revert to the original behaviour.
    if (sprite_y2 < 256 || sprite_y1 > 479 ||
        sprite_x2 < x2_bounds || (Config_engine.fix_bugs ? sprite_x1 >= x1_bounds : sprite_x1 > x1_bounds))
    {
        OSprites_hide_hwsprite(input, output);
        return;
    }

    // -------------------------------------------------------------------------
    // Set Palette & Sprite Bank Information
    // -------------------------------------------------------------------------
    osprite_set_pal(output, input->pal_dst); // Set Sprite Colour Palette
    osprite_set_offset(output, RomLoader_read16(Roms_rom0p, src_offsets + 8)); // Set Offset within selected sprite bank
    osprite_set_bank(output, RomLoader_read8(Roms_rom0p, src_offsets + 7) << 1); // Set Sprite Bank Value

    // -------------------------------------------------------------------------
    // Set Sprite Height
    // -------------------------------------------------------------------------
    if (sprite_y1 < 256)
    {
        int16_t y_adj = -(sprite_y1 - 256);
        y_adj *= RomLoader_read16(Roms_rom0p, src_offsets + 2); // Width of line data (Unsigned multiply)
        y_adj /= height; // Unsigned divide
        y_adj *= RomLoader_read16(Roms_rom0p, src_offsets + 4); // Length of line data (Unsigned multiply)
        osprite_inc_offset(output, y_adj);
        output->data[0x0] = (output->data[0x0] & 0xFF00) | 0x100; // Mask on negative y index
        osprite_set_height(output, (uint8_t) sprite_y2);
    }
    else
    {
        osprite_set_height(output, (uint8_t) height);
    }
    
    // -------------------------------------------------------------------------
    // Set Sprite Height Taking Elevation Of Road Into Account For Clipping Purposes
    //
    // Word 0: Priority of section
    // Word 1: Height of section
    //
    // Source: 0x9602
    // -------------------------------------------------------------------------

    // Start of priority elevation data in road ram
    uint16_t road_y_index = ORoad_road_p0 + 0x280;
    
    // Priority List Not Populated (Flat Elevation)
    if (ORoad_road_y[road_y_index + 0] == 0 && ORoad_road_y[road_y_index + 1] == 0)
    {
        // set_spr_height:
        if (sprite_y2 > 0x1DF)
            osprite_sub_height(output, sprite_y2 - 0x1DF);
    }
    // Priority List Populated (Elevated Section of road)
    else
    {
        // Count number of height_entries
        int16_t height_entry = 0;
        do
        {
            height_entry++;
            road_y_index += 2;
        }
        while (ORoad_road_y[road_y_index + 0] != 0 && ORoad_road_y[road_y_index + 1] != 0);

        height_entry--;

        do
        {
            road_y_index -= 2;
        }
        while (--height_entry > 0 && input->road_priority > ORoad_road_y[road_y_index + 0]);

        // Sprite has higher priority, draw sprite
        if (input->road_priority > ORoad_road_y[road_y_index])
        {
            // set_spr_height:
            if (sprite_y2 > 0x1DF)
                osprite_sub_height(output, sprite_y2 - 0x1DF);
        }
        // Road has higher priority, clip sprite
        else
        {
            // 9630:
            int16_t road_elevation = -ORoad_road_y[road_y_index + 1] + 0x1DF;
            if (sprite_y1 > road_elevation)
            {
                OSprites_hide_hwsprite(input, output);
                return;
            }
            else if (sprite_y2 <= road_elevation)
            {
                if (sprite_y2 > 0x1DF)
                    osprite_sub_height(output, sprite_y2 - 0x1DF);
            }
            else
            {
                osprite_sub_height(output, sprite_y2 - road_elevation);
            }
        }
    }

    // cont2:
    OSprites_set_hrender(input, output, RomLoader_read16(Roms_rom0p, src_offsets + 4), width);
    
    // -------------------------------------------------------------------------
    // Set Sprite Pitch & Priority
    // -------------------------------------------------------------------------
    osprite_set_pitch(output, RomLoader_read8(Roms_rom0p, src_offsets + 5) << 1);
    osprite_set_priority(output, input->shadow << 4); // todo: where does this get set?
}

// Hide Input And Output Entry
void OSprites_hide_hwsprite(oentry* input, osprite* output)
{
    input->control &= ~SPRITES_DRAW_SPRITE; // Hide input sprite
    osprite_hide(output);
}

// Sets Sprite Render Point
// 
// Source Address: 0x967C
// Input:          Jump Table Entry, Output Sprite Entry, Width & Height
// Output:         Updated Sprite Output Entry
//

void OSprites_set_sprite_xy(oentry* input, osprite* output, uint16_t width, uint16_t height)
{
    uint8_t anchor = input->draw_props;

    // -------------------------------------------------------------------------
    // Set Y Render Point
    // -------------------------------------------------------------------------

    int16_t y = input->y;

    switch((anchor & 0xC) >> 2)
    {
        // Anchor Center
        case 0:
        case 3:
            height >>= 1;
            y -= height;
            osprite_set_y(output, y + 256);
            break;
            
        // Anchor Left
        case 1:
            osprite_set_y(output, y + 256);
            break;

        // Anchor Right
        case 2:
            y -= height;
            osprite_set_y(output, y + 256);
            break;
    }

    // -------------------------------------------------------------------------
    // Set X Render Point
    // -------------------------------------------------------------------------

    int16_t x = input->x;

    switch(anchor & 0x3)
    {
        // Anchor Center
        case 0:
        case 3:
            width >>= 1;
            x -= width;
            osprite_set_x(output, x + 352);
            break;
            
        // Anchor Left
        case 1:
            osprite_set_x(output, x + 352);
            break;

        // Anchor Right
        case 2:
            x -= width;
            osprite_set_x(output, x + 352);
            break;
    }
}

// Determines whether to render sprite left-to-right or right-to-left
// 
// Source Address: 0x96E4
// Input:          Jump Table Entry, Output Sprite Entry, Offset
// Output:         Updated Sprite Output Entry
//
void OSprites_set_hrender(oentry* input, osprite* output, uint16_t offset, uint16_t width)
{
    uint8_t props = 0x60;
    uint8_t anchor = input->draw_props;

    // Anchor Top Left: Set Backwards & Left To Right Render
    if (anchor & 1)
        props = 0x60;
    // Anchor Bottom Right: Set Forwards & Right To Left Render
    else if (anchor & 2 || input->x < 0)
        props = 0;

    if (input->control & SPRITES_HFLIP)
    {
        if (props == 0) props = 0x40; // If H-Flip & Right To Left Render: Read data backwards
        else props = 0x20; // If H-Flip && Left To Right: Render left to right & Read data forwards
    }

    // setup_bank:

    // if reading forwards, increment the offset
    if ((props & 0x40) == 0)
    {
        osprite_inc_offset(output, offset - 1);
    }

    // if right to left, increment the x position
    if ((props & 0x20) == 0)
    {
        osprite_inc_x(output, width);
    }

    props |= 0x80; // Set render top to bottom
    osprite_set_render(output, props);
}

// Helper function to vary the move distance, based on the current frame-rate.
void OSprites_move_sprite(oentry* sprite, uint8_t shift)
{
    uint32_t addr = SPRITE_ZOOM_LOOKUP + (((sprite->z >> 16) << 2) | OSprites_sprite_scroll_speed);
    uint32_t value = RomLoader_read32(&Roms_rom0, addr) >> shift;

    if (Config_tick_fps == 60)
        value >>= 1;
    else if (Config_tick_fps == 120)
        value >>= 2;

    sprite->z += value;
}
