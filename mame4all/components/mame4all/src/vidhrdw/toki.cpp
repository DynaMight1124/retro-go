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

static struct tilemap *background_layer,*foreground_layer,*text_layer;

data16_t *toki_background1_videoram16;
data16_t *toki_background2_videoram16;
data16_t *toki_scrollram16;
static unsigned int toki_background_xscroll[256];
static unsigned int toki_foreground_xscroll[256];

// 16-bit versions of pointers
static unsigned short *videoram16;
static unsigned short *buffered_spriteram16;
// 8-bit versions of pointers
unsigned char *toki_background1_videoram;
unsigned char *toki_background2_videoram;
unsigned char *toki_scrollram;

/*************************************************************************
					RASTER EFFECTS

Xscroll can be altered per scanline to create rowscroll effect.

(The driver does not implement rowscroll on the bootleg. It seems unlikely
the bootleggers would have been able to do this on their chipset. They
remapped the scroll registers, so obviously they were using a different
chip.

Why then would the old ghost of one of the remapped registers
still cause rowscroll? Probably the bootleggers simply didn't bother to
remove all the code writing the $a0000 area.)

*************************************************************************/

WRITE_HANDLER( toki_control_w )
{
	offset >>= 1;	// 16-bit to 8-bit conversion

	int currline = cpu_getscanline();

	WRITE_WORD(&toki_scrollram16[offset], data);

	/* Keep a per-scanline record of X scroll offsets */
	if (offset==0x15 || offset==0x16)
		toki_foreground_xscroll[currline%256]=((toki_scrollram16[0x16] &0x7f) << 1)
								 |((toki_scrollram16[0x16] &0x80) >> 7)
								 |((toki_scrollram16[0x15] &0x10) << 4);

	if (offset==0x05 || offset==0x06)
		toki_background_xscroll[currline%256]=((toki_scrollram16[0x06] &0x7f) << 1)
								 |((toki_scrollram16[0x06] &0x80) >> 7)
								 |((toki_scrollram16[0x05] &0x10) << 4);
}

/* At EOF clear the previous frames scroll registers */
void toki_eof_callback(void)
{
	int i;

	toki_background_xscroll[0]=((toki_scrollram16[0x16] &0x7f) << 1)
								 |((toki_scrollram16[0x16] &0x80) >> 7)
								 |((toki_scrollram16[0x15] &0x10) << 4);
	toki_foreground_xscroll[0]=((toki_scrollram16[0x06] &0x7f) << 1)
								 |((toki_scrollram16[0x06] &0x80) >> 7)
								 |((toki_scrollram16[0x05] &0x10) << 4);

	for (i=1; i<256; i++)
		toki_background_xscroll[i]=toki_foreground_xscroll[i]=0xffff;

	buffer_spriteram_w(0,0);
}

static void get_text_tile_info(int tile_index)
{
	int tile = videoram16[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO( 0, tile, color)
}

static void get_back_tile_info(int tile_index)
{
	int tile = toki_background1_videoram16[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO( 2, tile, color)
}

static void get_fore_tile_info(int tile_index)
{
	int tile = toki_background2_videoram16[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO( 3, tile, color)
}


/*************************************
 *
 *		Start/Stop
 *
 *************************************/

int toki_vh_start (void)
{
	/* Create the 16-bit pointers from the 8-bit pointers */
	toki_background1_videoram16 = (data16_t *)toki_background1_videoram;
	toki_background2_videoram16 = (data16_t *)toki_background2_videoram;
	toki_scrollram16 = (data16_t *)toki_scrollram;
	videoram16 = (unsigned short *)videoram;
	buffered_spriteram16 = (unsigned short *)buffered_spriteram;

	text_layer       = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,32,32);
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if (!text_layer || !background_layer || !foreground_layer) return 1;

	tilemap_set_transparent_pen(text_layer,15);
	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_scroll_rows(foreground_layer,512);
	tilemap_set_scroll_rows(background_layer,512);

	return 0;
}


/*************************************/

WRITE_HANDLER( toki_foreground_videoram16_w )
{
	offset >>= 1;	// 16-bit to 8-bit conversion
	int oldword = videoram16[offset];
	WRITE_WORD(&videoram16[offset], data);
	if (oldword != videoram16[offset])
		tilemap_mark_tile_dirty(text_layer,offset);
}

WRITE_HANDLER( toki_background1_videoram16_w )
{
	offset >>= 1;	// 16-bit to 8-bit conversion
	int oldword = toki_background1_videoram16[offset];
	WRITE_WORD(&toki_background1_videoram16[offset], data);
	if (oldword != toki_background1_videoram16[offset])
		tilemap_mark_tile_dirty(background_layer,offset);
}

WRITE_HANDLER( toki_background2_videoram16_w )
{
	offset >>= 1;	// 16-bit to 8-bit conversion
	int oldword = toki_background2_videoram16[offset];
	WRITE_WORD(&toki_background2_videoram16[offset], data);
	if (oldword != toki_background2_videoram16[offset])
		tilemap_mark_tile_dirty(foreground_layer,offset);
}

/**************************************************************************/

void toki_mark_sprite_colours(void)
{
	UINT16 palette_map[16*4],usage;
	data16_t *sprite_word;
	int i,code,color,offs;

	memset (palette_map, 0, sizeof (palette_map));

	/* sprites */
	for (offs = 0;offs < spriteram_size / 2;offs += 4)
	{
		sprite_word = &buffered_spriteram16[offs];

		if ((sprite_word[2] != 0xf000) && (sprite_word[0] != 0xffff))
		{
			color = sprite_word[1] >> 12;
			code  = (sprite_word[1] & 0xfff) + ((sprite_word[2] & 0x8000) >> 3);
			palette_map[color] |= Machine->gfx[1]->pen_usage[code];
		}
	}

	/* expand it */
	for (color = 0; color < 16 * 4; color++)
	{
		usage = palette_map[color];

		if (usage)
		{
			for (i = 0; i < 15; i++)
				if (usage & (1 << i))
					palette_used_colors[color * 16 + i] = PALETTE_COLOR_USED;
			palette_used_colors[color * 16 + 15] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}

void tokib_mark_sprite_colours (void)
{
	UINT16 palette_map[16*4],usage;
	data16_t *sprite_word;
	int i,code,color,offs;

	memset (palette_map, 0, sizeof (palette_map));

	/* sprites */
	for (offs = 0;offs < spriteram_size / 2;offs += 4)
	{
		sprite_word = &buffered_spriteram16[offs];

		if (sprite_word[0] == 0xf100)
			break;

		color = sprite_word[2] >> 12;
		code  = sprite_word[1] & 0x1fff;
		palette_map[color] |= Machine->gfx[1]->pen_usage[code];
	}

	/* expand it */
	for (color = 0; color < 16 * 4; color++)
	{
		usage = palette_map[color];

		if (usage)
		{
			for (i = 0; i < 15; i++)
				if (usage & (1 << i))
					palette_used_colors[color * 16 + i] = PALETTE_COLOR_USED;
			palette_used_colors[color * 16 + 15] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}


/***************************************************************************
					SPRITES

	Original Spriteram
	------------------

	It's not clear what purpose is served by marking tiles as being part of
	big sprites. (Big sprites in the attract abduction scene have all tiles
	marked as "first" unlike big sprites in-game.)

	We just ignore this top nibble (although perhaps in theory the bits
	enable X/Y offsets in the low byte).

	+0   x....... ........  sprite disable ??
      +0   .xx..... ........  tile is part of big sprite (4=first, 6=middle, 2=last)
	+0   .....x.. ........  ??? always set? (could be priority - see Bloodbro)
	+0   .......x ........  Flip x
	+0   ........ xxxx....  X offset: add (this * 16) to X coord
	+0   ........ ....xxxx  Y offset: add (this * 16) to Y coord

 	+1   xxxx.... ........  Color bank
	+1   ....xxxx xxxxxxxx  Tile number (lo bits)
	+2   x....... ........  Tile number (hi bit)
	+2   .???.... ........  (set in not yet used entries)
	+2   .......x xxxxxxxx  X coordinate
	+3   .......x xxxxxxxx  Y coordinate

	f000 0000 f000 0000     entry not yet used: unless this is honored there
	                        will be junk sprites in top left corner
	ffff ???? ???? ????     sprite marked as dead: unless this is honored
	                        there will be junk sprites after floating monkey machine


	Bootleg Spriteram
	-----------------

	+0   .......x xxxxxxxx  Sprite Y coordinate
	+1   ...xxxxx xxxxxxxx  Sprite tile number
	+1   .x...... ........  Sprite flip x
 	+2   xxxx.... ........  Sprite color bank
	+3   .......x xxxxxxxx  Sprite X coordinate

	f100 ???? ???? ????     dead / unused sprite ??
	???? ???? 0000 ????     dead / unused sprite ??


***************************************************************************/


void toki_draw_sprites (struct osd_bitmap *bitmap)
{
	int x,y,xoffs,yoffs,tile,flipx,flipy,color,offs;
	data16_t *sprite_word;

	for (offs = (spriteram_size/2)-4;offs >= 0;offs -= 4)
	{
		sprite_word = &buffered_spriteram16[offs];

		if ((sprite_word[2] != 0xf000) && (sprite_word[0] != 0xffff))
		{
			xoffs = (sprite_word[0] &0xf0);
			x = (sprite_word[2] + xoffs) & 0x1ff;
			if (x > 256)
				x -= 512;

			yoffs = (sprite_word[0] &0xf) << 4;
			y = (sprite_word[3] + yoffs) & 0x1ff;
			if (y > 256)
				y -= 512;

			color = sprite_word[1] >> 12;
			flipx   = sprite_word[0] & 0x100;
			flipy   = 0;
			tile    = (sprite_word[1] & 0xfff) + ((sprite_word[2] & 0x8000) >> 3);

			if (flip_screen) {
				x=240-x;
				y=240-y;
				if (flipx) flipx=0; else flipx=1;
				flipy=1;
			}

			drawgfx (bitmap,Machine->gfx[1],
					tile,
					color,
					flipx,flipy,
					x,y,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}
}


void tokib_draw_sprites (struct osd_bitmap *bitmap)
{
	int x,y,tile,flipx,color,offs;
	data16_t *sprite_word;

	for (offs = 0;offs < spriteram_size / 2;offs += 4)
	{
		sprite_word = &buffered_spriteram16[offs];

		if (sprite_word[0] == 0xf100)
			break;
		if (sprite_word[2])
		{

			x = sprite_word[3] & 0x1ff;
			if (x > 256)
				x -= 512;

			y = sprite_word[0] & 0x1ff;
			if (y > 256)
				y = (512-y)+240;
			else
				y = 240-y;

			flipx   = sprite_word[1] & 0x4000;
			tile    = sprite_word[1] & 0x1fff;
			color   = sprite_word[2] >> 12;

			drawgfx (bitmap,Machine->gfx[1],
					tile,
					color,
					flipx,0,
					x,y-1,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}
}

/*************************************
 *
 *		Master update function
 *
 *************************************/

void toki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,background_y_scroll,foreground_y_scroll,latch1,latch2;

	background_y_scroll=((toki_scrollram16[0x0d]&0x10)<<4)+((toki_scrollram16[0x0e]&0x7f)<<1)+((toki_scrollram16[0x0e]&0x80)>>7);
	foreground_y_scroll=((toki_scrollram16[0x1d]&0x10)<<4)+((toki_scrollram16[0x1e]&0x7f)<<1)+((toki_scrollram16[0x1e]&0x80)>>7);
	tilemap_set_scrolly( background_layer, 0, background_y_scroll );
	tilemap_set_scrolly( foreground_layer, 0, foreground_y_scroll );

	latch1=toki_background_xscroll[0];
	latch2=toki_foreground_xscroll[0];
	for (i=0; i<256; i++) {
		if (toki_background_xscroll[i]!=0xffff)
			latch1=toki_background_xscroll[i];
		if (toki_foreground_xscroll[i]!=0xffff)
			latch2=toki_foreground_xscroll[i];

		tilemap_set_scrollx( background_layer, (i+background_y_scroll)%512, latch1 );
		tilemap_set_scrollx( foreground_layer, (i+foreground_y_scroll)%512, latch2 );
	}

	//flip_screen_set((toki_scrollram16[0x28]&0x8000)==0);

	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
	toki_mark_sprite_colours();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	if (toki_scrollram16[0x28]&0x100) {
		tilemap_draw(bitmap,background_layer,TILEMAP_IGNORE_TRANSPARENCY);
		tilemap_draw(bitmap,foreground_layer,0);
	} else {
		tilemap_draw(bitmap,foreground_layer,TILEMAP_IGNORE_TRANSPARENCY);
		tilemap_draw(bitmap,background_layer,0);
	}
	toki_draw_sprites (bitmap);
	tilemap_draw(bitmap,text_layer,0);
}

void tokib_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scroll_rows(foreground_layer,1);
	tilemap_set_scroll_rows(background_layer,1);
	tilemap_set_scrolly( background_layer, 0, toki_scrollram16[0]+1 );
	tilemap_set_scrollx( background_layer, 0, toki_scrollram16[1]-0x103 );
	tilemap_set_scrolly( foreground_layer, 0, toki_scrollram16[2]+1 );
	tilemap_set_scrollx( foreground_layer, 0, toki_scrollram16[3]-0x101 );

	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
	tokib_mark_sprite_colours();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	if (toki_scrollram16[3]&0x2000) {
		tilemap_draw(bitmap,background_layer,TILEMAP_IGNORE_TRANSPARENCY);
		tilemap_draw(bitmap,foreground_layer,0);
	} else {
		tilemap_draw(bitmap,foreground_layer,TILEMAP_IGNORE_TRANSPARENCY);
		tilemap_draw(bitmap,background_layer,0);
	}

	tokib_draw_sprites (bitmap);
	tilemap_draw(bitmap,text_layer,0);
}

#ifdef __cplusplus
}
#endif
