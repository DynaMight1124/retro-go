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
#include "../vidhrdw/zaccaria.cpp"

/***************************************************************************

Jack Rabbit memory map (preliminary)

driver by Nicola Salmoria
thanks to Andrea Babich for the manual.

TODO:
- correctly hook up TMS5200 (there's a kludge in zaccaria_ca2_r to make it work)

- there seems to be a strange kind of DAC connected to 8910 #0 port A, but it sounds
  horrible so I'm leaving its volume at 0.

- The 8910 outputs go through some analog circuitry to make them sound more like
  real intruments.
  #0 Ch. A = "rullante"/"cassa" (drum roll/bass drum) (selected by bits 3&4 of port A)
  #0 Ch. B = "basso" (bass)
  #0 Ch. C = straight out through an optional filter
  #1 Ch. A = "piano"
  #1 Ch. B = "tromba" (trumpet) (level selected by bit 0 of port A)
  #1 Ch. C = disabled (there's an open jumper, otherwise would go out through a filter)

- some minor color issues (see vidhrdw)


Notes:
- There is a protection device which I haven't located on the schematics. It
  sits on bits 4-7 of the data bus, and is read from locations where only bits
  0-3 are connected to regular devices (6400-6407 has 4-bit RAM, while 6c00-6c07
  has a 4-bit input port).

- The 6802 driving the TMS5220 has a push button connected to the NMI line. Test?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"
#include "machine/8255ppi.h"

extern data8_t *zaccaria_videoram,*zaccaria_attributesram;

void zaccaria_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int zaccaria_vh_start(void);
WRITE_HANDLER( zaccaria_videoram_w );
WRITE_HANDLER( zaccaria_attributes_w );
WRITE_HANDLER( zaccaria_flip_screen_x_w );
WRITE_HANDLER( zaccaria_flip_screen_y_w );
void zaccaria_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int dsw;

static WRITE_HANDLER( zaccaria_dsw_sel_w )
{
	switch (data & 0xf0)
	{
		case 0xe0:
			dsw = 0;
			break;

		case 0xd0:
			dsw = 1;
			break;

		case 0xb0:
			dsw = 2;
			break;

		default:
//logerror("PC %04x: portsel = %02x\n",cpu_get_pc(),data);
			break;
	}
}

static READ_HANDLER( zaccaria_dsw_r )
{
	return readinputport(dsw);
}



static WRITE_HANDLER( ay8910_port0a_w )
{
	// bits 0-2 go to a weird kind of DAC ??
	// bits 3-4 control the analog drum emulation on 8910 #0 ch. A

	if (data & 1)	/* DAC enable */
	{
		/* TODO: is this right? it sound awful */
		static int table[4] = { 0x05, 0x1b, 0x0b, 0x55 };
		DAC_signed_data_w(0,table[(data & 0x06) >> 1]);
	}
	else
		DAC_signed_data_w(0,0x80);
}


void zaccaria_irq0a(int state) { cpu_set_nmi_line(1,  state ? ASSERT_LINE : CLEAR_LINE); }
void zaccaria_irq0b(int state) { cpu_set_irq_line(1,0,state ? ASSERT_LINE : CLEAR_LINE); }

static int active_8910,port0a,acs;

static READ_HANDLER( zaccaria_port0a_r )
{
	if (active_8910 == 0)
		return AY8910_read_port_0_r(0);
	else
		return AY8910_read_port_1_r(0);
}

static WRITE_HANDLER( zaccaria_port0a_w )
{
	port0a = data;
}

static WRITE_HANDLER( zaccaria_port0b_w )
{
	static int last;


	/* bit 1 goes to 8910 #0 BDIR pin  */
	if ((last & 0x02) == 0x02 && (data & 0x02) == 0x00)
	{
		/* bit 0 goes to the 8910 #0 BC1 pin */
		if (last & 0x01)
			AY8910_control_port_0_w(0,port0a);
		else
			AY8910_write_port_0_w(0,port0a);
	}
	else if ((last & 0x02) == 0x00 && (data & 0x02) == 0x02)
	{
		/* bit 0 goes to the 8910 #0 BC1 pin */
		if (last & 0x01)
			active_8910 = 0;
	}
	/* bit 3 goes to 8910 #1 BDIR pin  */
	if ((last & 0x08) == 0x08 && (data & 0x08) == 0x00)
	{
		/* bit 2 goes to the 8910 #1 BC1 pin */
		if (last & 0x04)
			AY8910_control_port_1_w(0,port0a);
		else
			AY8910_write_port_1_w(0,port0a);
	}
	else if ((last & 0x08) == 0x00 && (data & 0x08) == 0x08)
	{
		/* bit 2 goes to the 8910 #1 BC1 pin */
		if (last & 0x04)
			active_8910 = 1;
	}

	last = data;
}

static int zaccaria_cb1_toggle(void)
{
	static int toggle;

	pia_0_cb1_w(0,toggle & 1);
	toggle ^= 1;

	return ignore_interrupt();
}



static int port1a,port1b;

static READ_HANDLER( zaccaria_port1a_r )
{
	if (~port1b & 1) return tms5220_status_r(0);
	else return port1a;
}

static WRITE_HANDLER( zaccaria_port1a_w )
{
	port1a = data;
}

static WRITE_HANDLER( zaccaria_port1b_w )
{
	port1b = data;

	// bit 0 = /RS

	// bit 1 = /WS
	if (~data & 2) tms5220_data_w(0,port1a);

	// bit 3 = "ACS" (goes, inverted, to input port 6 bit 3)
	acs = ~data & 0x08;

	// bit 4 = led (for testing?)
	//set_led_status(0,~data & 0x10);
}

static READ_HANDLER( zaccaria_ca2_r )
{
// TODO: this doesn't work, why?
//	return !tms5220_ready_r();

static int counter;
counter = (counter+1) & 0x0f;

return counter;

}

static void tms5220_irq_handler(int state)
{
	pia_1_cb1_w(0,state ? 0 : 1);
}



static struct pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zaccaria_port0a_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zaccaria_port0a_w, zaccaria_port0b_w, 0, 0,
	/*irqs   : A/B             */ zaccaria_irq0a, zaccaria_irq0b
};

static struct pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zaccaria_port1a_r, 0, 0, 0, zaccaria_ca2_r, 0,
	/*outputs: A/B,CA/B2       */ zaccaria_port1a_w, zaccaria_port1b_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};


static ppi8255_interface ppi8255_intf =
{
	1, 								/* 1 chip */
	input_port_3_r,				/* Port A read */
	input_port_4_r,				/* Port B read */
	input_port_5_r,				/* Port C read */
	0,							/* Port A write */
	0,							/* Port B write */
	zaccaria_dsw_sel_w, 			/* Port C write */
};


static void zaccaria_init_machine(void)
{
	ppi8255_init(&ppi8255_intf);
	
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &pia_1_intf);
	pia_reset();
}


static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(2,0,(data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( sound1_command_w )
{
	pia_0_ca1_w(0,data & 0x80);
	soundlatch2_w(0,data);
}

static WRITE_HANDLER( mc1408_data_w )
{
	DAC_data_w(1,data);
}


struct GameDriver monymony_driver;

READ_HANDLER( zaccaria_prot1_r )
{
	switch (offset)
	{
		case 0:
			return 0x50;    /* Money Money */

		case 4:
			return 0x40;    /* Jack Rabbit */

		case 6:
			if (Machine->gamedrv == &monymony_driver)
				return 0x70;    /* Money Money */
			return 0xa0;    /* Jack Rabbit */

		default:
			return 0;
	}
}

READ_HANDLER( zaccaria_prot2_r )
{
	switch (offset)
	{
		case 0:
			return (input_port_6_r(0) & 0x07) | (acs & 0x08);   /* bits 4 and 5 must be 0 in Jack Rabbit */

		case 2:
			return 0x10;    /* Jack Rabbit */

		case 4:
			return 0x80;    /* Money Money */

		case 6:
			return 0x00;    /* Money Money */

		default:
			return 0;
	}
}


static WRITE_HANDLER( coin_w )
{
	coin_counter_w(0,data & 1);
}

static WRITE_HANDLER( nmienable_w )
{
	interrupt_enable_w(0,data & 1);
}


static const struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x63ff, MRA_RAM },
	{ 0x6400, 0x6407, zaccaria_prot1_r },
	{ 0x6c00, 0x6c07, zaccaria_prot2_r },
	{ 0x6e00, 0x6e00, zaccaria_dsw_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0x7803, ppi8255_0_r },
	{ 0x7c00, 0x7c00, watchdog_reset_r },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x63ff, videoram_w, &videoram, &videoram_size },
	{ 0x6400, 0x67ff, colorram_w, &colorram },
	//{ 0x6000, 0x67ff, zaccaria_videoram_w, &zaccaria_videoram },	/* 6400-67ff is 4 bits wide */
	{ 0x6800, 0x683f, zaccaria_attributes_w, &zaccaria_attributesram },
	{ 0x6840, 0x685f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x6881, 0x68bc, MWA_RAM, &spriteram_2, &spriteram_2_size },
	{ 0x6c00, 0x6c00, zaccaria_flip_screen_x_w },
	{ 0x6c01, 0x6c01, zaccaria_flip_screen_y_w },
	{ 0x6c02, 0x6c02, MWA_NOP },    /* sound reset */
	{ 0x6e00, 0x6e00, sound_command_w },
	{ 0x6c06, 0x6c06, coin_w },
	{ 0x6c07, 0x6c07, nmienable_w },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0x7800, 0x7803, ppi8255_0_w },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryReadAddress sound_readmem1[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x500c, 0x500f, pia_0_r },
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress sound_writemem1[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x500c, 0x500f, pia_0_w },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryReadAddress sound_readmem2[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0090, 0x0093, pia_1_r },
	{ 0x1800, 0x1800, soundlatch_r },
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress sound_writemem2[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0090, 0x0093, pia_1_w },
	{ 0x1000, 0x1000, mc1408_data_w },	/* MC1408 */
	{ 0x1400, 0x1400, sound1_command_w },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( monymony )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )  /* random high scores? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "200000" )
	PORT_DIPSETTING(    0x02, "300000" )
	PORT_DIPSETTING(    0x03, "400000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Table Title" )
	PORT_DIPSETTING(    0x00, "Todays High Scores" )
	PORT_DIPSETTING(    0x04, "High Scores" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x8c, 0x84, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x8c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x70, 0x50, "Coin C" )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* other bits are outputs */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* "ACS" - from pin 13 of a PIA on the sound board */
	/* other bits come from a protection device */
INPUT_PORTS_END

INPUT_PORTS_START( jackrabt )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Table Title" )
	PORT_DIPSETTING(    0x00, "Todays High Scores" )
	PORT_DIPSETTING(    0x04, "High Scores" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x8c, 0x84, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x8c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x70, 0x50, "Coin C" )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* other bits are outputs */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* "ACS" - from pin 13 of a PIA on the sound board */
	/* other bits come from a protection device */
INPUT_PORTS_END



static const struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,  /* 3 bits per pixel */
	{ 2*1024*8*8, 1*1024*8*8, 0*1024*8*8},
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static const struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,  /* 3 bits per pixel */
	{ 2*1024*8*8, 1*1024*8*8, 0*1024*8*8},
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 32 },
	{ REGION_GFX1, 0, &spritelayout, 32*8, 32 },
	{ -1 } /* end of array */
};

struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	3580000/2,
	{ 15, 15 },
	{ 0, 0 },
	{ soundlatch2_r, 0 },
	{ ay8910_port0a_w, 0 },
	{ 0, 0 }
};

static const struct DACinterface dac_interface =
{
	2,
	{ 0,80 }	/* I'm leaving the first DAC(?) off because it sounds awful */
};

static struct TMS5220interface tms5220_interface =
{
	640000,				/* clock speed (80*samplerate) */
	80,					/* volume */
	tms5220_irq_handler	/* IRQ handler */
};


static const struct MachineDriver machine_driver_zaccaria =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6802 | CPU_AUDIO_CPU,
			3580000/4,	/* 895 kHz */
			sound_readmem1,sound_writemem1,0,0,
			ignore_interrupt,0,	/* IRQ and NMI triggered by the PIA */
			zaccaria_cb1_toggle,3580000/4096
		},
		{
			CPU_M6802 | CPU_AUDIO_CPU,
			3580000/4,	/* 895 kHz */
			sound_readmem2,sound_writemem2,0,0,
			ignore_interrupt,0	/* IRQ triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,   /* frames per second, vblank duration */
	1,  /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	zaccaria_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	512, 32*8+32*8,
	zaccaria_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	zaccaria_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( monymony )
	ROM_REGION( 0x10000, REGION_CPU1 )     /* 64k for code */
	ROM_LOAD( "1a",           0x0000, 0x1000, 0x13c227ca )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "1b",           0x1000, 0x1000, 0x87372545 )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "1c",           0x2000, 0x1000, 0x6aea9c01 )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "1d",           0x3000, 0x1000, 0x5fdec451 )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "2a",           0x4000, 0x1000, 0xaf830e3c )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "2c",           0x5000, 0x1000, 0x31da62b1 )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2 ) /* 64k for first 6802 */
	ROM_LOAD( "2g",           0xa000, 0x2000, 0x78b01b98 )
	ROM_LOAD( "1i",           0xe000, 0x2000, 0x94e3858b )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* 64k for second 6802 */
	ROM_LOAD( "1h",           0xa000, 0x1000, 0xaad76193 )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "1g",           0xb000, 0x1000, 0x1e8ffe3e )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "2d",           0x0000, 0x2000, 0x82ab4d1a )
	ROM_LOAD( "1f",           0x2000, 0x2000, 0x40d4e4d1 )
	ROM_LOAD( "1e",           0x4000, 0x2000, 0x36980455 )

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "monymony.9g",  0x0000, 0x0200, 0xfc9a0f21 )
	ROM_LOAD( "monymony.9f",  0x0200, 0x0200, 0x93106704 )
ROM_END

ROM_START( jackrabt )
	ROM_REGION( 0x10000, REGION_CPU1 )     /* 64k for code */
	ROM_LOAD( "cpu-01.1a",    0x0000, 0x1000, 0x499efe97 )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "cpu-01.2l",    0x1000, 0x1000, 0x4772e557 )
	ROM_LOAD( "cpu-01.3l",    0x2000, 0x1000, 0x1e844228 )
	ROM_LOAD( "cpu-01.4l",    0x3000, 0x1000, 0xebffcc38 )
	ROM_LOAD( "cpu-01.5l",    0x4000, 0x1000, 0x275e0ed6 )
	ROM_LOAD( "cpu-01.6l",    0x5000, 0x1000, 0x8a20977a )
	ROM_LOAD( "cpu-01.2h",    0x9000, 0x1000, 0x21f2be2a )
	ROM_LOAD( "cpu-01.3h",    0xa000, 0x1000, 0x59077027 )
	ROM_LOAD( "cpu-01.4h",    0xb000, 0x1000, 0x0b9db007 )
	ROM_LOAD( "cpu-01.5h",    0xc000, 0x1000, 0x785e1a01 )
	ROM_LOAD( "cpu-01.6h",    0xd000, 0x1000, 0xdd5979cf )

	ROM_REGION( 0x10000, REGION_CPU2 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, 0xfc05654e )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, 0x3dab977f )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, 0xf4507111 )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, 0xc722eff8 )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, 0x9f880ef5 )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, 0xafc04cd7 )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, 0x14f23cdd )

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, 0x85577107 )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, 0x085914d1 )
ROM_END

ROM_START( jackrab2 )
	ROM_REGION( 0x10000, REGION_CPU1 )     /* 64k for code */
	ROM_LOAD( "1cpu2.1a",     0x0000, 0x1000, 0xf9374113 )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "2cpu2.1b",     0x1000, 0x1000, 0x0a0eea4a )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "3cpu2.1c",     0x2000, 0x1000, 0x291f5772 )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "4cpu2.1d",     0x3000, 0x1000, 0x10972cfb )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "5cpu2.2a",     0x4000, 0x1000, 0xaa95d06d )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "6cpu2.2c",     0x5000, 0x1000, 0x404496eb )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, 0xfc05654e )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, 0x3dab977f )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, 0xf4507111 )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, 0xc722eff8 )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, 0x9f880ef5 )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, 0xafc04cd7 )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, 0x14f23cdd )

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, 0x85577107 )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, 0x085914d1 )
ROM_END

ROM_START( jackrabs )
	ROM_REGION( 0x10000, REGION_CPU1 )     /* 64k for code */
	ROM_LOAD( "1cpu.1a",      0x0000, 0x1000, 0x6698dc65 )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "2cpu.1b",      0x1000, 0x1000, 0x42b32929 )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "3cpu.1c",      0x2000, 0x1000, 0x89b50c9a )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "4cpu.1d",      0x3000, 0x1000, 0xd5520665 )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "5cpu.2a",      0x4000, 0x1000, 0x0f9a093c )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "6cpu.2c",      0x5000, 0x1000, 0xf53d6356 )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, 0xfc05654e )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, 0x3dab977f )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, 0xf4507111 )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, 0xc722eff8 )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, 0x9f880ef5 )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, 0xafc04cd7 )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, 0x14f23cdd )

	ROM_REGION( 0x0400, REGION_PROMS )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, 0x85577107 )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, 0x085914d1 )
ROM_END



GAMEX( 1983, monymony, 0,        zaccaria, monymony, 0, ROT90, "Zaccaria", "Money Money", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrabt, 0,        zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (set 1)", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrab2, jackrabt, zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (set 2)", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrabs, jackrabt, zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (special)", GAME_IMPERFECT_SOUND )

#ifdef __cplusplus
}
#endif
