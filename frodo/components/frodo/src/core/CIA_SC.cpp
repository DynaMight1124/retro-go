/*
 *  CIA_SC.cpp - Single-cycle 6526 emulation
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

/*
 * Notes:
 * ------
 *
 *  - The EmulateCycle() function is called for every emulated Phi2 clock
 *    cycle. It counts down the timers and triggers interrupts if necessary.
 *  - The TOD clocks are counted by CountTOD() during the VBlank, so the
 *    input frequency is 50Hz.
 *  - The fields KeyMatrix and RevMatrix contain one bit for each key on the
 *    C64 keyboard (0: key pressed, 1: key released). KeyMatrix is used for
 *    normal keyboard polling (PRA->PRB), RevMatrix for reversed polling
 *    (PRB->PRA).
 */

#include "sysdeps.h"

#include "CIA.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "VIC.h"
#include "Prefs.h"


/*
 *  Reset the CIA
 */

void MOS6526::Reset()
{
	pra = prb = ddra = ddrb = 0;

	tod_10ths = tod_sec = tod_min = 0; tod_hr = 1;
	ltc_10ths = ltc_sec = ltc_min = 0; ltc_hr = 1;
	alm_10ths = alm_sec = alm_min = alm_hr = 0;

	sdr = icr = cra = crb = int_mask = 0;

	ta.counter       = tb.counter       = 0;
	ta.latch         = tb.latch         = 0xffff;
	ta.pb_toggle     = tb.pb_toggle     = false;
	ta.idle          = tb.idle          = false;
	ta.output        = tb.output        = false;
	ta.count_delay   = tb.count_delay   = 0;
	ta.load_delay    = tb.load_delay    = 0;
	ta.oneshot_delay = tb.oneshot_delay = 0;

	tod_counter = 0;
	tod_halted = true;
	tod_latched = false;
	tod_alarm = false;

	sdr_shift_counter = 0;
	set_ir_delay = 0;
	clear_ir_delay = 0;
	irq_delay = 0;
	trigger_tb_bug = false;
}

void MOS6526_1::Reset()
{
	MOS6526::Reset();

	// Clear keyboard matrix and joystick states
	for (int i=0; i<8; i++) {
		KeyMatrix[i] = RevMatrix[i] = 0xff;
	}

	Joystick1 = Joystick2 = 0xff;
	prev_lp = 0x10;
}

void MOS6526_2::Reset()
{
	MOS6526::Reset();

	// VA14/15 = 0
	the_vic->ChangedVA(0);

	// IEC
	IECLines = 0x38;	// DATA, CLK, ATN high
}


/*
 *  Get CIA state
 */

void MOS6526::GetState(MOS6526State * s) const
{
	s->pra = pra;
	s->prb = prb;
	s->ddra = ddra;
	s->ddrb = ddrb;

	s->ta_lo = ta.counter & 0xff;
	s->ta_hi = ta.counter >> 8;
	s->tb_lo = tb.counter & 0xff;
	s->tb_hi = tb.counter >> 8;
	s->ta_latch = ta.latch;
	s->tb_latch = tb.latch;

	s->cra = cra;
	s->crb = crb;

	s->tod_10ths = tod_10ths;
	s->tod_sec = tod_sec;
	s->tod_min = tod_min;
	s->tod_hr = tod_hr;
	s->ltc_10ths = ltc_10ths;
	s->ltc_sec = ltc_sec;
	s->ltc_min = ltc_min;
	s->ltc_hr = ltc_hr;
	s->alm_10ths = alm_10ths;
	s->alm_sec = alm_sec;
	s->alm_min = alm_min;
	s->alm_hr = alm_hr;

	s->sdr = sdr;

	s->int_flags = icr;
	s->int_mask = int_mask;

	s->tod_counter = tod_counter;
	s->tod_halted = tod_halted;
	s->tod_latched = tod_latched;
	s->tod_alarm = tod_alarm;

	s->ta_pb_toggle = ta.pb_toggle;
	s->tb_pb_toggle = tb.pb_toggle;
	s->ta_output = ta.output;
	s->tb_output = tb.output;
	s->ta_count_delay = ta.count_delay;
	s->tb_count_delay = tb.count_delay;
	s->ta_load_delay = ta.load_delay;
	s->tb_load_delay = tb.load_delay;
	s->ta_oneshot_delay = ta.oneshot_delay;
	s->tb_oneshot_delay = tb.oneshot_delay;

	s->sdr_shift_counter = sdr_shift_counter;
	s->set_ir_delay = set_ir_delay;
	s->clear_ir_delay = clear_ir_delay;
	s->irq_delay = irq_delay;
	s->trigger_tb_bug = trigger_tb_bug;
}


/*
 *  Restore CIA state
 */

void MOS6526::SetState(const MOS6526State * s)
{
	pra = s->pra;
	prb = s->prb;
	ddra = s->ddra;
	ddrb = s->ddrb;

	ta.counter = (s->ta_hi << 8) | s->ta_lo;
	tb.counter = (s->tb_hi << 8) | s->tb_lo;
	ta.latch = s->ta_latch;
	tb.latch = s->tb_latch;

	cra = s->cra;
	crb = s->crb;

	tod_10ths = s->tod_10ths;
	tod_sec = s->tod_sec;
	tod_min = s->tod_min;
	tod_hr = s->tod_hr;
	ltc_10ths = s->ltc_10ths;
	ltc_sec = s->ltc_sec;
	ltc_min = s->ltc_min;
	ltc_hr = s->ltc_hr;
	alm_10ths = s->alm_10ths;
	alm_sec = s->alm_sec;
	alm_min = s->alm_min;
	alm_hr = s->alm_hr;

	sdr = s->sdr;

	icr = s->int_flags;
	int_mask = s->int_mask;

	tod_counter = s->tod_counter;
	tod_halted = s->tod_halted;
	tod_latched = s->tod_latched;
	tod_alarm = s->tod_alarm;

	ta.pb_toggle = s->ta_pb_toggle;
	tb.pb_toggle = s->tb_pb_toggle;
	ta.output = s->ta_output;
	tb.output = s->tb_output;
	ta.count_delay = s->ta_count_delay;
	tb.count_delay = s->tb_count_delay;
	ta.load_delay = s->ta_load_delay;
	tb.load_delay = s->tb_load_delay;
	ta.oneshot_delay = s->ta_oneshot_delay;
	tb.oneshot_delay = s->tb_oneshot_delay;
	ta.idle = false;
	tb.idle = false;

	sdr_shift_counter = s->sdr_shift_counter;
	set_ir_delay = s->set_ir_delay;
	clear_ir_delay = s->clear_ir_delay;
	irq_delay = s->irq_delay;
	trigger_tb_bug = s->trigger_tb_bug;
}

void MOS6526_2::SetState(const MOS6526State * s)
{
	MOS6526::SetState(s);

	uint8_t inv_out = ~PAOut();
	IECLines = inv_out & 0x38;
}


/*
 *  Output TA/TB to PB6/7
 */

uint8_t MOS6526::timer_on_pb(uint8_t byte) const
{
	if (cra & 0x02) {

		// TA output to PB6
		if ((cra & 0x04) ? ta.pb_toggle : ta.output) {
			byte |= 0x40;
		} else {
			byte &= 0xbf;
		}
	}

	if (crb & 0x02) {

		// TB output to PB7
		if ((crb & 0x04) ? tb.pb_toggle : tb.output) {
			byte |= 0x80;
		} else {
			byte &= 0x7f;
		}
	}

	return byte;
}


/*
 *  Read from register (CIA 1)
 */

uint8_t MOS6526_1::ReadRegister(uint8_t reg)
{
	switch (reg) {
		case 0: {	// Port A: Handle keyboard and joysticks
			uint8_t ret = PAOut(), tst = PBOut() & Joystick1;
			if (!(tst & 0x01)) ret &= RevMatrix[0];	// AND all active columns
			if (!(tst & 0x02)) ret &= RevMatrix[1];
			if (!(tst & 0x04)) ret &= RevMatrix[2];
			if (!(tst & 0x08)) ret &= RevMatrix[3];
			if (!(tst & 0x10)) ret &= RevMatrix[4];
			if (!(tst & 0x20)) ret &= RevMatrix[5];
			if (!(tst & 0x40)) ret &= RevMatrix[6];
			if (!(tst & 0x80)) ret &= RevMatrix[7];
			return ret & Joystick2;
		}
		case 1: {	// Port B: Handle keyboard and joysticks
			uint8_t ret = ~ddrb, tst = PAOut() & Joystick2;
			if (!(tst & 0x01)) ret &= KeyMatrix[0];	// AND all active rows
			if (!(tst & 0x02)) ret &= KeyMatrix[1];
			if (!(tst & 0x04)) ret &= KeyMatrix[2];
			if (!(tst & 0x08)) ret &= KeyMatrix[3];
			if (!(tst & 0x10)) ret &= KeyMatrix[4];
			if (!(tst & 0x20)) ret &= KeyMatrix[5];
			if (!(tst & 0x40)) ret &= KeyMatrix[6];
			if (!(tst & 0x80)) ret &= KeyMatrix[7];
			ret = (ret | (prb & ddrb)) & Joystick1;

			if ((cra | crb) & 0x02) {	// TA/TB output to PB enabled?
				ret = timer_on_pb(ret);
			}

			return ret;
		}
		default:
			return read_register(reg);
	}
}


/*
 *  Read from register (CIA 2)
 */

uint8_t MOS6526_2::ReadRegister(uint8_t reg)
{
	switch (reg) {
		case 0: {	// Port A: IEC
			uint8_t in = ((the_cpu_1541->CalcIECLines() & 0x30) << 2)	// DATA and CLK from bus
			           | 0x3f;											// Other lines high
			SetPAIn(in);
			break;
		}
		case 1: {	// Port B: User port
			SetPBIn(0xff);
			break;
		}
	}

	return read_register(reg);
}


/*
 *  Write to register (CIA 1)
 */

// Write to port B, check for lightpen interrupt
inline void MOS6526_1::check_lp()
{
	uint8_t new_lp = PBOut() & 0x10;
	if (new_lp != prev_lp) {
		the_vic->TriggerLightpen();
		prev_lp = new_lp;
	}
}

void MOS6526_1::WriteRegister(uint8_t reg, uint8_t byte)
{
	write_register(reg, byte);

	switch (reg) {
		case 1:	// Port B: Handle VIC lightpen input
		case 3:
			check_lp();
			break;
	}
}


/*
 *  Write to register (CIA 2)
 */

// Write to port A, check for VIC bank change and IEC lines
inline void MOS6526_2::write_pa(uint8_t inv_out)
{
	the_vic->ChangedVA(inv_out & 3);

	uint8_t old_lines = IECLines;
	IECLines = inv_out & 0x38;

	if ((IECLines ^ old_lines) & 0x08) {	// ATN changed
		if (old_lines & 0x08) {				// ATN 1->0
			the_cpu_1541->TriggerIECInterrupt();
		}
	}
}

void MOS6526_2::WriteRegister(uint8_t reg, uint8_t byte)
{
	write_register(reg, byte);

	switch (reg) {
		case 0:	// Port A: Handle VIC bank and IEC port
		case 2:
			write_pa(~PAOut());
			break;
	}
}


/*
 *  Emulate CIA for one cycle
 */

void MOS6526::emulate_timer(Timer & t, uint8_t & cr, bool input)
{
	// Decode CR bits
	if (input && (cr & 1)) {	// Started and input active
		t.count_delay |= 1;
	}
	if (cr & 8) {				// One-shot
		t.oneshot_delay |= 1;
	}

	// Count timer
	if (t.count_delay & 4) {	// Two cycles delay
		--t.counter;
	}

	// Check for end condition
	t.output = false;

	if ((t.counter == 0) && (t.count_delay & 2)) {
		t.output = true;				// Set timer output
		t.pb_toggle = !t.pb_toggle;

		if ((t.oneshot_delay & 3) != 0) {
			cr &= ~1;					// Stop timer in one-shot mode
			t.count_delay &= ~1;		// For at least two cycles
		}

		t.load_delay |= 4;				// Reload timer immediately
	}

	// Load timer
	if (t.load_delay & 4) {		// Two cycles delay
		t.counter = t.latch;
		t.count_delay &= ~2;	// Skip counting in next cycle
	}
}

void MOS6526::EmulateCycle()
{
	if (! ta.idle) {

		// Shift timer A delay lines
		ta.count_delay <<= 1;
		ta.load_delay <<= 1;
		ta.oneshot_delay <<= 1;

		// Emulate timer A
		bool ta_input = (cra & 0x20) == 0;	// Count Phi2
		emulate_timer(ta, cra, ta_input);

		if (ta.output) {
			set_int_flag(1);

			if (cra & 0x40) {	// Serial port in output mode?
				if (sdr_shift_counter > 0) {
					--sdr_shift_counter;
					if (sdr_shift_counter == 0) {
						set_int_flag(8);
					}
				}
			}

			// Wake up timer B if counting timer A
			if (crb & 0x40) {
				tb.idle = false;
			}
		}

		// Timer A now idle?
		if ((ta.count_delay | ta.load_delay) == 0) {
			ta.idle = true;
		}
	}

	bool tb_bug = false;
	if (trigger_tb_bug) {
		tb_bug = true;
		trigger_tb_bug = false;
	}

	if (! tb.idle) {

		// Shift timer B delay lines
		tb.count_delay <<= 1;
		tb.load_delay <<= 1;
		tb.oneshot_delay <<= 1;

		// Emulate timer B
		bool tb_input;
		switch (crb & 0x60) {
			case 0x00:	// Count Phi2
				tb_input = true;
				break;
			case 0x20:	// Count CNT (nothing connected)
				tb_input = false;
				break;
			default:	// Count TA, without or with CNT
				tb_input = ta.output;
				break;
		}
		emulate_timer(tb, crb, tb_input);

		if (tb.output) {
			set_int_flag(2);
			if (clear_ir_delay & 1) {	// Timer B acknowledges interrupt if ICR was read in previous cycle (HW bug)
				trigger_tb_bug = true;
			}
		}

		// Timer B now idle?
		if ((tb.count_delay | tb.load_delay) == 0) {
			tb.idle = true;
		}
	}

	// Update IRQ status
	if (icr & int_mask) {
		set_ir_delay |= 1;
		irq_delay |= 1;
	}
	if (clear_ir_delay & 2) {	// One cycle delay clearing IR
		if (tb_bug) {
			icr &= ~2;
		}
		icr &= 0x7f;
	}
	if (set_ir_delay & 2) {		// One cycle of delay setting IR
		icr |= 0x80;
	}
	if (irq_delay & 2) {		// One cycle delay asserting IRQ line
		trigger_irq();
	}

	set_ir_delay <<= 1;
	clear_ir_delay <<= 1;
	irq_delay <<= 1;
}


/*
 *  Count CIA TOD clock (called during VBlank)
 */

void MOS6526::CountTOD()
{
	if (tod_halted)
		return;

	// Increment TOD counter
	unsigned tod_compare = (cra & 0x80) ? 4 : 5;
	if (tod_counter != tod_compare) {
		++tod_counter;
		if (tod_counter > 5) {
			tod_counter = 0;
		}
		return;
	}

	// Count TOD
	tod_counter = 0;

	uint8_t lo, hi;

	// 1/10 seconds
	tod_10ths = (tod_10ths + 1) & 0x0f;
	if (tod_10ths == 10) {
		tod_10ths = 0;

		// Seconds
		lo = (tod_sec + 1) & 0x0f;
		hi = tod_sec >> 4;
		if (lo == 10) {
			lo = 0;
			hi = (hi + 1) & 0x07;
		}
		if (hi == 6) {
			tod_sec = 0;

			// Minutes
			lo = (tod_min + 1) & 0x0f;
			hi = tod_min >> 4;
			if (lo == 10) {
				lo = 0;
				hi = (hi + 1) & 0x07;
			}
			if (hi == 6) {
				tod_min = 0;

				// Hours
				lo = (tod_hr + 1) & 0x0f;
				hi = (tod_hr >> 4) & 1;
				if (((hi << 4) | lo) == 10) {
					lo = 0;
					hi = (hi + 1) & 1;
				}
				tod_hr = (tod_hr & 0x80) | (hi << 4) | lo;
				if ((tod_hr & 0x1f) == 0x13) {
					tod_hr = (tod_hr & 0x80) | 1;	// 1 AM follows 12 AM, 1 PM follows 12 PM
				} else if ((tod_hr & 0x1f) == 0x12) {
					tod_hr ^= 0x80;					// 12 AM follows 11 PM, 12 PM follows 11 AM
				}
			} else {
				tod_min = (hi << 4) | lo;
			}
		} else {
			tod_sec = (hi << 4) | lo;
		}
	}

	// Check for alarm
	check_tod_alarm();
}


/*
 *  Interrupt functions
 */

// Trigger IRQ (CIA 1)
void MOS6526_1::trigger_irq()
{
	the_cpu->TriggerCIAIRQ();
}

// Clear IRQ (CIA 1)
void MOS6526_1::clear_irq()
{
	the_cpu->ClearCIAIRQ();
}

// Trigger NMI (CIA 2)
void MOS6526_2::trigger_irq()
{
	the_cpu->TriggerNMI();
}

// Clear NMI (CIA 2)
void MOS6526_2::clear_irq()
{
	the_cpu->ClearNMI();
}
