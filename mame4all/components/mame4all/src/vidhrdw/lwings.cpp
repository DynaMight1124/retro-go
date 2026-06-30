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

unsigned char *lwings_fgvideoram;
unsigned char *lwings_bg1videoram;

static int bAvengersHardware, bg2_image;
static struct tilemap *fg_tilemap, *bg1_tilemap, *bg2_tilemap;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 get_bg2_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (row * 0x800) | (col * 2);
}

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = lwings_fgvideoram[tile_index];
	color = lwings_fgvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			0,
			code + ((color & 0xc0) << 2),
			color & 0x0f)
	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
}

static void lwings_get_bg1_tile_info(int tile_index)
{
	int code, color;

	code = lwings_bg1videoram[tile_index];
	color = lwings_bg1videoram[tile_index + 0x400];
	SET_TILE_INFO(
			1,
			code + ((color & 0xe0) << 3),
			color & 0x07)
	tile_info.flags = TILE_FLIPYX((color & 0x18) >> 3);
}

static void trojan_get_bg1_tile_info(int tile_index)
{
	const unsigned char avengers_color_remap[8]	= {6,7,4,5,2,3,0,1};
	int code, color;

	code = lwings_bg1videoram[tile_index];
	color = lwings_bg1videoram[tile_index + 0x400];
	code += (color & 0xe0)<<3;
	SET_TILE_INFO(
			1,
			code,
			bAvengersHardware ? avengers_color_remap[color & 7] : (color & 7))
	tile_info.flags = TILE_SPLIT((color & 0x08) >> 3) | ((color & 0x10) ? TILE_FLIPX : 0);
}

static void get_bg2_tile_info(int tile_index)
{
	int code, color;

	code = memory_region(REGION_GFX5)[bg2_image * 0x20 + tile_index];
	color = memory_region(REGION_GFX5)[bg2_image * 0x20 + tile_index + 1];
	SET_TILE_INFO(
			3,
			code + ((color & 0x80) << 1),
			color & 0x07)
	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int lwings_vh_start(void)
{
	fg_tilemap  = tilemap_create(get_fg_tile_info,        tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	bg1_tilemap = tilemap_create(lwings_get_bg1_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     16,16,32,32);

	if (!fg_tilemap || !bg1_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,3);

	return 0;
}

int trojan_vh_start(void)
{
	fg_tilemap  = tilemap_create(get_fg_tile_info,        tilemap_scan_rows,    TILEMAP_TRANSPARENT,8, 8,32,32);
	bg1_tilemap = tilemap_create(trojan_get_bg1_tile_info,tilemap_scan_cols,    TILEMAP_SPLIT,     16,16,32,32);
	bg2_tilemap = tilemap_create(get_bg2_tile_info,       get_bg2_memory_offset,TILEMAP_OPAQUE,    16,16,32,16);

	if( fg_tilemap && bg1_tilemap && bg2_tilemap )
	{
		tilemap_set_transparent_pen(fg_tilemap,3);
		tilemap_set_all_transmask(bg1_tilemap,0,0xffff,0x0001); /* split type 0 is totally transparent in front half */
		tilemap_set_all_transmask(bg1_tilemap,1,0xf07f,0x0f81); /* split type 1 has pens 7-11 opaque in front half */

		bAvengersHardware = 0;
		return 0;
	}
	return 1; /* error */
}

int avengers_vh_start( void )
{
	int result = trojan_vh_start();
	bAvengersHardware = 1;
	return result;
}

/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( lwings_fgvideoram_w )
{
	lwings_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( lwings_bg1videoram_w )
{
	lwings_bg1videoram[offset] = data;
	tilemap_mark_tile_dirty(bg1_tilemap,offset & 0x3ff);
}


WRITE_HANDLER( lwings_bg1_scrollx_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg1_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( lwings_bg1_scrolly_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrolly(bg1_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( lwings_bg2_scrollx_w )
{
	tilemap_set_scrollx(bg2_tilemap,0,data);
}

WRITE_HANDLER( lwings_bg2_image_w )
{
	if (bg2_image != data)
	{
		bg2_image = data;
		tilemap_mark_all_tiles_dirty(bg2_tilemap);
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

INLINE int is_sprite_on(int offs)
{
	int sx,sy;


	sx = buffered_spriteram[offs + 3] - 0x100 * (buffered_spriteram[offs + 1] & 0x01);
	sy = buffered_spriteram[offs + 2];

	return sx && sy;
}

static void lwings_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (is_sprite_on(offs))
		{
			int code,color,sx,sy,flipx,flipy;


			sx = buffered_spriteram[offs + 3] - 0x100 * (buffered_spriteram[offs + 1] & 0x01);
			sy = buffered_spriteram[offs + 2];
			code = buffered_spriteram[offs] | (buffered_spriteram[offs + 1] & 0xc0) << 2;
			color = (buffered_spriteram[offs + 1] & 0x38) >> 3;
			flipx = buffered_spriteram[offs + 1] & 0x02;
			flipy = buffered_spriteram[offs + 1] & 0x04;

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[2],
					code,color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}
}

static void trojan_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (is_sprite_on(offs))
		{
			int code,color,sx,sy,flipx,flipy;


			sx = buffered_spriteram[offs + 3] - 0x100 * (buffered_spriteram[offs + 1] & 0x01);
			sy = buffered_spriteram[offs + 2];
			code = buffered_spriteram[offs] |
				   ((buffered_spriteram[offs + 1] & 0x20) << 4) |
				   ((buffered_spriteram[offs + 1] & 0x40) << 2) |
				   ((buffered_spriteram[offs + 1] & 0x80) << 3);
			color = (buffered_spriteram[offs + 1] & 0x0e) >> 1;

			if( bAvengersHardware )
			{
				flipx = 0;										/* Avengers */
				flipy = ~buffered_spriteram[offs + 1] & 0x10;
			}
			else
			{
				flipx = buffered_spriteram[offs + 1] & 0x10;	/* Trojan */
				flipy = 1;
			}

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[2],
					code,color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}
}

static void lwings_mark_sprite_colors(void)
{
	int offs;
	unsigned char *sprite_colors;


	sprite_colors = palette_used_colors + Machine->drv->gfxdecodeinfo[2].color_codes_start;

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (is_sprite_on(offs))
		{
			int color;

			color = (buffered_spriteram[offs + 1] & 0x38) >> 3;
			memset(sprite_colors + color * 16, PALETTE_COLOR_USED, 15);
		}
	}
}

static void trojan_mark_sprite_colors(void)
{
	int offs;
	unsigned char *sprite_colors;


	sprite_colors = palette_used_colors + Machine->drv->gfxdecodeinfo[2].color_codes_start;

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (is_sprite_on(offs))
		{
			int color;

			color = (buffered_spriteram[offs + 1] & 0x0e) >> 1;
			memset(sprite_colors + color * 16, PALETTE_COLOR_USED, 15);
		}
	}
}

void lwings_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	lwings_mark_sprite_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg1_tilemap,0);
	lwings_draw_sprites(bitmap);
	tilemap_draw(bitmap,fg_tilemap,0);
}

void trojan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	trojan_mark_sprite_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg2_tilemap,0);
	tilemap_draw(bitmap,bg1_tilemap,TILEMAP_BACK);
	trojan_draw_sprites(bitmap);
	tilemap_draw(bitmap,bg1_tilemap,TILEMAP_FRONT);
	tilemap_draw(bitmap,fg_tilemap,0);
}

void lwings_eof_callback(void)
{
	buffer_spriteram_w(0,0);
}

#ifdef __cplusplus
}
#endif
