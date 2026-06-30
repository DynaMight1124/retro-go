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
#include "../vidhrdw/toki.cpp"
#include "../sndhrdw/toki.cpp"

/***************************************************************************

Toki

driver by Jarek Parchanski


Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.


TODO
----

Does the bootleg use a 68000 @ 10MHz ? This causes some bad slow-
downs at the floating monkey machine (round 1), so set to 12 MHz
for now. Even at 12 this slowdown still happens a little.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern unsigned char *toki_background1_videoram;
extern unsigned char *toki_background2_videoram;
extern unsigned char *toki_scrollram;
extern data16_t *toki_background1_videoram16;
extern data16_t *toki_background2_videoram16;
extern data16_t *toki_scrollram16;

int toki_interrupt(void);
int  toki_vh_start(void);
void toki_vh_stop(void);
void toki_eof_callback(void);
void toki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void tokib_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( toki_background1_videoram16_w );
WRITE_HANDLER( toki_background2_videoram16_w );
WRITE_HANDLER( toki_control_w );
WRITE_HANDLER( toki_foreground_videoram16_w );


static WRITE_HANDLER( tokib_soundcommand16_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ_HANDLER( pip16_r )
{
	return ~0;
}


static int msm5205next;

static void toki_adpcm_int (int data)
{
	static int toggle=0;

	MSM5205_data_w (0,msm5205next);
	msm5205next>>=4;

	toggle ^= 1;
	if (toggle)
		cpu_set_nmi_line(1, PULSE_LINE);
}

static WRITE_HANDLER( toki_adpcm_control_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);


	/* the code writes either 2 or 3 in the bottom two bits */
	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	MSM5205_reset_w(0,data & 0x08);
}

static WRITE_HANDLER( toki_adpcm_data_w )
{
	msm5205next = data;
}


/*****************************************************************************/


static const struct MemoryReadAddress toki_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x06d7ff, MRA_BANK2 },
	{ 0x06d800, 0x06dfff, MRA_BANK3 },
	{ 0x06e000, 0x06e7ff, MRA_BANK4 },
	{ 0x06e800, 0x06efff, MRA_BANK5 },
	{ 0x06f000, 0x06f7ff, MRA_BANK6 },
	{ 0x06f800, 0x06ffff, MRA_BANK7 },
	{ 0x080000, 0x08000d, seibu_main_word_r },
	{ 0x0c0000, 0x0c0001, input_port_1_r },
	{ 0x0c0002, 0x0c0003, input_port_2_r },
	{ 0x0c0004, 0x0c0005, input_port_3_r },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress toki_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x06d7ff, MWA_BANK2 },
	{ 0x06d800, 0x06dfff, MWA_BANK3, &spriteram, &spriteram_size },
	{ 0x06e000, 0x06e7ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x06e800, 0x06efff, toki_background1_videoram16_w, &toki_background1_videoram },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram16_w, &toki_background2_videoram },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram16_w, &videoram },
	{ 0x080000, 0x08000d, seibu_main_word_w },
	{ 0x0a0000, 0x0a005f, toki_control_w, &toki_scrollram },
	{ -1 }  /* end of table */
};

/* In the bootleg, sound and sprites are remapped to 0x70000 */
static const struct MemoryReadAddress tokib_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x06dfff, MRA_BANK2 },
	{ 0x06e000, 0x06e7ff, MRA_BANK3 },
	{ 0x06e800, 0x06efff, MRA_BANK4 },
	{ 0x06f000, 0x06f7ff, MRA_BANK5 },
	{ 0x06f800, 0x06ffff, MRA_BANK6 },
	{ 0x072000, 0x072001, watchdog_reset_r },   /* probably */
	{ 0x0c0000, 0x0c0001, input_port_0_r },
	{ 0x0c0002, 0x0c0003, input_port_1_r },
	{ 0x0c0004, 0x0c0005, input_port_2_r },
	{ 0x0c000e, 0x0c000f, pip16_r },  /* sound related, if we return 0 the code writes */
				/* the sound command quickly followed by 0 and the */
				/* sound CPU often misses the command. */
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress tokib_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x06dfff, MWA_BANK2 },
	{ 0x06e000, 0x06e7ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x06e800, 0x06efff, toki_background1_videoram16_w, &toki_background1_videoram },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram16_w, &toki_background2_videoram },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram16_w, &videoram },
	{ 0x071000, 0x071001, MWA_NOP },	/* sprite related? seems another scroll register */
				/* gets written the same value as 75000a (bg2 scrollx) */
	{ 0x071804, 0x071807, MWA_NOP },	/* sprite related, always 01be0100 */
	{ 0x07180e, 0x071e45, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x075000, 0x075001, tokib_soundcommand16_w },
	{ 0x075004, 0x07500b, MWA_RAM, &toki_scrollram },
	{ -1 }  /* end of table */
};

/*****************************************************************************/

static const struct MemoryReadAddress tokib_sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xec00, 0xec00, YM3812_status_port_0_r },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress tokib_sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe000, toki_adpcm_control_w },	/* MSM5205 + ROM bank */
	{ 0xe400, 0xe400, toki_adpcm_data_w },
	{ 0xec00, 0xec00, YM3812_control_port_0_w },
	{ 0xec01, 0xec01, YM3812_write_port_0_w },
	{ 0xec08, 0xec08, YM3812_control_port_0_w },	/* mirror address, it seems */
	{ 0xec09, 0xec09, YM3812_write_port_0_w },	/* mirror address, it seems */
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ -1 }  /* end of table */
};

/*****************************************************************************/

INPUT_PORTS_START( toki )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tokib )
	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*****************************************************************************/

static const struct GfxLayout toki_charlayout =
{
	8,8,
	4096,
	4,
	{ 4096*16*8+0, 4096*16*8+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const struct GfxLayout toki_tilelayout =
{
	16,16,
	4096,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static const struct GfxLayout toki_spritelayout =
{
	16,16,
	8192,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static const struct GfxDecodeInfo toki_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &toki_charlayout,  16*16, 16 },
	{ REGION_GFX2, 0, &toki_spritelayout, 0*16, 16 },
	{ REGION_GFX3, 0, &toki_tilelayout,  32*16, 16 },
	{ REGION_GFX4, 0, &toki_tilelayout,  48*16, 16 },
	{ -1 } /* end of array */
};

static const struct GfxLayout tokib_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{4096*8*8*3,4096*8*8*2,4096*8*8*1,4096*8*8*0 }, /* planes */
	{ 0, 1,  2,  3,  4,  5,  6,  7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},		/* y bit */
	8*8
};

static const struct GfxLayout tokib_tilelayout =
{
	16,16,	/* 16 by 16 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 4096*16*16*3,4096*16*16*2,4096*16*16*1,4096*16*16*0 },	/* planes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0x8000*8+0, 0x8000*8+1, 0x8000*8+2, 0x8000*8+3, 0x8000*8+4,
	  0x8000*8+5, 0x8000*8+6, 0x8000*8+7 }, 			/* x bit */
	{
	  0,8,16,24,32,40,48,56,
	  0x10000*8+ 0, 0x10000*8+ 8, 0x10000*8+16, 0x10000*8+24, 0x10000*8+32,
	  0x10000*8+40, 0x10000*8+48, 0x10000*8+56 },			/* y bit */
	8*8
};

static const struct GfxLayout tokib_spriteslayout =
{
	16,16,	/* 16 by 16 */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 8192*16*16*3,8192*16*16*2,8192*16*16*1,8192*16*16*0 },	/* planes */
	{	 0, 	1,	   2,	  3,	 4, 	5,	   6,	  7,
	 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },	/* x bit */
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120 },		/* y bit */
	16*16
};

static const struct GfxDecodeInfo tokib_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tokib_charlayout,	16*16, 16 },
	{ REGION_GFX2, 0, &tokib_spriteslayout,  0*16, 16 },
	{ REGION_GFX3, 0, &tokib_tilelayout,	32*16, 16 },
	{ REGION_GFX4, 0, &tokib_tilelayout,	48*16, 16 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);


static struct YM3526interface ym3812_tokib_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 60 }		/* (not supported) */
};

static const struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip			  */
	384000, 			/* 384KHz			  */
	{ toki_adpcm_int },/* interrupt function */
	{ MSM5205_S96_4B},	/* 4KHz 			  */
	{ 60 }
};


static const struct MachineDriver machine_driver_toki =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			20000000/2, 	/* Accurate?  There is a 20MHz near the cpu, but a 12MHz elsewhere */
			toki_readmem,toki_writemem,0,0,
			m68_level1_irq,1 /* VBL */
		},
		{
			SEIBU_SOUND_SYSTEM_CPU(14318180/4)
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	seibu_sound_init_1, /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },

	toki_gfxdecodeinfo,
	4*256, 4*256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	toki_eof_callback,
	toki_vh_start,
	0,
	toki_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
	}
};

static const struct MachineDriver machine_driver_tokib =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 10MHz causes bad slowdowns with monkey machine rd1 */
			tokib_readmem,tokib_writemem,0,0,
			m68_level6_irq,1 /* VBL (could be level1, same vector) */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz (?) */
			tokib_sound_readmem,tokib_sound_writemem,
			0,0,
			ignore_interrupt,0	/* IRQs are caused by the main CPU?? */
								/* NMIs are caused by the ADPCM chip */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,
	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	tokib_gfxdecodeinfo,
	4*256, 4*256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	toki_eof_callback,
	toki_vh_start,
	0,
	tokib_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_tokib_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tokij )
	ROM_REGION( 0x60000, REGION_CPU1 )	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "tokijp.006",   0x00000, 0x20000, 0x03d726b1 )
	ROM_LOAD_ODD ( "tokijp.004",       0x00000, 0x20000, 0x54a45e12 )
	ROM_LOAD_EVEN( "tokijp.005",   0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "tokijp.003",   0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION( 0x20000*2, REGION_CPU2 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, 0x6c87c4c5 )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, 0xa67969c4 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, 0x8aa964a2 )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, 0x86e87e48 )

	ROM_REGION( 0x100000, REGION_GFX2  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, 0xa27a80ba )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, 0xfa687718 )

	ROM_REGION( 0x080000, REGION_GFX3  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, 0xfdaa5f4b )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, 0xd86ac664 )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, 0xae7a6b8b )
ROM_END

ROM_START( tokia )
	ROM_REGION( 0x60000, REGION_CPU1 )	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "tokijp.006",   0x00000, 0x20000, 0x03d726b1 )
	ROM_LOAD_ODD ( "4c.10k",       0x00000, 0x20000, 0xb2c345c5 )
	ROM_LOAD_EVEN( "tokijp.005",   0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "tokijp.003",   0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION( 0x20000*2, REGION_CPU2 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, 0x6c87c4c5 )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, 0xa67969c4 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, 0x8aa964a2 )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, 0x86e87e48 )

	ROM_REGION( 0x100000, REGION_GFX2  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, 0xa27a80ba )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, 0xfa687718 )

	ROM_REGION( 0x080000, REGION_GFX3  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, 0xfdaa5f4b )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, 0xd86ac664 )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, 0xae7a6b8b )
ROM_END

ROM_START( toki )
	ROM_REGION( 0x60000, REGION_CPU1 )	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "l10_6.bin",    0x00000, 0x20000, 0x94015d91 )
	ROM_LOAD_ODD ( "k10_4e.bin",   0x00000, 0x20000, 0x531bd3ef )
	ROM_LOAD_EVEN( "tokijp.005",   0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "tokijp.003",   0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION( 0x20000*2, REGION_CPU2 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, 0x6c87c4c5 )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, 0xa67969c4 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, 0x8aa964a2 )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, 0x86e87e48 )

	ROM_REGION( 0x100000, REGION_GFX2  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, 0xa27a80ba )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, 0xfa687718 )

	ROM_REGION( 0x080000, REGION_GFX3  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, 0xfdaa5f4b )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, 0xd86ac664 )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, 0xae7a6b8b )
ROM_END

ROM_START( tokiu )
	ROM_REGION( 0x60000, REGION_CPU1 )	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "6b.10m",       0x00000, 0x20000, 0x3674d9fe )
	ROM_LOAD_ODD ( "14.10k",       0x00000, 0x20000, 0xbfdd48af )
	ROM_LOAD_EVEN( "tokijp.005",   0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "tokijp.003",   0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION( 0x20000*2, REGION_CPU2 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, 0x6c87c4c5 )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, 0xa67969c4 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, 0x8aa964a2 )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, 0x86e87e48 )

	ROM_REGION( 0x100000, REGION_GFX2  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, 0xa27a80ba )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, 0xfa687718 )

	ROM_REGION( 0x080000, REGION_GFX3  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, 0xfdaa5f4b )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, 0xd86ac664 )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, 0xae7a6b8b )
ROM_END

ROM_START( tokib )
	ROM_REGION( 0x60000, REGION_CPU1 )	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "toki.e3",      0x00000, 0x20000, 0xae9b3da4 )
	ROM_LOAD_ODD ( "toki.e5",      0x00000, 0x20000, 0x66a5a1d6 )
	ROM_LOAD_EVEN( "tokijp.005",   0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "tokijp.003",   0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION( 0x18000, REGION_CPU2 )	/* 64k for code + 32k for banked data */
	ROM_LOAD( "toki.e1",      0x00000, 0x8000, 0x2832ef75 )
	ROM_CONTINUE(             0x10000, 0x8000 ) /* banked at 8000-bfff */

	ROM_REGION( 0x020000, REGION_GFX1  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.e21",     0x000000, 0x08000, 0xbb8cacbd )   /* chars */
	ROM_LOAD( "toki.e13",     0x008000, 0x08000, 0x052ad275 )
	ROM_LOAD( "toki.e22",     0x010000, 0x08000, 0x04dcdc21 )
	ROM_LOAD( "toki.e7",      0x018000, 0x08000, 0x70729106 )

	ROM_REGION( 0x100000, REGION_GFX2  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.e26",     0x000000, 0x20000, 0xa8ba71fc )   /* sprites */
	ROM_LOAD( "toki.e28",     0x020000, 0x20000, 0x29784948 )
	ROM_LOAD( "toki.e34",     0x040000, 0x20000, 0xe5f6e19b )
	ROM_LOAD( "toki.e36",     0x060000, 0x20000, 0x96e8db8b )
	ROM_LOAD( "toki.e30",     0x080000, 0x20000, 0x770d2b1b )
	ROM_LOAD( "toki.e32",     0x0a0000, 0x20000, 0xc289d246 )
	ROM_LOAD( "toki.e38",     0x0c0000, 0x20000, 0x87f4e7fb )
	ROM_LOAD( "toki.e40",     0x0e0000, 0x20000, 0x96e87350 )

	ROM_REGION( 0x080000, REGION_GFX3  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.e23",     0x000000, 0x10000, 0xfeb13d35 )   /* tiles 1 */
	ROM_LOAD( "toki.e24",     0x010000, 0x10000, 0x5b365637 )
	ROM_LOAD( "toki.e15",     0x020000, 0x10000, 0x617c32e6 )
	ROM_LOAD( "toki.e16",     0x030000, 0x10000, 0x2a11c0f0 )
	ROM_LOAD( "toki.e17",     0x040000, 0x10000, 0xfbc3d456 )
	ROM_LOAD( "toki.e18",     0x050000, 0x10000, 0x4c2a72e1 )
	ROM_LOAD( "toki.e8",      0x060000, 0x10000, 0x46a1b821 )
	ROM_LOAD( "toki.e9",      0x070000, 0x10000, 0x82ce27f6 )

	ROM_REGION( 0x080000, REGION_GFX4  | REGIONFLAG_DISPOSE )
	ROM_LOAD( "toki.e25",     0x000000, 0x10000, 0x63026cad )   /* tiles 2 */
	ROM_LOAD( "toki.e20",     0x010000, 0x10000, 0xa7f2ce26 )
	ROM_LOAD( "toki.e11",     0x020000, 0x10000, 0x48989aa0 )
	ROM_LOAD( "toki.e12",     0x030000, 0x10000, 0xc2ad9342 )
	ROM_LOAD( "toki.e19",     0x040000, 0x10000, 0x6cd22b18 )
	ROM_LOAD( "toki.e14",     0x050000, 0x10000, 0x859e313a )
	ROM_LOAD( "toki.e10",     0x060000, 0x10000, 0xe15c1d0f )
	ROM_LOAD( "toki.e6",      0x070000, 0x10000, 0x6f4b878a )
ROM_END


static void init_toki(void)
{
	seibu_sound_decrypt(REGION_CPU2,0x2000);
}


void init_tokib(void)
{
	unsigned char *temp = malloc (65536 * 2);
	int i, offs;

	/* invert the sprite data in the ROMs */
	for (i = 0; i < memory_region_length(REGION_GFX2); i++)
		memory_region(REGION_GFX2)[i] ^= 0xff;

	/* merge background tile graphics together */
	if (temp)
	{
		for (offs = 0; offs < memory_region_length(REGION_GFX3); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX3)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}
		for (offs = 0; offs < memory_region_length(REGION_GFX4); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX4)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}

		free (temp);
	}
}



GAME( 1989, toki,  0,    toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 1)" )
GAME( 1989, tokia, toki, toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 2)" )
GAME( 1989, tokij, toki, toki,  toki,  toki,  ROT0, "Tad", "JuJu Densetsu (Japan)" )
GAME( 1989, tokiu, toki, toki,  toki,  toki,  ROT0, "Tad (Fabtek license)", "Toki (US)" )
GAME( 1989, tokib, toki, tokib, tokib, tokib, ROT0, "bootleg", "Toki (bootleg)" )

#ifdef __cplusplus
}
#endif
