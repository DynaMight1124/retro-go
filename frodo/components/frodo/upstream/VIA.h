/*
 *  VIA.h - 6522 emulation (for 1541)
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

#ifndef VIA_H
#define VIA_H


class MOS6502_1541;
struct MOS6522State;


// 6522 emulation (VIA)
class MOS6522 {
public:
	MOS6522(MOS6502_1541 * cpu, unsigned irq) : the_cpu(cpu), irq_type(irq) { }
	~MOS6522() { }

	void Reset();

	void GetState(MOS6522State * s) const;
	void SetState(const MOS6522State * s);

#ifdef FRODO_SC
	void EmulateCycle();			// Emulate one clock cycle
#else
	void CountTimers(int cycles);	// Emulate timers
#endif

	uint8_t ReadRegister(uint16_t adr);
	void WriteRegister(uint16_t adr, uint8_t byte);

	void SetPAIn(uint8_t byte) { pa_in = byte; }
	void SetPBIn(uint8_t byte) { pb_in = byte; }
	uint8_t PAOut() const { return pra | ~ddra; }
	uint8_t PBOut() const { return prb | ~ddrb; }

	uint8_t PCR() const { return pcr; }

	void TriggerCA1Interrupt();

private:
#ifndef FRODO_SC
	void trigger_irq();
#endif
	void clear_irq(uint8_t flag);

	MOS6502_1541 * the_cpu;	// Pointer to CPU object

	unsigned irq_type;		// Which interrupt type to trigger

	// Registers
	uint8_t pra;
	uint8_t ddra;
	uint8_t prb;
	uint8_t ddrb;
	uint16_t t1c;	// T1 counter
	uint16_t t1l;	// T1 latch
	uint16_t t2c;	// T2 counter
	uint16_t t2l;	// T2 latch
	uint8_t sr;
	uint8_t acr;
	uint8_t pcr;
	uint8_t ifr;
	uint8_t ier;

	// Input lines for ports
	uint8_t pa_in = 0;
	uint8_t pb_in = 0;

	// Flags for timer one-shot control
	bool t1_irq_blocked;
	bool t2_irq_blocked;

#ifdef FRODO_SC
	uint8_t t1_load_delay;	// Delay line for T1 reload
	uint8_t t2_load_delay;	// Delay line for T2 reload
	uint8_t t2_input_delay;	// Delay line for T2 counter input
	uint8_t irq_delay;		// Delay line for IRQ assertion
#endif
};


// VIA state
struct MOS6522State {
	uint8_t pra;			// Registers
	uint8_t ddra;
	uint8_t prb;
	uint8_t ddrb;
	uint16_t t1c;
	uint16_t t1l;
	uint16_t t2c;
	uint16_t t2l;
	uint8_t sr;
	uint8_t acr;
	uint8_t pcr;
	uint8_t ifr;
	uint8_t ier;

	bool t1_irq_blocked;
	bool t2_irq_blocked;

							// Frodo SC:
	uint8_t t1_load_delay;	// Delay line for T1 reload
	uint8_t t2_load_delay;	// Delay line for T2 reload
	uint8_t t2_input_delay;	// Delay line for T2 counter input
	uint8_t irq_delay;		// Delay line for IRQ assertion
};


/*
 *  Read from VIA register
 */

inline uint8_t MOS6522::ReadRegister(uint16_t adr)
{
	switch (adr & 0xf) {
		case 0:
			clear_irq(0x10);	// Clear CB1 interrupt
			return (prb & ddrb) | (pb_in & ~ddrb);
		case 1:
			clear_irq(0x02);	// Clear CA1 interrupt
			return (pra & ddra) | (pa_in & ~ddra);
		case 2:
			return ddrb;
		case 3:
			return ddra;
		case 4:
			clear_irq(0x40);	// Clear T1 interrupt
			return t1c;
		case 5:
			return t1c >> 8;
		case 6:
			return t1l;
		case 7:
			return t1l >> 8;
		case 8:
			clear_irq(0x20);	// Clear T2 interrupt
			return t2c;
		case 9:
			return t2c >> 8;
		case 10:
			return sr;
		case 11:
			return acr;
		case 12:
			return pcr;
		case 13:
			return ifr;
		case 14:
			return ier | 0x80;
		case 15:
			return (pra & ddra) | (pa_in & ~ddra);
		default:	// Can't happen
			return 0;
	}
}


/*
 *  Write to VIA register
 */

inline void MOS6522::WriteRegister(uint16_t adr, uint8_t byte)
{
	switch (adr & 0xf) {
		case 0:
			prb = byte;
			clear_irq(0x10);	// Clear CB1 interrupt
			break;
		case 1:
			pra = byte;
			clear_irq(0x02);	// Clear CA1 interrupt
			break;
		case 2:
			ddrb = byte;
			break;
		case 3:
			ddra = byte;
			break;
		case 4:
		case 6:
			t1l = (t1l & 0xff00) | byte;
			break;
		case 5:
			t1l = (t1l & 0xff) | (byte << 8);
#ifdef FRODO_SC
			t1_load_delay |= 1;	// Load in next cycle
#else
			t1c = t1l;			// Load immediately
#endif
			t1_irq_blocked = false;
			clear_irq(0x40);	// Clear T1 interrupt
			break;
		case 7:
			t1l = (t1l & 0xff) | (byte << 8);
			clear_irq(0x40);	// Clear T1 interrupt
			break;
		case 8:
			t2l = (t2l & 0xff00) | byte;
			break;
		case 9:
			t2l = (t2l & 0xff) | (byte << 8);
#ifdef FRODO_SC
			t2_load_delay |= 1;	// Load in next cycle
#else
			t2c = t2l;			// Load immediately
#endif
			t2_irq_blocked = false;
			clear_irq(0x20);	// Clear T2 interrupt
			break;
		case 10:
			sr = byte;
			break;
		case 11:
			acr = byte;
			break;
		case 12:
			pcr = byte;
			break;
		case 13:
			clear_irq(byte & 0x7f);
			break;
		case 14:
			if (byte & 0x80) {
				ier |= byte & 0x7f;
			} else {
				ier &= ~byte;
			}
			break;
		case 15:
			pra = byte;
			break;
	}
}


#endif // ndef VIA_H
