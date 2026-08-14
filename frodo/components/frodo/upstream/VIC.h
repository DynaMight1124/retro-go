/*
 *  VIC.h - 6569R5 emulation
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

#ifndef VIC_H
#define VIC_H


// Define this if you have a processor that can do unaligned accesses quickly
#if defined(__i386) || defined(__x86_64) || defined(mc68000) || defined(__MC68K__)
#define CAN_ACCESS_UNALIGNED
#endif


#ifdef NTSC
// Total number of raster lines (NTSC)
constexpr unsigned TOTAL_RASTERS = 0x107;
#else
// Total number of raster lines (PAL)
constexpr unsigned TOTAL_RASTERS = 0x138;
#endif

// Flags returned by EmulateCycle()/EmulateLine()
enum {
	VIC_HBLANK = 0x01,
	VIC_VBLANK = 0x02,
};


class MOS6510;
class Display;
class C64;
struct MOS6569State;


// 6569 emulation (VIC)
class MOS6569 {
public:
	MOS6569(C64 *c64, Display *disp, MOS6510 *CPU, uint8_t *RAM, uint8_t *Char, uint8_t *Color);

	uint8_t ReadRegister(uint16_t adr);
	void WriteRegister(uint16_t adr, uint8_t byte);

#ifdef FRODO_SC
	unsigned EmulateCycle();
#else
	unsigned EmulateLine(int & retCyclesLeft);
#endif

	void ChangedVA(uint16_t new_va);	// CIA VA14/15 has changed
	void TriggerLightpen();				// Trigger lightpen interrupt

	void GetState(MOS6569State *vd) const;
	void SetState(const MOS6569State *vd);

	// Get current raster line
	unsigned RasterY() const { return raster_y; }

#ifdef FRODO_SC
	uint8_t LastVICByte;
#endif

private:
	void raster_irq();
	void check_raster_irq();

	uint16_t mx[8];					// VIC registers
	uint8_t my[8];
	uint8_t mx8;
	uint8_t ctrl1, ctrl2;
	uint8_t lpx, lpy;
	uint8_t me, mxe, mye, mdp, mmc;
	uint8_t vbase;
	uint8_t irq_flag, irq_mask;
	uint8_t clx_spr, clx_bgr;
	uint8_t ec, b0c, b1c, b2c, b3c, mm0, mm1;
	uint8_t sc[8];

	uint8_t *ram, *char_rom, *color_ram; // Pointers to RAM and ROM
	C64 *the_c64;					// Pointer to C64 object
	Display *the_display;			// Pointer to Display object
	MOS6510 *the_cpu;				// Pointer to 6510 object

	uint8_t matrix_line[40];		// Buffer for video line, read in Bad Lines
	uint8_t color_line[40];			// Buffer for color line, read in Bad Lines

	uint8_t *chunky_line_start;		// Pointer to start of current line in bitmap buffer
	int xmod;						// Number of bytes per row

	uint16_t raster_y;				// Current raster line
	uint16_t irq_raster;			// Interrupt raster line
	uint16_t dy_start;				// Comparison values for border logic
	uint16_t dy_stop;
	uint16_t rc;					// Row counter
	uint16_t vc;					// Video counter
	uint16_t vc_base;				// Video counter base
	uint8_t x_scroll;				// X scroll value
	uint8_t y_scroll;				// Y scroll value
	uint16_t cia_vabase;			// CIA VA14/15 video base

	uint16_t mc[8];					// Sprite data counters

	unsigned display_idx;			// Index of current display mode

	long pad0;	// Keep buffers long-aligned
	uint8_t spr_coll_buf[0x1f8];				// Buffer for sprite-sprite collisions and priorities
	uint8_t fore_mask_buf[(0x200 + 48) / 8];	// Foreground mask for sprite-graphics collisions and priorities
												// (must be wider than line to handle X-expanded sprites under
												// the left border (MxX = 0x1e1..0x1f7))
#ifndef CAN_ACCESS_UNALIGNED
	uint8_t text_chunky_buf[40*8];				// Line graphics buffer
#endif

	bool display_state;			// true: Display state, false: Idle state
	bool border_on;				// Flag: Upper/lower border on (Frodo SC: Main border flip-flop)
	bool bad_lines_enabled;		// Flag: Bad Lines enabled for this frame
	bool lp_triggered;			// Flag: Lightpen was triggered in this frame

#ifdef FRODO_SC
	uint8_t read_byte(uint16_t adr);
	void matrix_access();
	void graphics_access();
	void load_pixel_shifter(uint8_t gfx_data, uint8_t char_data, uint8_t color_data);
	void draw_graphics();
	void draw_sprites();
	void draw_background();

	unsigned cycle;					// Current cycle in line (1..63)

	uint8_t *chunky_ptr;			// Pointer in chunky bitmap buffer (this is where our output goes)
	uint8_t *fore_mask_ptr;			// Pointer in fore_mask_buf

	uint16_t matrix_base;			// Video matrix base
	uint16_t char_base;				// Character generator base
	uint16_t bitmap_base;			// Bitmap base

	bool is_bad_line;				// Flag: Current line is bad line
	bool draw_this_line;			// Flag: Current line is drawn on the screen
	bool ud_border_on;				// Flag: Upper/lower border on
	bool ud_border_set;				// Flag: Set ud_border_on when checking for left/right border
	bool vblanking;					// Flag: VBlank in next cycle
	bool raster_irq_triggered;		// Flag: Raster IRQ triggered in this line
	bool hold_off_raster_irq;		// Flag: No raster IRQ in next line

	uint8_t aec_delay;				// Delay line for AEC after BA (111→011→001→000)

	bool border_on_sample[5];		// Samples of border state at different cycles (1, 17, 18, 56, 57)
	uint8_t border_color_sample[0x180/8+4]; // Samples of border color at each "displayed" cycle
											// (plus one extra)

	uint8_t ref_cnt;				// Refresh counter

	uint8_t spr_adv_y;				// 8 sprite advance line flip-flops
	uint8_t spr_dma_on;				// 8 flags: Sprite DMA active
	uint8_t spr_disp_on;			// 8 flags: Sprite armed for display
	uint16_t spr_ptr[8];			// Sprite data pointers
	uint16_t mc_base[8];			// Sprite data counter bases

	uint16_t raster_x;				// Current raster x position
	uint8_t new_x_scroll;			// Delay for XSCROLL value

	unsigned ml_index;				// Index in matrix/color_line[]

	uint16_t gfx_delay;				// Lower 8 bits: newly read data, shifted 8 bits up each cycle
	uint16_t char_delay;
	uint16_t color_delay;
	uint32_t pixel_shifter;			// Output color values (4-bit) for next 8 pixels (note: lower 4 bits = leftmost pixel)
	uint32_t back_pixel;			// Background pixel value in upper 4 bits
	uint8_t fore_mask_shifter;		// Foreground mask for next 8 pixel values

	uint8_t spr_data[8][4];			// Sprite data read from memory

	struct SprLatch {
		bool disp_on;				// Sprite display on
		bool mxe;					// X expansion flag
		bool mdp;					// Foreground priority flag
		bool mmc;					// Multicolor flag
		uint32_t data;				// Sprite data
		uint16_t mx;				// X position
		uint8_t sc;					// Sprite color
	};
	SprLatch spr_latch[8];			// Latched sprite data for drawing
#else
	uint8_t *get_physical(uint16_t adr);
	void make_mc_table();
	void el_std_text(uint8_t *p, uint8_t *q, uint8_t *r);
	void el_mc_text(uint8_t *p, uint8_t *q, uint8_t *r);
	void el_std_bitmap(uint8_t *p, uint8_t *q, uint8_t *r);
	void el_mc_bitmap(uint8_t *p, uint8_t *q, uint8_t *r);
	void el_ecm_text(uint8_t *p, uint8_t *q, uint8_t *r);
	void el_std_idle(uint8_t *p, uint8_t *r);
	void el_mc_idle(uint8_t *p, uint8_t *r);
	void el_sprites(uint8_t *chunky_ptr);
	int el_update_mc(int raster);

	uint8_t colors[256];			// Indices of the 16 C64 colors (16 times mirrored to avoid "& 0x0f")

	uint8_t ec_color, b0c_color, b1c_color,
	        b2c_color, b3c_color;	// Indices for exterior/background colors
	uint8_t mm0_color, mm1_color;	// Indices for MOB multicolors
	uint8_t spr_color[8];			// Indices for MOB colors

	uint32_t ec_color_long;			// ec_color expanded to 32 bits

	uint16_t mc_color_lookup[4];

	bool border_40_col;				// Flag: 40 column border
	uint8_t sprite_on;				// 8 flags: Sprite display/DMA active

	uint8_t *matrix_base;			// Video matrix base
	uint8_t *char_base;				// Character generator base
	uint8_t *bitmap_base;			// Bitmap base
#endif
};


// VIC state
struct MOS6569State {
	uint8_t m0x;				// Sprite coordinates
	uint8_t m0y;
	uint8_t m1x;
	uint8_t m1y;
	uint8_t m2x;
	uint8_t m2y;
	uint8_t m3x;
	uint8_t m3y;
	uint8_t m4x;
	uint8_t m4y;
	uint8_t m5x;
	uint8_t m5y;
	uint8_t m6x;
	uint8_t m6y;
	uint8_t m7x;
	uint8_t m7y;
	uint8_t mx8;

	uint8_t ctrl1;				// Control registers
	uint8_t raster;
	uint8_t lpx;
	uint8_t lpy;
	uint8_t me;
	uint8_t ctrl2;
	uint8_t mye;
	uint8_t vbase;
	uint8_t irq_flag;
	uint8_t irq_mask;
	uint8_t mdp;
	uint8_t mmc;
	uint8_t mxe;
	uint8_t mm;
	uint8_t md;

	uint8_t ec;					// Color registers
	uint8_t b0c;
	uint8_t b1c;
	uint8_t b2c;
	uint8_t b3c;
	uint8_t mm0;
	uint8_t mm1;
	uint8_t m0c;
	uint8_t m1c;
	uint8_t m2c;
	uint8_t m3c;
	uint8_t m4c;
	uint8_t m5c;
	uint8_t m6c;
	uint8_t m7c;
								// Additional registers
	uint8_t pad0;
	uint16_t irq_raster;		// IRQ raster line
	uint16_t vc;				// Video counter
	uint16_t vc_base;			// Video counter base
	uint8_t rc;					// Row counter
	uint8_t spr_dma;			// 8 Flags: Sprite DMA active
	uint8_t spr_disp;			// 8 Flags: Sprite display active
	uint8_t mc[8];				// Sprite data counters
	uint8_t mc_base[8];			// Sprite data counter bases
	bool display_state;			// true: Display state, false: Idle state
	bool bad_line;				// Flag: Bad Line state
	bool bad_line_enable;		// Flag: Bad Lines enabled for this frame
	bool lp_triggered;			// Flag: Lightpen was triggered in this frame
	bool border_on;				// Flag: Upper/lower border on (Frodo SC: Main border flip-flop)

	uint16_t bank_base;			// VIC bank base address
	uint16_t matrix_base;		// Video matrix base
	uint16_t char_base;			// Character generator base
	uint16_t bitmap_base;		// Bitmap base
	uint16_t sprite_base[8];	// Sprite bases

								// Frodo SC:
	uint16_t raster_x;			// Current raster x position
	uint8_t cycle;				// Current cycle in line (1..63)
	uint8_t ml_index;			// Index in matrix/color_line[]
	uint8_t ref_cnt;			// Refresh counter
	uint8_t last_vic_byte;		// Last byte read by VIC
	bool ud_border_on;			// Flag: Upper/lower border on
	bool ud_border_set;			// Flag: Set ud_border_on when checking for left/right border
	bool raster_irq_triggered;	// Flag: Raster IRQ triggered in this line
	bool hold_off_raster_irq;	// Flag: No raster IRQ in next line
};


#endif // ndef VIC_H
