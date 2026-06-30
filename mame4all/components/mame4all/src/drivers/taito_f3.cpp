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
#include "../vidhrdw/taito_f3.cpp"
#include "../sndhrdw/taito_f3.cpp"

/***************************************************************************

	Taito F3 Package System (aka F3 Cybercore System)

	Emulation by Bryan McPhail, mish@tendril.co.uk/mish@mame.net
	Thanks to Ian Schmidt and Stileto for sound information!
	Major thanks to Aaron Giles for sound info, figuring out the 68K/ES5505
	rom interface and ES5505 emulator!

	Main Issues:
		Alpha blending not supported (sprite & playfield).
		Some early games could fit in 8 bit colour - but marking pens is difficult
			because graphics are 6bpp with 4bpp palette indexes.
		Sound eats lots of memory as 8 bit PCM data is decoded as 16 bit for
			use by the current ES5505 core (which rightly should be 16 bit).
		Only 270 degree rotation is supported in the custom renderer (so you must use
			flipscreen on Gunlock & Gseeker which are really 90 degree rotation games).
		Zoomed layers are not always positioned quite correctly in flipscreen mode

	Other Issues:
		Dsp isn't hooked up.
		Crowd/boards not shown in the football games
		Sound doesn't work in RidingF/RingRage/QTheater?
		Input bit to switch between analogue/digital control panels for Arkanoid/
			Puchi Carat is not found.

	Feel free to report any other issues to me.

	Taito custom chips on motherboard:

		TC0630 FDP - Playfield generator?  (Nearest tile roms)
		TC0640 FI0 - I/O & watchdog?
		TC0650 FDA - Priority mixer?  (Near paletteram & video output)
		TC0660 FCM - Sprites? (Nearest sprite roms)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
//ALEK #include "drivers/taito_f3.h"
#include "state.h"

#define TRY_ALPHA 0

int  f3_vh_start(void);
void f3_vh_stop(void);
void f3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void f3_eof_callback(void);

extern data32_t *f3_vram,*f3_line_ram;
extern data32_t *f3_pf_data,*f3_pivot_ram;
static data32_t coin_word[2], *f3_ram;
data32_t *f3_shared_ram;
int f3_game;

WRITE_HANDLER( f3_control_0_w );
WRITE_HANDLER( f3_control_1_w );
WRITE_HANDLER( f3_palette_24bit_w );
WRITE_HANDLER( f3_pf_data_w );
WRITE_HANDLER( f3_vram_w );
WRITE_HANDLER( f3_pivot_w );
WRITE_HANDLER( f3_videoram_w );

/* from Machine.c */
READ_HANDLER(f3_68000_share_r);
WRITE_HANDLER(f3_68000_share_w);
READ_HANDLER(f3_68681_r);
WRITE_HANDLER(f3_68681_w);
READ_HANDLER(es5510_dsp_r);
WRITE_HANDLER(es5510_dsp_w);
WRITE_HANDLER(f3_volume_w);
WRITE_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);

/******************************************************************************/

static READ_HANDLER( f3_control_r )
{
	int e;

	switch (offset)
	{
		case 0x0: /* MSW: Test switch, coins, eeprom access, LSW: Player Buttons, Start, Tilt, Service */
			e=EEPROM_read_bit();
			e=e|(e<<8);
			return ((e | readinputport(2) | (readinputport(2)<<8))<<16) /* top byte may be mirror of bottom byte??  see bubblem */
					| readinputport(1);

		case 0x1: /* MSW: Coin counters/lockouts are readable, LSW: Joysticks (Player 1 & 2) */
			return (coin_word[0]<<16) | readinputport(0) | 0xff00;

		case 0x2: /* Analog control 1 */
			return ((readinputport(3)&0xf)<<12) | ((readinputport(3)&0xff0)>>4);

		case 0x3: /* Analog control 2 */
			return ((readinputport(4)&0xf)<<12) | ((readinputport(4)&0xff0)>>4);

		case 0x4: /* Player 3 & 4 fire buttons (Player 2 top fire buttons in Kaiser Knuckle) */
			return readinputport(5)<<8;

		case 0x5: /* Player 3 & 4 joysticks (Player 1 top fire buttons in Kaiser Knuckle) */
			return (coin_word[1]<<16) | readinputport(6);
	}

	logerror("CPU #0 PC %06x: warning - read unmapped control address %06x\n",cpu_get_pc(),offset);
	return 0xffffffff;
}

static WRITE_HANDLER( f3_control_w )
{
	switch (offset)
	{
		case 0x00: /* Watchdog */
			watchdog_reset_w(0,0);
			return;
		case 0x01: /* Coin counters & lockouts */
			if (ACCESSING_MSB32) {
				coin_lockout_w(0,~data & 0x01000000);
				coin_lockout_w(1,~data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x08000000);
				coin_word[0]=(data>>16)&0xffff;
			}
			return;
		case 0x04: /* Eeprom */
			if (ACCESSING_LSB32) {
				EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_write_bit(data & 0x04);
				EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
			}
			return;
		case 0x05:	/* Player 3 & 4 coin counters */
			if (ACCESSING_MSB32) {
				coin_lockout_w(2,~data & 0x01000000);
				coin_lockout_w(3,~data & 0x02000000);
				coin_counter_w(2, data & 0x04000000);
				coin_counter_w(3, data & 0x08000000);
				coin_word[1]=(data>>16)&0xffff;
			}
			return;
	}
	logerror("CPU #0 PC %06x: warning - write unmapped control address %06x %08x\n",cpu_get_pc(),offset,data);
}

static WRITE_HANDLER( f3_sound_reset_0_w )
{
	cpu_set_reset_line(1, CLEAR_LINE);
}

static WRITE_HANDLER( f3_sound_reset_1_w )
{
	cpu_set_reset_line(1, ASSERT_LINE);
}

/******************************************************************************/

static const struct MemoryReadAddress f3_readmem[] =
{
	{ 0x000000, 0x1fffff, MRA_ROM },
  	{ 0x400000, 0x43ffff, MRA_RAM },
	{ 0x440000, 0x447fff, MRA_RAM }, /* Palette ram */
	{ 0x4a0000, 0x4a0017, f3_control_r },
	{ 0x600000, 0x60ffff, MRA_RAM }, /* Object data */
	{ 0x610000, 0x61bfff, MRA_RAM }, /* Playfield data */
	{ 0x61c000, 0x61dfff, MRA_RAM }, /* Text layer */
	{ 0x61e000, 0x61ffff, MRA_RAM }, /* Vram */
	{ 0x620000, 0x62ffff, MRA_RAM }, /* Line ram */
	{ 0x630000, 0x63ffff, MRA_RAM }, /* Pivot ram */
	{ 0xc00000, 0xc007ff, MRA_RAM }, /* Sound CPU shared ram */
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress f3_writemem[]=
{
	{ 0x000000, 0x1fffff, MWA_RAM },
	{ 0x400000, 0x43ffff, MWA_RAM, &f3_ram },
	{ 0x440000, 0x447fff, f3_palette_24bit_w, &paletteram32 },
	{ 0x4a0000, 0x4a001f, f3_control_w },
	{ 0x600000, 0x60ffff, MWA_RAM, &spriteram32, &spriteram_size },
	{ 0x610000, 0x61bfff, f3_pf_data_w, &f3_pf_data },
	{ 0x61c000, 0x61dfff, f3_videoram_w, &videoram32 },
	{ 0x61e000, 0x61ffff, f3_vram_w, &f3_vram },
	{ 0x620000, 0x62ffff, MWA_RAM, &f3_line_ram },
	{ 0x630000, 0x63ffff, f3_pivot_w, &f3_pivot_ram },
	{ 0x660000, 0x66000f, f3_control_0_w },
	{ 0x660010, 0x66001f, f3_control_1_w },
	{ 0xc00000, 0xc007ff, MWA_RAM, &f3_shared_ram },
	{ 0xc80000, 0xc80003, f3_sound_reset_0_w },
	{ 0xc80100, 0xc80103, f3_sound_reset_1_w },
	{ -1 }  /* end of table */
};

/******************************************************************************/

static const struct MemoryReadAddress sound_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_r },
	{ 0x200000, 0x20001f, ES5505_data_0_r },
	{ 0x260000, 0x2601ff, es5510_dsp_r },
	{ 0x280000, 0x28001f, f3_68681_r },
	{ 0xc00000, 0xcfffff, MRA_BANK1 },
	{ 0xff8000, 0xffffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress sound_writemem[] = 
{
	{ 0x000000, 0x03ffff, MWA_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_w },
	{ 0x200000, 0x20001f, ES5505_data_0_w },
	{ 0x260000, 0x2601ff, es5510_dsp_w },
	{ 0x280000, 0x28001f, f3_68681_w },
	{ 0x300000, 0x30003f, f3_es5505_bank_w },
	{ 0x340000, 0x340003, f3_volume_w }, /* 8 channel volume control */
	{ 0xc00000, 0xcfffff, MWA_ROM },
	{ 0xff8000, 0xffffff, MWA_RAM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( f3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE2 ) /* Only on some games */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE3 ) /* Only on some games */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* Eprom data bit */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Another service mode */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START
	PORT_ANALOGX( 0xfff, 0x000, IPT_DIAL | IPF_PLAYER1, 25, 25, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START
	PORT_ANALOGX( 0xfff, 0x000, IPT_DIAL | IPF_PLAYER2, 25, 25, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
INPUT_PORTS_END

/******************************************************************************/

static const struct GfxLayout charlayout =
{
	8,8,
	256,
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
    { 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4, 0*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const struct GfxLayout pivotlayout =
{
	8,8,
	2048,
	4,
	{ 0,1,2,3 },
#ifdef LSB_FIRST
    { 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
    { 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4, 0*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const struct GfxLayout spriteram_layout =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1, 0, 1, 2, 3 },
	{
	4, 0, 12, 8,
	16+4, 16+0, 16+12, 16+8,
	32+4, 32+0, 32+12, 32+8,
	48+4, 48+0, 48+12, 48+8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,2),
	6,
	{ RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+3, 0, 1, 2, 3 },
	{
	4, 0, 16+4, 16+0,
    8+4, 8+0, 24+4, 24+0,
	32+4, 32+0, 48+4, 48+0,
    40+4, 40+0, 56+4, 56+0,
   	},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0,           0x000000, &charlayout,          0,  64 }, /* Dynamically modified */
	{ REGION_GFX2, 0x000000, &tile_layout, 	       0, 512 }, /* Tiles area */
	{ REGION_GFX1, 0x000000, &spriteram_layout, 4096, 256 }, /* Sprites area */
	{ 0,           0x000000, &pivotlayout,         0,  64 }, /* Dynamically modified */
	{ -1 } /* end of array */
};

/******************************************************************************/

static int f3_interrupt(void)
{
	if (cpu_getiloops()) return 3;
	return 2;
}

static void f3_machine_reset(void)
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1,&RAM[0x80000]);

	RAM[0]=RAM[0x80000]; /* Stack and Reset vectors */
	RAM[1]=RAM[0x80001];
	RAM[2]=RAM[0x80002];
	RAM[3]=RAM[0x80003];

	cpu_set_reset_line(1, ASSERT_LINE);
	f3_68681_reset();
}

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ 16000000 },		/* freq */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
};

static struct EEPROM_interface f3_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&f3_eeprom_interface);
		if (file)
			EEPROM_load(file);
	}
}

static const struct MachineDriver machine_driver_f3 =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68EC020,
			16000000,
			f3_readmem,f3_writemem,0,0,
			f3_interrupt,2
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			16000000,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, 624, /* 58.97 Hz, 624us vblank time */
	1,
	f3_machine_reset,

 	/* video hardware */
	40*8+48*2, 32*8, { 46, 40*8-1+46, 3*8, 32*8-1 },

	gfxdecodeinfo,
	8192, 8192,
	0,

#if TRY_ALPHA
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT,
#else
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN,
#endif

	f3_eof_callback,
	f3_vh_start,
	f3_vh_stop,
	f3_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_ES5505,
			&es5505_interface
		}
	},

	nvram_handler
};

/******************************************************************************/

ROM_START( elvactr )
	ROM_REGION(0x200000, REGION_CPU1 ) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("ea2w.b63",  0x000003, 0x80000, 0xcd97182b )

	ROM_REGION(0x800000, REGION_GFX1 | REGIONFLAG_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2)	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

ROM_START( elvactrj )
	ROM_REGION(0x200000, REGION_CPU1 ) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("e02-09.17", 0x000003, 0x80000, 0x23997907 )

	ROM_REGION(0x800000, REGION_GFX1 | REGIONFLAG_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2 )	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

ROM_START( elvact2u )
	ROM_REGION(0x200000, REGION_CPU1 ) /* 68020 code */
 	ROM_LOAD32_BYTE("e02-12.20", 0x000000, 0x80000, 0xea5f5a32 )
	ROM_LOAD32_BYTE("e02-11.19", 0x000001, 0x80000, 0xbcced8ff )
	ROM_LOAD32_BYTE("e02-10.18", 0x000002, 0x80000, 0x72f1b952 )
	ROM_LOAD32_BYTE("ea2.b63",   0x000003, 0x80000, 0xba9028bd )

	ROM_REGION(0x800000, REGION_GFX1 | REGIONFLAG_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE("e02-03.12", 0x000000, 0x200000, 0xc884ebb5 )
	ROM_LOAD16_BYTE("e02-02.8",  0x000001, 0x200000, 0xc8e06cfb )
	ROM_LOAD       ("e02-01.4",  0x600000, 0x200000, 0x2ba94726 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x800000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* Tiles */
	ROM_LOAD16_BYTE("e02-08.47", 0x000000, 0x200000, 0x29c9bd02 )
	ROM_LOAD16_BYTE("e02-07.45", 0x000001, 0x200000, 0x5eeee925 )
	ROM_LOAD       ("e02-06.43", 0x600000, 0x200000, 0x4c8726e9 )
	ROM_FILL       (             0x400000, 0x200000, 0 )

	ROM_REGION(0x180000, REGION_CPU2 )	/* 68000 sound CPU */
	ROM_LOAD16_BYTE("e02-13.32", 0x100000, 0x40000, 0x80932702 )
	ROM_LOAD16_BYTE("e02-14.33", 0x100001, 0x40000, 0x706671a5 )

	ROM_REGION16_BE(0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE("e02-04.38", 0x000000, 0x200000, 0xb74307af )
	ROM_LOAD16_BYTE("e02-05.39", 0xc00000, 0x200000, 0xeb729855 )
ROM_END

/******************************************************************************/

static void tile_decode(int uses_5bpp_tiles)
{
	unsigned char lsb,msb;
	unsigned int offset,i;
	UINT8 *gfx = memory_region(REGION_GFX2);
	int size=memory_region_length(REGION_GFX2);
	int half=size/2,data;

	/* Setup ROM formats:

		Some games will only use 4 or 5 bpp sprites, and some only use 4 bpp tiles,
		I don't believe this is software or prom controlled but simply the unused data lines
		are tied low on the game board if unused.  This is backed up by the fact the palette
		indexes are always related to 4 bpp data, even in 6 bpp games.

	*/
	if (uses_5bpp_tiles)
		for (i=half; i<size; i+=2)
			gfx[i+1]=0;

	offset = size/2;
	for (i = size/2+size/4; i<size; i+=2)
	{
		/* Expand 2bits into 4bits format */
		lsb = gfx[i+1];
		msb = gfx[i];

		gfx[offset+0]=((msb&0x02)<<3) | ((msb&0x01)>>0) | ((lsb&0x02)<<4) | ((lsb&0x01)<<1);
		gfx[offset+2]=((msb&0x08)<<1) | ((msb&0x04)>>2) | ((lsb&0x08)<<2) | ((lsb&0x04)>>1);
		gfx[offset+1]=((msb&0x20)>>1) | ((msb&0x10)>>4) | ((lsb&0x20)<<0) | ((lsb&0x10)>>3);
		gfx[offset+3]=((msb&0x80)>>3) | ((msb&0x40)>>6) | ((lsb&0x80)>>2) | ((lsb&0x40)>>5);

		offset+=4;
	}

	gfx = memory_region(REGION_GFX1);
	size=memory_region_length(REGION_GFX1);

	offset = size/2;
	for (i = size/2+size/4; i<size; i++)
	{
		int d1,d2,d3,d4;

		/* Expand 2bits into 4bits format */
		data = gfx[i];
		d1 = (data>>0) & 3;
		d2 = (data>>2) & 3;
		d3 = (data>>4) & 3;
		d4 = (data>>6) & 3;

		gfx[offset] = (d1<<2) | (d2<<6);
		offset++;

		gfx[offset] = (d3<<2) | (d4<<6);
		offset++;
	}
	//ALEK state_save_register_UINT32("f3", 0, "coinword", coin_word, 2);
}

#define F3_IRQ_SPEEDUP_3_R(GAME, counter, mem_addr, stack) 		\
static READ_HANDLER( irq_speedup_r_##GAME )					\
{																\
	int ptr;													\
	if ((cpu_get_sp()&2)==0) ptr=f3_ram[(cpu_get_sp()&0x1ffff)/4];	\
	else ptr=(((f3_ram[(cpu_get_sp()&0x1ffff)/4])&0x1ffff)<<16) | \
	(f3_ram[((cpu_get_sp()&0x1ffff)/4)+1]>>16); 				\
	if (cpu_get_pc()==counter && ptr==stack)					\
		cpu_spinuntil_int();									\
	return f3_ram[mem_addr];									\
}

F3_IRQ_SPEEDUP_3_R(eaction2, 0x133c,   0x07a0/4, 0x00001048 )

static void init_elvactr(void)
{
	install_mem_READ_handler(0, 0x4007a0, 0x4007a3, irq_speedup_r_eaction2 );
	f3_game=EACTION2;
	tile_decode(1);
}


/******************************************************************************/

GAME( 1994, elvactr,  0       , f3, f3, elvactr,  ROT0_16BIT,   "Taito Corporation Japan",   "Elevator Action Returns (World)" )
GAME( 1994, elvactrj, elvactr , f3, f3, elvactr,  ROT0_16BIT,   "Taito Corporation",         "Elevator Action Returns (Japan)" )
GAME( 1994, elvact2u, elvactr , f3, f3, elvactr,  ROT0_16BIT,   "Taito America Corporation", "Elevator Action 2 (US)" )

#ifdef __cplusplus
}
#endif
