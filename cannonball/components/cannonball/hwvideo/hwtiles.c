#include "globals.h"
#include "romloader.h"
#include "hwvideo/hwtiles.h"
#include "frontend/config.h"

#include <string.h>
#include <stdint.h>

/***************************************************************************
    Video Emulation: OutRun Tilemap Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

uint8_t HWTiles_text_ram[0x1000]; // Text RAM
uint8_t HWTiles_tile_ram[0x10000]; // Tile RAM

int16_t HWTiles_x_clamp;
    
// S16 Width, ignoring widescreen related scaling.
uint16_t HWTiles_s16_width_noscale;

#define TILES_LENGTH 0x10000
uint32_t *HWTiles_tiles = NULL;        // Converted tiles

uint16_t HWTiles_page[4];
uint16_t HWTiles_scroll_x[4];
uint16_t HWTiles_scroll_y[4];

uint8_t HWTiles_tile_banks[2] = { 0, 1 };

static const uint16_t NUM_TILES = 0x2000; // Length of graphic rom / 24
static const uint16_t TILEMAP_COLOUR_OFFSET = 0x1c00;
    
void (*HWTiles_render8x8_tile_mask)(
    uint16_t *buf,
    uint16_t nTileNumber, 
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
        
void (*HWTiles_render8x8_tile_mask_clip)(
    uint16_t *buf,
    uint16_t nTileNumber, 
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
        
void HWTiles_render8x8_tile_mask_lores(
    uint16_t *buf,
    uint16_t nTileNumber, 
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 

void HWTiles_render8x8_tile_mask_clip_lores(
    uint16_t *buf,
    uint16_t nTileNumber, 
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset);
        
void HWTiles_render8x8_tile_mask_hires(
    uint16_t *buf,
    uint16_t nTileNumber, 
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
        
void HWTiles_render8x8_tile_mask_clip_hires(
    uint16_t *buf,
    uint16_t nTileNumber, 
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset);
        
 void HWTiles_set_pixel_x4(uint16_t *buf, uint32_t data);

void HWTiles_Create(void)
{
    if (HWTiles_tiles == NULL) HWTiles_tiles = malloc(TILES_LENGTH * sizeof(uint32_t));

    uint8_t i;
    for (i = 0; i < 2; i++)
        HWTiles_tile_banks[i] = i;

    HWTiles_set_x_clamp(HWTILES_CENTRE);
}

void HWTiles_Destroy(void)
{
    if (HWTiles_tiles) free(HWTiles_tiles);
    HWTiles_tiles = NULL;
}

// Convert S16 tiles to a more useable format
void HWTiles_init(uint8_t* src_tiles, const uint8_t hires)
{
    uint32_t i;
    uint8_t ii;
    if (src_tiles)
    {
        for (i = 0; i < TILES_LENGTH; i++)
        {
            uint8_t p0 = src_tiles[i];
            uint8_t p1 = src_tiles[i + 0x10000];
            uint8_t p2 = src_tiles[i + 0x20000];

            uint32_t val = 0;

            for (ii = 0; ii < 8; ii++) 
            {
                uint8_t bit = 7 - ii;
                uint8_t pix = ((((p0 >> bit)) & 1) | (((p1 >> bit) << 1) & 2) | (((p2 >> bit) << 2) & 4));
                val = (val << 4) | pix;
            }
            HWTiles_tiles[i] = val; // Store converted value
        }
    }
    
    if (hires)
    {
        HWTiles_s16_width_noscale = Config_s16_width >> 1;
        HWTiles_render8x8_tile_mask      = &HWTiles_render8x8_tile_mask_hires;
        HWTiles_render8x8_tile_mask_clip = &HWTiles_render8x8_tile_mask_clip_hires;
    }
    else
    {
        HWTiles_s16_width_noscale = Config_s16_width;
        HWTiles_render8x8_tile_mask      = &HWTiles_render8x8_tile_mask_lores;
        HWTiles_render8x8_tile_mask_clip = &HWTiles_render8x8_tile_mask_clip_lores;
    }
}

void HWTiles_patch_tiles(RomLoader* patch)
{
    uint32_t i;

    for (i = 0; i < patch->length;)
    {
        uint32_t tile_index =         RomLoader_read16IncP(patch, &i) << 3;
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
    }
}

void HWTiles_restore_tiles()
{
}

void HWTiles_set_x_clamp(const uint16_t props)
{
    if (props == HWTILES_LEFT)
    {
        HWTiles_x_clamp = 192;
    }
    else if (props == HWTILES_RIGHT)
    {
        HWTiles_x_clamp = (512 - HWTiles_s16_width_noscale);
    }
    else if (props == HWTILES_CENTRE)
    {
        HWTiles_x_clamp = 192 - Config_s16_x_off;
    }
}

void HWTiles_update_tile_values()
{
    int i;
    for (i = 0; i < 4; i++)
    {
        HWTiles_page[i] = ((HWTiles_text_ram[0xe80 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe80 + (i * 2) + 1]);

        HWTiles_scroll_x[i] = ((HWTiles_text_ram[0xe98 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe98 + (i * 2) + 1]);
        HWTiles_scroll_y[i] = ((HWTiles_text_ram[0xe90 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe90 + (i * 2) + 1]);
    }
}

void HWTiles_render_all_tiles(uint16_t* buf)
{
    uint32_t Code = 0, Colour = 5, x, y;
    for (y = 0; y < 224; y += 8) 
    {
        for (x = 0; x < 320; x += 8) 
        {
            HWTiles_render8x8_tile_mask(buf, Code, x, y, Colour, 3, 0, TILEMAP_COLOUR_OFFSET);
            Code++;
        }
    }
}

void IRAM_ATTR HWTiles_render_tile_layer(uint16_t* buf, uint8_t page_index, uint8_t priority_draw)
{
    uint8_t my, mx;
    int16_t Colour, x, y, Priority = 0;

    uint16_t ActPage = 0;
    uint16_t EffPage = HWTiles_page[page_index];
    uint16_t xScroll = HWTiles_scroll_x[page_index];
    uint16_t yScroll = HWTiles_scroll_y[page_index];

    if ((xScroll & 0x8000) != 0)
        xScroll = (HWTiles_text_ram[0xf80 + (0x40 * page_index) + 0] << 8) | HWTiles_text_ram[0xf80 + (0x40 * page_index) + 1];
    if ((yScroll & 0x8000) != 0)
        yScroll = (HWTiles_text_ram[0xf16 + (0x40 * page_index) + 0] << 8) | HWTiles_text_ram[0xf16 + (0x40 * page_index) + 1];

    const int16_t xScrollOffset = (HWTiles_x_clamp - xScroll) & 0x3ff;
    const int16_t yScrollOffset = yScroll & 0x1ff;

    for (my = 0; my < 64; my++) 
    {
        y = 8 * my - yScrollOffset;
        if (y < -288)
            y += 512;

        // Only about 28 of the 64 tilemap rows can contribute to a 224-line
        // output. Reject the others before touching tile RAM in PSRAM.
        if (y <= -8 || y >= S16_HEIGHT)
            continue;

        for (mx = 0; mx < 128; mx++) 
        {
            x = 8 * mx - xScrollOffset;
            if (x < -HWTiles_x_clamp)
                x += 1024;

            // Likewise, only about 40 of the 128 columns are visible. This
            // early clip removes most tile lookup, bank and palette work.
            if (x <= -8 || x >= HWTiles_s16_width_noscale)
                continue;

            if (my < 32)
                ActPage = (mx < 64) ? (EffPage & 0x0f) : ((EffPage >> 4) & 0x0f);
            else
                ActPage = (mx < 64) ? ((EffPage >> 8) & 0x0f) : ((EffPage >> 12) & 0x0f);

            uint32_t TileIndex = 64 * 32 * 2 * ActPage + ((2 * 64 * my) & 0xfff) + ((2 * mx) & 0x7f);
            uint16_t Data = (HWTiles_tile_ram[TileIndex + 0] << 8) | HWTiles_tile_ram[TileIndex + 1];
            Priority = (Data >> 15) & 1;

            if (Priority == priority_draw) 
            {
                uint32_t Code = Data & 0x1fff;
                Code = HWTiles_tile_banks[Code / 0x1000] * 0x1000 + Code % 0x1000;
                Code &= (NUM_TILES - 1);
                if (Code == 0) continue;
                Colour = (Data >> 6) & 0x7f;

                uint16_t ColourOff = TILEMAP_COLOUR_OFFSET;
                if (Colour >= 0x20) ColourOff = 0x100 | TILEMAP_COLOUR_OFFSET;
                if (Colour >= 0x40) ColourOff = 0x200 | TILEMAP_COLOUR_OFFSET;
                if (Colour >= 0x60) ColourOff = 0x300 | TILEMAP_COLOUR_OFFSET;

                if (x > 7 && x < (HWTiles_s16_width_noscale - 8) && y > 7 && y <= (S16_HEIGHT - 8))
                    HWTiles_render8x8_tile_mask(buf, Code, x, y, Colour, 3, 0, ColourOff);
                else if (x > -8 && x < HWTiles_s16_width_noscale && y > -8 && y < S16_HEIGHT)
					HWTiles_render8x8_tile_mask_clip(buf, Code, x, y, Colour, 3, 0, ColourOff);
            }
        }
    }
}

void IRAM_ATTR HWTiles_render_text_layer(uint16_t* buf, uint8_t priority_draw)
{
    uint16_t mx, my, Code, Colour, x, y, Priority, TileIndex;

    // Text coordinates are fixed at x = 8*mx-192 and y = 8*my. Only columns
    // 24-63 and rows 0-27 can touch the 320x224 output; avoid scanning the
    // other 928 entries every frame.
    for (my = 0; my < 28; my++)
    {
        TileIndex = ((my * 64) + 24) * 2;
        for (mx = 24; mx < 64; mx++)
        {
            Code = (HWTiles_text_ram[TileIndex + 0] << 8) | HWTiles_text_ram[TileIndex + 1];
            Priority = (Code >> 15) & 1;

            if (Priority == priority_draw) 
            {
                Colour = (Code >> 9) & 0x07;
                Code &= 0x1ff;
                Code += HWTiles_tile_banks[0] * 0x1000;
                Code &= (NUM_TILES - 1);

                if (Code != 0) 
                {
                    x = 8 * mx;
                    y = 8 * my;
                    x -= 192;
                    if (x > 7 && x < (HWTiles_s16_width_noscale - 8) && y > 7 && y <= (S16_HEIGHT - 8))
                        HWTiles_render8x8_tile_mask(buf, Code, x + Config_s16_x_off, y, Colour, 3, 0, TILEMAP_COLOUR_OFFSET);
                    else if (x > -8 && x < HWTiles_s16_width_noscale && y >= 0 && y < S16_HEIGHT) 
                        HWTiles_render8x8_tile_mask_clip(buf, Code, x + Config_s16_x_off, y, Colour, 3, 0, TILEMAP_COLOUR_OFFSET);
                }
            }
            TileIndex += 2;
        }
    }
}

void IRAM_ATTR HWTiles_render8x8_tile_mask_lores(uint16_t *buf, uint16_t nTileNumber, uint16_t StartX, uint16_t StartY, uint16_t nTilePalette, uint16_t nColourDepth, uint16_t nMaskColour, uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = HWTiles_tiles + (nTileNumber << 3);
    buf += (StartY * Config_s16_width) + StartX;

    for (y = 0; y < 8; y++) 
    {
        uint32_t p0 = *pTileData;
        if (p0 != nMaskColour) 
        {
            uint32_t c7 = p0 & 0xf;
            uint32_t c6 = (p0 >> 4) & 0xf;
            uint32_t c5 = (p0 >> 8) & 0xf;
            uint32_t c4 = (p0 >> 12) & 0xf;
            uint32_t c3 = (p0 >> 16) & 0xf;
            uint32_t c2 = (p0 >> 20) & 0xf;
            uint32_t c1 = (p0 >> 24) & 0xf;
            uint32_t c0 = (p0 >> 28);
            if (c0) buf[0] = Video_output_color(nPalette + c0);
            if (c1) buf[1] = Video_output_color(nPalette + c1);
            if (c2) buf[2] = Video_output_color(nPalette + c2);
            if (c3) buf[3] = Video_output_color(nPalette + c3);
            if (c4) buf[4] = Video_output_color(nPalette + c4);
            if (c5) buf[5] = Video_output_color(nPalette + c5);
            if (c6) buf[6] = Video_output_color(nPalette + c6);
            if (c7) buf[7] = Video_output_color(nPalette + c7);
        }
        buf += Config_s16_width;
        pTileData++;
    }
}

void IRAM_ATTR HWTiles_render8x8_tile_mask_clip_lores(uint16_t *buf, uint16_t nTileNumber, int16_t StartX, int16_t StartY, uint16_t nTilePalette, uint16_t nColourDepth, uint16_t nMaskColour, uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = HWTiles_tiles + (nTileNumber << 3);
    buf += (StartY * Config_s16_width) + StartX;

    for (y = 0; y < 8; y++) 
    {
        if ((StartY + y) >= 0 && (StartY + y) < S16_HEIGHT) 
        {
            uint32_t p0 = *pTileData;
            if (p0 != nMaskColour) 
            {
                uint32_t c7 = p0 & 0xf;
                uint32_t c6 = (p0 >> 4) & 0xf;
                uint32_t c5 = (p0 >> 8) & 0xf;
                uint32_t c4 = (p0 >> 12) & 0xf;
                uint32_t c3 = (p0 >> 16) & 0xf;
                uint32_t c2 = (p0 >> 20) & 0xf;
                uint32_t c1 = (p0 >> 24) & 0xf;
                uint32_t c0 = (p0 >> 28);
                if (c0 && 0 + StartX >= 0 && 0 + StartX < Config_s16_width) buf[0] = Video_output_color(nPalette + c0);
                if (c1 && 1 + StartX >= 0 && 1 + StartX < Config_s16_width) buf[1] = Video_output_color(nPalette + c1);
                if (c2 && 2 + StartX >= 0 && 2 + StartX < Config_s16_width) buf[2] = Video_output_color(nPalette + c2);
                if (c3 && 3 + StartX >= 0 && 3 + StartX < Config_s16_width) buf[3] = Video_output_color(nPalette + c3);
                if (c4 && 4 + StartX >= 0 && 4 + StartX < Config_s16_width) buf[4] = Video_output_color(nPalette + c4);
                if (c5 && 5 + StartX >= 0 && 5 + StartX < Config_s16_width) buf[5] = Video_output_color(nPalette + c5);
                if (c6 && 6 + StartX >= 0 && 6 + StartX < Config_s16_width) buf[6] = Video_output_color(nPalette + c6);
                if (c7 && 7 + StartX >= 0 && 7 + StartX < Config_s16_width) buf[7] = Video_output_color(nPalette + c7);
            }
        }
        buf += Config_s16_width;
        pTileData++;
    }
}

void HWTiles_render8x8_tile_mask_hires(uint16_t *buf, uint16_t nTileNumber, uint16_t StartX, uint16_t StartY, uint16_t nTilePalette, uint16_t nColourDepth, uint16_t nMaskColour, uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = HWTiles_tiles + (nTileNumber << 3);
    buf += ((StartY << 1) * Config_s16_width) + (StartX << 1);
    
    for (y = 0; y < 8; y++) 
    {
        uint32_t p0 = *pTileData;
        if (p0 != nMaskColour) 
        {
            uint32_t c7 = p0 & 0xf;
            uint32_t c6 = (p0 >> 4) & 0xf;
            uint32_t c5 = (p0 >> 8) & 0xf;
            uint32_t c4 = (p0 >> 12) & 0xf;
            uint32_t c3 = (p0 >> 16) & 0xf;
            uint32_t c2 = (p0 >> 20) & 0xf;
            uint32_t c1 = (p0 >> 24) & 0xf;
            uint32_t c0 = (p0 >> 28);
            if (c0) HWTiles_set_pixel_x4(&buf[0],  Video_output_color(nPalette + c0));
            if (c1) HWTiles_set_pixel_x4(&buf[2],  Video_output_color(nPalette + c1));
            if (c2) HWTiles_set_pixel_x4(&buf[4],  Video_output_color(nPalette + c2));
            if (c3) HWTiles_set_pixel_x4(&buf[6],  Video_output_color(nPalette + c3));
            if (c4) HWTiles_set_pixel_x4(&buf[8],  Video_output_color(nPalette + c4));
            if (c5) HWTiles_set_pixel_x4(&buf[10], Video_output_color(nPalette + c5));
            if (c6) HWTiles_set_pixel_x4(&buf[12], Video_output_color(nPalette + c6));
            if (c7) HWTiles_set_pixel_x4(&buf[14], Video_output_color(nPalette + c7));
        }
        buf += (Config_s16_width << 1);
        pTileData++;
    }
}

void HWTiles_render8x8_tile_mask_clip_hires(uint16_t *buf, uint16_t nTileNumber, int16_t StartX, int16_t StartY, uint16_t nTilePalette, uint16_t nColourDepth, uint16_t nMaskColour, uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = HWTiles_tiles + (nTileNumber << 3);
    buf += ((StartY << 1) * Config_s16_width) + (StartX << 1);

    for (y = 0; y < 8; y++) 
    {
        if ((StartY + y) >= 0 && (StartY + y) < S16_HEIGHT) 
        {
            uint32_t p0 = *pTileData;
            if (p0 != nMaskColour) 
            {
                uint32_t c7 = p0 & 0xf;
                uint32_t c6 = (p0 >> 4) & 0xf;
                uint32_t c5 = (p0 >> 8) & 0xf;
                uint32_t c4 = (p0 >> 12) & 0xf;
                uint32_t c3 = (p0 >> 16) & 0xf;
                uint32_t c2 = (p0 >> 20) & 0xf;
                uint32_t c1 = (p0 >> 24) & 0xf;
                uint32_t c0 = (p0 >> 28);
                if (c0 && 0 + StartX >= 0 && 0 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[0],  Video_output_color(nPalette + c0));
                if (c1 && 1 + StartX >= 0 && 1 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[2],  Video_output_color(nPalette + c1));
                if (c2 && 2 + StartX >= 0 && 2 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[4],  Video_output_color(nPalette + c2));
                if (c3 && 3 + StartX >= 0 && 3 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[6],  Video_output_color(nPalette + c3));
                if (c4 && 4 + StartX >= 0 && 4 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[8],  Video_output_color(nPalette + c4));
                if (c5 && 5 + StartX >= 0 && 5 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[10], Video_output_color(nPalette + c5));
                if (c6 && 6 + StartX >= 0 && 6 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[12], Video_output_color(nPalette + c6));
                if (c7 && 7 + StartX >= 0 && 7 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[14], Video_output_color(nPalette + c7));
            }
        }
        buf += (Config_s16_width << 1);
        pTileData++;
    }
}

void HWTiles_set_pixel_x4(uint16_t *buf, uint32_t data)
{
    buf[0] = buf[1] = buf[0  + Config_s16_width] = buf[1 + Config_s16_width] = data;
}
