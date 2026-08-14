/*
 *  VIC.cpp - 6569R5 emulation (line based)
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Notes:
 * ------
 *
 *  - The EmulateLine() function is called for every emulated
 *    raster line. It computes one pixel row of the graphics
 *    according to the current VIC register settings and returns
 *    the number of cycles available for the CPU in that line.
 *  - The graphics are output into an 8 bit chunky bitmap
 *  - The sprite-graphics priority handling and collision
 *    detection is done in a bit-oriented way with masks.
 *    The foreground/background pixel mask for the graphics
 *    is stored in the fore_mask_buf[] array. Multicolor
 *    sprites are converted from their original chunky format
 *    to a bitplane representation (two bit masks) for easier
 *    handling of priorities and collisions.
 *  - The sprite-sprite priority handling and collision
 *    detection is done in with the byte array spr_coll_buf[],
 *    that is used to keep track of which sprites are already
 *    visible at certain X positions.
 *
 * Incompatibilities:
 * ------------------
 *
 *  - Raster effects that are achieved by modifying VIC registers
 *    in the middle of a raster line cannot be emulated
 *  - Sprite collisions are only detected within the visible
 *    screen area (excluding borders)
 *  - Sprites are only drawn if they completely fit within the
 *    left/right limits of the chunky bitmap
 *  - The Char ROM is not visible in the bitmap displays at
 *    addresses $0000 and $8000
 *  - The IRQ is cleared on every write access to the flag
 *    register. This is a hack for the RMW instructions of the
 *    6510 that first write back the original value.
 */

#include "sysdeps.h"

#include "VIC.h"
#include "C64.h"
#include "CPUC64.h"
#include "Display.h"
#include "Prefs.h"


// Test alignment on run-time for processors that can't access unaligned:
#undef ALIGNMENT_CHECK

// First and last displayed line
const unsigned FIRST_DISP_LINE = 0x10;
const unsigned LAST_DISP_LINE = 0x11f;

// First and last possible line for Bad Lines
const unsigned FIRST_DMA_LINE = 0x30;
const unsigned LAST_DMA_LINE = 0xf7;

// Display window coordinates
const int ROW25_YSTART = 0x33;
const int ROW25_YSTOP = 0xfb;
const int ROW24_YSTART = 0x37;
const int ROW24_YSTOP = 0xf7;

const int COL40_XSTART = 0x20;
const int COL40_XSTOP = 0x160;
const int COL38_XSTART = 0x27;
const int COL38_XSTOP = 0x157;


// Tables for sprite X expansion
uint16_t ExpTable[256] = {
	0x0000, 0x0003, 0x000C, 0x000F, 0x0030, 0x0033, 0x003C, 0x003F,
	0x00C0, 0x00C3, 0x00CC, 0x00CF, 0x00F0, 0x00F3, 0x00FC, 0x00FF,
	0x0300, 0x0303, 0x030C, 0x030F, 0x0330, 0x0333, 0x033C, 0x033F,
	0x03C0, 0x03C3, 0x03CC, 0x03CF, 0x03F0, 0x03F3, 0x03FC, 0x03FF,
	0x0C00, 0x0C03, 0x0C0C, 0x0C0F, 0x0C30, 0x0C33, 0x0C3C, 0x0C3F,
	0x0CC0, 0x0CC3, 0x0CCC, 0x0CCF, 0x0CF0, 0x0CF3, 0x0CFC, 0x0CFF,
	0x0F00, 0x0F03, 0x0F0C, 0x0F0F, 0x0F30, 0x0F33, 0x0F3C, 0x0F3F,
	0x0FC0, 0x0FC3, 0x0FCC, 0x0FCF, 0x0FF0, 0x0FF3, 0x0FFC, 0x0FFF,
	0x3000, 0x3003, 0x300C, 0x300F, 0x3030, 0x3033, 0x303C, 0x303F,
	0x30C0, 0x30C3, 0x30CC, 0x30CF, 0x30F0, 0x30F3, 0x30FC, 0x30FF,
	0x3300, 0x3303, 0x330C, 0x330F, 0x3330, 0x3333, 0x333C, 0x333F,
	0x33C0, 0x33C3, 0x33CC, 0x33CF, 0x33F0, 0x33F3, 0x33FC, 0x33FF,
	0x3C00, 0x3C03, 0x3C0C, 0x3C0F, 0x3C30, 0x3C33, 0x3C3C, 0x3C3F,
	0x3CC0, 0x3CC3, 0x3CCC, 0x3CCF, 0x3CF0, 0x3CF3, 0x3CFC, 0x3CFF,
	0x3F00, 0x3F03, 0x3F0C, 0x3F0F, 0x3F30, 0x3F33, 0x3F3C, 0x3F3F,
	0x3FC0, 0x3FC3, 0x3FCC, 0x3FCF, 0x3FF0, 0x3FF3, 0x3FFC, 0x3FFF,
	0xC000, 0xC003, 0xC00C, 0xC00F, 0xC030, 0xC033, 0xC03C, 0xC03F,
	0xC0C0, 0xC0C3, 0xC0CC, 0xC0CF, 0xC0F0, 0xC0F3, 0xC0FC, 0xC0FF,
	0xC300, 0xC303, 0xC30C, 0xC30F, 0xC330, 0xC333, 0xC33C, 0xC33F,
	0xC3C0, 0xC3C3, 0xC3CC, 0xC3CF, 0xC3F0, 0xC3F3, 0xC3FC, 0xC3FF,
	0xCC00, 0xCC03, 0xCC0C, 0xCC0F, 0xCC30, 0xCC33, 0xCC3C, 0xCC3F,
	0xCCC0, 0xCCC3, 0xCCCC, 0xCCCF, 0xCCF0, 0xCCF3, 0xCCFC, 0xCCFF,
	0xCF00, 0xCF03, 0xCF0C, 0xCF0F, 0xCF30, 0xCF33, 0xCF3C, 0xCF3F,
	0xCFC0, 0xCFC3, 0xCFCC, 0xCFCF, 0xCFF0, 0xCFF3, 0xCFFC, 0xCFFF,
	0xF000, 0xF003, 0xF00C, 0xF00F, 0xF030, 0xF033, 0xF03C, 0xF03F,
	0xF0C0, 0xF0C3, 0xF0CC, 0xF0CF, 0xF0F0, 0xF0F3, 0xF0FC, 0xF0FF,
	0xF300, 0xF303, 0xF30C, 0xF30F, 0xF330, 0xF333, 0xF33C, 0xF33F,
	0xF3C0, 0xF3C3, 0xF3CC, 0xF3CF, 0xF3F0, 0xF3F3, 0xF3FC, 0xF3FF,
	0xFC00, 0xFC03, 0xFC0C, 0xFC0F, 0xFC30, 0xFC33, 0xFC3C, 0xFC3F,
	0xFCC0, 0xFCC3, 0xFCCC, 0xFCCF, 0xFCF0, 0xFCF3, 0xFCFC, 0xFCFF,
	0xFF00, 0xFF03, 0xFF0C, 0xFF0F, 0xFF30, 0xFF33, 0xFF3C, 0xFF3F,
	0xFFC0, 0xFFC3, 0xFFCC, 0xFFCF, 0xFFF0, 0xFFF3, 0xFFFC, 0xFFFF
};

uint16_t MultiExpTable[256] = {
	0x0000, 0x0005, 0x000A, 0x000F, 0x0050, 0x0055, 0x005A, 0x005F,
	0x00A0, 0x00A5, 0x00AA, 0x00AF, 0x00F0, 0x00F5, 0x00FA, 0x00FF,
	0x0500, 0x0505, 0x050A, 0x050F, 0x0550, 0x0555, 0x055A, 0x055F,
	0x05A0, 0x05A5, 0x05AA, 0x05AF, 0x05F0, 0x05F5, 0x05FA, 0x05FF,
	0x0A00, 0x0A05, 0x0A0A, 0x0A0F, 0x0A50, 0x0A55, 0x0A5A, 0x0A5F,
	0x0AA0, 0x0AA5, 0x0AAA, 0x0AAF, 0x0AF0, 0x0AF5, 0x0AFA, 0x0AFF,
	0x0F00, 0x0F05, 0x0F0A, 0x0F0F, 0x0F50, 0x0F55, 0x0F5A, 0x0F5F,
	0x0FA0, 0x0FA5, 0x0FAA, 0x0FAF, 0x0FF0, 0x0FF5, 0x0FFA, 0x0FFF,
	0x5000, 0x5005, 0x500A, 0x500F, 0x5050, 0x5055, 0x505A, 0x505F,
	0x50A0, 0x50A5, 0x50AA, 0x50AF, 0x50F0, 0x50F5, 0x50FA, 0x50FF,
	0x5500, 0x5505, 0x550A, 0x550F, 0x5550, 0x5555, 0x555A, 0x555F,
	0x55A0, 0x55A5, 0x55AA, 0x55AF, 0x55F0, 0x55F5, 0x55FA, 0x55FF,
	0x5A00, 0x5A05, 0x5A0A, 0x5A0F, 0x5A50, 0x5A55, 0x5A5A, 0x5A5F,
	0x5AA0, 0x5AA5, 0x5AAA, 0x5AAF, 0x5AF0, 0x5AF5, 0x5AFA, 0x5AFF,
	0x5F00, 0x5F05, 0x5F0A, 0x5F0F, 0x5F50, 0x5F55, 0x5F5A, 0x5F5F,
	0x5FA0, 0x5FA5, 0x5FAA, 0x5FAF, 0x5FF0, 0x5FF5, 0x5FFA, 0x5FFF,
	0xA000, 0xA005, 0xA00A, 0xA00F, 0xA050, 0xA055, 0xA05A, 0xA05F,
	0xA0A0, 0xA0A5, 0xA0AA, 0xA0AF, 0xA0F0, 0xA0F5, 0xA0FA, 0xA0FF,
	0xA500, 0xA505, 0xA50A, 0xA50F, 0xA550, 0xA555, 0xA55A, 0xA55F,
	0xA5A0, 0xA5A5, 0xA5AA, 0xA5AF, 0xA5F0, 0xA5F5, 0xA5FA, 0xA5FF,
	0xAA00, 0xAA05, 0xAA0A, 0xAA0F, 0xAA50, 0xAA55, 0xAA5A, 0xAA5F,
	0xAAA0, 0xAAA5, 0xAAAA, 0xAAAF, 0xAAF0, 0xAAF5, 0xAAFA, 0xAAFF,
	0xAF00, 0xAF05, 0xAF0A, 0xAF0F, 0xAF50, 0xAF55, 0xAF5A, 0xAF5F,
	0xAFA0, 0xAFA5, 0xAFAA, 0xAFAF, 0xAFF0, 0xAFF5, 0xAFFA, 0xAFFF,
	0xF000, 0xF005, 0xF00A, 0xF00F, 0xF050, 0xF055, 0xF05A, 0xF05F,
	0xF0A0, 0xF0A5, 0xF0AA, 0xF0AF, 0xF0F0, 0xF0F5, 0xF0FA, 0xF0FF,
	0xF500, 0xF505, 0xF50A, 0xF50F, 0xF550, 0xF555, 0xF55A, 0xF55F,
	0xF5A0, 0xF5A5, 0xF5AA, 0xF5AF, 0xF5F0, 0xF5F5, 0xF5FA, 0xF5FF,
	0xFA00, 0xFA05, 0xFA0A, 0xFA0F, 0xFA50, 0xFA55, 0xFA5A, 0xFA5F,
	0xFAA0, 0xFAA5, 0xFAAA, 0xFAAF, 0xFAF0, 0xFAF5, 0xFAFA, 0xFAFF,
	0xFF00, 0xFF05, 0xFF0A, 0xFF0F, 0xFF50, 0xFF55, 0xFF5A, 0xFF5F,
	0xFFA0, 0xFFA5, 0xFFAA, 0xFFAF, 0xFFF0, 0xFFF5, 0xFFFA, 0xFFFF
};

static union {
	struct {
		uint8_t a,b,c,d;
	} a;
	uint32_t b;
} TextColorTable[16][16][256][2];


/*
 *  Constructor: Initialize variables
 */

static void init_text_color_table(uint8_t *colors)
{
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			for (int k = 0; k < 256; k++) {
				TextColorTable[i][j][k][0].a.a = colors[k & 128 ? i : j];
				TextColorTable[i][j][k][0].a.b = colors[k & 64 ? i : j];
				TextColorTable[i][j][k][0].a.c = colors[k & 32 ? i : j];
				TextColorTable[i][j][k][0].a.d = colors[k & 16 ? i : j];
				TextColorTable[i][j][k][1].a.a = colors[k & 8 ? i : j];
				TextColorTable[i][j][k][1].a.b = colors[k & 4 ? i : j];
				TextColorTable[i][j][k][1].a.c = colors[k & 2 ? i : j];
				TextColorTable[i][j][k][1].a.d = colors[k & 1 ? i : j];
			}
		}
	}
}

MOS6569::MOS6569(C64 *c64, Display *disp, MOS6510 *CPU, uint8_t *RAM, uint8_t *Char, uint8_t *Color)
	: ram(RAM), char_rom(Char), color_ram(Color), the_c64(c64), the_display(disp), the_cpu(CPU)
{
	// Set pointers
	matrix_base = RAM;
	char_base = RAM;
	bitmap_base = RAM;

	// Get bitmap info
	chunky_line_start = disp->BitmapBase();
	xmod = disp->BitmapXMod();

	// Initialize VIC registers
	mx8 = 0;
	ctrl1 = ctrl2 = 0;
	lpx = lpy = 0;
	me = mxe = mye = mdp = mmc = 0;
	vbase = irq_flag = irq_mask = 0;
	clx_spr = clx_bgr = 0;
	cia_vabase = 0;
	ec = b0c = b1c = b2c = b3c = mm0 = mm1 = 0;
	for (unsigned i = 0; i < 8; ++i) {
		mx[i] = my[i] = sc[i] = 0;
	}

	// Initialize other variables
	raster_y = 0xffff;
	rc = 7;
	irq_raster = vc = vc_base = x_scroll = y_scroll = 0;
	dy_start = ROW24_YSTART;
	dy_stop = ROW24_YSTOP;

	display_idx = 0;
	display_state = false;
	border_on = false;
	bad_lines_enabled = false;
	lp_triggered = false;

	sprite_on = 0;
	for (unsigned i = 0; i < 8; ++i) {
		mc[i] = 63;
	}

	// Clear foreground mask
	memset(fore_mask_buf, 0, DISPLAY_X/8);

	// Set one-to-one palette for VIC
	// TODO: This is obsolete code for direct access to indexed frame
	// buffers and should probably be removed
	for (unsigned i = 0; i < 256; ++i) {
		colors[i] = i & 0x0f;
	}

	// Preset colors to black
	init_text_color_table(colors);
	ec_color = b0c_color = b1c_color = b2c_color = b3c_color = mm0_color = mm1_color = colors[0];
	ec_color_long = (ec_color << 24) | (ec_color << 16) | (ec_color << 8) | ec_color;
	for (unsigned i = 0; i < 8; ++i) {
		spr_color[i] = colors[0];
	}
}


/*
 *  Init multicolor lookup table
 */

void MOS6569::make_mc_table()
{
	mc_color_lookup[0] = b0c_color | (b0c_color << 8);
	mc_color_lookup[1] = b1c_color | (b1c_color << 8);
	mc_color_lookup[2] = b2c_color | (b2c_color << 8);
}


/*
 *  Convert video address to pointer
 */

inline uint8_t *MOS6569::get_physical(uint16_t adr)
{
	int va = adr | cia_vabase;
	if ((va & 0x7000) == 0x1000) {
		return char_rom + (va & 0x0fff);
	} else {
		return ram + va;
	}
}


/*
 *  Get VIC state
 */

void MOS6569::GetState(MOS6569State *vd) const
{
	vd->m0x = mx[0] & 0xff; vd->m0y = my[0];
	vd->m1x = mx[1] & 0xff; vd->m1y = my[1];
	vd->m2x = mx[2] & 0xff; vd->m2y = my[2];
	vd->m3x = mx[3] & 0xff; vd->m3y = my[3];
	vd->m4x = mx[4] & 0xff; vd->m4y = my[4];
	vd->m5x = mx[5] & 0xff; vd->m5y = my[5];
	vd->m6x = mx[6] & 0xff; vd->m6y = my[6];
	vd->m7x = mx[7] & 0xff; vd->m7y = my[7];
	vd->mx8 = mx8;

	vd->ctrl1 = (ctrl1 & 0x7f) | ((raster_y & 0x100) >> 1);
	vd->raster = raster_y & 0xff;
	vd->lpx = lpx; vd->lpy = lpy;
	vd->ctrl2 = ctrl2;
	vd->vbase = vbase;
	vd->irq_flag = irq_flag;
	vd->irq_mask = irq_mask;

	vd->me = me; vd->mxe = mxe; vd->mye = mye; vd->mdp = mdp; vd->mmc = mmc;
	vd->mm = clx_spr; vd->md = clx_bgr;

	vd->ec = ec;
	vd->b0c = b0c; vd->b1c = b1c; vd->b2c = b2c; vd->b3c = b3c;
	vd->mm0 = mm0; vd->mm1 = mm1;
	vd->m0c = sc[0]; vd->m1c = sc[1];
	vd->m2c = sc[2]; vd->m3c = sc[3];
	vd->m4c = sc[4]; vd->m5c = sc[5];
	vd->m6c = sc[6]; vd->m7c = sc[7];

	vd->pad0 = 0;
	vd->irq_raster = irq_raster;
	vd->vc = vc;
	vd->vc_base = vc_base;
	vd->rc = rc;
	vd->spr_dma = vd->spr_disp = sprite_on;
	for (unsigned i = 0; i < 8; ++i) {
		vd->mc[i] = vd->mc_base[i] = mc[i];
	}
	vd->display_state = display_state;
	vd->bad_line = raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled;
	vd->bad_line_enable = bad_lines_enabled;
	vd->lp_triggered = lp_triggered;
	vd->border_on = border_on;

	vd->bank_base = cia_vabase;
	vd->matrix_base = ((vbase & 0xf0) << 6) | cia_vabase;
	vd->char_base = ((vbase & 0x0e) << 10) | cia_vabase;
	vd->bitmap_base = ((vbase & 0x08) << 10) | cia_vabase;
	for (unsigned i = 0; i < 8; ++i) {
		vd->sprite_base[i] = (matrix_base[0x3f8 + i] << 6) | cia_vabase;
	}

	vd->raster_x = 0;
	vd->cycle = 1;
	vd->ml_index = 0;
	vd->ref_cnt = 0xff;
	vd->last_vic_byte = 0;
	vd->ud_border_on = vd->ud_border_set = border_on;
	vd->hold_off_raster_irq = false;
}


/*
 *  Set VIC state (only works if in VBlank)
 */

void MOS6569::SetState(const MOS6569State *vd)
{
	int i, j;

	mx[0] = vd->m0x; my[0] = vd->m0y;
	mx[1] = vd->m1x; my[1] = vd->m1y;
	mx[2] = vd->m2x; my[2] = vd->m2y;
	mx[3] = vd->m3x; my[3] = vd->m3y;
	mx[4] = vd->m4x; my[4] = vd->m4y;
	mx[5] = vd->m5x; my[5] = vd->m5y;
	mx[6] = vd->m6x; my[6] = vd->m6y;
	mx[7] = vd->m7x; my[7] = vd->m7y;
	mx8 = vd->mx8;
	for (i=0, j=1; i<8; i++, j<<=1) {
		if (mx8 & j) {
			mx[i] |= 0x100;
		} else {
			mx[i] &= 0xff;
		}
	}

	ctrl1 = vd->ctrl1;
	ctrl2 = vd->ctrl2;
	x_scroll = ctrl2 & 7;
	y_scroll = ctrl1 & 7;
	if (ctrl1 & 8) {
		dy_start = ROW25_YSTART;
		dy_stop = ROW25_YSTOP;
	} else {
		dy_start = ROW24_YSTART;
		dy_stop = ROW24_YSTOP;
	}
	border_40_col = ctrl2 & 8;
	display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;

	raster_y = vd->raster | ((vd->ctrl1 & 0x80) << 1);
	lpx = vd->lpx; lpy = vd->lpy;

	vbase = vd->vbase;
	cia_vabase = vd->bank_base;
	matrix_base = get_physical((vbase & 0xf0) << 6);
	char_base = get_physical((vbase & 0x0e) << 10);
	bitmap_base = get_physical((vbase & 0x08) << 10);

	irq_flag = vd->irq_flag;
	irq_mask = vd->irq_mask;

	me = vd->me; mxe = vd->mxe; mye = vd->mye; mdp = vd->mdp; mmc = vd->mmc;
	clx_spr = vd->mm; clx_bgr = vd->md;

	ec = vd->ec;
	ec_color = colors[ec];
	ec_color_long = (ec_color << 24) | (ec_color << 16) | (ec_color << 8) | ec_color;

	b0c = vd->b0c; b1c = vd->b1c; b2c = vd->b2c; b3c = vd->b3c;
	b0c_color = colors[b0c];
	b1c_color = colors[b1c];
	b2c_color = colors[b2c];
	b3c_color = colors[b3c];
	make_mc_table();

	mm0 = vd->mm0; mm1 = vd->mm1;
	mm0_color = colors[mm0];
	mm1_color = colors[mm1];

	sc[0] = vd->m0c; sc[1] = vd->m1c;
	sc[2] = vd->m2c; sc[3] = vd->m3c;
	sc[4] = vd->m4c; sc[5] = vd->m5c;
	sc[6] = vd->m6c; sc[7] = vd->m7c;
	for (unsigned i = 0; i < 8; ++i) {
		spr_color[i] = colors[sc[i]];
	}

	irq_raster = vd->irq_raster;
	vc = vd->vc;
	vc_base = vd->vc_base;
	rc = vd->rc;
	sprite_on = vd->spr_dma;
	for (unsigned i = 0; i < 8; ++i) {
		mc[i] = vd->mc[i];
	}
	display_state = vd->display_state;
	bad_lines_enabled = vd->bad_line_enable;
	lp_triggered = vd->lp_triggered;
	border_on = vd->border_on;
}


/*
 *  Trigger raster IRQ
 */

inline void MOS6569::raster_irq()
{
	irq_flag |= 0x01;
	if (irq_mask & 0x01) {
		irq_flag |= 0x80;
		the_cpu->TriggerVICIRQ();
	}
}


/*
 *  Read from VIC register
 */

uint8_t MOS6569::ReadRegister(uint16_t adr)
{
	switch (adr) {
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0a: case 0x0c: case 0x0e:
			return mx[adr >> 1];

		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0b: case 0x0d: case 0x0f:
			return my[adr >> 1];

		case 0x10:	// Sprite X position MSB
			return mx8;

		case 0x11:	// Control register 1
			return (ctrl1 & 0x7f) | ((raster_y & 0x100) >> 1);

		case 0x12:	// Raster counter
			return raster_y;

		case 0x13:	// Light pen X
			return lpx;

		case 0x14:	// Light pen Y
			return lpy;

		case 0x15:	// Sprite enable
			return me;

		case 0x16:	// Control register 2
			return ctrl2 | 0xc0;

		case 0x17:	// Sprite Y expansion
			return mye;

		case 0x18:	// Memory pointers
			return vbase | 0x01;

		case 0x19:	// IRQ flags
			return irq_flag | 0x70;

		case 0x1a:	// IRQ mask
			return irq_mask | 0xf0;

		case 0x1b:	// Sprite data priority
			return mdp;

		case 0x1c:	// Sprite multicolor
			return mmc;

		case 0x1d:	// Sprite X expansion
			return mxe;

		case 0x1e:{	// Sprite-sprite collision
			uint8_t ret = clx_spr;
			clx_spr = 0;	// Read and clear
			return ret;
		}

		case 0x1f:{	// Sprite-background collision
			uint8_t ret = clx_bgr;
			clx_bgr = 0;	// Read and clear
			return ret;
		}

		case 0x20: return ec | 0xf0;
		case 0x21: return b0c | 0xf0;
		case 0x22: return b1c | 0xf0;
		case 0x23: return b2c | 0xf0;
		case 0x24: return b3c | 0xf0;
		case 0x25: return mm0 | 0xf0;
		case 0x26: return mm1 | 0xf0;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			return sc[adr - 0x27] | 0xf0;

		default:
			return 0xff;
	}
}


/*
 *  Write to VIC register
 */

void MOS6569::WriteRegister(uint16_t adr, uint8_t byte)
{
	switch (adr) {
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0a: case 0x0c: case 0x0e:
			mx[adr >> 1] = (mx[adr >> 1] & 0xff00) | byte;
			break;

		case 0x10:{
			int i, j;
			mx8 = byte;
			for (i=0, j=1; i<8; i++, j<<=1) {
				if (mx8 & j) {
					mx[i] |= 0x100;
				} else {
					mx[i] &= 0xff;
				}
			}
			break;
		}

		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0b: case 0x0d: case 0x0f:
			my[adr >> 1] = byte;
			break;

		case 0x11: {	// Control register 1
			ctrl1 = byte;
			y_scroll = byte & 7;

			uint16_t new_irq_raster = (irq_raster & 0xff) | ((byte & 0x80) << 1);
			if (irq_raster != new_irq_raster && raster_y == new_irq_raster) {
				raster_irq();
			}
			irq_raster = new_irq_raster;

			if (byte & 8) {
				dy_start = ROW25_YSTART;
				dy_stop = ROW25_YSTOP;
			} else {
				dy_start = ROW24_YSTART;
				dy_stop = ROW24_YSTOP;
			}

			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;
		}

		case 0x12: {	// Raster counter
			uint16_t new_irq_raster = (irq_raster & 0xff00) | byte;
			if (irq_raster != new_irq_raster && raster_y == new_irq_raster) {
				raster_irq();
			}
			irq_raster = new_irq_raster;
			break;
		}

		case 0x15:	// Sprite enable
			me = byte;
			break;

		case 0x16:	// Control register 2
			ctrl2 = byte;
			x_scroll = byte & 7;
			border_40_col = byte & 8;
			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;

		case 0x17:	// Sprite Y expansion
			mye = byte;
			break;

		case 0x18:	// Memory pointers
			vbase = byte;
			matrix_base = get_physical((byte & 0xf0) << 6);
			char_base = get_physical((byte & 0x0e) << 10);
			bitmap_base = get_physical((byte & 0x08) << 10);
			break;

		case 0x19: // IRQ flags
			irq_flag = irq_flag & (~byte & 0x0f);
			the_cpu->ClearVICIRQ();	// Clear interrupt (hack!)
			if (irq_flag & irq_mask) { // Set master bit if allowed interrupt still pending
				irq_flag |= 0x80;
			}
			break;
		
		case 0x1a:	// IRQ mask
			irq_mask = byte & 0x0f;
			if (irq_flag & irq_mask) { // Trigger interrupt if pending and now allowed
				irq_flag |= 0x80;
				the_cpu->TriggerVICIRQ();
			} else {
				irq_flag &= 0x7f;
				the_cpu->ClearVICIRQ();
			}
			break;

		case 0x1b:	// Sprite data priority
			mdp = byte;
			break;

		case 0x1c:	// Sprite multicolor
			mmc = byte;
			break;

		case 0x1d:	// Sprite X expansion
			mxe = byte;
			break;

		case 0x20:
			ec_color = colors[ec = byte];
			ec_color_long = (ec_color << 24) | (ec_color << 16) | (ec_color << 8) | ec_color;
			break;

		case 0x21:
			if (b0c != byte) {
				b0c_color = colors[b0c = byte & 0xF];
				make_mc_table();
			}
			break;

		case 0x22: 
			if (b1c != byte) {
				b1c_color = colors[b1c = byte & 0xF];
				make_mc_table();
			}
			break;

		case 0x23: 
			if (b2c != byte) {
				b2c_color = colors[b2c = byte & 0xF];
				make_mc_table();
			}
			break;

		case 0x24: b3c_color = colors[b3c = byte & 0xF]; break;
		case 0x25: mm0_color = colors[mm0 = byte]; break;
		case 0x26: mm1_color = colors[mm1 = byte]; break;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			spr_color[adr - 0x27] = colors[sc[adr - 0x27] = byte];
			break;
	}
}


/*
 *  CIA VA14/15 has changed
 */

void MOS6569::ChangedVA(uint16_t new_va)
{
	cia_vabase = new_va << 14;
	WriteRegister(0x18, vbase); // Force update of memory pointers
}


/*
 *  Trigger lightpen interrupt, latch lightpen coordinates
 */

void MOS6569::TriggerLightpen()
{
	if (!lp_triggered) {	// Lightpen triggers only once per frame
		lp_triggered = true;

		lpx = 0;			// Latch current coordinates
		lpy = raster_y;

		irq_flag |= 0x08;	// Trigger IRQ
		if (irq_mask & 0x08) {
			irq_flag |= 0x80;
			the_cpu->TriggerVICIRQ();
		}
	}
}


/*
 *  Display line drawing routines...
 */

inline void MOS6569::el_std_text(uint8_t *p, uint8_t *q, uint8_t *r)
{
	unsigned int b0cc = b0c;
	uint32_t *lp = (uint32_t *)p;
	uint8_t *cp = color_line;
	uint8_t *mp = matrix_line;

	// Loop for 40 characters
	for (int i=0; i<40; i++) {
		uint8_t color = cp[i];
		uint8_t data = r[i] = q[mp[i] << 3];

		*lp++ = TextColorTable[color][b0cc][data][0].b;
		*lp++ = TextColorTable[color][b0cc][data][1].b;
	}
}


inline void MOS6569::el_mc_text(uint8_t *p, uint8_t *q, uint8_t *r)
{
	uint16_t *wp = (uint16_t *)p;
	uint8_t *cp = color_line;
	uint8_t *mp = matrix_line;
	uint16_t *mclp = mc_color_lookup;

	// Loop for 40 characters
	for (unsigned i = 0; i < 40; ++i) {
		uint8_t data = q[mp[i] << 3];

		if (cp[i] & 8) {
			uint8_t color = colors[cp[i] & 7];
			r[i] = (data & 0xaa) | (data & 0xaa) >> 1;
			mclp[3] = color | (color << 8);
			*wp++ = mclp[(data >> 6) & 3];
			*wp++ = mclp[(data >> 4) & 3];
			*wp++ = mclp[(data >> 2) & 3];
			*wp++ = mclp[(data >> 0) & 3];

		} else { // Standard mode in multicolor mode
			uint8_t color = cp[i];
			r[i] = data;
			*(uint32_t *)wp = TextColorTable[color][b0c][data][0].b;
			wp += 2;
			*(uint32_t *)wp = TextColorTable[color][b0c][data][1].b;
			wp += 2;
		}
	}
}


inline void MOS6569::el_std_bitmap(uint8_t *p, uint8_t *q, uint8_t *r)
{
	uint32_t *lp = (uint32_t *)p;
	uint8_t *mp = matrix_line;

	// Loop for 40 characters
	for (unsigned i = 0; i < 40; ++i, q += 8) {
		uint8_t data = r[i] = *q;
		uint8_t color = mp[i] >> 4;
		uint8_t bcolor = mp[i] & 15;

		*lp++ = TextColorTable[color][bcolor][data][0].b;
		*lp++ = TextColorTable[color][bcolor][data][1].b;
	}
}


inline void MOS6569::el_mc_bitmap(uint8_t *p, uint8_t *q, uint8_t *r)
{
	uint16_t lookup[4];
	uint16_t *wp = (uint16_t *)p - 1;
	uint8_t *cp = color_line;
	uint8_t *mp = matrix_line;

	lookup[0] = (b0c_color << 8) | b0c_color;

	// Loop for 40 characters
	for (unsigned i = 0; i < 40; ++i, q+=8) {
		uint8_t color, acolor, bcolor;

		color = colors[mp[i] >> 4];
		lookup[1] = (color << 8) | color;
		bcolor = colors[mp[i]];
		lookup[2] = (bcolor << 8) | bcolor;
		acolor = colors[cp[i]];
		lookup[3] = (acolor << 8) | acolor;

		uint8_t data = *q;
		r[i] = (data & 0xaa) | (data & 0xaa) >> 1;

		*++wp = lookup[(data >> 6) & 3];
		*++wp = lookup[(data >> 4) & 3];
		*++wp = lookup[(data >> 2) & 3];
		*++wp = lookup[(data >> 0) & 3];
	}
}


inline void MOS6569::el_ecm_text(uint8_t *p, uint8_t *q, uint8_t *r)
{
	uint32_t *lp = (uint32_t *)p;
	uint8_t *cp = color_line;
	uint8_t *mp = matrix_line;
	uint8_t *bcp = &b0c;

	// Loop for 40 characters
	for (unsigned i = 0; i < 40; ++i) {
		uint8_t data = r[i] = mp[i];
		uint8_t color = cp[i];
		uint8_t bcolor = bcp[(data >> 6) & 3];

		data = q[(data & 0x3f) << 3];
		*lp++ = TextColorTable[color][bcolor][data][0].b;
		*lp++ = TextColorTable[color][bcolor][data][1].b;
	}
}


inline void MOS6569::el_std_idle(uint8_t *p, uint8_t *r)
{
	uint8_t data = *get_physical(ctrl1 & 0x40 ? 0x39ff : 0x3fff);
	uint32_t *lp = (uint32_t *)p;
	uint32_t conv0 = TextColorTable[0][b0c][data][0].b;
	uint32_t conv1 = TextColorTable[0][b0c][data][1].b;

	for (unsigned i = 0; i < 40; ++i) {
		*lp++ = conv0;
		*lp++ = conv1;
		*r++ = data;
	}
}


inline void MOS6569::el_mc_idle(uint8_t *p, uint8_t *r)
{
	uint8_t data = *get_physical(0x3fff);
	uint32_t *lp = (uint32_t *)p - 1;
	r--;

	uint16_t lookup[4];
	lookup[0] = (b0c_color << 8) | b0c_color;
	lookup[1] = lookup[2] = lookup[3] = colors[0];

	uint16_t conv0 = (lookup[(data >> 6) & 3] << 16) | lookup[(data >> 4) & 3];
	uint16_t conv1 = (lookup[(data >> 2) & 3] << 16) | lookup[(data >> 0) & 3];

	for (unsigned i = 0; i < 40; ++i) {
		*++lp = conv0;
		*++lp = conv1;
		*++r = data;
	}
}


inline void MOS6569::el_sprites(uint8_t *chunky_ptr)
{
	unsigned spr_coll = 0, gfx_coll = 0;

	// Draw each active sprite
	for (unsigned snum = 0; snum < 8; ++snum) {
		uint8_t sbit = 1 << snum;

		// Is sprite visible?
		if ((sprite_on & sbit) && mx[snum] < DISPLAY_X-32) {
			uint8_t *p = chunky_ptr + mx[snum] + 8;
			uint8_t *q = spr_coll_buf + mx[snum] + 8;

			// Fetch sprite data and mask
			uint8_t *sdatap = get_physical(matrix_base[0x3f8 + snum] << 6 | mc[snum]);
			uint32_t sdata = (*sdatap << 24) | (*(sdatap+1) << 16) | (*(sdatap+2) << 8);

			uint8_t color = spr_color[snum];

			unsigned spr_mask_pos = mx[snum] + 8 - x_scroll;	// Sprite bit position in fore_mask_buf
			unsigned sshift = spr_mask_pos & 7;
			
			uint8_t *fmbp = fore_mask_buf + (spr_mask_pos / 8);
			uint32_t fore_mask = (fmbp[0] << 24) | (fmbp[1] << 16) | (fmbp[2] << 8) | (fmbp[3] << 0);
			fore_mask = (fore_mask << sshift) | (fmbp[4] >> (8-sshift));

			if (mxe & sbit) {		// X-expanded
				if (mx[snum] >= DISPLAY_X-56)
					continue;

				// Fetch extra sprite mask
				uint32_t sdata_l = 0, sdata_r = 0;
				uint32_t fore_mask_r = (fmbp[4] << 24) | (fmbp[5] << 16) | (fmbp[6] << 8);
				fore_mask_r <<= sshift;

				if (mmc & sbit) {	// X-expanded multicolor mode
					uint32_t plane0_l, plane0_r, plane1_l, plane1_r;

					// Expand sprite data
					sdata_l = MultiExpTable[sdata >> 24 & 0xff] << 16 | MultiExpTable[sdata >> 16 & 0xff];
					sdata_r = MultiExpTable[sdata >> 8 & 0xff] << 16;

					// Convert sprite chunky pixels to bitplanes
					plane0_l = (sdata_l & 0x55555555) | (sdata_l & 0x55555555) << 1;
					plane1_l = (sdata_l & 0xaaaaaaaa) | (sdata_l & 0xaaaaaaaa) >> 1;
					plane0_r = (sdata_r & 0x55555555) | (sdata_r & 0x55555555) << 1;
					plane1_r = (sdata_r & 0xaaaaaaaa) | (sdata_r & 0xaaaaaaaa) >> 1;

					// Collision with graphics?
					if ((fore_mask & (plane0_l | plane1_l)) || (fore_mask_r & (plane0_r | plane1_r))) {
						gfx_coll |= sbit;
					}

					// Mask sprite if in background
					if ((mdp & sbit) == 0) {
						fore_mask = 0;
						fore_mask_r = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 32; ++i, plane0_l <<= 1, plane1_l <<= 1, fore_mask <<= 1) {
						uint8_t col;
						if (plane1_l & 0x80000000) {
							if (plane0_l & 0x80000000) {
								col = mm1_color;
							} else {
								col = color;
							}
						} else {
							if (plane0_l & 0x80000000) {
								col = mm0_color;
							} else {
								continue;
							}
						}
						if (q[i]) {	// Obscured by higher-priority data?
							spr_coll |= q[i] | sbit;
						} else if ((fore_mask & 0x80000000) == 0) {
							p[i] = col;
						}
						q[i] |= sbit;
					}
					for (unsigned i = 32; i < 48; ++i, plane0_r <<= 1, plane1_r <<= 1, fore_mask_r <<= 1) {
						uint8_t col;
						if (plane1_r & 0x80000000) {
							if (plane0_r & 0x80000000) {
								col = mm1_color;
							} else {
								col = color;
							}
						} else {
							if (plane0_r & 0x80000000) {
								col = mm0_color;
							} else {
								continue;
							}
						}
						if (q[i]) {	// Obscured by higher-priority data?
							spr_coll |= q[i] | sbit;
						} else if ((fore_mask_r & 0x80000000) == 0) {
							p[i] = col;
						}
						q[i] |= sbit;
					}

				} else {			// X-expanded standard mode

					// Expand sprite data
					sdata_l = ExpTable[sdata >> 24 & 0xff] << 16 | ExpTable[sdata >> 16 & 0xff];
					sdata_r = ExpTable[sdata >> 8 & 0xff] << 16;

					// Collision with graphics?
					if ((fore_mask & sdata_l) || (fore_mask_r & sdata_r)) {
						gfx_coll |= sbit;
					}

					// Mask sprite if in background
					if ((mdp & sbit) == 0) {
						fore_mask = 0;
						fore_mask_r = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 32; ++i, sdata_l <<= 1, fore_mask <<= 1) {
						if (sdata_l & 0x80000000) {
							if (q[i]) {	// Obscured by higher-priority data?
								spr_coll |= q[i] | sbit;
							} else if ((fore_mask & 0x80000000) == 0) {
								p[i] = color;
							}
							q[i] |= sbit;
						}
					}
					for (unsigned i = 32; i < 48; ++i, sdata_r <<= 1, fore_mask_r <<= 1) {
						if (sdata_r & 0x80000000) {
							if (q[i]) {	// Obscured by higher-priority data?
								spr_coll |= q[i] | sbit;
							} else if ((fore_mask_r & 0x80000000) == 0) {
								p[i] = color;
							}
							q[i] |= sbit;
						}
					}
				}

			} else {				// Unexpanded

				if (mmc & sbit) {	// Unexpanded multicolor mode
					uint32_t plane0, plane1;

					// Convert sprite chunky pixels to bitplanes
					plane0 = (sdata & 0x55555555) | (sdata & 0x55555555) << 1;
					plane1 = (sdata & 0xaaaaaaaa) | (sdata & 0xaaaaaaaa) >> 1;

					// Collision with graphics?
					if (fore_mask & (plane0 | plane1)) {
						gfx_coll |= sbit;
					}

					// Mask sprite if in background
					if ((mdp & sbit) == 0) {
						fore_mask = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 24; ++i, plane0 <<= 1, plane1 <<= 1, fore_mask <<= 1) {
						uint8_t col;
						if (plane1 & 0x80000000) {
							if (plane0 & 0x80000000) {
								col = mm1_color;
							} else {
								col = color;
							}
						} else {
							if (plane0 & 0x80000000) {
								col = mm0_color;
							} else {
								continue;
							}
						}
						if (q[i]) {	// Obscured by higher-priority data?
							spr_coll |= q[i] | sbit;
						} else if ((fore_mask & 0x80000000) == 0) {
							p[i] = col;
						}
						q[i] |= sbit;
					}

				} else {			// Unexpanded standard mode

					// Collision with graphics?
					if (fore_mask & sdata) {
						gfx_coll |= sbit;
					}
	
					// Mask sprite if in background
					if ((mdp & sbit) == 0) {
						fore_mask = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 24; ++i, sdata <<= 1, fore_mask <<= 1) {
						if (sdata & 0x80000000) {
							if (q[i]) {		// Obscured by higher-priority data?
								spr_coll |= q[i] | sbit;
							} else if ((fore_mask & 0x80000000) == 0) {
								p[i] = color;
							}
							q[i] |= sbit;
						}
					}
				}
			}
		}
	}

	if (ThePrefs.SpriteCollisions) {

		// Check sprite-sprite collisions
		if (spr_coll) {
			uint8_t old_clx_spr = clx_spr;
			clx_spr |= spr_coll;
			if (old_clx_spr == 0) {	// Interrupt on first detected collision
				irq_flag |= 0x04;
				if (irq_mask & 0x04) {
					irq_flag |= 0x80;
					the_cpu->TriggerVICIRQ();
				}
			}
		}

		// Check sprite-background collisions
		if (gfx_coll) {
			uint8_t old_clx_bgr = clx_bgr;
			clx_bgr |= gfx_coll;
			if (old_clx_bgr == 0) {	// Interrupt on first detected collision
				irq_flag |= 0x02;
				if (irq_mask & 0x02) {
					irq_flag |= 0x80;
					the_cpu->TriggerVICIRQ();
				}
			}
		}
	}
}


inline int MOS6569::el_update_mc(int raster)
{
	int i, j;
	int cycles_used = 0;
	uint8_t spron = sprite_on;
	uint8_t spren = me;
	uint8_t sprye = mye;
	uint8_t raster8bit = raster;
	uint16_t *mcp = mc;
	uint8_t *myp = my;

	// Increment sprite data counters
	for (i=0, j=1; i<8; i++, j<<=1, mcp++, myp++) {

		// Sprite enabled?
		if (spren & j) {

			// Yes, activate if Y position matches raster counter
			if (*myp == (raster8bit & 0xff)) {
				*mcp = 0;
				spron |= j;
			} else {
				goto spr_off;
			}
		} else {
spr_off:
			// No, turn sprite off when data counter exceeds 60
			//  and increment counter
			if (*mcp != 63) {
				if (sprye & j) {	// Y expansion
					if (!((*myp ^ raster8bit) & 1)) {
						*mcp += 3;
						cycles_used += 2;
						if (*mcp == 63) {
							spron &= ~j;
						}
					}
				} else {
					*mcp += 3;
					cycles_used += 2;
					if (*mcp == 63) {
						spron &= ~j;
					}
				}
			}
		}
	}

	sprite_on = spron;
	return cycles_used;
}


/*
 *  Emulate one raster line.
 *  Returns VIC_VBLANK if new frame has started.
 *  Also returns the number of cycles left for the CPU in this line.
 */

unsigned MOS6569::EmulateLine(int & retCyclesLeft)
{
	int cycles_left = ThePrefs.NormalCycles;	// Cycles left for CPU
	bool is_bad_line = false;

	// Get raster counter into local variable for faster access and increment
	unsigned raster = raster_y + 1;

	// End of screen reached?
	if (raster == TOTAL_RASTERS) {

		// Yes, reset some stuff
		raster = 0;
		vc_base = 0;
		lp_triggered = false;

	} else if (raster == 1) {

		// Get bitmap pointer for next frame. This must be done
		// after the C64 VBlank activities because the preferences
		// and screen configuration may have been changed there
		chunky_line_start = the_display->BitmapBase();
		xmod = the_display->BitmapXMod();
	}

	raster_y = raster;

	// Trigger raster IRQ if IRQ line reached
	if (raster == irq_raster) {
		raster_irq();
	}

	// In line $30, the DEN bit controls if Bad Lines can occur
	if (raster == 0x30) {
		bad_lines_enabled = ctrl1 & 0x10;
	}

	// Within the visible range?
	if (raster >= FIRST_DISP_LINE && raster <= LAST_DISP_LINE) {

		// Our output goes here
		uint8_t *chunky_ptr = chunky_line_start;

		// Set video counter
		vc = vc_base;

		// Bad Line condition?
		if (raster >= FIRST_DMA_LINE && raster <= LAST_DMA_LINE && ((raster & 7) == y_scroll) && bad_lines_enabled) {

			// Turn on display
			display_state = is_bad_line = true;
			cycles_left = ThePrefs.BadLineCycles;
			rc = 0;

			// Read and latch 40 bytes from video matrix and color RAM
			uint8_t *mp = matrix_line - 1;
			uint8_t *cp = color_line - 1;
			int vc1 = vc - 1;
			uint8_t *mbp = matrix_base + vc1;
			uint8_t *crp = color_ram + vc1;
			for (unsigned i = 0; i < 40; ++i) {
				*++mp = *++mbp;
				*++cp = *++crp;
			}
		}

		// Handler upper/lower border
		if (raster == dy_stop) {
			border_on = true;
		}
		if (raster == dy_start && (ctrl1 & 0x10)) { // Don't turn off border if DEN bit cleared
			border_on = false;
		}

		if (!border_on) {

			// Display window contents
			uint8_t *p = chunky_ptr + COL40_XSTART;		// Pointer in chunky display buffer
			uint8_t *r = fore_mask_buf + COL40_XSTART/8;	// Pointer in foreground mask buffer
#ifdef ALIGNMENT_CHECK
			uint8_t *use_p = ((((int)p) & 3) == 0) ? p : text_chunky_buf;
#endif

			{
				p--;
				uint8_t b0cc = b0c_color;
				unsigned limit = x_scroll;
				for (unsigned i = 0; i < limit; ++i) {	// Background on the left if XScroll>0
					*++p = b0cc;
				}
				p++;
			}

			if (display_state) {
				switch (display_idx) {

					case 0:	// Standard text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_text(use_p, char_base + rc, r);
						if (use_p != p) {
							memcpy(p, use_p, 8*40);
						}
#else
						if (x_scroll) {
							el_std_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_std_text(p, char_base + rc, r);
						}
#endif
#else
						el_std_text(p, char_base + rc, r);
#endif
						break;

					case 1:	// Multicolor text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_text(use_p, char_base + rc, r);
						if (use_p != p) {
							memcpy(p, use_p, 8*40);
						}
#else
						if (x_scroll) {
							el_mc_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_mc_text(p, char_base + rc, r);
						}
#endif
#else
						el_mc_text(p, char_base + rc, r);
#endif
						break;

					case 2:	// Standard bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_bitmap(use_p, bitmap_base + (vc << 3) + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_std_bitmap(text_chunky_buf, bitmap_base + (vc << 3) + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_std_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
#else
						el_std_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
						break;

					case 3:	// Multicolor bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_bitmap(use_p, bitmap_base + (vc << 3) + rc, r);
						if (use_p != p) {
							memcpy(p, use_p, 8*40);
						}
#else
						if (x_scroll) {
							el_mc_bitmap(text_chunky_buf, bitmap_base + (vc << 3) + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_mc_bitmap(p, bitmap_base + (vc << 3) + rc, r);
						}
#endif
#else
						el_mc_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
						break;

					case 4:	// ECM text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_ecm_text(use_p, char_base + rc, r);
						if (use_p != p) {
							memcpy(p, use_p, 8*40);
						}
#else
						if (x_scroll) {
							el_ecm_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_ecm_text(p, char_base + rc, r);
						}
#endif
#else
						el_ecm_text(p, char_base + rc, r);
#endif
						break;

					default:	// Invalid mode (all black)
						memset(p, colors[0], 320);
						memset(r, 0, 40);
						break;
				}
				vc += 40;

			} else {	// Idle state graphics
				switch (display_idx) {

					case 0:		// Standard text
					case 1:		// Multicolor text
					case 4:		// ECM text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_idle(use_p, r);
						if (use_p != p) {memcpy(p, use_p, 8*40);}
#else
						if (x_scroll) {
							el_std_idle(text_chunky_buf, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_std_idle(p, r);
						}
#endif
#else
						el_std_idle(p, r);
#endif
						break;

					case 3:		// Multicolor bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_idle(use_p, r);
						if (use_p != p) {memcpy(p, use_p, 8*40);}
#else
						if (x_scroll) {
							el_mc_idle(text_chunky_buf, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else {
							el_mc_idle(p, r);
						}
#endif
#else
						el_mc_idle(p, r);
#endif
						break;

					default:	// Invalid mode (all black)
						memset(p, colors[0], 320);
						memset(r, 0, 40);
						break;
				}
			}

			// Draw sprites
			if (sprite_on) {

				// Clear sprite collision buffer
				uint32_t *lp = (uint32_t *)spr_coll_buf - 1;
				for (unsigned i = 0; i < DISPLAY_X/4; ++i) {
					*++lp = 0;
				}

				el_sprites(chunky_ptr);
			}

			// Handle left/right border
			uint32_t *lp = (uint32_t *)chunky_ptr - 1;
			uint32_t c = ec_color_long;
			for (unsigned i = 0; i < COL40_XSTART/4; ++i) {
				*++lp = c;
			}
			lp = (uint32_t *)(chunky_ptr + COL40_XSTOP) - 1;
			for (unsigned i = 0; i < (DISPLAY_X-COL40_XSTOP)/4; ++i) {
				*++lp = c;
			}
			if (!border_40_col) {
				c = ec_color;
				p = chunky_ptr + COL40_XSTART - 1;
				for (unsigned i = 0; i < COL38_XSTART-COL40_XSTART; ++i) {
					*++p = c;
				}
				p = chunky_ptr + COL38_XSTOP - 1;
				for (unsigned i = 0; i < COL40_XSTOP-COL38_XSTOP; ++i) {
					*++p = c;
				}
			}

#if 0	// For debugging...
			if (is_bad_line) {
				chunky_ptr[DISPLAY_X-2] = colors[7];
				chunky_ptr[DISPLAY_X-3] = colors[7];
			}
			if (rc & 1) {
				chunky_ptr[DISPLAY_X-4] = colors[1];
				chunky_ptr[DISPLAY_X-5] = colors[1];
			}
			if (rc & 2) {
				chunky_ptr[DISPLAY_X-6] = colors[1];
				chunky_ptr[DISPLAY_X-7] = colors[1];
			}
			if (rc & 4) {
				chunky_ptr[DISPLAY_X-8] = colors[1];
				chunky_ptr[DISPLAY_X-9] = colors[1];
			}
			if (ctrl1 & 0x40) {
				chunky_ptr[DISPLAY_X-10] = colors[5];
				chunky_ptr[DISPLAY_X-11] = colors[5];
			}
			if (ctrl1 & 0x20) {
				chunky_ptr[DISPLAY_X-12] = colors[3];
				chunky_ptr[DISPLAY_X-13] = colors[3];
			}
			if (ctrl2 & 0x10) {
				chunky_ptr[DISPLAY_X-14] = colors[2];
				chunky_ptr[DISPLAY_X-15] = colors[2];
			}
#endif
		} else {

			// Display border
			uint32_t *lp = (uint32_t *)chunky_ptr - 1;
			uint32_t c = ec_color_long;
			for (unsigned i = 0; i < DISPLAY_X/4; ++i) {
				*++lp = c;
			}
		}

		// Increment pointer in chunky buffer
		chunky_line_start += xmod;

		// Increment row counter, go to idle state on overflow
		if (rc == 7) {
			display_state = false;
			vc_base = vc;
		} else {
			rc++;
		}

		if (raster >= FIRST_DMA_LINE-1 && raster <= LAST_DMA_LINE-1 && (((raster+1) & 7) == y_scroll) && bad_lines_enabled) {
			rc = 0;
		}
	}

	// Skip this if all sprites are off
	if (me | sprite_on) {
		cycles_left -= el_update_mc(raster);
	}

	retCyclesLeft = cycles_left;

	if (raster == 0) {
		return VIC_VBLANK;
	} else {
		return 0;
	}
}
