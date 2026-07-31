/***************************************************************************
    Course Map Logic & Rendering. 
    
    This is the full-screen map that is displayed at the end of the game. 
    
    The logo is built from multiple sprite components.
    
    The course map itself is made up of sprites and pieced together. 
    It's not a tilemap.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/oferrari.h"
#include "engine/omap.h"
#include "engine/otiles.h"
#include "engine/otraffic.h"
#include "engine/ostats.h"
#include <stdint.h>

// Position of Ferrari in Jump Table
const uint8_t MAP_SPRITE_FERRARI = 25;

uint8_t OMap_init_sprites;

// Total sprite pieces that comprise course map. 3c
const static uint8_t MAP_PIECES = 0x3C;

uint8_t map_state;

enum
{
    MAP_INIT  = 0,
    // Do Route [Note map is displayed from this point on]
    MAP_ROUTE = 0x4,
    // Do Final Segment Of Route [Car still moving]
    MAP_ROUTE_FINAL = 0x08,
    // Route Concluded
    MAP_ROUTE_DONE = 0x0C,
    // Init Delay Counter For Map Display
    MAP_INIT_DELAY = 0x10,
    // Display Map
    MAP_DISPLAY = 0x14,
    // Clear Course Map
    MAP_CLEAR = 0x18,
};

// Direction to move on mini-map

// Bit 0: 0 = Up   (Left Route)
//        1 = Down (Right Route)
uint8_t map_route;

// Minimap Position (Per Segment Basis)
int16_t map_pos;

// Minimap Position (Final Segment)
int16_t map_pos_final;

// Map Delay Counter
int16_t map_delay;

// Stage counter for course map screen. [Increments]
int16_t map_stage1;

// Stage counter for course map screen. [Decrements]
// Loaded with stage, then counts down as course map logic runs.
int16_t map_stage2;

// Minicar Movement Enabled (set = enabled)
uint8_t minicar_enable;

void OMap_draw_horiz_end(oentry*);
void OMap_draw_vert_bottom(oentry*);
void OMap_draw_vert_top(oentry*);
void OMap_draw_piece(oentry*, uint32_t);
void OMap_do_route_final();
void OMap_end_route();
void OMap_init_map_delay();
void OMap_map_display();
void OMap_move_mini_car(oentry*);


void OMap_init()
{
    OFerrari_car_ctrl_active = 0; // -1
    Video_clear_text_ram();
    OSprites_disable_sprites();
    OTraffic_disable_traffic();
    OSprites_clear_palette_data();
    OInitEngine_car_increment = 0;
    OFerrari_car_inc_old      = 0;
    OSprites_spr_cnt_main     = 0;
    OSprites_spr_cnt_shadow   = 0;
    ORoad_road_ctrl           = ROAD_BOTH_P0;
    ORoad_horizon_base        = -0x3FF;
    OTiles_fill_tilemap_color(0xABD); //  Paint pinkish colour on tilemap 16
    OMap_init_sprites = 1;
}

// Process route through levels
// Process end position on final level
// Source: 0x345E
void OMap_tick()
{
    // 60 FPS Code to simply render sprites
    if (!Outrun_tick_frame)
    {
        OMap_blit();
        return;
    }

    // Initialize Course Map Sprites if necessary
    if (OMap_init_sprites)
    {
        OMap_load_sprites();
        OMap_init_sprites = 0;
        return;
    }

    switch (map_state)
    {
        // Initialise Route Info
        case MAP_INIT:
            HWSprites_set_x_clip(0); // Don't clip the area in wide-screen mode
            map_route  = RomLoader_read8(&Roms_rom0, MAP_ROUTE_LOOKUP + OStats_routes[1]);
            map_pos    = 0;
            map_stage1 = 0;
            map_stage2 = OStats_cur_stage;
            if (map_stage2 > 0)
                map_state = MAP_ROUTE;
            else
            {
                map_state = MAP_ROUTE_FINAL;
                OMap_do_route_final();
                break;
            }

        // Do Route [Note map is displayed from this point on]
        case MAP_ROUTE:
            if (++map_pos > 0x1B)
            {
                if (--map_stage2 <= 0)
                {   //map_end_route
                    map_pos = 0;
                    map_stage1++;
                    uint16_t route_info = OStats_routes[1 + map_stage1];
                    if (route_info)
                    {
                        map_route = RomLoader_read8(&Roms_rom0, MAP_ROUTE_LOOKUP + route_info);                      
                    }
                    else
                    {
                        map_route = RomLoader_read8(&Roms_rom0, MAP_ROUTE_LOOKUP + OStats_routes[0 + map_stage1] + 0x10);
                    }

                    map_state = MAP_ROUTE_FINAL;
                    OMap_do_route_final();
                }
                else
                {
                    map_pos = 0;
                    map_stage1++;
                    map_route = RomLoader_read8(&Roms_rom0, MAP_ROUTE_LOOKUP + OStats_routes[1 + map_stage1]);
                }
            }
            break;
        
        // Do Final Segment Of Route [Car still moving]
        case MAP_ROUTE_FINAL:
            OMap_do_route_final();
            break;

        // Route Concluded       
        case MAP_ROUTE_DONE:
            OMap_end_route();
            break;
        
        // Init Delay Counter For Map Display
        case MAP_INIT_DELAY:
            OMap_init_map_delay();
            break;

        // Display Map
        case MAP_DISPLAY:
            OMap_map_display();
            break;

        // Clear Course Map        
        case MAP_CLEAR:
            Outrun_init_best_outrunners();
            return;
    }

    OMap_draw_course_map();
}

// Render sprites only. No Logic
void OMap_blit()
{
    uint8_t i;
    for (i = 0; i <= MAP_PIECES; i++)
    {
        oentry* sprite = &OSprites_jump_table[i];
        if (sprite->control & SPRITES_ENABLE)
            OSprites_do_spr_order_shadows(sprite);
    }
}

void OMap_draw_course_map()
{
    uint8_t i;
    
    oentry* sprite = OSprites_jump_table;

    // Draw Road Components
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_vert_bottom(sprite++);
    OMap_draw_vert_top   (sprite++);
    OMap_draw_horiz_end  (sprite++);
    OMap_draw_horiz_end  (sprite++);
    OMap_draw_horiz_end  (sprite++);
    OMap_draw_horiz_end  (sprite++);
    OMap_draw_horiz_end  (sprite++);

    // Draw Mini Car
    OMap_move_mini_car   (sprite++);

    // Draw Backdrop Map Pieces
    for (i = 26; i <= MAP_PIECES; i++)
    {
        if (sprite->control & SPRITES_ENABLE)
            OSprites_do_spr_order_shadows(sprite++);
    }
}


void OMap_position_ferrari(uint8_t index)
{
    oentry* segment = &OSprites_jump_table[index];
    OSprites_jump_table[MAP_SPRITE_FERRARI].x = segment->x - 8;
    OSprites_jump_table[MAP_SPRITE_FERRARI].y = segment->y;
}

// Initalize Course Map Sprites
// 
// Notes: Index 26 is start of water that needs to be changed for widescreen
// 
// Source: 0x33F4
void OMap_load_sprites()
{
    uint8_t i;
    // hacks
    /*OStats_cur_stage = 4;
    OStats_routes[0] = 4;
    OStats_routes[1] = 0x08;
    OStats_routes[2] = 0x18;
    OStats_routes[3] = 0x28;
    OStats_routes[4] = 0x38;
    OInitEngine_rd_split_state = 0x16;
    ORoad_road_pos = 0x192 << 16;*/
    // end hacks

    uint32_t adr = Outrun_adr.sprite_coursemap;

    for (i = 0; i <= MAP_PIECES; i++)
    {
        oentry* sprite     = &OSprites_jump_table[i];
        sprite->id         = i+1;
        sprite->control    = RomLoader_read8IncP(Roms_rom0p, &adr);
        sprite->draw_props = RomLoader_read8IncP(Roms_rom0p, &adr);
        sprite->shadow     = RomLoader_read8IncP(Roms_rom0p, &adr);
        sprite->zoom       = RomLoader_read8IncP(Roms_rom0p, &adr);
        sprite->pal_src    = (uint8_t) RomLoader_read16IncP(Roms_rom0p, &adr);
        sprite->priority   = sprite->road_priority = RomLoader_read16IncP(Roms_rom0p, &adr);
        sprite->x          = RomLoader_read16IncP(Roms_rom0p, &adr);
        sprite->y          = RomLoader_read16IncP(Roms_rom0p, &adr);
        sprite->addr       = RomLoader_read32IncP(Roms_rom0p, &adr);
        sprite->counter    = 0;  
        
        adr += 4; // throw this address away

        OSprites_map_palette(sprite);
    }

   // Wide-screen hack to extend sea to edge of screen.
    if (Config_s16_x_off != 0 || Config_engine.fix_bugs)
    {
        for (i = 26; i <= 30; i++)
        {
            oentry* sprite = &OSprites_jump_table[i];
            sprite->addr   = OSprites_jump_table[31].addr;
            sprite->x      -= 64;
            sprite->zoom   = 0x7F;
        }
    }

    // Minicar initalization moved here
    minicar_enable = 0;
    OSprites_jump_table[MAP_SPRITE_FERRARI].x = -0x80;
    OSprites_jump_table[MAP_SPRITE_FERRARI].y = 0x78;
    map_state = MAP_INIT;
}

// Source: 0x355A
void OMap_do_route_final()
{
    int16_t pos = ORoad_road_pos >> 16;
    if (OInitEngine_rd_split_state)
        pos += 0x79C;   

    pos = (pos * 0x1B) / 0x94D;
    map_pos_final = pos;

    map_state = MAP_ROUTE_DONE;
    OMap_end_route();
}

// Source: 0x3584
void OMap_end_route()
{
    map_pos++;

    if (map_pos_final < map_pos)
    {
        // 359C
        map_pos = map_pos_final;
        minicar_enable = 1;
        map_state = MAP_INIT_DELAY;
        OMap_init_map_delay();
    }
}

// Source: 0x35B6
void OMap_init_map_delay()
{
    map_route = 0;
    map_delay = 0x80;
    map_state = MAP_DISPLAY;
    OMap_map_display();
}

// Source: 0x35CC
void OMap_map_display()
{
    // Init Best OutRunners
    if (--map_delay <= 0)
    {
        map_state = MAP_CLEAR;
        Outrun_init_best_outrunners();
    }
}

// ------------------------------------------------------------------------------------------------
// Colour sprite based road as car moves over it on mini-map
// ------------------------------------------------------------------------------------------------

// Source: 0x3740
void OMap_draw_vert_top(oentry* sprite)
{
    if (sprite->control & SPRITES_ENABLE)
        OMap_draw_piece(sprite, Outrun_adr.sprite_coursemap_top);
}

// Source: 0x3736
void OMap_draw_vert_bottom(oentry* sprite)
{
    if (sprite->control & SPRITES_ENABLE)
        OMap_draw_piece(sprite, Outrun_adr.sprite_coursemap_bot);
}

// Source: 0x372C
void OMap_draw_horiz_end(oentry* sprite)
{
    if (sprite->control & SPRITES_ENABLE)
        OMap_draw_piece(sprite, Outrun_adr.sprite_coursemap_end);
}

// Source: 0x3746
void OMap_draw_piece(oentry* sprite, uint32_t adr)
{
    // Update palette of background piece, to highlight route as minicar passes over it
    if (map_route == sprite->id)
    {
        sprite->priority = 0x102;
        sprite->road_priority = 0x102;

        adr += (map_pos << 3);

        sprite->addr    = RomLoader_read32(Roms_rom0p, adr);
        sprite->pal_src = RomLoader_read8(Roms_rom0p, 4 + adr);
        OSprites_map_palette(sprite);
    }

    OSprites_do_spr_order_shadows(sprite);
}

// Move mini car sprite on Course Map Screen
// Source: 0x3696
void OMap_move_mini_car(oentry* sprite)
{
    // Move Mini Car
    if (!minicar_enable)
    {
        // Remember that the minimap is angled, so we still need to adjust both the x and y positions
        uint32_t movement_table = (map_route & 1) ? MAP_MOVEMENT_RIGHT : MAP_MOVEMENT_LEFT;
        
        int16_t pos = (map_stage1 < 4) ? map_pos : map_pos >> 1;
        pos <<= 1; // do not try to merge with previous line

        sprite->x += RomLoader_read16(&Roms_rom0, movement_table + pos);
        int16_t y_change = RomLoader_read16(&Roms_rom0, movement_table + pos + 0x40);
        sprite->y -= y_change;

        if (y_change == 0)
            sprite->addr = Outrun_adr.sprite_minicar_right;
        else if (y_change < 0)
            sprite->addr = Outrun_adr.sprite_minicar_down;
        else
            sprite->addr = Outrun_adr.sprite_minicar_up;
    }

    OSprites_do_spr_order_shadows(sprite);
}
