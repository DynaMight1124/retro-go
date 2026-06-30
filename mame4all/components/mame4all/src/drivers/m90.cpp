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
#include "../vidhrdw/m90.cpp"

/*****************************************************************************

	Irem M90/M97 system games:

	Hasamu							1991 M90
	Bomberman						1992 M90
	Bomberman World / Atomic Punk	1992 M97
	Quiz F-1 1,2finish				1992 M97
	Risky Challenge / Gussun Oyoyo	1993 M97
	Shisensho II					1993 M97


	Uses M72 sound hardware.

	Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy!

Notes:

- Samples are not played in bbmanw/atompunk.

- Not sure about the clock speeds. In hasamu and quizf1 service mode, the
  selection moves too fast with the clock set at 16 MHz. It's still fast at
  8 MHz, but at least it's usable.

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/irem_cpu.h"
#include "sndhrdw/m72.h"


extern unsigned char *m90_video_data;

void m90_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void m90_bootleg_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( m90_video_control_w );
WRITE_HANDLER( m90_video_w );
int m90_vh_start(void);

/***************************************************************************/

static WRITE_HANDLER( m90_coincounter_w )
{
	if (offset==0) {
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		if (data&0xfe) logerror("Coin counter %02x\n",data);
	}
}

/***************************************************************************/

static const struct MemoryReadAddress readmem[] =
{
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0x80000, 0x8ffff, MRA_BANK1 },	/* Quiz F1 only */
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xd0000, 0xdffff, MRA_RAM },
	{ 0xe0000, 0xe03ff, paletteram_r },
	{ 0xffff0, 0xfffff, MRA_ROM },
	{ -1 }	/* end of table */
};


static const struct MemoryWriteAddress writemem[] =
{
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0x80000, 0x8ffff, MWA_ROM },	/* Quiz F1 only */
	{ 0xa0000, 0xa3fff, MWA_RAM },
	{ 0xd0000, 0xdffff, m90_video_w, &m90_video_data },
	{ 0xe0000, 0xe03ff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },
	{ 0xffff0, 0xfffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static const struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r }, /* Player 1 */
	{ 0x01, 0x01, input_port_1_r }, /* Player 2 */
	{ 0x02, 0x02, input_port_2_r }, /* Coins */
	{ 0x03, 0x03, MRA_NOP },		/* Unused?  High byte of above */
	{ 0x04, 0x04, input_port_3_r }, /* Dip 1 */
	{ 0x05, 0x05, input_port_4_r }, /* Dip 2 */
	{ 0x06, 0x06, input_port_5_r }, /* Player 3 */
	{ 0x07, 0x07, input_port_6_r }, /* Player 4 */
	{ -1 }	/* end of table */
};


static const struct IOWritePort writeport[] =
{
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, m90_coincounter_w },
	{ 0x80, 0x8f, m90_video_control_w },
	{ -1 }	/* end of table */
};


/*****************************************************************************/

static const struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static const struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static const struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, soundlatch_r },
	{ 0x84, 0x84, m72_sample_r },
	{ -1 }	/* end of table */
};

static const struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x81, rtype2_sample_addr_w },
	{ 0x82, 0x82, m72_sample_w },
	{ 0x83, 0x83, m72_sound_irq_ack_w },
	{ -1 }	/* end of table */
};

static const struct IOReadPort bbmanw_sound_readport[] =
{
	{ 0x41, 0x41, YM2151_status_port_0_r },
	{ 0x42, 0x42, soundlatch_r },
//	{ 0x41, 0x41, m72_sample_r },
	{ -1 }	/* end of table */
};

static const struct IOWritePort bbmanw_sound_writeport[] =
{
	{ 0x40, 0x40, YM2151_register_port_0_w },
	{ 0x41, 0x41, YM2151_data_port_0_w },
	{ 0x42, 0x42, m72_sound_irq_ack_w },
//	{ 0x40, 0x41, rtype2_sample_addr_w },
//	{ 0x42, 0x42, m72_sample_w },
	{ -1 }	/* end of table */
};

/*****************************************************************************/

INPUT_PORTS_START( hasamu )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) //service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
INPUT_PORTS_END

INPUT_PORTS_START( dynablst )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) //service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, "2 Player Upright" )
	PORT_DIPSETTING(    0x06, "4 Player Upright A" ) /* Seperate Coin Slots */
	PORT_DIPSETTING(    0x02, "4 Player Upright B" ) /* Shared Coin Slots */
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )  /* This setting shows screen with offset, no cocktail support :-( */
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
INPUT_PORTS_END

INPUT_PORTS_START( bombrman ) /* Does not appear to support 4 players or cocktail mode */
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) //service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
INPUT_PORTS_END

INPUT_PORTS_START( bbmanw )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) //service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, "2 Player" )
	PORT_DIPSETTING(    0x06, "4 Player Seprate Coins" )		/* Each player has a seperate Coin Slot */
	PORT_DIPSETTING(    0x02, "4 Player Shared Coins" )		/* All 4 players Share coin 1&2 */
	PORT_DIPSETTING(    0x00, "4 Player 1&2 3&4 Share Coins" )	/* Players 1&2 share coin 1&2, Players 3&4 share coin 3&4 */
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
INPUT_PORTS_END

/*****************************************************************************/

static const struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout, 256, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/

static const struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(90,MIXER_PAN_LEFT,90,MIXER_PAN_RIGHT) },
	{ m72_ym2151_irq_handler },
	{ 0 }
};

static const struct DACinterface dac_interface =
{
	1,	/* 1 channel */
	{ 60 }
};

static int m90_interrupt(void)
{
	return 0x60/4;
}

static const struct MachineDriver machine_driver_m90 =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/4,	/* 8 MHz ??????? */
			readmem,writemem,readport,writeport,
			m90_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_sound,

	/* video hardware */
	64*8, 64*8, { 6*8, 54*8-1, 17*8, 47*8-1 },

	gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m90_vh_start,
	0,
	m90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static const struct MachineDriver machine_driver_bombrman =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/4,	/* 8 MHz ??????? */
			readmem,writemem,readport,writeport,
			m90_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_sound,

	/* video hardware */
	64*8, 64*8, { 10*8, 50*8-1, 17*8, 47*8-1 },

	gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m90_vh_start,
	0,
	m90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static const struct MachineDriver machine_driver_bbmanw =
{
	/* basic machine hardware */
	{
		{
			CPU_V30,
			32000000/4,	/* 8 MHz ??????? */
			readmem,writemem,readport,writeport,
			m90_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			sound_readmem,sound_writemem,bbmanw_sound_readport,bbmanw_sound_writeport,
			nmi_interrupt,128	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	m72_init_sound,

	/* video hardware */
	64*8, 64*8, { 10*8, 50*8-1, 17*8, 47*8-1 },

	gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	m90_vh_start,
	0,
	m90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono? */
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

/***************************************************************************/

ROM_START( hasamu )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "hasc-p1.bin",   0x00000, 0x20000, 0x53df9834 )
	ROM_LOAD_V20_ODD( "hasc-p0.bin",    0x00000, 0x20000, 0xdff0ba6e )
	
	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "hasc-sp.bin",    0x0000, 0x10000, 0x259b1687 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "hasc-c0.bin",    0x000000, 0x20000, 0xdd5a2174 )
	ROM_LOAD( "hasc-c1.bin",    0x020000, 0x20000, 0x76b8217c )
	ROM_LOAD( "hasc-c2.bin",    0x040000, 0x20000, 0xd90f9a68 )
	ROM_LOAD( "hasc-c3.bin",    0x060000, 0x20000, 0x6cfe0d39 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* samples */
	/* No samples */
ROM_END

ROM_START( bombrman )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "bbm-p1.bin",   0x00000, 0x20000, 0x982bd166 )
	ROM_LOAD_V20_ODD( "bbm-p0.bin",    0x00000, 0x20000, 0x0a20afcc )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.bin",    0x0000, 0x10000, 0x251090cd )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bbm-c3.bin",    0x000000, 0x40000, 0x3c3613af )
	ROM_LOAD( "bbm-c2.bin",    0x040000, 0x40000, 0x0700d406 )
	ROM_LOAD( "bbm-c1.bin",    0x080000, 0x40000, 0x4c7c8bbc )
	ROM_LOAD( "bbm-c0.bin",    0x0c0000, 0x40000, 0x695d2019 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* samples */
	ROM_LOAD( "bbm-v0.bin",    0x0000, 0x20000, 0x0fa803fe )
ROM_END

ROM_START( dynablst )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "bbm-cp1e.d10",  0x00000, 0x20000, 0x27667681 )
	ROM_LOAD_V20_ODD( "bbm-cp0e.bin",   0x00000, 0x20000, 0x95db7a67 )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.bin",    0x0000, 0x10000, 0x251090cd ) 

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bbm-c0.bin",    0x000000, 0x40000, 0x695d2019 )
	ROM_LOAD( "bbm-c1.bin",    0x040000, 0x40000, 0x4c7c8bbc )
	ROM_LOAD( "bbm-c2.bin",    0x080000, 0x40000, 0x0700d406 )
	ROM_LOAD( "bbm-c3.bin",    0x0c0000, 0x40000, 0x3c3613af )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* samples */
	ROM_LOAD( "bbm-v0.bin",    0x0000, 0x20000, 0x0fa803fe )
ROM_END

ROM_START( bbmanw )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "db_h0-b.rom", 0x00000, 0x40000, 0x567d3709 )
	ROM_LOAD_V20_ODD( "db_l0-b.rom",  0x00000, 0x40000, 0xe762c22b )

	ROM_REGION( 0x10000, REGION_CPU2 )	/* 64k for the audio CPU */
	ROM_LOAD( "db_sp.rom",    0x0000, 0x10000, 0x6bc1689e )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "bbm2_c0.bin",  0x000000, 0x40000, 0xe7ce058a )
	ROM_LOAD( "bbm2_c1.bin",  0x080000, 0x40000, 0x636a78a9 )
	ROM_LOAD( "bbm2_c2.bin",  0x100000, 0x40000, 0x9ac2142f )
	ROM_LOAD( "bbm2_c3.bin",  0x180000, 0x40000, 0x47af1750 )

	ROM_REGION( 0x20000, REGION_SOUND1  )
	ROM_LOAD( "db_w04m.rom",    0x0000, 0x20000, 0x4ad889ed )
ROM_END

ROM_START( bbmanwj )
	ROM_REGION( 0x100000 * 2, REGION_CPU1 )
	ROM_LOAD_V20_EVEN( "bbm2_h0.bin", 0x00000, 0x40000, 0xe1407b91 )
	ROM_LOAD_V20_ODD( "bbm2_l0.bin",  0x00000, 0x40000, 0x20873b49 )

	ROM_REGION( 0x10000, REGION_CPU2  )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2sp-b.bin", 0x0000, 0x10000, 0xb8d8108c )

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "bbm2_c0.bin",  0x000000, 0x40000, 0xe7ce058a )
	ROM_LOAD( "bbm2_c1.bin",  0x080000, 0x40000, 0x636a78a9 )
	ROM_LOAD( "bbm2_c2.bin",  0x100000, 0x40000, 0x9ac2142f )
	ROM_LOAD( "bbm2_c3.bin",  0x180000, 0x40000, 0x47af1750 )

	ROM_REGION( 0x20000, REGION_SOUND1 )	/* samples */
	ROM_LOAD( "bbm2_vo.bin",  0x0000, 0x20000, 0x0ae655ff )
ROM_END

static void init_hasamu(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x3fff0,0x10); /* Start vector */
	irem_cpu_decrypt(0,gunforce_decryption_table);
}

static void init_bombrman(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x3fff0,0x10); /* Start vector */
	irem_cpu_decrypt(0,bomberman_decryption_table);
}

/* Bomberman World executes encrypted code from RAM! */
static WRITE_HANDLER (bbmanw_ram_write)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	RAM[0x0a0c00+offset]=data;
	RAM[0x1a0c00+offset]=dynablaster_decryption_table[data];
}

static void init_bbmanw(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy(RAM+0xffff0,RAM+0x7fff0,0x10); /* Start vector */
	irem_cpu_decrypt(0,dynablaster_decryption_table);

	install_mem_write_handler(0, 0xa0c00, 0xa0cff, bbmanw_ram_write);
}

//    YEAR,  NAME,    PARENT,   MACHINE,  INPUT,    INIT,   MONITOR,COMPANY, FULLNAME, FLAGS
GAMEX(1991, hasamu,   0,        m90,      hasamu,   hasamu,   ROT0, "Irem", "Hasamu (Japan)", GAME_NO_COCKTAIL )
GAMEX(1991, bbmanw,   0,        bbmanw,   bbmanw,   bbmanw,   ROT0, "Irem", "Bomber Man World (World)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
GAMEX(1991, bbmanwj,  bbmanw,   bombrman, bbmanw,   bbmanw,   ROT0, "Irem", "Bomber Man World (Japan)", GAME_NO_COCKTAIL )
GAMEX(1992, dynablst, 0,        bombrman, dynablst, bombrman, ROT0, "Irem (licensed from Hudson Soft)", "Dynablaster (World)", GAME_NO_COCKTAIL )
GAMEX(1992, bombrman, dynablst, bombrman, bombrman, bombrman, ROT0, "Irem (licensed from Hudson Soft)", "Bomberman (Japan)", GAME_NO_COCKTAIL )

#ifdef __cplusplus
}
#endif
