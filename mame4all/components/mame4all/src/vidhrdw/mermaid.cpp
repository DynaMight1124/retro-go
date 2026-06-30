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
#include "tilemap.h"
#include "vidhrdw/generic.h"


unsigned char* mermaid_background_videoram;
unsigned char* mermaid_foreground_videoram;
unsigned char* mermaid_foreground_colorram;
unsigned char* mermaid_background_scrollram;
unsigned char* mermaid_foreground_scrollram;

static struct osd_bitmap *helper;
static struct osd_bitmap *helper2;
static struct tilemap *bg_tilemap, *fg_tilemap;

static int coll_bit0,coll_bit1,coll_bit2,coll_bit3,coll_bit6 = 0;
static int mermaid_flipscreen_x, mermaid_flipscreen_y;
static int rougien_gfxbank1, rougien_gfxbank2;


static struct rectangle spritevisiblearea =
{
	0*8, 26*8-1,
	2*8, 30*8-1
};

WRITE_HANDLER( mermaid_background_videoram_w )
{
	mermaid_background_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

WRITE_HANDLER( mermaid_foreground_videoram_w )
{
	mermaid_foreground_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}

WRITE_HANDLER( mermaid_foreground_colorram_w )
{
	mermaid_foreground_colorram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}

WRITE_HANDLER( mermaid_flip_screen_x_w )
{
	mermaid_flipscreen_x = data & 0x01;
}

WRITE_HANDLER( mermaid_flip_screen_y_w )
{
	mermaid_flipscreen_y = data & 0x01;
}

WRITE_HANDLER( mermaid_bg_scroll_w )
{
	mermaid_background_scrollram[offset] = data;
	tilemap_set_scrolly(bg_tilemap, offset, data);
}

WRITE_HANDLER( mermaid_fg_scroll_w )
{
	mermaid_foreground_scrollram[offset] = data;
	tilemap_set_scrolly(fg_tilemap, offset, data);
}

WRITE_HANDLER( rougien_gfxbankswitch1_w )
{
	rougien_gfxbank1 = data & 0x01;
}

WRITE_HANDLER( rougien_gfxbankswitch2_w )
{
	rougien_gfxbank2 = data & 0x01;
}

READ_HANDLER( mermaid_collision_r )
{
	/*
        collision register active LOW:

	with coll = spriteram[offs + 2] & 0xc0

        Bit 0 - Sprite (coll = 0x40) - Sprite (coll = 0x00)
        Bit 1 - Sprite (coll = 0x40) - Foreground
        Bit 2 - Sprite (coll = 0x40) - Background
        Bit 3 - Sprite (coll = 0x80) - Sprite (coll = 0x00)
        Bit 4
        Bit 5
        Bit 6 - Sprite (coll = 0x40) - Sprite (coll = 0x80)
        Bit 7
	*/

	int collision = 0xff;

	if(coll_bit0) collision &= 0xfe;
	if(coll_bit1) collision &= 0xfd;
	if(coll_bit2) collision &= 0xfb;
	if(coll_bit3) collision &= 0xf7;
	if(coll_bit6) collision &= 0xbf;

	return collision;
}
/***************************************************************************

  Convert the color PROMs into a more useable format.

  I'm not sure about the resistor value, I'm using the Galaxian ones.

***************************************************************************/
void mermaid_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	int i;

	/* first, the char acter/sprite palette */
	for (i = 0;i < TOTAL_COLORS(0); i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	/* blue background */
	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0xff;


	/* set up background palette */
    COLOR(2,0) = 32;
    COLOR(2,1) = 33;

    COLOR(2,2) = 64;
    COLOR(2,3) = 33;
}

void rougien_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	int i;

	/* first, the char acter/sprite palette */
	for (i = 0;i < TOTAL_COLORS(0); i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	/* black background */
	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0;


	/* set up background palette */
	COLOR(2,0) = 0x40;
	COLOR(2,1) = 0x00;

	COLOR(2,2) = 0x00;
	COLOR(2,3) = 0x02;
}

static void draw_sprites(struct osd_bitmap *dest_bitmap)
{
	int offs;

	/* draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy,code,bank,attr,color;

		attr = spriteram[offs + 2];
		color = attr & 0x0f;
		bank = (attr & 0x30) >> 4;
		code = (spriteram[offs] & 0x3f) | (bank << 6);
		code |= rougien_gfxbank1 * 0x2800;
		code |= rougien_gfxbank2 * 0x2400;

		sx = spriteram[offs + 3] + 1;
		if (sx >= 0xf0) sx -= 256;
		sy = 240 - spriteram[offs + 1];
		flipx = spriteram[offs + 0] & 0x40;
		flipy = spriteram[offs + 0] & 0x80;

		drawgfx(dest_bitmap,Machine->gfx[1],
				code,
				color,
				flipx, flipy,
				sx, sy,
				&spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}



static void get_bg_tile_info (int tile_index)
{
	int code = mermaid_background_videoram[tile_index];
	int sx = tile_index % 32;
	int color = (sx >= 26) ? 0 : 1;

	SET_TILE_INFO(2, code, color);
}

static void get_fg_tile_info (int tile_index)
{
	int attr = mermaid_foreground_colorram[tile_index];
	int code = mermaid_foreground_videoram[tile_index] + ((attr & 0x30) << 4);
	int color = attr & 0x0f;

	code |= rougien_gfxbank1 * 0x2800;
	code |= rougien_gfxbank2 * 0x2400;

	SET_TILE_INFO(0, code, color);
	tile_info.flags = TILE_FLIPYX((attr & 0xc0) >> 6);
}

static unsigned char mermaid_colcheck(int bit, const struct rectangle* rect)
{
	unsigned char data = 0;

	int x;
	int y;

	for (y = rect->min_y; y <= rect->max_y; y++)
	{
		for (x = rect->min_x; x <= rect->max_x; x++)
		{
			unsigned int pa = read_pixel(helper, x, y);
			unsigned int pb = read_pixel(helper2, x, y);

			//a[x- rect->min_x] =  pa;
			//b[x- rect->min_x] =  pb;

			if (pb != 0)
				if ((pa != 0) & (pa != 0x40))
					data |= 0x01;
		}

	}

	return data;
}

void mermaid_vh_eof(void)
{
	const struct rectangle *visarea = &Machine->visible_area;

	int offs,offs2;

	coll_bit1 = 0;
	coll_bit2 = 0;
	coll_bit3 = 0;
	coll_bit6 = 0;
	coll_bit0 = 0;

	// check for bit 0 (sprite-sprite), 1 (sprite-foreground), 2 (sprite-background)

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{
		int attr = spriteram[offs + 2];
		int bank = (attr & 0x30) >> 4;
		int coll = (attr & 0xc0) >> 6;
		int code = (spriteram[offs] & 0x3f) | (bank << 6);
		int flipx = spriteram[offs] & 0x40;
		int flipy = spriteram[offs] & 0x80;
		int sx = spriteram[offs + 3] + 1;
		int sy = 240 - spriteram[offs + 1];

		struct rectangle rect;

		if (coll != 1) continue;

		code |= rougien_gfxbank1 * 0x2800;
		code |= rougien_gfxbank2 * 0x2400;

#if 0
		if (mermaid_flipscreen_x)
		{
			flipx = !flipx;
			sx = 240 - sx;
		}

		if (mermaid_flipscreen_y)
		{
			flipy = !flipy;
			sy = 240 - sy;
		}
#endif

		rect.min_x = sx;
		rect.min_y = sy;
		rect.max_x = sx + Machine->gfx[1]->width - 1;
		rect.max_y = sy + Machine->gfx[1]->height - 1;

		if (rect.min_x < visarea->min_x)
			rect.min_x = visarea->min_x;
		if (rect.min_y < visarea->min_y)
			rect.min_y = visarea->min_y;
		if (rect.max_x > visarea->max_x)
			rect.max_x = visarea->max_x;
		if (rect.max_y > visarea->max_y)
			rect.max_y = visarea->max_y;

		tilemap_set_clip(bg_tilemap, &rect);
		tilemap_set_clip(fg_tilemap, &rect);

		// check collision sprite - background

		fillbitmap(helper,0,&rect);
		fillbitmap(helper2,0,&rect);

		tilemap_draw(helper, bg_tilemap, 0);

		drawgfx(helper2, Machine->gfx[1], code, 0, flipx, flipy, sx, sy,
			&rect,TRANSPARENCY_PEN, 0);

		coll_bit2 |= mermaid_colcheck(2, &rect);

		// check collision sprite - foreground

		fillbitmap(helper,0,&rect);
		fillbitmap(helper2,0,&rect);

		tilemap_draw(helper, fg_tilemap, 0);

		drawgfx(helper2, Machine->gfx[1], code, 0, flipx, flipy, sx, sy,
			&rect,TRANSPARENCY_PEN, 0);

		coll_bit1 |= mermaid_colcheck(1, &rect);

		// check collision sprite - sprite

		fillbitmap(helper,0,&rect);
		fillbitmap(helper2,0,&rect);

		for (offs2 = spriteram_size - 4; offs2 >= 0; offs2 -= 4)
			if (offs != offs2)
			{
				int attr2 = spriteram[offs2 + 2];
				int bank2 = (attr2 & 0x30) >> 4;
				int coll2 = (attr2 & 0xc0) >> 6;
				int code2 = (spriteram[offs2] & 0x3f) | (bank2 << 6);
				int flipx2 = spriteram[offs2] & 0x40;
				int flipy2 = spriteram[offs2] & 0x80;
				int sx2 = spriteram[offs2 + 3] + 1;
				int sy2 = 240 - spriteram[offs2 + 1];

				if (coll2 != 0) continue;

				code2 |= rougien_gfxbank1 * 0x2800;
				code2 |= rougien_gfxbank2 * 0x2400;

#if 0
				if (mermaid_flip_screen_x)
				{
					flipx2 = !flipx2;
					sx2 = 240 - sx2;
				}

				if (mermaid_flip_screen_y)
				{
					flipy2 = !flipy2;
					sy2 = 240 - sy2;
				}
#endif

				drawgfx(helper, Machine->gfx[1], code2, 0, flipx2, flipy2, sx2, sy2,
					&rect,TRANSPARENCY_PEN, 0);
			}

		drawgfx(helper2, Machine->gfx[1], code, 0, flipx, flipy, sx, sy,
				&rect,TRANSPARENCY_PEN, 0);

		coll_bit0 |= mermaid_colcheck(0, &rect);
	}

	// check for bit 3 (sprite-sprite)

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{
		int attr = spriteram[offs + 2];
		int bank = (attr & 0x30) >> 4;
		int coll = (attr & 0xc0) >> 6;
		int code = (spriteram[offs] & 0x3f) | (bank << 6);
		int flipx = spriteram[offs] & 0x40;
		int flipy = spriteram[offs] & 0x80;
		int sx = spriteram[offs + 3] + 1;
		int sy = 240 - spriteram[offs + 1];

		struct rectangle rect;

		if (coll != 2) continue;

		code |= rougien_gfxbank1 * 0x2800;
		code |= rougien_gfxbank2 * 0x2400;

#if 0
		if (mermaid_flipscreen_x)
		{
			flipx = !flipx;
			sx = 240 - sx;
		}

		if (mermaid_flipscreen_y)
		{
			flipy = !flipy;
			sy = 240 - sy;
		}
#endif
		rect.min_x = sx;
		rect.min_y = sy;
		rect.max_x = sx + Machine->gfx[1]->width - 1;
		rect.max_y = sy + Machine->gfx[1]->height - 1;

		if (rect.min_x < visarea->min_x)
			rect.min_x = visarea->min_x;
		if (rect.min_y < visarea->min_y)
			rect.min_y = visarea->min_y;
		if (rect.max_x > visarea->max_x)
			rect.max_x = visarea->max_x;
		if (rect.max_y > visarea->max_y)
			rect.max_y = visarea->max_y;

		// check collision sprite - sprite

		fillbitmap(helper,0,&rect);
		fillbitmap(helper2,0,&rect);

		for (offs2 = spriteram_size - 4; offs2 >= 0; offs2 -= 4)
			if (offs != offs2)
			{
				int attr2 = spriteram[offs2 + 2];
				int bank2 = (attr2 & 0x30) >> 4;
				int coll2 = (attr2 & 0xc0) >> 6;
				int code2 = (spriteram[offs2] & 0x3f) | (bank2 << 6);
				int flipx2 = spriteram[offs2] & 0x40;
				int flipy2 = spriteram[offs2] & 0x80;
				int sx2 = spriteram[offs2 + 3] + 1;
				int sy2 = 240 - spriteram[offs2 + 1];

				if (coll2 != 0) continue;

				code2 |= rougien_gfxbank1 * 0x2800;
				code2 |= rougien_gfxbank2 * 0x2400;

#if 0
				if (mermaid_flipscreen_x)
				{
					flipx = !flipx;
					sx = 240 - sx;
				}

				if (mermaid_flipscreen_y)
				{
					flipy = !flipy;
					sy = 240 - sy;
				}
#endif

				drawgfx(helper, Machine->gfx[1], code2, 0, flipx2, flipy2, sx2, sy2,
					&rect,TRANSPARENCY_PEN, 0);
			}

		drawgfx(helper2, Machine->gfx[1], code, 0, flipx, flipy, sx, sy,
				&rect,TRANSPARENCY_PEN, 0);

		coll_bit3 |= mermaid_colcheck(3, &rect);
	}

	// check for bit 6

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{
		int attr = spriteram[offs + 2];
		int bank = (attr & 0x30) >> 4;
		int coll = (attr & 0xc0) >> 6;
		int code = (spriteram[offs] & 0x3f) | (bank << 6);
		int flipx = spriteram[offs] & 0x40;
		int flipy = spriteram[offs] & 0x80;
		int sx = spriteram[offs + 3] + 1;
		int sy = 240 - spriteram[offs + 1];

		struct rectangle rect;

		if (coll != 1) continue;

		code |= rougien_gfxbank1 * 0x2800;
		code |= rougien_gfxbank2 * 0x2400;

#if 0
		if (mermaid_flipscreen_x)
		{
			flipx = !flipx;
			sx = 240 - sx;
		}

		if (mermaid_flipscreen_y)
		{
			flipy = !flipy;
			sy = 240 - sy;
		}
#endif
		rect.min_x = sx;
		rect.min_y = sy;
		rect.max_x = sx + Machine->gfx[1]->width - 1;
		rect.max_y = sy + Machine->gfx[1]->height - 1;

		if (rect.min_x < visarea->min_x)
			rect.min_x = visarea->min_x;
		if (rect.min_y < visarea->min_y)
			rect.min_y = visarea->min_y;
		if (rect.max_x > visarea->max_x)
			rect.max_x = visarea->max_x;
		if (rect.max_y > visarea->max_y)
			rect.max_y = visarea->max_y;

		// check collision sprite - sprite

		fillbitmap(helper,0,&rect);
		fillbitmap(helper2,0,&rect);

		for (offs2 = spriteram_size - 4; offs2 >= 0; offs2 -= 4)
			if (offs != offs2)
			{
				int attr2 = spriteram[offs2 + 2];
				int bank2 = (attr2 & 0x30) >> 4;
				int coll2 = (attr2 & 0xc0) >> 6;
				int code2 = (spriteram[offs2] & 0x3f) | (bank2 << 6);
				int flipx2 = spriteram[offs2] & 0x40;
				int flipy2 = spriteram[offs2] & 0x80;
				int sx2 = spriteram[offs2 + 3] + 1;
				int sy2 = 240 - spriteram[offs2 + 1];

				if (coll2 != 2) continue;

				code2 |= rougien_gfxbank1 * 0x2800;
				code2 |= rougien_gfxbank2 * 0x2400;

#if 0
				if (mermaid_flipscreen_x)
				{
					flipx = !flipx;
					sx = 240 - sx;
				}

				if (mermaid_flipscreen_y)
				{
					flipy = !flipy;
					sy = 240 - sy;
				}
#endif

				drawgfx(helper, Machine->gfx[1], code2, 0, flipx2, flipy2, sx2, sy2,
					&rect,TRANSPARENCY_PEN, 0);
			}

		drawgfx(helper2, Machine->gfx[1], code, 0, flipx, flipy, sx, sy,
				&rect,TRANSPARENCY_PEN, 0);

		coll_bit6 |= mermaid_colcheck(6, &rect);
	}
}

int mermaid_vh_start(void)
{
	helper = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!helper)
	    return 1;

	helper2 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	if (!helper2)
	{
	    free (helper);
	    helper = 0;
	    return 1;
	}

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	tilemap_set_scroll_cols(bg_tilemap, 32);

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	tilemap_set_scroll_cols(fg_tilemap, 32);
	tilemap_set_transparent_pen(fg_tilemap, 0);

	if (bg_tilemap && fg_tilemap)
	    return 0;
	else
	    return 1;
}

void mermaid_vh_stop(void)
{
	if (helper)
	{
	    free (helper);
	    helper = 0;
	}

	if (helper2)
	{
	    free (helper2);
	    helper2 = 0;
	}
}

void mermaid_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_clip(bg_tilemap, &Machine->visible_area);
	tilemap_set_clip(fg_tilemap, &Machine->visible_area);

	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap, bg_tilemap, 0);
	tilemap_draw(bitmap, fg_tilemap, 0);
	draw_sprites(bitmap);
}

#ifdef __cplusplus
}
#endif
