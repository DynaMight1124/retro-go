/*
 *  CPUC64_SC.cpp - Single-cycle 6510 (C64) emulation
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
 * Opcode execution:
 *  - All opcodes are resolved into single clock cycles. There is one
 *    switch case for each cycle.
 *  - The "state" variable specifies the routine to be executed in the
 *    next cycle. Its upper 8 bits contain the current opcode, its lower 8
 *    bits contain the cycle number (0..7) within the opcode.
 *  - Opcodes are fetched in cycle 0 (state = 0)
 *  - The states 0x0008..0x0017 are used for interrupts
 *  - There is exactly one memory access in each clock cycle
 *  - All memory accesses are done with the read_byte() and write_byte()
 *    functions which also do the memory address decoding.
 *  - If a write occurs to addresses 0 or 1, new_config() is called to check
 *    whether the memory configuration has changed.
 *  - The possible interrupt sources are:
 *      INT_VICIRQ: I flag is checked, jump to ($fffe)
 *      INT_CIAIRQ: I flag is checked, jump to ($fffe)
 *      INT_NMI: Jump to ($fffa)
 *      INT_RESET: Jump to ($fffc)
 *  - The z_flag variable has the inverse meaning of the 6510 Z flag.
 *  - Only the highest bit of the n_flag variable is used.
 *  - The $f2 opcode that would normally crash the 6510 is used to implement
 *    emulator-specific functions, mainly those for the IEC routines.
 */

#include "sysdeps.h"

#include "CPUC64.h"
#include "CPU_common.h"
#include "C64.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"
#include "REU.h"
#include "IEC.h"
#include "Tape.h"

#include <format>


/*
 *  6510 constructor: Initialize registers
 */

MOS6510::MOS6510(C64 *c64, uint8_t *Ram, uint8_t *Basic, uint8_t *Kernal, uint8_t *Char, uint8_t *Color)
 : the_c64(c64), ram(Ram), basic_rom(Basic), kernal_rom(Kernal), char_rom(Char), color_ram(Color)
{
	a = x = y = 0;
	sp = 0xff;
	n_flag = z_flag = 0;
	v_flag = d_flag = c_flag = false;
	i_flag = true;

	BALow = false;

	int_line[INT_VICIRQ] = false;
	int_line[INT_CIAIRQ] = false;
	int_line[INT_NMI] = false;
	int_line[INT_RESET] = false;

	irq_pending = false;
	irq_delay = 0;
	irq_off_delay = 0;

	nmi_triggered = false;
	nmi_pending = false;
	nmi_delay = 0;

	tape_write = false;
}


/*
 *  Reset CPU asynchronously
 */

void MOS6510::AsyncReset()
{
	int_line[INT_RESET] = true;
}


/*
 *  Raise NMI asynchronously (Restore key)
 */

void MOS6510::AsyncNMI()
{
	TriggerNMI();
}


/*
 *  Reset CPU
 */

void MOS6510::Reset()
{
	// Initialize extra 6510 registers and memory configuration
	ddr = pr = pr_out = 0;
	pr_in = 0x17;
	new_config();

	// Clear all interrupt lines
	int_line[INT_VICIRQ] = false;
	int_line[INT_CIAIRQ] = false;
	int_line[INT_NMI] = false;
	int_line[INT_RESET] = false;

	irq_pending = false;
	irq_delay = 0;
	irq_off_delay = 0;

	nmi_triggered = false;
	nmi_pending = false;
	nmi_delay = 0;

	// Set I flag
	i_flag = true;

	// Read reset vector
	pc = read_word(0xfffc);
	state = O_FETCH;
	jammed = false;
}


/*
 *  Get 6510 register state
 */

void MOS6510::GetState(MOS6510State *s) const
{
	s->a = a;
	s->x = x;
	s->y = y;

	s->p = 0x20 | (n_flag & 0x80);
	if (v_flag) s->p |= 0x40;
	if (d_flag) s->p |= 0x08;
	if (i_flag) s->p |= 0x04;
	if (!z_flag) s->p |= 0x02;
	if (c_flag) s->p |= 0x01;
	
	s->pc = pc;
	s->sp = sp | 0x0100;

	s->ddr = ddr;
	s->pr = pr;
	s->pr_out = pr_out;

	s->int_line[INT_VICIRQ] = int_line[INT_VICIRQ];
	s->int_line[INT_CIAIRQ] = int_line[INT_CIAIRQ];
	s->int_line[INT_NMI] = int_line[INT_NMI];

	s->irq_pending = irq_pending;
	s->irq_delay = irq_delay;
	s->irq_off_delay = irq_off_delay;

	s->nmi_triggered = nmi_triggered;
	s->nmi_pending = nmi_pending;
	s->nmi_delay = nmi_delay;

	s->dfff_byte = 0x55;

	s->instruction_complete = (state == O_FETCH);
	s->state = state;
	s->op = op;
	s->ar = ar;
	s->ar2 = ar2;
	s->rdbuf = rdbuf;
}


/*
 *  Restore 6510 state
 */

void MOS6510::SetState(const MOS6510State *s)
{
	a = s->a;
	x = s->x;
	y = s->y;

	n_flag = s->p;
	v_flag = s->p & 0x40;
	d_flag = s->p & 0x08;
	i_flag = s->p & 0x04;
	z_flag = !(s->p & 0x02);
	c_flag = s->p & 0x01;

	ddr = s->ddr;
	pr = s->pr;
	pr_out = s->pr_out;
	new_config();

	pc = s->pc;
	sp = s->sp & 0xff;

	int_line[INT_VICIRQ] = s->int_line[INT_VICIRQ];
	int_line[INT_CIAIRQ] = s->int_line[INT_CIAIRQ];
	int_line[INT_NMI] = s->int_line[INT_NMI];

	irq_pending = s->irq_pending;
	irq_delay = s->irq_delay;
	irq_off_delay = s->irq_off_delay;

	nmi_triggered = s->nmi_triggered;
	nmi_pending = s->nmi_pending;
	nmi_delay = s->nmi_delay;

	state = s->state;
	op = s->op;
	ar = s->ar;
	ar2 = s->ar2;
	rdbuf = s->rdbuf;
}


/*
 *  Set tape sense line status
 */

void MOS6510::SetTapeSense(bool pressed)
{
	if (pressed) {
		pr_in &= 0xef;
	} else {
		pr_in |= 0x10;
	}
}


/*
 *  Memory configuration has probably changed
 */

void MOS6510::new_config()
{
	pr_out = (pr_out & ~ddr) | (pr & ddr);
	uint8_t port = pr | ~ddr;

	basic_in = (port & 3) == 3;
	kernal_in = port & 2;
	if (the_cart->notGAME) {
		char_in = (port & 3) && !(port & 4);
	} else {
		char_in = (port & 2) && !(port & 4);
	}
	io_in = (port & 3) && (port & 4);

	bool tape_motor = (port & 0x20) == 0;
	the_tape->SetMotor(tape_motor);

	bool new_tape_write = port & 0x08;
	if (new_tape_write != tape_write) {
		tape_write = new_tape_write;
		if (tape_write) {	// Rising edge
			the_tape->WritePulse(the_c64->CycleCounter());
		}
	}
}


/*
 *  Read a byte from I/O / ROM space
 */

inline uint8_t MOS6510::read_byte_io(uint16_t adr)
{
	switch (adr >> 12) {
		case 0x8:
		case 0x9:
			if (the_cart->notEXROM) {
				return ram[adr];
			} else {
				return the_cart->ReadROML(adr & 0x1fff, ram[adr], basic_in);
			}
		case 0xa:
		case 0xb:
			if (the_cart->notEXROM || the_cart->notGAME) {
				return basic_in ? basic_rom[adr & 0x1fff] : ram[adr];
			} else {
				return the_cart->ReadROMH(adr & 0x1fff, ram[adr], basic_rom[adr & 0x1fff], basic_in, kernal_in);
			}
		case 0xc:
			return ram[adr];
		case 0xd:
			if (io_in) {
				switch ((adr >> 8) & 0x0f) {
					case 0x0:	// VIC
					case 0x1:
					case 0x2:
					case 0x3:
						return the_vic->ReadRegister(adr & 0x3f);
					case 0x4:	// SID
					case 0x5:
					case 0x6:
					case 0x7:
						return the_sid->ReadRegister(adr & 0x1f);
					case 0x8:	// Color RAM
					case 0x9:
					case 0xa:
					case 0xb:
						return (color_ram[adr & 0x03ff] & 0x0f) | (the_vic->LastVICByte & 0xf0);
					case 0xc:	// CIA 1
						return the_cia1->ReadRegister(adr & 0x0f);
					case 0xd:	// CIA 2
						return the_cia2->ReadRegister(adr & 0x0f);
					case 0xe:	// Cartridge I/O 1 (or open)
						return the_cart->ReadIO1(adr & 0xff, the_vic->LastVICByte);
					case 0xf:	// Cartridge I/O 2 (or open)
						return the_cart->ReadIO2(adr & 0xff, the_vic->LastVICByte);
				}
			} else if (char_in) {
				return char_rom[adr & 0x0fff];
			} else {
				return ram[adr];
			}
		case 0xe:
		case 0xf:
			if (kernal_in) {
				return kernal_rom[adr & 0x1fff];
			} else {
				return ram[adr];
			}
		default:	// Can't happen
			return 0;
	}
}


/*
 *  Read a byte from the CPU's address space
 */

uint8_t MOS6510::read_byte(uint16_t adr)
{
	if (adr < 0x8000) {
		if (adr >= 2) {
			return ram[adr];
		} else if (adr == 0) {
			return ddr;
		} else {
			uint8_t byte = (pr | ~ddr) & (pr_out | pr_in);
			if (!(ddr & 0x20)) {
				byte &= 0xdf;
			}
			return byte;
		}
	} else {
		return read_byte_io(adr);
	}
}


/*
 *  Read a word (little-endian) from the CPU's address space
 */

inline uint16_t MOS6510::read_word(uint16_t adr)
{
	return read_byte(adr) | (read_byte(adr+1) << 8);
}


/*
 *  Write a byte to I/O space
 */

inline void MOS6510::write_byte_io(uint16_t adr, uint8_t byte)
{
	if (adr >= 0xe000) {
		ram[adr] = byte;
		if (adr == 0xff00) {
			the_cart->FF00Trigger();
		}
	} else if (io_in) {
		switch ((adr >> 8) & 0x0f) {
			case 0x0:	// VIC
			case 0x1:
			case 0x2:
			case 0x3:
				the_vic->WriteRegister(adr & 0x3f, byte);
				return;
			case 0x4:	// SID
			case 0x5:
			case 0x6:
				the_sid->WriteRegister(adr & 0x1f, byte);
				return;
			case 0x7:
				if (ThePrefs.TestBench && adr == 0xd7ff) {
					the_c64->RequestQuit(byte);
				} else {
					the_sid->WriteRegister(adr & 0x1f, byte);
				}
				return;
			case 0x8:	// Color RAM
			case 0x9:
			case 0xa:
			case 0xb:
				color_ram[adr & 0x03ff] = byte & 0x0f;
				return;
			case 0xc:	// CIA 1
				the_cia1->WriteRegister(adr & 0x0f, byte);
				return;
			case 0xd:	// CIA 2
				the_cia2->WriteRegister(adr & 0x0f, byte);
				return;
			case 0xe:	// Cartridge I/O 1 (or open)
				the_cart->WriteIO1(adr & 0xff, byte);
				return;
			case 0xf:	// Cartridge I/O 2 (or open)
				the_cart->WriteIO2(adr & 0xff, byte);
				return;
		}
	} else {
		ram[adr] = byte;
	}
}


/*
 *  Write a byte to the CPU's address space
 */

void MOS6510::write_byte(uint16_t adr, uint8_t byte)
{
	if (adr < 0xd000) {
		if (adr >= 2) {
			ram[adr] = byte;
		} else if (adr == 0) {
			ddr = byte;
			ram[0] = the_vic->LastVICByte;
			new_config();
		} else {
			pr = byte;
			ram[1] = the_vic->LastVICByte;
			new_config();
		}
	} else {
		write_byte_io(adr, byte);
	}
}


/*
 *  Read byte from 6510 address space with special memory config (used by SAM)
 */

uint8_t MOS6510::ExtReadByte(uint16_t adr)
{
	// Save old memory configuration
	bool bi = basic_in, ki = kernal_in, ci = char_in, ii = io_in;

	// Set new configuration
	basic_in = (ExtConfig & 3) == 3;
	kernal_in = ExtConfig & 2;
	char_in = (ExtConfig & 3) && ~(ExtConfig & 4);
	io_in = (ExtConfig & 3) && (ExtConfig & 4);

	// Read byte
	uint8_t byte = read_byte(adr);

	// Restore old configuration
	basic_in = bi; kernal_in = ki; char_in = ci; io_in = ii;

	return byte;
}


/*
 *  Write byte to 6510 address space with special memory config (used by SAM)
 */

void MOS6510::ExtWriteByte(uint16_t adr, uint8_t byte)
{
	// Save old memory configuration
	bool bi = basic_in, ki = kernal_in, ci = char_in, ii = io_in;

	// Set new configuration
	basic_in = (ExtConfig & 3) == 3;
	kernal_in = ExtConfig & 2;
	char_in = (ExtConfig & 3) && ~(ExtConfig & 4);
	io_in = (ExtConfig & 3) && (ExtConfig & 4);

	// Write byte
	write_byte(adr, byte);

	// Restore old configuration
	basic_in = bi; kernal_in = ki; char_in = ci; io_in = ii;
}


/*
 *  Read byte from 6510 address space with current memory config (used by REU)
 */

uint8_t MOS6510::REUReadByte(uint16_t adr)
{
	return read_byte(adr);
}


/*
 *  Write byte to 6510 address space with current memory config (used by REU)
 */

void MOS6510::REUWriteByte(uint16_t adr, uint8_t byte)
{
	write_byte(adr, byte);
}


/*
 *  ADC instruction
 */

inline void MOS6510::do_adc(uint8_t byte)
{
	if (!d_flag) {
		uint16_t tmp;

		// Binary mode
		tmp = a + byte + (c_flag ? 1 : 0);
		c_flag = tmp > 0xff;
		v_flag = !((a ^ byte) & 0x80) && ((a ^ tmp) & 0x80);
		z_flag = n_flag = a = tmp;

	} else {
		uint16_t al, ah;

		// Decimal mode
		al = (a & 0x0f) + (byte & 0x0f) + (c_flag ? 1 : 0);		// Calculate lower nybble
		if (al > 9) al += 6;									// BCD fixup for lower nybble

		ah = (a >> 4) + (byte >> 4);							// Calculate upper nybble
		if (al > 0x0f) ah++;

		z_flag = a + byte + (c_flag ? 1 : 0);					// Set flags
		n_flag = ah << 4;	// Only highest bit used
		v_flag = (((ah << 4) ^ a) & 0x80) && !((a ^ byte) & 0x80);

		if (ah > 9) ah += 6;									// BCD fixup for upper nybble
		c_flag = ah > 0x0f;										// Set carry flag
		a = (ah << 4) | (al & 0x0f);							// Compose result
	}
}


/*
 *  SBC instruction
 */

inline void MOS6510::do_sbc(uint8_t byte)
{
	uint16_t tmp = a - byte - (c_flag ? 0 : 1);

	if (!d_flag) {

		// Binary mode
		c_flag = tmp < 0x100;
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ byte) & 0x80);
		z_flag = n_flag = a = tmp;

	} else {
		uint16_t al, ah;

		// Decimal mode
		al = (a & 0x0f) - (byte & 0x0f) - (c_flag ? 0 : 1);		// Calculate lower nybble
		ah = (a >> 4) - (byte >> 4);							// Calculate upper nybble
		if (al & 0x10) {
			al -= 6;											// BCD fixup for lower nybble
			ah--;
		}
		if (ah & 0x10) ah -= 6;									// BCD fixup for upper nybble

		c_flag = tmp < 0x100;									// Set flags
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ byte) & 0x80);
		z_flag = n_flag = tmp;

		a = (ah << 4) | (al & 0x0f);							// Compose result
	}
}


/*
 *  Illegal opcode encountered
 */

void MOS6510::illegal_op(uint16_t adr)
{
	// Notify user once
	if (! jammed) {
		std::string s = std::format("C64 crashed at ${:04X}, press F12 to reset", adr);
		the_c64->ShowNotification(s);
		jammed = true;
	}

	// Keep executing opcode
	--pc;
	state = O_FETCH;
}


/*
 *  Emulate one 6510 clock cycle
 */

// Read byte from memory
#define read_to(adr, to) \
	if (BALow) \
		return; \
	to = read_byte(adr);

// Read byte from memory, throw away result
#define read_idle(adr) \
	if (BALow) \
		return; \
	read_byte(adr);

// Check for pending interrupts
void MOS6510::check_interrupts()
{
	if ((int_line[INT_VICIRQ] || int_line[INT_CIAIRQ] || irq_off_delay) && !i_flag && irq_delay == 0 && !jammed) {
		irq_pending = true;
	}

	if (nmi_triggered && nmi_delay == 0 && !jammed) {
		nmi_pending = true;
		nmi_triggered = false;
	}
}

void MOS6510::EmulateCycle()
{
	uint8_t data, tmp;

	// Shift delay lines
	irq_delay >>= 1;
	irq_off_delay >>= 1;
	nmi_delay >>= 1;

#define RESET_PENDING (int_line[INT_RESET])
#define CHECK_SO ;

#include "CPU_emulcycle.h"

		// Extension opcode
		case O_EXT:
			if ((pc < 0xa000) || (pc >= 0xc000 && pc < 0xe000)) {
				illegal_op(pc - 1);
				break;
			}
			switch (read_byte(pc++)) {
				case 0x00:
					ram[0x90] |= the_iec->Out(ram[0x95], ram[0xa3] & 0x80);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x01:
					ram[0x90] |= the_iec->OutATN(ram[0x95]);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x02:
					ram[0x90] |= the_iec->OutSec(ram[0x95]);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x03:
					ram[0x90] |= the_iec->In(a);
					set_nz(a);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x04:
					the_iec->SetATN();
					pc = 0xedfb;
					Last;
				case 0x05:
					the_iec->RelATN();
					pc = 0xedac;
					Last;
				case 0x06:
					the_iec->Turnaround();
					pc = 0xedac;
					Last;
				case 0x07:
					the_iec->Release();
					pc = 0xedac;
					Last;
				case 0x10:
					the_c64->AutoStartOp();
					x = 0;	// patch replaces LDX #0
					Last;
				default:
					illegal_op(pc - 1);
					break;
			}
			break;

		default:
			illegal_op(pc - 1);
			break;
	}
}
