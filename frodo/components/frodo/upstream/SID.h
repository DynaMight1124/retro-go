/*
 *  SID.h - 6581 emulation
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SID_H
#define SID_H

#include <stdlib.h>


class SIDRenderer;
class Prefs;
struct MOS6581State;

// Class for administrative functions
class MOS6581 {
public:
	MOS6581();
	~MOS6581();

	void Reset();
	uint8_t ReadRegister(uint16_t adr);
	void WriteRegister(uint16_t adr, uint8_t byte);
	void NewPrefs(const Prefs * prefs);
	void PauseSound();
	void ResumeSound();
	void GetState(MOS6581State * s) const;
	void SetState(const MOS6581State * s);
	void EmulateLine();

	static const int16_t EGDivTable[16];	// Clock divisors for A/D/R settings
	static const uint8_t EGDRShift[256];	// For exponential approximation of D/R

	const uint16_t * TriSawTable = nullptr;
	const uint16_t * TriRectTable = nullptr;
	const uint16_t * SawRectTable = nullptr;
	const uint16_t * TriSawRectTable = nullptr;

	static const uint16_t TriSawTable_6581[0x1000];
	static const uint16_t TriRectTable_6581[0x1000];
	static const uint16_t SawRectTable_6581[0x1000];
	static const uint16_t TriSawRectTable_6581[0x1000];

	static const uint16_t TriSawTable_8580[0x1000];
	static const uint16_t TriRectTable_8580[0x1000];
	static const uint16_t SawRectTable_8580[0x1000];
	static const uint16_t TriSawRectTable_8580[0x1000];

private:
	void open_close_renderer(int old_type, int new_type);
	void set_wave_tables(int sid_type);

	uint8_t v3_random();
	void update_osc3();
	uint8_t read_osc3();
	uint8_t read_env3() const;

	SIDRenderer *the_renderer;	// Pointer to current renderer

	uint8_t regs[32];			// Copies of the 25 write-only SID registers

	unsigned last_sid_seq;		// SID data bus leakage sequence step (counts down to 0)
	uint16_t last_sid_cycles;	// Remaining cycles for SID data bus leakage sequence step
	uint8_t last_sid_byte;		// Last value on SID data bus

	static uint16_t sid_leakage_cycles[9];
	static uint8_t sid_leakage_mask[9];

	uint32_t fake_v3_update_cycle;	// Cycle of last fake voice 3 oscillator update
	uint32_t fake_v3_count;			// Fake voice 3 phase accumulator for oscillator read-back
	int32_t fake_v3_eg_level;		// Fake voice 3 EG level (8.16 fixed) for EG read-back
	int fake_v3_eg_state;			// Fake voice 3 EG state

	uint32_t v3_random_seed = 1;	// Fake voice 3 noise RNG seed value
};


// Renderers do the actual audio data processing
class SIDRenderer {
public:
	virtual ~SIDRenderer() {}
	virtual void Reset() = 0;
	virtual void EmulateLine() = 0;
	virtual void WriteRegister(uint16_t adr, uint8_t byte) = 0;
	virtual void NewPrefs(const Prefs * prefs) = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
};


// SID state
struct MOS6581State {
	uint8_t freq_lo_1;
	uint8_t freq_hi_1;
	uint8_t pw_lo_1;
	uint8_t pw_hi_1;
	uint8_t ctrl_1;
	uint8_t AD_1;
	uint8_t SR_1;

	uint8_t freq_lo_2;
	uint8_t freq_hi_2;
	uint8_t pw_lo_2;
	uint8_t pw_hi_2;
	uint8_t ctrl_2;
	uint8_t AD_2;
	uint8_t SR_2;

	uint8_t freq_lo_3;
	uint8_t freq_hi_3;
	uint8_t pw_lo_3;
	uint8_t pw_hi_3;
	uint8_t ctrl_3;
	uint8_t AD_3;
	uint8_t SR_3;

	uint8_t fc_lo;
	uint8_t fc_hi;
	uint8_t res_filt;
	uint8_t mode_vol;

	uint8_t pot_x;
	uint8_t pot_y;

	uint32_t v3_update_cycle;
	uint32_t v3_count;
	int32_t v3_eg_level;
	uint32_t v3_eg_state;
	uint32_t v3_random_seed;

	uint16_t last_sid_cycles;
	uint8_t last_sid_seq;
	uint8_t last_sid_byte;
};


// EG states
enum {
	EG_ATTACK,
	EG_DECAY_SUSTAIN,
	EG_RELEASE
};


/*
 *  Simulate voice 3 oscillator and EG for read-back emulation,
 *  and sample master volume for sampled voice reproduction
 */

#ifdef NTSC
constexpr unsigned SID_CYCLES_PER_LINE = 65;  // Clock cycles per raster line (NTSC)
#else
constexpr unsigned SID_CYCLES_PER_LINE = 63;  // Clock cycles per raster line (PAL)
#endif

inline void MOS6581::EmulateLine()
{
	// The actual voice 3 is emulated in calc_buffer() which runs
	// asynchronously from the sound thread. For more consistent results
	// from the OSC3 and ENV3 read-back registers, we run another "fake"
	// emulation of the voice 3 oscillator and EG once per line.

	// Simulate voice 3 envelope generator
	switch (fake_v3_eg_state) {
		case EG_ATTACK:
			fake_v3_eg_level += (SID_CYCLES_PER_LINE << 16) / EGDivTable[regs[0x13] >> 4];
			if (fake_v3_eg_level > 0xffffff) {
				fake_v3_eg_level = 0xffffff;
				fake_v3_eg_state = EG_DECAY_SUSTAIN;
			}
			break;
		case EG_DECAY_SUSTAIN: {
			int32_t s_level = (regs[0x14] >> 4) * 0x111111;
			fake_v3_eg_level -= ((SID_CYCLES_PER_LINE << 16) / EGDivTable[regs[0x13] & 0x0f]) >> EGDRShift[fake_v3_eg_level >> 16];
			if (fake_v3_eg_level < s_level) {
				fake_v3_eg_level = s_level;
			}
			break;
		}
		case EG_RELEASE:
			if (fake_v3_eg_level != 0) {
				fake_v3_eg_level -= ((SID_CYCLES_PER_LINE << 16) / EGDivTable[regs[0x14] & 0x0f]) >> EGDRShift[fake_v3_eg_level >> 16];
				if (fake_v3_eg_level < 0) {
					fake_v3_eg_level = 0;
				}
			}
			break;
	}

	// Simulate internal SID data bus leakage
	if (last_sid_seq > 0) {
		if (last_sid_cycles > SID_CYCLES_PER_LINE) {
			last_sid_cycles -= SID_CYCLES_PER_LINE;
		} else {
			last_sid_byte &= sid_leakage_mask[last_sid_seq];	// Leak one bit, advance sequence
			--last_sid_seq;
			last_sid_cycles = sid_leakage_cycles[last_sid_seq];
		}
	}

	if (the_renderer != nullptr) {
		the_renderer->EmulateLine();
	}
}


/*
 *  Read from register
 */

inline uint8_t MOS6581::ReadRegister(uint16_t adr)
{
	bool start_leakage = false;

	if (adr == 0x19 || adr == 0x1a) {
		last_sid_byte = 0xff;			// A/D converters are not implemented
		start_leakage = true;
	} else if (adr == 0x1b) {
		last_sid_byte = read_osc3();	// Voice 3 oscillator read-back
		start_leakage = true;
	} else if (adr == 0x1c) {
		last_sid_byte = read_env3();	// Voice 3 EG read-back
		start_leakage = true;
	}

	// Start SID data bus leakage sequence?
	if (start_leakage) {
		last_sid_seq = 8;	// 8 bits to leak
		last_sid_cycles = sid_leakage_cycles[last_sid_seq];
	}

	// Return value on SID data bus
	return last_sid_byte;
}


/*
 *  Write to register
 */

inline void MOS6581::WriteRegister(uint16_t adr, uint8_t byte)
{
	// Handle fake voice 3 oscillator
	if (adr == 0x0e || adr == 0x0f || adr == 0x12) {	// Voice 3 frequency or control register
		update_osc3();
	}

	// Handle fake voice 3 EG state
	if (adr == 0x12) {	// Voice 3 control register
		uint8_t gate = byte & 0x01;
		if ((regs[0x12] & 0x01) != gate) {
			if (gate) {		// Gate turned on
				fake_v3_eg_state = EG_ATTACK;
			} else {		// Gate turned off
				fake_v3_eg_state = EG_RELEASE;
			}
		}
	}

	// Keep a local copy of the register values
	regs[adr] = byte;

	// Start SID data bus leakage sequence
	last_sid_byte = byte;
	last_sid_seq = 8;	// 8 bits to leak
	last_sid_cycles = sid_leakage_cycles[last_sid_seq];

	if (the_renderer != nullptr) {
		the_renderer->WriteRegister(adr, byte);
	}
}


#endif // ndef SID_H
