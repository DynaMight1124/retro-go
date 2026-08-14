/*
 *  VIC_SC.cpp - 6569R5 emulation (cycle based)
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
 * Incompatibilities:
 * ------------------
 *
 *  - Changes to color registers are visible 1 pixel too early.
 *  - Changes to display mode are visible 4 pixels too early.
 *  - Sprites are effectively drawn in a line-based fashion in cycle 60,
 *    so changes to sprite registers (except for Y position) within the
 *    line are not displayed properly, and sprite collisions are detected
 *    too late.
 */

#include "sysdeps.h"

#include "VIC.h"
#include "C64.h"
#include "CPUC64.h"
#include "Display.h"
#include "Prefs.h"


// Define to enable VIC state overlay
#undef VIS_DEBUG


// First and last displayed line
const int FIRST_DISP_LINE = 0x10;
const int LAST_DISP_LINE = 0x11f;

// First and last possible line for Bad Lines
const int FIRST_DMA_LINE = 0x30;
const int LAST_DMA_LINE = 0xf7;

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


/*
 *  Constructor: Initialize variables
 */

MOS6569::MOS6569(C64 *c64, Display *disp, MOS6510 *CPU, uint8_t *RAM, uint8_t *Char, uint8_t *Color)
	: ram(RAM), char_rom(Char), color_ram(Color), the_c64(c64), the_display(disp), the_cpu(CPU)
{
	// Set pointers
	matrix_base = 0;
	char_base = 0;
	bitmap_base = 0;

	// Get bitmap info
	chunky_ptr = chunky_line_start = disp->BitmapBase();
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
	raster_y = TOTAL_RASTERS - 1;
	rc = 7;
	irq_raster = vc = vc_base = x_scroll = new_x_scroll = y_scroll = 0;
	dy_start = ROW24_YSTART;
	dy_stop = ROW24_YSTOP;
	ml_index = 0;

	cycle = 0;
	raster_x = 0x190;
	display_idx = 0;
	display_state = false;
	border_on = ud_border_on = ud_border_set = true;
	vblanking = raster_irq_triggered = hold_off_raster_irq = false;
	bad_lines_enabled = false;
	lp_triggered = draw_this_line = false;
	is_bad_line = false;

    spr_adv_y = 0xff;
    spr_dma_on = spr_disp_on = 0;
	for (unsigned i = 0; i < 8; ++i) {
		mc[i] = 63;
		mc_base[i] = 0;
		spr_ptr[i] = 0;
	}

	memset(spr_latch, 0, sizeof(spr_latch));
	memset(spr_coll_buf, 0, sizeof(spr_coll_buf));
	memset(fore_mask_buf, 0, sizeof(fore_mask_buf));
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
	vd->m0c = sc[0];
	vd->m1c = sc[1];
	vd->m2c = sc[2];
	vd->m3c = sc[3];
	vd->m4c = sc[4];
	vd->m5c = sc[5];
	vd->m6c = sc[6];
	vd->m7c = sc[7];

	vd->pad0 = 0;
	vd->irq_raster = irq_raster;
	vd->vc = vc;
	vd->vc_base = vc_base;
	vd->rc = rc;
	vd->spr_dma = spr_dma_on;
	vd->spr_disp = spr_disp_on;
	for (unsigned i = 0; i < 8; ++i) {
		vd->mc[i] = mc[i];
		vd->mc_base[i] = mc_base[i];
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
		vd->sprite_base[i] = spr_ptr[i] | cia_vabase;
	}

	vd->raster_x = raster_x;
	vd->cycle = cycle;
	vd->ml_index = ml_index;
	vd->ref_cnt = ref_cnt;
	vd->last_vic_byte = LastVICByte;
	vd->ud_border_on = ud_border_on;
	vd->ud_border_set = ud_border_set;
	vd->raster_irq_triggered = raster_irq_triggered;
	vd->hold_off_raster_irq = hold_off_raster_irq;
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
	x_scroll = new_x_scroll = ctrl2 & 7;
	y_scroll = ctrl1 & 7;
	if (ctrl1 & 8) {
		dy_start = ROW25_YSTART;
		dy_stop = ROW25_YSTOP;
	} else {
		dy_start = ROW24_YSTART;
		dy_stop = ROW24_YSTOP;
	}
	display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;

	raster_y = vd->raster | ((vd->ctrl1 & 0x80) << 1);
	lpx = vd->lpx; lpy = vd->lpy;

	vbase = vd->vbase;
	cia_vabase = vd->bank_base;
	matrix_base = (vbase & 0xf0) << 6;
	char_base = (vbase & 0x0e) << 10;
	bitmap_base = (vbase & 0x08) << 10;

	irq_flag = vd->irq_flag;
	irq_mask = vd->irq_mask;

	me = vd->me; mxe = vd->mxe; mye = vd->mye; mdp = vd->mdp; mmc = vd->mmc;
	clx_spr = vd->mm; clx_bgr = vd->md;

	ec = vd->ec;
	b0c = vd->b0c;
	b1c = vd->b1c;
	b2c = vd->b2c;
	b3c = vd->b3c;

	mm0 = vd->mm0;
	mm1 = vd->mm1;

	sc[0] = vd->m0c; sc[1] = vd->m1c;
	sc[2] = vd->m2c; sc[3] = vd->m3c;
	sc[4] = vd->m4c; sc[5] = vd->m5c;
	sc[6] = vd->m6c; sc[7] = vd->m7c;

	irq_raster = vd->irq_raster;
	vc = vd->vc;
	vc_base = vd->vc_base;
	rc = vd->rc;
	spr_dma_on = vd->spr_dma;
	spr_disp_on = vd->spr_disp;
	for (unsigned i = 0; i < 8; ++i) {
		mc[i] = vd->mc[i];
		mc_base[i] = vd->mc_base[i];
		spr_ptr[i] = vd->sprite_base[i] & 0x3fff;
	}
	display_state = vd->display_state;
	bad_lines_enabled = vd->bad_line_enable;
	lp_triggered = vd->lp_triggered;
	border_on = vd->border_on;

	raster_x = vd->raster_x;
	cycle = vd->cycle;
	ml_index = vd->ml_index;
	ref_cnt = vd->ref_cnt;
	LastVICByte = vd->last_vic_byte;
	ud_border_on = vd->ud_border_on;
	ud_border_set = vd->ud_border_set;
	raster_irq_triggered = vd->raster_irq_triggered;
	hold_off_raster_irq = vd->hold_off_raster_irq;
}


/*
 *  Trigger raster IRQ
 */

inline void MOS6569::raster_irq()
{
	raster_irq_triggered = true;
	irq_flag |= 0x01;
	if (irq_mask & 0x01) {
		irq_flag |= 0x80;
		the_cpu->TriggerVICIRQ();
	}
}


/*
 *  Check if raster IRQ line is reached
 */

inline void MOS6569::check_raster_irq()
{
	// Setting raster IRQ in last cycle of line doesn't trigger it in the next line
	if (raster_y == TOTAL_RASTERS - 1) {
		if (cycle == 1) {	// Last line is effectively one cycle longer
			hold_off_raster_irq = true;
			return;
		}
	} else {
		if (cycle == CYCLES_PER_LINE) {
			hold_off_raster_irq = true;
			return;
		}
	}

	// Trigger raster IRQ unless already triggered in this line
	if (raster_y == irq_raster && !raster_irq_triggered) {
		raster_irq();
	}
}


/*
 *  Read from VIC register
 */

uint8_t MOS6569::ReadRegister(uint16_t adr)
{
	uint8_t ret;

	switch (adr) {
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0a: case 0x0c: case 0x0e:
			ret = mx[adr >> 1];
			break;

		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0b: case 0x0d: case 0x0f:
			ret = my[adr >> 1];
			break;

		case 0x10:	// Sprite X position MSB
			ret = mx8;
			break;

		case 0x11:	// Control register 1
			ret = (ctrl1 & 0x7f) | ((raster_y & 0x100) >> 1);
			break;

		case 0x12:	// Raster counter
			ret = raster_y;
			break;

		case 0x13:	// Light pen X
			ret = lpx;
			break;

		case 0x14:	// Light pen Y
			ret = lpy;
			break;

		case 0x15:	// Sprite enable
			ret = me;
			break;

		case 0x16:	// Control register 2
			ret = ctrl2 | 0xc0;
			break;

		case 0x17:	// Sprite Y expansion
			ret = mye;
			break;

		case 0x18:	// Memory pointers
			ret = vbase | 0x01;
			break;

		case 0x19:	// IRQ flags
			ret = irq_flag | 0x70;
			break;

		case 0x1a:	// IRQ mask
			ret = irq_mask | 0xf0;
			break;

		case 0x1b:	// Sprite data priority
			ret = mdp;
			break;

		case 0x1c:	// Sprite multicolor
			ret = mmc;
			break;

		case 0x1d:	// Sprite X expansion
			ret = mxe;
			break;

		case 0x1e:	// Sprite-sprite collision
			ret = clx_spr;
			clx_spr = 0;	// Read and clear
			break;

		case 0x1f:	// Sprite-background collision
			ret = clx_bgr;
			clx_bgr = 0;	// Read and clear
			break;

		case 0x20: ret = ec  | 0xf0; break;
		case 0x21: ret = b0c | 0xf0; break;
		case 0x22: ret = b1c | 0xf0; break;
		case 0x23: ret = b2c | 0xf0; break;
		case 0x24: ret = b3c | 0xf0; break;
		case 0x25: ret = mm0 | 0xf0; break;
		case 0x26: ret = mm1 | 0xf0; break;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			ret = sc[adr - 0x27] | 0xf0;
			break;

		default:
			ret = 0xff;
			break;
	}

	return ret;
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
			if (irq_raster != new_irq_raster) {
				irq_raster = new_irq_raster;
				check_raster_irq();
			}

			if (byte & 8) {
				dy_start = ROW25_YSTART;
				dy_stop = ROW25_YSTOP;
			} else {
				dy_start = ROW24_YSTART;
				dy_stop = ROW24_YSTOP;
			}

			// In line $30, the DEN bit controls if Bad Lines can occur
			if (raster_y == 0x30 && (byte & 0x10)) {
				bad_lines_enabled = true;
			}

			// Bad Line condition?
			is_bad_line = (raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled);

			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;
		}

		case 0x12: {	// Raster counter
			uint16_t new_irq_raster = (irq_raster & 0xff00) | byte;
			if (irq_raster != new_irq_raster) {
				irq_raster = new_irq_raster;
				check_raster_irq();
			}
			break;
		}

		case 0x15:	// Sprite enable
			me = byte;
			break;

		case 0x16:	// Control register 2
			ctrl2 = byte;
			new_x_scroll = byte & 7;
			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;

		case 0x17:	// Sprite Y expansion
			mye = byte;
			for (unsigned i = 0; i < 8; ++i) {
				uint8_t mask = 1 << i;
				if (!(mye & mask) && !(spr_adv_y & mask)) {
					spr_adv_y |= mask;

					// Handle sprite crunch
					if (cycle == 15) {
						mc[i] = (mc_base[i] & mc[i] & 0x2a) | ((mc_base[i] | mc[i]) & 0x15);
					}
				}
			}
			break;

		case 0x18:	// Memory pointers
			vbase = byte;
			matrix_base = (byte & 0xf0) << 6;
			char_base = (byte & 0x0e) << 10;
			bitmap_base = (byte & 0x08) << 10;
			break;

		case 0x19: // IRQ flags
			irq_flag = irq_flag & (~byte & 0x0f);
			if (irq_flag & irq_mask) {	// Set master bit if allowed interrupt still pending
				irq_flag |= 0x80;
			} else {
				the_cpu->ClearVICIRQ();	// Else clear interrupt
			}
			break;
		
		case 0x1a:	// IRQ mask
			irq_mask = byte & 0x0f;
			if (irq_flag & irq_mask) {	// Trigger interrupt if pending and now allowed
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

		case 0x20: ec = byte & 0xf; break;
		case 0x21: b0c = byte & 0xf; break;
		case 0x22: b1c = byte & 0xf; break;
		case 0x23: b2c = byte & 0xf; break;
		case 0x24: b3c = byte & 0xf; break;
		case 0x25: mm0 = byte & 0xf; break;
		case 0x26: mm1 = byte & 0xf; break;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			sc[adr - 0x27] = byte & 0xf;
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
	if (!lp_triggered) {		// Lightpen triggers only once per frame
		lp_triggered = true;

		unsigned x = raster_x + 4;
		if (x >= 0x1f8) {
			x -= 0x1f8;
		}
		lpx = x >> 1;			// Latch current coordinates
		lpy = raster_y;

		irq_flag |= 0x08;		// Trigger IRQ
		if (irq_mask & 0x08) {
			irq_flag |= 0x80;
			the_cpu->TriggerVICIRQ();
		}
	}
}


/*
 *  Read a byte from the VIC's address space
 */

inline uint8_t MOS6569::read_byte(uint16_t adr)
{
	uint16_t va = adr | cia_vabase;
	if ((va & 0x7000) == 0x1000) {
		return LastVICByte = char_rom[va & 0x0fff];
	} else {
		return LastVICByte = ram[va];
	}
}


/*
 *  Quick memset of 8 bytes
 */

inline void memset8(uint8_t *p, uint8_t c)
{
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = p[7] = c;
}


/*
 *  Video matrix access
 */

void MOS6569::matrix_access()
{
	if (the_cpu->BALow) {
		if (aec_delay) {
			matrix_line[ml_index] = 0xff;
			color_line[ml_index] = ram[the_cpu->GetPC()] & 0xf;	// TODO: This may not be entirely correct for cartridges
		} else {
			uint16_t adr = (vc & 0x03ff) | matrix_base;
			matrix_line[ml_index] = read_byte(adr);
			color_line[ml_index] = color_ram[adr & 0x03ff];
		}
	}
}


/*
 *  Graphics data access
 */

void MOS6569::graphics_access()
{
	if (display_state) {

		uint16_t adr;
		if (ctrl1 & 0x20) {	// Bitmap
			adr = ((vc & 0x03ff) << 3) | bitmap_base | rc;
		} else {			// Text
			adr = (matrix_line[ml_index] << 3) | char_base | rc;
		}
		if (ctrl1 & 0x40) {	// ECM
			adr &= 0xf9ff;
		}
		gfx_delay = (gfx_delay << 8) | read_byte(adr);
		char_delay = (char_delay << 8) | matrix_line[ml_index];
		color_delay = (color_delay << 8) | color_line[ml_index];
		ml_index++;
		vc++;

	} else {

		// Display is off
		gfx_delay = (gfx_delay << 8) | read_byte(ctrl1 & 0x40 ? 0x39ff : 0x3fff);
		char_delay <<= 8;
		color_delay <<= 8;
	}
}


/*
 *  Display 8 pixels of background
 */

void MOS6569::draw_background()
{
	if (!draw_this_line)
		return;

	uint8_t *p = chunky_ptr;
	chunky_ptr += 8;

	uint8_t char_data = char_delay >> 8;

	uint8_t c;

	switch (display_idx) {
		case 0:		// Standard text
		case 1:		// Multicolor text
		case 3:		// Multicolor bitmap
			c = b0c;
			break;
		case 2:		// Standard bitmap
			c = char_data & 0xf;
			break;
		case 4:		// ECM text
			if (char_data & 0x80) {
				if (char_data & 0x40) {
					c = b3c;
				} else {
					c = b2c;
				}
			} else {
				if (char_data & 0x40) {
					c = b1c;
				} else {
					c = b0c;
				}
			}
			break;
		default:
			c = 0;	// Black
			break;
	}

	memset8(p, c);

	pixel_shifter = c * 0x11111111;
	back_pixel = c << 28;
	fore_mask_shifter = 0;
}


/*
 *  Display 8 pixels of graphics
 */

// Load pixel output shift registers with 8 new pixels according to current
// display mode
void MOS6569::load_pixel_shifter(uint8_t gfx_data, uint8_t char_data, uint8_t color_data)
{
	uint8_t c[4];
	uint32_t s;

	switch (display_idx) {

		case 0:		// Standard text
			c[0] = b0c;
			c[1] = color_data;
			goto draw_std;

		case 1:		// Multicolor text
			if (color_data & 8) {
				c[0] = b0c;
				c[1] = b1c;
				c[2] = b2c;
				c[3] = color_data & 7;
				goto draw_multi;
			} else {
				c[0] = b0c;
				c[1] = color_data;
				goto draw_std;
			}

		case 2:		// Standard bitmap
			c[0] = char_data & 0xf;
			c[1] = char_data >> 4;
			goto draw_std;

		case 3:		// Multicolor bitmap
			c[0] = b0c;
			c[1] = char_data >> 4;
			c[2] = char_data & 0xf;
			c[3] = color_data;
			goto draw_multi;

		case 4:		// ECM text
			if (char_data & 0x80) {
				if (char_data & 0x40) {
					c[0] = b3c;
				} else {
					c[0] = b2c;
				}
			} else {
				if (char_data & 0x40) {
					c[0] = b1c;
				} else {
					c[0] = b0c;
				}
			}
			c[1] = color_data;
			goto draw_std;

		case 5:		// Invalid multicolor text
			pixel_shifter = back_pixel = 0;
			if (color_data & 8) {
				fore_mask_shifter = (gfx_data & 0xaa) | ((gfx_data & 0xaa) >> 1);
			} else {
				fore_mask_shifter = gfx_data;
			}
			return;

		case 6:		// Invalid standard bitmap
			pixel_shifter = back_pixel = 0;
			fore_mask_shifter = gfx_data;
			return;

		case 7:		// Invalid multicolor bitmap
			pixel_shifter = back_pixel = 0;
			fore_mask_shifter = (gfx_data & 0xaa) | ((gfx_data & 0xaa) >> 1);
			return;

		default:	// Can't happen
			return;
	}

draw_std:
	fore_mask_shifter = gfx_data;

	s =            c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1]; gfx_data >>= 1;
	s = (s << 4) | c[gfx_data & 1];

	pixel_shifter = s;
	back_pixel = c[0] << 28;
	return;

draw_multi:
	fore_mask_shifter = (gfx_data & 0xaa) | ((gfx_data & 0xaa) >> 1);

	s =            c[gfx_data & 3];
	s = (s << 4) | c[gfx_data & 3]; gfx_data >>= 2;
	s = (s << 4) | c[gfx_data & 3];
	s = (s << 4) | c[gfx_data & 3]; gfx_data >>= 2;
	s = (s << 4) | c[gfx_data & 3];
	s = (s << 4) | c[gfx_data & 3]; gfx_data >>= 2;
	s = (s << 4) | c[gfx_data & 3];
	s = (s << 4) | c[gfx_data & 3];

	pixel_shifter = s;
	back_pixel = c[0] << 28;
	return;
}

void MOS6569::draw_graphics()
{
	if (!draw_this_line)
		return;

	if (ud_border_on) {
		draw_background();
		return;
	}

	uint8_t * p = chunky_ptr;
	uint8_t * f = fore_mask_ptr;

	// Get new graphics data
	uint8_t gfx_data = gfx_delay >> 8;
	uint8_t char_data = char_delay >> 8;
	uint8_t color_data = color_delay >> 8;

	if (x_scroll == 0) {

		// Load next 8 pixels into shifter
		load_pixel_shifter(gfx_data, char_data, color_data);

		// Draw pixels to output buffer
		uint32_t s = pixel_shifter;
		for (unsigned i = 0; i < 8; ++i) {
			*p++ = s & 0xf;
			s = (s >> 4) | back_pixel;
		}
		pixel_shifter = s;

		// Set foreground mask
		f[0] = fore_mask_shifter;

	} else {

		// Draw remaining pixels from shifter to output buffer
		uint32_t s = pixel_shifter;
		for (unsigned i = 0; i < x_scroll; ++i) {
			*p++ = s & 0xf;
			s = (s >> 4) | back_pixel;
		}

		uint8_t m0 = fore_mask_shifter << (8 - x_scroll);

		// Load next 8 pixels into shifter
		// (logically when XSCROLL == lower 3 bits of raster X position)
		load_pixel_shifter(gfx_data, char_data, color_data);

		// Draw new pixels
		s = pixel_shifter;
		for (unsigned i = 0; i < 8 - x_scroll; ++i) {
			*p++ = s & 0xf;
			s = (s >> 4) | back_pixel;
		}
		pixel_shifter = s;

		uint8_t m1 = fore_mask_shifter >> x_scroll;

		// Set foreground mask
		f[0] = m0 | m1;
	}

	chunky_ptr += 8;
	fore_mask_ptr++;
}


/*
 *  Sprite display
 */

inline void MOS6569::draw_sprites()
{
	unsigned spr_coll = 0, gfx_coll = 0;

	// Any sprite ready to be drawn?
	bool draw_any_sprite = false;
	for (unsigned snum = 0; snum < 8; ++snum) {
		if (spr_latch[snum].disp_on) {
			draw_any_sprite = true;
			break;
		}
	}
	if (! draw_any_sprite)
		return;

	// Clear sprite collision/priority buffer
	memset(spr_coll_buf, 0, sizeof(spr_coll_buf));

	// Loop for all sprites in descending order of priority
	for (unsigned snum = 0; snum < 8; ++snum) {
		uint8_t sbit = 1 << snum;
		SprLatch * latch = &spr_latch[snum];

		// Is sprite visible?
		if (latch->disp_on) {
			unsigned x = latch->mx;

			uint8_t *p = chunky_line_start + 8 + x;	// Start of sprite in pixel buffer
			if (x >= DISPLAY_X - 8) {
				p -= 0x1f8;
			}
			uint8_t *q = spr_coll_buf;

			uint8_t color = latch->sc;

			// Fetch sprite data and mask
			uint32_t sdata = latch->data;
			latch->data = 0;

			unsigned spr_mask_pos = x + 8;	// Sprite bit position in fore_mask_buf
			unsigned sshift = spr_mask_pos & 7;

			uint8_t *fmbp = fore_mask_buf + (spr_mask_pos / 8);
			uint32_t fore_mask = (fmbp[0] << 24) | (fmbp[1] << 16) | (fmbp[2] << 8) | (fmbp[3] << 0);
			fore_mask = (fore_mask << sshift) | (fmbp[4] >> (8-sshift));

			if (latch->mxe) {		// X-expanded

				// Perform clipping
				unsigned first_pix = 0;
				if (x > 0x1c0 && x < 0x1f0) {
					first_pix = 0x1f0 - x;		// Clipped on the left
				}

				unsigned last_pix = 48;
				if (x > (DISPLAY_X - 8 - 48)) {
					if (x < (DISPLAY_X - 8)) {	// Clipped on the right
						last_pix = DISPLAY_X - 8 - x;
					} else if (x <= 0x1c0) {	// Too far to the left to be visible
						last_pix = 0;
					}
				}

				// Fetch extra sprite mask
				uint32_t sdata_l = 0, sdata_r = 0;
				uint32_t fore_mask_r = (fmbp[4] << 24) | (fmbp[5] << 16) | (fmbp[6] << 8);
				fore_mask_r <<= sshift;

				if (latch->mmc) {	// X-expanded multicolor mode
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
					if (!latch->mdp) {
						fore_mask = 0;
						fore_mask_r = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 32; ++i, plane0_l <<= 1, plane1_l <<= 1, fore_mask <<= 1) {
						uint8_t col;
						if (plane1_l & 0x80000000) {
							if (plane0_l & 0x80000000) {
								col = mm1;
							} else {
								col = color;
							}
						} else {
							if (plane0_l & 0x80000000) {
								col = mm0;
							} else {
								continue;
							}
						}

						unsigned qi = x + i;
						if (qi >= 0x1f8) { qi -= 0x1f8; }

						if (q[qi]) {	// Obscured by higher-priority data?
							spr_coll |= q[qi] | sbit;
						} else if ((fore_mask & 0x80000000) == 0) {
							if (i >= first_pix && i < last_pix) {
								p[i] = col;
							}
						}
						q[qi] |= sbit;
					}
					for (unsigned i = 32; i < 48; ++i, plane0_r <<= 1, plane1_r <<= 1, fore_mask_r <<= 1) {
						uint8_t col;
						if (plane1_r & 0x80000000) {
							if (plane0_r & 0x80000000) {
								col = mm1;
							} else {
								col = color;
							}
						} else {
							if (plane0_r & 0x80000000) {
								col = mm0;
							} else {
								continue;
							}
						}

						unsigned qi = x + i;
						if (qi >= 0x1f8) { qi -= 0x1f8; }

						if (q[qi]) {		// Obscured by higher-priority data?
							spr_coll |= q[qi] | sbit;
						} else if ((fore_mask_r & 0x80000000) == 0) {
							if (i >= first_pix && i < last_pix) {
								p[i] = col;
							}
						}
						q[qi] |= sbit;
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
					if (!latch->mdp) {
						fore_mask = 0;
						fore_mask_r = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 32; ++i, sdata_l <<= 1, fore_mask <<= 1) {
						if (sdata_l & 0x80000000) {
							unsigned qi = x + i;
							if (qi >= 0x1f8) { qi -= 0x1f8; }

							if (q[qi]) {	// Obscured by higher-priority data?
								spr_coll |= q[qi] | sbit;
							} else if ((fore_mask & 0x80000000) == 0) {
								if (i >= first_pix && i < last_pix) {
									p[i] = color;
								}
							}
							q[qi] |= sbit;
						}
					}
					for (unsigned i = 32; i < 48; ++i, sdata_r <<= 1, fore_mask_r <<= 1) {
						if (sdata_r & 0x80000000) {
							unsigned qi = x + i;
							if (qi >= 0x1f8) { qi -= 0x1f8; }

							if (q[qi]) {	// Obscured by higher-priority data?
								spr_coll |= q[qi] | sbit;
							} else if ((fore_mask_r & 0x80000000) == 0) {
								if (i >= first_pix && i < last_pix) {
									p[i] = color;
								}
							}
							q[qi] |= sbit;
						}
					}
				}

			} else {				// Unexpanded

				// Perform clipping
				unsigned first_pix = 0;
				if (x > 0x1d8 && x < 0x1f0) {
					first_pix = 0x1f0 - x;		// Clipped on the left
				}

				unsigned last_pix = 24;
				if (x > (DISPLAY_X - 8 - 24)) {
					if (x < (DISPLAY_X - 8)) {	// Clipped on the right
						last_pix = DISPLAY_X - 8 - x;
					} else if (x <= 0x1d8) {	// Too far to the left to be visible
						last_pix = 0;
					}
				}

				if (latch->mmc) {	// Unexpanded multicolor mode
					uint32_t plane0, plane1;

					// Convert sprite chunky pixels to bitplanes
					plane0 = (sdata & 0x55555555) | (sdata & 0x55555555) << 1;
					plane1 = (sdata & 0xaaaaaaaa) | (sdata & 0xaaaaaaaa) >> 1;

					// Collision with graphics?
					if (fore_mask & (plane0 | plane1)) {
						gfx_coll |= sbit;
					}

					// Mask sprite if in background
					if (!latch->mdp) {
						fore_mask = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 24; ++i, plane0 <<= 1, plane1 <<= 1, fore_mask <<= 1) {
						uint8_t col;
						if (plane1 & 0x80000000) {
							if (plane0 & 0x80000000) {
								col = mm1;
							} else {
								col = color;
							}
						} else {
							if (plane0 & 0x80000000) {
								col = mm0;
							} else {
								continue;
							}
						}

						unsigned qi = x + i;
						if (qi >= 0x1f8) { qi -= 0x1f8; }

						if (q[qi]) {	// Obscured by higher-priority data?
							spr_coll |= q[qi] | sbit;
						} else if ((fore_mask & 0x80000000) == 0) {
							if (i >= first_pix && i < last_pix) {
								p[i] = col;
							}
						}
						q[qi] |= sbit;
					}

				} else {			// Unexpanded standard mode

					// Collision with graphics?
					if (fore_mask & sdata) {
						gfx_coll |= sbit;
					}

					// Mask sprite if in background
					if (!latch->mdp) {
						fore_mask = 0;
					}

					// Paint sprite
					for (unsigned i = 0; i < 24; ++i, sdata <<= 1, fore_mask <<= 1) {
						if (sdata & 0x80000000) {
							unsigned qi = x + i;
							if (qi >= 0x1f8) { qi -= 0x1f8; }

							if (q[qi]) {	// Obscured by higher-priority data?
								spr_coll |= q[qi] | sbit;
							} else if ((fore_mask & 0x80000000) == 0) {
								if (i >= first_pix && i < last_pix) {
									p[i] = color;
								}
							}
							q[qi] |= sbit;
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


/*
 *  Emulate one clock cycle.
 *  Returns VIC_HBLANK if new raster line has started.
 *  Returns VIC_VBLANK if new frame has started.
 */

// Set BA low
#define SetBALow \
	if (!the_cpu->BALow) { \
		aec_delay = 7; \
		the_cpu->BALow = true; \
	}

// Set BA high
#define SetBAHigh \
	the_cpu->BALow = false;

// Turn on display if Bad Line
#define DisplayIfBadLine \
	if (is_bad_line) { \
		display_state = true; \
	}

// Turn on display and matrix access if Bad Line
#define FetchIfBadLine \
	if (is_bad_line) { \
		display_state = true; \
		SetBALow; \
	} else { \
		SetBAHigh; \
	}

// Turn on display and matrix access and reset RC if Bad Line
#define RCIfBadLine \
	if (is_bad_line) { \
		display_state = true; \
		rc = 0; \
		SetBALow; \
	}

// Idle access
#define IdleAccess \
	read_byte(0x3fff)

// Refresh access
#define RefreshAccess \
	read_byte(0x3f00 | ref_cnt--)

// Turn on sprite DMA if necessary
#define CheckSpriteDMA \
	for (unsigned i = 0; i < 8; ++i) { \
		uint8_t mask = 1 << i; \
		if ((me & mask) && (raster_y & 0xff) == my[i] && !(spr_dma_on & mask)) { \
			spr_dma_on |= mask; \
			mc_base[i] = 0; \
			spr_adv_y |= mask; \
		} \
	}

// Fetch sprite data pointer
#define SprPtrAccess(num) \
	spr_ptr[num] = read_byte(matrix_base | 0x03f8 | num) << 6;

// Fetch sprite data, increment data counter
#define SprDataAccess(num, bytenum) \
	if (spr_dma_on & (1 << num)) { \
		if (aec_delay && (bytenum == 0 || bytenum == 2)) { \
			spr_data[num][bytenum] = 0xff; /* TODO: or byte which CPU is reading from/writing to VIC register right now */ \
		} else { \
			spr_data[num][bytenum] = read_byte(mc[num] | spr_ptr[num]); \
		} \
		mc[num] = (mc[num] + 1) & 0x3f; \
	} else { \
		if (bytenum == 1) { \
			spr_data[num][bytenum] = read_byte(0x3fff); \
		} else { \
			spr_data[num][bytenum] = 0xff; /* TODO: or byte which CPU is reading from/writing to VIC register right now */ \
		} \
	}

// Sample border color for deferred drawing at end of line
#define SampleBorderColor \
	if (draw_this_line) { \
		border_color_sample[cycle-13] = ec; \
	}


#ifdef VIS_DEBUG
struct DebugSample {
	bool ba_low, aec_delay, display_state;
	uint8_t display_idx;
	uint8_t vbase;
	uint8_t rc;
};

static DebugSample vis_debug[CYCLES_PER_LINE + 1];
#endif


unsigned MOS6569::EmulateCycle()
{
	unsigned retFlags = 0;

	// Shift delay lines
	aec_delay >>= 1;

	// Increment cycle counter
	++cycle;
	if (cycle > CYCLES_PER_LINE) {
		cycle = 1;
	}

	// Latch sprite data on X position match for display at end of line
	if (spr_disp_on) {
		unsigned cx = raster_x & 0x1f8;	// Mask out lower 3 bits for comparison

		for (unsigned i = 0; i < 8; ++i) {
			unsigned mask = 1 << i;
			SprLatch * latch = &spr_latch[i];

			unsigned sx = mx[i];
			if (sx >= 0x1f8) {
				continue;	// Sprite invisible
			} else if (sx >= 0x1f4) {
				sx += 12;
			} else {
				sx += 4;
			}

 			if (!latch->disp_on && (spr_disp_on & mask) && (sx & 0x1f8) == cx) {
				latch->disp_on = true;
				latch->mxe = mxe & mask;
				latch->mdp = mdp & mask;
				latch->mmc = mmc & mask;
				latch->sc = sc[i];
				latch->mx = mx[i];
				latch->data = (spr_data[i][0] << 24) | (spr_data[i][1] << 16) | (spr_data[i][2] << 8);
			}
		}
	}

	switch (cycle) {

		// Fetch sprite pointer 3, increment raster counter, trigger raster IRQ,
		// test for Bad Line, reset BA if sprites 3 and 4 off
		case 1:
			if (raster_y == TOTAL_RASTERS - 1) {

				// Trigger VBlank in cycle 2
				vblanking = true;

			} else {

				// Increment raster counter
				raster_y++;

				// Trigger raster IRQ if IRQ line reached
				if (raster_y == irq_raster && !hold_off_raster_irq) {
					raster_irq();
				} else {
					raster_irq_triggered = false;
				}
				hold_off_raster_irq = false;

				// In line $30, the DEN bit controls if Bad Lines can occur
				if (raster_y == 0x30) {
					bad_lines_enabled = ctrl1 & 0x10;
				}

				// Bad Line condition?
				is_bad_line = (raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled);

				// Don't draw all lines, hide some at the top and bottom
				draw_this_line = (raster_y >= FIRST_DISP_LINE && raster_y <= LAST_DISP_LINE);
			}

			// First sample of border state
			border_on_sample[0] = border_on;

			SprPtrAccess(3);
			SprDataAccess(3, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x18)) {
				SetBAHigh;
			}
			break;

		// Set BA for sprite 5, tell C64 that frame is over, read data of sprite 3
		case 2:
			if (vblanking) {

				// Vertical blank, reset counters
				raster_y = vc_base = 0;
				ref_cnt = 0xff;
				lp_triggered = vblanking = false;

				retFlags = VIC_VBLANK;

				chunky_line_start = the_display->BitmapBase();
				xmod = the_display->BitmapXMod();

				// Trigger raster IRQ if IRQ in line 0
				if (irq_raster == 0 && !hold_off_raster_irq) {
					raster_irq();
				} else {
					raster_irq_triggered = false;
				}
				hold_off_raster_irq = false;
			}

			// Our output goes here
			chunky_ptr = chunky_line_start;

			// Clear foreground mask
			memset(fore_mask_buf, 0, sizeof(fore_mask_buf));
			fore_mask_ptr = fore_mask_buf + 4;	// Offset because of gfx delay

			SprDataAccess(3, 1);
			SprDataAccess(3, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x20) {
				SetBALow;
			}
			break;

		// Fetch sprite pointer 4, reset BA if sprites 4 and 5 off
		case 3:
			SprPtrAccess(4);
			SprDataAccess(4, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x30)) {
				SetBAHigh;
			}
			break;

		// Set BA for sprite 6, read data of sprite 4 
		case 4:
			SprDataAccess(4, 1);
			SprDataAccess(4, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x40) {
				SetBALow;
			}
			break;

		// Fetch sprite pointer 5, reset BA if sprites 5 and 6 off
		case 5:
			SprPtrAccess(5);
			SprDataAccess(5, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x60)) {
				SetBAHigh;
			}
			break;

		// Set BA for sprite 7, read data of sprite 5
		case 6:
			SprDataAccess(5, 1);
			SprDataAccess(5, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x80) {
				SetBALow;
			}
			break;

		// Fetch sprite pointer 6, reset BA if sprites 6 and 7 off
		case 7:
			SprPtrAccess(6);
			SprDataAccess(6, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0xc0)) {
				SetBAHigh;
			}
			break;

		// Read data of sprite 6
		case 8:
			SprDataAccess(6, 1);
			SprDataAccess(6, 2);
			DisplayIfBadLine;
			break;

		// Fetch sprite pointer 7, reset BA if sprite 7 off
		case 9:
			SprPtrAccess(7);
			SprDataAccess(7, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x80)) {
				SetBAHigh;
			}
			break;

		// Read data of sprite 7
		case 10:
			SprDataAccess(7, 1);
			SprDataAccess(7, 2);
			DisplayIfBadLine;
			break;

		// Refresh, reset BA
		case 11:
			RefreshAccess;
			DisplayIfBadLine;
			SetBAHigh;
			break;

		// Refresh, turn on matrix access if Bad Line
		case 12:
			RefreshAccess;
			FetchIfBadLine;
			break;

		// Refresh, turn on matrix access if Bad Line, reset raster_x
		case 13:
			RefreshAccess;
			FetchIfBadLine;
			raster_x = 0xfff8;	// Wrap-around at end of function
			break;

		// Refresh, VCBASE->VCCOUNT, turn on matrix access and reset RC if Bad Line, graphics display starts here
		case 14:
			draw_background();
			SampleBorderColor;
			RefreshAccess;
			RCIfBadLine;
			vc = vc_base;
			break;

		// Refresh and matrix access
		case 15:
			draw_background();
			SampleBorderColor;
			RefreshAccess;
			FetchIfBadLine;
			ml_index = 0;
			matrix_access();
			break;

		// Graphics and matrix access, handle sprite Y expansion and check if sprite DMA can be
		// turned off
		case 16:
			draw_background();
			SampleBorderColor;
			char_delay |= (char_delay >> 8);	// Keep last character for next draw_background()
			graphics_access();
			FetchIfBadLine;

			for (unsigned i = 0; i < 8; ++i) {
				uint8_t mask = 1 << i;
				if (spr_adv_y & mask) {
					mc_base[i] = mc[i] & 0x3f;
					if (mc_base[i] == 0x3f) {
						spr_dma_on &= ~mask;
					}
				}
			}

			matrix_access();
			break;

		// Graphics and matrix access, turn off border in 40 column mode, display window starts here
		case 17:
			if (ctrl2 & 8) {
				if (raster_y == dy_stop) {
					ud_border_set = true;
				}
				ud_border_on = ud_border_set;
				if (!ud_border_on) {
					border_on = false;
				}
			}

			// Second sample of border state
			border_on_sample[1] = border_on;

			draw_background();
			SampleBorderColor;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			break;

		// Turn off border in 38 column mode
		case 18:
			if (!(ctrl2 & 8)) {
				if (raster_y == dy_stop) {
					ud_border_set = true;
				}
				ud_border_on = ud_border_set;
				if (!ud_border_on) {
					border_on = false;
				}
			}

			// Third sample of border state
			border_on_sample[2] = border_on;

			// Falls through

		// Graphics and matrix access
		case 19: case 20: case 21: case 22: case 23: case 24:
		case 25: case 26: case 27: case 28: case 29: case 30:
		case 31: case 32: case 33: case 34: case 35: case 36:
		case 37: case 38: case 39: case 40: case 41: case 42:
		case 43: case 44: case 45: case 46: case 47: case 48:
		case 49: case 50: case 51: case 52: case 53: case 54:	// Gnagna...
			draw_graphics();
			SampleBorderColor;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			break;

		// Last graphics access, turn off matrix access, turn on sprite DMA if Y coordinate is
		// right and sprite is enabled, set BA for sprite 0
		case 55:
			draw_graphics();
			SampleBorderColor;
			graphics_access();
			DisplayIfBadLine;
			CheckSpriteDMA;

#ifndef NTSC
			if (spr_dma_on & 0x01) {
				SetBALow;
			} else {
				SetBAHigh;
			}
#endif
			break;

		// Turn on border in 38 column mode, turn on sprite DMA if Y coordinate is right and
		// sprite is enabled, handle sprite Y expansion, set BA for sprite 0, display window
		// ends here
		case 56:
			if (!(ctrl2 & 8)) {
				border_on = true;
			}

			// Fourth sample of border state
			border_on_sample[3] = border_on;

			draw_graphics();
			SampleBorderColor;
			gfx_delay <<= 8;	// No graphics_access(), but still shift delay lines along
			char_delay <<= 8;
			color_delay <<= 8;
			IdleAccess;
			DisplayIfBadLine;
			CheckSpriteDMA;

			// Invert advance line flip-flop if bit in MYE is set
			for (unsigned i = 0; i < 8; ++i) {
				uint8_t mask = 1 << i;
				if ((spr_dma_on & mask) && (mye & mask)) {
					spr_adv_y ^= mask;
				}
			}

#ifndef NTSC
			if (spr_dma_on & 0x01) {
				SetBALow;
			}
#endif
			break;

		// Turn on border in 40 column mode, set BA for sprite 1, paint sprites
		case 57:
			if (ctrl2 & 8) {
				border_on = true;
			}

			// Fifth sample of border state
			border_on_sample[4] = border_on;

			draw_graphics();
			SampleBorderColor;
			gfx_delay <<= 8;	// No graphics_access(), but still shift gfx delay line along for remaining pixels
			IdleAccess;
			DisplayIfBadLine;

#ifdef NTSC
			if (spr_dma_on & 0x01) {
				SetBALow;
			} else {
				SetBAHigh;
			}
#else
			if (spr_dma_on & 0x02) {
				SetBALow;
			}
#endif
			break;

		// Fetch sprite pointer 0, MCBASE->MC, turn sprite display on/off,
		// reset BA if sprites 0 and 1 off, turn off display and load VCCOUNT->VCBASE if RC=7
		case 58:
			if (x_scroll > 0) {
				draw_graphics();	// Final call to drain remaining pixels from delay line
				gfx_delay = 0;
			} else {
				draw_background();
			}
			SampleBorderColor;

			for (unsigned i = 0; i < 8; ++i) {
				uint8_t mask = 1 << i;
				mc[i] = mc_base[i];
				if (spr_dma_on & mask) {
					if ((me & mask) && (raster_y & 0xff) == my[i]) {
						spr_disp_on |= mask;
					}
				} else {
					spr_disp_on &= ~mask;
				}
			}

#ifdef NTSC
			IdleAccess;
			if (spr_dma_on & 0x01) {
				SetBALow;
			}
#else
			SprPtrAccess(0);
			SprDataAccess(0, 0);
			if (!(spr_dma_on & 0x03)) {
				SetBAHigh;
			}
#endif

			if (rc == 7) {
				vc_base = vc;
				display_state = false;
			}
			if (is_bad_line || display_state) {
				display_state = true;
				rc = (rc + 1) & 7;
			}
			break;

		// Set BA for sprite 2, read data of sprite 0
		case 59:
			draw_background();
			SampleBorderColor;

#ifdef NTSC
			IdleAccess;
			DisplayIfBadLine;
			if (spr_dma_on & 0x02) {
				SetBALow;
			}
#else
			SprDataAccess(0, 1);
			SprDataAccess(0, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x04) {
				SetBALow;
			}
#endif
			break;

		// Fetch sprite pointer 1, reset BA if sprites 1 and 2 off
		case 60:
			draw_background();
			SampleBorderColor;

#ifdef NTSC
			SprPtrAccess(0);
			SprDataAccess(0, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x03)) {
				SetBAHigh;
			}
#else
			SprPtrAccess(1);
			SprDataAccess(1, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x06)) {
				SetBAHigh;
			}
#endif
			break;

		// Set BA for sprite 3, read data of sprite 1, graphics display ends here
		case 61:
			draw_background();
			SampleBorderColor;

			if (draw_this_line) {

				// Copy foreground mask from left side to higher X
				// coordinates to properly handle X-expanded sprites under
				// the left border (MxX = 0x1e1..0x1f7)
				fore_mask_buf[0x218 / 8 + 0] = fore_mask_buf[COL40_XSTART / 8 + 0];
				fore_mask_buf[0x218 / 8 + 1] = fore_mask_buf[COL40_XSTART / 8 + 1];
				fore_mask_buf[0x218 / 8 + 2] = fore_mask_buf[COL40_XSTART / 8 + 2];

				// Draw sprites
				draw_sprites();

				// Draw border
				if (border_on_sample[0]) {
					for (unsigned i = 0; i < 4; ++i) {
						memset8(chunky_line_start+i*8, border_color_sample[i+1]);
					}
				}
				if (border_on_sample[1]) {	// 38 columns: 7 pixels on left side
					uint8_t c = border_color_sample[5];
					chunky_line_start[4*8+0] = c;
					chunky_line_start[4*8+1] = c;
					chunky_line_start[4*8+2] = c;
					chunky_line_start[4*8+3] = c;
					chunky_line_start[4*8+4] = c;
					chunky_line_start[4*8+5] = c;
					chunky_line_start[4*8+6] = c;
				}
				if (border_on_sample[2]) {
					chunky_line_start[4*8+7] = border_color_sample[5];
					for (unsigned i = 5; i < 43; ++i) {
						memset8(chunky_line_start+i*8, border_color_sample[i+1]);
					}
				}
				if (border_on_sample[3]) {	// 38 columns: 9 pixels on right side
					uint8_t c = border_color_sample[44];
					chunky_line_start[42*8+7] = c;
					memset8(chunky_line_start+43*8, c);
				}
				if (border_on_sample[4]) {
					for (unsigned i = 44; i < DISPLAY_X/8; ++i) {
						memset8(chunky_line_start+i*8, border_color_sample[i+1]);
					}
				}

#ifdef VIS_DEBUG
				for (unsigned c = 12; c <= 59; ++c) {
					uint8_t * p = chunky_line_start + (c - 12) * 8;

					// First pixel: BA/AEC state
					if (vis_debug[c].ba_low) {
						if (vis_debug[c].aec_delay) {
							p[0] = 10;	// Light red: BA low, AEC clocking
						} else {
							p[0] = 2;	// Red: BA and AEC low
						}
					}

					// Pixel 1: Display mode
					if (vis_debug[c].display_state) {
						unsigned idx = vis_debug[c].display_idx;
						if (idx == 0) {
							p[1] = 5;	// Green: standard text
						} else if (idx == 1 || idx == 4) {
							p[1] = 13;	// Light green: multicolor text
						} else if (idx == 2) {
							p[1] = 6;	// Blue: standard bitmap
						} else if (idx == 3) {
							p[1] = 14;	// Light blue: multicolor bitmap
						} else {
							p[1] = 7;	// Yellow: invalid
						}
					} else {
						p[1] = 12;		// Gray: idle
					}

					if (raster_y >= FIRST_DMA_LINE && raster_y < LAST_DMA_LINE + 8 && vis_debug[c].display_state) {

						// Pixel 2: Video matrix base
						p[2] = vis_debug[c].vbase >> 4;

						// Pixel 3: Character generator base
						p[3] = vis_debug[c].vbase & 0x0f;
					}

					// Pixels 4..6: RC
					if (c == 13 || c == 14 || c ==57 || c == 58) {
						if (vis_debug[c].rc & 4) {
							p[4] = 15;
						}
						if (vis_debug[c].rc & 2) {
							p[5] = 15;
						}
						if (vis_debug[c].rc & 1) {
							p[6] = 15;
						}
					}
				}
#endif

				// Increment pointer in chunky buffer
				chunky_line_start += xmod;
			}

			// Clear sprite latches
			if (draw_this_line || raster_y == FIRST_DISP_LINE - 1) {
				for (unsigned i = 0; i < 8; ++i) {
					spr_latch[i].disp_on = false;
				}
			}

#ifdef NTSC
			SprDataAccess(0, 1);
			SprDataAccess(0, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x04) {
				SetBALow;
			}
#else
			SprDataAccess(1, 1);
			SprDataAccess(1, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x08) {
				SetBALow;
			}
#endif
			break;

		// Fetch sprite pointer 2, reset BA if sprites 2 and 3 off
		case 62:
#ifdef NTSC
			SprPtrAccess(1);
			SprDataAccess(1, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x06)) {
				SetBAHigh;
			}
#else
			SprPtrAccess(2);
			SprDataAccess(2, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x0c)) {
				SetBAHigh;
			}
#endif
			break;

		// Set BA for sprite 4, read data of sprite 2
		case 63:
#ifdef NTSC
			SprDataAccess(1, 1);
			SprDataAccess(1, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x08) {
				SetBALow;
			}
#else
			SprDataAccess(2, 1);
			SprDataAccess(2, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x10) {
				SetBALow;
			}

			ud_border_on = ud_border_set;

			retFlags = VIC_HBLANK;
#endif
			break;

#ifdef NTSC
		case 64:
			SprPtrAccess(2);
			SprDataAccess(2, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x0c)) {
				SetBAHigh;
			}
			break;

		case 65:
			SprDataAccess(2, 1);
			SprDataAccess(2, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x10) {
				SetBALow;
			}

			ud_border_on = ud_border_set;

			retFlags = VIC_HBLANK;
			break;
#endif
	}

	// Handle vertical border for next line
	if (raster_y == dy_stop) {
		ud_border_set = true;
	} else if (raster_y == dy_start && (ctrl1 & 0x10)) {
		ud_border_on = ud_border_set = false;
	}

#ifdef VIS_DEBUG
	// Sample VIC state for debugging
	vis_debug[cycle].ba_low = the_cpu->BALow;
	vis_debug[cycle].aec_delay = aec_delay;
	vis_debug[cycle].display_state = display_state;
	vis_debug[cycle].display_idx = display_idx;
	vis_debug[cycle].vbase = vbase;
	vis_debug[cycle].rc = rc;
#endif

	// Next cycle
	if (cycle != 57) {	// Keep current XSCROLL for final 0..7 pixels drawn in cycle 58
		x_scroll = new_x_scroll;
	}
	raster_x += 8;

	return retFlags;
}
