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

#ifndef _T1_H_
#define _T1_H_

#include "audio.h"
#include "ram.h"
#include "interrupts.h"
#include "state.h"

class VE_VMS_TIMER1
{
public:
    VE_VMS_TIMER1(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler, VE_VMS_AUDIO *_audio);
    ~VE_VMS_TIMER1();

    void serialize(VE_STATE &s);

    void runTimer();
    
private:
    int TRLStarted;
    int TRHStarted;
    
    //The counters in T1 are implicit (Not visible to the programmer)
    VE_VMS_RAM *ram;
    VE_VMS_INTERRUPTS *intHandler;
    VE_VMS_AUDIO *audio;
};

#endif // _T1_H_
