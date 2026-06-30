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

	Seibu Sound System v1.02, designed 1986 by Seibu Kaihatsu

	The Seibu sound system comprises of a Z80A, a YM3812, a YM3931*, and
	an Oki MSM6205.  As well as sound the Z80 can controls coins and pass
	data to the main cpu.  There are a few little quirks that make it
	worthwhile emulating in a seperate file:

	* The YM3812 generates interrupt RST10, by asserting the interrupt line,
	and placing 0xd7 on the data bus.

	* The main cpu generates interrupt RST18, by asserting the interrupt line,
	and placing 0xdf on the data bus.

	A problem can occur if both the YM3812 and the main cpu try to assert
	the interrupt line at the same time.  The effect in the old Mame
	emulation would be for sound to stop playing - this is because a RST18
	cancelled out a RST10, and if a single RST10 is dropped sound stops
	as the YM3812 timer is not reset.  The problem occurs because even
	if both interrupts happen at the same time, there can only be one value
	on the data bus.  Obviously the real hardware must have some circuit
	to prevent this.  It is emulated by user timers to control the z80
	interrupt vector.

	(*What on earth _is_ the YM3931??  There are no unknown memory writes)

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "sndhrdw/seibu.h"

static int sound_cpu;
unsigned char *seibu_shared_sound_ram;

/***************************************************************************/

enum
{
	VECTOR_INIT,
	RST10_ASSERT,
	RST10_CLEAR,
	RST18_ASSERT,
	RST18_CLEAR
};

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
	if ((irq1 & irq2) == 0xff)	/* no IRQs pending */
		cpu_set_irq_line(sound_cpu,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpu_set_irq_line(sound_cpu,0,ASSERT_LINE);
}

WRITE_HANDLER( seibu_rst10_ack_w )
{
	/* Unused for now */
}

WRITE_HANDLER( seibu_rst18_ack_w )
{
	timer_set(TIME_NOW,RST18_CLEAR,setvector_callback);
}

void seibu_ym3812_irqhandler(int linestate)
{
	if (linestate)
		timer_set(TIME_NOW,RST10_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,RST10_CLEAR,setvector_callback);
}

/***************************************************************************/

/* Use this if the sound cpu is cpu 1 */
void seibu_sound_init_1(void)
{
	sound_cpu=1;
	setvector_callback(VECTOR_INIT);
}

/* Use this if the sound cpu is cpu 2 */
void seibu_sound_init_2(void)
{
	sound_cpu=2;
	setvector_callback(VECTOR_INIT);
}

/***************************************************************************/

WRITE_HANDLER( seibu_bank_w )
{
	unsigned char *RAM;

	if (sound_cpu==1) RAM = memory_region(REGION_CPU2);
	else RAM = memory_region(REGION_CPU3);

	if (data&1) { cpu_setbank(1,&RAM[0x0000]); }
	else { cpu_setbank(1,&RAM[0x10000]); }
}

READ_HANDLER( seibu_soundlatch_r )
{
	return seibu_shared_sound_ram[offset<<1];
}

WRITE_HANDLER( seibu_soundclear_w )
{
	seibu_shared_sound_ram[0]=data;
}

WRITE_HANDLER( seibu_soundlatch_w )
{
	seibu_shared_sound_ram[offset]=data;
	if (offset==0xc && seibu_shared_sound_ram[0]!=0)
		timer_set(TIME_NOW,RST18_ASSERT,setvector_callback);
}

WRITE_HANDLER( seibu_main_data_w )
{
	seibu_shared_sound_ram[offset<<1]=data;
}

/***************************************************************************/

static READ_HANDLER( sound_cpu_spin_r )
{
	unsigned char *RAM;

	if (sound_cpu==1) RAM = memory_region(REGION_CPU2);
	else RAM = memory_region(REGION_CPU3);

	if (cpu_get_pc()==0x129 && RAM[0x201c]==0)
		cpu_spinuntil_int();

	return RAM[0x201c+offset];
}

void install_seibu_sound_speedup(int cpu)
{
	install_mem_read_handler(cpu, 0x201c, 0x201d, sound_cpu_spin_r);
}

/***************************************************************************/

/* NOTICE!  This is not currently correct, this table works for the first
128 bytes, but goes wrong after that.  I suspect the bytes in this table
are shifted according to an address line.  I have not confirmed the pattern
repeats after 128 bytes, it may be more...

There is also a 0xff fill at the end of the rom.

*/

/* Game using encrypted sound cpu - Raiden, Dynamite Duke, Dead Angle */
#if 0
void seibu_sound_decrypt(void)
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int xor_table[128]={
		0x00,0x00,0x10,0x10,0x08,0x00,0x00,0x18,
		0x00,0x00,0x10,0x10,0x08,0x08,0x18,0x18,

		0x00,0x00,0x00,0x10,0x08,0x08,0x18,0x18,
		0x00,0x00,0x00,0x10,0x08,0x08,0x18,0x18,

		0x00,0x00,0x10,0x10,0x08,0x08,0x18,0x18,
		0x00,0x00,0x10,0x10,0x08,0x08,0x18,0x18,

		0x00,0x00,0x10,0x10,0x08,0x08,0x18,0x18,
		0x00,0x00,0x10,0x10,0x08,0x08,0x18,0x18,

		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,
		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,

		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,
		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,

		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,
		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,

		0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x08,
		0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08,
	};
	int i;

	for (i=0; i<0x18000; i++)
		RAM[i]=RAM[i]^xor_table[i%128];
}
#endif

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
	       ((BIT(val,B7) << 7) | \
		(BIT(val,B6) << 6) | \
		(BIT(val,B5) << 5) | \
		(BIT(val,B4) << 4) | \
		(BIT(val,B3) << 3) | \
		(BIT(val,B2) << 2) | \
		(BIT(val,B1) << 1) | \
		(BIT(val,B0) << 0))


static UINT8 decrypt_data(int a,int src)
{
    if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
    if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
    if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
    if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
    if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

    if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
    if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);

    return src;
}

static UINT8 decrypt_opcode(int a,int src)
{
    if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
    if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
    if (~BIT(a,13) & BIT(a,12))             src ^= 0x20;
    if (~BIT(a,6)  &  BIT(a,1))             src ^= 0x10;
    if (~BIT(a,12) &  BIT(a,2))             src ^= 0x08;
    if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
    if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
    if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

    if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
    if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);
    if (BIT(a,12) &  BIT(a,9)) src = BITSWAP8(src,7,6,4,5,3,2,1,0);
    if (BIT(a,11) & ~BIT(a,6)) src = BITSWAP8(src,6,7,5,4,3,2,1,0);

    return src;
}


void seibu_sound_decrypt(int cpu_region,int length)
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

void seibu_adpcm_decrypt(int region)
{
    data8_t *ROM = memory_region(region);
    int i;

    for (i = 0; i < memory_region_length(region); i++)
    {
	ROM[i] = BITSWAP8(ROM[i], 7, 5, 3, 1, 6, 4, 2, 0);
    }
}

#ifdef __cplusplus
}
#endif
