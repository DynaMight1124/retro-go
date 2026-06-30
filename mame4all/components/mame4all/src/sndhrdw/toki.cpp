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
enum
{
    VECTOR_INIT,
    RST10_ASSERT,
    RST10_CLEAR,
    RST18_ASSERT,
    RST18_CLEAR
};


static int sound_cpu=1;
static UINT8 main2sub[2],sub2main[2];
static int main2sub_pending,sub2main_pending;
static int start, end, start1, end1;

READ_HANDLER( seibu_main_word_r );
WRITE_HANDLER( seibu_main_word_w );


static void setvector_callback(int param)
{
    static int irq1,irq2;

    switch(param)
    {
	case VECTOR_INIT:
	    irq1 = irq2 = 0xff;
	    break;

	case RST10_ASSERT:
	    irq1 = 0xd7;
	    break;

	case RST10_CLEAR:
	    irq1 = 0xff;
	    break;

	case RST18_ASSERT:
	    irq2 = 0xdf;
	    break;

	case RST18_CLEAR:
	    irq2 = 0xff;
	    break;
    }

    cpu_irq_line_vector_w(sound_cpu,0,irq1 & irq2);
    if ((irq1 & irq2) == 0xff)      /* no IRQs pending */
	cpu_set_irq_line(sound_cpu,0,CLEAR_LINE);
    else    /* IRQ pending */
	cpu_set_irq_line(sound_cpu,0,ASSERT_LINE);
}


#if 0
void seibu_ym3812_irqhandler(int linestate)
{
    if (linestate)
	timer_set(TIME_NOW,RST10_ASSERT,setvector_callback);
    else
	timer_set(TIME_NOW,RST10_CLEAR,setvector_callback);
}
#endif

WRITE_HANDLER( cabal_sound_irq_trigger_word_w )
{
	seibu_main_word_w(8,data);

	/* spin for a while to let the Z80 read the command, otherwise coins "stick" */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}

#if 0
/* Use this if the sound cpu is cpu 1 */
void seibu_sound_init_1(void)
{
	sound_cpu=1;
	setvector_callback(VECTOR_INIT);
}
#endif

static READ_HANDLER( seibu_soundlatch_r )
{
	return main2sub[offset];
}

static WRITE_HANDLER( seibu_main_data_w )
{
	sub2main[offset] = data;
}

WRITE_HANDLER( seibu_coin_w )
{
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);
}

READ_HANDLER( seibu_main_data_pending_r )
{
	return sub2main_pending ? 1 : 0;
}

WRITE_HANDLER( seibu_irq_clear_w )
{
	setvector_callback(VECTOR_INIT);
}

#if 0
WRITE_HANDLER( seibu_rst10_ack_w )
{
    /* Unused for now */
}

WRITE_HANDLER( seibu_rst18_ack_w )
{
    timer_set(TIME_NOW,RST18_CLEAR,setvector_callback);
}
#else
extern WRITE_HANDLER( seibu_rst10_ack_w );
extern WRITE_HANDLER( seibu_rst18_ack_w );
#endif

READ_HANDLER( seibu_main_word_r )
{
    offset >>= 1; // 8-bit to 16-bit conversion

    switch (offset)
    {
    case 2:
    case 3:
	return sub2main[(offset)-2];
    case 5:
	return main2sub_pending ? 1 : 0;
    default:
	return 0xffff;
    }
}

WRITE_HANDLER( seibu_main_word_w )
{
    offset >>= 1; // 8-bit to 16-bit conversion

    if (ACCESSING_LSB)
    {
	switch (offset)
	{
	    case 0:
	    case 1:
		main2sub[offset] = data;
		break;
	    case 4:
		timer_set(TIME_NOW,RST18_ASSERT,setvector_callback);
		break;
	    case 6:
		/* just a guess */
		sub2main_pending = 0;
		main2sub_pending = 1;
		break;
	}
    }
}


WRITE_HANDLER( seibu_adpcm_adr_1_w )
{
    if (offset)
    {
	end = data<<8;
    }
    else
    {
	start = data<<8;
    }
}

WRITE_HANDLER( seibu_adpcm_ctl_1_w )
{
    switch (data)
    {
	case 0:
	    ADPCM_stop(0);
	    break;
	case 2:
	    break;
	case 1:
	    ADPCM_play(0, start, end-start);
	    break;
    }
}

WRITE_HANDLER( seibu_adpcm_adr_2_w )
{
    if (offset)
    {
	end1 = data<<8;
    }
    else
    {
	start1 = data<<8;
    }
}

WRITE_HANDLER( seibu_adpcm_ctl_2_w )
{
    switch (data)
    {
	case 0:
	    ADPCM_stop(1);
	    break;
	case 2:
	    break;
	case 1:
	    start1 += 0x10000;
	    end1 += 0x10000;
	    ADPCM_play(1, start1, end1-start1);
	    break;
    }
}

static WRITE_HANDLER( seibu_bank_w )
{
    UINT8 *rom = memory_region(REGION_CPU1+sound_cpu);

    cpu_setbank(1,rom + 0x10000 + 0x8000 * (data & 1));
}

WRITE_HANDLER( seibu_pending_w )
{
    /* just a guess */
    main2sub_pending = 0;
    sub2main_pending = 1;
}


static UINT8 decrypt_data(int a,int src)
{
	int xxor = 0;

	if ( BIT(a,9)  &  BIT(a,8))                                    xxor |= 0x80;
	if ( BIT(a,11) &  BIT(a,4)  & BIT(a,1))                        xxor |= 0x40;
	if ( BIT(a,11) & ~BIT(a,8)  & BIT(a,1))                        xxor |= 0x04;
	if ( BIT(a,13) & ~BIT(a,11) & BIT(a,9) &  BIT(a,4) & BIT(a,2)) xxor |= 0x02;
	if ( BIT(a,13) & ~BIT(a,11) & BIT(a,9) & ~BIT(a,4) & BIT(a,2)) xxor |= 0x01;
	if (~BIT(a,13) & ~BIT(a,11) & BIT(a,9) &  BIT(a,2))            xxor |= 0x01;
	if ( BIT(a,13) & ~BIT(a,6)  & BIT(a,4))                        xxor |= 0x01;

	if (BIT(a,13) & BIT(a,4) & (BIT(src,0) ^ BIT(src,1))) xxor ^= 0x03;
	if (BIT(a, 8) & BIT(a,4) & (BIT(src,2) ^ BIT(src,3))) xxor ^= 0x0C;

	return src ^ xxor;
}


static UINT8 decrypt_opcode(int a,int src)
{
    int xxor = 0;

    if (~BIT(a,11) &  BIT(a,9) &  BIT(a,8))                          xxor |= 0x80;
    if (~BIT(a,12) &  BIT(a,11) &  BIT(a,9) &  BIT(a,8) &  BIT(a,6)) xxor |= 0x80;
    if (~BIT(a,12) &  BIT(a,11) & ~BIT(a,6) &  BIT(a,4) &  BIT(a,1)) xxor |= 0x80;
    if (~BIT(a,12) &  BIT(a,11) &  BIT(a,9) &  BIT(a,8) & ~BIT(a,6)) xxor |= 0x40;
    if (~BIT(a,12) &  BIT(a,11) &  BIT(a,6) &  BIT(a,4) &  BIT(a,1)) xxor |= 0x40;
    if ( BIT(a,12) & ~BIT(a,9))                                      xxor |= 0x20;
    if ( BIT(a,12) &  BIT(a,9)  & ~BIT(a,6) &  BIT(a,1))             xxor |= 0x20;
    if (~BIT(a,12) & ~BIT(a,11) & ~BIT(a,6) &  BIT(a,1))             xxor |= 0x10;
    if (~BIT(a,12) &  BIT(a,11) & ~BIT(a,6) &  BIT(a,1))             xxor |= 0x10;
    if ( BIT(a,12) &  BIT(a,9))                                      xxor |= 0x10;
    if ( BIT(a,12) & ~BIT(a,9)  & ~BIT(a,6) &  BIT(a,1))             xxor |= 0x10;
    if (~BIT(a,12) & ~BIT(a,11) & ~BIT(a,4) &  BIT(a,2))             xxor |= 0x08;
    if (~BIT(a,12) & ~BIT(a,11) & ~BIT(a,8) &  BIT(a,4) &  BIT(a,2)) xxor |= 0x08;
    if (~BIT(a,12) &  BIT(a,11) & ~BIT(a,8) &  BIT(a,2))             xxor |= 0x08;
    if (~BIT(a,12) &  BIT(a,11) & ~BIT(a,4) &  BIT(a,2))             xxor |= 0x08;
    if (~BIT(a,12) & ~BIT(a,11) &  BIT(a,8) &  BIT(a,4) &  BIT(a,2)) xxor |= 0x04;
    if (~BIT(a,12) &  BIT(a,11) & ~BIT(a,8) &  BIT(a,1))             xxor |= 0x04;
    if (~BIT(a,12) &  BIT(a,11) &  BIT(a,8) &  BIT(a,4) &  BIT(a,2)) xxor |= 0x04;
    if (~BIT(a,12) & ~BIT(a,11) &  BIT(a,9) &  BIT(a,2))             xxor |= 0x01;
    if ( BIT(a,12) &  BIT(a,9)  &  BIT(a,2))                         xxor |= 0x01;

    if (BIT(a, 8) &  BIT(a,4) & (BIT(src,2) ^ BIT(src,3))) xxor ^= 0x0C;
    if (BIT(a,12) &  BIT(a,9) & (BIT(src,4) ^ BIT(src,5))) xxor ^= 0x30;
    if (BIT(a,11) & ~BIT(a,6) & (BIT(src,6) ^ BIT(src,7))) xxor ^= 0xC0;

    return src ^ xxor;

}


static void seibu_sound_decrypt(int cpu_region,int length)
{
    UINT8 *rom = memory_region(cpu_region);
    int diff =  memory_region_length(cpu_region)/2;
    int i;

    memory_set_opcode_base(cpu_region-REGION_CPU1,rom+diff);

    for (i = 0;i < length;i++)
    {
	UINT8 src = rom[i];

	rom[i]      = decrypt_data(i,src);
	rom[i+diff] = decrypt_opcode(i,src);
    }
}


/***************************************************************************/

static const struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, input_port_0_r },
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ -1 }  /* end of table */
};

static const struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4007, 0x4007, seibu_bank_w },
	{ 0x4008, 0x4008, YM3812_control_port_0_w },
	{ 0x4009, 0x4009, YM3812_write_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

#ifdef __cplusplus
}
#endif
