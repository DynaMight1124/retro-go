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
/*****************************************************************************

	Irem M90 system.  There is 1 video chip - NANAO GA-25, it produces
	2 tilemaps and sprites.  16 control bytes:

	0:  Playfield 1 X scroll
	2:  Playfield 1 Y scroll
	4:  Playfield 2 X scroll
	6:  Playfield 2 Y scroll
	8:  Unused?
	10: Playfield 1 control
		Bit 0x02 - Playfield 1 VRAM base (0 is 0x0000, 0x2 is 0x8000)
 		Bit 0x04 - Playfield 1 width (0 is 64 tiles, 0x4 is 128 tiles)
		Bit 0x10 - Playfield 1 disable
		Bit 0x20 - Playfield 1 rowscroll enable
	12: Playfield 2 control
		Bit 0x02 - Playfield 2 VRAM base (0 is 0x0000, 0x2 is 0x8000)
 		Bit 0x04 - Playfield 2 width (0 is 64 tiles, 0x4 is 128 tiles)
		Bit 0x10 - Playfield 2 disable
		Bit 0x20 - Playfield 2 rowscroll enable
	14: Unused?

	Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy!

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char *m90_spriteram;
unsigned char *m90_video_data;
static struct tilemap *pf1_layer,*pf2_layer,*pf1_wide_layer,*pf2_wide_layer;
static int m90_video_control_data[16];

static void get_pf1_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index+((m90_video_control_data[0xa]&0x2)*0x4000);

	tile=m90_video_data[tile_index]+(m90_video_data[tile_index+1]<<8);
	color=m90_video_data[tile_index+2];
	SET_TILE_INFO(0,tile,color&0xf)

	tile_info.flags = TILE_FLIPYX(m90_video_data[tile_index+2]>>6);
}

static void get_pf2_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index+((m90_video_control_data[0xc]&0x2)*0x4000);

	tile=m90_video_data[tile_index]+(m90_video_data[tile_index+1]<<8);
	color=m90_video_data[tile_index+2];
	SET_TILE_INFO(0,tile,color&0xf)

	tile_info.flags = TILE_FLIPYX(m90_video_data[tile_index+2]>>6);
}

int m90_vh_start(void)
{
	pf1_layer = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	pf1_wide_layer = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
	pf2_layer = tilemap_create(get_pf2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);
	pf2_wide_layer = tilemap_create(get_pf2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,128,64);

	if (!pf1_layer || !pf1_wide_layer || !pf2_layer || !pf2_wide_layer)
		return 1;

	tilemap_set_transparent_pen(pf1_layer,0);
	tilemap_set_transparent_pen(pf1_wide_layer,0);

	return 0;
}

static void mark_sprite_colours(void)
{
	int i,offs,color,pal_base,colmask[16];
    unsigned int *pen_usage; /* Save some struct derefs */

	int sprite_mask=(Machine->drv->gfxdecodeinfo[1].gfxlayout->total)-1;

	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	pen_usage=Machine->gfx[1]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0x0;offs < 0x200-8; offs+=6)
	{
		int sprite;

	    sprite=m90_spriteram[offs+2] | (m90_spriteram[offs+3]<<8);
		color=(m90_spriteram[offs+1]>>1)&0xf;

		colmask[color] |= pen_usage[(sprite+0)&sprite_mask] | pen_usage[(sprite+1)&sprite_mask];
		colmask[color] |= pen_usage[(sprite+2)&sprite_mask] | pen_usage[(sprite+3)&sprite_mask];
		colmask[color] |= pen_usage[(sprite+4)&sprite_mask] | pen_usage[(sprite+5)&sprite_mask];
		colmask[color] |= pen_usage[(sprite+6)&sprite_mask] | pen_usage[(sprite+7)&sprite_mask];
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}
}

static void m90_drawsprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0x0;offs <0x200-8;offs+= 6) {
		int x,y,sprite,colour,fx,fy,y_multi,i;

		y=(m90_spriteram[offs+0] | (m90_spriteram[offs+1]<<8))&0x1ff;
		x=(m90_spriteram[offs+4] | (m90_spriteram[offs+5]<<8))&0x1ff;

		x = x - 16;
		y = 496 - y;

	    sprite=(m90_spriteram[offs+2] | (m90_spriteram[offs+3]<<8));
		colour=(m90_spriteram[offs+1]>>1)&0xf;
		fx=m90_spriteram[offs+5]&0x2;
		fy=0;//m90_spriteram[offs+5]&2;

		y_multi=1;
		if ((m90_spriteram[offs+1]&0xe0)==0x20) { y=y-0x10;y_multi=2;}
		if ((m90_spriteram[offs+1]&0xe0)==0x40) { y=y-0x30;y_multi=4;}
		if ((m90_spriteram[offs+1]&0xe0)==0x60) { y=y-0x70;y_multi=8;}
//		if ((m90_spriteram[offs+1]&0x80)==0x80) colour=rand()%0xf;

		if (y_multi)
			for (i=0; i<y_multi; i++)
				drawgfx(bitmap,Machine->gfx[1],
						sprite+i,
						colour,
						fx,fy,
						x,y+i*16,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
		else
			drawgfx(bitmap,Machine->gfx[1],
						sprite,
						colour,
						fx,fy,
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

static void bootleg_drawsprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0x0;offs <0x800-8;offs+= 8) {
		int x,y,sprite,colour,fx,fy;

		if (/*spriteram[offs+0]==0x78 &&*/ spriteram[offs+1]==0x7f) continue;

		y=(spriteram[offs+0] | (spriteram[offs+1]<<8))&0x1ff;
		x=(spriteram[offs+6] | (spriteram[offs+7]<<8))&0x1ff;

		x = x - /*64 -*/ 16;
		y = 256 - /*32 -*/ y;

	    sprite=(spriteram[offs+2] | (spriteram[offs+3]<<8));
		colour=(spriteram[offs+5]>>1)&0xf;

		fx=spriteram[offs+5]&1;
		fy=0;//spriteram[offs+5]&2;

		drawgfx(bitmap,Machine->gfx[1],
				sprite&0x1fff,
				colour,
				fx,fy,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

WRITE_HANDLER( m90_video_control_w )
{
	m90_video_control_data[offset]=data;
}

WRITE_HANDLER( m90_video_w )
{
	m90_video_data[offset]=data;

	if ((m90_video_control_data[0xa]&0x2)==0) {
		if (offset<0x4000) tilemap_mark_tile_dirty(pf1_layer,offset/4);
		if (offset<0x8000) tilemap_mark_tile_dirty(pf1_wide_layer,offset/4);
	} else if ((m90_video_control_data[0xa]&0x2)==2) {
		if ((offset-0x8000)<0x4000) tilemap_mark_tile_dirty(pf1_layer,(offset-0x8000)/4);
		if ((offset-0x8000)<0x8000) tilemap_mark_tile_dirty(pf1_wide_layer,(offset-0x8000)/4);
	}

	if ((m90_video_control_data[0xc]&0x2)==0) {
		if (offset<0x4000) tilemap_mark_tile_dirty(pf2_layer,offset/4);
		if (offset<0x8000) tilemap_mark_tile_dirty(pf2_wide_layer,offset/4);
	} else if ((m90_video_control_data[0xc]&0x2)==2) {
		if ((offset-0x8000)<0x4000) tilemap_mark_tile_dirty(pf2_layer,(offset-0x8000)/4);
		if ((offset-0x8000)<0x8000) tilemap_mark_tile_dirty(pf2_wide_layer,(offset-0x8000)/4);
	}
}

void m90_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	static int last_pf1,last_pf2;
	int pf1_base=m90_video_control_data[0xa]&0x2,pf2_base=m90_video_control_data[0xc]&0x2;
	int i,pf1_enable,pf2_enable;

	if (m90_video_control_data[0xa]&0x10) pf1_enable=0; else pf1_enable=1;
	if (m90_video_control_data[0xc]&0x10) pf2_enable=0; else pf2_enable=1;
//	tilemap_set_enable(pf1_layer,pf1_enable);
//	tilemap_set_enable(pf2_layer,pf2_enable);
//	tilemap_set_enable(pf1_wide_layer,pf1_enable);
//	tilemap_set_enable(pf2_wide_layer,pf2_enable);

	/* Dirty tilemaps if VRAM base changes */
	if (pf1_base!=last_pf1) {
		tilemap_mark_all_tiles_dirty(pf1_layer);
		tilemap_mark_all_tiles_dirty(pf1_wide_layer);
	}
	if (pf2_base!=last_pf2)  {
		tilemap_mark_all_tiles_dirty(pf2_layer);
		tilemap_mark_all_tiles_dirty(pf2_wide_layer);
	}
	last_pf1=pf1_base;
	last_pf2=pf2_base;

	m90_spriteram=m90_video_data+0xee00;

	/* Setup scrolling */
	if (m90_video_control_data[0xa]&0x20) {
		tilemap_set_scroll_rows(pf1_layer,512);
		tilemap_set_scroll_rows(pf1_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_layer,i/2, (m90_video_data[0xf000+i]+(m90_video_data[0xf001+i]<<8))+2);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_wide_layer,i/2, (m90_video_data[0xf000+i]+(m90_video_data[0xf001+i]<<8))+256+2);
	} else {
		tilemap_set_scroll_rows(pf1_layer,1);
		tilemap_set_scroll_rows(pf1_wide_layer,1);
		tilemap_set_scrollx( pf1_layer,0, (m90_video_control_data[3]<<8)+m90_video_control_data[2]+2);
		tilemap_set_scrollx( pf1_wide_layer,0, (m90_video_control_data[3]<<8)+m90_video_control_data[2]+256+2);
	}

	/* Setup scrolling */
	if (m90_video_control_data[0xc]&0x20) {
		tilemap_set_scroll_rows(pf2_layer,512);
		tilemap_set_scroll_rows(pf2_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_layer,i/2, (m90_video_data[0xf400+i]+(m90_video_data[0xf401+i]<<8))-2);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_wide_layer,i/2, (m90_video_data[0xf400+i]+(m90_video_data[0xf401+i]<<8))+256-2);
	} else {
		tilemap_set_scroll_rows(pf2_layer,1);
		tilemap_set_scroll_rows(pf2_wide_layer,1);
		tilemap_set_scrollx( pf2_layer,0, (m90_video_control_data[7]<<8)+m90_video_control_data[6]-2);
		tilemap_set_scrollx( pf2_wide_layer,0, (m90_video_control_data[7]<<8)+m90_video_control_data[6]+256-2 );
	}

	tilemap_set_scrolly( pf1_layer,0, (m90_video_control_data[1]<<8)+m90_video_control_data[0] );
	tilemap_set_scrolly( pf2_layer,0, (m90_video_control_data[5]<<8)+m90_video_control_data[4] );
	tilemap_set_scrolly( pf1_wide_layer,0, (m90_video_control_data[1]<<8)+m90_video_control_data[0] );
	tilemap_set_scrolly( pf2_wide_layer,0, (m90_video_control_data[5]<<8)+m90_video_control_data[4] );

	tilemap_update(ALL_TILEMAPS);
	palette_init_used_colors();
	mark_sprite_colours();
	palette_recalc();

	tilemap_render(ALL_TILEMAPS);

	if (!pf2_enable) {
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	} else {
		if (m90_video_control_data[0xc]&0x4)
			tilemap_draw(bitmap,pf2_wide_layer,0);
		else
			tilemap_draw(bitmap,pf2_layer,0);
	}

	m90_drawsprites(bitmap);

	if (m90_video_control_data[0xa]&0x4)
		tilemap_draw(bitmap,pf1_wide_layer,0);
	else
		tilemap_draw(bitmap,pf1_layer,0);
}

#ifdef __cplusplus
}
#endif
