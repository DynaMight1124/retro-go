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
#include "../vidhrdw/canyon.cpp"
/***************************************************************************

Atari Canyon Bomber Driver

Memory Map:
        0000-01FF       WRAM
        0400-04FF       W A0=0:MOTOR1, A0=1:MOTOR2
        0500-05FF       W A0=0:EXPLODE, A0=1:TIMER RESET
        0600-067F       W A0=0:WHISTLE1, A0=1:WHISTLE2
        0680-06FF       W A0=0:LED1, A0=1:LED2
        0700-077F       W A0=0:ATTRACT1, A0=1:ATTRACT2
        0800-0FFF       DISPLAY / RAM
        1000-17FF       SWITCHES
        1800-1FFF       OPTIONS
        2000-27FF       ROM1
        2800-2FFF       ROM2
        3000-37FF       ROM3
        3800-3FFF       ROM4 (Program ROM)
       (F800-FFFF)      ROM4 (Program ROM) - only needed for the 6502 vectors

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

//struct discrete_sound_block *canyon_sound_interface;

/* vidhrdw/canyon.c */
void canyon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static READ_HANDLER( canyon_options_r )
{
	switch (offset & 0x03)
	{
		case 0x00:
			return ((input_port_0_r(0) >> 6) & 0x03);
		case 0x01:
			return ((input_port_0_r(0) >> 4) & 0x03);
		case 0x02:
			return ((input_port_0_r(0) >> 2) & 0x03);
		case 0x03:
			return ((input_port_0_r(0) >> 0) & 0x03);
	}

	return 0xFF;
}

static READ_HANDLER( canyon_switches_r )
{
	switch (offset & 0x07)
	{
		case 0x00:
			return ((input_port_3_r(0) << 7) & 0x80);
		case 0x01:
			return ((input_port_3_r(0) << 6) & 0x80);
		case 0x02:
			return ((input_port_3_r(0) << 5) & 0x80) | input_port_1_r(0);
		case 0x03:
			return ((input_port_3_r(0) << 4) & 0x80) | input_port_2_r(0);
		case 0x04:
			return ((input_port_3_r(0) << 3) & 0x80);
		case 0x05:
			return ((input_port_3_r(0) << 2) & 0x80);
		case 0x06:
			return ((input_port_3_r(0) << 1) & 0x80) | input_port_1_r(0);
		case 0x07:
			return ((input_port_3_r(0) << 0) & 0x80) | input_port_2_r(0);
	}

	return 0xFF;
}


/************************************************************************/
/* canyon Sound System Analog emulation                                 */
/************************************************************************/

const struct discrete_555_astbl_desc canyonWhistl555 =
{
	DISC_555_OUT_CAP | DISC_555_OUT_AC,
	5,		// B+ voltage of 555
	5.0 - 1.7,	// High output voltage of 555 (Usually v555 - 1.7)
	5.0 * 2.0 /3.0,	// normally 2/3 of v555
	5.0 / 3.0	// normally 1/3 of v555
};

const struct discrete_lfsr_desc canyon_lfsr={
	16,                 /* Bit Length */
	0,                  /* Reset Value */
	6,                  /* Use Bit 6 as XOR input 0 */
	8,                  /* Use Bit 8 as XOR input 1 */
	DISC_LFSR_XNOR,     /* Feedback stage1 is XNOR */
	DISC_LFSR_OR,       /* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,  /* Feedback stage3 replaces the shifted register contents */
	0x000001,           /* Everything is shifted into the first bit only */
	0,                  /* Output is not inverted, Active Low Reset */
	15                  /* Output bit */
};

/* Nodes - Inputs */
#define CANYON_MOTORSND1_DATA		NODE_01
#define CANYON_MOTORSND2_DATA		NODE_02
#define CANYON_EXPLODESND_DATA		NODE_03
#define CANYON_WHISTLESND1_EN		NODE_04
#define CANYON_WHISTLESND2_EN		NODE_05
#define CANYON_ATTRACT1_EN		NODE_06
#define CANYON_ATTRACT2_EN		NODE_07
/* Nodes - Sounds */
#define CANYON_MOTORSND1		NODE_10
#define CANYON_MOTORSND2		NODE_11
#define CANYON_EXPLODESND		NODE_12
#define CANYON_WHISTLESND1		NODE_13
#define CANYON_WHISTLESND2		NODE_14
#define CANYON_NOISE			NODE_15
#define CANYON_FINAL_MIX1		NODE_16
#define CANYON_FINAL_MIX2		NODE_17

static DISCRETE_SOUND_START(canyon_sound_interface)
	/************************************************/
	/* Canyon sound system: 5 Sound Sources         */
	/*                     Relative Volume          */
	/*    1/2) Motor           14.29%               */
	/*      3) Explode        100.00%               */
	/*    4/5) Whistle         51.94%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit taking voltages  */
	/* into account                                 */
	/*                                              */
	/* Motor   3.8V * 5/(5+100) = 0.1810            */
	/* Explode 3.8V * 5/(5+10)  = 1.2667            */
	/* Whistle 5.0V * 5/(5+33)  = 0.6579            */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Motorsound frequency 1                */
	/*  $01 - Motorsound frequency 2                */
	/*  $02 - Whistle enable 1                      */
	/*  $03 - Whistle enable 2                      */
	/*  $04 - Attract enable 1                      */
	/*  $05 - Attract enable 2                      */
	/*  $06 - Explode volume                        */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for canyon            */
	/************************************************/
	/*                   NODE                  ADDR  MASK    GAIN    OFFSET  INIT */
	DISCRETE_INPUTX(CANYON_MOTORSND1_DATA    , 0x00, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUTX(CANYON_MOTORSND2_DATA    , 0x01, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUT (CANYON_WHISTLESND1_EN    , 0x02, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_WHISTLESND2_EN    , 0x03, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_ATTRACT1_EN       , 0x04, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_ATTRACT2_EN       , 0x05, 0x000f,                  0.0)
	DISCRETE_INPUTX(CANYON_EXPLODESND_DATA   , 0x06, 0x000f, 1000.0/15.0, 0,  0.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 1uf capacitor on the R-ladder.          */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, CANYON_MOTORSND1_DATA, 123000, 1e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 3)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (142.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(CANYON_MOTORSND1, CANYON_ATTRACT1_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* The motor2 sound is basically the same as    */
	/* for 1.  But I shifted the frequencies up for */
	/* it to sound different from motor 1.          */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, CANYON_MOTORSND2_DATA, 123000, 1e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 4)
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (142.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(CANYON_MOTORSND2, CANYON_ATTRACT2_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Explode circuit is built around a noise      */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LOGIC_OR(NODE_60, 1, CANYON_ATTRACT1_EN, CANYON_ATTRACT2_EN)
	DISCRETE_LFSR_NOISE(CANYON_NOISE, NODE_60, NODE_60, 15750.0/4, 1.0, 0, 0, &canyon_lfsr)

	DISCRETE_MULTIPLY(NODE_61, 1, CANYON_NOISE, CANYON_EXPLODESND_DATA)
	DISCRETE_RCFILTER(CANYON_EXPLODESND, 1, NODE_61, 545, 5e-6)

	/************************************************/
	/* Whistle circuit is a 555 capacitor charge    */
	/* waveform.  The original game pot varies from */
	/* 0-100k, but we are going to limit it because */
	/* below 50k the frequency is too high.         */
	/* When triggered it starts at it's highest     */
	/* frequency, then decays at the rate set by    */
	/* a 68k resistor and 22uf capacitor.           */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_70, 1, 50000, 100000, DISC_LINADJ, 5)	/* R59 */
	DISCRETE_MULTADD(NODE_71, 1, CANYON_WHISTLESND1_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_72, CANYON_WHISTLESND1_EN, NODE_71, 1.0, NODE_71, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_555_ASTABLE(NODE_73, CANYON_WHISTLESND1_EN, 33000, NODE_70, 1e-8, NODE_72, &canyonWhistl555)
	DISCRETE_MULTIPLY(CANYON_WHISTLESND1, CANYON_WHISTLESND1_EN, NODE_73, 519.4/3.3)

	DISCRETE_ADJUSTMENT(NODE_75, 1, 50000, 100000, DISC_LINADJ, 6)	/* R69 */
	DISCRETE_MULTADD(NODE_76, 1, CANYON_WHISTLESND2_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_77, CANYON_WHISTLESND2_EN, NODE_76, 1.0, NODE_76, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_555_ASTABLE(NODE_78, CANYON_WHISTLESND2_EN, 33000, NODE_75, 1e-8, NODE_77, &canyonWhistl555)
	DISCRETE_MULTIPLY(CANYON_WHISTLESND2, CANYON_WHISTLESND2_EN, NODE_78, 519.4/3.3)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, CANYON_MOTORSND1, CANYON_EXPLODESND, CANYON_WHISTLESND1)
	DISCRETE_ADDER3(NODE_91, 1, CANYON_MOTORSND2, CANYON_EXPLODESND, CANYON_WHISTLESND2)
	DISCRETE_GAIN(CANYON_FINAL_MIX1, NODE_90, 77)
	DISCRETE_GAIN(CANYON_FINAL_MIX2, NODE_91, 77)

	DISCRETE_OUTPUT(CANYON_FINAL_MIX1, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(CANYON_FINAL_MIX2, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END

/*************************************
 *
 *      Output ports
 *
 *************************************/

WRITE_HANDLER( canyon_motor_w )
{
        discrete_sound_w(offset & 0x01, data & 0x0f);
}

WRITE_HANDLER( canyon_explode_w )
{
        discrete_sound_w(6, data >> 4);
}

WRITE_HANDLER( canyon_attract_w )
{
        discrete_sound_w(4 + (offset & 0x01), !(offset & 0x02));
}

WRITE_HANDLER( canyon_whistle_w )
{
        discrete_sound_w(2 + (offset & 0x01), (offset & 0x02) >> 1);
}

static const struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM }, /* WRAM */
	{ 0x0800, 0x0bff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x1000, 0x17ff, canyon_switches_r }, /* SWITCHES */
	{ 0x1800, 0x1fff, canyon_options_r }, /* OPTIONS */
	{ 0x2000, 0x27ff, MRA_NOP }, /* PROM1 */
	{ 0x2800, 0x2fff, MRA_NOP }, /* PROM2 */
	{ 0x3000, 0x37ff, MRA_NOP }, /* PROM3 */
	{ 0x3800, 0x3fff, MRA_ROM }, /* PROM4 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM4 for 6502 vectors */
	{ -1 }	/* end of table */
};

static const struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM }, /* WRAM */
        { 0x0400, 0x0401, canyon_motor_w },
        { 0x0500, 0x0500, canyon_explode_w },
        { 0x0600, 0x0603, canyon_whistle_w },
//      { 0x0680, 0x0683, canyon_led_w },
        { 0x0700, 0x0703, canyon_attract_w },
	{ 0x0bd0, 0x0bdf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x2000, 0x27ff, MWA_NOP }, /* PROM1 */
	{ 0x2800, 0x2fff, MWA_NOP }, /* PROM2 */
	{ 0x3000, 0x37ff, MWA_NOP }, /* PROM3 */
	{ 0x3800, 0x3fff, MWA_ROM }, /* PROM4 */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( canyon )
	PORT_START      /* DSW - fake port, gets mapped to Canyon Bomber ports */
	PORT_DIPNAME( 0x03, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Spanish" )
	PORT_DIPSETTING(    0x02, "French" )
	PORT_DIPSETTING(    0x03, "German" )
	PORT_DIPNAME( 0x30, 0x00, "Misses Per Play" )
	PORT_DIPSETTING(    0x00, "Three" )
	PORT_DIPSETTING(    0x10, "Four" )
	PORT_DIPSETTING(    0x20, "Five" )
	PORT_DIPSETTING(    0x30, "Six" )
	PORT_DIPNAME( 0xC0, 0x80, "Game Cost" )
	PORT_DIPSETTING(    0x80, "1 coin/player" )
	PORT_DIPSETTING(    0xC0, "2 coins/player" )
	PORT_DIPSETTING(    0x40, "2 players/coin" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START      /* IN1 - fake port, gets mapped */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT(0xFE, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 - fake port, gets mapped */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(0xFE, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 - fake port, gets mapped */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_TILT ) /* SLAM */

INPUT_PORTS_END



static const struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
    64,     /* 64 characters */
    1,      /* 1 bit per pixel */
    { 0 },  /* no separation in 1 bpp */
    { 4, 5, 6, 7, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static const struct GfxLayout motionlayout =
{
	32,16,   /* 32*16 characters */
	4,       /* 4 characters? */
	1,       /* 1 bit per pixel */
	{ 0 },   /* no separation in 1 bpp */
	{ 0x100*8 + 7, 0x100*8 + 6, 0x100*8 + 5, 0x100*8 + 4, 7, 6, 5, 4,
	  0x100*8 + 15, 0x100*8 + 14, 0x100*8 + 13, 0x100*8 + 12, 15, 14, 13, 12,
	  0x100*8 + 256+7, 0x100*8 + 256+6, 0x100*8 + 256+5, 0x100*8 + 256+4, 256+7, 256+6, 256+5, 256+4,
	  0x100*8 + 256+15, 0x100*8 + 256+14, 0x100*8 + 256+13, 0x100*8 + 256+12, 256+15, 256+14, 256+13, 256+12 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8     /* every char takes 64 consecutive bytes */
};

static const struct GfxLayout bomb_layout =
{
	2,2,	/* 2*2 bomb */
    1,      /* 1 character */
    1,      /* 1 bit per pixel */
    { 0 },  /* no separation in 1 bpp */
    { 4, 4 }, /* I know that this bit is 1 */
	{ 3*16, 3*16 },  /* I know that this bit is 1 */
	16*8	/* every char takes 16 consecutive bytes */
};

static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 2 },
	{ REGION_GFX2, 0, &motionlayout, 0, 2 },
	{ REGION_GFX1, 0, &bomb_layout,  0, 2 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0x80,0x80,0x80, /* LT GREY */
	0xff,0xff,0xff, /* WHITE */
};
static unsigned short colortable[] =
{
	0x01, 0x00,
	0x01, 0x02,
};
static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,palette,sizeof(palette));
	memcpy(game_colortable,colortable,sizeof(colortable));
}


static const struct MachineDriver machine_driver_canyon =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			750000,        /* 0.75 Mhz */
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 30*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3, sizeof(colortable) / sizeof(colortable[0]),
	init_palette,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	canyon_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
        {
                {
                        SOUND_DISCRETE,
                        &canyon_sound_interface
                }
        }


};


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( canyon )
	ROM_REGION( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "9496-01.d1", 0x3800, 0x0800, 0x8be15080 )
	ROM_RELOAD(             0xF800, 0x0800 )

	ROM_REGION( 0x400, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, 0x7449f754 )

	ROM_REGION( 0x200, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9505-01.n5", 0x0000, 0x0100, 0x60507c07 )
	ROM_LOAD( "9506-01.m5", 0x0100, 0x0100, 0x0d63396a )
ROM_END


ROM_START( canbprot )
	ROM_REGION( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD_NIB_LOW ( "cbp3000l.j1", 0x3000, 0x0800, 0x49cf29a0 )
	ROM_LOAD_NIB_HIGH( "cbp3000m.p1", 0x3000, 0x0800, 0xb4385c23 )
	ROM_LOAD_NIB_LOW ( "cbp3800l.h1", 0x3800, 0x0800, 0xc7ee4431 )
	ROM_RELOAD_NIB_LOW (              0xf800, 0x0800 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "cbp3800m.r1", 0x3800, 0x0800, 0x94246a9a )
	ROM_RELOAD_NIB_HIGH (             0xf800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x400, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, 0x7449f754 )

	ROM_REGION( 0x200, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9505-01.n5", 0x0000, 0x0100, 0x60507c07 )
	ROM_LOAD( "9506-01.m5", 0x0100, 0x0100, 0x0d63396a )
ROM_END



GAME ( 1977, canyon,   0,      canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber" )
GAME ( 1977, canbprot, canyon, canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber (prototype)" )






#ifdef __cplusplus
}
#endif
