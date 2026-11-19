/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "basetimer.h"

VE_VMS_BASETIMER::VE_VMS_BASETIMER(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_CPU *_cpu)
{
	ram = _ram;
	intHandler = _intHandler;
	cpu = _cpu;
	BTR = 0;
	frac = 0.0;
}

VE_VMS_BASETIMER::~VE_VMS_BASETIMER()
{
}

void VE_VMS_BASETIMER::runTimer() 
{
	int int1cycle;
	int BTCR_data    = ram->readByte_RAW(BTCR);
	bool BTStarted   = (BTCR_data & 64) != 0;
	bool Int0Enabled = (BTCR_data & 1) != 0;
	bool Int1Enabled = (BTCR_data & 4) != 0;
	int cycleControl = ((BTCR_data & 16) >> 4) | ((BTCR_data & 32) >> 4);

	switch (cycleControl)
	{
		case 0:
			int1cycle = 32;
			break;
		case 1:
			int1cycle = 128;
			break;
		case 2:
			int1cycle = 512;
			break;
		case 3:
			int1cycle = 2048;
			break;
		default:
			int1cycle = 2048;
	}

	/* Interrupt 0 fires on overflow of the whole 14-bit counter, twice a
	   second, unless BTCR.7 shortens it to every 64 counts. */
	int int0cycle = ((BTCR_data & 128) != 0) ? 64 : 16384;

	if(!BTStarted)
	{
		BTR  = 0;
		frac = 0.0;
		return;
	}

	/* The base timer is clocked by the 32768Hz quartz, not by the CPU, so a
	   CPU cycle is worth a fraction of a count and the remainder carries. */
	frac += 32768.0 / cpu->getCurrentFrequency();

	while(frac >= 1.0)
	{
		frac -= 1.0;
		BTR = (BTR + 1) & 0x3FFF;

		/* Both source flags are set once per period, on the count that
		   completes it, and are cleared by software afterwards. Setting one
		   for as long as the count is past its period instead leaves it
		   permanently set as far as a handler that reads it back can tell. */
		if((BTR % (unsigned)int1cycle) == 0)
		{
			BTCR_data |= 8;
			ram->writeByte_RAW(BTCR, BTCR_data);

			if(Int1Enabled)
				intHandler->setINT3();
		}

		if((BTR % (unsigned)int0cycle) == 0)
		{
			BTCR_data |= 2;
			ram->writeByte_RAW(BTCR, BTCR_data);

			if(Int0Enabled)
				intHandler->setINT3();
		}
	}
}

void VE_VMS_BASETIMER::serialize(VE_STATE &s)
{
   s.u32(BTR);
   s.f64(frac);
}
