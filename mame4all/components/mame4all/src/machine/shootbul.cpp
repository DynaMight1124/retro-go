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
#include "driver.h"
#include "vidhrdw/generic.h"




static unsigned char decrypt_shootbul1(unsigned char e)
{
	return e;
}

static unsigned char decrypt_shootbul2(unsigned char e)
{
	unsigned char d;

	d  = ((e) & 0x80) >> 4; // 08
	d |= ((e) & 0x40) >> 6; // 01
	d |= ((~e) & 0x20) << 2; // 80
	d |= ((~e) & 0x10) << 1; // 20
	d |= ((e) & 0x08) << 1; // 10
	d |= ((e) & 0x04) >> 1; // 02
	d |= ((~e) & 0x02) << 1; // 04
	d |= ((e) & 0x01) << 6; // 40

	return d;
}

static unsigned char decrypt_shootbul3(unsigned char e)
{
	unsigned char d;

	d  = ((e) & 0x80) >> 3; //10
	d |= ((~e) & 0x40) >> 4; //04
	d |= ((e) & 0x20) >> 5; //01
	d |= ((~e) & 0x10) >> 1; //08
	d |= ((~e) & 0x08) << 3; //40
	d |= ((e) & 0x04) << 5;  //80
	d |= ((~e) & 0x02) << 4; //20
	d |= ((~e) & 0x01) << 1; // 02

	return d;
}

static unsigned char decrypt_shootbul4(unsigned char e)
{
	unsigned char d;

	d  = (e & 0x80) >> 0;
	d |= (e & 0x40) >> 0;
	d |= ((e) & 0x20) >> 2; // 08
	d |= (e & 0x10) >> 0;   // 10
	d |= ((~e) & 0x08) << 2; // 20
	d |= (e & 0x04) >> 0;  //04
	d |= (e & 0x02) >> 0;  //02
	d |= (e & 0x01) >> 0;  //01

	return d;
}

static unsigned char decrypt_shootbul5(unsigned char e)
{
	unsigned char d;

	d  = ((~e) & 0x80) >> 4; // 08
	d |= ((e) & 0x40) >> 6; // 01
	d |= ((~e) & 0x20) << 2; // 80
	d |= ((e) & 0x10) << 1; // 20
	d |= ((e) & 0x08) << 1; // 10
	d |= ((e) & 0x04) >> 1; // 02
	d |= ((~e) & 0x02) << 1; //04
	d |= ((e) & 0x01) << 6; // 40

	return d;
}

static unsigned char decrypt_shootbul6(unsigned char e)
{
	unsigned char d;


	d  = ((e) & 0x80) >> 3; //  10
	d |= ((~e) & 0x40) >> 4; // 04
	d |= ((e) & 0x20) >> 5; // 01
	d |= ((e) & 0x10) << 1; //  20
	d |= ((~e) & 0x08) << 3; //  40
	d |= ((e) & 0x04) << 5;  //  80
	d |= ((~e) & 0x02) << 2; //  08
	d |= ((~e) & 0x01) << 1; //  02

	return d;
}

static unsigned char decrypt_shootbul(int addr, unsigned char e)
{
	unsigned int method = 1;

	switch (addr & 0x2A5)
	{
		case 0x001:
		case 0x021:
		case 0x025:
		case 0x080:
		case 0x084:
		case 0x0a1:
		case 0x0a5:
		case 0x201:
		case 0x221:
		case 0x225:
		case 0x2a1:
		case 0x2a5:
			method = 2;
			break;

		case 0x004:
		case 0x005:
		case 0x020:
		case 0x085:
		case 0x0a0:
        case 0x200:
		case 0x204:
		case 0x220:
		case 0x224:
		case 0x281:
		case 0x285:
		case 0x2a0:
		case 0x2a4:
			method = 3;
			break;

	}

	if ((addr & 0x200) && !(addr & 0x800))
		method = method + 3;
		if (!(addr & 0x200) && (addr & 0x800))
		method = method + 3;

 if (addr==0x200) printf("%d\n",method);
	switch (method)
	{
		case 1:		return decrypt_shootbul1(e);
		case 2:		return decrypt_shootbul2(e);
		case 3:		return decrypt_shootbul3(e);
		case 4:		return decrypt_shootbul4(e);
		case 5:		return decrypt_shootbul5(e);
		case 6:		return decrypt_shootbul6(e);
	}

	return 0;
}





void shootbul_decode(void)
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	RAM = memory_region(REGION_CPU1);
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] = decrypt_shootbul(i,RAM[i]);
	}
}



#ifdef __cplusplus
}
#endif
