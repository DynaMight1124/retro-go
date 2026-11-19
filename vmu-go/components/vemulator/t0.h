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

#ifndef _T0_H_
#define _T0_H_

#include "ram.h"
#include "interrupts.h"
#include "cpu.h"
#include "state.h"

class VE_VMS_TIMER0
{
public:
    VE_VMS_TIMER0(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_CPU *_cpu, byte *_prescaler);
    ~VE_VMS_TIMER0();

    void serialize(VE_STATE &s);

    void runTimer();
    
private:
	int TRLStarted;
    int TRHStarted;
    VE_VMS_RAM *ram;
    VE_VMS_INTERRUPTS *intHandler;
    VE_VMS_CPU *cpu;
    byte *prescaler;
    
    double TRL_data;
	double TRH_data;

	/* Last level seen on the T0IN pin, for counting edges when T0L is clocked
	   externally rather than from the prescaler. */
	byte T0IN_old;
};

#endif // _T0_H_
