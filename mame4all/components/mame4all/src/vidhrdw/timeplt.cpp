#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

extern unsigned char *spriteram,*spriteram_2;
extern size_t spriteram_size;

unsigned char *timeplt_videoram,*timeplt_colorram;
static struct tilemap *bg_tilemap;


/*
 * sprites are multiplexed, so we have to buffer the spriteram
 * scanline by scanline.
 * */
static unsigned char *sprite_mux_buffer,*sprite_mux_buffer_2;
static int scanline;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Pilot has two 32x8 palette PROMs and two 256x4 lookup table PROMs
  (one for characters, one for sprites).
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 390 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 560 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 1.2kohm resistor  -- BLUE
        -- 390 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
  bit 0 -- 560 ohm resistor  -- GREEN

  bit 7 -- 820 ohm resistor  -- GREEN
        -- 1.2kohm resistor  -- GREEN
        -- 390 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 560 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
        -- 1.2kohm resistor  -- RED
  bit 0 -- not connected

***************************************************************************/
void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,bit4;


		bit0 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 4) & 0x01;
		bit4 = (color_prom[Machine->drv->total_colors] >> 5) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[Machine->drv->total_colors] >> 6) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 7) & 0x01;
		bit2 = (color_prom[0] >> 0) & 0x01;
		bit3 = (color_prom[0] >> 1) & 0x01;
		bit4 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[0] >> 4) & 0x01;
		bit2 = (color_prom[0] >> 5) & 0x01;
		bit3 = (color_prom[0] >> 6) & 0x01;
		bit4 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;

		color_prom++;
	}

	color_prom += Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* sprites */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = *(color_prom++) & 0x0f;

	/* characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = (*(color_prom++) & 0x0f) + 0x10;
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	unsigned char attr = timeplt_colorram[tile_index];
	SET_TILE_INFO(0,timeplt_videoram[tile_index] + ((attr & 0x20) << 3),attr & 0x1f)
	tile_info.flags = TILE_FLIPYX((attr & 0xc0) >> 6);
	tile_info.priority = (attr & 0x10) >> 4;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int timeplt_vh_start(void)
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	sprite_mux_buffer = (unsigned char *)malloc(256 * spriteram_size);
	sprite_mux_buffer_2 = (unsigned char *)malloc(256 * spriteram_size);

	if (!bg_tilemap || !sprite_mux_buffer || !sprite_mux_buffer_2)
	    return 1;

	return 0;
}

void timeplt_vh_stop (void)
{
    if (sprite_mux_buffer)
    {
	free(sprite_mux_buffer);
	sprite_mux_buffer = 0;
    }

    if (sprite_mux_buffer_2)
    {
	free (sprite_mux_buffer_2);
	sprite_mux_buffer_2 = 0;
    }
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( timeplt_videoram_w )
{
	if (timeplt_videoram[offset] != data)
	{
		timeplt_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE_HANDLER( timeplt_colorram_w )
{
	if (timeplt_colorram[offset] != data)
	{
		timeplt_colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE_HANDLER( timeplt_flipscreen_w )
{
	flip_screen_w(0,~data & 1);
}

/* Return the current video scan line */
READ_HANDLER( timeplt_scanline_r )
{
	return scanline;
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	struct rectangle *cliprect = &Machine->visible_area;
	const struct GfxElement *gfx = Machine->gfx[1];
	struct rectangle clip = *cliprect;
	int offs;
	int line;


	for (line = 0;line < 256;line++)
	{
		if (line >= cliprect->min_y && line <= cliprect->max_y)
		{
			unsigned char *sr,*sr2;

			sr = sprite_mux_buffer + line * spriteram_size;
			sr2 = sprite_mux_buffer_2 + line * spriteram_size;
			clip.min_y = clip.max_y = line;

			for (offs = spriteram_size - 2;offs >= 0;offs -= 2)
			{
				int code,color,sx,sy,flipx,flipy;

				sx = sr[offs];
				sy = 241 - sr2[offs + 1];

				if (sy > line-16 && sy <= line)
				{
					code = sr[offs + 1];
					color = sr2[offs] & 0x3f;
					flipx = ~sr2[offs] & 0x40;
					flipy = sr2[offs] & 0x80;

					drawgfx(bitmap,gfx,
							code,
							color,
							flipx,flipy,
							sx,sy,
							&clip,TRANSPARENCY_PEN,0);
















				}
			}
		}
	}
}


void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,bg_tilemap,1);
}


int timeplt_interrupt (void )
{
	scanline = 255 - cpu_getiloops();

	memcpy(sprite_mux_buffer + scanline * spriteram_size,spriteram,spriteram_size);
	memcpy(sprite_mux_buffer_2 + scanline * spriteram_size,spriteram_2,spriteram_size);

	if (scanline == 255)
		return nmi_interrupt();
	else
		return ignore_interrupt();
}

#ifdef __cplusplus
}
#endif
