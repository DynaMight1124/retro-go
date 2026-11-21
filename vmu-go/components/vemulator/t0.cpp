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

#include "t0.h"

VE_VMS_TIMER0::VE_VMS_TIMER0(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_CPU *_cpu, byte *_prescaler)
{
   ram        = _ram;
   intHandler = _intHandler;
   cpu        = _cpu;
   prescaler  = _prescaler;

   TRLStarted = 0;
   TRHStarted = 0;

   TRL_data   = 0;
   TRH_data   = 0;

   T0IN_old   = 0;
}

VE_VMS_TIMER0::~VE_VMS_TIMER0()
{
	
}

void VE_VMS_TIMER0::runTimer() 
{
	int TCNT_data      = ram->readByte_RAW(T0CNT); //Timer control register
	bool TRLEnabled    = (TCNT_data & 64) != 0;
	bool TRHEnabled    = (TCNT_data & 128) != 0;
	bool TRLONGEnabled = (TCNT_data & 32) != 0;
	bool TRLExternal   = (TCNT_data & 16) != 0;

	/* T0L takes either the prescaler or a signal on the connector, which is
	   how one VMU counts pulses from another. T0LEXT picks between them and
	   ISL bit 0 picks the pin: P72 is connector pin 13, P73 is pin 6. T0H is
	   always the prescaler. */
	bool T0LTick;

	if(TRLExternal)
	{
		byte pinMask = (ram->readByte_RAW(ISL) & 1) ? 0x08 : 0x04;
		byte level   = (ram->readByte_RAW(P7) & pinMask) ? 1 : 0;

		T0LTick  = (level != 0) && (T0IN_old == 0);
		T0IN_old = level;
	}
	else
		T0LTick = (*prescaler == 1);

	//Increase timers
	if(TRLEnabled)
	{
		if(TRLStarted == 0)
			TRL_data = ram->readByte_RAW(T0LR);
		else if(T0LTick)
         TRL_data++;
		TRLStarted = 1;
	} 
	else 
	{
		TRL_data = ram->readByte_RAW(T0LR);
		TRLStarted = 0;
	}

	if(TRHEnabled) 
	{
		if(TRHStarted == 0)
			TRH_data = ram->readByte_RAW(T0HR);
		else if(!TRLONGEnabled)
		{
			if(*prescaler == 1)
				TRH_data++;
		}
		
		TRHStarted = 1;
	} 
	else 
	{
		TRH_data = ram->readByte_RAW(T0HR);
		TRHStarted = 0;
	}
	
	//Overflow in TRL_data, 8-bit mode
	if(TRL_data > 255 && !TRLONGEnabled)
	{
		TCNT_data |= 2;

		if (TCNT_data & 1) intHandler->setINT2();

		//Stop timer
		//TCNT_data &= 0xBF;

		//Reload contents
		TRL_data = ram->readByte_RAW(T0LR);
	}
	//Overflow in TRL_data, 16-bit mode
	else if(TRL_data > 255 && TRLONGEnabled)
	{
		/* The carry into the high half, which only counts if it is running.
		   No interrupt: in 16-bit mode the two halves are one counter, and
		   only the high overflow is an event. */
		if(TRHEnabled)
			TRH_data++;

		//Reload contents
		TRL_data = ram->readByte_RAW(T0LR);
	}

	//Overflow in TRH_data, 8-bit mode
	if(TRH_data > 255 && !TRLONGEnabled)
	{
		TCNT_data |= 8;

		if (TCNT_data & 4) intHandler->setT0HOV();

		/* T0H reloads and keeps running; an overflow does not stop it. */
		//Reload contents
		TRH_data = ram->readByte_RAW(T0HR);
	}
	//Overflow in 16-bit mode
	else if(TRH_data > 255 && TRLONGEnabled)
	{
		//printf("T0 LONG overflow 16-bit\n");
		TCNT_data |= 2;
		TCNT_data |= 8;

		if (TCNT_data & 4) intHandler->setT0HOV();

		//Stop both timers
		//TCNT_data &= 0x3F;

		//Reload contents
		TRL_data = ram->readByte_RAW(T0LR);
		TRH_data = ram->readByte_RAW(T0HR);
	}
	
	ram->writeByte_RAW(T0L, TRL_data);
	ram->writeByte_RAW(T0H, TRH_data);

	
	ram->writeByte_RAW(T0CNT, TCNT_data);
}

void VE_VMS_TIMER0::serialize(VE_STATE &s)
{
   s.i32(TRLStarted);
   s.i32(TRHStarted);
   s.f64(TRL_data);
   s.f64(TRH_data);
   s.u8(T0IN_old);
}
