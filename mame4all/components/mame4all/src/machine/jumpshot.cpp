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


static unsigned char decrypt_jumpshot1(unsigned char e)
{
	return e;
}

static unsigned char decrypt_jumpshot2(unsigned char e)
{
	unsigned char d;

	d  = (e & 0x80) >> 3;
	d |= ((~e) & 0x40) >> 4;
	d |= (e & 0x20) >> 5;
	d |= ((~e) & 0x10) >> 1;
	d |= ((~e) & 0x08) << 3;
	d |= (e & 0x04) << 5;
	d |= ((~e) & 0x02) << 4;
	d |= ((~e) & 0x01) << 1;

	return d;
}

static unsigned char decrypt_jumpshot3(unsigned char e)
{
	unsigned char d;

	d  = (e & 0x80) >> 4;
	d |= (e & 0x40) >> 6;
	d |= ((~e) & 0x20) << 2;
	d |= ((~e) & 0x10) << 1;
	d |= (e & 0x08) << 1;
	d |= (e & 0x04) >> 1;
	d |= ((~e) & 0x02) << 1;
	d |= (e & 0x01) << 6;

	return d;
}

static unsigned char decrypt_jumpshot4(unsigned char e)
{
	unsigned char d;

	d  = (e & 0x80) >> 0;
	d |= (e & 0x40) >> 0;
	d |= (e & 0x20) >> 2;
	d |= (e & 0x10) >> 0;
	d |= ((~e) & 0x08) << 2;
	d |= (e & 0x04) >> 0;
	d |= (e & 0x02) >> 0;
	d |= (e & 0x01) >> 0;

	return d;
}

static unsigned char decrypt_jumpshot5(unsigned char e)
{
	unsigned char d;

	d  = (e & 0x80) >> 3;
	d |= ((~e) & 0x40) >> 4;
	d |= (e & 0x20) >> 5;
	d |= (e & 0x10) << 1;
	d |= ((~e) & 0x08) << 3;
	d |= (e & 0x04) << 5;
	d |= ((~e)  & 0x02) << 2;
	d |= ((~e) & 0x01) << 1;

	return d;
}

static unsigned char decrypt_jumpshot6(unsigned char e)
{
	unsigned char d;

	d  = ((~e) & 0x80) >> 4;
	d |= (e & 0x40) >> 6;
	d |= ((~e) & 0x20) << 2;
	d |= (e & 0x10) << 1;
	d |= (e & 0x08) << 1;
	d |= (e & 0x04) >> 1;
	d |= ((~e) & 0x02) <<1;
	d |= (e & 0x01) << 6;

	return d;
}

static unsigned char decrypt_jumpshot(int addr, unsigned char e)
{
	unsigned int method = 1;

	switch (addr & 0x2A5)
	{
		case 0x004:
		case 0x005:
		case 0x020:
		case 0x085:
		case 0x0A0:
			method = 2;
			break;
		case 0x001:
		case 0x021:
		case 0x025:
		case 0x080:
		case 0x084:
		case 0x0A1:
		case 0x0A5:
			method = 3;
			break;
		case 0x205:
		case 0x280:
		case 0x284:
			method = 4;
			break;
		case 0x200:
		case 0x204:
		case 0x220:
		case 0x224:
		case 0x281:
		case 0x285:
		case 0x2A0:
		case 0x2A4:
			method = 5;
			break;
		case 0x201:
		case 0x221:
		case 0x225:
		case 0x2A1:
		case 0x2A5:
			method = 6;
			break;







	}

	if ((addr & 0x800) == 0x800)
		method = method + 3;
	if (method > 6) method=method-6;

	switch (method)
	{
		case 1:		return decrypt_jumpshot1(e);
		case 2:		return decrypt_jumpshot2(e);
		case 3:		return decrypt_jumpshot3(e);
		case 4:		return decrypt_jumpshot4(e);
		case 5:		return decrypt_jumpshot5(e);
		case 6:		return decrypt_jumpshot6(e);
	}

	return 0;
}


void jumpshot_decode(void)
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	RAM = memory_region(REGION_CPU1);
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] = decrypt_jumpshot(i,RAM[i]);
	}
}



#ifdef __cplusplus
}
#endif
