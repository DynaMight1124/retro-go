/*
 *  CPUC64.cpp - 6510 (C64) emulation (line based)
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
 *  - The EmulateLine() function is called for every emulated
 *    raster line. It has a cycle counter that is decremented by every
 *    executed opcode and if the counter goes below zero, the function
 *    returns.
 *  - All memory accesses are done with the read_byte() and
 *    write_byte() functions which also do the memory address decoding. The
 *    read_zp() and write_zp() functions allow faster access to the zero
 *    page, the pop_byte() and push_byte() macros for the stack.
 *  - If a write occurs to addresses 0 or 1, new_config() is called to check
 *    whether the memory configuration has changed
 *  - The possible interrupt sources are:
 *      INT_VICIRQ: I flag is checked, jump to ($fffe)
 *      INT_CIAIRQ: I flag is checked, jump to ($fffe)
 *      INT_NMI: Jump to ($fffa)
 *      INT_RESET: Jump to ($fffc)
 *  - Interrupts are not checked before every opcode but only at certain
 *    times:
 *      On entering EmulateLine()
 *      On CLI
 *      On PLP if the I flag was cleared
 *      On RTI if the I flag was cleared
 *  - The z_flag variable has the inverse meaning of the 6510 Z flag.
 *  - Only the highest bit of the n_flag variable is used.
 *  - The $f2 opcode that would normally crash the 6510 is used to implement
 *    emulator-specific functions, mainly those for the IEC routines.
 */

#include "sysdeps.h"

#include "CPUC64.h"
#include "C64.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"
#include "REU.h"
#include "IEC.h"
#include "Tape.h"
#include "Version.h"

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

	int_line[INT_VICIRQ] = false;
	int_line[INT_CIAIRQ] = false;
	int_line[INT_NMI] = false;
	int_line[INT_RESET] = false;

	nmi_triggered = false;

	tape_sense = false;

	borrowed_cycles = 0;
	dfff_byte = 0x55;
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
	ram[0] = ram[1] = 0;
	tape_sense = false;
	new_config();

	// Clear all interrupt lines
	int_line[INT_VICIRQ] = false;
	int_line[INT_CIAIRQ] = false;
	int_line[INT_NMI] = false;
	int_line[INT_RESET] = false;

	nmi_triggered = false;

	// Set I flag
	i_flag = true;

	// Read reset vector
	pc = read_word(0xfffc);
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

	s->ddr = ram[0];
	s->pr = ram[1] & 0x3f;
	s->pr_out = s->pr & s->ddr;

	s->int_line[INT_VICIRQ] = int_line[INT_VICIRQ];
	s->int_line[INT_CIAIRQ] = int_line[INT_CIAIRQ];
	s->int_line[INT_RESET] = int_line[INT_RESET];

	s->irq_pending = false;
	s->irq_delay = 0;
	s->irq_off_delay = 0;

	s->nmi_triggered = nmi_triggered;
	s->nmi_pending = false;
	s->nmi_delay = 0;

	s->dfff_byte = dfff_byte;

	s->instruction_complete = true;
	s->state = 0;
	s->op = 0;
	s->ar = s->ar2 = 0;
	s->rdbuf = 0;
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

	ram[0] = s->ddr;
	ram[1] = s->pr;
	new_config();

	pc = s->pc;
	sp = s->sp & 0xff;

	int_line[INT_VICIRQ] = s->int_line[INT_VICIRQ];
	int_line[INT_CIAIRQ] = s->int_line[INT_CIAIRQ];
	int_line[INT_NMI] = s->int_line[INT_NMI];

	nmi_triggered = s->nmi_triggered;

	dfff_byte = s->dfff_byte;
}


/*
 *  Set tape sense line status
 */

void MOS6510::SetTapeSense(bool pressed)
{
	tape_sense = pressed;

	if ((ram[0] & 0x10) == 0) {
		if (pressed) {
			ram[1] &= 0xef;
		} else {
			ram[1] |= 0x10;
		}
	}
}


/*
 *  Memory configuration has probably changed
 */

void MOS6510::new_config()
{
	if ((ram[0] & 0x10) == 0) {
		if (tape_sense) {
			ram[1] &= 0xef;
		} else {
			ram[1] |= 0x10;
		}
	}

	uint8_t port = ~ram[0] | ram[1];

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
}


/*
 *  Read a byte from I/O / ROM space
 */

inline uint8_t MOS6510::read_byte_io(uint16_t adr)
{
	switch (adr >> 12) {
		case 0x8:	// Cartridge ROML or RAM
		case 0x9:
			if (the_cart->notEXROM) {
				return ram[adr];
			} else {
				return the_cart->ReadROML(adr & 0x1fff, ram[adr], basic_in);
			}
		case 0xa:	// Cartridge ROMH or RAM or BASIC ROM
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
						return color_ram[adr & 0x03ff] | (rand() & 0xf0);
					case 0xc:	// CIA 1
						return the_cia1->ReadRegister(adr & 0x0f);
					case 0xd:	// CIA 2
						return the_cia2->ReadRegister(adr & 0x0f);
					case 0xe:	// Cartridge I/O 1 (or open)
						return the_cart->ReadIO1(adr & 0xff, rand());
					case 0xf:	// Cartridge I/O 2 (or open)
						if (adr < 0xdfa0) {
							return the_cart->ReadIO2(adr & 0xff, rand());
						} else {
							return read_emulator_id(adr & 0x7f);
						}
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
		return ram[adr];
	} else {
		return read_byte_io(adr);
	}
}


/*
 *  $dfa0-$dfff: Emulator identification
 */

const char frodo_id[0x5c] = "FRODO\r(C) CHRISTIAN BAUER";

uint8_t MOS6510::read_emulator_id(uint16_t adr)
{
	switch (adr) {
		case 0x7c:	// $dffc: revision
			return FRODO_REVISION << 4;
		case 0x7d:	// $dffd: version
			return FRODO_VERSION;
		case 0x7e:	// $dffe returns 'F' (Frodo ID)
			return 'F';
		case 0x7f:	// $dfff alternates between $55 and $aa
			dfff_byte = ~dfff_byte;
			return dfff_byte;
		default:
			return frodo_id[adr - 0x20];
	}
}


/*
 *  Read a word (little-endian) from the CPU's address space
 */

#if LITTLE_ENDIAN_UNALIGNED

inline uint16_t MOS6510::read_word(uint16_t adr)
{
	switch (adr >> 12) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			return *(uint16_t *)&ram[adr];
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			return read_byte_io(adr) | (read_byte_io(adr + 1) << 8);
		case 0xc:
			return *(uint16_t *)&ram[adr];
		case 0xd:
			if (io_in) {
				return read_byte_io(adr) | (read_byte_io(adr + 1) << 8);
			} else if (char_in) {
				return *(uint16_t *)&char_rom[adr & 0x0fff];
			} else {
				return *(uint16_t *)&ram[adr];
			}
		case 0xe:
		case 0xf:
			if (kernal_in) {
				return *(uint16_t *)&kernal_rom[adr & 0x1fff];
			} else {
				return *(uint16_t *)&ram[adr];
			}
		default:	// Can't happen
			return 0;
	}
}

#else

inline uint16_t MOS6510::read_word(uint16_t adr)
{
	return read_byte(adr) | (read_byte(adr + 1) << 8);
}

#endif


/*
 *  Write byte to I/O space
 */

void MOS6510::write_byte_io(uint16_t adr, uint8_t byte)
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

inline void MOS6510::write_byte(uint16_t adr, uint8_t byte)
{
	if (adr < 0xd000) {
		ram[adr] = byte;
		if (adr < 2) {
			new_config();
		}
	} else {
		write_byte_io(adr, byte);
	}
}


/*
 *  Read a byte from the zeropage
 */

inline uint8_t MOS6510::read_zp(uint16_t adr)
{
	return ram[adr & 0xff];
}


/*
 *  Read a word (little-endian) from the zeropage
 */

inline uint16_t MOS6510::read_zp_word(uint16_t adr)
{
    // Zeropage word addressing wraps around
	return ram[adr & 0xff] | (ram[(adr+1) & 0xff] << 8);
}


/*
 *  Write a byte to the zeropage
 */

inline void MOS6510::write_zp(uint16_t adr, uint8_t byte)
{
	ram[adr & 0xff] = byte;

	// Check if memory configuration may have changed.
	if (adr < 2) {
		new_config();
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

void MOS6510::do_adc(uint8_t byte)
{
	if (!d_flag) {
		uint16_t tmp = a + (byte) + (c_flag ? 1 : 0);
		c_flag = tmp > 0xff;
		v_flag = !((a ^ (byte)) & 0x80) && ((a ^ tmp) & 0x80);
		z_flag = n_flag = a = tmp;
	} else {
		uint16_t al, ah;
		al = (a & 0x0f) + ((byte) & 0x0f) + (c_flag ? 1 : 0);
		if (al > 9) al += 6;
		ah = (a >> 4) + ((byte) >> 4);
		if (al > 0x0f) ah++;
		z_flag = a + (byte) + (c_flag ? 1 : 0);
		n_flag = ah << 4;
		v_flag = (((ah << 4) ^ a) & 0x80) && !((a ^ (byte)) & 0x80);
		if (ah > 9) ah += 6;
		c_flag = ah > 0x0f;
		a = (ah << 4) | (al & 0x0f);
	}
}


/*
 *  SBC instruction
 */

void MOS6510::do_sbc(uint8_t byte)
{
	uint16_t tmp = a - (byte) - (c_flag ? 0 : 1);
	if (!d_flag) {
		c_flag = tmp < 0x100;
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ (byte)) & 0x80);
		z_flag = n_flag = a = tmp;
	} else {
		uint16_t al, ah;
		al = (a & 0x0f) - ((byte) & 0x0f) - (c_flag ? 0 : 1);
		ah = (a >> 4) - ((byte) >> 4);
		if (al & 0x10) {
			al -= 6;
			ah--;
		}
		if (ah & 0x10) ah -= 6;
		c_flag = tmp < 0x100;
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ (byte)) & 0x80);
		z_flag = n_flag = tmp;
		a = (ah << 4) | (al & 0x0f);
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
}


/*
 *  Emulate cycles_left worth of 6510 instructions
 *  Returns number of cycles of last instruction
 */

int MOS6510::EmulateLine(int cycles_left)
{
	uint8_t tmp, tmp2;
	uint16_t adr, tmp_adr;

	int last_cycles = 0;

#define RESET_PENDING (int_line[INT_RESET])
#define IRQ_PENDING (int_line[INT_VICIRQ] || int_line[INT_CIAIRQ])
#define CHECK_SO ;

#include "CPU_emulline.h"

		// Extension opcode
		case 0xf2:
			if ((pc < 0xa000) || (pc >= 0xc000 && pc < 0xe000)) {
				illegal_op(pc - 1);
			} else switch (read_byte_imm()) {
				case 0x00:
					ram[0x90] |= the_iec->Out(ram[0x95], ram[0xa3] & 0x80);
					c_flag = false;
					jump(0xedac);
					break;
				case 0x01:
					ram[0x90] |= the_iec->OutATN(ram[0x95]);
					c_flag = false;
					jump(0xedac);
					break;
				case 0x02:
					ram[0x90] |= the_iec->OutSec(ram[0x95]);
					c_flag = false;
					jump(0xedac);
					break;
				case 0x03:
					ram[0x90] |= the_iec->In(a);
					set_nz(a);
					c_flag = false;
					jump(0xedac);
					break;
				case 0x04:
					the_iec->SetATN();
					jump(0xedfb);
					break;
				case 0x05:
					the_iec->RelATN();
					jump(0xedac);
					break;
				case 0x06:
					the_iec->Turnaround();
					jump(0xedac);
					break;
				case 0x07:
					the_iec->Release();
					jump(0xedac);
					break;
				case 0x10:
					the_c64->AutoStartOp();
					x = 0;	// patch replaces LDX #0
					break;
				default:
					illegal_op(pc - 1);
					break;
			}
			ENDOP(2);
		}
	}

	return last_cycles;
}
