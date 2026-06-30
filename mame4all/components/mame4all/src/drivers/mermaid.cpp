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
#include "../vidhrdw/mermaid.cpp"

/***************************************************************************

Mermaid

Driver by Zsolt Vasvari

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* rougien MSM5205 sound */
static unsigned int	m_adpcm_pos = 0;
static unsigned int	m_adpcm_end = 0;
static unsigned int	m_adpcm_idle = 1;
static unsigned int	m_adpcm_trigger = 0;
static unsigned int	m_adpcm_rom_sel = 0;
static unsigned int	m_adpcm_play_reg = 0;


static unsigned char *mermaid_AY8910_enable;

static WRITE_HANDLER( mermaid_AY8910_write_port_w )
{
    if (mermaid_AY8910_enable[0])  AY8910_write_port_0_w(offset, data);
    if (mermaid_AY8910_enable[1])  AY8910_write_port_1_w(offset, data);
}

static WRITE_HANDLER( mermaid_AY8910_control_port_w )
{
    if (mermaid_AY8910_enable[0])  AY8910_control_port_0_w(offset, data);
    if (mermaid_AY8910_enable[1])  AY8910_control_port_1_w(offset, data);
}

static WRITE_HANDLER(rougien_sample_rom_lo_w)
{
    m_adpcm_rom_sel = (data & 1) | (m_adpcm_rom_sel & 2);
}

static WRITE_HANDLER(rougien_sample_rom_hi_w)
{
    m_adpcm_rom_sel = ((data & 1)<<1) | (m_adpcm_rom_sel & 1);
}

static WRITE_HANDLER(rougien_sample_playback_w)
{
    if((m_adpcm_play_reg & 1) && ((data & 1) == 0))
    {
        m_adpcm_pos = m_adpcm_rom_sel*0x1000;
        m_adpcm_end = m_adpcm_pos+0x1000;
        m_adpcm_idle = 0;
        MSM5205_reset_w(0,0);
    }

    m_adpcm_play_reg = data & 1;
}

static void rougien_adpcm_int( int num )
{
    if (m_adpcm_pos >= m_adpcm_end || m_adpcm_idle)
    {
        MSM5205_reset_w(0, 1);
        m_adpcm_trigger = 0;
    }
    else
    {
        //UINT8 *ROM = device->machine().root_device().memregion("adpcm")->base();
        unsigned char *ROM = memory_region(REGION_SOUND1);

        int m_adpcm_data = ((m_adpcm_trigger ? (ROM[m_adpcm_pos] & 0x0f) : (ROM[m_adpcm_pos] & 0xf0) >> 4));
        MSM5205_data_w(0, m_adpcm_data);
        m_adpcm_trigger ^= 1;
        if (m_adpcm_trigger == 0)
        {
            m_adpcm_pos++;
        }
    }
}


static const struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd81f, MRA_RAM },
	{ 0xd840, 0xd8bf, MRA_RAM },
	{ 0xdc00, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, mermaid_collision_r },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcbff, mermaid_background_videoram_w, &mermaid_background_videoram, &videoram_size },
	{ 0xd000, 0xd3ff, mermaid_foreground_videoram_w, &mermaid_foreground_videoram },
	{ 0xd800, 0xd81f, mermaid_bg_scroll_w, &mermaid_background_scrollram },
	{ 0xd840, 0xd85f, mermaid_fg_scroll_w, &mermaid_foreground_scrollram },
	{ 0xd880, 0xd8bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdc00, 0xdfff, mermaid_foreground_colorram_w, &mermaid_foreground_colorram },
	{ 0xe000, 0xe001, MWA_RAM, &mermaid_AY8910_enable },
	{ 0xe005, 0xe005, mermaid_flip_screen_x_w },
	{ 0xe006, 0xe006, mermaid_flip_screen_y_w },
	{ 0xe007, 0xe007, interrupt_enable_w },
	{ 0xe807, 0xe807, MWA_NOP },	/* watchdog? */
	{ 0xe804, 0xe804, rougien_gfxbankswitch1_w },
	{ 0xe805, 0xe805, rougien_gfxbankswitch2_w },
	{ 0xf802, 0xf802, MWA_NOP },	/* ??? see memory map */
	{ 0xf806, 0xf806, mermaid_AY8910_write_port_w },
	{ 0xf807, 0xf807, mermaid_AY8910_control_port_w },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress rougien_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcbff, mermaid_background_videoram_w, &mermaid_background_videoram, &videoram_size },
	{ 0xd000, 0xd3ff, mermaid_foreground_videoram_w, &mermaid_foreground_videoram },
	{ 0xd800, 0xd81f, mermaid_bg_scroll_w, &mermaid_background_scrollram },
	{ 0xd840, 0xd85f, mermaid_fg_scroll_w, &mermaid_foreground_scrollram },
	{ 0xd880, 0xd8bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdc00, 0xdfff, mermaid_foreground_colorram_w, &mermaid_foreground_colorram },
	{ 0xe000, 0xe001, MWA_RAM, &mermaid_AY8910_enable },
	{ 0xe002, 0xe002, rougien_sample_playback_w },
	{ 0xe005, 0xe005, mermaid_flip_screen_x_w },
	{ 0xe006, 0xe006, mermaid_flip_screen_y_w },
	{ 0xe007, 0xe007, interrupt_enable_w },
	{ 0xe807, 0xe807, MWA_NOP },	/* watchdog? */
	{ 0xe802, 0xe802, rougien_sample_rom_hi_w },
	{ 0xe803, 0xe803, rougien_sample_rom_lo_w },
	{ 0xe804, 0xe804, rougien_gfxbankswitch1_w },
	{ 0xe805, 0xe805, rougien_gfxbankswitch2_w },
	{ 0xf802, 0xf802, MWA_NOP },	/* ??? see memory map */
	{ 0xf806, 0xf806, mermaid_AY8910_write_port_w },
	{ 0xf807, 0xf807, mermaid_AY8910_control_port_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( mermaid )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPSETTING(    0x04, "30K" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )


	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

INPUT_PORTS_END

INPUT_PORTS_START( rougien )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50K" )
	PORT_DIPSETTING(    0x04, "70K" )
	PORT_DIPNAME( 0x08, 0x08, "Difficulty" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )


	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

INPUT_PORTS_END


static const struct GfxLayout background_charlayout =
{
	8,8,    /* 8*8 chars */
	RGN_FRAC(1,1),    /* 256 characters */
	1,      /* 1 bit per pixel */
	{ 0 },  /* single bitplane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static const struct GfxLayout foreground_charlayout =
{
	8,8,    /* 8*8 chars */
	RGN_FRAC(1,2),   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, RGN_FRAC(1,2) },  /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static const struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,2),	/* 256 sprites */
	2,		/* 2 bits per pixel */
	{ 0, RGN_FRAC(1,2) },   /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &foreground_charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout,              0, 16 },
	{ REGION_GFX2, 0, &background_charlayout,  4*16, 2  },
	{ -1 } /* end of array */
};


static const struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static const struct MachineDriver machine_driver_mermaid =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,        /* 4.00 MHz??? */
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	4*16+1, 4*16+2*2,
	mermaid_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	mermaid_vh_eof,
	mermaid_vh_start,
	mermaid_vh_stop,
	mermaid_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};


struct MSM5205interface rougien_msm5205_interface =
{
    1,
    384000,
    { rougien_adpcm_int },    /* interrupt function */
    { MSM5205_S96_4B },       /* 4KHz, 4 Bits */
    { 80 }
};


static const struct MachineDriver machine_driver_rougien =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,        /* 4.00 MHz??? */
			readmem,rougien_writemem,0,0,
			nmi_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	4*16+1, 4*16+2*2,
	rougien_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	mermaid_vh_eof,
	mermaid_vh_start,
	mermaid_vh_stop,
	mermaid_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&rougien_msm5205_interface
		}

	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( mermaid )
	ROM_REGION( 0x10000, REGION_CPU1 )       /* 64k for code */
	ROM_LOAD( "g960_32.15",	  0x0000, 0x1000, 0x8311f090 )
	ROM_LOAD( "g960_33.16",	  0x1000, 0x1000, 0x9f274fc4 )
	ROM_LOAD( "g960_34.17",	  0x2000, 0x1000, 0x5f910179 )
	ROM_LOAD( "g960_35.18",	  0x3000, 0x1000, 0xdb1868a1 )
	ROM_LOAD( "g960_36.19",	  0x4000, 0x1000, 0x178a3567 )
	ROM_LOAD( "g960_37.20",	  0x5000, 0x1000, 0x7d602527 )
	ROM_LOAD( "g960_38.21",	  0x6000, 0x1000, 0xbf9f623c )
	ROM_LOAD( "g960_39.22",	  0x7000, 0x1000, 0xdf0db390 )
	ROM_LOAD( "g960_40.23",	  0x8000, 0x1000, 0xfb7aba3f )
	ROM_LOAD( "g960_41.24",	  0x9000, 0x1000, 0xd022981d )

	ROM_REGION( 0x4000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "g960_45.77",	  0x0000, 0x1000, 0x1f6b735e )
	ROM_LOAD( "g960_44.76",	  0x1000, 0x1000, 0xfd76074e )
	ROM_LOAD( "g960_47.79",	  0x2000, 0x1000, 0x3b7d4ad0 )
	ROM_LOAD( "g960_46.78",	  0x3000, 0x1000, 0x50c117cd )

	ROM_REGION( 0x1000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "g960_43.26",	  0x0000, 0x1000, 0x6f077417 )

	ROM_REGION( 0x0040, REGION_PROMS )
	ROM_LOAD( "col_a", 	      0x0000, 0x0020, 0xef87bcd6 )
	ROM_LOAD( "col_b", 	      0x0020, 0x0020, 0xca48abdd )
ROM_END

ROM_START( rougien )
	ROM_REGION( 0x10000, REGION_CPU1 )       // 64k for code
	ROM_LOAD( "rou-00.bin", 0x0000, 0x1000, 0x14cd1108)
	ROM_LOAD( "rou-01.bin", 0x1000, 0x1000, 0xee40670d)
	ROM_LOAD( "rou-02.bin", 0x2000, 0x1000, 0x5e528f46)
	ROM_LOAD( "rou-03.bin", 0x3000, 0x1000, 0xd235a7e8)
	ROM_LOAD( "rou-04.bin", 0x4000, 0x1000, 0x7352bb66)
	ROM_LOAD( "rou-05.bin", 0x5000, 0x1000, 0xaf77444d)
	ROM_LOAD( "rou-06.bin", 0x6000, 0x1000, 0x2c16c857)
	ROM_LOAD( "rou-07.bin", 0x7000, 0x1000, 0xfbf32f2e)
	ROM_LOAD( "rou-08.bin", 0x8000, 0x1000, 0xbfac531c)
	ROM_LOAD( "rou-09.bin", 0x9000, 0x1000, 0xaf854340)

	ROM_REGION( 0xc000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "rou-21.bin", 0x0000, 0x1000, 0x36e4ba8c)
	ROM_LOAD( "rou-20.bin", 0x1000, 0x1000, 0xc5dc1258)
	ROM_LOAD( "rou-23.bin", 0x2000, 0x1000, 0x5974c848)
	ROM_LOAD( "rou-22.bin", 0x3000, 0x1000, 0x35811443)

	ROM_LOAD( "rou-25.bin", 0x4000, 0x1000, 0x706d9864)
	ROM_LOAD( "rou-24.bin", 0x5000, 0x1000, 0x56ceb0be)
	ROM_LOAD( "rou-27.bin", 0x6000, 0x1000, 0x522fa2e0)
	ROM_LOAD( "rou-26.bin", 0x7000, 0x1000, 0xfbbc6339)

	ROM_LOAD( "rou-29.bin", 0x8000, 0x1000, 0xbf018a7e)
	ROM_LOAD( "rou-28.bin", 0x9000, 0x1000, 0x33f160dc)
	ROM_LOAD( "rou-31.bin", 0xa000, 0x1000, 0xb2a6f058)
	ROM_LOAD( "rou-30.bin", 0xb000, 0x1000, 0xc75be223)

	ROM_REGION( 0x1000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "rou-43.bin", 0x0000, 0x1000, 0xee4b9de4)

	ROM_REGION( 0x3000, REGION_SOUND1 )
	ROM_LOAD( "rou-42.bin", 0x0000, 0x1000, 0x5ce13444) // speech
	ROM_LOAD( "rou-41.bin", 0x1000, 0x1000, 0x59ed0d88) // laugh
	ROM_LOAD( "rou-40.bin", 0x2000, 0x1000, 0xab38b942) // alien whistle

	ROM_REGION( 0x40, REGION_PROMS )
	ROM_LOAD( "prom_a.bin", 0x0000, 0x0020, 0x49f619b9)
	ROM_LOAD( "prom_b.bin", 0x0020, 0x0020, 0x41ad4fc8)

ROM_END



GAME ( 1982, mermaid, 0, mermaid, mermaid, 0, ROT0, "Rock-ola", "Mermaid" )
GAME ( 1982, rougien, 0, rougien, rougien, 0, ROT0, "Sanritsu", "Rougien" )

#ifdef __cplusplus
}
#endif
