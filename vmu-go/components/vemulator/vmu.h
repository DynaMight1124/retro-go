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

#ifndef _VMU_H_
#define _VMU_H_

#include <time.h>
#include "common.h"
#include "ram.h"
#include "cpu.h"
#include "video.h"
#include "rom.h"
#include "audio.h"
#include "t0.h"
#include "t1.h"
#include "basetimer.h"
#include "interrupts.h"
#include "serial.h"
#include "bitwisemath.h"
#include "state.h"

class VMU
{
public:
	VE_VMS_RAM *ram;
	VE_VMS_ROM *rom;
	VE_VMS_FLASH *flash;
	VE_VMS_CPU *cpu;
	VE_VMS_TIMER0 *t0;
	VE_VMS_TIMER1 *t1;
	VE_VMS_BASETIMER *baseTimer;
	VE_VMS_INTERRUPTS *intHandler;
	VE_VMS_VIDEO *video;
	VE_VMS_AUDIO *audio;
	VE_VMS_SERIAL *serial;

    VMU(uint16_t *_frameBuffer);
    
    ~VMU();

    /* The machine's whole state, in one pass. serializeSize() counts it,
       saveState() writes it and loadState() reads it back; all three name the
       same fields through VMU::serialize, so none can drift from the others. */
    size_t serializeSize();
    bool   saveState(void *buffer, size_t size);
    bool   loadState(const void *buffer, size_t size);
    void   serialize(VE_STATE &s);
    

    int loadBIOS(const char *filePath);

    bool hasBIOS();

    /* Connector pins 13 and 6 (P7 bits 2 and 3): how the machine sees that
       another VMU is clipped on. Software checks this before it touches the
       serial port. */
    byte linkConnectorBits;

    void halt();

    void setDate();

    /* Seed setDate() with FIXED_DATE instead of the host clock. Read at load,
       before startCPU. */
    bool fixedClock;

    //Sets system variables in RAM
    void initBIOS();

    void startCPU();

    void initializeHLE();

    void runCycle();
    
    void reset();
    
private:
    int ccount;
    int64_t cycle_count;
    int64_t time_reg;
    int64_t frame_skip;
    double CPS;
    byte prescaler;
    int pcount;
    int oldPRR;

    int OSC;
    int OCR_old;
    bool threadReady;
    bool inSleepState;
    bool BIOSExists;
    bool enableSound;
    bool useT1ELD;
    
    int cycles_left; //This counts how many cycles an instruction has, and gets decreased each cycle. Next instruction is processed when it gets 0.
    
    uint16_t *frameBuffer;
};

#endif // _VMU_H_
