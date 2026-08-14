/*
 *  CPU1541.h - 6502 (1541) emulation
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

#ifndef CPU_1541_H
#define CPU_1541_H

#include "VIA.h"


// Set this to 1 for more precise CPU cycle calculation
#ifndef PRECISE_CPU_CYCLES
#define PRECISE_CPU_CYCLES 0
#endif


// Interrupt types
enum {
	INT_VIA1IRQ,
	INT_VIA2IRQ,
	INT_RESET1541
};


class C64;
class GCRDisk;
class MOS6526_2;
struct MOS6502State;


// 6502 emulation (1541)
class MOS6502_1541 {
public:
	MOS6502_1541(C64 * c64, GCRDisk * gcr, uint8_t * Ram, uint8_t * Rom);
	~MOS6502_1541();

#ifdef FRODO_SC
	void EmulateCPUCycle();				// Emulate one clock cycle
	void EmulateVIACycle();
#else
	int EmulateLine(int cycles_left);	// Emulate until cycles_left underflows
	void CountVIATimers(int cycles);
#endif
	void Reset();
	void AsyncReset();					// Reset the CPU asynchronously

	uint32_t CycleCounter() const { return cycle_counter; }

	void GetState(MOS6502State * s) const;
	void SetState(const MOS6502State * s);

	uint8_t ExtReadByte(uint16_t adr);
	void ExtWriteByte(uint16_t adr, uint8_t byte);

	void TriggerInterrupt(unsigned which);
	void ClearInterrupt(unsigned which) { int_line[which] = false; }

	void TriggerIECInterrupt();
	uint8_t CalcIECLines() const;

	MOS6526_2 *TheCIA2;		// Pointer to C64 CIA 2

	uint8_t IECLines;		// State of IEC lines from 1541 side
							// (bit 5 - DATA, bit 4 - CLK, bit 3 - ATN)
							// Wire-AND with C64 state to obtain physical line state

	bool Idle;				// true: 1541 is idle

private:
	uint8_t read_byte(uint16_t adr);
	uint8_t read_byte_via1(uint16_t adr);
	uint8_t read_byte_via2(uint16_t adr);
	uint16_t read_word(uint16_t adr);
	void write_byte(uint16_t adr, uint8_t byte);
	void write_byte_via1(uint16_t adr, uint8_t byte);
	void write_byte_via2(uint16_t adr, uint8_t byte);

	void set_iec_lines(uint8_t inv_out);
	bool set_overflow_enabled() const { return (via2->PCR() & 0x0e) == 0x0e; }	// CA2 high output

	uint8_t read_zp(uint16_t adr);
	uint16_t read_zp_word(uint16_t adr);
	void write_zp(uint16_t adr, uint8_t byte);

	void jump(uint16_t adr);
	void illegal_op(uint16_t adr);

	void do_adc(uint8_t byte);
	void do_sbc(uint8_t byte);

	uint8_t * ram;			// Pointer to main RAM
	uint8_t * rom;			// Pointer to ROM
	C64 * the_c64;			// Pointer to C64 object
	GCRDisk * the_gcr_disk;	// Pointer to GCR disk object

	uint32_t cycle_counter;

	bool int_line[3];		// Interrupt line state (index: INT_*)
	bool nmi_triggered;		// Unused on 1541

	uint8_t n_flag, z_flag;
	bool v_flag, d_flag, i_flag, c_flag;
	uint8_t a, x, y, sp;

	uint16_t pc;

	bool jammed;			// Flag: CPU jammed, user notified

#ifdef FRODO_SC
	void check_interrupts();

	bool irq_pending;
	uint8_t irq_delay;		// Delay line for IRQ recognition (11→01→00)

	bool nmi_pending;		// Unused on 1541

	uint8_t state, op;		// Current state and opcode
	uint16_t ar, ar2;		// Address registers
	uint8_t rdbuf;			// Data buffer for RMW instructions
#else
	int borrowed_cycles;	// Borrowed cycles from next line
#endif

	uint8_t atn_ack;		// ATN acknowledge: 0x00 or 0x08 (XOR value for IECLines ATN)

	MOS6522 * via1 = nullptr;	// VIA 1 object
	MOS6522 * via2 = nullptr;	// VIA 2 object
};


// 6502 and VIA state
struct MOS6502State {
	uint32_t cycle_counter;

	uint8_t a, x, y;
	uint8_t p;				// Processor flags
	uint16_t pc, sp;

	bool int_line[3];		// Interrupt line state

	bool idle;

	MOS6522State via1;		// VIA 1
	MOS6522State via2;		// VIA 2

							// Frodo SC:
	bool instruction_complete;
	uint8_t state, op;
	uint16_t ar, ar2;
	uint8_t rdbuf;

	bool irq_pending;
	uint8_t irq_delay;
};


#endif // ndef CPU_1541_H
