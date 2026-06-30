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
#include "../vidhrdw/bsktball.cpp"
#include "../machine/bsktball.cpp"

/***************************************************************************

Atari Basketball Driver

Note:  The original hardware uses the Player 1 and Player 2 Start buttons
as the Jump/Shoot buttons.	I've taken button 1 and mapped it to the Start
buttons to keep people from getting confused.

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* machine/bsktball.c */
WRITE_HANDLER( bsktball_nmion_w );
extern int bsktball_interrupt(void);
WRITE_HANDLER( bsktball_ld1_w );
WRITE_HANDLER( bsktball_ld2_w );
READ_HANDLER( bsktball_in0_r );
WRITE_HANDLER( bsktball_led1_w );
WRITE_HANDLER( bsktball_led2_w );

/* vidhrdw/bsktball.c */
extern unsigned char *bsktball_motion;
extern void bsktball_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static const struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM }, /* Zero Page RAM */
	{ 0x0800, 0x0800, bsktball_in0_r },
	{ 0x0802, 0x0802, input_port_5_r },
	{ 0x0803, 0x0803, input_port_6_r },
	{ 0x1800, 0x1cff, MRA_RAM }, /* video ram */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROGRAM */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
	{ -1 }	/* end of table */
};

static const struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM }, /* WRAM */
	{ 0x1000, 0x1000, MWA_RAM }, /* Timer Reset */
	{ 0x1010, 0x101f, bsktball_bounce_w }, /* Crowd Amp / Bounce */
	{ 0x1022, 0x1023, MWA_RAM }, /* Coin Counter */
	{ 0x1024, 0x1025, bsktball_led1_w }, /* LED 1 */
	{ 0x1026, 0x1027, bsktball_led2_w }, /* LED 2 */
	{ 0x1028, 0x1029, bsktball_ld1_w }, /* LD 1 */
	{ 0x102a, 0x102b, bsktball_ld2_w }, /* LD 2 */
	{ 0x102c, 0x102d, bsktball_noise_reset_w }, /* Noise Reset */
	{ 0x102e, 0x102f, bsktball_nmion_w }, /* NMI On */
	{ 0x1030, 0x103f, bsktball_note_w }, /* Music Ckt Note Dvsr */
	{ 0x1800, 0x1bbf, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x1bc0, 0x1bff, MWA_RAM, &bsktball_motion },
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( bsktball )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_X, 100, 10, 0, 0 ) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_Y, 100, 10, 0, 0 )

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 10, 0, 0 ) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 10, 0, 0 )

	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* SPARE */
	/* 0x10 - DR0 = PL2 H DIR */
	/* 0x20 - DR1 = PL2 V DIR */
	/* 0x40 - DR2 = PL1 H DIR */
	/* 0x80 - DR3 = PL1 V DIR */

	PORT_START		/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* TEST STEP */
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* COIN 0 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) /* COIN 1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) /* COIN 2 */

	PORT_START		/* DSW */
	PORT_DIPNAME( 0x07, 0x00, "Coin Mode" )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x06, "2:30/Credit" )
	PORT_DIPSETTING(	0x05, "2:00/Credit" )
	PORT_DIPSETTING(	0x04, "1:30/Credit" )
	PORT_DIPSETTING(	0x03, "1:15/Credit" )
	PORT_DIPSETTING(	0x02, "0:45/Credit" )
	PORT_DIPSETTING(	0x01, "0:30/Credit" )
	PORT_DIPSETTING(	0x00, "1:00/Credit" )
	PORT_DIPNAME( 0x18, 0x00, "Dollar Coin Mode" )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x20, 0x00, "Cost" )
	PORT_DIPSETTING(	0x20, "Two Coin Minimum" )
	PORT_DIPSETTING(	0x00, "One Coin Minimum" )
	PORT_DIPNAME( 0xC0, 0x00, "Language" )
	PORT_DIPSETTING(	0xC0, "German" )
	PORT_DIPSETTING(	0x80, "French" )
	PORT_DIPSETTING(	0x40, "Spanish" )
	PORT_DIPSETTING(	0x00, "English" )
INPUT_PORTS_END

static const struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	64, 	/* 64 characters */
	2,		/* 2 bits per pixel */
	{ 0, 8*0x800 }, 	   /* bitplanes separated by $800 bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static const struct GfxLayout motionlayout =
{
	8,32,	/* 8*32 characters */
	64, 	/* 64 characters */
	2,		/* 2 bits per pixel */
	{ 0, 8*0x800 }, 	   /* bitplanes separated by $800 bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{	0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
		24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8 },
	32*8	/* every char takes 32 consecutive bytes */
};

static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0600, &charlayout,   0x00, 0x02 },
	{ REGION_GFX1, 0x0000, &motionlayout, 0x08, 0x40 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0x80,0x80,0x80, /* LIGHT GREY */
	0x50,0x50,0x50, /* DARK GREY */
	0xff,0xff,0xff, /* WHITE */
};
static unsigned short colortable[] =
{
	/* Playfield */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x03, 0x03, 0x03,

	/* Motion */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00,
	0x01, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x03, 0x00,

	0x01, 0x01, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00,
	0x01, 0x01, 0x02, 0x00,
	0x01, 0x01, 0x03, 0x00,

	0x01, 0x02, 0x00, 0x00,
	0x01, 0x02, 0x01, 0x00,
	0x01, 0x02, 0x02, 0x00,
	0x01, 0x02, 0x03, 0x00,

	0x01, 0x03, 0x00, 0x00,
	0x01, 0x03, 0x01, 0x00,
	0x01, 0x03, 0x02, 0x00,
	0x01, 0x03, 0x03, 0x00,

	0x01, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x01, 0x01,
	0x01, 0x00, 0x02, 0x01,
	0x01, 0x00, 0x03, 0x01,

	0x01, 0x01, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x02, 0x01,
	0x01, 0x01, 0x03, 0x01,

	0x01, 0x02, 0x00, 0x01,
	0x01, 0x02, 0x01, 0x01,
	0x01, 0x02, 0x02, 0x01,
	0x01, 0x02, 0x03, 0x01,

	0x01, 0x03, 0x00, 0x01,
	0x01, 0x03, 0x01, 0x01,
	0x01, 0x03, 0x02, 0x01,
	0x01, 0x03, 0x03, 0x01,

	0x01, 0x00, 0x00, 0x02,
	0x01, 0x00, 0x01, 0x02,
	0x01, 0x00, 0x02, 0x02,
	0x01, 0x00, 0x03, 0x02,

	0x01, 0x01, 0x00, 0x02,
	0x01, 0x01, 0x01, 0x02,
	0x01, 0x01, 0x02, 0x02,
	0x01, 0x01, 0x03, 0x02,

	0x01, 0x02, 0x00, 0x02,
	0x01, 0x02, 0x01, 0x02,
	0x01, 0x02, 0x02, 0x02,
	0x01, 0x02, 0x03, 0x02,

	0x01, 0x03, 0x00, 0x02,
	0x01, 0x03, 0x01, 0x02,
	0x01, 0x03, 0x02, 0x02,
	0x01, 0x03, 0x03, 0x02,

	0x01, 0x00, 0x00, 0x03,
	0x01, 0x00, 0x01, 0x03,
	0x01, 0x00, 0x02, 0x03,
	0x01, 0x00, 0x03, 0x03,

	0x01, 0x01, 0x00, 0x03,
	0x01, 0x01, 0x01, 0x03,
	0x01, 0x01, 0x02, 0x03,
	0x01, 0x01, 0x03, 0x03,

	0x01, 0x02, 0x00, 0x03,
	0x01, 0x02, 0x01, 0x03,
	0x01, 0x02, 0x02, 0x03,
	0x01, 0x02, 0x03, 0x03,

	0x01, 0x03, 0x00, 0x03,
	0x01, 0x03, 0x01, 0x03,
	0x01, 0x03, 0x02, 0x03,
	0x01, 0x03, 0x03, 0x03,

};
static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,palette,sizeof(palette));
	memcpy(game_colortable,colortable,sizeof(colortable));
}

/************************************************************************/
/* bsktball Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc bsktball_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	15			/* Output bit */
};

/* Nodes - Inputs */
#define BSKTBALL_NOTE_DATA		NODE_01
#define BSKTBALL_CROWD_DATA		NODE_02
#define BSKTBALL_NOISE_EN		NODE_03
#define BSKTBALL_BOUNCE_EN		NODE_04
/* Nodes - Sounds */
#define BSKTBALL_NOISE			NODE_10
#define BSKTBALL_BOUNCE_SND		NODE_11
#define BSKTBALL_NOTE_SND		NODE_12
#define BSKTBALL_CROWD_SND		NODE_13

static DISCRETE_SOUND_START(bsktball_sound_interface)
	/************************************************/
	/* bsktball  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Note          3.8      47/47        1000.0   */
	/* Bounce        3.8      47/47        1000.0   */
	/* Crowd         3.8      47/220        213.6   */
	/************************************************/

	/************************************************/
	/* Input register mapping for bsktball          */
	/************************************************/
	/*              NODE                 ADDR  MASK    GAIN     OFFSET  INIT */
	DISCRETE_INPUT (BSKTBALL_NOTE_DATA,  0x00, 0x000f,                  0.0)
	DISCRETE_INPUTX(BSKTBALL_CROWD_DATA, 0x01, 0x000f, 213.6/15, 0,     0.0)
	DISCRETE_INPUTX(BSKTBALL_BOUNCE_EN,  0x02, 0x000f, 1000.0/2, 0,     0.0)
	DISCRETE_INPUT (BSKTBALL_NOISE_EN,   0x03, 0x000f,                  0.0)

	/************************************************/
	/* Bounce is a trigger fed directly to the amp  */
	/************************************************/
	DISCRETE_CRFILTER(BSKTBALL_BOUNCE_SND, 1, BSKTBALL_BOUNCE_EN, 100000, 1.e-8)	// remove DC (C54)

	/************************************************/
	/* Crowd effect is variable amplitude, filtered */
	/* random noise.                                */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(BSKTBALL_NOISE, BSKTBALL_NOISE_EN, BSKTBALL_NOISE_EN, 15750.0, BSKTBALL_CROWD_DATA, 0, 0, &bsktball_lfsr)
	DISCRETE_FILTER2(BSKTBALL_CROWD_SND, 1, BSKTBALL_NOISE, 330.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Note sound is created by a divider circuit.  */
	/* The master clock is the 32H signal, which is */
	/* 12.096MHz/128.  This is then sent to a       */
	/* preloadable 8 bit counter, which loads the   */
	/* value from OUT30 when overflowing from 0xFF  */
	/* to 0x00.  Therefore it divides by 2 (OUT30   */
	/* = FE) to 256 (OUT30 = 00).                   */
	/* There is also a final /2 stage.              */
	/* Note that there is no music disable line.    */
	/* When there is no music, the game sets the    */
	/* oscillator to 0Hz.  (OUT30 = FF)             */
	/************************************************/
	DISCRETE_ADDER2(NODE_20, 1, BSKTBALL_NOTE_DATA, 1)	/* To get values of 1 - 256 */
	DISCRETE_DIVIDE(NODE_21, 1, 12096000.0/128/2, NODE_20)
	DISCRETE_SQUAREWAVE(BSKTBALL_NOTE_SND, BSKTBALL_NOTE_DATA, NODE_21, 1000, 50.0, 0, 0.0)	/* NOTE=FF Disables audio */

	DISCRETE_ADDER3(NODE_90, 1, BSKTBALL_BOUNCE_SND, BSKTBALL_NOTE_SND, BSKTBALL_CROWD_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+2000.0+213.6))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END


static const struct MachineDriver machine_driver_bsktball =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			750000, 	   /* 750 KHz */
			readmem,writemem,0,0,
			bsktball_interrupt,8
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3, sizeof(colortable) / sizeof(colortable[0]),
	init_palette,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	bsktball_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DISCRETE,
			&bsktball_sound_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bsktball )
	ROM_REGION( 0x10000, REGION_CPU1 ) /* 64k for code */
	ROM_LOAD( "034765.d1",    0x2000, 0x0800, 0x798cea39 )
	ROM_LOAD( "034764.c1",    0x2800, 0x0800, 0xa087109e )
	ROM_LOAD( "034766.f1",    0x3000, 0x0800, 0xa82e9a9f )
	ROM_LOAD( "034763.b1",    0x3800, 0x0800, 0x1fc69359 )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x1000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "034757.a6",    0x0000, 0x0800, 0x010e8ad3 )
	ROM_LOAD( "034758.b6",    0x0800, 0x0800, 0xf7bea344 )
ROM_END



GAME( 1979, bsktball, 0, bsktball, bsktball, 0, ROT0, "Atari", "Basketball" )

#ifdef __cplusplus
}
#endif
