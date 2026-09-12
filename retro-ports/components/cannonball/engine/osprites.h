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

#pragma once

#include "oentry.h"
#include "osprite.h"
#include "outrun.h"


enum 
{
	SPRITES_HFLIP = 0x1,			// Bit 0: Horizontally flip sprite
	SPRITES_WIDE_ROAD = 0x4,		// Bit 2: Set if road_width >= 0x118, 
    SPRITES_TRAFFIC_SPRITE = 0x8,   // Bit 3: Traffic Sprite - Set for traffic
    SPRITES_SHADOW = 0x10,          // Bit 4: Sprite has shadows
	SPRITES_DRAW_SPRITE = 0x20,	    // Bit 5: Toggle sprite visibility
    SPRITES_TRAFFIC_RHS = 0x40,	    // Bit 6: Traffic Sprites - Set to spawn on RHS
	SPRITES_ENABLE = 0x80,	        // Bit 7: Toggle sprite visibility
};

// Note, the original game has 0x4F entries.
// But we can set it to a higher value, to fix the broken arches on Gateway.
// this is a limitation in the original game.
// We just leave the larger array in place when changing the settings. 
    
// Total sprite entries in Jump Table (start at offset #3)
#define SPRITE_ENTRIES 0x62
    
// This is initalized based on the config
extern uint8_t OSprites_no_sprites;

// Total number of object entries, including SPRITE_ENTRIES, FERRARI, PASSENGERS, TRAFFIC etc.
#define JUMP_ENTRIES_TOTAL          (SPRITE_ENTRIES + 24)

#define SPRITE_FERRARI              (SPRITE_ENTRIES + 1)
#define SPRITE_PASS1                (SPRITE_ENTRIES + 2)   // Passengers
#define SPRITE_PASS2                (SPRITE_ENTRIES + 3)
#define SPRITE_SPRITES_SHADOW       (SPRITE_ENTRIES + 4)   // Ferrari Shadow
#define SPRITE_SMOKE1               (SPRITE_ENTRIES + 5)   // Ferrari Tyre Smoke/Effects
#define SPRITE_SMOKE2               (SPRITE_ENTRIES + 6)
#define SPRITE_TRAFF1               (SPRITE_ENTRIES + 7)   // 8 Traffic Entries
#define SPRITE_TRAFF2               (SPRITE_ENTRIES + 8)
#define SPRITE_TRAFF3               (SPRITE_ENTRIES + 9)
#define SPRITE_TRAFF4               (SPRITE_ENTRIES + 10)
#define SPRITE_TRAFF5               (SPRITE_ENTRIES + 11)
#define SPRITE_TRAFF6               (SPRITE_ENTRIES + 12)
#define SPRITE_TRAFF7               (SPRITE_ENTRIES + 13)
#define SPRITE_TRAFF8               (SPRITE_ENTRIES + 14)
                                    
#define SPRITE_CRASH                (SPRITE_ENTRIES + 15)
#define SPRITE_CRASH_SPRITES_SHADOW (SPRITE_ENTRIES + 16)
#define SPRITE_CRASH_PASS1          (SPRITE_ENTRIES + 17)
#define SPRITE_CRASH_PASS1_S        (SPRITE_ENTRIES + 18)
#define SPRITE_CRASH_PASS2          (SPRITE_ENTRIES + 19)
#define SPRITE_CRASH_PASS2_S        (SPRITE_ENTRIES + 20)

#define SPRITE_FLAG                 (SPRITE_ENTRIES + 21)    // Flag Man

// Jump Table Sprite Entries
extern oentry *OSprites_jump_table; 

// Converted sprite entries in RAM for hardware.
extern osprite *OSprites_sprite_entries;

// -------------------------------------------------------------------------
// Jump Table 2 Entries For Sprite Control
// -------------------------------------------------------------------------

// +22 [Word] Road Position For Next Segment Of Sprites
extern uint16_t OSprites_seg_pos;

// +24 [Byte] Number Of Sprites In Segment  
extern uint8_t OSprites_seg_total_sprites;

// +26 [Word] Sprite Frequency Bitmask
extern uint16_t OSprites_seg_sprite_freq;

// +28 [Word] Sprite Info Offset - Start Value. Loaded Into 2A. 
extern int16_t OSprites_seg_spr_offset2;

// +2A [Word] Sprite Info Offset
extern int16_t OSprites_seg_spr_offset1;

// +2C [Long] Sprite Info Base - Lookup for Sprite X World, Sprite Y World, Sprite Type Table Info [8 byte boundary blocks in ROM]
extern uint32_t OSprites_seg_spr_addr;

// -------------------------------------------------------------------------

// Speed at which sprites should scroll. Depends on granular position difference.
extern uint16_t OSprites_sprite_scroll_speed;

// Shadow multiplication value (signed). Offsets the shadow from the sprite.
// Adjusted by the tilemap horizontal scroll, so shadows change depending on how much we've scrolled left and right
extern int16_t OSprites_shadow_offset;

// Number of sprites to draw for sprite drawing routine (sum of spr_cnt_main and spr_cnt_shadow).
extern uint16_t OSprites_sprite_count;

// Number of sprites to draw
extern uint16_t OSprites_spr_cnt_main;

// Number of shadows to draw
extern uint16_t OSprites_spr_cnt_shadow;

void OSprites_init();
void OSprites_disable_sprites();
void OSprites_tick();
void OSprites_update_sprites();

void OSprites_clear_palette_data();
void OSprites_copy_palette_data();
void OSprites_map_palette(oentry* spr);
void OSprites_sprite_copy();
void OSprites_blit_sprites();

void OSprites_do_spr_order_shadows(oentry*);
void OSprites_do_sprite(oentry*);
void OSprites_set_sprite_xy(oentry*, osprite*, uint16_t, uint16_t);
void OSprites_set_hrender(oentry*, osprite*, uint16_t, uint16_t);

void OSprites_move_sprite(oentry*, uint8_t);

