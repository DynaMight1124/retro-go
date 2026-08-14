/*
 *  REU.cpp - 17xx REU and GeoRAM emulation
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
 * Incompatibilities:
 * ------------------
 *
 *  - REU interrupts are not emulated.
 *  - Transfer time is not accounted for, all transfers are done in 0 cycles.
 */

#include "sysdeps.h"

#include "REU.h"
#include "CPUC64.h"
#include "Prefs.h"


/*
 *  REU constructor
 */

REU::REU(MOS6510 * cpu, int prefs_reu_size) : the_cpu(cpu)
{
	// Allocate expansion RAM
	if (prefs_reu_size != REU_NONE) {
		switch (prefs_reu_size) {
			case REU_128K:
				ram_size = 0x20000;
				break;
			case REU_256K:
				ram_size = 0x40000;
				break;
			case REU_512K:
			default:
				ram_size = 0x80000;
				break;
		}
		ram_mask = ram_size - 1;
		ex_ram = new uint8_t[ram_size];
	} else {
		ram_size = ram_mask = 0;
		ex_ram = nullptr;
	}

	// Clear expansion RAM
	memset(ex_ram, 0, ram_size);

	// Reset registers
	Reset();
}


/*
 *  REU destructor
 */

REU::~REU()
{
	// Free expansion RAM
	delete[] ex_ram;
}


/*
 *  Reset the REU
 */

void REU::Reset()
{
	// Set size bit in status register
	if (ram_size > 0x20000) {
		regs[0] = 0x10;
	} else {
		regs[0] = 0x00;
	}

	// FF00 disabled in command register
	regs[1] = 0x10;

	for (unsigned i = 2; i < 10; ++i) {
		regs[i] = 0;
	}

	// Transfer length = $ffff
	regs[7] = regs[8] = 0xff;

	// Unconnected registers
	for (unsigned i = 11; i < 16; ++i) {
		regs[i] = 0xff;
	}

	// Autoload registers
	autoload_c64_adr_lo = 0;
	autoload_c64_adr_hi = 0;
	autoload_reu_adr_lo = 0;
	autoload_reu_adr_hi = 0;
	autoload_reu_adr_bank = 0;
	autoload_length_lo = 0xff;
	autoload_length_hi = 0xff;
}


/*
 *  Read from REU register
 */

uint8_t REU::ReadIO2(uint16_t adr, uint8_t bus_byte)
{
	if (ex_ram == nullptr)
		return bus_byte;
	if ((adr & 0x1f) >= 0x10)
		return 0xff;

	switch (adr & 0xf) {
		case 0:{
			uint8_t ret = regs[0];
			regs[0] &= 0x1f;	// Clear status bits
			return ret;
		}
		case 6:
			return regs[6] | 0xf8;
		case 9:
			return regs[9] | 0x1f;
		case 10:
			return regs[10] | 0x3f;
		default:
			return regs[adr & 0xf];
	}
}


/*
 *  Write to REU register
 */

void REU::WriteIO2(uint16_t adr, uint8_t byte)
{
	if (ex_ram == nullptr)
		return;
	if ((adr & 0x1f) >= 0x10)
		return;

	switch (adr & 0xf) {
		case 0:		// Status register is read-only
		case 2:
			regs[2] = autoload_c64_adr_lo = byte;
			regs[3] = autoload_c64_adr_hi;
			break;
		case 3:
			regs[2] = autoload_c64_adr_lo;
			regs[3] = autoload_c64_adr_hi = byte;
			break;
		case 4:
			regs[4] = autoload_reu_adr_lo = byte;
			regs[5] = autoload_reu_adr_hi;
			break;
		case 5:
			regs[4] = autoload_reu_adr_lo;
			regs[5] = autoload_reu_adr_hi = byte;
			break;
		case 6:
			regs[6] = autoload_reu_adr_bank = byte;
			break;
		case 7:
			regs[7] = autoload_length_lo = byte;
			regs[8] = autoload_length_hi;
			break;
		case 8:
			regs[7] = autoload_length_lo;
			regs[8] = autoload_length_hi = byte;
			break;
		case 11:	// Unconnected registers
		case 12:
		case 13:
		case 14:
		case 15:
			break;
		case 1:		// Command register
			regs[1] = byte;
			if ((byte & 0x90) == 0x90) {
				execute_dma();
			}
			break;
		default:
			regs[adr & 0xf] = byte;
			break;
	}
}


/*
 *  CPU triggered REU by writing to $ff00
 */

void REU::FF00Trigger()
{
	if (ex_ram == nullptr)
		return;

	// Execute bit set and FF00 enabled?
	if ((regs[1] & 0x90) == 0x80) {
		execute_dma();
	}
}


/*
 *  Execute REU DMA transfer
 */

void REU::execute_dma()
{
	// Clear execute bit in command register
	regs[1] &= 0x7f;

	// Set FF00 disable bit in command register
	regs[1] |= 0x10;

	// Get C64 and REU transfer base addresses
	uint16_t c64_adr = regs[2] | (regs[3] << 8);
	uint32_t reu_adr = regs[4] | (regs[5] << 8) | (regs[6] << 16);

	// Get transfer length
	uint16_t length = regs[7] | (regs[8] << 8);

	// Calculate address increments
	unsigned c64_inc = (regs[10] & 0x80) ? 0 : 1;
	unsigned reu_inc = (regs[10] & 0x40) ? 0 : 1;

	// Do transfer
	bool verify_error = false;
	while (! verify_error) {
		switch (regs[1] & 3) {
			case 0:		// C64 -> REU
				ex_ram[reu_adr & ram_mask] = the_cpu->REUReadByte(c64_adr);
				break;
			case 1:		// C64 <- REU
				the_cpu->REUWriteByte(c64_adr, ex_ram[reu_adr & ram_mask]);
				break;
			case 2: {	// C64 <-> REU
				uint8_t tmp = the_cpu->REUReadByte(c64_adr);
				the_cpu->REUWriteByte(c64_adr, ex_ram[reu_adr & ram_mask]);
				ex_ram[reu_adr & ram_mask] = tmp;
				break;
			}
			case 3:		// Compare
				if (ex_ram[reu_adr & ram_mask] != the_cpu->REUReadByte(c64_adr)) {
					regs[0] |= 0x20;	// Verify error
					verify_error = true;
				}
				break;
		}

		c64_adr += c64_inc;
		reu_adr += reu_inc;
		if (length == 1) {
			regs[0] |= 0x40;	// Transfer finished
			break;
		}
		--length;
	}

	// Update address and length registers
	if (regs[1] & 0x20) {
		regs[2] = autoload_c64_adr_lo;
		regs[3] = autoload_c64_adr_hi;
		regs[4] = autoload_reu_adr_lo;
		regs[5] = autoload_reu_adr_hi;
		regs[6] = autoload_reu_adr_bank;
		regs[7] = autoload_length_lo;
		regs[8] = autoload_length_hi;
	} else {
		reu_adr &= ram_mask;
		regs[2] = c64_adr & 0xff;
		regs[3] = c64_adr >> 8;
		regs[4] = reu_adr & 0xff;
		regs[5] = (reu_adr >> 8) & 0xff;
		regs[6] = (reu_adr >> 16) & 0xff;
		regs[7] = length & 0xff;
		regs[8] = (length >> 8) & 0xff;
	}
}


/*
 *  GeoRAM constructor
 */

GeoRAM::GeoRAM()
{
	// Allocate expansion RAM (512K)
	ram_size = 0x80000;
	ex_ram = new uint8_t[ram_size];

	// Clear expansion RAM
	memset(ex_ram, 0, ram_size);

	// Reset registers
	Reset();
}


/*
 *  GeoRAM destructor
 */

GeoRAM::~GeoRAM()
{
	// Free expansion RAM
	delete[] ex_ram;
}


/*
 *  Reset GeoRAM
 */

void GeoRAM::Reset()
{
	// Reset registers
	track = sector = 0;
}


/*
 *  Read from GeoRAM expansion RAM
 */

uint8_t GeoRAM::ReadIO1(uint16_t adr, uint8_t bus_byte)
{
	return ex_ram[(track << 13) + (sector << 8) + adr];
}


/*
 *  Write to GeoRAM expansion RAM
 */

void GeoRAM::WriteIO1(uint16_t adr, uint8_t byte)
{
	ex_ram[(track << 13) + (sector << 8) + adr] = byte;
}


/*
 *  Read from GeoRAM register
 */

uint8_t GeoRAM::ReadIO2(uint16_t adr, uint8_t bus_byte)
{
	// Registers are write-only
	return bus_byte;
}


/*
 *  Write to GeoRAM register
 */

void GeoRAM::WriteIO2(uint16_t adr, uint8_t byte)
{
	if ((adr & 0xc1) == 0xc0) {
		track = byte & 0x3f;
	} else if ((adr & 0xc1) == 0xc1) {
		sector = byte & 0x1f;
	}
}
