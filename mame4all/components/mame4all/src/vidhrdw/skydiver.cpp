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

static int skydiver_lamps[8];
static int skydiver_width = 0;


READ_HANDLER( skydiver_wram_r )
{
	return videoram[offset | 0x380];
}

WRITE_HANDLER( skydiver_wram_w )
{
	videoram[offset | 0x0380] = data;
}


WRITE_HANDLER( skydiver_width_w )
{
	skydiver_width = offset;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void skydiver_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int pic;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int charcode;
			int color;
			int sx,sy;

			dirtybuffer[offs]=0;

			charcode = videoram[offs] & 0x3F;
			color    = (videoram[offs] & 0xc0) >> 6;

			sx = (offs & 31) << 3;
			sy = (offs >> 5) << 3;
			drawgfx(tmpbitmap,Machine->gfx[0],
				charcode, color,
				0,0,sx,sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Draw "SKYDIVER" lights? */
	{
		int light;

		for (light = 0; light < 8; light++)
		{
			char *text = "SKYDIVER";

			drawgfx(bitmap,Machine->gfx[0],
				text[light], skydiver_lamps[light] + 4,
				0,0,light*8,28*8,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw each one of our four motion objects */
	for (pic=3;pic>=0;pic--)
	{
		int sx,sy;
		int charcode;
		int xflip, yflip;
		int color;
		int wide;

		sx = 29*8 - videoram[pic + 0x0390];
		sy = 30*8 - videoram[pic*2 + 0x0398];
		charcode = videoram[pic*2 + 0x0399];
		xflip = charcode & 0x10;
		yflip = charcode & 0x08;
		wide = (~pic & 0x02) && skydiver_width;
		charcode = (charcode & 0x07) | ((charcode & 0x60) >> 2);
		color = pic & 0x01;


                if (wide)
		{
		    sx -= 8;
		}


		drawgfxzoom(bitmap,Machine->gfx[1],
			charcode, color,
			xflip,yflip,sx,sy,
			&Machine->visible_area,TRANSPARENCY_PEN, 0,
			wide ? 0x20000 : 0x10000, 0x10000);

	}
}



#ifdef __cplusplus
}
#endif
