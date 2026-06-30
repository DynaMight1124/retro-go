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

  Nintendo 8080 hardware

	- Space Fever
	- Space Fever High Splitter
	- Space Launcher
	- Sheriff / Bandido
	- Helifire

***************************************************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include <math.h>

#define HARDWARE_IS_SPACE_FEVER     ( n8080_hardware == 1 )
#define HARDWARE_IS_SHERIFF         ( n8080_hardware == 2 )
#define HARDWARE_IS_HELIFIRE        ( n8080_hardware == 3 )

static int n8080_hardware;

static timer_entry* sound_timer[3];
static timer_entry* cannon_timer;

static int mono_flop[3];

static int sheriff_color_mode;
static int sheriff_color_data;

static const float ATTACK_RATE = 10e-6 * 500;
static const float DECAY_RATE = 10e-6 * 16000;

static int helifire_dac_phase;
static float helifire_dac_volume;
static float helifire_dac_timing;

static unsigned char helifire_LSFR[63];
static unsigned char *helifire_palette;

static unsigned char* n8080_videoram;
static unsigned char* n8080_colorram;

static int spacefev_ufo_frame;
static int spacefev_ufo_cycle;
static int spacefev_red_screen;
static int spacefev_red_cannon;

static int helifire_flash;

static unsigned shift_data;
static unsigned shift_bits;

static unsigned short prev_sound_pins;
static unsigned short curr_sound_pins;

static int n8080_flip_screen;

static unsigned helifire_mv;
static unsigned helifire_sc; /* IC56 */


/* following data is based on screen shots */

static const unsigned char sheriff_color_PROM[] =
{
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x9, 0x9, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x2, 0x2, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x2, 0x2, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x9, 0x9, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
};


static const struct DACinterface n8080_dac_interface =
{
	1, { 30 }
};


struct SN76477interface sheriff_sn76477_interface =
{
	1,
	{ 35 },
	{ RES_K(36)  },  /* 04 */
	{ RES_K(100) },  /* 05 */
	{ CAP_N(1)   },  /* 06 */
	{ RES_K(620) },  /* 07 */
	{ CAP_U(1)   },  /* 08 */
	{ RES_K(20)  },  /* 10 */
	{ RES_K(150) },  /* 11 */
	{ RES_K(47)  },  /* 12 */
	{ 0          },  /* 16 */
	{ CAP_N(1)   },  /* 17 */
	{ RES_M(1.5) },  /* 18 */
	{ 0          },  /* 19 */
	{ RES_M(1.5) },  /* 20 */
	{ CAP_N(47)  },  /* 21 */
	{ CAP_N(47)  },  /* 23 */
	{ RES_K(560) },  /* 24 */
};


struct SN76477interface spacefev_sn76477_interface =
{
	1,
	{ 35 },
	{ RES_K(36)  },  /* 04 */
	{ RES_K(150) },  /* 05 */
	{ CAP_N(1)   },  /* 06 */
	{ RES_M(1)   },  /* 07 */
	{ CAP_U(1)   },  /* 08 */
	{ RES_K(20)  },  /* 10 */
	{ RES_K(150) },  /* 11 */
	{ RES_K(47)  },  /* 12 */
	{ 0          },  /* 16 */
	{ CAP_N(1)   },  /* 17 */
	{ RES_M(1.5) },  /* 18 */
	{ 0          },  /* 19 */
	{ RES_M(1)   },  /* 20 */
	{ CAP_N(47)  },  /* 21 */
	{ CAP_N(47)  },  /* 23 */
	{ RES_K(820) },  /* 24 */
};


static WRITE_HANDLER( n8080_shift_bits_w )
{
	shift_bits = data & 7;
}
static WRITE_HANDLER( n8080_shift_data_w )
{
	shift_data = (shift_data >> 8) | (data << 8);
}


static READ_HANDLER( n8080_shift_r )
{
	return shift_data >> (8 - shift_bits);
}


static void palette_init_n8080(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0; i <  Machine->drv->total_colors; i++)
	{
		//palette_change_color(i, (i & 1) ? 255 : 0, (i & 2) ? 255 : 0, (i & 4) ? 255 : 0);

		*(palette++) = (i & 1) ? 255 : 0;
		*(palette++) = (i & 2) ? 255 : 0;
		*(palette++) = (i & 4) ? 255 : 0;
	}
}


static void palette_init_helifire(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	helifire_palette = palette; // Save game_palette for later

	for (i = 0; i < 8; i++)
	{
		/* core colors */
		*(palette++) = (i & 1) ? 255 : 0;
		*(palette++) = (i & 2) ? 255 : 0;
		*(palette++) = (i & 4) ? 255 : 0;
	}

	for (i = 0; i < 0x20; i++)
	{
		int level = 0xff * exp(-3 * i / 31.); /* capacitor discharge */
		/* shades of blue */
		*(palette++) = 0;
		*(palette++) = 0;
		*(palette++) = level;
	}

	for (i = 0; i < 0x20; i++)
	{
		int level = 0xff * exp(-3 * i / 31.); /* capacitor discharge */
		/* shades of blue w/ green star */
		*(palette++) = 0;
		*(palette++) = 0xc0;
		*(palette++) = level;
	}

	for (i = 0; i < 0x20; i++)
	{
		int level = 0xff * exp(-3 * i / 31.); /* capacitor discharge */
		/* shades of red */
		*(palette++) = level;
		*(palette++) = 0;
		*(palette++) = 0;
	}

	for (i = 0; i < 0x20; i++)
	{
		int level = 0xff * exp(-3 * i / 31.); /* capacitor discharge */
		/* shades of red w/ green star */
		*(palette++) = level;
		*(palette++) = 0xc0;
		*(palette++) = 0;
	}
}



static int spacefev_vh_start(void)
{
	n8080_flip_screen = 0;

	spacefev_ufo_frame = 0;
	spacefev_ufo_cycle = 0;



	spacefev_red_screen = 0;
	spacefev_red_cannon = 0;

	return 0;
}

static int sheriff_vh_start(void)
{
	n8080_flip_screen = 0;

	sheriff_color_mode = 0;
	sheriff_color_data = 0;

	return 0;
}


static int helifire_vh_start(void)
{
	unsigned char data = 0;

	int i;

	helifire_mv = 0;
	helifire_sc = 0;

	helifire_flash = 0;
	n8080_flip_screen = 0;

	for (i = 0; i < 63; i++)
	{
		int bit =
			(data >> 6) ^
			(data >> 7) ^ 1;

		data = (data << 1) | (bit & 1);

		helifire_LSFR[i] = data;
	}

	return 0;
}

void helifire_next_line(void)
{
	helifire_mv++;

	if (helifire_sc % 4 == 2)
	{
	    helifire_mv %= 256;
	}
	else
	{
	    if (flip_screen)
	    {
		helifire_mv %= 255;
	    }
	    else
	    {
		helifire_mv %= 257;
	    }
	}

	if (helifire_mv == 128)
	{
	    helifire_sc++;
	}
}


void spacefev_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mask = n8080_flip_screen ? 0xff : 0x00;

	int x;
	int y;

	const unsigned char* pRAM = n8080_videoram;

	for (y = 0; y < 256; y++)
	{
		//unsigned char* pLine = bitmap->line[y ^ mask];
		int yc = y ^ mask;

		for (x = 0; x < 256; x += 8)
		{
			int n;

			unsigned char color = 0;

			if (spacefev_red_screen)
			{
				color = 1;
			}
			else
			{
				unsigned char val = memory_region(REGION_PROMS)[x >> 3];

				if ((x >> 3) == 0x06)
				{
					color = spacefev_red_cannon ? 1 : 7;
				}

				if ((x >> 3) == 0x1b)
				{
					static const unsigned char ufo_color[] =
					{
						1, /* red     */
						2, /* green   */
						7, /* white   */
						3, /* yellow  */
						5, /* magenta */
						6, /* cyan    */
					};

					color = ufo_color[spacefev_ufo_cycle];
				}

				for (n = color + 1; n < 8; n++)
				{
					if (~val & (1 << n))
					{
						color = n;
					}
				}
			}

			for (n = 0; n < 8; n++)
			{
				//pLine[(x + n) ^ mask] = (pRAM[x >> 3] & (1 << n)) ? color : 0;
				int xc = (x + n) ^ mask;
				int pc = (pRAM[x >> 3] & (1 << n)) ? color : 0;
				plot_pixel(Machine->scrbitmap,xc,yc,Machine->pens[pc]);
			}
		}

		pRAM += 32;
	}
}


void sheriff_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mask = n8080_flip_screen ? 0xff : 0x00;

	int x;
	int y;

	const unsigned char* pRAM = n8080_videoram;

	for (y = 0; y < 256; y++)
	{
		//unsigned char* pLine = bitmap->line[y ^ mask];
		int yc = y ^ mask;

		for (x = 0; x < 256; x += 8)
		{
			int n;

			unsigned char color = sheriff_color_PROM[32 * (y >> 3) + (x >> 3)];

			if (sheriff_color_mode == 1 && !(color & 8))
			{
				color = sheriff_color_data ^ 7;
			}

			if (sheriff_color_mode == 2)
			{
				color = sheriff_color_data ^ 7;
			}

			if (sheriff_color_mode == 3)
			{
				color = 7;
			}

			for (n = 0; n < 8; n++)
			{
				//pLine[(x + n) ^ mask] = (pRAM[x >> 3] & (1 << n)) ? (color & 7) : 0;
				int xc = (x + n) ^ mask;
				int pc = (pRAM[x >> 3] & (1 << n)) ? (color & 7) : 0;
				//int pc = (pRAM[x >> 3] & (1 << n)) ? 7 : 0; // always white
				plot_pixel(Machine->scrbitmap,xc,yc,Machine->pens[pc]);
			}
		}

		pRAM += 32;
	}
}


void helifire_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int SUN_BRIGHTNESS = readinputport(4);
	int SEA_BRIGHTNESS = readinputport(5);

	static const int wave[8] = { 0, 1, 2, 2, 2, 1, 0, 0 };

	unsigned saved_mv = helifire_mv;
	unsigned saved_sc = helifire_sc;

	int x;
	int y;


	for (y = 0; y < 256; y++)
	{
		//UINT16* pLine = bitmap->line[y];

		int level = 120 + wave[helifire_mv & 7];

		/* draw sky */

		for (x = level; x < 256; x++)
		{
			//pLine[x] = 0x200 + 8 + SUN_BRIGHTNESS + x - level;
			int pc = 0x40 + 8 + ((SUN_BRIGHTNESS + x - level) >> 3);
			plot_pixel(Machine->scrbitmap,x,y,Machine->pens[pc]);
		}

		/* draw stars */

		if (helifire_mv % 8 == 4) /* upper half */
		{
			int step = (320 * helifire_mv) % sizeof helifire_LSFR;

			int data =
				((helifire_LSFR[step] & 1) << 6) |
				((helifire_LSFR[step] & 2) << 4) |
				((helifire_LSFR[step] & 4) << 2) |
				((helifire_LSFR[step] & 8) << 0);

			//pLine[0x80 + data] |= 0x100;
			int xc = 0x80 + data;
			int pc = 0x40 + 8 + ((SUN_BRIGHTNESS + xc - level) >> 3);
			pc |= 0x20;
			plot_pixel(Machine->scrbitmap,xc,y,Machine->pens[pc]);
		}
		if (helifire_mv % 8 == 5) /* lower half */
		{
			int step = (320 * (helifire_mv - 1)) % sizeof helifire_LSFR;

			int data =
				((helifire_LSFR[step] & 1) << 6) |
				((helifire_LSFR[step] & 2) << 4) |
				((helifire_LSFR[step] & 4) << 2) |
				((helifire_LSFR[step] & 8) << 0);

			//pLine[0x00 + data] |= 0x100;
			int xc = data;
			int pc = 0x40 + 8 + ((SUN_BRIGHTNESS + xc - level) >> 3);
			pc |= 0x20;
			plot_pixel(Machine->scrbitmap,xc,y,Machine->pens[pc]);
		}

		/* draw sea */

		for (x = 0; x < level; x++)
		{
			//pLine[x] = 8 + SEA_BRIGHTNESS + x;
			int pc = 8 + ((SEA_BRIGHTNESS + x ) >> 3);
			plot_pixel(Machine->scrbitmap,x,y,Machine->pens[pc]);
		}

		/* draw foreground */

		for (x = 0; x < 256; x += 8)
		{
			int offset = 32 * y + (x >> 3);

			int n;

			for (n = 0; n < 8; n++)
			{
				if (n8080_flip_screen)
				{
					if ((n8080_videoram[offset ^ 0x1fff] << n) & 0x80)
					{
						//pLine[x + n] = colorram[offset ^ 0x1fff] & 7;
						int xc = x + n;
						int pc = n8080_colorram[offset ^ 0x1fff] & 7;
						plot_pixel(Machine->scrbitmap,xc,y,Machine->pens[pc]);
					}
				}
				else
				{
					if ((n8080_videoram[offset] >> n) & 1)
					{
						//pLine[x + n] = colorram[offset] & 7;
						int xc = x + n;
						int pc = n8080_colorram[offset] & 7;
						plot_pixel(Machine->scrbitmap,xc,y,Machine->pens[pc]);
					}
				}
			}
		}
		/* next line */

		helifire_next_line();

	}

	helifire_mv = saved_mv;
	helifire_sc = saved_sc;
}


void helifire_vh_eof(void)
{
    int n = (cpu_getcurrentframe() >> 1) % sizeof helifire_LSFR;

    int i;

    unsigned char * palette = helifire_palette;
    for (i = 0; i < 8; i++)
    {
	int R = (i & 1);
	int G = (i & 2);
	int B = (i & 4);

	if (helifire_flash)
	{
	    if (helifire_LSFR[n] & 0x20)
	    {
		G |= B;
	    }

	    if (cpu_getcurrentframe() & 0x04)
	    {
		R |= G;
	    }
	}

	//palette_set_color(i, R ? 255 : 0, G ? 255 : 0, B ? 255 : 0);
	// Hack to set the palette
	*(palette++) = (i & 1) ? 255 : 0;
	*(palette++) = (i & 2) ? 255 : 0;
	*(palette++) = (i & 4) ? 255 : 0;
    }

    for (i = 0; i < 256; i++)
    {
	helifire_next_line();
    }
}


static int n8080_interrupt (void)
{
	static int count = 0;

	count++;

	if (count & 1)
		return 0x00cf;  /* RST 08h */
	else
		return 0x00d7;  /* RST 10h */
}


static void spacefev_vco_voltage_timer(int dummy)
{
	double voltage = 0;

	if (mono_flop[2])
	{
		voltage = 5 * (1 - exp(- timer_timeelapsed(sound_timer[2]) / 0.22));
	}

	SN76477_set_vco_voltage(0, voltage);
}


static void helifire_dac_volume_timer(int dummy)
{
    float t = helifire_dac_timing - timer_get_time();

    if (helifire_dac_phase)
    {
	helifire_dac_volume = 1 - exp(t / ATTACK_RATE);
    }
    else
    {
	helifire_dac_volume = exp(t / DECAY_RATE);
    }
}



static void spacefev_update_SN76477_status(void)
{
	double dblR0 = RES_M(1.0);
	double dblR1 = RES_M(1.5);

	if (!mono_flop[0])
	{
		dblR0 = 1 / (1 / RES_K(150) + 1 / dblR0); /* ? */
	}
	if (!mono_flop[1])
	{
		dblR1 = 1 / (1 / RES_K(620) + 1 / dblR1); /* ? */
	}

	SN76477_set_decay_res(0, dblR0);

	SN76477_set_vco_res(0, dblR1);

	SN76477_enable_w(0,
		!mono_flop[0] &&
		!mono_flop[1] &&
		!mono_flop[2]);

	SN76477_vco_w(0, mono_flop[1]);

	SN76477_mixer_b_w(0, mono_flop[0]);
}


static void sheriff_update_SN76477_status(void)
{
	if (mono_flop[1])
	{
		SN76477_set_vco_voltage(0, 5);
	}
	else
	{
		SN76477_set_vco_voltage(0, 0);
	}

	SN76477_enable_w(0,
		!mono_flop[0] &&
		!mono_flop[1]);

	SN76477_vco_w(0, mono_flop[0]);

	SN76477_mixer_b_w(0, !mono_flop[0]);
}


static void update_SN76477_status(void)
{
	if (HARDWARE_IS_SPACE_FEVER)
	{
		spacefev_update_SN76477_status();
	}
	if (HARDWARE_IS_SHERIFF)
	{
		sheriff_update_SN76477_status();
	}
}


static void start_mono_flop(int n, double expire)
{
	mono_flop[n] = 1;

	update_SN76477_status();

	//timer_adjust(sound_timer[n], expire, n, 0);
	timer_reset(sound_timer[n], expire);
}


static void stop_mono_flop(int n)
{
	mono_flop[n] = 0;

	update_SN76477_status();

	//timer_adjust(sound_timer[n], TIME_NEVER, n, 0);
	timer_reset(sound_timer[n], TIME_NEVER);
}


static void spacefev_start_red_cannon(void)
{
	spacefev_red_cannon = 1;

	//timer_adjust(cannon_timer, TIME_IN_MSEC(0.55 * 68 * 10), 0, 0);
	timer_reset(cannon_timer, TIME_IN_MSEC(0.55 * 68 * 10));
}


static void spacefev_stop_red_cannon(int dummy)
{
	spacefev_red_cannon = 0;

	//timer_adjust(cannon_timer, TIME_NEVER, 0, 0);
	timer_reset(cannon_timer, TIME_NEVER);
}



static void spacefev_sound_pins_changed(void)
{
	unsigned short changes = ~curr_sound_pins & prev_sound_pins;

	if (changes & (1 << 0x3))
	{
		stop_mono_flop(1);
	}
	if (changes & ((1 << 0x3) | (1 << 0x6)))
	{
		stop_mono_flop(2);
	}
	if (changes & (1 << 0x3))
	{
		start_mono_flop(0, TIME_IN_MSEC(0.55 * 36 * 100));
	}
	if (changes & (1 << 0x6))
	{
		start_mono_flop(1, TIME_IN_MSEC(0.55 * 22 * 33));
	}
	if (changes & (1 << 0x4))
	{
		start_mono_flop(2, TIME_IN_MSEC(0.55 * 22 * 33));
	}
	if (changes & ((1 << 0x2) | (1 << 0x3) | (1 << 0x5)))
	{
		cpu_set_irq_line(1, 0, PULSE_LINE);
	}
}


static void sheriff_sound_pins_changed(void)
{
	unsigned short changes = ~curr_sound_pins & prev_sound_pins;

	if (changes & (1 << 0x6))
	{
		stop_mono_flop(1);
	}
	if (changes & (1 << 0x6))
	{
		start_mono_flop(0, TIME_IN_MSEC(0.55 * 33 * 33));
	}
	if (changes & (1 << 0x4))
	{
		start_mono_flop(1, TIME_IN_MSEC(0.55 * 33 * 33));
	}
	if (changes & ((1 << 0x2) | (1 << 0x3) | (1 << 0x5)))
	{
		cpu_set_irq_line(1, 0, PULSE_LINE);
	}
}


static void helifire_sound_pins_changed(void)
{
	unsigned short changes = ~curr_sound_pins & prev_sound_pins;

	/* lacking emulation of sound bits 10, 11, 12 and 4 */

	if (changes & (1 << 6))
	{
		cpu_set_irq_line(1, 0, PULSE_LINE);
	}
}


static void sound_pins_changed(void)
{
	if (HARDWARE_IS_SPACE_FEVER)
	{
		spacefev_sound_pins_changed();
	}
	if (HARDWARE_IS_SHERIFF)
	{
		sheriff_sound_pins_changed();
	}
	if (HARDWARE_IS_HELIFIRE)
	{
		helifire_sound_pins_changed();
	}

	prev_sound_pins = curr_sound_pins;
}


static void delayed_sound_1(int data)
{
	static unsigned char prev_data = 0;

	curr_sound_pins &= ~(
		(1 << 0x7) |
		(1 << 0x5) |
		(1 << 0x6) |
		(1 << 0x3) |
		(1 << 0x4) |
		(1 << 0x1));

	if (~data & 0x01) curr_sound_pins |= 1 << 0x7;
	if (~data & 0x02) curr_sound_pins |= 1 << 0x5; /* pulse */
	if (~data & 0x04) curr_sound_pins |= 1 << 0x6; /* pulse */
	if (~data & 0x08) curr_sound_pins |= 1 << 0x3; /* pulse (except in Helifire) */
	if (~data & 0x10) curr_sound_pins |= 1 << 0x4; /* pulse (except in Helifire) */
	if (~data & 0x20) curr_sound_pins |= 1 << 0x1;

	if (HARDWARE_IS_SPACE_FEVER)
	{
		if (data & ~prev_data & 0x10)
		{
			spacefev_start_red_cannon();
		}

		spacefev_red_screen = data & 0x08;
		prev_data = data;
	}

	sound_pins_changed();

}


static void delayed_sound_2(int data)
{
	curr_sound_pins &= ~(
		(1 << 0x8) |
		(1 << 0x9) |
		(1 << 0xA) |
		(1 << 0xB) |
		(1 << 0x2) |
		(1 << 0xC));

	if (~data & 0x01) curr_sound_pins |= 1 << 0x8;
	if (~data & 0x02) curr_sound_pins |= 1 << 0x9;
	if (~data & 0x04) curr_sound_pins |= 1 << 0xA;
	if (~data & 0x08) curr_sound_pins |= 1 << 0xB;
	if (~data & 0x10) curr_sound_pins |= 1 << 0x2; /* pulse */
	if (~data & 0x20) curr_sound_pins |= 1 << 0xC;

	if (HARDWARE_IS_SPACE_FEVER)
	{
		n8080_flip_screen = data & 0x20;
	}
	if (HARDWARE_IS_HELIFIRE)
	{
		helifire_flash = data & 0x20;
	}

	sound_pins_changed();
}


static WRITE_HANDLER( n8080_sound_1_w )
{
	timer_set(TIME_NOW, data, delayed_sound_1); /* force CPUs to sync */
}
static WRITE_HANDLER( n8080_sound_2_w )
{
	timer_set(TIME_NOW, data, delayed_sound_2); /* force CPUs to sync */
}


static READ_HANDLER( n8080_8035_p1_r )
{
	unsigned char val = 0;

	if (curr_sound_pins & (1 << 0xB)) val |= 0x01;
	if (curr_sound_pins & (1 << 0xA)) val |= 0x02;
	if (curr_sound_pins & (1 << 0x9)) val |= 0x04;
	if (curr_sound_pins & (1 << 0x8)) val |= 0x08;
	if (curr_sound_pins & (1 << 0x5)) val |= 0x10;
	if (curr_sound_pins & (1 << 0x3)) val |= 0x20;
	if (curr_sound_pins & (1 << 0x2)) val |= 0x40;
	if (curr_sound_pins & (1 << 0x1)) val |= 0x80;

	return val;
}


static READ_HANDLER( helifire_8035_external_ram_r )
{
	unsigned char val = 0;

	if (curr_sound_pins & (1 << 0x7)) val |= 0x01;
	if (curr_sound_pins & (1 << 0x8)) val |= 0x02;
	if (curr_sound_pins & (1 << 0x9)) val |= 0x04;
	if (curr_sound_pins & (1 << 0x1)) val |= 0x08;

	return val;
}


static READ_HANDLER( n8080_8035_t0_r )
{
	return (curr_sound_pins >> 0x7) & 1;
}
static READ_HANDLER( n8080_8035_t1_r )
{
	return (curr_sound_pins >> 0xC) & 1;
}


static READ_HANDLER( helifire_8035_t0_r )
{
	return (curr_sound_pins >> 0x3) & 1;
}

static READ_HANDLER( helifire_8035_t1_r )
{
	return (curr_sound_pins >> 0x4) & 1;
}

static READ_HANDLER( helifire_8035_p2_r )
{
       return ((curr_sound_pins >> 0xC) & 1) ? 0x10 : 0x00; /* not used */
}


static WRITE_HANDLER( n8080_dac_w )
{
	DAC_data_w(0, data & 0x80);
}


static WRITE_HANDLER( helifire_dac_w )
{
//if (helifire_dac_volume != 1.0 || data != 0)
	DAC_data_w(0, data * helifire_dac_volume);
//if (helifire_dac_volume != 1.0 || data != 0)
}


static WRITE_HANDLER( helifire_sound_ctrl_w )
{
    helifire_dac_phase = data & 0x80;

    /* data & 0x40 not emulated */
    /* data & 0x20 not emulated */

    if (helifire_dac_phase)
    {
	helifire_dac_timing = ATTACK_RATE * log(1 - helifire_dac_volume);
    }
    else
    {
	helifire_dac_timing = DECAY_RATE * log(helifire_dac_volume);
    }

    helifire_dac_timing += timer_get_time();
}


static WRITE_HANDLER( n8080_video_control_w )
{
	sheriff_color_mode = (data >> 3) & 3;
	sheriff_color_data = (data >> 0) & 7;

	n8080_flip_screen = data & 0x20;
}


static const struct MemoryReadAddress n8080_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress n8080_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM, &n8080_videoram },
	{ -1 }  /* end of table */
};

static const struct MemoryReadAddress helifire_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress helifire_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM, &n8080_videoram },
	{ 0xc000, 0xdfff, MWA_RAM, &n8080_colorram },
	{ -1 }  /* end of table */
};

static const struct IOReadPort n8080_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, n8080_shift_r },
	{ 0x04, 0x04, input_port_3_r },
	{ -1 }  /* end of table */
};

static const struct IOWritePort n8080_writeport[] =
{
	{ 0x02, 0x02, n8080_shift_bits_w },
	{ 0x03, 0x03, n8080_shift_data_w },
	{ 0x04, 0x04, n8080_sound_1_w },
	{ 0x05, 0x05, n8080_sound_2_w },
	{ 0x06, 0x06, n8080_video_control_w },
	{ -1 }  /* end of table */
};


static const struct MemoryReadAddress n8080_sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress n8080_sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static const struct IOReadPort n8080_sound_readport[] =
{
	{ I8039_t0, I8039_t0, n8080_8035_t0_r },
	{ I8039_t1, I8039_t1, n8080_8035_t1_r },
	{ I8039_p1, I8039_p1, n8080_8035_p1_r },
	{ -1 }  /* end of table */
};

static const struct IOWritePort n8080_sound_writeport[] =
{
	{ I8039_p2, I8039_p2, n8080_dac_w },
	{ -1 }  /* end of table */
};


static const struct IOReadPort helifire_sound_readport[] =
{
	{ I8039_t0, I8039_t0, helifire_8035_t0_r },
	{ I8039_t1, I8039_t1, helifire_8035_t1_r },
	{ I8039_p2, I8039_p2, helifire_8035_p2_r },
	{ 0x00,	    0x7f,     helifire_8035_external_ram_r },
	{ -1 }  /* end of table */
};

static const struct IOWritePort helifire_sound_writeport[] =
{
	{ I8039_p1, I8039_p1, helifire_dac_w },
	{ I8039_p2, I8039_p2, helifire_sound_ctrl_w },
	{ -1 }  /* end of table */
};


void init_machine_spacefev(void)
{
	n8080_hardware = 1;

	timer_pulse(TIME_IN_HZ(1000), 0, spacefev_vco_voltage_timer);

	//sound_timer[0] = timer_alloc(stop_mono_flop);
	//sound_timer[1] = timer_alloc(stop_mono_flop);
	//sound_timer[2] = timer_alloc(stop_mono_flop);
	sound_timer[0] = timer_set(TIME_NEVER, 0, stop_mono_flop);
	sound_timer[1] = timer_set(TIME_NEVER, 1, stop_mono_flop);
	sound_timer[2] = timer_set(TIME_NEVER, 2, stop_mono_flop);

	//cannon_timer = timer_alloc(spacefev_stop_red_cannon);
	cannon_timer = timer_set(TIME_NEVER, 0, spacefev_stop_red_cannon);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_noise_clock_w(0, 0);

	mono_flop[0] = 0;
	mono_flop[1] = 0;
	mono_flop[2] = 0;

	delayed_sound_1(0);
	delayed_sound_2(0);
}


void init_machine_sheriff(void)
{
	n8080_hardware = 2;

	//sound_timer[0] = timer_alloc(stop_mono_flop);
	//sound_timer[1] = timer_alloc(stop_mono_flop);
	sound_timer[0] = timer_set(TIME_NEVER, 0, stop_mono_flop);
	sound_timer[1] = timer_set(TIME_NEVER, 1, stop_mono_flop);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_noise_clock_w(0, 0);

	mono_flop[0] = 0;
	mono_flop[1] = 0;

	delayed_sound_1(0);
	delayed_sound_2(0);

	n8080_video_control_w(0, 0);
}


void init_machine_helifire(void)
{
	n8080_hardware = 3;

	timer_pulse(TIME_IN_HZ(1000), 0, helifire_dac_volume_timer);

	helifire_dac_volume = 1.0;
	helifire_dac_timing = 0.0;

	delayed_sound_1(0);
	delayed_sound_2(0);

	n8080_video_control_w(0, 0);

	helifire_dac_phase = 0;
}


static const struct MachineDriver machine_driver_sheriff =
{
    /* basic machine hardware */
    {
	{
	    CPU_8080,
	    2016000,        /* 2.016 Mhz */
	    n8080_readmem,n8080_writemem,
	    n8080_readport,n8080_writeport,
	    n8080_interrupt,2    /* two interrupts per frame */
	},
	{
	    CPU_I8035 | CPU_AUDIO_CPU,
	    400000,     /* 0.4 Mhz */
	    n8080_sound_readmem,n8080_sound_writemem,
	    n8080_sound_readport,n8080_sound_writeport,
	    ignore_interrupt,1
	}
    },
    60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
    1,      /* no need for interleaving */
    init_machine_sheriff,

    /* video hardware */
    32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
    0,      /* no gfxdecodeinfo - bitmapped display */
    8, 0,   /* 8 colors mapped to the overlay */
    palette_init_n8080,

    VIDEO_TYPE_RASTER,
    0,	/* vh_eof */
    sheriff_vh_start,
    0,  /* vh_stop */
    sheriff_vh_screenrefresh,

    /* sound hardware */
    0, 0, 0, 0,
    {
	{
	    SOUND_DAC,
	    &n8080_dac_interface
	},
	{
	    SOUND_SN76477,
	    &sheriff_sn76477_interface
	}
    }
};

static const struct MachineDriver machine_driver_spacefev =
{
    /* basic machine hardware */
    {
	{
	    CPU_8080,
	    2016000,        /* 2.016 Mhz */
	    n8080_readmem,n8080_writemem,
	    n8080_readport,n8080_writeport,
	    n8080_interrupt,2    /* two interrupts per frame */
	},
	{
	    CPU_I8035 | CPU_AUDIO_CPU,
	    400000,     /* 0.4 Mhz */
	    n8080_sound_readmem,n8080_sound_writemem,
	    n8080_sound_readport,n8080_sound_writeport,
	    ignore_interrupt,1
	}
    },
    60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
    1,      /* no need for interleaving */
    init_machine_spacefev,

    /* video hardware */
    32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
    0,      /* no gfxdecodeinfo - bitmapped display */
    8, 0,   /* 8 colors */
    palette_init_n8080,

    VIDEO_TYPE_RASTER,
    0,  /* vh_eof */
    spacefev_vh_start,
    0,  /* vh_stop */
    spacefev_vh_screenrefresh,

    /* sound hardware */
    0, 0, 0, 0,
    {
	{
	    SOUND_DAC,
	    &n8080_dac_interface
	},
	{
	    SOUND_SN76477,
	    &spacefev_sn76477_interface
	}
    }
};

static const struct MachineDriver machine_driver_helifire =
{
    /* basic machine hardware */
    {
	{
	    CPU_8080,
	    2016000,        /* 2.016 Mhz */
	    helifire_readmem,helifire_writemem,
	    n8080_readport,n8080_writeport,
	    n8080_interrupt,2    /* two interrupts per frame */
	},
	{
	    CPU_I8035 | CPU_AUDIO_CPU,
	    400000,     /* 0.4 Mhz */
	    n8080_sound_readmem,n8080_sound_writemem,
	    helifire_sound_readport,helifire_sound_writeport,
	    ignore_interrupt,1
	}
    },
    60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
    1,      /* single CPU, no need for interleaving */
    init_machine_helifire,

    /* video hardware */
    32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
    0,      /* no gfxdecodeinfo - bitmapped display */
    0x80 + 8, 0,   /* 128 + 8 colors */
    palette_init_helifire,

    VIDEO_TYPE_RASTER,
    helifire_vh_eof,
    helifire_vh_start,
    0,  /* vh_stop */
    helifire_vh_screenrefresh,

    /* sound hardware */
    0, 0, 0, 0,
    {
	{
	    SOUND_DAC,
	    &n8080_dac_interface
	}
    }
};



INPUT_PORTS_START( spacefev )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BITX(0x08, 0x00, 0, "Game A", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, 0, "Game B", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, 0, "Game C", KEYCODE_E, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* enables diagnostic ROM at $1c00 */

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START

INPUT_PORTS_END


INPUT_PORTS_START( sheriff )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 enables diagnostic ROM at $2400 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
INPUT_PORTS_END


INPUT_PORTS_START( bandido )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN )

	PORT_START

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 enables diagnostic ROM at $2400 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown )) /* don't know if this is used */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
INPUT_PORTS_END


INPUT_PORTS_START( helifire )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x04, "6000" )
	PORT_DIPSETTING(    0x08, "8000" )
	PORT_DIPSETTING(    0x0c, "10000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))

	/* potentiometers */

	PORT_START	/* 04 */
	PORT_DIPNAME( 0xff, 0x50, "VR1 sun brightness" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x30, "30" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x50, "50" )
	PORT_DIPSETTING(    0x60, "60" )
	PORT_DIPSETTING(    0x70, "70" )

	PORT_START	/* 05 */
	PORT_DIPNAME( 0xff, 0x00, "VR2 sea brightness" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x30, "30" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x50, "50" )
	PORT_DIPSETTING(    0x60, "60" )
	PORT_DIPSETTING(    0x70, "70" )

INPUT_PORTS_END


ROM_START( spacefev )
	ROM_REGION( 0x8000, REGION_CPU1 )
	ROM_LOAD( "sf.f1",    0x0000, 0x0400, 0x35f295bd)
	ROM_LOAD( "sf.f2",    0x0400, 0x0400, 0x0c633f4c)
	ROM_LOAD( "sf.g1",    0x0800, 0x0400, 0xf3d851cb)
	ROM_LOAD( "sf.g2",    0x0c00, 0x0400, 0x1faef63a)
	ROM_LOAD( "sf.h1",    0x1000, 0x0400, 0xb365389d)
	ROM_LOAD( "sf.h2",    0x1400, 0x0400, 0xa163e800)
	ROM_LOAD( "sf.i1",    0x1800, 0x0400, 0x00027be2)

	ROM_REGION( 0x0400, REGION_CPU2 )
	ROM_LOAD( "sf.snd",   0x0000, 0x0400, 0x95c2c1ee)

	ROM_REGION( 0x0020, REGION_PROMS )
	ROM_LOAD( "sf.prm",   0x0000, 0x0020, 0xc5914ec1)
ROM_END

ROM_START( spacefva )
	ROM_REGION( 0x8000, REGION_CPU1 )
	ROM_LOAD( "sf-a.f1",    0x0000, 0x0400, 0x7fa305e8)
	ROM_LOAD( "sf-a.f2",    0x0400, 0x0400, 0x7c1429aa)
	ROM_LOAD( "sf-a.g1",    0x0800, 0x0400, 0x75f6efc1)
	ROM_LOAD( "sf-a.g2",    0x0c00, 0x0400, 0xfb6bcf4a)
	ROM_LOAD( "sf-a.h1",    0x1000, 0x0400, 0x3beef037)
	ROM_LOAD( "sf-a.h2",    0x1400, 0x0400, 0xbddbc94f)
	ROM_LOAD( "sf-a.i1",    0x1800, 0x0400, 0x437786c5)

	ROM_REGION( 0x0400, REGION_CPU2 )
	ROM_LOAD( "sf.snd",   0x0000, 0x0400, 0x95c2c1ee)

	ROM_REGION( 0x0020, REGION_PROMS )
	ROM_LOAD( "sf.prm",   0x0000, 0x0020, 0xc5914ec1)
ROM_END

ROM_START( sheriff )
	ROM_REGION( 0x8000, REGION_CPU1 )
	ROM_LOAD( "f1",    0x0000, 0x0400, 0xe79df6e8)
	ROM_LOAD( "f2",    0x0400, 0x0400, 0xda67721a)
	ROM_LOAD( "g1",    0x0800, 0x0400, 0x3fb7888e)
	ROM_LOAD( "g2",    0x0c00, 0x0400, 0x585fcfee)
	ROM_LOAD( "h1",    0x1000, 0x0400, 0xe59eab52)
	ROM_LOAD( "h2",    0x1400, 0x0400, 0x79e69a6a)
	ROM_LOAD( "i1",    0x1800, 0x0400, 0xdda7d1e8)
	ROM_LOAD( "i2",    0x1c00, 0x0400, 0x5c5f3f86)
	ROM_LOAD( "j1",    0x2000, 0x0400, 0x0aa8b79a)

	ROM_REGION( 0x0400, REGION_CPU2 )
	ROM_LOAD( "basnd.u2",   0x0000, 0x0400, 0x75731745)

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "sh.prm",   0x0000, 0x0400, 0x0 )
ROM_END

ROM_START( bandido )
	ROM_REGION( 0x8000, REGION_CPU1 )             /* 64k for code */
	ROM_LOAD( "baf1-3",       0x0000, 0x0400, 0xaec94829 )
	ROM_LOAD( "f2",           0x0400, 0x0400, 0xda67721a )
	ROM_LOAD( "g1",           0x0800, 0x0400, 0x3fb7888e )
	ROM_LOAD( "g2",           0x0c00, 0x0400, 0x585fcfee )
	ROM_LOAD( "bah1-1",       0x1000, 0x0400, 0x5cb63677 )
	ROM_LOAD( "h2",           0x1400, 0x0400, 0x79e69a6a )
	ROM_LOAD( "i1",           0x1800, 0x0400, 0xdda7d1e8 )
	ROM_LOAD( "i2",           0x1c00, 0x0400, 0x5c5f3f86 )
	ROM_LOAD( "j1",           0x2000, 0x0400, 0x0aa8b79a )
	ROM_LOAD( "baj2-2",       0x2400, 0x0400, 0xa10b848a )

	ROM_REGION( 0x0400, REGION_CPU2 )       /* Sound 8035 + 76477 Sound Generator */
	ROM_LOAD( "basnd.u2",     0x0000, 0x0400, 0x75731745 )

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "sh.prm",   0x0000, 0x0400, 0x0 )
ROM_END

ROM_START( helifire )
	ROM_REGION( 0x8000, REGION_CPU1 )
	ROM_LOAD( "hf.f1",    0x0000, 0x0400, 0x032f89ca)
	ROM_LOAD( "hf.f2",    0x0400, 0x0400, 0x2774e70f)
	ROM_LOAD( "hf.g1",    0x0800, 0x0400, 0xb5ad6e8a)
	ROM_LOAD( "hf.g2",    0x0c00, 0x0400, 0x5e015bf4)
	ROM_LOAD( "hf.h1",    0x1000, 0x0400, 0x23bb4e5a)
	ROM_LOAD( "hf.h2",    0x1400, 0x0400, 0x358227c6)
	ROM_LOAD( "hf.i1",    0x1800, 0x0400, 0x0c679f44)
	ROM_LOAD( "hf.i2",    0x1c00, 0x0400, 0xd8b7a398)
	ROM_LOAD( "hf.j1",    0x2000, 0x0400, 0x98ef24db)
	ROM_LOAD( "hf.j2",    0x2400, 0x0400, 0x5e2b5877)

	ROM_REGION( 0x0400, REGION_CPU2 )
	ROM_LOAD( "hf.snd",   0x0000, 0x0400, 0x9d77a31f)
ROM_END

ROM_START( helifira )
	ROM_REGION( 0x8000, REGION_CPU1 )
	ROM_LOAD( "hf-a.f1",  0x0000, 0x0400, 0x92c9d6c1)
	ROM_LOAD( "hf-a.f2",  0x0400, 0x0400, 0xa264dde8)
	ROM_LOAD( "hf.g1",    0x0800, 0x0400, 0xb5ad6e8a)
	ROM_LOAD( "hf-a.g2",  0x0c00, 0x0400, 0xa987ebcd)
	ROM_LOAD( "hf-a.h1",  0x1000, 0x0400, 0x25abcaf0)
	ROM_LOAD( "hf.h2",    0x1400, 0x0400, 0x358227c6)
	ROM_LOAD( "hf.i1",    0x1800, 0x0400, 0x0c679f44)
	ROM_LOAD( "hf-a.i2",  0x1c00, 0x0400, 0x296610fd)
	ROM_LOAD( "hf.j1",    0x2000, 0x0400, 0x98ef24db)
	ROM_LOAD( "hf.j2",    0x2400, 0x0400, 0x5e2b5877)

	ROM_REGION( 0x0400, REGION_CPU2 )
	ROM_LOAD( "hf.snd",   0x0000, 0x0400, 0x9d77a31f)
ROM_END

//   (YEAR, NAME,     PARENT,   MACHINE,  INPUT,  INIT,MONITOR,COMPANY,    FULLNAME) 
GAME (1979, spacefev, 0,        spacefev, spacefev, 0, ROT270, "Nintendo", "Space Fever (set 1)" )
GAME (1979, spacefva, spacefev, spacefev, spacefev, 0, ROT270, "Nintendo", "Space Fever (set 2)" )
GAME (1979, sheriff,  0,        sheriff,  sheriff,  0, ROT270, "Nintendo", "Sheriff" )
GAME (1980, bandido,  sheriff,  sheriff,  bandido,  0, ROT270, "Exidy",    "Bandido" )
GAMEX(1980, helifire, 0,        helifire, helifire, 0, ROT270, "Nintendo", "HeliFire (set 1)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
GAMEX(1980, helifira, helifire, helifire, helifire, 0, ROT270, "Nintendo", "HeliFire (set 2)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )

#ifdef __cplusplus
}
#endif
