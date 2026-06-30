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
#include "vidhrdw/generic.h"



data8_t *popeye_background_pos;
data8_t *popeye_palettebank;
data8_t *popeye_textram;

static struct osd_bitmap *tmpbitmap2;
static int invertmask;

#define BGRAM_SIZE 0x2000


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Popeye has four color PROMS:
  - 32x8 char palette
  - 32x8 background palette
  - two 256x4 sprite palette

  The char and sprite PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE (inverted)
        -- 470 ohm resistor  -- BLUE (inverted)
        -- 220 ohm resistor  -- GREEN (inverted)
        -- 470 ohm resistor  -- GREEN (inverted)
        -- 1  kohm resistor  -- GREEN (inverted)
        -- 220 ohm resistor  -- RED (inverted)
        -- 470 ohm resistor  -- RED (inverted)
  bit 0 -- 1  kohm resistor  -- RED (inverted)

  The background PROM is connected to the RGB output this way:

  bit 7 -- 470 ohm resistor  -- BLUE (inverted)
        -- 680 ohm resistor  -- BLUE (inverted)
        -- 470 ohm resistor  -- GREEN (inverted)
        -- 680 ohm resistor  -- GREEN (inverted)
        -- 1.2kohm resistor  -- GREEN (inverted)
        -- 470 ohm resistor  -- RED (inverted)
        -- 680 ohm resistor  -- RED (inverted)
  bit 0 -- 1.2kohm resistor  -- RED (inverted)

  The bootleg is the same, but the outputs are not inverted.

***************************************************************************/
static void convert_color_prom(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* palette entries 0-15 are directly used by the background and changed at runtime */
	palette += 3*16;
	color_prom += 32;

	/* characters */
	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = ((*color_prom ^ invertmask) >> 0) & 0x01;
		bit1 = ((*color_prom ^ invertmask) >> 1) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = ((*color_prom ^ invertmask) >> 3) & 0x01;
		bit1 = ((*color_prom ^ invertmask) >> 4) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = ((*color_prom ^ invertmask) >> 6) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	color_prom += 16;	/* skip unused part of the PROM */

	/* sprites */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = ((color_prom[0] ^ invertmask) >> 0) & 0x01;
		bit1 = ((color_prom[0] ^ invertmask) >> 1) & 0x01;
		bit2 = ((color_prom[0] ^ invertmask) >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = ((color_prom[0] ^ invertmask) >> 3) & 0x01;
		bit1 = ((color_prom[256] ^ invertmask) >> 0) & 0x01;
		bit2 = ((color_prom[256] ^ invertmask) >> 1) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = ((color_prom[256] ^ invertmask) >> 2) & 0x01;
		bit2 = ((color_prom[256] ^ invertmask) >> 3) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	/* palette entries 0-15 are directly used by the background */

	for (i = 0;i < 16;i++)	/* characters */
	{
		*(colortable++) = 0;	/* since chars are transparent, the PROM only */
								/* stores the non transparent color */
		*(colortable++) = i + 16;
	}
	for (i = 0;i < 256;i++)	/* sprites */
	{
		*(colortable++) = i + 16+16;
	}
}

static void popeyebl_vh_convert_color_prom(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom)
{
	invertmask = 0xff;

	convert_color_prom(palette,colortable,color_prom);
}

static void popeyebl_vh_convert_color_prom_bl(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom)
{
	invertmask = 0x00;

	convert_color_prom(palette,colortable,color_prom);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int popeye_vh_start(void)
{
	if ((tmpbitmap2 = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
	    return 1;
	}

	return 0;
}

void popeye_vh_stop(void)
{
	if (tmpbitmap2)
	    bitmap_free(tmpbitmap2);
}


WRITE_HANDLER( popeye_bitmap_w )
{
	int sx,sy,x,y,colour;

	sx = 8 * (offset % 64);
	sy = 4 * (offset / 64);

	colour = Machine->pens[data & 0x0f];
	for (y = 0; y < 4; y++)
	{
		for (x = 0; x < 8; x++)
		{
			plot_pixel(tmpbitmap2, sx+x, sy+y, colour);
		}
	}
}

WRITE_HANDLER( popeyebl_bitmap_w )
{
	offset = ((offset & 0xfc0) << 1) | (offset & 0x03f);
	if (data & 0x80)
		offset |= 0x40;

	popeye_bitmap_w(offset,data);
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void set_background_palette(int bank)
{
	int i;
	UINT8 *color_prom = memory_region(REGION_PROMS) + 16 * bank;

	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2;
		int r,g,b;

		/* red component */
		bit0 = ((*color_prom ^ invertmask) >> 0) & 0x01;
		bit1 = ((*color_prom ^ invertmask) >> 1) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 2) & 0x01;
		r = 0x1c * bit0 + 0x31 * bit1 + 0x47 * bit2;
		/* green component */
		bit0 = ((*color_prom ^ invertmask) >> 3) & 0x01;
		bit1 = ((*color_prom ^ invertmask) >> 4) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 5) & 0x01;
		g = 0x1c * bit0 + 0x31 * bit1 + 0x47 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = ((*color_prom ^ invertmask) >> 6) & 0x01;
		bit2 = ((*color_prom ^ invertmask) >> 7) & 0x01;
		b = 0x1c * bit0 + 0x31 * bit1 + 0x47 * bit2;

		//palette_set_color(i,r,g,b);
		palette_change_color(i,r,g,b);

		color_prom++;
	}
	palette_recalc();
}


static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int code,color;

		/*
		 * offs+3:
		 * bit 7 ?
		 * bit 6 ?
		 * bit 5 ?
		 * bit 4 MSB of sprite code
		 * bit 3 vertical flip
		 * bit 2 sprite bank
		 * bit 1 \ color (with bit 2 as well)
		 * bit 0 /
		 */

		code = (spriteram[offs + 2] & 0x7f) + ((spriteram[offs + 3] & 0x10) << 3)
							+ ((spriteram[offs + 3] & 0x04) << 6);
		color = (spriteram[offs + 3] & 0x07) + 8*(*popeye_palettebank & 0x07);

		if (spriteram[offs] != 0)
			drawgfx(bitmap,Machine->gfx[1],
					code ^ 0x1ff,
					color,
					spriteram[offs + 2] & 0x80,spriteram[offs + 3] & 0x08,
					2*(spriteram[offs])-8,2*(256-spriteram[offs + 1]),
					&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void popeye_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	set_background_palette((*popeye_palettebank & 0x08) >> 3);

	if (popeye_background_pos[0] == 0)	/* no background */
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	}
	else
	{
		/* copy the background graphics */

		int scrollx = 199 - popeye_background_pos[0];	/* ??? */
		int scrolly = 2 * (256 - popeye_background_pos[1]);
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	draw_sprites(bitmap);


	for (offs = 0;offs < 0x400;offs++)
	{
		int sx,sy;

		sx = 16 * (offs % 32);
		sy = 16 * (offs / 32);

		drawgfx(bitmap,Machine->gfx[0],
				popeye_textram[offs],
				popeye_textram[offs + 0x400],
				0,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

#ifdef __cplusplus
}
#endif
