/*
 *  CPUC64.h - 6510 (C64) emulation
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

#ifndef CPU_C64_H
#define CPU_C64_H

#include "C64.h"


// Set this to 1 for more precise CPU cycle calculation
#ifndef PRECISE_CPU_CYCLES
#define PRECISE_CPU_CYCLES 0
#endif

// Set this to 1 for instruction-aligned CIA emulation
#ifndef PRECISE_CIA_CYCLES
#define PRECISE_CIA_CYCLES 0
#endif


// Interrupt types
enum {
	INT_VICIRQ,
	INT_CIAIRQ,
	INT_NMI,
	INT_RESET
};


class MOS6569;
class MOS6581;
class MOS6526_1;
class MOS6526_2;
class Cartridge;
class IEC;
class Tape;
struct MOS6510State;


// 6510 emulation (C64)
class MOS6510 {
public:
	MOS6510(C64 * c64, uint8_t * Ram, uint8_t * Basic, uint8_t * Kernal, uint8_t * Char, uint8_t * Color);

	// Set pointers to other objects
	void SetChips(MOS6569 * vic, MOS6581 * sid, MOS6526_1 * cia1, MOS6526_2 * cia2,
	              Cartridge * cart, IEC * iec, Tape * tape)
	{
		the_vic = vic;
		the_sid = sid;
		the_cia1 = cia1;
		the_cia2 = cia2;
		the_cart = cart;
		the_iec = iec;
		the_tape = tape;
	}

#ifdef FRODO_SC
	void EmulateCycle();				// Emulate one clock cycle
#else
	int EmulateLine(int cycles_left);	// Emulate until cycles_left underflows
#endif

	void Reset();
	void AsyncReset();					// Reset the CPU asynchronously
	void AsyncNMI();					// Raise NMI asynchronously (NMI pulse)

	void GetState(MOS6510State *s) const;
	void SetState(const MOS6510State *s);

	uint8_t ExtReadByte(uint16_t adr);
	void ExtWriteByte(uint16_t adr, uint8_t byte);
	uint8_t REUReadByte(uint16_t adr);
	void REUWriteByte(uint16_t adr, uint8_t byte);

	void TriggerVICIRQ();
	void ClearVICIRQ();
	void TriggerCIAIRQ();
	void ClearCIAIRQ();
	void TriggerNMI();
	void ClearNMI();

	void SetTapeSense(bool pressed);

	uint16_t GetPC() const { return pc; }

	int ExtConfig;			// Memory configuration for ExtRead/WriteByte (0..7)

#ifdef FRODO_SC
	bool BALow;				// BA line for Frodo SC
#endif

private:
	uint8_t read_byte(uint16_t adr);
	uint8_t read_byte_io(uint16_t adr);
	uint16_t read_word(uint16_t adr);
	void write_byte(uint16_t adr, uint8_t byte);
	void write_byte_io(uint16_t adr, uint8_t byte);

	uint8_t read_zp(uint16_t adr);
	uint16_t read_zp_word(uint16_t adr);
	void write_zp(uint16_t adr, uint8_t byte);

	void new_config();
	void illegal_op(uint16_t adr);

	void do_adc(uint8_t byte);
	void do_sbc(uint8_t byte);

	uint8_t read_emulator_id(uint16_t adr);

	C64 * the_c64;			// Pointer to C64 object

	MOS6569 * the_vic;		// Pointer to VIC object
	MOS6581 * the_sid;		// Pointer to SID object
	MOS6526_1 * the_cia1;	// Pointer to CIA 1 object
	MOS6526_2 * the_cia2;	// Pointer to CIA 2 object
	Cartridge * the_cart;	// Pointer to cartridge object
	IEC * the_iec;			// Pointer to drive array
	Tape * the_tape;		// Pointer to datasette object

	uint8_t * ram;			// Pointer to main RAM
	uint8_t * basic_rom;	// Pointers to ROMs
	uint8_t * kernal_rom;
	uint8_t * char_rom;
	uint8_t * color_ram;	// Pointer to color RAM

	bool int_line[4];		// Interrupt line state (index: INT_*)
	bool nmi_triggered;		// Flag: NMI triggered by transition

	uint8_t n_flag, z_flag;
	bool v_flag, d_flag, i_flag, c_flag;
	uint8_t a, x, y, sp;

	uint16_t pc;

	bool jammed;			// Flag: CPU jammed, user notified

#ifdef FRODO_SC
	void check_interrupts();

	bool irq_pending;
	uint8_t irq_delay;			// Delay line for IRQ recognition (11→01→00)
	uint8_t irq_off_delay;		// Delay line for IRQ deassertion (11→01→00)

	bool nmi_pending;
	uint8_t nmi_delay;			// Delay line for NMI recognition (11→01→00)

	bool tape_write;			// Tape write signal

	uint8_t state, op;			// Current state and opcode
	uint16_t ar, ar2;			// Address registers
	uint8_t rdbuf;				// Data buffer for RMW instructions
	uint8_t ddr, pr;			// Processor port
	uint8_t pr_out, pr_in;
#else
	bool tape_sense;			// Tape sense line (true = button pressed)
	int	borrowed_cycles;		// Borrowed cycles from next line
	uint8_t dfff_byte;			// Byte at $dfff for emulator ID
#endif

	bool basic_in, kernal_in, char_in, io_in;
};


// 6510 state
struct MOS6510State {
	uint8_t a, x, y;
	uint8_t p;					// Processor flags
	uint16_t pc, sp;

	uint8_t ddr, pr, pr_out;	// Processor port

	bool int_line[4];			// Interrupt line state
	bool nmi_triggered;	

	uint8_t dfff_byte;
								// Frodo SC:
	bool instruction_complete;
	uint8_t state, op;
	uint16_t ar, ar2;
	uint8_t rdbuf;

	bool irq_pending;
	uint8_t irq_delay;
	uint8_t irq_off_delay;
	bool nmi_pending;
	uint8_t nmi_delay;
};


// Interrupt functions
inline void MOS6510::TriggerVICIRQ()
{
#ifdef FRODO_SC
	if (!(int_line[INT_VICIRQ] || int_line[INT_CIAIRQ])) {
		irq_delay = 3;	// Two cycles delay until recognition
	}
#endif
	int_line[INT_VICIRQ] = true;
}

inline void MOS6510::TriggerCIAIRQ()
{
#ifdef FRODO_SC
	if (!(int_line[INT_VICIRQ] || int_line[INT_CIAIRQ])) {
		irq_delay = 3;	// Two cycles delay until recognition
	}
#endif
	int_line[INT_CIAIRQ] = true;
}

inline void MOS6510::TriggerNMI()
{
	if (!int_line[INT_NMI]) {
#ifdef FRODO_SC
		nmi_delay = 3;	// Two cycles delay until recognition
#endif
		nmi_triggered = true;
		int_line[INT_NMI] = true;
	}
}

inline void MOS6510::ClearVICIRQ()
{
#ifdef FRODO_SC
	if (int_line[INT_VICIRQ] && ! int_line[INT_CIAIRQ]) {
		irq_off_delay = 3;	// Two cycles delay after deassertion
	}
#endif
	int_line[INT_VICIRQ] = false;
}

inline void MOS6510::ClearCIAIRQ()
{
#ifdef FRODO_SC
	if (int_line[INT_CIAIRQ] && ! int_line[INT_VICIRQ]) {
		irq_off_delay = 3;	// Two cycles delay after deassertion
	}
#endif
	int_line[INT_CIAIRQ] = false;
}

inline void MOS6510::ClearNMI()
{
	int_line[INT_NMI] = false;
}


#endif // ndef CPU_C64_H
