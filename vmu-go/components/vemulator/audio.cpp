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

#include <string.h>

#include "audio.h"

VE_VMS_AUDIO::VE_VMS_AUDIO(VE_VMS_CPU *_cpu, VE_VMS_RAM *_ram)
{
	T1LR_reg = 0;
	T1LC_reg = 128;
	T1HR_reg = 0;
	T1HC_reg = 0;
	LongMode = false;
	DoubleRate = false;
	IsEnabled = false;
	phase = 0.0;
	interval = 0;
	sampleDebt = 0.0;

	ram = _ram;
	cpu = _cpu;

	sampleArray = (int16_t *)calloc(2*((SAMPLE_RATE + FPS - 1)/FPS), sizeof(int16_t));	//Multiplied by 2 because 2 channels

	//Until OCR is written: the 879236Hz main clock over the 6 timer 1 counts in
	frequency = 146539.3;
}

VE_VMS_AUDIO::~VE_VMS_AUDIO()
{
	free(sampleArray);
}

/* Centred on zero: a square wave with a DC offset thumps every time the sound
   starts or stops. The peak to peak is the same either way. */
#define AUDIO_HIGH  16383
#define AUDIO_LOW  (-16384)

void VE_VMS_AUDIO::generateSignal(retro_audio_sample_batch_t &audio_batch_cb)
{
	/* The declared rate over the declared frame rate, fraction carried. */
	int count;
	int i;

	sampleDebt += (double)SAMPLE_RATE / (double)FPS;
	count = (int)sampleDebt;
	sampleDebt -= count;

	if(count <= 0)
		return;

	/* The period of the wave in samples, and how much of it the output spends
	   low. T1LR counts up to 256, so 256 - T1LR is the period in timer ticks;
	   T1LC is where in that period the output flips.
	 *
	 *   cycle           = (256 - T1LR) x Ttc
	 *   "L" pulse width = (T1LC - T1LR) x Ttc
	 *
	 * The long generator repeats that interval (256 - T1HR) times and spends
	 * T1HC extra ticks low across the whole repetition, one interval at a
	 * time, so that
	 *
	 *   total "L" width = ((256 - T1HR) x (T1LC - T1LR) + T1HC) x Ttc
	 *
	 * Which interval an extra tick lands in matters to the waveform, so they
	 * are placed rather than averaged: the first T1HC of them are one tick
	 * longer. `intervals` is how many make up the long one, and `interval` is
	 * where in it the wave currently stands. */
	double period = 0.0;
	double lowTicks = 0.0;
	double tickFraction = 0.0;
	int    intervals = 1;

	if(IsEnabled && T1LR_reg < 256)
	{
		double ticksPerCycle = DoubleRate ? 2.0 : 1.0;

		period = (256.0 - T1LR_reg)
		       * ((double)SAMPLE_RATE / (frequency * ticksPerCycle));

		lowTicks     = T1LC_reg - T1LR_reg;
		tickFraction = 1.0 / (256.0 - T1LR_reg);

		if(LongMode && T1HR_reg < 256)
			intervals = 256 - T1HR_reg;
	}

	if(interval >= intervals)
		interval = 0;

	//Above what this sample rate can carry, so there is no wave to draw
	if(period < 2.0)
	{
		memset(sampleArray, 0, count * 2 * sizeof(int16_t));
		phase = 0.0;
		interval = 0;
		audio_batch_cb(sampleArray, count);
		return;
	}

	{
		double step = 1.0 / period;

		for(i = 0; i < count; i++)
		{
			/* This interval's low time: one tick longer for the first T1HC of
			   them, which is how the long generator spends its extra ticks. */
			double lowHere = lowTicks;
			int16_t amplitude;

			if(interval < T1HC_reg)
				lowHere += 1.0;

			amplitude = (phase < lowHere * tickFraction) ? AUDIO_LOW : AUDIO_HIGH;

			sampleArray[i * 2]     = amplitude;
			sampleArray[i * 2 + 1] = amplitude;

			phase += step;
			while(phase >= 1.0)
			{
				phase -= 1.0;
				if(++interval >= intervals)
					interval = 0;
			}
		}
	}

	audio_batch_cb(sampleArray, count);
}

void VE_VMS_AUDIO::setAudioFrequency(double f)
{
	frequency = f;
}

void VE_VMS_AUDIO::setPulse(int lr, int lc, int hr, int hc,
                            bool longMode, bool doubleRate)
{
	T1LR_reg   = lr & 0xFF;
	T1LC_reg   = lc & 0xFF;
	T1HR_reg   = hr & 0xFF;
	T1HC_reg   = hc & 0xFF;
	LongMode   = longMode;
	DoubleRate = doubleRate;
}

void VE_VMS_AUDIO::setEnabled(bool e)
{
	//So the first cycle of a note is a whole one
	if(e && !IsEnabled)
	{
		phase = 0.0;
		interval = 0;
	}

	IsEnabled = e;
}

void VE_VMS_AUDIO::serialize(VE_STATE &s)
{
   s.i32(T1LR_reg);
   s.i32(T1LC_reg);
   s.i32(T1HR_reg);
   s.i32(T1HC_reg);
   s.b(LongMode);
   s.b(DoubleRate);
   s.b(IsEnabled);
   s.f64(frequency);
   s.f64(phase);
   s.f64(sampleDebt);
   s.i32(interval);
}
