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

  aeroboto (preliminary)

***************************************************************************/
 
#include "driver.h"
#include "vidhrdw/generic.h"

data8_t *aeroboto_hscroll, *aeroboto_vscroll, *aeroboto_tilecolor;
data8_t *aeroboto_starx, *aeroboto_stary, *aeroboto_starcolor;

static int aeroboto_charbank;

static struct tilemap *bg_tilemap;

/***************************************************************************

  Memory handlers

***************************************************************************/

void aeroboto_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* first, the character/sprite palette */
	for (i = 0;i < Machine->drv->total_colors-1; i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}
}

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	unsigned char code = videoram[tile_index];
	SET_TILE_INFO(0,code + (aeroboto_charbank << 8),aeroboto_tilecolor[code]);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int aeroboto_vh_start(void)
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,64);

	if (!bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap,0);

	tilemap_set_scroll_rows(bg_tilemap,64);

    if (generic_vh_start())
		return 1;
    else
		return 0;
}

/***************************************************************************

  Memory handlers

***************************************************************************/
READ_HANDLER( aeroboto_in0_r )
{
	return readinputport(flip_screen ? 1 : 0);
}

WRITE_HANDLER( aeroboto_3000_w )
{
	/* bit 0 selects both flip screen and player1/player2 controls */
	flip_screen_w(0, data & 0x01);

	/* bit 1 = char bank select */
	if (aeroboto_charbank != ((data & 0x02) >> 1))
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap);
		aeroboto_charbank = (data & 0x02) >> 1;
	}
}

WRITE_HANDLER( aeroboto_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE_HANDLER( aeroboto_tilecolor_w )
{
	if (aeroboto_tilecolor[offset] != data)
	{
		aeroboto_tilecolor[offset] = data;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int x = spriteram[offs+3];
		int y = 240 - spriteram[offs];

		if (flip_screen)
		{
			x = 248 - x;
			y = 240 - y;
		}

		drawgfx(bitmap, Machine->gfx[2],
				spriteram[offs+1],
				spriteram[offs+2] & 0x07,
				flip_screen, flip_screen,
				((x + 8) & 0xff) - 8, y,
				&Machine->visible_area,TRANSPARENCY_PEN, 0);
	}
}

void aeroboto_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int y;

    tilemap_update(ALL_TILEMAPS);
    tilemap_render(ALL_TILEMAPS);

	for (y = 0;y < 64; y++)
		tilemap_set_scrollx(bg_tilemap,y,aeroboto_hscroll[y]);

	// the playfield is part of a splitscreen and should not overlap with status display
	tilemap_set_scrolly(bg_tilemap,0,*aeroboto_vscroll);
	tilemap_draw(bitmap,bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY);

	draw_sprites(bitmap);

	// the status display behaves more closely to a 40-line splitscreen than an overlay
	tilemap_set_scrolly(bg_tilemap,0,0);
	tilemap_draw(bitmap,bg_tilemap,0);
}

#ifdef __cplusplus
}
#endif
