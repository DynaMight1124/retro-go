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

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <math.h>
#include <libretro.h>
#include "common.h"
#include "cpu.h"
#include "ram.h"
#include "state.h"

/* The VMU's sound is one square wave: timer 1's low half free-runs, T1LR sets
   the period and T1LC the point in it where the output flips, and the pin
   drives a piezo. There is no volume and no second voice. */
class VE_VMS_AUDIO
{
public:
    VE_VMS_AUDIO(VE_VMS_CPU *_cpu, VE_VMS_RAM *_ram);

    ~VE_VMS_AUDIO();

    void serialize(VE_STATE &s);

    /* One frame of audio, always: a frame of silence still has to be handed
       over, or the frontend starves. */
    void generateSignal(retro_audio_sample_batch_t &audio_batch_cb);

    void setAudioFrequency(double f);

    /* The shape of the pulse timer 1 is generating, straight from its
       registers. `longMode` is the 9-to-16 bit generator, where T1HR sets how
       many 8-bit intervals make up the long one and T1HC lengthens the low
       time of that many of them by one count. `doubleRate` is the half-Tcyc
       clock, which ticks twice per cycle. */
    void setPulse(int lr, int lc, int hr, int hc, bool longMode, bool doubleRate);

    void setEnabled(bool e);

private:
	int T1LR_reg;
	int T1LC_reg;
	int T1HR_reg;
	int T1HC_reg;
	bool LongMode;
	bool DoubleRate;
	bool IsEnabled;

	double frequency;	//The clock timer 1 counts, which follows OCR

	double phase;		//carried across frames, so the wave has no step in it

	/* Which of the long generator's small intervals the wave is in, for
	   placing the extra low ticks T1HC asks for. */
	int interval;

	double sampleDebt;	//SAMPLE_RATE / FPS is not a whole number

	int16_t *sampleArray;

	VE_VMS_CPU *cpu;
	VE_VMS_RAM *ram;
};

#endif // _AUDIO_H_
