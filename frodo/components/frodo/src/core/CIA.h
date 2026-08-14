/*
 *  CIA.h - 6526 emulation
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

#ifndef CIA_H
#define CIA_H

#include "Prefs.h"


class MOS6510;
class MOS6502_1541;
class MOS6569;
struct MOS6526State;


// 6526 emulation (CIA) base class
class MOS6526 {
public:
	MOS6526(MOS6510 * cpu) : the_cpu(cpu) { }
	virtual ~MOS6526() { }

	void Reset();

	void GetState(MOS6526State * s) const;
	virtual void SetState(const MOS6526State * s);

#ifdef FRODO_SC
	void EmulateCycle();
#else
	void EmulateLine(int cycles);
#endif
	void CountTOD();

	void SetPAIn(uint8_t byte) { pa_in = byte; }
	void SetPBIn(uint8_t byte) { pb_in = byte; }
	uint8_t PAOut() const { return pra | ~ddra; }
	uint8_t PBOut() const { return prb | ~ddrb; }

	void TriggerFlagLine();

protected:
	uint8_t read_register(uint8_t reg);
	void write_register(uint8_t reg, uint8_t byte);

	void set_int_flag(uint8_t flag);
	virtual void trigger_irq() = 0;
	virtual void clear_irq() = 0;

	void check_tod_alarm();

	MOS6510 * the_cpu;	// Pointer to CPU object

	// Registers
	uint8_t pra, prb, ddra, ddrb;

	uint8_t tod_10ths, tod_sec, tod_min, tod_hr;	// TOD counter
	uint8_t ltc_10ths, ltc_sec, ltc_min, ltc_hr;	// TOD latch
	uint8_t alm_10ths, alm_sec, alm_min, alm_hr;	// TOD alarm

	uint8_t sdr, icr, cra, crb;
	uint8_t int_mask;

	// Timer state
	struct Timer {
		uint16_t counter;
		uint16_t latch;

		bool pb_toggle;			// Timer output to PB toggle state

#ifdef FRODO_SC
		bool idle;				// Flag: Timer idle
		bool output;			// Timer output state
		uint8_t count_delay;	// Delay line for counter input
		uint8_t load_delay;		// Delay line for counter load
		uint8_t oneshot_delay;	// Delay line for one-shot state
#endif
	};

	Timer ta, tb;

#ifdef FRODO_SC
	void emulate_timer(Timer & t, uint8_t & cr, bool input);
#endif
	uint8_t timer_on_pb(uint8_t prb) const;

	// TOD state
	unsigned tod_counter;	// TOD frequency counter
	bool tod_halted;		// Flag: TOD halted
	bool tod_latched;		// Flag: TOD latched
	bool tod_alarm;			// Flag: TOD in alarm state

	// Input lines for ports
	uint8_t pa_in = 0;
	uint8_t pb_in = 0;

#ifdef FRODO_SC
	unsigned sdr_shift_counter;	// Phase counter for SDR output

	uint8_t set_ir_delay;	// Delay line for setting IR bit in ICR
	uint8_t clear_ir_delay;	// Delay line for clearing IR bit in ICR
	uint8_t irq_delay;		// Delay line for asserting IRQ
	bool trigger_tb_bug;	// Flag: Timer B bug triggered
#endif
};


// First CIA of C64 ($dcxx)
class MOS6526_1 : public MOS6526 {
public:
	MOS6526_1(MOS6510 * cpu, MOS6569 * vic) : MOS6526(cpu), the_vic(vic) { }

	void Reset();

	uint8_t ReadRegister(uint8_t reg);
	void WriteRegister(uint8_t reg, uint8_t byte);

	uint8_t KeyMatrix[8];	// C64 keyboard matrix, 1 bit/key (0: key down, 1: key up)
	uint8_t RevMatrix[8];	// Reversed keyboard matrix

	uint8_t Joystick1;		// Joystick 1 AND value
	uint8_t Joystick2;		// Joystick 2 AND value

private:
	void trigger_irq() override;
	void clear_irq() override;

	void check_lp();

	MOS6569 * the_vic;		// Pointer to VIC object

	uint8_t prev_lp;		// Previous state of LP line (bit 4)
};


// Second CIA of C64 ($ddxx)
class MOS6526_2 : public MOS6526{
public:
	MOS6526_2(MOS6510 * cpu, MOS6569 * vic, MOS6502_1541 * cpu_1541) : MOS6526(cpu), the_vic(vic), the_cpu_1541(cpu_1541) { }

	void Reset();

	void SetState(const MOS6526State * s) override;

	uint8_t ReadRegister(uint8_t reg);
	void WriteRegister(uint8_t reg, uint8_t byte);

	uint8_t IECLines;		// State of IEC lines from C64 side
							// (bit 5 - DATA, bit 4 - CLK, bit 3 - ATN)
							// Wire-AND with 1541 state to obtain physical line state

private:
	void trigger_irq() override;
	void clear_irq() override;

	void write_pa(uint8_t inv_out);

	MOS6569 * the_vic;				// Pointer to VIC object
	MOS6502_1541 * the_cpu_1541;	// Pointer to 1541 CPU object
};


// CIA state
struct MOS6526State {
	uint8_t pra;
	uint8_t ddra;
	uint8_t prb;
	uint8_t ddrb;
	uint8_t ta_lo;
	uint8_t ta_hi;
	uint8_t tb_lo;
	uint8_t tb_hi;
	uint8_t tod_10ths;
	uint8_t tod_sec;
	uint8_t tod_min;
	uint8_t tod_hr;
	uint8_t sdr;
	uint8_t int_flags;	// Pending interrupts
	uint8_t cra;
	uint8_t crb;
						// Additional registers
	uint16_t ta_latch;	// Timer latches
	uint16_t tb_latch;
	bool ta_pb_toggle;
	bool tb_pb_toggle;

	uint8_t ltc_10ths;	// TOD latch
	uint8_t ltc_sec;
	uint8_t ltc_min;
	uint8_t ltc_hr;
	uint8_t alm_10ths;	// TOD alarm time
	uint8_t alm_sec;
	uint8_t alm_min;
	uint8_t alm_hr;

	uint8_t int_mask;	// Enabled interrupts

	uint8_t tod_counter;
	bool tod_halted;
	bool tod_latched;
	bool tod_alarm;

						// FrodoSC:
	bool ta_output;
	bool tb_output;
	uint8_t ta_count_delay;
	uint8_t tb_count_delay;
	uint8_t ta_load_delay;
	uint8_t tb_load_delay;
	uint8_t ta_oneshot_delay;
	uint8_t tb_oneshot_delay;
	uint8_t sdr_shift_counter;
	uint8_t set_ir_delay;
	uint8_t clear_ir_delay;
	uint8_t irq_delay;
	bool trigger_tb_bug;
};


/*
 *  Set interrupt flag
 */

inline void MOS6526::set_int_flag(uint8_t flag)
{
	icr |= flag;
#ifndef FRODO_SC
	if (int_mask & flag) {
		icr |= 0x80;
		trigger_irq();
	}
#endif
}


/*
 *  Set FLAG interrupt flag
 */

inline void MOS6526::TriggerFlagLine()
{
	set_int_flag(0x10);
}


/*
 *  Check for TOD alarm
 */

inline void MOS6526::check_tod_alarm()
{
	bool alarm_match = (tod_10ths == alm_10ths && tod_sec == alm_sec &&
	                    tod_min == alm_min && tod_hr == alm_hr);

	// Raise interrupt on positive edge of alarm match
	if (alarm_match && !tod_alarm) {
		set_int_flag(4);
	}

	tod_alarm = alarm_match;
}


/*
 *  Read from CIA register
 */

inline uint8_t MOS6526::read_register(uint8_t reg)
{
	switch (reg) {
		case 0:
			return (pra & ddra) | (pa_in & ~ddra);
		case 1: {
			uint8_t ret = (prb & ddrb) | (pb_in & ~ddrb);

			if ((cra | crb) & 0x02) {	// TA/TB output to PB enabled?
				ret = timer_on_pb(ret);
			}

			return ret;
		}
		case 2:
			return ddra;
		case 3:
			return ddrb;

		case 4:
			return ta.counter & 0xff;
		case 5:
			return ta.counter >> 8;
		case 6:
			return tb.counter & 0xff;
		case 7:
			return tb.counter >> 8;

		case 8: {
			uint8_t ret = tod_latched ? ltc_10ths : tod_10ths;
			tod_latched = false;
			return ret;
		}
		case 9:
			return tod_latched ? ltc_sec : tod_sec;
		case 10:
			return tod_latched ? ltc_min : tod_min;
		case 11: {
			if (! tod_latched) {
				ltc_10ths = tod_10ths;
				ltc_sec = tod_sec;
				ltc_min = tod_min;
				ltc_hr = tod_hr;
				tod_latched = true;
			}
			return tod_latched ? ltc_hr : tod_hr;
		}

		case 12:
			return sdr;

		case 13: {
			uint8_t ret = icr;
#ifdef FRODO_SC
			icr &= 0x80;
			clear_ir_delay |= 1;	// One cycle delay clearing IR
			irq_delay &= ~2;		// But deassert IRQ immediately
#else
			icr = 0;
#endif
			clear_irq();
			return ret;
		}

		case 14:
			return cra;
		case 15:
			return crb;

		default:	// Can't happen
			return 0;
	}
}


/*
 *  Write to CIA register
 */

inline void MOS6526::write_register(uint8_t reg, uint8_t byte)
{
	switch (reg) {
		case 0:
			pra = byte;
			break;
		case 1:
			prb = byte;
			break;
		case 2:
			ddra = byte;
			break;
		case 3:
			ddrb = byte;
			break;

		case 4:
			ta.latch = (ta.latch & 0xff00) | byte;
#ifdef FRODO_SC
			if (ta.load_delay & 4) {
				ta.counter = ta.latch;	// Cut-through if timer is just being loaded
			}
#endif
			break;
		case 5:
			ta.latch = (ta.latch & 0xff) | (byte << 8);
			if (!(cra & 1)) {			// Timer stopped?
#ifdef FRODO_SC
				ta.load_delay |= 1;		// Load timer in two cycles
				ta.idle = false;
#else
				ta.counter = ta.latch;	// Load timer immediately
#endif
			}
#ifdef FRODO_SC
			if (ta.load_delay & 4) {
				ta.counter = ta.latch;	// Cut-through if timer is just being loaded
			}
#endif
			break;
		case 6:
			tb.latch = (tb.latch & 0xff00) | byte;
#ifdef FRODO_SC
			if (tb.load_delay & 4) {
				tb.counter = tb.latch;	// Cut-through if timer is just being loaded
			}
#endif
			break;
		case 7:
			tb.latch = (tb.latch & 0xff) | (byte << 8);
			if (!(crb & 1)) {			// Timer stopped?
#ifdef FRODO_SC
				tb.load_delay |= 1;		// Load timer in two cycles
				tb.idle = false;
#else
				tb.counter = tb.latch;	// Load timer immediately
#endif
			}
#ifdef FRODO_SC
			if (tb.load_delay & 4) {
				tb.counter = tb.latch;	// Cut-through if timer is just being loaded
			}
#endif
			break;

		case 8:
			byte &= 0x0f;
			if (crb & 0x80) {
				if (alm_10ths != byte) {
					check_tod_alarm();
				}
				alm_10ths = byte;
			} else {
				if (tod_10ths != byte) {
					check_tod_alarm();
				}
				tod_10ths = byte;
				tod_halted = false;
			}
			check_tod_alarm();
			break;
		case 9:
			byte &= 0x7f;
			if (crb & 0x80) {
				alm_sec = byte;
			} else {
				tod_sec = byte;
			}
			check_tod_alarm();
			break;
		case 10:
			byte &= 0x7f;
			if (crb & 0x80) {
				alm_min = byte;
			} else {
				tod_min = byte;
			}
			check_tod_alarm();
			break;
		case 11:
			byte &= 0x9f;
			if ((byte & 0x1f) == 0x12) {
				byte ^= 0x80;	// Invert AM/PM if hours = 12
			}
			if (crb & 0x80) {
				alm_hr = byte;
			} else {
				tod_hr = byte;
				tod_halted = true;
				tod_counter = 0;
			}
			check_tod_alarm();
			break;

		case 12:
			sdr = byte;
#ifdef FRODO_SC
			if (cra & 0x40) {	// Serial port in output mode?
				if (sdr_shift_counter == 0) {
					sdr_shift_counter = 15;
				}
			}
#else
			set_int_flag(8);	// Fake SDR interrupt for programs that need it
#endif
			break;

		case 13:
#ifdef FRODO_SC
			if (byte & 0x80) {
				int_mask |= byte & 0x1f;
			} else {
				int_mask &= ~byte;
			}
			if ((icr & int_mask) == 0) {
				if (clear_ir_delay & 4) {	// Read from ICR in previous cycle?
					set_ir_delay &= ~2;		// Cancel pending interrupt
					irq_delay &= ~2;
				}
			}
#else
			if (ThePrefs.CIAIRQHack) {	// Hack for addressing modes that read from the address
				icr = 0;
			}
			if (byte & 0x80) {
				int_mask |= byte & 0x1f;
				if (icr & int_mask) {	// Trigger IRQ if pending
					icr |= 0x80;
					trigger_irq();
				}
			} else {
				int_mask &= ~byte;
			}
#endif
			break;

		case 14:
			if ((cra & 1) == 0 && (byte & 1) != 0) {
				ta.pb_toggle = true;	// Starting timer A resets PB toggle state
			}
			cra = byte;
			if (cra & 0x10) {			// Timer A force load
				cra &= ~0x10;
#ifdef FRODO_SC
				ta.load_delay |= 1;		// Load timer in two cycles
#else
				ta.counter = ta.latch;	// Load timer immediately
#endif
			}
#ifdef FRODO_SC
			if ((cra & 0x40) == 0) {	// Serial port in input mode?
				sdr_shift_counter = 0;
			}
			ta.idle = false;
#endif
			break;

		case 15:
			if ((crb & 1) == 0 && (byte & 1) != 0) {
				tb.pb_toggle = true;	// Starting timer B resets PB toggle state
			}
			crb = byte;
			if (crb & 0x10) {			// Timer B force load
				crb &= ~0x10;
#ifdef FRODO_SC
				tb.load_delay |= 1;		// Load timer in two cycles
#else
				tb.counter = tb.latch;	// Load timer immediately
#endif
			}
#ifdef FRODO_SC
			tb.idle = false;
#endif
			break;
	}
}


#endif // ndef CIA_H
