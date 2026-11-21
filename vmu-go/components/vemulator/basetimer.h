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

#ifndef _BASETIMER_H_
#define _BASETIMER_H_

#include "ram.h"
#include "interrupts.h"
#include "cpu.h"
#include "state.h"

class VE_VMS_BASETIMER
{
public:
    VE_VMS_BASETIMER(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_CPU *_cpu);
    ~VE_VMS_BASETIMER();

    void serialize(VE_STATE &s);

    void runTimer();
    
private:
	unsigned int BTR;   //14-bit counter, clocked at 32768Hz
	double       frac;  //fraction of a base timer tick carried between cycles
	VE_VMS_RAM *ram;
	VE_VMS_INTERRUPTS *intHandler;
	VE_VMS_CPU *cpu;
};

#endif // _BASETIMER_H_
