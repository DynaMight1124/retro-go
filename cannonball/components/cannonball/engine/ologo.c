/***************************************************************************
    Attract Mode: Animated OutRun Logo Graphic
    
    The logo is built from multiple sprite components.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.h"
#include "engine/osprites.h"
#include "engine/outils.h"
#include "engine/ologo.h"



// Palm Tree Frame Addresses
uint32_t palm_frames[8];

// Background Palette Entries
static const uint8_t bg_pal[] = { 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9A, 0x9B, 0x9C };
	
static uint8_t entry_start;

// Y Offset To Draw Logo At
int16_t y_off;
	
void OLogo_setup_sprite1();
void OLogo_setup_sprite2();
void OLogo_setup_sprite3();
void OLogo_setup_sprite4();
void OLogo_setup_sprite5();
void OLogo_setup_sprite6();
void OLogo_setup_sprite7();

void OLogo_sprite_logo_bg();
void OLogo_sprite_logo_car();
void OLogo_sprite_logo_bird1();
void OLogo_sprite_logo_bird2();
void OLogo_sprite_logo_road();
void OLogo_sprite_logo_palm();
void OLogo_sprite_logo_text();




void OLogo_enable(int16_t y)
{
    int i;
    y_off = -y;

    entry_start = SPRITE_ENTRIES - 0x10;

    // Enable block of sprites
    for (i = entry_start; i < entry_start + 7; i++)
    {
        oentry_init(&OSprites_jump_table[i], i);
    }

    palm_frames[0] = Outrun_adr.sprite_logo_palm1;
    palm_frames[1] = Outrun_adr.sprite_logo_palm2;
    palm_frames[2] = Outrun_adr.sprite_logo_palm3;
    palm_frames[3] = Outrun_adr.sprite_logo_palm2;
    palm_frames[4] = Outrun_adr.sprite_logo_palm1;
    palm_frames[5] = Outrun_adr.sprite_logo_palm2;
    palm_frames[6] = Outrun_adr.sprite_logo_palm3;
    palm_frames[7] = Outrun_adr.sprite_logo_palm2;

    OLogo_setup_sprite1(); 
    OLogo_setup_sprite2();
    OLogo_setup_sprite3();
    OLogo_setup_sprite4();
    OLogo_setup_sprite5();
    OLogo_setup_sprite6();
    OLogo_setup_sprite7();
}

void OLogo_disable()
{
    int i;
    // Enable block of sprites
    for (i = entry_start; i < entry_start + 7; i++)
    {
        OSprites_jump_table[i].control &= ~SPRITES_ENABLE;
    }
}

void OLogo_tick()
{
    OLogo_sprite_logo_bg();
    OLogo_sprite_logo_car();
    OLogo_sprite_logo_bird1();
    OLogo_sprite_logo_bird2();
    OLogo_sprite_logo_road();
    OLogo_sprite_logo_palm();
    OLogo_sprite_logo_text();
}

// Animated Background Oval Sprite.
void OLogo_setup_sprite1()
{
    oentry *e = &OSprites_jump_table[entry_start + 0];
    e->x = 0;
    e->y = 0x70 - y_off;
    e->road_priority = 0xFF;
    e->priority = 0x1FA;
    e->zoom = 0x7F;
    e->pal_src = 0x99;
    e->addr = Outrun_adr.sprite_logo_bg;
    OSprites_map_palette(e);
}

// Sprite Car Graphic
void OLogo_setup_sprite2()
{
    oentry *e = &OSprites_jump_table[entry_start + 1];
    e->x = -3;
    e->y = 0x88 - y_off;
    e->road_priority = 0x100;
    e->priority = 0x1FB;
    e->zoom = 0x7F;
    e->pal_src = 0x6E;
    e->addr = Outrun_adr.sprite_logo_car;
    OSprites_map_palette(e);
}

// Sprite Flying Bird #1
void OLogo_setup_sprite3()
{
    oentry *e = &OSprites_jump_table[entry_start + 2];
    e->x = 0x8;
    e->y = 0x4E - y_off;
    e->road_priority = 0x102;
    e->priority = 0x1FD;
    e->zoom = 0x7F;
    e->counter = 0;
    e->pal_src = 0x8B;
    e->addr = Outrun_adr.sprite_logo_bird1;
    OSprites_map_palette(e);
}

// Sprite Flying Bird #2
void OLogo_setup_sprite4()
{
    oentry *e = &OSprites_jump_table[entry_start + 3];
    e->x = 0x8;
    e->y = 0x4E - y_off;
    e->road_priority = 0x102;
    e->priority = 0x1FD;
    e->zoom = 0x7F;
    e->counter = 0x20;
    e->pal_src = 0x8C;
    e->addr = Outrun_adr.sprite_logo_bird2;
    OSprites_map_palette(e);
}

// Sprite Road Base Section
void OLogo_setup_sprite5()
{
    oentry *e = &OSprites_jump_table[entry_start + 4];
    e->x = -0x20;
    e->y = 0x8F - y_off;
    e->road_priority = 0x101;
    e->priority = 0x1FC;
    e->zoom = 0x7F;
    e->pal_src = 0x6E;
    e->addr = Outrun_adr.sprite_logo_base;
    OSprites_map_palette(e);
}

// Palm Tree
void OLogo_setup_sprite6()
{
    oentry *e = &OSprites_jump_table[entry_start + 5];
    e->x = -0x40;
    e->y = 0x6D - y_off;
    e->road_priority = 0x102;
    e->priority = 0x1FD;
    e->zoom = 0x7F;
    e->pal_src = 0x65;
    e->addr = Outrun_adr.sprite_logo_palm1;
    OSprites_map_palette(e);
}

// OutRun logo text
void OLogo_setup_sprite7()
{
    oentry *e = &OSprites_jump_table[entry_start + 6];
    e->x = 0x11;
    e->y = 0x65 - y_off;
    e->road_priority = 0x103;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->counter = 0;
    e->pal_src = 0x65;
    e->addr = Outrun_adr.sprite_logo_text;
    OSprites_map_palette(e);
}

// Draw Background of OutRun logo in attract mode
void OLogo_sprite_logo_bg()
{
    oentry *e = &OSprites_jump_table[entry_start + 0];
    e->reload++;
    uint16_t d0 = e->reload;
    uint16_t d1 = d0 - 1;
    d1 ^= d0;

    // Map new palette
    if (d1 & BIT_3)
    {
        e->pal_src = bg_pal[outils_random() & 7];
        OSprites_map_palette(e);
    }

    OSprites_do_spr_order_shadows(e);
}

void OLogo_sprite_logo_car()
{
    oentry *e = &OSprites_jump_table[entry_start + 1];
    // Flicker background palette of car
    e->reload++;
    e->pal_src = ((e->reload & 2) >> 1) ? 0x8A : 0x6E;
    OSprites_map_palette(e);
    OSprites_do_spr_order_shadows(e);
}

void OLogo_sprite_logo_bird1()
{
    oentry *e = &OSprites_jump_table[entry_start + 2];
    e->counter++;

    // Set Bird X Value
    uint16_t index = (e->counter << 1) & 0xFF;
    int8_t bird_x = RomLoader_read8(&Roms_rom0, DATA_MOVEMENT + index); // Note we sign the value here
    int8_t zoom = bird_x >> 3;
    e->x = (bird_x >> 3) + 8;

    // Set Zoom Lookup
    e->zoom = zoom + 0x70;

    // Set Bird Y Value
    index = (index << 1) & 0xFF;
    int8_t bird_y = RomLoader_read8(&Roms_rom0, DATA_MOVEMENT + index); // Note we sign the value here
    e->y = (bird_y >> 5) + 0x4E - y_off;

    // Set Frame
    e->reload++;
    uint16_t frame = (e->reload & 4) >> 2;
    e->addr = frame ? Outrun_adr.sprite_logo_bird2 : Outrun_adr.sprite_logo_bird1;
    OSprites_do_spr_order_shadows(e);
}

void OLogo_sprite_logo_bird2()
{
    oentry *e = &OSprites_jump_table[entry_start + 3];
    e->counter++;

    // Set Bird X Value
    uint16_t index = (e->counter << 1) & 0xFF;
    int8_t bird_x = RomLoader_read8(&Roms_rom0, DATA_MOVEMENT + index); // Note we sign the value here
    int8_t zoom = bird_x >> 3;
    e->x = (bird_x >> 3) - 2; // Different from sprite_logo_bird1

    // Set Zoom Lookup
    e->zoom = zoom + 0x70;

    // Set Bird Y Value
    index = (index << 1) & 0xFF;
    int8_t bird_y = RomLoader_read8(&Roms_rom0, DATA_MOVEMENT + index); // Note we sign the value here
    e->y = (bird_y >> 5) + 0x52 - y_off; // Different from sprite_logo_bird1

    // Set Frame
    e->reload++;
    uint16_t frame = (e->reload & 4) >> 2;
    e->addr = frame ? Outrun_adr.sprite_logo_bird2 : Outrun_adr.sprite_logo_bird1;
    OSprites_do_spr_order_shadows(e);
}

void OLogo_sprite_logo_road()
{
    OSprites_do_spr_order_shadows(&OSprites_jump_table[entry_start + 4]); 
}

void OLogo_sprite_logo_palm()
{
    oentry *e = &OSprites_jump_table[entry_start + 5]; 
    e->reload++; // Increment frame number
    e->addr = palm_frames[(e->reload & 0xE) >> 1];
    OSprites_do_spr_order_shadows(e);
}

void OLogo_sprite_logo_text()
{
    OSprites_do_spr_order_shadows(&OSprites_jump_table[entry_start + 6]); 
}

// Blit Only: Used when frame skipping
void OLogo_blit()
{
    int i;
    for (i = 0; i < 7; i++)
        OSprites_do_spr_order_shadows(&OSprites_jump_table[entry_start + i]);
}
