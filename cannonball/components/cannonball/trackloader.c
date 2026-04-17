/***************************************************************************
    Track Loading Code.

    Abstracts the level format, so that the original ROMs as well as
    in conjunction with track data from the LayOut editor.

    - Handles levels (path, width, height, scenery)
    - Handles additional level sections (road split, end section)
    - Handles road/level related palettes
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "globals.h"
#include "trackloader.h"
#include <stdio.h>
#include <stdint.h>
#include "roms.h"
#include "engine/outrun.h"
#include "engine/oaddresses.h"

// ------------------------------------------------------------------------------------------------
// Stage Mapping Data: This is the master table that controls the order of the stages.
//
// You can change the stage order by editing this table.
// Bear in mind that the double lanes are hard coded in Stage 1.
//
// For USA there are unused tracks:
// 0x3A = Unused Coconut Beach
// 0x25 = Original Gateway track from Japanese edition
// 0x19 = Devils Canyon Variant
// ------------------------------------------------------------------------------------------------

static uint8_t STAGE_MAPPING_USA[] = 
{ 
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 1
    0x1E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 2
    0x20, 0x2F, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 3
    0x2D, 0x35, 0x33, 0x21, 0x00, 0x00, 0x00, 0x00,  // Stage 4
    0x32, 0x23, 0x38, 0x22, 0x26, 0x00, 0x00, 0x00,  // Stage 5
};

static uint8_t STAGE_MAPPING_JAP[] = 
{ 
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 1
    0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 2
    0x1E, 0x2F, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 3
    0x2D, 0x25, 0x33, 0x21, 0x00, 0x00, 0x00, 0x00,  // Stage 4
    0x32, 0x23, 0x38, 0x22, 0x26, 0x00, 0x00, 0x00,  // Stage 5
};


uint8_t* TrackLoader_stage_data;

Level* TrackLoader_current_level;

// Display start line on Stage 1
uint8_t TrackLoader_display_start_line;

uint32_t TrackLoader_curve_offset;
uint32_t TrackLoader_wh_offset;
uint32_t TrackLoader_scenery_offset;

// Shared Structures
uint8_t* TrackLoader_pal_sky_data;
uint8_t* TrackLoader_pal_gnd_data;
uint8_t* TrackLoader_heightmap_data;
uint8_t* TrackLoader_scenerymap_data;

uint32_t TrackLoader_pal_sky_offset;
uint32_t TrackLoader_pal_gnd_offset;
uint32_t TrackLoader_heightmap_offset;
uint32_t TrackLoader_scenerymap_offset;


RomLoader TrackLoader_layout;

int mode;

Level *TrackLoader_levels = NULL; // Normal Stages
Level *TrackLoader_levels_end = NULL;  // End Section
Level TrackLoader_level_split;    // Split Section

uint8_t* current_path; // CPU 1 Road Path

void TrackLoader_setup_level(Level* l, RomLoader* data, const int STAGE_ADR);
void TrackLoader_setup_section(Level* l, RomLoader* data, const int STAGE_ADR);

void TrackLoader_Create()
{
    if (!TrackLoader_levels) TrackLoader_levels = malloc(STAGES * sizeof(Level));
    if (!TrackLoader_levels_end) TrackLoader_levels_end = malloc(5 * sizeof(Level));
    TrackLoader_current_level = &TrackLoader_levels[0];

    mode       = MODE_ORIGINAL;
}

void TrackLoader_Destroy()
{
    if (TrackLoader_levels) free(TrackLoader_levels);
    if (TrackLoader_levels_end) free(TrackLoader_levels_end);
    TrackLoader_levels = NULL;
    TrackLoader_levels_end = NULL;
}
void TrackLoader_init(uint8_t jap)
{
    if (mode == MODE_ORIGINAL)
        TrackLoader_init_original_tracks(jap);
    else
        TrackLoader_init_layout_tracks(jap);
}

uint8_t TrackLoader_set_layout_track(const char* filename)
{
    RomLoader_create(&TrackLoader_layout);
    
    if (RomLoader_load_binary(&TrackLoader_layout, filename))
        return 0;

    mode = MODE_LAYOUT;

    return 1;
}

void TrackLoader_init_original_tracks(uint8_t jap)
{
    int i = 0;
    
    TrackLoader_stage_data = jap ? STAGE_MAPPING_JAP : STAGE_MAPPING_USA;

    TrackLoader_display_start_line = 1;

    // --------------------------------------------------------------------------------------------
    // Setup Shared Data
    // --------------------------------------------------------------------------------------------

    // Height Map Entries
    TrackLoader_heightmap_offset  = Outrun_adr.road_height_lookup;
    TrackLoader_heightmap_data    = &Roms_rom1p->rom[0];  

    // Scenery Map Entries
    TrackLoader_scenerymap_offset = Outrun_adr.sprite_master_table;
    TrackLoader_scenerymap_data   = &Roms_rom0p->rom[0]; 

    // Palette Entries
    TrackLoader_pal_sky_offset    = PAL_SKY_TABLE;
    TrackLoader_pal_sky_data      = &Roms_rom0.rom[0];

    TrackLoader_pal_gnd_offset    = PAL_GND_TABLE;
    TrackLoader_pal_gnd_data      = &Roms_rom0.rom[0];

    // --------------------------------------------------------------------------------------------
    // Iterate and setup 15 stages
    // --------------------------------------------------------------------------------------------

    static const uint32_t STAGE_ORDER[] = { 0, 
                                            0x8, 0x9, 
                                            0x10, 0x11, 0x12, 
                                            0x18, 0x19, 0x1A, 0x1B, 
                                            0x20, 0x21, 0x22, 0x23, 0x24};

    for (i = 0; i < STAGES; i++)
    {
        const uint16_t STAGE_OFFSET = TrackLoader_stage_data[STAGE_ORDER[i]] << 2;

        // CPU 0 Data
        const uint32_t STAGE_ADR = RomLoader_read32(Roms_rom0p, Outrun_adr.road_seg_table + STAGE_OFFSET);
        TrackLoader_setup_level(&TrackLoader_levels[i], Roms_rom0p, STAGE_ADR);

        // CPU 1 Data
        const uint32_t PATH_ADR = RomLoader_read32(Roms_rom1p, ROAD_DATA_LOOKUP + STAGE_OFFSET);
        TrackLoader_levels[i].path = &Roms_rom1p->rom[PATH_ADR];        
    }

    // --------------------------------------------------------------------------------------------
    // Setup End Sections & Split Stages
    // --------------------------------------------------------------------------------------------

    // Split stages don't contain palette information
    TrackLoader_setup_section(&TrackLoader_level_split, Roms_rom0p, Outrun_adr.road_seg_split);
    TrackLoader_level_split.path         = &Roms_rom1p->rom[ROAD_DATA_SPLIT];

    for (i = 0; i < 5; i++)
    {
        const uint32_t STAGE_ADR = RomLoader_read32(Roms_rom0p, Outrun_adr.road_seg_end + (i << 2));
        TrackLoader_setup_section(&TrackLoader_levels_end[i], Roms_rom0p, STAGE_ADR);
        TrackLoader_levels_end[i].path  = &Roms_rom1p->rom[ROAD_DATA_BONUS];
    }
}

void TrackLoader_init_layout_tracks(uint8_t jap)
{
    int i = 0;
    TrackLoader_stage_data = STAGE_MAPPING_USA;

    // --------------------------------------------------------------------------------------------
    // Check Version is Correct
    // --------------------------------------------------------------------------------------------
    if (RomLoader_read32(&TrackLoader_layout, LAYOUT_HEADER) != LAYOUT_EXPECTED_VERSION)
    {
		fprintf(stderr, "Incompatible LayOut Version Detected. Try upgrading CannonBall to the latest version.\n");
        TrackLoader_init_original_tracks(jap);
        return;
    }

    TrackLoader_display_start_line = RomLoader_read8(&TrackLoader_layout, (uint32_t)LAYOUT_HEADER + (uint32_t)sizeof(uint32_t));

    // --------------------------------------------------------------------------------------------
    // Setup Shared Data
    // --------------------------------------------------------------------------------------------

    // Height Map Entries
    TrackLoader_heightmap_offset  = RomLoader_read32(&TrackLoader_layout, LAYOUT_HEIGHT_MAPS);
    TrackLoader_heightmap_data    = &TrackLoader_layout.rom[0];  

    // Scenery Map Entries
    TrackLoader_scenerymap_offset = RomLoader_read32(&TrackLoader_layout, LAYOUT_SPRITE_MAPS);
    TrackLoader_scenerymap_data   = &TrackLoader_layout.rom[0]; 

    // Palette Entries
    TrackLoader_pal_sky_offset    = RomLoader_read32(&TrackLoader_layout, LAYOUT_PAL_SKY);
    TrackLoader_pal_sky_data      = &TrackLoader_layout.rom[0];

    TrackLoader_pal_gnd_offset    = RomLoader_read32(&TrackLoader_layout, LAYOUT_PAL_GND);
    TrackLoader_pal_gnd_data      = &TrackLoader_layout.rom[0];

    // --------------------------------------------------------------------------------------------
    // Iterate and setup 15 stages
    // --------------------------------------------------------------------------------------------
    for (i = 0; i < STAGES; i++)
    {
        // CPU 0 Data
        const uint32_t STAGE_ADR = RomLoader_read32(&TrackLoader_layout, LAYOUT_LEVELS + (i * sizeof(uint32_t)));
        TrackLoader_setup_level(&TrackLoader_levels[i], &TrackLoader_layout, STAGE_ADR);

        // CPU 1 Data
        const uint32_t PATH_ADR = RomLoader_read32(&TrackLoader_layout, LAYOUT_PATH);
        TrackLoader_levels[i].path = &TrackLoader_layout.rom[ PATH_ADR + ((ROAD_END_CPU1 * sizeof(uint32_t)) * i) ];
    }

    // --------------------------------------------------------------------------------------------
    // Setup End Sections & Split Stages
    // --------------------------------------------------------------------------------------------

    // Split stages don't contain palette information
    TrackLoader_setup_section(&TrackLoader_level_split, &TrackLoader_layout, RomLoader_read32(&TrackLoader_layout, LAYOUT_SPLIT_LEVEL));
    TrackLoader_level_split.path = &TrackLoader_layout.rom[ RomLoader_read32(&TrackLoader_layout, LAYOUT_SPLIT_PATH) ];

    // End sections don't contain palette information. Shared path.
    uint8_t* end_path = &TrackLoader_layout.rom[ RomLoader_read32(&TrackLoader_layout, LAYOUT_END_PATH) ];
    for (i = 0; i < 5; i++)
    {
        const uint32_t STAGE_ADR = RomLoader_read32(&TrackLoader_layout, LAYOUT_END_LEVELS + (i * sizeof(uint32_t)));
        TrackLoader_setup_section(&TrackLoader_levels_end[i], &TrackLoader_layout, STAGE_ADR);
        TrackLoader_levels_end[i].path = end_path;
    }
}

// Setup a normal level
void TrackLoader_setup_level(Level* l, RomLoader* data, const int STAGE_ADR)
{
    // Sky Palette
    uint32_t adr = RomLoader_read32(data, STAGE_ADR + 0);
    l->pal_sky   = RomLoader_read16(data, adr);

    // Load Road Pallete
    adr = RomLoader_read32(data, STAGE_ADR + 4);
    l->palr1.stripe_centre = RomLoader_read32IncP(data, &adr);
    l->palr2.stripe_centre = RomLoader_read32(data, adr);

    adr = RomLoader_read32(data, STAGE_ADR + 8);
    l->palr1.stripe = RomLoader_read32IncP(data, &adr);
    l->palr2.stripe = RomLoader_read32(data, adr);

    adr = RomLoader_read32(data, STAGE_ADR + 12);
    l->palr1.side = RomLoader_read32IncP(data, &adr);
    l->palr2.side = RomLoader_read32(data, adr);

    adr = RomLoader_read32(data, STAGE_ADR + 16);
    l->palr1.road = RomLoader_read32IncP(data, &adr);
    l->palr2.road = RomLoader_read32(data, adr);

    // Ground Palette
    adr = RomLoader_read32(data, STAGE_ADR + 20);
    l->pal_gnd = RomLoader_read16(data, adr);

    // Curve Data
    TrackLoader_curve_offset = RomLoader_read32(data, STAGE_ADR + 24);
    l->curve = &data->rom[TrackLoader_curve_offset];

    // Width / Height Lookup
    TrackLoader_wh_offset = RomLoader_read32(data, STAGE_ADR + 28);
    l->width_height = &data->rom[TrackLoader_wh_offset];

    // Sprite Information
    TrackLoader_scenery_offset = RomLoader_read32(data, STAGE_ADR + 32);
    l->scenery = &data->rom[TrackLoader_scenery_offset];
}

// Setup a special section of track (end section or level split)
// Special sections do not contain palette information
void TrackLoader_setup_section(Level* l, RomLoader* data, const int STAGE_ADR)
{
    // Curve Data
    TrackLoader_curve_offset = RomLoader_read32(data, STAGE_ADR + 0);
    l->curve = &data->rom[TrackLoader_curve_offset];

    // Width / Height Lookup
    TrackLoader_wh_offset = RomLoader_read32(data, STAGE_ADR + 4);
    l->width_height = &data->rom[TrackLoader_wh_offset];

    // Sprite Information
    TrackLoader_scenery_offset = RomLoader_read32(data, STAGE_ADR + 8);
    l->scenery = &data->rom[TrackLoader_scenery_offset];
}

// ------------------------------------------------------------------------------------------------
//                                 CPU 0: Track Data (Scenery, Width, Height)
// ------------------------------------------------------------------------------------------------

void TrackLoader_init_track(const uint32_t offset)
{
    TrackLoader_curve_offset   = 0;
    TrackLoader_wh_offset      = 0;
    TrackLoader_scenery_offset = 0;
    TrackLoader_current_level  = &TrackLoader_levels[TrackLoader_stage_offset_to_level(offset)];
}

int8_t TrackLoader_stage_offset_to_level(uint32_t id)
{
    static const uint8_t ID_OFFSET[] = {0, 1, 3, 6, 10};
    return (ID_OFFSET[id / 8]) + (id & 7);
}


void TrackLoader_init_track_split()
{
    TrackLoader_curve_offset   = 0;
    TrackLoader_wh_offset      = 0;
    TrackLoader_scenery_offset = 0;
    TrackLoader_current_level  = &TrackLoader_level_split;
}

void TrackLoader_init_track_bonus(const uint32_t id)
{
    TrackLoader_curve_offset   = 0;
    TrackLoader_wh_offset      = 0;
    TrackLoader_scenery_offset = 0;
    TrackLoader_current_level  = &TrackLoader_levels_end[id];
}

// ------------------------------------------------------------------------------------------------
//                                    CPU 1: Road Path Functions
// ------------------------------------------------------------------------------------------------

void TrackLoader_init_path(const uint32_t offset)
{
    current_path = TrackLoader_levels[TrackLoader_stage_offset_to_level(offset)].path;
}

void TrackLoader_init_path_split()
{
    current_path = TrackLoader_level_split.path;
}

void TrackLoader_init_path_end()
{
    current_path = TrackLoader_levels_end[0].path; // Path is shared for end sections
}

// ------------------------------------------------------------------------------------------------
//                                        HELPER FUNCTIONS TO READ DATA
// ------------------------------------------------------------------------------------------------

int16_t TrackLoader_readPath(uint32_t addr)
{
    return (current_path[addr] << 8) | current_path[addr+1];
}

int16_t TrackLoader_readPathIncP(uint32_t* addr)
{
    int16_t value = (current_path[*addr] << 8) | (current_path[*addr+1]);
    *addr += 2;
    return value;
}

int16_t TrackLoader_read_width_height(uint32_t* addr)
{
    int16_t value = (TrackLoader_current_level->width_height[*addr + TrackLoader_wh_offset] << 8) | (TrackLoader_current_level->width_height[*addr+1 + TrackLoader_wh_offset]);
    *addr += 2;
    return value;
}

int16_t TrackLoader_read_curve(uint32_t addr)
{
    return (TrackLoader_current_level->curve[addr + TrackLoader_curve_offset] << 8) | TrackLoader_current_level->curve[addr+1 + TrackLoader_curve_offset];
}

uint16_t TrackLoader_read_scenery_pos()
{
    return (TrackLoader_current_level->scenery[TrackLoader_scenery_offset] << 8) | TrackLoader_current_level->scenery[TrackLoader_scenery_offset + 1];
}

uint8_t TrackLoader_read_total_sprites()
{
    return TrackLoader_current_level->scenery[TrackLoader_scenery_offset + 2];
}

uint8_t TrackLoader_read_sprite_pattern_index()
{
    return TrackLoader_current_level->scenery[TrackLoader_scenery_offset + 3];
}

Level* TrackLoader_get_level(uint32_t id)
{
    return &TrackLoader_levels[TrackLoader_stage_offset_to_level(id)];
}

uint32_t TrackLoader_read_pal_sky_table(uint16_t entry)
{
    return TrackLoader_read32(TrackLoader_pal_sky_data, TrackLoader_pal_sky_offset + (entry * 4));
}

uint32_t TrackLoader_read_pal_gnd_table(uint16_t entry)
{
    return TrackLoader_read32(TrackLoader_pal_gnd_data, TrackLoader_pal_gnd_offset + (entry * 4));
}

uint32_t TrackLoader_read_heightmap_table(uint16_t entry)
{
    return TrackLoader_read32(TrackLoader_heightmap_data, TrackLoader_heightmap_offset + (entry * 4));
}

uint32_t TrackLoader_read_scenerymap_table(uint16_t entry)
{
    return TrackLoader_read32(TrackLoader_scenerymap_data, TrackLoader_scenerymap_offset + (entry * 4));
}

 int32_t TrackLoader_read32IncP(uint8_t* data, uint32_t* addr)
{    
    int32_t value = (data[*addr] << 24) | (data[*addr+1] << 16) | (data[*addr+2] << 8) | (data[*addr+3]);
    *addr += 4;
    return value;
}

 int16_t TrackLoader_read16IncP(uint8_t* data, uint32_t* addr)
{
    int16_t value = (data[*addr] << 8) | (data[*addr+1]);
    *addr += 2;
    return value;
}

 int8_t TrackLoader_read8IncP(uint8_t* data, uint32_t* addr)
{
    return data[(*addr)++]; 
}

 int32_t TrackLoader_read32(uint8_t* data, uint32_t addr)
{    
    return (data[addr] << 24) | (data[addr+1] << 16) | (data[addr+2] << 8) | data[addr+3];
}

 int16_t TrackLoader_read16(uint8_t* data, uint32_t addr)
{
    return (data[addr] << 8) | data[addr+1];
}

 int8_t TrackLoader_read8(uint8_t* data, uint32_t addr)
{
    return data[addr];
}
