/***************************************************************************
    Music Selection Screen.

    This is a combination of a tilemap and overlayed sprites.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include "engine/oferrari.h"
#include "engine/ohud.h"
#include "engine/oinputs.h"
#include "engine/ologo.h"
#include "engine/omusic.h"
#include "engine/otiles.h"
#include "engine/otraffic.h"
#include "engine/ostats.h"

uint8_t OMusic_music_selected;

static uint16_t entry_start;

// Used to preview music track
int16_t last_music_selected;
int8_t preview_counter;
    
void OMusic_setup_sprite1();
void OMusic_setup_sprite2();
void OMusic_setup_sprite3();
void OMusic_setup_sprite4();
void OMusic_setup_sprite5();
void OMusic_blit_music_select();

RomLoader tilemap;
RomLoader tile_patch;



// Load Modified Widescreen version of tilemap
uint8_t OMusic_load_widescreen_map()
{
    int status = 0;

    if (!tilemap.loaded)
    {
        status += RomLoader_load_binary(&tilemap, "res/tilemap.bin");
    }

    if (!tile_patch.loaded)
    {
        status += RomLoader_load_binary(&tile_patch, "res/tilepatch.bin");
    }

    return status == 0;
}


// Initialize Music Selection Screen
//
// Source: 0xB342
void OMusic_enable()
{
    int i;
    OFerrari_car_ctrl_active = 0;
    Video_clear_text_ram();
    OSprites_disable_sprites();
    OTraffic_disable_traffic();
    //edit jump table 3
    OInitEngine_car_increment = 0;
    OFerrari_car_inc_old      = 0;
    OSprites_spr_cnt_main     = 0;
    OSprites_spr_cnt_shadow   = 0;
    ORoad_road_ctrl           = ROAD_BOTH_P0;
    ORoad_horizon_base        = -0x3FF;
    last_music_selected       = -1;
    preview_counter           = -20; // Delay before playing music
    OStats_time_counter       = 0x30; // Move 30 seconds to timer countdown (note on the original roms this is 15 seconds)
    OStats_frame_counter      = OStats_frame_reset;  
     
    OMusic_blit_music_select();
    OHud_blit_text2(TEXT2_SELECT_MUSIC); // Select Music By Steering
  
    OSoundInt_queue_sound(sound_RESET);
    if (!Config_sound.preview)
        OSoundInt_queue_sound(sound_PCM_WAVE); // Wave Noises

    // Enable block of sprites
    entry_start = SPRITE_ENTRIES - 0x10;    
    for (i = entry_start; i < entry_start + 5; i++)
    {
        oentry_init(&OSprites_jump_table[i], i);
    }

    OMusic_setup_sprite1();
    OMusic_setup_sprite2();
    OMusic_setup_sprite3();
    OMusic_setup_sprite4();
    OMusic_setup_sprite5();

  
    // Widescreen tiles need additional palette information copied over
    if (tile_patch.loaded && Config_s16_x_off > 0)
    {
        HWTiles_patch_tiles(&tile_patch);
        OTiles_setup_palette_widescreen();
    }

    HWTiles_set_x_clamp(HWTILES_CENTRE);
}

void OMusic_disable()
{
    int i;
    // Disable block of sprites
    for (i = entry_start; i < entry_start + 5; i++)
    {
        OSprites_jump_table[i].control &= ~SPRITES_ENABLE;
    }

    HWTiles_set_x_clamp(HWTILES_RIGHT);

    // Restore original palette for widescreen tiles.
    if (Config_s16_x_off > 0)
    {
        HWTiles_restore_tiles();
        OTiles_setup_palette_tilemap();
    }

    Video_enabled = 0; // Turn screen off
}

// Music Selection Screen: Setup Radio Sprite
// Source: 0xCAF0
void OMusic_setup_sprite1()
{
    oentry *e = &OSprites_jump_table[entry_start + 0];
    e->x = 28;
    e->y = 180;
    e->road_priority = 0xFF;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->pal_src = 0xB0;
    e->addr = Outrun_adr.sprite_radio;
    OSprites_map_palette(e);
}

// Music Selection Screen: Setup Equalizer Sprite
// Source: 0xCB2A
void OMusic_setup_sprite2()
{
    oentry *e = &OSprites_jump_table[entry_start + 1];
    e->x = 4;
    e->y = 189;
    e->road_priority = 0xFF;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->pal_src = 0xA7;
    e->addr = Outrun_adr.sprite_eq;
    OSprites_map_palette(e);
}

// Music Selection Screen: Setup FM Radio Readout
// Source: 0xCB64
void OMusic_setup_sprite3()
{
    oentry *e = &OSprites_jump_table[entry_start + 2];
    e->x = -8;
    e->y = 176;
    e->road_priority = 0xFF;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->pal_src = 0x87;
    e->addr = Outrun_adr.sprite_fm_left;
    OSprites_map_palette(e);
}

// Music Selection Screen: Setup FM Radio Dial
// Source: 0xCB9E
void OMusic_setup_sprite4()
{
    oentry *e = &OSprites_jump_table[entry_start + 3];
    e->x = 68;
    e->y = 181;
    e->road_priority = 0xFF;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->pal_src = 0x89;
    e->addr = Outrun_adr.sprite_dial_left;
    OSprites_map_palette(e);
}

// Music Selection Screen: Setup Hand Sprite
// Source: 0xCBD8
void OMusic_setup_sprite5()
{
    oentry *e = &OSprites_jump_table[entry_start + 4];
    e->x = 21;
    e->y = 196;
    e->road_priority = 0xFF;
    e->priority = 0x1FE;
    e->zoom = 0x7F;
    e->pal_src = 0xAF;
    e->addr = Outrun_adr.sprite_hand_left;
    OSprites_map_palette(e);
}

// Check for start button during music selection screen
//
// Source: 0xB768
void OMusic_check_start()
{
    if (OStats_credits && Input_is_pressed_clear(INPUT_START))
    {
        Outrun_game_state = GS_INIT_GAME;
        OLogo_disable();
        OMusic_disable();
    }
}

// Tick and Blit
void OMusic_tick()
{
    // Note tiles to append to left side of text
    const uint32_t NOTE_TILES1 = 0x8A7A8A7B;
    const uint32_t NOTE_TILES2 = 0x8A7C8A7D;

    // Radio Sprite
    OSprites_do_spr_order_shadows(&OSprites_jump_table[entry_start + 0]);

    // Animated EQ Sprite (Cycle the graphical equalizer on the radio)
    oentry *e = &OSprites_jump_table[entry_start + 1];
    e->reload++; // Increment palette entry
    e->pal_src = RomLoader_read8(&Roms_rom0, (e->reload & 0x3E) >> 1 | MUSIC_EQ_PAL);
    OSprites_map_palette(e);
    OSprites_do_spr_order_shadows(e);

    // Draw appropriate FM station on radio, depending on steering setting
    // Draw Dial on radio, depending on steering setting
    e = &OSprites_jump_table[entry_start + 2];
    oentry *e2 = &OSprites_jump_table[entry_start + 3];
    oentry *hand = &OSprites_jump_table[entry_start + 4];

    // Steer Left
    if (OInputs_steering_adjust + 0x80 <= 0x55)
    {                
        hand->x = 17;

        e->addr    = Outrun_adr.sprite_fm_left;
        e2->addr   = Outrun_adr.sprite_dial_left;
        hand->addr = Outrun_adr.sprite_hand_left;

        
        if (Config_sound.custom_music[0].enabled)
        {
            OHud_blit_text_big(11, Config_sound.custom_music[0].title, 1);
            OMusic_music_selected = 0;
        }
        else
        {
            OHud_blit_text2(TEXT2_MAGICAL);
            Video_write_text32(0x1105C0, NOTE_TILES1);
            Video_write_text32(0x110640, NOTE_TILES2);
            OMusic_music_selected = SOUND_MUSIC_MAGICAL;
        }

    }
    // Centre
    else if (OInputs_steering_adjust + 0x80 <= 0xAA)
    {
        hand->x = 21;

        e->addr    = Outrun_adr.sprite_fm_centre;
        e2->addr   = Outrun_adr.sprite_dial_centre;
        hand->addr = Outrun_adr.sprite_hand_centre;

        if (Config_sound.custom_music[1].enabled)
        {
            OHud_blit_text_big(11, Config_sound.custom_music[1].title, 1);
            OMusic_music_selected = 1;
        }
        else
        {
            OHud_blit_text2(TEXT2_BREEZE);
            Video_write_text32(0x1105C6, NOTE_TILES1);
            Video_write_text32(0x110646, NOTE_TILES2);
            OMusic_music_selected = SOUND_MUSIC_BREEZE;
        }
    }
    // Steer Right
    else
    {
        hand->x = 21;

        e->addr    = Outrun_adr.sprite_fm_right;
        e2->addr   = Outrun_adr.sprite_dial_right;
        hand->addr = Outrun_adr.sprite_hand_right;

        if (Config_sound.custom_music[2].enabled)
        {
            OHud_blit_text_big(11, Config_sound.custom_music[2].title, 1);
            OMusic_music_selected = 2;
        }
        else
        {
            OHud_blit_text2(TEXT2_SPLASH);
            Video_write_text32(0x1105C8, NOTE_TILES1);
            Video_write_text32(0x110648, NOTE_TILES2);
            OMusic_music_selected = SOUND_MUSIC_SPLASH;
        }
    }

    OSprites_do_spr_order_shadows(e);
    OSprites_do_spr_order_shadows(e2);
    OSprites_do_spr_order_shadows(hand);

    // Enhancement: Preview Music On Sound Selection Screen
    if (Config_sound.preview)
    {
        //if (mod)
        //{
        //    SND_StopModule();
        //    SND_EjectModule(mod);            
        //    mod = NULL;
        //}
        
        switch (OMusic_music_selected)
        {
            // Cycle in-built sounds
            case SOUND_MUSIC_BREEZE:
                //mod = SND_LoadModule("data/outrun_1.mod");
                //SND_SetFXChannel(2);
                //SND_PlayModule(mod);
                
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();
                    I_CAMD_PlaySong("data/Pass-Brz.mid");
                }
                
                break;
            case SOUND_MUSIC_SPLASH:
                //mod = SND_LoadModule("data/outrun_2.mod");
                //SND_SetFXChannel(2);                    
                //SND_PlayModule(mod);
                
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();
                    I_CAMD_PlaySong("data/Splash.mid");
                }
                
                break;
            case SOUND_MUSIC_MAGICAL:
                //mod = SND_LoadModule("data/outrun_3.mod");
                //SND_SetFXChannel(2);                    
                //SND_PlayModule(mod);
                
                if (Config_sound.amiga_midi)
                {
                    I_CAMD_StopSong();
                    I_CAMD_PlaySong("data/Magical.mid");
                }
                break;
        }
            
        if (OMusic_music_selected != last_music_selected)
        {
            if (preview_counter == 0 && last_music_selected != -1)
                OSoundInt_queue_sound(sound_FM_RESET);

            if (++preview_counter >= 10)
            {
                preview_counter = 0;
                OSoundInt_queue_sound(OMusic_music_selected);
                last_music_selected = OMusic_music_selected;
            }
        
        }
    }
}

// Blit Only: Used when frame skipping
void OMusic_blit()
{
    int i;
    for (i = 0; i < 5; i++)
        OSprites_do_spr_order_shadows(&OSprites_jump_table[entry_start + i]);
}

// Blit Music Selection Tiles to text ram layer (Double Row)
// 
// Source Address: 0xE0DC
// Input:          Destination address into tile ram
// Output:         None
//
// Tilemap data is stored in the ROM as a series of words.
//
// A basic compression format is used:
//
// 1/ If a word is not '0000', copy it directly to tileram
// 2/ If a word is '0000' a long follows which details the compression.
//    The upper word of the long is the value to copy.
//    The lower word of the long is the number of times to copy that value.
//
// Tile structure:
//
// MSB          LSB
// ---nnnnnnnnnnnnn Tile index (0-8191)
// ---ccccccc------ Palette (0-127)
// p--------------- Priority flag
// -??------------- Unknown

void OMusic_blit_music_select()
{
    int i,y,x;
    const uint32_t TILEMAP_RAM_16 = 0x10F030;

    // Palette Ram: 1F Long Entries For Sky Shade On Horizon, For Colour Change Effect
    const uint32_t PAL_RAM_SKY = 0x120F00;

    uint32_t src_addr = PAL_MUSIC_SELECT;
    uint32_t dst_addr = PAL_RAM_SKY;

    // Write 32 Palette Longs to Palette RAM
    for (i = 0; i < 32; i++)
        Video_write_pal32IncP(&dst_addr, RomLoader_read32IncP(&Roms_rom0, &src_addr));

    // Set Tilemap Scroll
    OTiles_set_scroll(Config_s16_x_off, 0);
    
    // --------------------------------------------------------------------------------------------
    // Blit to Tilemap 16: Widescreen Version. Uses Custom Tilemap. 
    // --------------------------------------------------------------------------------------------
    if (tilemap.loaded && Config_s16_x_off > 0)
    {
        uint32_t tilemap16 = TILEMAP_RAM_16 - 20;
        src_addr = 0;

        const uint16_t rows = RomLoader_read16IncP(&tilemap, &src_addr);
        const uint16_t cols = RomLoader_read16IncP(&tilemap, &src_addr);

        for (y = 0; y < rows; y++)
        {
            dst_addr = tilemap16;
            for (x = 0; x < cols; x++)
                Video_write_tile16IncP(&dst_addr, RomLoader_read16IncP(&tilemap, &src_addr));
            tilemap16 += 0x80; // next line of tiles
        }
    }

    // --------------------------------------------------------------------------------------------
    // Blit to Tilemap 16: Original 4:3 Version. 
    // --------------------------------------------------------------------------------------------
	else
	{
	    uint32_t tilemap16 = TILEMAP_RAM_16;
	    src_addr = TILEMAP_MUSIC_SELECT;
	
	    for (y = 0; y < 28; y++)
	    {
	        dst_addr = tilemap16;
	        for (x = 0; x < 40;)
	        {
	            // get next tile
	            uint32_t data = RomLoader_read16IncP(&Roms_rom0, &src_addr);
	            // No Compression: write tile directly to tile ram
	            if (data != 0)
	            {
	                Video_write_tile16IncP(&dst_addr, data);    
	                x++;
	            }
	            // Compression
	            else
	            {
	                uint16_t value = RomLoader_read16IncP(&Roms_rom0, &src_addr); // tile index to copy
	                uint16_t count = RomLoader_read16IncP(&Roms_rom0, &src_addr); // number of times to copy value
	
	                for (i = 0; i <= count; i++)
	                {
	                    Video_write_tile16IncP(&dst_addr, value);
	                    x++;
	                }
	            }
	        }
	        tilemap16 += 0x80; // next line of tiles
	    } // end for
	
	    // Fix Misplaced tile on music select screen (above steering wheel)
	    if (Config_engine.fix_bugs)
	        Video_write_tile16(0x10F730, 0x0C80);
	}
}
