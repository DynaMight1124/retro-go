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
#include "../vidhrdw/lwings.cpp"

/***************************************************************************

  Legendary Wings
  Section Z
  Trojan
  Avengers

  Driver provided by Paul Leaman

To Do:
-	sectionz does "false contacts" on the coin counters, causing them to
	increment twice per coin.
-	clean up Avengers protection; it currently checks against hard-coded program
	counter rather than behaving as a memory-mapped black box.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Avengers runs on hardware almost identical to Trojan, but with a protection
 * device and some small changes to the memory map and videohardware.
 *
 * Background colors are fetched 64 bytes at a time and copied to palette RAM.
 *
 * Another function takes as input 2 pairs of (x,y) coordinates, and returns
 * a code reflecting the direction (8 angles) from one point to the other.
 */
static data8_t avengers_param[4];
static int avengers_palette_pen;

static data8_t avengers_adpcm;

WRITE_HANDLER( avengers_adpcm_w )
{
	avengers_adpcm = data;
}

READ_HANDLER( avengers_adpcm_r )
{
	return avengers_adpcm;
}

static WRITE_HANDLER( lwings_bankswitch_w )
{
	unsigned char *RAM;
	int bank;

	/* bit 0 is flip screen */
	flip_screen_w(0,~data & 0x01);

	/* bits 1 and 2 select ROM bank */
	RAM = memory_region(REGION_CPU1);
	bank = (data & 0x06) >> 1;
	cpu_setbank(1,&RAM[0x10000 + bank*0x4000]);

	/* bit 3 enables NMI */
	interrupt_enable_w(0,data & 0x08);

	/* bits 6 and 7 are coin counters */
	coin_counter_w(1,data & 0x40);
	coin_counter_w(0,data & 0x80);
}

static int lwings_interrupt(void)
{
	return 0x00d7; /* RST 10h */
}

static int avengers_interrupt( void )
{
	if( cpu_getiloops()==0 )
	{
		return interrupt();
	}
	return nmi_interrupt();
}

static WRITE_HANDLER( avengers_protection_w )
{
	int pc = cpu_get_pc();

	if( pc == 0x2eeb )
	{
		avengers_param[0] = data;
	}
	else if( pc == 0x2f09 )
	{
		avengers_param[1] = data;
	}
	else if( pc == 0x2f26 )
	{
		avengers_param[2] = data;
	}
	else if( pc == 0x2f43 )
	{
		avengers_param[3] = data;
	}
	else if( pc == 0x0445 )
	{
		soundlatch_w( 0, data );
	}
}

static WRITE_HANDLER( avengers_prot_bank_w )
{
	avengers_palette_pen = data*64;
}

static int avengers_fetch_paldata( void )
{
	const char pal_data[] =
	/* page 1: 0x03,0x02,0x01,0x00 */
	"0000000000000000" "A65486A6364676D6" "C764C777676778A7" "A574E5E5C5756AE5"
	"0000000000000000" "F51785D505159405" "A637B6A636269636" "F45744E424348824"
	"0000000000000000" "A33263B303330203" "4454848454440454" "A27242C232523632"
	"0000000000000000" "1253327202421102" "3386437373631373" "41A331A161715461"
	"0000000000000000" "1341715000711203" "4442635191622293" "5143D48383D37186"
	"0000000000000000" "2432423000412305" "6633343302333305" "7234A565A5A4A2A8"
	"0000000000000000" "46232422A02234A7" "88241624A21454A7" "A3256747A665D3AA"
	"0000000000000000" "070406020003050B" "0A05090504050508" "05060A090806040C"

	/* page2: 0x07,0x06,0x05,0x04 */
	"0000000000000000" "2472030503230534" "6392633B23433B53" "0392846454346423"
	"0000000000000000" "1313052405050423" "3223754805354832" "323346A38686A332"
	"0000000000000000" "72190723070723D2" "81394776070776D1" "A15929F25959F2F1"
	"0000000000000000" "650706411A2A1168" "770737C43A3A3466" "87071F013C0C3175"
	"0000000000000000" "2001402727302020" "4403048F4A484344" "4A050B074E0E4440"
	"0000000000000000" "3003800C35683130" "5304035C587C5453" "5607080C5B265550"
	"0000000000000000" "4801D00043854245" "6C020038669A6569" "6604050A69446764"
	"0000000000000000" "0504000001030504" "0A05090504060307" "04090D0507010403"

	/* page3: 0x0b,0x0a,0x09,0x08 */
	"0000000000000000" "685A586937F777F7" "988A797A67A7A7A7" "B8CA898DC737F787"
	"0000000000000000" "4738A61705150505" "8797672835250535" "7777072A25350525"
	"0000000000000000" "3525642404340404" "6554453554440454" "5544053634540434"
	"0000000000000000" "2301923203430303" "4333834383630373" "3324034473730363"
	"0000000000000000" "3130304000762005" "5352525291614193" "6463635483D06581"
	"0000000000000000" "4241415100483107" "6463631302335304" "76757415A5A077A3"
	"0000000000000000" "53525282A02A43AA" "76747424A31565A5" "88888536A66089A4"
	"0000000000000000" "05040304000D050C" "0806050604070707" "0A0A060808000C06"

	/* page4: 0x0f,0x0e,0x0d,0x0c */
	"0000000000000000" "3470365956342935" "5590578997554958" "73C078A8C573687A"
	"0000000000000000" "5355650685030604" "2427362686042607" "010A070584010508"
	"0000000000000000" "0208432454022403" "737A243455733406" "000D050353000307"
	"0000000000000000" "000A023233003202" "424C134234424204" "000F241132001105"
	"0000000000000000" "3031113030300030" "5152215252512051" "7273337374723272"
	"0000000000000000" "4141214041411041" "6263326363623162" "8385448585834383"
	"0000000000000000" "5153225152512051" "7375437475734273" "9598559697946495"
	"0000000000000000" "0205020303020102" "0407040606040304" "060A060809060506"

	/* page5: 0x13,0x12,0x11,0x10 */
	"0000000000000000" "4151D141D3D177F7" "5454C44482C4A7A7" "0404D45491D4F787"
	"0000000000000000" "0303032374230505" "9696962673560535" "0505054502850525"
	"0000000000000000" "0303030355030404" "7777770754470454" "0606060603760434"
	"0000000000000000" "0505053547050303" "4949492945390373" "0808083804580363"
	"0000000000000000" "0B0C444023442005" "3D3F333433334193" "0000043504046581"
	"0000000000000000" "0809565085863107" "0B6A352374455304" "00700644050677A3"
	"0000000000000000" "06073879C8C843AA" "09492739A58765A5" "0050084A060889A4"
	"0000000000000000" "05060B070B0B050C" "0707090707090707" "00000B08070B0C06"

	/* page6: 0x17,0x16,0x15,0x14 */
	"0000000000000000" "0034308021620053" "0034417042512542" "0034526064502E31"
	"0000000000000000" "0106412032733060" "11A6522053628350" "22A6632072620D42"
	"0000000000000000" "1308223052242080" "2478233071235170" "3578243090230960"
	"0000000000000000" "2111334333331404" "3353324232324807" "45B5314131310837"
	"0000000000000000" "3232445444445302" "445443534343B725" "567642524242B745"
	"0000000000000000" "4343556555550201" "5575546454540524" "6787536353537554"
	"0000000000000000" "6474667676660100" "7696657575650423" "88A8647474645473"
	"0000000000000000" "0001070701050004" "0003060603040303" "0005050505040302";

	int bank = avengers_palette_pen/64;
	int offs = avengers_palette_pen%64;
	int page = bank/4;					/* 0..7 */
	int base = (3-(bank&3));			/* 0..3 */
	int row = offs&0xf;					/* 0..15 */
	int col = offs/16 + base*4;			/* 0..15 */
	int digit0 = pal_data[page*256*2 + (31-row*2)*16+col];
	int digit1 = pal_data[page*256*2 + (30-row*2)*16+col];
	int result;

	if( digit0>='A' ) digit0 += 10 - 'A'; else digit0 -= '0';
	if( digit1>='A' ) digit1 += 10 - 'A'; else digit1 -= '0';
	result = digit0 * 16 + digit1;

	if( (avengers_palette_pen&0x3f)!=0x3f ) avengers_palette_pen++;

	return result;
}

static READ_HANDLER( avengers_protection_r )
{
	const int xpos[8] = { 10, 7,  0, -7, -10, -7,   0,  7 };
	const int ypos[8] = {  0, 7, 10,  7,   0, -7, -10, -7 };
	int best_dist = 0;
	int best_dir = 0;
	int x,y;
	int dx,dy,dist,dir;

	if( cpu_get_pc() == 0x7c7 )
	{
		/* palette data */
		return avengers_fetch_paldata();
	}

	/*	Point to Angle Function

		Input: two cartesian points
		Output: direction code (north,northeast,east,...)
	 */
	x = avengers_param[0] - avengers_param[2];
	y = avengers_param[1] - avengers_param[3];
	for( dir=0; dir<8; dir++ )
	{
		dx = xpos[dir]-x;
		dy = ypos[dir]-y;
		dist = dx*dx+dy*dy;
		if( dist < best_dist || dir==0 )
		{
			best_dir = dir;
			best_dist = dist;
		}
	}
	return best_dir<<5;
}

static WRITE_HANDLER( msm5205_w )
{
	MSM5205_reset_w(offset,(data>>7)&1);
	MSM5205_data_w(offset,data);
	MSM5205_vclk_w(offset,1);
	MSM5205_vclk_w(offset,0);
}

static MEMORY_READ_START( avengers_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xf7ff, MRA_RAM },
	{ 0xf808, 0xf808, input_port_0_r },
	{ 0xf809, 0xf809, input_port_1_r },
	{ 0xf80a, 0xf80a, input_port_2_r },
	{ 0xf80b, 0xf80b, input_port_3_r },
	{ 0xf80c, 0xf80c, input_port_4_r },
	{ 0xf80d, 0xf80d, avengers_protection_r },
MEMORY_END

static MEMORY_WRITE_START( avengers_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xddff, MWA_RAM },
	{ 0xde00, 0xdf7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdf80, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, lwings_fgvideoram_w, &lwings_fgvideoram },
	{ 0xe800, 0xefff, lwings_bg1videoram_w, &lwings_bg1videoram },
	{ 0xf000, 0xf3ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0xf400, 0xf7ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0xf800, 0xf801, lwings_bg1_scrollx_w },
	{ 0xf802, 0xf803, lwings_bg1_scrolly_w },
	{ 0xf804, 0xf804, lwings_bg2_scrollx_w },
	{ 0xf805, 0xf805, lwings_bg2_image_w },
	{ 0xf808, 0xf808, MWA_NOP }, /* ? */
	{ 0xf809, 0xf809, avengers_protection_w },
	{ 0xf80c, 0xf80c, avengers_prot_bank_w },
	{ 0xf80d, 0xf80d, avengers_adpcm_w },
	{ 0xf80e, 0xf80e, lwings_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( readmem ) /* common to trojan and lwings */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xf7ff, MRA_RAM },
	{ 0xf808, 0xf808, input_port_0_r },
	{ 0xf809, 0xf809, input_port_1_r },
	{ 0xf80a, 0xf80a, input_port_2_r },
	{ 0xf80b, 0xf80b, input_port_3_r },
	{ 0xf80c, 0xf80c, input_port_4_r },
MEMORY_END

static MEMORY_WRITE_START( writemem ) /* lwings */
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xddff, MWA_RAM },
	{ 0xde00, 0xdfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe7ff, lwings_fgvideoram_w, &lwings_fgvideoram },
	{ 0xe800, 0xefff, lwings_bg1videoram_w, &lwings_bg1videoram },
	{ 0xf000, 0xf3ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0xf400, 0xf7ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0xf808, 0xf809, lwings_bg1_scrollx_w },
	{ 0xf80a, 0xf80b, lwings_bg1_scrolly_w },
	{ 0xf80c, 0xf80c, soundlatch_w },
	{ 0xf80d, 0xf80d, watchdog_reset_w },
	{ 0xf80e, 0xf80e, lwings_bankswitch_w },
MEMORY_END

static MEMORY_WRITE_START( trojan_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xddff, MWA_RAM },
	{ 0xde00, 0xdf7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdf80, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, lwings_fgvideoram_w, &lwings_fgvideoram },
	{ 0xe800, 0xefff, lwings_bg1videoram_w, &lwings_bg1videoram },
	{ 0xf000, 0xf3ff, paletteram_RRRRGGGGBBBBxxxx_split2_w, &paletteram_2 },
	{ 0xf400, 0xf7ff, paletteram_RRRRGGGGBBBBxxxx_split1_w, &paletteram },
	{ 0xf800, 0xf801, lwings_bg1_scrollx_w },
	{ 0xf802, 0xf803, lwings_bg1_scrolly_w },
	{ 0xf804, 0xf804, lwings_bg2_scrollx_w },
	{ 0xf805, 0xf805, lwings_bg2_image_w },
	{ 0xf80c, 0xf80c, soundlatch_w },
	{ 0xf80d, 0xf80d, watchdog_reset_w },
	{ 0xf80e, 0xf80e, lwings_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0xe006, 0xe006, MRA_RAM },    /* Avengers - ADPCM status?? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ 0xe006, 0xe006, MWA_RAM },    /* Avengers - ADPCM output??? */
MEMORY_END

static MEMORY_READ_START( adpcm_readmem )
	{ 0x0000, 0xffff, MRA_ROM },
MEMORY_END

/* Yes, _no_ ram */
static MEMORY_WRITE_START( adpcm_writemem )
/*	{ 0x0000, 0xffff, MWA_ROM }, avoid cluttering up error.log */
	{ 0x0000, 0xffff, MWA_NOP },
MEMORY_END

static PORT_READ_START( avengers_adpcm_readport )
	{ 0x00, 0x00, avengers_adpcm_r },
PORT_END

static PORT_READ_START( adpcm_readport )
	{ 0x00, 0x00, soundlatch_r },
PORT_END

static PORT_WRITE_START( adpcm_writeport )
	{ 0x01, 0x01, msm5205_w },
PORT_END

INPUT_PORTS_START( sectionz )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x38, "20000 50000" )
	PORT_DIPSETTING(    0x18, "20000 60000" )
	PORT_DIPSETTING(    0x28, "20000 70000" )
	PORT_DIPSETTING(    0x08, "30000 60000" )
	PORT_DIPSETTING(    0x30, "30000 70000" )
	PORT_DIPSETTING(    0x10, "30000 80000" )
	PORT_DIPSETTING(    0x20, "40000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*      PORT_DIPSETTING(    0x80, "???" )       probably unused */
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( lwings )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty?" )
	PORT_DIPSETTING(    0x02, "Easy?" )
	PORT_DIPSETTING(    0x06, "Normal?" )
	PORT_DIPSETTING(    0x04, "Difficult?" )
	PORT_DIPSETTING(    0x00, "Very Difficult?" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xe0, "20000 50000" )
	PORT_DIPSETTING(    0x60, "20000 60000" )
	PORT_DIPSETTING(    0xa0, "20000 70000" )
	PORT_DIPSETTING(    0x20, "30000 60000" )
	PORT_DIPSETTING(    0xc0, "30000 70000" )
	PORT_DIPSETTING(    0x40, "30000 80000" )
	PORT_DIPSETTING(    0x80, "40000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
INPUT_PORTS_END

INPUT_PORTS_START( trojan )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x03, DEF_STR( Cocktail ) )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x0c, "20000 70000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x1c, "30000 60000" )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x14, "30000 80000" )
	PORT_DIPSETTING(    0x04, "40000 80000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xe0, 0xe0, "Starting Level" )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
/* 0x00 and 0x20 start at level 6 */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

/* Trojan with level selection - starting level dip switches not used */
INPUT_PORTS_START( trojanls )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x03, DEF_STR( Cocktail ) )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x0c, "20000 70000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x1c, "30000 60000" )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x14, "30000 80000" )
	PORT_DIPSETTING(    0x04, "40000 80000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( avengers )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus" )
	PORT_DIPSETTING(    0x30, "20k 60k" )
	PORT_DIPSETTING(    0x10, "20k 70k" )
	PORT_DIPSETTING(    0x20, "20k 80k" )
	PORT_DIPSETTING(    0x00, "30k 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x00, "6" )

	PORT_START      /* DSWA */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

static const struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static const struct GfxLayout bg1_tilelayout =
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

static const struct GfxLayout bg2_tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};


static const struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     512, 16 }, /* colors 512-575 */
	{ REGION_GFX2, 0, &bg1_tilelayout,   0,  8 }, /* colors   0-127 */
	{ REGION_GFX3, 0, &spritelayout,   384,  8 }, /* colors 384-511 */
	{ -1 } /* end of array */
};

static const struct GfxDecodeInfo gfxdecodeinfo_trojan[] =
{
	{ REGION_GFX1, 0, &charlayout,     768, 16 }, /* colors 768-831 */
	{ REGION_GFX2, 0, &bg1_tilelayout, 256,  8 }, /* colors 256-383 */
	{ REGION_GFX3, 0, &spritelayout,   640,  8 }, /* colors 640-767 */
	{ REGION_GFX4, 0, &bg2_tilelayout,   0,  8 }, /* colors   0-127 */
	{ -1 } /* end of array */
};

static const struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(10,20), YM2203_VOL(10,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static const struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip */
	384000,				/* 384KHz ? */
	{ 0 },				/* interrupt function */
	{ MSM5205_SEX_4B },	/* slave mode */
	{ 50 }
};

static const struct MachineDriver machine_driver_lwings =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 MHz (?) */
			readmem,writemem,0,0,
			lwings_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,        /* 3 MHz (?) */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	lwings_eof_callback,
	lwings_vh_start,
	0,
	lwings_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static const struct MachineDriver machine_driver_trojan =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 MHz (?) */
			readmem,trojan_writemem,0,0,
			lwings_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,        /* 3 MHz (?) */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? */
			adpcm_readmem,adpcm_writemem,adpcm_readport,adpcm_writeport,
			0,0,
			interrupt,4000
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo_trojan,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	lwings_eof_callback,
	trojan_vh_start,
	0,
	trojan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface,
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};

static const struct MachineDriver machine_driver_avengers =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 MHz (?) */
			avengers_readmem,avengers_writemem,0,0,
			avengers_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,        /* 3 MHz (?) */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? */
			adpcm_readmem,adpcm_writemem,avengers_adpcm_readport,adpcm_writeport,
			0,0,
			interrupt,4000
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo_trojan,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	lwings_eof_callback,
	avengers_vh_start,
	0,
	trojan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface,
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

ROM_START( lwings )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_lw01.bin",  0x00000, 0x8000, 0xb55a7f60 )
	ROM_LOAD( "7c_lw02.bin",  0x10000, 0x8000, 0xa5efbb1b )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, 0xec5cc201 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0xa20337a2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, 0x091d923c ) /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3e_lw14.bin",  0x00000, 0x8000, 0x5436392c ) /* tiles */
	ROM_LOAD( "1e_lw08.bin",  0x08000, 0x8000, 0xb491bbbb )
	ROM_LOAD( "3d_lw13.bin",  0x10000, 0x8000, 0xfdd1908a )
	ROM_LOAD( "1d_lw07.bin",  0x18000, 0x8000, 0x5c73d406 )
	ROM_LOAD( "3b_lw12.bin",  0x20000, 0x8000, 0x32e17b3c )
	ROM_LOAD( "1b_lw06.bin",  0x28000, 0x8000, 0x52e533c1 )
	ROM_LOAD( "3f_lw15.bin",  0x30000, 0x8000, 0x99e134ba )
	ROM_LOAD( "1f_lw09.bin",  0x38000, 0x8000, 0xc8f28777 )

	ROM_REGION( 0x20000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3j_lw17.bin",  0x00000, 0x8000, 0x5ed1bc9b )  /* sprites */
	ROM_LOAD( "1j_lw11.bin",  0x08000, 0x8000, 0x2a0790d6 )
	ROM_LOAD( "3h_lw16.bin",  0x10000, 0x8000, 0xe8834006 )
	ROM_LOAD( "1h_lw10.bin",  0x18000, 0x8000, 0xb693f5a5 )

	//ROM_REGION( 0x0100, REGION_PROMS )
	//ROM_LOAD( "63s141.15g",   0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
ROM_END

ROM_START( lwings2 )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "u13-l",        0x00000, 0x8000, 0x3069c01c )
	ROM_LOAD( "u14-k",        0x10000, 0x8000, 0x5d91c828 )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, 0xec5cc201 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0xa20337a2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, 0x091d923c )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b_03e.rom",    0x00000, 0x8000, 0x176e3027 )  /* tiles */
	ROM_LOAD( "b_01e.rom",    0x08000, 0x8000, 0xf5d25623 )
	ROM_LOAD( "b_03d.rom",    0x10000, 0x8000, 0x001caa35 )
	ROM_LOAD( "b_01d.rom",    0x18000, 0x8000, 0x0ba008c3 )
	ROM_LOAD( "b_03b.rom",    0x20000, 0x8000, 0x4f8182e9 )
	ROM_LOAD( "b_01b.rom",    0x28000, 0x8000, 0xf1617374 )
	ROM_LOAD( "b_03f.rom",    0x30000, 0x8000, 0x9b374dcc )
	ROM_LOAD( "b_01f.rom",    0x38000, 0x8000, 0x23654e0a )

	ROM_REGION( 0x20000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b_03j.rom",    0x00000, 0x8000, 0x8f3c763a )  /* sprites */
	ROM_LOAD( "b_01j.rom",    0x08000, 0x8000, 0x7cc90a1d )
	ROM_LOAD( "b_03h.rom",    0x10000, 0x8000, 0x7d58f532 )
	ROM_LOAD( "b_01h.rom",    0x18000, 0x8000, 0x3e396eda )

	//ROM_REGION( 0x0100, REGION_PROMS )
	//ROM_LOAD( "63s141.15g",   0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
ROM_END

ROM_START( lwingsjp )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "a_06c.rom",    0x00000, 0x8000, 0x2068a738 )
	ROM_LOAD( "a_07c.rom",    0x10000, 0x8000, 0xd6a2edc4 )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, 0xec5cc201 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, 0xa20337a2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, 0x091d923c )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b_03e.rom",    0x00000, 0x8000, 0x176e3027 )  /* tiles */
	ROM_LOAD( "b_01e.rom",    0x08000, 0x8000, 0xf5d25623 )
	ROM_LOAD( "b_03d.rom",    0x10000, 0x8000, 0x001caa35 )
	ROM_LOAD( "b_01d.rom",    0x18000, 0x8000, 0x0ba008c3 )
	ROM_LOAD( "b_03b.rom",    0x20000, 0x8000, 0x4f8182e9 )
	ROM_LOAD( "b_01b.rom",    0x28000, 0x8000, 0xf1617374 )
	ROM_LOAD( "b_03f.rom",    0x30000, 0x8000, 0x9b374dcc )
	ROM_LOAD( "b_01f.rom",    0x38000, 0x8000, 0x23654e0a )

	ROM_REGION( 0x20000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b_03j.rom",    0x00000, 0x8000, 0x8f3c763a )  /* sprites */
	ROM_LOAD( "b_01j.rom",    0x08000, 0x8000, 0x7cc90a1d )
	ROM_LOAD( "b_03h.rom",    0x10000, 0x8000, 0x7d58f532 )
	ROM_LOAD( "b_01h.rom",    0x18000, 0x8000, 0x3e396eda )

	//ROM_REGION( 0x0100, REGION_PROMS )
	//ROM_LOAD( "63s141.15g",   0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
ROM_END

ROM_START( sectionz )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_sz01.bin",  0x00000, 0x8000, 0x69585125 )
	ROM_LOAD( "7c_sz02.bin",  0x10000, 0x8000, 0x22f161b8 )
	ROM_LOAD( "9c_sz03.bin",  0x18000, 0x8000, 0x4c7111ed )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, 0xa6073566 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9h_sz05.bin",  0x00000, 0x4000, 0x3173ba2e )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3e_sz14.bin",  0x00000, 0x8000, 0x63782e30 )  /* tiles */
	ROM_LOAD( "1e_sz08.bin",  0x08000, 0x8000, 0xd57d9f13 )
	ROM_LOAD( "3d_sz13.bin",  0x10000, 0x8000, 0x1b3d4d7f )
	ROM_LOAD( "1d_sz07.bin",  0x18000, 0x8000, 0xf5b3a29f )
	ROM_LOAD( "3b_sz12.bin",  0x20000, 0x8000, 0x11d47dfd )
	ROM_LOAD( "1b_sz06.bin",  0x28000, 0x8000, 0xdf703b68 )
	ROM_LOAD( "3f_sz15.bin",  0x30000, 0x8000, 0x36bb9bf7 )
	ROM_LOAD( "1f_sz09.bin",  0x38000, 0x8000, 0xda8f06c9 )

	ROM_REGION( 0x20000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3j_sz17.bin",  0x00000, 0x8000, 0x8df7b24a )  /* sprites */
	ROM_LOAD( "1j_sz11.bin",  0x08000, 0x8000, 0x685d4c54 )
	ROM_LOAD( "3h_sz16.bin",  0x10000, 0x8000, 0x500ff2bb )
	ROM_LOAD( "1h_sz10.bin",  0x18000, 0x8000, 0x00b3d244 )

	//ROM_REGION( 0x0100, REGION_PROMS )
	//ROM_LOAD( "mb7114e.15g",  0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
ROM_END

ROM_START( sctionza )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "sz-01a.bin",   0x00000, 0x8000, 0x98df49fd )
	ROM_LOAD( "7c_sz02.bin",  0x10000, 0x8000, 0x22f161b8 )
	ROM_LOAD( "sz-03j.bin",   0x18000, 0x8000, 0x94547abf )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, 0xa6073566 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "9h_sz05.bin",  0x00000, 0x4000, 0x3173ba2e )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3e_sz14.bin",  0x00000, 0x8000, 0x63782e30 )  /* tiles */
	ROM_LOAD( "1e_sz08.bin",  0x08000, 0x8000, 0xd57d9f13 )
	ROM_LOAD( "3d_sz13.bin",  0x10000, 0x8000, 0x1b3d4d7f )
	ROM_LOAD( "1d_sz07.bin",  0x18000, 0x8000, 0xf5b3a29f )
	ROM_LOAD( "3b_sz12.bin",  0x20000, 0x8000, 0x11d47dfd )
	ROM_LOAD( "1b_sz06.bin",  0x28000, 0x8000, 0xdf703b68 )
	ROM_LOAD( "3f_sz15.bin",  0x30000, 0x8000, 0x36bb9bf7 )
	ROM_LOAD( "1f_sz09.bin",  0x38000, 0x8000, 0xda8f06c9 )

	ROM_REGION( 0x20000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3j_sz17.bin",  0x00000, 0x8000, 0x8df7b24a )  /* sprites */
	ROM_LOAD( "1j_sz11.bin",  0x08000, 0x8000, 0x685d4c54 )
	ROM_LOAD( "3h_sz16.bin",  0x10000, 0x8000, 0x500ff2bb )
	ROM_LOAD( "1h_sz10.bin",  0x18000, 0x8000, 0x00b3d244 )

	ROM_REGION( 0x0100, REGION_PROMS )
	ROM_LOAD( "mb7114e.15g",  0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
ROM_END

ROM_START( trojan )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "t4",           0x00000, 0x8000, 0xc1bbeb4e )
	ROM_LOAD( "t6",           0x10000, 0x8000, 0xd49592ef )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, 0x9273b264 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, 0x21154797 )

	ROM_REGION( 0x10000, REGION_CPU3 )     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, 0x1c0f91b2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, 0x581a2b4c )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, 0x285a052b )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, 0xaeb693f7 )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, 0xdfb0fe5c )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, 0xd3a4c9d1 )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, 0x00f0f4fd )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, 0xdff2ee02 )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, 0x14bfac18 )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, 0x71ba8a6d )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, 0x862c4713 )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, 0xd86f8cbd )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, 0x12a73b3f )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, 0xbb1a2769 )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, 0x39daafd4 )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, 0x94615d2a )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, 0x66c642bd )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, 0x81d5ab36 )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, 0x6e38c6fa )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, 0x14fc6cf2 )

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "tb23.bin",     0x00000, 0x08000, 0xeda13c0e )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbp24s10.7j",  0x0000, 0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "mb7114e.1e",   0x0100, 0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END

ROM_START( trojanr )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "tb04.bin",     0x00000, 0x8000, 0x92670f27 )
	ROM_LOAD( "tb06.bin",     0x10000, 0x8000, 0xa4951173 )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, 0x9273b264 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, 0x21154797 )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, 0x1c0f91b2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, 0x581a2b4c )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, 0x285a052b )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, 0xaeb693f7 )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, 0xdfb0fe5c )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, 0xd3a4c9d1 )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, 0x00f0f4fd )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, 0xdff2ee02 )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, 0x14bfac18 )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, 0x71ba8a6d )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, 0x862c4713 )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, 0xd86f8cbd )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, 0x12a73b3f )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, 0xbb1a2769 )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, 0x39daafd4 )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, 0x94615d2a )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, 0x66c642bd )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, 0x81d5ab36 )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, 0x6e38c6fa )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, 0x14fc6cf2 )

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "tb23.bin",     0x0000,  0x8000, 0xeda13c0e )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbp24s10.7j",  0x0000,  0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "mb7114e.1e",   0x0100,  0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END

ROM_START( trojanj )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "troj-04.rom",  0x00000, 0x8000, 0x0b5a7f49 )
	ROM_LOAD( "troj-06.rom",  0x10000, 0x8000, 0xdee6ed92 )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, 0x9273b264 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, 0x21154797 )

	ROM_REGION( 0x10000, REGION_CPU3 )     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, 0x1c0f91b2 )

	ROM_REGION( 0x04000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, 0x581a2b4c )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, 0x285a052b )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, 0xaeb693f7 )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, 0xdfb0fe5c )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, 0xd3a4c9d1 )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, 0x00f0f4fd )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, 0xdff2ee02 )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, 0x14bfac18 )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, 0x71ba8a6d )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, 0x862c4713 )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, 0xd86f8cbd )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, 0x12a73b3f )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, 0xbb1a2769 )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, 0x39daafd4 )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, 0x94615d2a )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, 0x66c642bd )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, 0x81d5ab36 )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, 0x6e38c6fa )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, 0x14fc6cf2 )

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "tb23.bin",     0x0000,  0x8000, 0xeda13c0e )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbp24s10.7j",  0x0000,  0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "mb7114e.1e",   0x0100,  0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END

ROM_START( avengers )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "04.10n",       0x00000, 0x8000, 0xa94aadcc )
	ROM_LOAD( "06.13n",       0x10000, 0x8000, 0x39cd80bd )
	ROM_LOAD( "05.12n",       0x18000, 0x8000, 0x06b1cec9 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000, 0x8000, 0x107a2e17 ) /* ?? */

	ROM_REGION( 0x10000, REGION_CPU3 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000, 0x8000, 0xc1e5d258 ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, 0xefb5883e )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, 0x9b5ff305 ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, 0x08323355 )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, 0x6d5261ba ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, 0xa13d9f54 )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, 0xa2911d8b ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, 0xcde78d32 )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, 0x44ac2671 ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, 0xb1a717cb )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, 0x3c876a17 ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, 0x4b1ff3ac )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, 0x4eb543ef )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, 0x8041de7f )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, 0xbdaa8b22 ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, 0x566e3059 )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, 0x301059aa )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, 0xa00485ec )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE ) /* bg tiles */
	ROM_LOAD( "25.15n",       0x00000, 0x8000, 0x230d9e30 ) /* planes 0,1 */
	ROM_LOAD( "24.13n",       0x08000, 0x8000, 0xa6354024 ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, 0xc0a93ef6 )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END

ROM_START( avenger2 )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "avg4.bin",     0x00000, 0x8000, 0x0fea7ac5 )
	ROM_LOAD( "av_06a.13n",   0x10000, 0x8000, 0x491a712c )
	ROM_LOAD( "av_05.12n",    0x18000, 0x8000, 0x9a214b42 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000,  0x8000, 0x107a2e17 ) /* MISSING from this set */

	ROM_REGION( 0x10000, REGION_CPU3 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000,  0x8000, 0xc1e5d258 ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, 0xefb5883e )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, 0x9b5ff305 ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, 0x08323355 )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, 0x6d5261ba ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, 0xa13d9f54 )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, 0xa2911d8b ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, 0xcde78d32 )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, 0x44ac2671 ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, 0xb1a717cb )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, 0x3c876a17 ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, 0x4b1ff3ac )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, 0x4eb543ef )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, 0x8041de7f )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, 0xbdaa8b22 ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, 0x566e3059 )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, 0x301059aa )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, 0xa00485ec )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "25.15n",       0x00000, 0x8000, 0x230d9e30 ) /* planes 0,1 */
	ROM_LOAD( "24.13n",       0x08000, 0x8000, 0xa6354024 ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, 0xc0a93ef6 )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END


ROM_START( buraiken )
	ROM_REGION( 0x20000, REGION_CPU1 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "av_04a.10n",   0x00000, 0x8000, 0x361fc614 )
	ROM_LOAD( "av_06a.13n",   0x10000, 0x8000, 0x491a712c )
	ROM_LOAD( "av_05.12n",    0x18000, 0x8000, 0x9a214b42 )

	ROM_REGION( 0x10000, REGION_CPU2 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000,  0x8000, 0x107a2e17 )

	ROM_REGION( 0x10000, REGION_CPU3 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000,  0x8000, 0xc1e5d258 ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, 0xefb5883e )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2 | REGIONFLAG_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, 0x9b5ff305 ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, 0x08323355 )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, 0x6d5261ba ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, 0xa13d9f54 )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, 0xa2911d8b ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, 0xcde78d32 )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, 0x44ac2671 ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, 0xb1a717cb )

	ROM_REGION( 0x40000, REGION_GFX3 | REGIONFLAG_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, 0x3c876a17 ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, 0x4b1ff3ac )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, 0x4eb543ef )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, 0x8041de7f )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, 0xbdaa8b22 ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, 0x566e3059 )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, 0x301059aa )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, 0xa00485ec )

	ROM_REGION( 0x10000, REGION_GFX4 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "av_25.15n",    0x00000, 0x8000, 0x88a505a7 ) /* planes 0,1 */
	ROM_LOAD( "av_24.13n",    0x08000, 0x8000, 0x1f4463c8 ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, 0xc0a93ef6 )  /* Tile Map */

	//ROM_REGION( 0x0200, REGION_PROMS )
	//ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, 0xd96bcc98 )	/* timing (not used) */
	//ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, 0x5052fa9d )	/* priority (not used) */
ROM_END

GAME( 1985, sectionz, 0,        lwings,   sectionz, 0, ROT0,  "Capcom", "Section Z (set 1)" )
GAME( 1985, sctionza, sectionz, lwings,   sectionz, 0, ROT0,  "Capcom", "Section Z (set 2)" )
GAME( 1986, lwings,   0,        lwings,   lwings,   0, ROT90, "Capcom", "Legendary Wings (US set 1)" )
GAME( 1986, lwings2,  lwings,   lwings,   lwings,   0, ROT90, "Capcom", "Legendary Wings (US set 2)" )
GAME( 1986, lwingsjp, lwings,   lwings,   lwings,   0, ROT90, "Capcom", "Ales no Tsubasa (Japan)" )
GAME( 1986, trojan,   0,        trojan,   trojanls, 0, ROT0,  "Capcom", "Trojan (US)" )
GAME( 1986, trojanr,  trojan,   trojan,   trojan,   0, ROT0,  "Capcom (Romstar license)", "Trojan (Romstar)" )
GAME( 1986, trojanj,  trojan,   trojan,   trojan,   0, ROT0,  "Capcom", "Tatakai no Banka (Japan)" )
GAME( 1987, avengers, 0,        avengers, avengers, 0, ROT90, "Capcom", "Avengers (US set 1)" )
GAME( 1987, avenger2, avengers, avengers, avengers, 0, ROT90, "Capcom", "Avengers (US set 2)" )
GAME( 1987, buraiken, avengers, avengers, avengers, 0, ROT90, "Capcom", "Hissatsu Buraiken (Japan)" )

#ifdef __cplusplus
}
#endif
