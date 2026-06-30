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

   Taito F3 Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

Brief overview:

	4 scrolling layers (512x512 or 1024x512) of 4/5/6 bpp tiles.
	1 scrolling text layer (512x512, characters generated in vram), 4bpp chars.
	1 scrolling pixel layer (512x256 pixels generated in pivot ram), 4bpp pixels.
	2 sprite banks (for double buffering of sprites)
	Sprites can be 4, 5 or 6 bpp
	Sprite scaling.
	Rowscroll on all playfields
	Line by line zoom on all playfields
	Column scroll on all playfields
	Line by line sprite and playfield priority mixing

Not yet supported:
	Alpha blending on playfields and VRAM layer

Notes:
	All special effects are controlled by an area in 'line ram'.  Typically
	there are 256 values, one for each line of the screen (including clipped
	lines at the top of the screen).  For example, at 0x8000 in lineram,
	there are 4 sets of 256 values (one for each playfield) and each value
	is the scale control for that line in the destination bitmap (screen).
	Therefore each line can have a different zoom value for special effects.

	This also applies to playfield priority, rowscroll, column scroll, sprite
	priority and VRAM/pivot layers.

	However - at the start of line ram there are also sets of 256 values
	controlling each effect - effects can be selectively applied to individual
	playfields or only certain lines out of the 256 can be active - in which
	case the last allowed value can be latched (typically used so a game can
	use one zoom or row value over the whole playfield).

	The programmers of some of these games made strange use of flipscreen -
	some games have all their graphics flipped in ROM, and use the flipscreen
	bit to display them correctly!.

	Most games display 232 scanlines, but some use lineram effects to clip
	themselves to 224 or less.

****************************************************************************

Line ram memory map:

	Here 'playfield 1' refers to the first playfield in memory, etc

	0x0000: Column line control ram (Confirmed) (256 lines)
		100x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0200: ? (Not sprite)
	0x0400: ?
	0x0600: Pivot?
	0x0800: Sprite?
	0x0a00: Zoom? (not sprite)
	0x0c00: Rowscroll line control ram (Confirmed) (256 lines)
		280x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0e00: Priority line control ram? (256 lines)

	0x4000: Playfield 1 column scroll (on source bitmap, not destination)
	0x4200: Playfield 2 column scroll (on source bitmap, not destination)
	0x4400: Playfield 3 column scroll (on source bitmap, not destination)
	0x4600: Playfield 4 column scroll (on source bitmap, not destination)

	0x5000: ?????

	0x6000:	Pivot layer control
		Cupfinal a255 display, 0255 don't
		Bubsymph a255 display  0255 don't
		Pbobbl4u a2ff display (colour 0xa) 02ff don't
		Landmakr 00df dont display
		qtheater 01ff dont display
		0x00p0 - p = priority? (See BubSymph continue screen)
	0x6200: Alpha blending control
		Cupfinal 3000
		Landmakr bbbb
		qtheater bbbb
		bubsymph 3000
	0x6400: Pivot layer control?
		Cupfinal 7000
		landmakr 7000
		qtheater 7000
		bubsymph 7000
		pbobbl4u 7000
	0x6600: Always zero?

	0x7000: ?
	0x7200: ?
	0x7400: ?
	0x7600: Sprite priority values
		0xf000:	Relative priority for sprites with pri value 0xc0
		0x0f00:	Relative priority for sprites with pri value 0x80
		0x00f0:	Relative priority for sprites with pri value 0x40
		0x000f:	Relative priority for sprites with pri value 0x00

	0x8000: Playfield 1 scale (1 word per line, 256 lines, 0x80 = no scale)
	0x8200: Playfield 2 scale
	0x8400: Playfield 3 scale
	0x8600: Playfield 4 scale
		0x0080 = No scale
		< 0x80 = Zoom Out
		> 0x80 = Zoom in

	0xa000: Playfield 1 rowscroll (1 word per line, 256 lines)
	0xa200: Playfield 2 rowscroll
	0xa400: Playfield 3 rowscroll
	0xa600: Playfield 4 rowscroll

	0xb000: Playfield 1 priority (1 word per line, 256 lines)
	0xb200: Playfield 2 priority
	0xb400: Playfield 3 priority
	0xb600: Playfield 4 priority
		0xf000 = Disable playfield (ElvAct2 second level)
		0x8000 = Enable alpha-blending for this line
		0x4000 = Disable line? (Darius Gaiden 0x5000 = disable, 0x3000, 0xb000 & 0x7000 = display)
		0x2000 = ?
		0x1000 = ?
		0x0800 = Disable line (Used by KTiger2 to clip screen)
		0x07f0 = ?
		0x000f = Playfield priority

	0xc000 - 0xffff: Unused.

****************************************************************************

	F3 sprite format:

	Word 0:	0xffff		Tile number (LSB)
	Word 1:	0xff00		X zoom
			0x00ff		Y zoom
	Word 2:	0x03ff		X position
	Word 3:	0x03ff		Y position
	Word 4:	0xf000		Sprite block controls
			0x0800		Sprite block start
			0x0400		Use same colour on this sprite as block start
			0x0200		Y flip
			0x0100		X flip
			0x00ff		Colour
	Word 5: 0xffff		Tile number (MSB), probably only low bits used
	Word 6:	0x8000		If set, jump to sprite location in low bits
			0x03ff		Location to jump to.
	Word 7: 0xffff		Unused?  Always zero?

****************************************************************************

	Playfield control information (0x660000-1f):

	Word 0- 3: X scroll values for the 4 playfields.
	Word 4- 7: Y scroll values for the 4 playfields.
	Word 8-11: Unused.  Always zero.
	Word   12: Pixel + VRAM playfields X scroll
	Word   13: Pixel + VRAM playfields Y scroll
	Word   14: Unused. Always zero.
	Word   15: If set to 0x80, then 1024x512 playfields are used, else 512x512

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/taito_f3.h"
#include "state.h"

#define DARIUSG_KLUDGE
#define DEBUG_F3 0
#define TRY_ALPHA 0

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static data32_t *gfx_base,*spriteram32_buffered;
static int vram_dirty[256];
static int pivot_changed,vram_changed,scroll_kludge_y,scroll_kludge_x;
static data32_t f3_control_0[8];
static data32_t f3_control_1[8];
static int flipscreen;
static int primasks[4];
static UINT16 *scanline;
static UINT8 *pivot_dirty;
static int pf23_y_kludge,clip_pri_kludge,pivot_active;

data32_t *f3_vram,*f3_line_ram;
data32_t *f3_pf_data,*f3_pivot_ram;

extern int f3_game;
static int sprite_pri_word;
static struct osd_bitmap *pixel_layer;

/* Game specific data, some of this can be
removed when the software values are figured out */
struct F3config
{
	int name;
	int extend;
	int sx;
	int sy;
	int pivot;
	int xyzoom;
};

struct F3config *f3_game_config;
static struct F3config f3_config_table[]=
{
	/* Name    Extend  X Offset, Y Offset  Flip Pivot XYZoom */  /* Uses 5000, uses control bits,works with line X zoom */
	{ RINGRAGE,  0,      0,        -30,         0,       0    }, //no,no,no
	{ ARABIANM,  0,      0,        -30,         0,       0    }, //ff00 in 5000, no,yes
	{ RIDINGF,   1,      0,        -30,         0,       1    }, //yes,yes,yes
	{ GSEEKER,   0,      0,        -31,         0,       0    }, //yes,yes,yes
	{ TRSTAR,    1,      0,          0,         0,       0    }, //
	{ GUNLOCK,   1,      1,        -30,         0,       1    }, //yes,yes,partial
	{ TWINQIX,   1,      0,        -30,         1,       0    },
	{ SCFINALS,  0,      0,        -31,         1,       1    },
	{ LIGHTBR,   1,      0,        -30,         0,       0    }, //yes,?,no
	{ KAISERKN,  0,      1,        -30,         1,       1    },
	{ DARIUSG,   0,      0,        -23,         0,       1    },
	{ BUBSYMPH,  1,      0,        -30,         0,       0    }, //yes,yes,?
	{ SPCINVDX,  1,      0,        -30,         1,       0    }, //yes,yes,?
	{ QTHEATER,  1,      0,          0,         0,       0    },
	{ HTHERO95,  0,      0,        -30,         1,       1    },
	{ SPCINV95,  0,      0,        -30,         0,       0    },
	{ EACTION2,  1,      0,        -23,         0,       1    }, //no,yes,?
	{ QUIZHUHU,  1,      0,        -23,         0,       0    },
	{ PBOBBLE2,  0,      1,          0,         1,       1    }, //no,no,?
	{ GEKIRIDO,  0,      0,        -23,         0,       1    },
	{ KTIGER2,   0,      1,        -24,         0,       0    },//no,yes,partial
	{ BUBBLEM,   1,      0,        -30,         0,       0    },
	{ CLEOPATR,  0,      0,        -30,         0,       0    },
	{ PBOBBLE3,  0,      1,          0,         0,       0    },
	{ ARKRETRN,  1,      0,        -23,         0,       0    },
	{ KIRAMEKI,  0,      0,        -30,         0,       0    },
	{ PUCHICAR,  1,      0,        -23,         0,       0    },
	{ PBOBBLE4,  0,      1,          0,         1,       1    },
	{ POPNPOP,   1,      0,        -23,         0,       0    },
	{ LANDMAKR,  1,      0,        -23,         0,       1    },
	{0}
};

struct tempsprite
{
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;

/******************************************************************************/

static void print_debug_info(int t0, int t1, int t2, int t3, int c0, int c1, int c2, int c3)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;
	int j,trueorientation,l[16];
	char buf[64];

	trueorientation = Machine->orientation;
	Machine->orientation = ROT0;

	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[0]>>22,(f3_control_0[0]&0xffff)>>6,f3_control_0[1]>>22,(f3_control_0[1]&0xffff)>>6);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,40,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[2]>>23,(f3_control_0[2]&0xffff)>>7,f3_control_0[3]>>23,(f3_control_0[3]&0xffff)>>7);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,48,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[0]>>16,f3_control_1[0]&0xffff,f3_control_1[1]>>16,f3_control_1[1]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,58,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[2]>>16,f3_control_1[2]&0xffff,f3_control_1[3]>>16,f3_control_1[3]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,66,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[0]>>16,spriteram32_buffered[0]&0xffff,spriteram32_buffered[1]>>16,spriteram32_buffered[1]&0xffff,spriteram32_buffered[2]>>16,spriteram32_buffered[2]&0xffff,spriteram32_buffered[3]>>16,spriteram32_buffered[3]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,76,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[4]>>16,spriteram32_buffered[4]&0xffff,spriteram32_buffered[5]>>16,spriteram32_buffered[5]&0xffff,spriteram32_buffered[6]>>16,spriteram32_buffered[6]&0xffff,spriteram32_buffered[7]>>16,spriteram32_buffered[7]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,84,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[8]>>16,spriteram32_buffered[8]&0xffff,spriteram32_buffered[9]>>16,spriteram32_buffered[9]&0xffff,spriteram32_buffered[10]>>16,spriteram32_buffered[10]&0xffff,spriteram32_buffered[11]>>16,spriteram32_buffered[11]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,92,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0040]&0xffff;
	l[1]=f3_line_ram[0x00c0]&0xffff;
	l[2]=f3_line_ram[0x0140]&0xffff;
	l[3]=f3_line_ram[0x01c0]&0xffff;
	sprintf(buf,"Ctr1: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*13,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0240]&0xffff;
	l[1]=f3_line_ram[0x02c0]&0xffff;
	l[2]=f3_line_ram[0x0340]&0xffff;
	l[3]=f3_line_ram[0x03c0]&0xffff;
	sprintf(buf,"Ctr2: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*14,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2c60]&0xffff;
	l[1]=f3_line_ram[0x2ce0]&0xffff;
	l[2]=f3_line_ram[0x2d60]&0xffff;
	l[3]=f3_line_ram[0x2de0]&0xffff;
	sprintf(buf,"Pri : %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*15,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2060]&0xffff;
	l[1]=f3_line_ram[0x20e0]&0xffff;
	l[2]=f3_line_ram[0x2160]&0xffff;
	l[3]=f3_line_ram[0x21e0]&0xffff;
	sprintf(buf,"Zoom: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*16,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2860]&0xffff;
	l[1]=f3_line_ram[0x28e0]&0xffff;
	l[2]=f3_line_ram[0x2960]&0xffff;
	l[3]=f3_line_ram[0x29e0]&0xffff;
	sprintf(buf,"Line: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*17,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1c60]&0xffff;
	l[1]=f3_line_ram[0x1ce0]&0xffff;
	l[2]=f3_line_ram[0x1d60]&0xffff;
	l[3]=f3_line_ram[0x1de0]&0xffff;
	sprintf(buf,"Sprt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*18,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1860]&0xffff;
	l[1]=f3_line_ram[0x18e0]&0xffff;
	l[2]=f3_line_ram[0x1960]&0xffff;
	l[3]=f3_line_ram[0x19e0]&0xffff;
	sprintf(buf,"Pivt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*19,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1060]&0xffff;
	l[1]=f3_line_ram[0x10e0]&0xffff;
	l[2]=f3_line_ram[0x1160]&0xffff;
	l[3]=f3_line_ram[0x11e0]&0xffff;
	sprintf(buf,"Colm: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*20,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1460]&0xffff;
	l[1]=f3_line_ram[0x14e0]&0xffff;
	l[2]=f3_line_ram[0x1560]&0xffff;
	l[3]=f3_line_ram[0x15e0]&0xffff;
	sprintf(buf,"5000: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*21,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"SPri: %04x",sprite_pri_word);
	for (j = 0;j < 10; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*23,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"TPri: %04x %04x %04x %04x",t0,t1,t2,t3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*24,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"Cstm: %04x %04x %04x %04x",c0,c1,c2,c3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*25,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"6000: %08x %08x %08x",f3_line_ram[0x1800],f3_line_ram[0x1890],f3_line_ram[0x1910]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*27,0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}

/******************************************************************************/

WRITE_HANDLER( f3_palette_24bit_w )
{
	int r,g,b;

	//ALEK COMBINE_DATA(&paletteram32[offset]);

	/* 12 bit palette games - there has to be a palette select bit somewhere */
	if (f3_game==SPCINVDX || f3_game==RIDINGF || f3_game==ARABIANM || f3_game==RINGRAGE) {
		b = 15 * ((paletteram32[offset] >> 4) & 0xf);
		g = 15 * ((paletteram32[offset] >> 8) & 0xf);
		r = 15 * ((paletteram32[offset] >> 12) & 0xf);
	}

	/* This is weird - why are only the sprites and VRAM palettes 21 bit? */
	else if (f3_game==CLEOPATR) {
		if (offset<0x100 || offset>0x1000) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* Another weird one */
	else if (f3_game==TWINQIX) {
		if (offset>0x1c00) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* All other games - standard 24 bit palette */
	else {
	 	r = (paletteram32[offset] >>16) & 0xff;
		g = (paletteram32[offset] >> 8) & 0xff;
		b = (paletteram32[offset] >> 0) & 0xff;
	}

	palette_change_color(offset,r,g,b);
}

/******************************************************************************/

static void f3_tilemap_draw(struct osd_bitmap *bitmap,struct tilemap *tilemap,UINT32 trans,int pos,int sx,int sy, int dpri)
{
	struct osd_bitmap *srcbitmap = tilemap_get_pixmap(tilemap);
	UINT16 *dst,*src;
	int x,y,inc;
	int x_index,y_index;
	int width_mask=0x1ff;
	int line_base,zoom_base,col_base,pri_base;
	int colscroll,x_offset,bit_select;
	UINT32 x_zoom,y_zoom,rot=0;
	UINT32 uses_xyzoom=f3_game_config->xyzoom;
	UINT16 pri;
	int dd;

	if (f3_game_config->extend)	width_mask=0x3ff;

	sx+=46;
	if (flipscreen) {
		line_base=0xa1fe + (pos*0x200);
		zoom_base=0x81fe + (pos*0x200);
		col_base =0x41fe + (pos*0x200);
		pri_base =0xb1fe + (pos*0x200);
		inc=-2;
		y_index=(-sy-256)<<16; /* Adjust for flipped scroll position */
		if (f3_game_config->extend)	sx=-sx+188-512; else sx=-sx+188; /* Adjust for flipped scroll position */
		y=255;
		if (Machine->orientation == ROT0) rot=ORIENTATION_FLIP_Y; else rot=ORIENTATION_FLIP_X;
	} else {
		line_base=0xa000 + (pos*0x200);
		zoom_base=0x8000 + (pos*0x200);
		col_base =0x4000 + (pos*0x200);
		pri_base =0xb000 + (pos*0x200);
		inc=2;
		y_index=sy<<16;
		y=0;
	}

	x_offset=sx<<16;
	colscroll=0;
	y_zoom=0x10000;
	x_zoom=0x10000;

	do {
		/* The zoom, column and row values can latch according to control ram */
		if (line_base&2) {
			bit_select=1<<pos;
			if (f3_line_ram[0x300+(y>>1)]&bit_select) x_offset=(sx<<16)+((f3_line_ram[line_base/4]&0xffc0)<<10);
			if (f3_line_ram[0x200+(y>>1)]&bit_select) {
				if (uses_xyzoom && (f3_line_ram[zoom_base/4]&0xffff)!=0) {
					x_zoom=0x10000 - ((((f3_line_ram[zoom_base/4]>> 0)&0xff00)>>8)*280 );
					y_zoom=0x10000 + ((((f3_line_ram[zoom_base/4]>> 0)&0x00ff)-0x80)*520);
				} else {
					y_zoom=x_zoom=0x10080 - ((f3_line_ram[zoom_base/4]>> 0)&0xffff);
				}
			}
			if (f3_line_ram[0x000+(y>>1)]&bit_select)
				colscroll=(f3_line_ram[col_base/4]>> 0)&0x1ff;
		} else {
			bit_select=0x10000<<pos;
			if (f3_line_ram[0x300+(y>>1)]&bit_select) x_offset=(sx<<16)+((f3_line_ram[line_base/4]&0xffc00000)>>6);
			if (f3_line_ram[0x200+(y>>1)]&bit_select) {
				if (uses_xyzoom && (f3_line_ram[zoom_base/4]>>16)!=0) {
					x_zoom=0x10000 - ((((f3_line_ram[zoom_base/4]>>16)&0xff00)>>8)*280 );
					y_zoom=0x10000 + ((((f3_line_ram[zoom_base/4]>>16)&0x00ff)-0x80)*520);
				} else {
					y_zoom=x_zoom=0x10080 - ((f3_line_ram[zoom_base/4]>>16)&0xffff);
				}
			}

			if (f3_line_ram[0x000+(y>>1)]&bit_select)
				colscroll=(f3_line_ram[col_base/4]>>16)&0x1ff;
		}

		/* Priority register for this line */
		if (line_base&2)
			pri=f3_line_ram[pri_base/4]&0xffff;
		else
			pri=(f3_line_ram[pri_base/4]>>16)&0xffff;

		dd=1;
		if (!flipscreen && y<24) dd=0;
		else if (flipscreen && y>232) dd=0;

		/* Check priority register for a disabled line */
		if (dd && (pri&0xf000)!=0x5000 && (pri&0xf000)!=0x1000 && (pri&0x800)!=0x800) {
			x_index=x_offset;
			x=x_index+(320*x_zoom); /* 320 pixels */
			dst=scanline;
			if (Machine->orientation == ROT0) {
				src=(unsigned short *)srcbitmap->line[((y_index>>16)+colscroll)&0x1ff];
				while (x_index<x) {
					*dst++ = src[(x_index>>16)&width_mask];
					x_index += x_zoom;
				}
			} else {
				while (x_index<x) { /* 270 degree rotation - Slow, slow, slow */
					src=(unsigned short *)srcbitmap->line[(width_mask-(x_index>>16))&width_mask];
					*dst++ = src[((y_index>>16)+1+colscroll)&0x1ff];
					x_index += x_zoom;
				}
			}

			if (trans==TILEMAP_IGNORE_TRANSPARENCY)
				pdraw_scanline16(bitmap,46,y,320,scanline,0,-1,rot,dpri);
			else
				pdraw_scanline16(bitmap,46,y,320,scanline,0,palette_transparent_pen,rot,dpri);
		}

		y_index += y_zoom;
		zoom_base+=inc;
		line_base+=inc;
		col_base +=inc;
		pri_base +=inc;
		if (flipscreen) y--; else y++;
	} while ( (flipscreen && y>=0) || (!flipscreen && y<256) );
}

/******************************************************************************/

static void f3_clip_top_border(struct osd_bitmap *bitmap)
{
	int pri_base,i;

	if (f3_game==TRSTAR) return; /* TRStar does not set pri RAM like the others */
	for (i=0; i<512; i++) scanline[i]=0;

	/* Lineram is reversed when screen is flipped */
	if (flipscreen)
		pri_base=0xb1fe;
	else
		pri_base=0xb000;

	for (i=0; i<256; i++) {
		/* If priority RAM for this line==0 or it's disabled then it should not be drawn */
		if (pri_base&2) {
			if ((f3_line_ram[pri_base/4]&0x0800)==0 && (f3_line_ram[pri_base/4]&0xffff)!=0)
				return;
		} else {
			if ((f3_line_ram[pri_base/4]&0x08000000)==0 && (f3_line_ram[pri_base/4]>>16)!=0)
				return;
		}

		draw_scanline16(bitmap,46,i,320+46,scanline,0,-1);
		if (flipscreen) pri_base-=2; else pri_base+=2;
	}
}

static void f3_clip_bottom_border(struct osd_bitmap *bitmap)
{
	int pri_base,i;

	if (f3_game==TRSTAR) return; /* TRStar does not set pri RAM like the others */
	for (i=0; i<512; i++) scanline[i]=0;

	/* Lineram is reversed when screen is flipped */
	if (flipscreen)
		pri_base=0xb000;
	else
		pri_base=0xb1fe;

	for (i=255; i>=0; i--) {
		/* If priority RAM for this line==0 or it's disabled then it should not be drawn */
		if (pri_base&2) {
			if ((f3_line_ram[pri_base/4]&0x0800)==0 && (f3_line_ram[pri_base/4]&0xffff)!=0)
				return;
		} else {
			if ((f3_line_ram[pri_base/4]&0x08000000)==0 && (f3_line_ram[pri_base/4]>>16)!=0)
				return;
		}

		/* Erase this line */
		draw_scanline16(bitmap,46,i,320+46,scanline,0,-1);
		if (flipscreen) pri_base+=2; else pri_base-=2;
	}
}

/******************************************************************************/

static void f3_draw_pivot_layer(struct osd_bitmap *bitmap)
{
	int mx,my,tile,sx,sy,col,fx=f3_game_config->pivot,i;
	int pivot_base,col_off,y_offs;
	struct rectangle pivot_clip;

	/* A poor way to guess if the pivot layer is enabled, but quicker than
		parsing control ram. */
	int ctrl  = f3_line_ram[0x180f]&0xa000; /* SpcInvDX sets only this address */
	int ctrl2 = f3_line_ram[0x1870]&0xa0000000; /* SpcInvDX flipscreen */
	int ctrl3 = f3_line_ram[0x1820]&0xa000; /* Other games set the whole range 0x6000-0x61ff */
	int ctrl4 = f3_line_ram[0x1840]&0xa000; /* ScFinals only sets a small range over the screen area */

	/* Quickly decide whether to process the rest of the pivot layer */
	if (!(ctrl || ctrl2 || ctrl3 || ctrl4)) {
		pivot_active=0;
		return;
	}
	pivot_active=1;

	if (flipscreen) {
		sx=-(f3_control_1[2]>>16)+12;
		sy=-(f3_control_1[2]&0xff);
		if (fx) fx=0; else fx=1;
		y_offs=0x100-(f3_control_1[2]&0x1ff)+scroll_kludge_y; //todo - not quite correct
	} else {
		sx=(f3_control_1[2]>>16)+5;
		sy=f3_control_1[2]&0xff;
		y_offs=(f3_control_1[2]&0x1ff)+scroll_kludge_y;
	}

	/* Clip top scanlines according to line ram - Bubble Memories makes use of this */
	pivot_clip.min_x=Machine->visible_area.min_x;
	pivot_clip.max_x=Machine->visible_area.max_x;
	pivot_clip.min_y=Machine->visible_area.min_y;
	pivot_clip.max_y=Machine->visible_area.max_y;
	if (flipscreen)
		pivot_base=0x61fe;
	else
		pivot_base=0x6000;

	for (i=0; i<256; i++) {
		/* Loop through until first visible line */
		if (pivot_base&2) {
			if ((f3_line_ram[pivot_base/4]&0xa000)==0xa000) {
				pivot_clip.min_y=i;
				i=256;
			}
		} else {
			if ((f3_line_ram[pivot_base/4]&0xa0000000)==0xa0000000) {
				pivot_clip.min_y=i;
				i=256;
			}
		}
		if (flipscreen) pivot_base-=2; else pivot_base+=2;
	}

	/* Update bitmap */
	if (pivot_changed)
		for (tile = 0;tile < 2048;tile++)
			if (pivot_dirty[tile]) {
				decodechar(Machine->gfx[3],tile,(UINT8 *)f3_pivot_ram,Machine->drv->gfxdecodeinfo[3].gfxlayout);
				if (flipscreen) {
					my=31-(tile%32);
					mx=63-(tile/32);
				} else {
					my=tile%32;
					mx=tile/32;
				}

				/* Colour is shared with VRAM layer */
				if ((((tile%32)*8 + y_offs)&0x1ff)>0xff)
					col_off=0x800+((tile%32)*0x40)+((tile&0xfe0)>>5);
				else
					col_off=((tile%32)*0x40)+((tile&0xfe0)>>5);

				if (col_off&1)
		        	col = ((videoram32[col_off>>1]&0xffff)>>9)&0x3f;
				else
					col = ((videoram32[col_off>>1]>>16)>>9)&0x3f;

				palette_used_colors[16 * col]=PALETTE_COLOR_TRANSPARENT;

				drawgfx(pixel_layer,Machine->gfx[3],
					tile,
					col,
					fx,flipscreen,
					mx*8,my*8,
					0,TRANSPARENCY_NONE,0);
				pivot_dirty[tile]=0;
			}
	pivot_changed=0;

	copyscrollbitmap(bitmap,pixel_layer,1,&sx,1,&sy,&pivot_clip,TRANSPARENCY_PEN,palette_transparent_pen);
}

/******************************************************************************/

static void f3_fix_transparent_colours(int p0, int p1, int p2, int p3)
{
 	int color, i, pos=0, index, max;

	/* Find layer with lowest priority - no transparency for this layer */
	for (i=0; i<0x10; i++) {
		if (p0==i) { if (f3_game_config->extend) pos=0x0000; else pos=0x0000; i=0x10; }
		if (p1==i) { if (f3_game_config->extend) pos=0x0800; else pos=0x0400; i=0x10; }
		if (p2==i) { if (f3_game_config->extend) pos=0x1000; else pos=0x0800; i=0x10; }
		if (p3==i) { if (f3_game_config->extend) pos=0x1800; else pos=0x0c00; i=0x10; }
	}

	/* Ensure any transparent 4bpp palettes don't interfere with opaque 5bpp or 6bpp layers */
	max=pos+0x400+(f3_game_config->extend*0x400);
	for (i=pos; i<max; i++) {
		color=f3_pf_data[i]>>16;
		index=16 * (color&0x1ff);
//		palette_used_colors[index+ 0] = PALETTE_COLOR_USED;
		palette_used_colors[index+16] = PALETTE_COLOR_USED;
//		palette_used_colors[index+32] = PALETTE_COLOR_USED;
//		palette_used_colors[index+48] = PALETTE_COLOR_USED;
	}
}

static void f3_draw_vram_layer(struct osd_bitmap *bitmap)
{
	int offs,mx,my,tile,color,fx,fy,sx,sy;

	sx=(f3_control_1[2]>>16)+5;
	sy=f3_control_1[2]&0xffff;

   	for (offs = 0; offs < 0x2000 ;offs += 2)
	{
		mx = (offs%128)/2;
		my = offs/128;

		if (offs&2)
        	tile = videoram32[offs>>2]&0xffff;
		else
			tile = videoram32[offs>>2]>>16;

        /* Transparency hack, 6010 for PB2, 1205 for PB3 */
		if (tile==0x6010 || tile==0x1205) continue;

        color = (tile>>9) &0x3f;
		fx = tile&0x0100;
		fy = tile&0x8000;

        tile&=0xff;
        if (!tile) continue;

		/* Graphics flip */
		if (flipscreen) {
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					504+17-(((8*mx)+sx)&0x1ff),504-(((8*my)+sy)&0x1ff),
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
		else
	        drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					((8*mx)+sx)&0x1ff,((8*my)+sy)&0x1ff,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/

static void f3_drawsprites(struct osd_bitmap *bitmap)
{
	int offs,spritecont,flipx,flipy,old_x,old_y,color,x,y;
	int sprite,global_x=0,global_y=0,subglobal_x=0,subglobal_y=0;
	int block_x=0, block_y=0;
	int last_color=0,last_x=0,last_y=0,block_zoom_x=0,block_zoom_y=0;
	int this_x,this_y;
	int y_addition=16, x_addition=16;
	int multi=0;
	int sprite_top;

	/*	pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	color=0;
    flipx=flipy=0;
    old_y=old_x=0;
    y=x=0;

	sprite_top=0x1000;
	for (offs = 0; offs < sprite_top; offs += 4)
	{
		/* Check if special command bit is set */
		if (spriteram32_buffered[offs+1] & 0x8000) {
			data32_t cntrl=(spriteram32_buffered[offs+2])&0xffff;
			flipscreen=cntrl&0x2000;

			/*	cntrl&0x1000 = disabled?  (From F2 driver, doesn't seem used anywhere)
				cntrl&0x0010 = ???
				cntrl&0x0020 = ???
			*/

			/* Sprite bank select */
			if (cntrl&1) {
				offs=offs|0x2000;
				sprite_top=sprite_top|0x2000;
			}
			continue;
		}

		/* Check if the sprite list jump command bit is set */
		if ((spriteram32_buffered[offs+3]>>16) & 0x8000) {
			data32_t jump = (spriteram32_buffered[offs+3]>>16)&0x3ff;

			offs=((offs&0x2000)|((jump<<4)/4));
			continue;
		}

		/* Set global sprite scroll */
		if (((spriteram32_buffered[offs+1]>>16) & 0xf000) == 0xa000) {
			global_x = (spriteram32_buffered[offs+1]>>16) & 0xfff;
			if (global_x >= 0x800) global_x -= 0x1000;
			global_y = spriteram32_buffered[offs+1] & 0xfff;
			if (global_y >= 0x800) global_y -= 0x1000;
		}

		/* And sub-global sprite scroll */
		if (((spriteram32_buffered[offs+1]>>16) & 0xf000) == 0x5000) {
			subglobal_x = (spriteram32_buffered[offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_buffered[offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
		}

		if (((spriteram32_buffered[offs+1]>>16) & 0xf000) == 0xb000) {
			subglobal_x = (spriteram32_buffered[offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_buffered[offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
			global_y=subglobal_y;
			global_x=subglobal_x;
		}

		/* A real sprite to process! */
		sprite = (spriteram32_buffered[offs]>>16) | ((spriteram32_buffered[offs+2]&1)<<16);
		spritecont = spriteram32_buffered[offs+2]>>24;

/* These games either don't set the XY control bits properly (68020 bug?), or
	have some different mode from the others */
#ifdef DARIUSG_KLUDGE
		if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR)
			multi=spritecont&0xf0;
#endif

		/* Check if this sprite is part of a continued block */
		if (multi) {
			/* Bit 0x4 is 'use previous colour' for this block part */
			if (spritecont&0x4) color=last_color;
			else color=(spriteram32_buffered[offs+2]>>16)&0xff;

#ifdef DARIUSG_KLUDGE
			if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR) {
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					if (spritecont & 0x4) {
						x = block_x;
					} else {
						this_x = spriteram32_buffered[offs+1]>>16;
						if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

						if ((spriteram32_buffered[offs+1]>>16)&0x8000) {
							this_x+=0;
						} else if ((spriteram32_buffered[offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_x+=global_x;
						} else { /* Apply both scroll offsets */
							this_x+=global_x+subglobal_x;
						}

						x = block_x = this_x;
					}
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
				}

				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					if (spritecont & 0x4) {
						y = block_y;
					} else {
						this_y = spriteram32_buffered[offs+1]&0xffff;
						if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;

						if ((spriteram32_buffered[offs+1]>>16)&0x8000) {
							this_y+=0;
						} else if ((spriteram32_buffered[offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_y+=global_y;
						} else { /* Apply both scroll offsets */
							this_y+=global_y+subglobal_y;
						}

						y = block_y = this_y;
					}
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
				}
			} else
#endif
			{
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					x = block_x;
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
				}
				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					y = block_y;
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
				}
				/* Both zero = reread block latch? */
			}
		}
		/* Else this sprite is the possible start of a block */
		else {
			color = (spriteram32_buffered[offs+2]>>16)&0xff;
			last_color=color;

			/* Sprite positioning */
			this_y = spriteram32_buffered[offs+1]&0xffff;
			this_x = spriteram32_buffered[offs+1]>>16;
			if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;
			if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

			/* Ignore both scroll offsets for this block */
			if ((spriteram32_buffered[offs+1]>>16)&0x8000) {
				this_x+=0;
				this_y+=0;
			} else if ((spriteram32_buffered[offs+1]>>16)&0x4000) {
				/* Ignore subglobal (but apply global) */
				this_x+=global_x;
				this_y+=global_y;
			} else { /* Apply both scroll offsets */
				this_x+=global_x+subglobal_x;
				this_y+=global_y+subglobal_y;
			}

	        block_y = y = this_y;
            block_x = x = this_x;
			y_addition=x_addition=16;

			block_zoom_x=spriteram32_buffered[offs];
			block_zoom_y=(block_zoom_x>>8)&0xff;
			block_zoom_x&=0xff;
		}

		/* These features are common to sprite and block parts */
  		flipx = spritecont&0x1;
		flipy = spritecont&0x2;
		multi = spritecont&0x8;
		last_x=x;
		last_y=y;
		if (block_zoom_x) x_addition=(0x110 - block_zoom_x) / 16; else x_addition=16;
		if (block_zoom_y) y_addition=(0x110 - block_zoom_y) / 16; else y_addition=16;
		if (!sprite) continue;

		if (flipscreen) {
			sprite_ptr->x = 512-16-x;
			sprite_ptr->y = 240-y;
			sprite_ptr->flipx = !flipx;
			sprite_ptr->flipy = !flipy;
		} else {
			sprite_ptr->x = x;
			sprite_ptr->y = y;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
		}

		if (DEBUG_F3 && keyboard_pressed(KEYCODE_A) && (color & 0xc0)==0xc0) color=rand()%0xff;
		if (DEBUG_F3 && keyboard_pressed(KEYCODE_S) && (color & 0xc0)==0x80) color=rand()%0xff;
		if (DEBUG_F3 && keyboard_pressed(KEYCODE_D) && (color & 0xc0)==0x40) color=rand()%0xff;
		if (DEBUG_F3 && keyboard_pressed(KEYCODE_F) && (color & 0xc0)==0x00) color=rand()%0xff;

		sprite_ptr->code = sprite;
		sprite_ptr->color = color;
		if (block_zoom_x) sprite_ptr->zoomx = ((0x10f-block_zoom_x)<<8)|0x80; else sprite_ptr->zoomx=0x10000;
		if (block_zoom_y) sprite_ptr->zoomy = ((0x10f-block_zoom_y)<<8)|0x80; else sprite_ptr->zoomy=0x10000;
		sprite_ptr->primask = primasks[(color & 0xc0) >> 6];
		sprite_ptr++;
	}

	/* Render using the priority buffer */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfxzoom(bitmap,Machine->gfx[2],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->zoomx,sprite_ptr->zoomy,
				sprite_ptr->primask);
	}
}

/******************************************************************************/

void f3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int use_custom[4],tpri[4],zoom[4];
	int sprite_pri[4],layer_pri[4],enable[4],alpha[4];
	unsigned int sy[4],sx[4],rs[4];
	int tile,pos,pos2,trans,i,inc,pri;
	int lay,ctrl;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Dynamically decode VRAM chars if dirty */
	if (vram_changed)
		for (tile = 0;tile < 256;tile++)
			if (vram_dirty[tile]) {
				decodechar(Machine->gfx[0],tile,(UINT8 *)f3_vram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				vram_dirty[tile]=0;
			}
	vram_changed=0;

	/* Calculate and apply scroll offsets */
	if (flipscreen) {
		sy[0]=0x300-((f3_control_0[2]&0xffff0000)>>23)-scroll_kludge_y;
		sy[1]=0x300-((f3_control_0[2]&0x0000ffff)>> 7)-scroll_kludge_y-pf23_y_kludge;
		sy[2]=0x300-((f3_control_0[3]&0xffff0000)>>23)-scroll_kludge_y-pf23_y_kludge;
		sy[3]=0x300-((f3_control_0[3]&0x0000ffff)>> 7)-scroll_kludge_y;

		sx[0]=-0x1a0-((f3_control_0[0]&0xffff0000)>>22)+6+scroll_kludge_x;
		sx[1]=-0x1a0-((f3_control_0[0]&0x0000ffff)>> 6)+10+scroll_kludge_x;
		sx[2]=-0x1a0-((f3_control_0[1]&0xffff0000)>>22)+14+scroll_kludge_x;
		sx[3]=-0x1a0-((f3_control_0[1]&0x0000ffff)>> 6)+18+scroll_kludge_x;

		/* Rowscroll - values can latch if the control bit isn't set per playfield per line */
		rs[0]=rs[1]=rs[2]=rs[3]=0;
		for (i=254; i>=0; i-=2) {
			ctrl=f3_line_ram[0x300+(i>>1)];

			if (ctrl&0x1) rs[0]=(f3_line_ram[0x2800+(i>>1)]>>6);
			if (ctrl&0x2) rs[1]=(f3_line_ram[0x2880+(i>>1)]>>6);
			if (ctrl&0x4) rs[2]=(f3_line_ram[0x2900+(i>>1)]>>6);
			if (ctrl&0x8) rs[3]=(f3_line_ram[0x2980+(i>>1)]>>6);

			tilemap_set_scrollx( pf1_tilemap,(i+0+sy[0])&0x1ff, sx[0]-rs[0] );
			tilemap_set_scrollx( pf2_tilemap,(i+0+sy[1])&0x1ff, sx[1]-rs[1] );
			tilemap_set_scrollx( pf3_tilemap,(i+0+sy[2])&0x1ff, sx[2]-rs[2] );
			tilemap_set_scrollx( pf4_tilemap,(i+0+sy[3])&0x1ff, sx[3]-rs[3] );

			if (ctrl&0x10000) rs[0]=f3_line_ram[0x2800+(i>>1)]>>22;
			if (ctrl&0x20000) rs[1]=f3_line_ram[0x2880+(i>>1)]>>22;
			if (ctrl&0x40000) rs[2]=f3_line_ram[0x2900+(i>>1)]>>22;
			if (ctrl&0x80000) rs[3]=f3_line_ram[0x2980+(i>>1)]>>22;

			tilemap_set_scrollx( pf1_tilemap,(i-1+sy[0])&0x1ff, sx[0]-rs[0] );
			tilemap_set_scrollx( pf2_tilemap,(i-1+sy[1])&0x1ff, sx[1]-rs[1] );
			tilemap_set_scrollx( pf3_tilemap,(i-1+sy[2])&0x1ff, sx[2]-rs[2] );
			tilemap_set_scrollx( pf4_tilemap,(i-1+sy[3])&0x1ff, sx[3]-rs[3] );
		}

		tilemap_set_scrolly( pf1_tilemap,0, sy[0] );
		tilemap_set_scrolly( pf2_tilemap,0, sy[1] );
		tilemap_set_scrolly( pf3_tilemap,0, sy[2] );
		tilemap_set_scrolly( pf4_tilemap,0, sy[3] );
	} else {
		sy[0]=((f3_control_0[2]&0xffff0000)>>23)+scroll_kludge_y;
		sy[1]=((f3_control_0[2]&0x0000ffff)>> 7)+scroll_kludge_y+pf23_y_kludge;
		sy[2]=((f3_control_0[3]&0xffff0000)>>23)+scroll_kludge_y+pf23_y_kludge;
		sy[3]=((f3_control_0[3]&0x0000ffff)>> 7)+scroll_kludge_y;

		sx[0]=((f3_control_0[0]&0xffff0000)>>22)-6-scroll_kludge_x;
		sx[1]=((f3_control_0[0]&0x0000ffff)>> 6)-10-scroll_kludge_x;
		sx[2]=((f3_control_0[1]&0xffff0000)>>22)-14-scroll_kludge_x;
		sx[3]=((f3_control_0[1]&0x0000ffff)>> 6)-18-scroll_kludge_x;

		/* Rowscroll - values can latch if the control bit isn't set per playfield per line */
		rs[0]=rs[1]=rs[2]=rs[3]=0;
		for (i=0; i<256; i+=2) {
			ctrl=f3_line_ram[0x300+(i>>1)];

			if (ctrl&0x10000) rs[0]=f3_line_ram[0x2800+(i>>1)]>>22;
			if (ctrl&0x20000) rs[1]=f3_line_ram[0x2880+(i>>1)]>>22;
			if (ctrl&0x40000) rs[2]=f3_line_ram[0x2900+(i>>1)]>>22;
			if (ctrl&0x80000) rs[3]=f3_line_ram[0x2980+(i>>1)]>>22;

			tilemap_set_scrollx( pf1_tilemap,(i+sy[0])&0x1ff, sx[0]+rs[0] );
			tilemap_set_scrollx( pf2_tilemap,(i+sy[1])&0x1ff, sx[1]+rs[1] );
			tilemap_set_scrollx( pf3_tilemap,(i+sy[2])&0x1ff, sx[2]+rs[2] );
			tilemap_set_scrollx( pf4_tilemap,(i+sy[3])&0x1ff, sx[3]+rs[3] );

			if (ctrl&0x1) rs[0]=(f3_line_ram[0x2800+(i>>1)]>>6);
			if (ctrl&0x2) rs[1]=(f3_line_ram[0x2880+(i>>1)]>>6);
			if (ctrl&0x4) rs[2]=(f3_line_ram[0x2900+(i>>1)]>>6);
			if (ctrl&0x8) rs[3]=(f3_line_ram[0x2980+(i>>1)]>>6);

			tilemap_set_scrollx( pf1_tilemap,(i+1+sy[0])&0x1ff, sx[0]+rs[0] );
			tilemap_set_scrollx( pf2_tilemap,(i+1+sy[1])&0x1ff, sx[1]+rs[1] );
			tilemap_set_scrollx( pf3_tilemap,(i+1+sy[2])&0x1ff, sx[2]+rs[2] );
			tilemap_set_scrollx( pf4_tilemap,(i+1+sy[3])&0x1ff, sx[3]+rs[3] );
		}

		tilemap_set_scrolly( pf1_tilemap,0, sy[0] );
		tilemap_set_scrolly( pf2_tilemap,0, sy[1] );
		tilemap_set_scrolly( pf3_tilemap,0, sy[2] );
		tilemap_set_scrolly( pf4_tilemap,0, sy[3] );
	}

	/* Update tilemaps */
	if (f3_game_config->extend) inc=0x800; else inc=0x400;
	gfx_base=f3_pf_data;
	tilemap_update(pf1_tilemap);
	gfx_base=f3_pf_data+inc;
	tilemap_update(pf2_tilemap);
	gfx_base=f3_pf_data+inc+inc;
	tilemap_update(pf3_tilemap);
	gfx_base=f3_pf_data+inc+inc+inc;
	tilemap_update(pf4_tilemap);

	/* Calculate relative playfield priorities - the real hardware can assign
		a different value to each scanline in each playfield but some games
		(Kaiser Knuckle, TRStars) only set the first scanline and expect it to
		apply to all scanlines (controlled by the RAM at the start of lineram
		in some way..

		For now, I'm going to use the first priority value for each playfield,
		this is correct 99% of the time (Landmaker continue screen is one
		exception).

		We also need to figure out whether to render the playfield using
		the standard Mame tilemap manager (supports rowscroll) or whether
		we need to use the custom renderer (rowscroll, colscoll, line zoom).

	*/

	use_custom[0]=use_custom[1]=use_custom[2]=use_custom[3]=0;

	/* Run over lineram to grab zoom & pri values for each layer */
	for (pos=0; pos<4; pos++) {
		int need_t,need_z,t_pos,z_pos;
		need_t=need_z=1;
		if (flipscreen) { t_pos=0xb1fe + (pos*0x200); z_pos=0x81fe + (pos*0x200); }
		else { t_pos=0xb000+(pos*0x200); z_pos=0x8000+(pos*0x200); }
		for (i=0; i<256; i++) {

			/* Some games leave bad values in top area (which is clipped anyway) */
			if (i==0) { i=clip_pri_kludge; t_pos+=clip_pri_kludge; z_pos+=clip_pri_kludge; }

			if ((i&1 && !flipscreen) || ((i&1)==0 && flipscreen)) {
				if (need_t) tpri[pos]=f3_line_ram[t_pos/4]&0xffff;
				if (need_z) zoom[pos]=f3_line_ram[z_pos/4]&0xffff;
			}
			else {
				if (need_t) tpri[pos]=f3_line_ram[t_pos/4]>>16;
				if (need_z) zoom[pos]=f3_line_ram[z_pos/4]>>16;
			}

			/* KTiger 2 needs the first _visible_ pri value - not a disabled line */
			if (tpri[pos]!=0 && (tpri[pos]&0x0900)!=0x0800) need_t=0;
			if (zoom[pos]!=0) need_z=0;
			if (flipscreen) { t_pos-=2; z_pos-=2; } else { t_pos+=2; z_pos+=2; }
			if (need_t==0 && need_z==0)
				i=256;
		}
	}

	/* Use custom renderer? Cheat & check commonly used locations for speed */
	if (zoom[0]!=0x80) use_custom[0]=1;
	else if ((f3_line_ram[0x8164/4]&0xffff)!=0x80 && (f3_line_ram[0x8164/4]&0xffff)) use_custom[0]=1;
	else if ((f3_line_ram[0x8038/4]>>16)!=0x80 && (f3_line_ram[0x8038/4]>>16)) use_custom[0]=1;
	else if ((f3_line_ram[0x81a0/4]>>16)!=0x80 && (f3_line_ram[0x81a0/4]>>16)) use_custom[0]=1;
	else if ((f3_line_ram[0x4040/4]&0xfff) || (f3_line_ram[0x4180/4]&0xfff) || (f3_line_ram[0x4080/4]&0xfff)) use_custom[0]=1;

	if (zoom[1]!=0x80) use_custom[1]=1;
	else if ((f3_line_ram[0x8300/4]&0xffff)!=0x80 && (f3_line_ram[0x8300/4]&0xffff)) use_custom[1]=1;
	else if ((f3_line_ram[0x8238/4]>>16)!=0x80 && (f3_line_ram[0x8238/4]>>16)) use_custom[1]=1;
	else if ((f3_line_ram[0x8390/4]>>16)!=0x80 && (f3_line_ram[0x8390/4]>>16)) use_custom[1]=1;
	else if ((f3_line_ram[0x4240/4]&0xfff) || (f3_line_ram[0x4380/4]&0xfff) || (f3_line_ram[0x4280/4]&0xfff)) use_custom[1]=1;

	if (zoom[2]!=0x80) use_custom[2]=1;
	else if ((f3_line_ram[0x8500/4]&0xffff)!=0x80 && (f3_line_ram[0x8500/4]&0xffff)) use_custom[2]=1;
	else if ((f3_line_ram[0x8438/4]>>16)!=0x80 && (f3_line_ram[0x8438/4]>>16)) use_custom[2]=1;
	else if ((f3_line_ram[0x4440/4]&0xfff) || (f3_line_ram[0x4580/4]&0xfff) || (f3_line_ram[0x4480/4]&0xfff)) use_custom[2]=1;

	if (zoom[3]!=0x80) use_custom[3]=1;
	else if ((f3_line_ram[0x8700/4]&0xffff)!=0x80 && (f3_line_ram[0x8700/4]&0xffff)) use_custom[3]=1;
	else if ((f3_line_ram[0x8638/4]>>16)!=0x80 && (f3_line_ram[0x8638/4]>>16)) use_custom[3]=1;
	else if ((f3_line_ram[0x4640/4]&0xfff) || (f3_line_ram[0x4780/4]&0xfff) || (f3_line_ram[0x4680/4]&0xfff)) use_custom[3]=1;

	enable[0]=enable[1]=enable[2]=enable[3]=1;
	if ((tpri[0]&0xf000)==0 || (tpri[0]&0xf000)==0x1000 || (tpri[0]&0xf000)==0x5000 || (tpri[0]&0xf000)==0xf000) enable[0]=0;
	if ((tpri[1]&0xf000)==0 || (tpri[1]&0xf000)==0x1000 || (tpri[1]&0xf000)==0x5000 || (tpri[1]&0xf000)==0xf000) enable[1]=0;
	if ((tpri[2]&0xf000)==0 || (tpri[2]&0xf000)==0x1000 || (tpri[2]&0xf000)==0x5000 || (tpri[2]&0xf000)==0xf000) enable[2]=0;
	if ((tpri[3]&0xf000)==0 || (tpri[3]&0xf000)==0x1000 || (tpri[3]&0xf000)==0x5000 || (tpri[3]&0xf000)==0xf000) enable[3]=0;

#if TRY_ALPHA
	alpha[0]=tpri[0]&0x8000;
	alpha[1]=tpri[1]&0x8000;
	alpha[2]=tpri[2]&0x8000;
	alpha[3]=tpri[3]&0x8000;
#else
	alpha[0]=alpha[1]=alpha[2]=alpha[3]=0;
#endif

	/* These games have a solid white alpha layer which is of course opaque until we support it - disable for now */
	if ((tpri[3]&0x8000) && (f3_game==TWINQIX || f3_game==POPNPOP || f3_game==ARKRETRN)) enable[3]=0;
	if ((tpri[2]&0x8000) && f3_game==QUIZHUHU) enable[2]=0;
	if ((tpri[3]&0x100) && f3_game==BUBBLEM) enable[3]=0; /* Bosses have an alpha layer on top of everything */

	/* We don't need to mark pens in 16 or 24 bit modes, only transparent colours */
	f3_fix_transparent_colours(tpri[0]&0xf,tpri[1]&0xf,tpri[2]&0xf,tpri[3]&0xf);

	/* Dirty cached pivot layer on palette changes */
	if (palette_recalc()) {
		for (i=0; i<2048; i++)
			pivot_dirty[i]=1;
		pivot_changed=1;
	}

	if (DEBUG_F3 && keyboard_pressed(KEYCODE_Q))
		use_custom[0]=use_custom[1]=use_custom[2]=use_custom[3]=1;
	if (DEBUG_F3 && keyboard_pressed(KEYCODE_W))
		use_custom[0]=use_custom[1]=use_custom[2]=use_custom[3]=0;

	/* Some debug tools - keypresses to enable/disable layers */
	if (DEBUG_F3 && keyboard_pressed(KEYCODE_Z)) enable[0]=enable[0]^1;
	if (DEBUG_F3 && keyboard_pressed(KEYCODE_X)) enable[1]=enable[1]^1;
	if (DEBUG_F3 && keyboard_pressed(KEYCODE_C)) enable[2]=enable[2]^1;
	if (DEBUG_F3 && keyboard_pressed(KEYCODE_V)) enable[3]=enable[3]^1;

	/* Draw playfields, there are 16 priority levels, we also keep track
		of layer ordering so it can be applied to the sprite rendering */
	pri=1;
	lay=0;
	fillbitmap(priority_bitmap,0,NULL);
	trans=TILEMAP_IGNORE_TRANSPARENCY; /* First playfield will be drawn opaque */
	for (i=0; i<0x10; i++) {
		if ((tpri[0]&0xf)==i && enable[0] && !alpha[0]) {
			if (use_custom[0])
				f3_tilemap_draw(bitmap,pf1_tilemap,trans,0,sx[0],sy[0],pri);
			else
				tilemap_draw(bitmap,pf1_tilemap,trans,pri);
			trans=0;
			pri<<=1;
			layer_pri[lay++]=tpri[0]&0xf;
		}

		if ((tpri[1]&0xf)==i && enable[1] && !alpha[1]) {
			if (use_custom[1])
				f3_tilemap_draw(bitmap,pf2_tilemap,trans,1,sx[1],sy[1],pri);
			else
				tilemap_draw(bitmap,pf2_tilemap,trans,pri);
			trans=0;
			pri<<=1;
			layer_pri[lay++]=tpri[1]&0xf;
		}

		if ((tpri[2]&0xf)==i && enable[2] && !alpha[2]) {
			if (use_custom[2])
				f3_tilemap_draw(bitmap,pf3_tilemap,trans,2,sx[2],sy[2],pri);
			else
				tilemap_draw(bitmap,pf3_tilemap,trans,pri);
			trans=0;
			pri<<=1;
			layer_pri[lay++]=tpri[2]&0xf;
		}

		if ((tpri[3]&0xf)==i && enable[3] && !alpha[3]) {
			if (use_custom[3])
				f3_tilemap_draw(bitmap,pf4_tilemap,trans,3,sx[3],sy[3],pri);
			else
				tilemap_draw(bitmap,pf4_tilemap,trans,pri);
			pri<<=1;
			trans=0;
			layer_pri[lay++]=tpri[3]&0xf;
		}
	}

	/* Calculate priority masks for the sprites relative to the layers:

		The real hardware can assign different priority values per scanline,
		thankfully this is rarely (if ever) used, so we can just take one value
		and apply that to all sprites.  Most games set every scanline to be the
		same value, but some only set 1 value at the start and expect it to apply
		to all lines (this is controlled by the RAM at the start of lineram).

		Sprite priority relative to VRAM & Pixel layers is not implemented.

	*/
	sprite_pri_word=0;
	if (flipscreen) { pos=0x5fe; pos2=0x77fe; inc=-2; } else { pos=0x400; pos2=0x7600; inc=2; }
	for (i=0; i<224; i++) { /* 224 instead of 256 as some games leave garbage at the bottom */
		int spri;

		if ((i&1 && !flipscreen) || ((i&1)==0 && flipscreen)) {
			ctrl=f3_line_ram[pos /4]&0xffff;
			spri=f3_line_ram[pos2/4]&0xffff;
		}
		else {
			ctrl=f3_line_ram[pos /4]>>16;
			spri=f3_line_ram[pos2/4]>>16;
		}

		if (flipscreen) { pos-=2; pos2-=2; } else { pos+=2; pos2+=2; }
		/* When the bottom 4 bits of control ram are NOT set, then the corresponding
			priority value is invalid, and the last _good_ value is used */
		if ((ctrl&0xf) && spri)
			sprite_pri_word=spri;
	}

	/* Arkanoid Returns kludge - the bats are under a playfield!? */
	if (f3_game==ARKRETRN && sprite_pri_word==0x369c) sprite_pri_word=0x669c;

	sprite_pri[3]=(sprite_pri_word>>12)&0xf;
	sprite_pri[2]=(sprite_pri_word>> 8)&0xf;
	sprite_pri[1]=(sprite_pri_word>> 4)&0xf;
	sprite_pri[0]=(sprite_pri_word>> 0)&0xf;
	primasks[0]=primasks[1]=primasks[2]=primasks[3]=0;
	for (i = 0;i < 4;i++) {
		if (sprite_pri[i] < layer_pri[0]) primasks[i] |= 0xaaaa;
		if (sprite_pri[i] < layer_pri[1]) primasks[i] |= 0xcccc;
		if (sprite_pri[i] < layer_pri[2]) primasks[i] |= 0xf0f0;
		if (sprite_pri[i] < layer_pri[3]) primasks[i] |= 0xff00;
	}

	/* Pixel layer is drawn as 8x8 chars */
	if (DEBUG_F3) {
		if (!keyboard_pressed(KEYCODE_N))
			f3_draw_pivot_layer(bitmap);
		if (!keyboard_pressed(KEYCODE_B))
			f3_drawsprites(bitmap);
		if (!keyboard_pressed(KEYCODE_M))
			f3_draw_vram_layer(bitmap);
	} else {
		f3_draw_pivot_layer(bitmap);
		f3_drawsprites(bitmap);
		f3_draw_vram_layer(bitmap);
	}

#if TRY_ALPHA
	pri=1;
	lay=0;
	trans=TILEMAP_ALPHA;
	for (i=0; i<0x10; i++) {
		if ((tpri[0]&0xf)==i && enable[0] && alpha[0]) {
			if (use_custom[0])
				f3_tilemap_draw(bitmap,pf1_tilemap,trans,0,sx[0],sy[0],pri);
			else
				tilemap_draw(bitmap,pf1_tilemap,trans,pri);
			pri<<=1;
		}

		if ((tpri[1]&0xf)==i && enable[1] && alpha[1]) {
			if (use_custom[1])
				f3_tilemap_draw(bitmap,pf2_tilemap,trans,1,sx[1],sy[1],pri);
			else
				tilemap_draw(bitmap,pf2_tilemap,trans,pri);
			pri<<=1;
		}

		if ((tpri[2]&0xf)==i && enable[2] && alpha[2]) {
			if (use_custom[2])
				f3_tilemap_draw(bitmap,pf3_tilemap,trans,2,sx[2],sy[2],pri);
			else
				tilemap_draw(bitmap,pf3_tilemap,trans,pri);
			pri<<=1;
		}

		if ((tpri[3]&0xf)==i && enable[3] && alpha[3]) {
			if (use_custom[3])
				f3_tilemap_draw(bitmap,pf4_tilemap,trans,3,sx[3],sy[3],pri);
			else
				tilemap_draw(bitmap,pf4_tilemap,trans,pri);
			pri<<=1;
		}
	}
#endif

	/* Clip borders - values are taken from Layer 0 priority RAM, which is
		perhaps not 100% accurate - they could potentially vary between layers */
	f3_clip_top_border(bitmap);
	f3_clip_bottom_border(bitmap);

	if (DEBUG_F3 && keyboard_pressed(KEYCODE_0))
		print_debug_info(tpri[0],tpri[1],tpri[2],tpri[3],use_custom[0],use_custom[1],use_custom[2],use_custom[3]);
}

/******************************************************************************/

static void get_tile_info(int tile_index)
{
	int tile,color;

	color=gfx_base[tile_index]>>16;
	tile=gfx_base[tile_index];

	/*ALEK SET_TILE_INFO(
			1,
			tile&0xffff,
			color&0x1ff,
			TILE_FLIPYX( color >>14 ))*/
}

WRITE_HANDLER( f3_pf_data_w )
{
	//ALEK COMBINE_DATA(&f3_pf_data[offset]);

#if TRY_ALPHA==0
	/* This is a bit silly, but is needed to make the zoom layers transparency work */
	palette_used_colors[16 * ((data>>16)&0x1ff)] = PALETTE_COLOR_TRANSPARENT;
#endif

	if (f3_game_config->extend) {
		if (offset<0x800) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0800);
		else if (offset<0x1800) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x1000);
		else if (offset<0x2000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0x1800);
	} else {
		if (offset<0x400) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x800) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0400);
		else if (offset<0xc00) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x0800);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0xc00);
	}
}

WRITE_HANDLER( f3_control_0_w )
{
	//ALEK COMBINE_DATA(&f3_control_0[offset]);
}

WRITE_HANDLER( f3_control_1_w )
{
	//ALEK COMBINE_DATA(&f3_control_1[offset]);
}

WRITE_HANDLER( f3_videoram_w )
{
	int tile,col_off;
	//ALEK COMBINE_DATA(&videoram32[offset]);

	if (offset>0x3ff) offset-=0x400;

	tile=offset<<1;
	col_off=((tile%32)*0x40)+((tile&0xfe0)>>5);

	pivot_dirty[col_off]=1;
	pivot_dirty[col_off+1]=1;
	if (pivot_active) {
		palette_used_colors[16 * (((videoram32[offset]>>16)>>9)&0x3f)] = PALETTE_COLOR_TRANSPARENT;
		palette_used_colors[16 * (((videoram32[offset]>>0)>>9)&0x3f)] = PALETTE_COLOR_TRANSPARENT;
	}
	pivot_changed=1;
}

WRITE_HANDLER( f3_vram_w )
{
	//ALEK COMBINE_DATA(&f3_vram[offset]);
	vram_dirty[offset/8]=1;
	vram_changed=1;
}

WRITE_HANDLER( f3_pivot_w )
{
	//ALEK COMBINE_DATA(&f3_pivot_ram[offset]);
	pivot_dirty[offset/8]=1;
	pivot_changed=1;
}

/******************************************************************************/

void f3_eof_callback(void)
{
	memcpy(spriteram32_buffered,spriteram32,spriteram_size);
}

void f3_vh_stop (void)
{
	if (DEBUG_F3) {
		FILE *fp;

		fp=fopen("line.dmp","wb");
		if (fp) {
			fwrite(f3_line_ram,0x10000/4, 4 ,fp);
			fclose(fp);
		}
		fp=fopen("sprite.dmp","wb");
		if (fp) {
			fwrite(spriteram32,0x10000/4, 4 ,fp);
			fclose(fp);
		}
		fp=fopen("vram.dmp","wb");
		if (fp) {
			fwrite(f3_pf_data,0xc000, 1 ,fp);
			fclose(fp);
		}
	}

	if (scanline) {
		free(scanline);
		scanline=0;
	}
	if (pixel_layer) {
		bitmap_free(pixel_layer);
		pixel_layer=0;
	}
	if (spritelist) {
		free(spritelist);
		spritelist=0;
	}
	if (spriteram32_buffered) {
		free(spriteram32_buffered);
		spriteram32_buffered=0;
	}
	if (pivot_dirty) {
		free(pivot_dirty);
		pivot_dirty=0;
	}
}

int f3_vh_start(void)
{
	struct F3config *pCFG=&f3_config_table[0];
	int tile;

	scanline=0;
	pixel_layer=0;
	spritelist=0;
	spriteram32_buffered=0;
	pivot_dirty=0;

	/* Setup individual game */
	do {
		if (pCFG->name==f3_game)
		{
			break;
		}
		pCFG++;
	} while(pCFG->name);

	f3_game_config=pCFG;

	if (f3_game_config->extend) {
		pf1_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf2_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf3_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf4_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	} else {
		pf1_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf2_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf3_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf4_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	}

	scanline = (UINT16 *)malloc(1024);
	spriteram32_buffered = (UINT32 *)malloc(0x10000);
	spritelist = malloc(0x400 * sizeof(*spritelist));
	pixel_layer = bitmap_alloc (512, 256);
	pivot_dirty = (UINT8 *)malloc(2048);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap
		 || !spritelist || !pixel_layer || !spriteram32_buffered || !scanline || !pivot_dirty)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);

	tilemap_set_scroll_rows(pf1_tilemap,512);
	tilemap_set_scroll_rows(pf2_tilemap,512);
	tilemap_set_scroll_rows(pf3_tilemap,512);
	tilemap_set_scroll_rows(pf4_tilemap,512);

	/* Y Offset is related to the first visible line in line ram in some way */
	scroll_kludge_y=f3_game_config->sy;
	scroll_kludge_x=f3_game_config->sx;

	/* Palettes have 4 bpp indexes despite up to 6 bpp data */
	Machine->gfx[1]->color_granularity=16;
	Machine->gfx[2]->color_granularity=16;

	flipscreen = 0;
	memset(spriteram32_buffered,0,spriteram_size);
	memset(spriteram32,0,spriteram_size);

	//ALEK state_save_register_UINT32("f3", 0, "vcontrol0", f3_control_0, 8);
	//ALEK state_save_register_UINT32("f3", 0, "vcontrol1", f3_control_1, 8);

	/* Why?!??  These games have different offsets for the two middle playfields only */
	if (f3_game==LANDMAKR || f3_game==EACTION2 || f3_game==DARIUSG || f3_game==GEKIRIDO)
		pf23_y_kludge=23;
	else
		pf23_y_kludge=0;

	/* These games leave garbage pri values in the unused lines at the top - skip them */
	if (f3_game==PBOBBLE4 || f3_game==PBOBBLE2 || f3_game==LANDMAKR || f3_game==GEKIRIDO)
		clip_pri_kludge=0x30;
	else if (f3_game==BUBBLEM)
		clip_pri_kludge=0x60;
	else
		clip_pri_kludge=0;

	for (tile = 0;tile < 256;tile++)
		vram_dirty[tile]=1;
	for (tile = 0;tile < 2048;tile++)
		pivot_dirty[tile]=1;

#if TRY_ALPHA
	alpha_set_level(0x80);
#endif

	return 0;
}

#ifdef __cplusplus
}
#endif
