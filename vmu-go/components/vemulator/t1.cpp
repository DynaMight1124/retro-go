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

#include "t1.h"

VE_VMS_TIMER1::VE_VMS_TIMER1(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_AUDIO *_audio)
{
   ram        = _ram;
   intHandler = _intHandler;
   audio      = _audio;

   TRLStarted = 0;
   TRHStarted = 0;
}

VE_VMS_TIMER1::~VE_VMS_TIMER1()
{
}

void VE_VMS_TIMER1::runTimer()
{
	int TCNT_data      = ram->readByte_RAW(T1CNT); //Timer control register
	bool TRLEnabled    = (TCNT_data & 64) != 0;
	bool TRHEnabled    = (TCNT_data & 128) != 0;
	bool TRLONGEnabled = (TCNT_data & 32) != 0;

	//Increase timers
	if(TRLEnabled) 
	{
		if(TRLStarted == 0)
			ram->T1RL_data = ram->readByte_RAW(T1LR);

		ram->T1RL_data++;
		if(TRLONGEnabled && !TRHEnabled) ram->T1RL_data++; //Tcyc/2, equivalent to ram->T1RL_data += 2;

		TRLStarted = 1;
	} 
	else 
	{
		ram->T1RL_data = ram->readByte_RAW(T1LR);
		TRLStarted = 0;
	}

	/* The buzzer only sounds when timer 1's pulse output is routed to P17,
	   which takes P17FCR and P17DDR both set. Timer 1 used as a plain timer
	   drives nothing. Both generators reach the pin -- the 8-bit one with
	   T1LONG clear, the 9-to-16 bit one with it set -- and the long one
	   clocks at half Tcyc, twice per cycle, when T1H is not running. */
	{
		bool pulseToPin = (ram->readByte_RAW(P1FCR) & 0x80) != 0
		               && (ram->readByte_RAW(P1DDR) & 0x80) != 0;

		audio->setPulse(ram->readByte_RAW(T1LR), ram->readByte_RAW(T1LC),
		                ram->readByte_RAW(T1HR), ram->readByte_RAW(T1HC),
		                TRLONGEnabled, TRLONGEnabled && !TRHEnabled);

		audio->setEnabled(TRLEnabled && pulseToPin);
	}

	/* The comparator is loaded from T1LC/T1HC when ELDT1C is set, but not
	   there and then: it waits for the next overflow -- T1L's in 8-bit mode,
	   T1H's in 16-bit -- so a write lands on an interval boundary rather than
	   part way through one. While the timer is stopped it loads straight
	   away. */
	if(!TRLEnabled && (TCNT_data & 16) != 0)
	{
		ram->writeByte_RAW(T1LC, ram->T1LC_Temp);
		ram->writeByte_RAW(T1HC, ram->T1HC_Temp);
	}



	if(TRHEnabled) 
	{
		TRHStarted = 1;
		if(!TRLONGEnabled) ram->T1RH_data++;
	} 
	else 
	{
		ram->T1RH_data = ram->readByte_RAW(T1HR);
		TRHStarted = 0;
	}

	//Overflow in ram->T1RL_data, 8-bit mode
	if(ram->T1RL_data > 255 && !TRLONGEnabled)
	{
		TCNT_data |= 2;

		if ((TCNT_data & 1) != 0) intHandler->setT1HLOV();

		//Reload contents
		ram->T1RL_data = ram->readByte_RAW(T1LR);

		//The overflow ELDT1C has been waiting for
		if((TCNT_data & 16) != 0)
		{
			ram->writeByte_RAW(T1LC, ram->T1LC_Temp);
			ram->writeByte_RAW(T1HC, ram->T1HC_Temp);
		}
	}
	//Overflow in ram->T1RL_data, 16-bit mode
	else if(ram->T1RL_data > 255 && TRLONGEnabled)
	{
		/* Timer 1 sets its low flag on every T1L overflow whatever the bit
		   length. Timer 0 does not: its low flag stays clear in 16-bit mode.
		   The two really do differ here. */
		TCNT_data |= 2;

		if ((TCNT_data & 1) != 0) intHandler->setT1HLOV();

		//The carry into the high half, which only counts if it is running
		if(TRHEnabled)
			ram->T1RH_data++;

		//Reload contents
		ram->T1RL_data = ram->readByte_RAW(T1LR);
	}

	//Overflow in ram->T1RH_data, 8-bit mode
	if(ram->T1RH_data > 255 && !TRLONGEnabled)
	{
		TCNT_data |= 8;

		if ((TCNT_data & 4) != 0) intHandler->setT1HLOV();

		//Stop timer
		//TCNT_data &= 0x7F;

		//Reload contents
		ram->T1RH_data = ram->readByte_RAW(T1HR);
	}

	//Overflow in 16-bit mode
	else if(ram->T1RH_data > 255 && TRLONGEnabled)
	{
		TCNT_data |= 2;
		TCNT_data |= 8;

		if ((TCNT_data & 4) != 0) intHandler->setT1HLOV();

		//The overflow ELDT1C waits for when the long generator is running
		if((TCNT_data & 16) != 0)
		{
			ram->writeByte_RAW(T1LC, ram->T1LC_Temp);
			ram->writeByte_RAW(T1HC, ram->T1HC_Temp);
		}

		//Reload contents
		ram->T1RL_data = ram->readByte_RAW(T1LR);
		ram->T1RH_data = ram->readByte_RAW(T1HR);
	}

	ram->writeByte_RAW(T1CNT, TCNT_data);
}

void VE_VMS_TIMER1::serialize(VE_STATE &s)
{
   s.i32(TRLStarted);
   s.i32(TRHStarted);
}
