/*
 *  CPU1541_SC.cpp - Single-cycle 6502 (1541) emulation
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
 *    next cycle. Its upper 8 bits contain the current opcode, its lower
 *    8 bits contain the cycle number (0..7) within the opcode.
 *  - Opcodes are fetched in cycle 0 (state = 0)
 *  - The states 0x0008..0x0017 are used for interrupts
 *  - There is exactly one memory access in each clock cycle
 *
 * Memory map (from 1541-II):
 *
 * $0000-$07ff RAM (2K)
 * $0800-$17ff open
 * $1800-$1bff VIA 1
 * $1c00-$1fff VIA 2
 * $2000-$7fff mirrors of the above
 * $8000-$bfff ROM mirror
 * $c000-$ffff ROM (16K)
 *
 *  - All memory accesses are done with the read_byte() and
 *    write_byte() functions which also do the memory address
 *    decoding.
 *  - The possible interrupt sources are:
 *      INT_VIA1IRQ: I flag is checked, jump to ($fffe)
 *      INT_VIA2IRQ: I flag is checked, jump to ($fffe)
 *      INT_RESET1541: Jump to ($fffc)
 *  - The z_flag variable has the inverse meaning of the
 *    6502 Z flag
 *  - Only the highest bit of the n_flag variable is used
 *  - The $f2 opcode that would normally crash the 6502 is
 *    used to implement emulator-specific functions
 */

#include "sysdeps.h"

#include "CPU1541.h"
#include "CPU_common.h"
#include "1541gcr.h"
#include "C64.h"
#include "CIA.h"
#include "IEC.h"

#include <format>


/*
 *  6502 constructor: Initialize registers
 */

MOS6502_1541::MOS6502_1541(C64 * c64, GCRDisk * gcr, uint8_t * Ram, uint8_t * Rom)
 : ram(Ram), rom(Rom), the_c64(c64), the_gcr_disk(gcr)
{
	a = x = y = 0;
	sp = 0xff;
	n_flag = z_flag = 0;
	v_flag = d_flag = c_flag = false;
	i_flag = true;

	cycle_counter = 0;

	int_line[INT_VIA1IRQ] = false;
	int_line[INT_VIA2IRQ] = false;
	int_line[INT_RESET1541] = false;

	irq_pending = false;
	irq_delay = 0;

	nmi_triggered = false;
	nmi_pending = false;

	via1 = new MOS6522(this, INT_VIA1IRQ);
	via2 = new MOS6522(this, INT_VIA2IRQ);

	Reset();
}


/*
 *  6502 destructor
 */

MOS6502_1541::~MOS6502_1541()
{
	delete via1;
	delete via2;
}


/*
 *  Reset CPU asynchronously
 */

void MOS6502_1541::AsyncReset()
{
	int_line[INT_RESET1541] = true;
	Idle = false;
}


/*
 *  Reset 1541
 */

void MOS6502_1541::Reset()
{
	// Clear all interrupt lines
	int_line[INT_VIA1IRQ] = false;
	int_line[INT_VIA2IRQ] = false;
	int_line[INT_RESET1541] = false;

	irq_pending = false;

	nmi_triggered = false;
	nmi_pending = false;

	// Read reset vector
	pc = read_word(0xfffc);
	state = O_FETCH;
	jammed = false;

	// IEC lines and VIA registers
	IECLines = 0x38;
	atn_ack = 0x08;

	via1->Reset();
	via2->Reset();

	// Wake up 1541
	Idle = false;
}


/*
 *  Get 1541 register state
 */

void MOS6502_1541::GetState(MOS6502State *s) const
{
	s->cycle_counter = cycle_counter;

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

	s->int_line[INT_VIA1IRQ] = int_line[INT_VIA1IRQ];
	s->int_line[INT_VIA2IRQ] = int_line[INT_VIA2IRQ];

	s->irq_pending = irq_pending;
	s->irq_delay = irq_delay;

	s->instruction_complete = (state == O_FETCH);
	s->state = state;
	s->op = op;
	s->ar = ar;
	s->ar2 = ar2;
	s->rdbuf = rdbuf;

	s->idle = Idle;

	via1->GetState(&(s->via1));
	via2->GetState(&(s->via2));
}


/*
 *  Restore 1541 state
 */

void MOS6502_1541::SetState(const MOS6502State *s)
{
	cycle_counter = s->cycle_counter;

	a = s->a;
	x = s->x;
	y = s->y;

	n_flag = s->p;
	v_flag = s->p & 0x40;
	d_flag = s->p & 0x08;
	i_flag = s->p & 0x04;
	z_flag = !(s->p & 0x02);
	c_flag = s->p & 0x01;

	pc = s->pc;
	sp = s->sp & 0xff;

	int_line[INT_VIA1IRQ] = s->int_line[INT_VIA1IRQ];
	int_line[INT_VIA2IRQ] = s->int_line[INT_VIA2IRQ];

	irq_pending = s->irq_pending;
	irq_delay = s->irq_delay;

	state = s->state;
	op = s->op;
	ar = s->ar;
	ar2 = s->ar2;
	rdbuf = s->rdbuf;

	Idle = s->idle;

	via1->SetState(&(s->via1));
	via2->SetState(&(s->via2));

	set_iec_lines(~(via1->PBOut()));
}


/*
 *  Return physical state of IEC lines
 */

uint8_t MOS6502_1541::CalcIECLines() const
{
	uint8_t iec = IECLines & TheCIA2->IECLines;
	iec &= ((iec ^ atn_ack) << 2) | 0xdf;	// ATN acknowledge pulls DATA low
	return iec;
}


/*
 *  Trigger VIA interrupt
 */

void MOS6502_1541::TriggerInterrupt(unsigned which)
{
	if (!(int_line[INT_VIA1IRQ] || int_line[INT_VIA2IRQ])) {
		irq_delay = 3;	// Two cycles delay until recognition
	}
	int_line[which] = true;

	// Wake up 1541
	Idle = false;
}

// Interrupt by negative edge of ATN on IEC bus
void MOS6502_1541::TriggerIECInterrupt()
{
	via1->TriggerCA1Interrupt();
}


void MOS6502_1541::EmulateVIACycle()
{
	++cycle_counter;

	via1->EmulateCycle();
	via2->EmulateCycle();
}


/*
 *  Read a byte from the CPU's address space
 */

uint8_t MOS6502_1541::read_byte(uint16_t adr)
{
	if (adr >= 0x8000) {

		// ROM
		return rom[adr & 0x3fff];

	} else if ((adr & 0x1800) == 0x0000) {

		// RAM
		return ram[adr & 0x07ff];

	} else if ((adr & 0x1c00) == 0x1800) {

		// VIA 1
		switch (adr & 0xf) {
			case 0: {	// Port B
				uint8_t iec = ~CalcIECLines();		// 1541 reads inverted bus lines
				uint8_t in = ((iec & 0x20) >> 5)	// DATA from bus on PB0
						   | ((iec & 0x10) >> 2)	// CLK from bus on PB2
						   | ((iec & 0x08) << 4)	// ATN from bus on PB7
						   | 0x1a;					// Output lines high
				via1->SetPBIn(in);
				break;
			}
			case 1:		// Port A
			case 15:	// Port A (no handshake)
				via1->SetPAIn(0xff);
				break;
		}
		return via1->ReadRegister(adr);

	} else if ((adr & 0x1c00) == 0x1c00) {

		// VIA 2
		switch (adr & 0xf) {
			case 0: {	// Port B
				uint8_t in = the_gcr_disk->WPSensorClosed(cycle_counter) ? 0 : 0x10;
				if (!the_gcr_disk->SyncFound(cycle_counter)) {
					in |= 0x80;
				}
				via2->SetPBIn(in);
				break;
			}
			case 1:		// Port A
			case 15:	// Port A (no handshake)
				uint8_t in = the_gcr_disk->ReadGCRByte(cycle_counter);
				via2->SetPAIn(in);
				break;
		}
		return via2->ReadRegister(adr);

	} else {

		// Open address
		return adr >> 8;
	}
}


/*
 *  Read a word (little-endian) from the CPU's address space
 */

inline uint16_t MOS6502_1541::read_word(uint16_t adr)
{
	return read_byte(adr) | (read_byte(adr + 1) << 8);
}


/*
 *  Set state of 1541 IEC lines from inverted VIA 1 port B output
 */

void MOS6502_1541::set_iec_lines(uint8_t inv_out)
{
	IECLines = ((inv_out & 0x02) << 4)	// DATA on PB1
	         | ((inv_out & 0x08) << 1)	// CLK on PB3
	         | 0x08;					// No output on ATN

	atn_ack = (~inv_out & 0x10) >> 1;	// PB4
}


/*
 *  Write a byte to the CPU's address space
 */

inline void MOS6502_1541::write_byte(uint16_t adr, uint8_t byte)
{
	if (adr >= 0x8000) {

		// ignore writes to ROM

	} else if ((adr & 0x1800) == 0x0000) {

		// RAM
		ram[adr & 0x07ff] = byte;

	} else if ((adr & 0x1c00) == 0x1800) {

		// VIA 1
		via1->WriteRegister(adr, byte);

		switch (adr & 0xf) {
			case 0:	// Port B
			case 2:	// DDR B
				set_iec_lines(~(via1->PBOut()));
				break;
		}

	} else if ((adr & 0x1c00) == 0x1c00) {

		// VIA 2
		uint8_t old_pb_out = via2->PBOut();

		via2->WriteRegister(adr, byte);

		switch (adr & 0xf) {
			case 0:		// Port B
			case 2: {	// DDR B
				uint8_t pb_out = via2->PBOut();
				uint8_t changed = old_pb_out ^ pb_out;

				// Bits 0/1: Stepper motor
				if (changed & 0x03) {
					if ((old_pb_out & 3) == ((pb_out + 1) & 3)) {
						the_gcr_disk->MoveHeadOut();
					} else if ((old_pb_out & 3) == ((pb_out - 1) & 3)) {
						the_gcr_disk->MoveHeadIn();
					}
				}

				// Bit 2: Spindle motor
				if (changed & 0x04) {
					the_gcr_disk->SetMotor(pb_out & 0x04);
				}

				// Bit 3: Drive LED
				int led_status;
				if (ram[0x26c] != 0 && ram[0x7c] == 0) {
					// Error flag on and no attention pending
					// (Note: flags may change even if port bit stays the same)
					led_status = (pb_out & 0x08) ? DRVLED_ERROR_ON : DRVLED_ERROR_OFF;
				} else {
					led_status = (pb_out & 0x08) ? DRVLED_ON : DRVLED_OFF;
				}
				the_c64->SetDriveLEDs(led_status, DRVLED_OFF, DRVLED_OFF, DRVLED_OFF);

				// Bits 5/6: GCR bit rate
				if (changed & 0x60) {
					the_gcr_disk->SetBitRate((pb_out >> 5) & 0x03);
				}
				break;
			}
		}
	}
}


/*
 *  Read byte from 6502/1541 address space (used by SAM)
 */

uint8_t MOS6502_1541::ExtReadByte(uint16_t adr)
{
	return read_byte(adr);
}


/*
 *  Write byte to 6502/1541 address space (used by SAM)
 */

void MOS6502_1541::ExtWriteByte(uint16_t adr, uint8_t byte)
{
	write_byte(adr, byte);
}


/*
 *  ADC instruction
 */

inline void MOS6502_1541::do_adc(uint8_t byte)
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

inline void MOS6502_1541::do_sbc(uint8_t byte)
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

void MOS6502_1541::illegal_op(uint16_t adr)
{
	// Notify user once
	if (! jammed) {
		std::string s = std::format("1541 crashed at ${:04X}, press F12 to reset", adr);
		the_c64->ShowNotification(s);
		jammed = true;
	}

	// Keep executing opcode
	--pc;
	state = O_FETCH;
}


/*
 *  Emulate one CPU clock cycle
 */

// Read byte from memory
#define read_to(adr, to) \
	to = read_byte(adr);

// Read byte from memory, throw away result
#define read_idle(adr) \
	read_byte(adr);

// Check for pending interrupts
void MOS6502_1541::check_interrupts()
{
	if ((int_line[INT_VIA1IRQ] || int_line[INT_VIA2IRQ]) && !i_flag && irq_delay == 0 && !jammed) {
		irq_pending = true;
	}
}

void MOS6502_1541::EmulateCPUCycle()
{
	uint8_t data, tmp;

	// Shift delay lines
	irq_delay >>= 1;

#define IS_CPU_1541
#define RESET_PENDING (int_line[INT_RESET1541])
#define CHECK_SO \
	if (set_overflow_enabled() && the_gcr_disk->ByteReady(cycle_counter)) { \
		v_flag = true; \
	}

#include "CPU_emulcycle.h"

		// Extension opcode
		case O_EXT:
			if (pc < 0xc000) {
				illegal_op(pc - 1);
				break;
			}
			switch (read_byte(pc++)) {
				case 0x00:	// Go to sleep in DOS idle loop if error flag is clear and no attention pending
					Idle = !(ram[0x26c] | ram[0x7c]);
					pc = 0xebff;
					Last;
				case 0x01:	// Write sector
					the_gcr_disk->WriteSector();
					pc = 0xf5dc;
					Last;
				case 0x02:	// Format track
					the_gcr_disk->FormatTrack();
					pc = 0xfd8b;
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
