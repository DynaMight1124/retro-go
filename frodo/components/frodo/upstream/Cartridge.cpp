/*
 *  Cartridge.cpp - Cartridge emulation
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

#include "sysdeps.h"

#include "Cartridge.h"

#include <filesystem>
namespace fs = std::filesystem;


// Base class for cartridge with ROM
ROMCartridge::ROMCartridge(unsigned num_banks, unsigned bank_size) : numBanks(num_banks), bankSize(bank_size)
{
	// Allocate ROM
	rom = new uint8_t[num_banks * bank_size];
	memset(rom, 0xff, num_banks * bank_size);
}

ROMCartridge::~ROMCartridge()
{
	// Free ROM
	delete[] rom;
}


// 8K ROM cartridge (EXROM = 0, GAME = 1)
Cartridge8K::Cartridge8K() : ROMCartridge(1, 0x2000)
{
	notEXROM = false;
}

uint8_t Cartridge8K::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr] : ram_byte;
}


// 16K ROM cartridge (EXROM = 0, GAME = 0)
Cartridge16K::Cartridge16K() : ROMCartridge(1, 0x4000)
{
	notEXROM = false;
	notGAME = false;
}

uint8_t Cartridge16K::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr] : ram_byte;
}

uint8_t Cartridge16K::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + 0x2000] : ram_byte;
}


// Simons' BASIC cartridge (switchable 8K/16K ROM cartridge)
CartridgeSimonsBasic::CartridgeSimonsBasic() : ROMCartridge(1, 0x4000)
{
	notEXROM = false;
	notGAME = true;
}

void CartridgeSimonsBasic::Reset()
{
	notGAME = true;
}

uint8_t CartridgeSimonsBasic::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr] : ram_byte;
}

uint8_t CartridgeSimonsBasic::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + 0x2000] : ram_byte;
}

uint8_t CartridgeSimonsBasic::ReadIO1(uint16_t adr, uint8_t bus_byte)
{
	notGAME = true;		// 8K mode
	return bus_byte;
}

void CartridgeSimonsBasic::WriteIO1(uint16_t adr, uint8_t byte)
{
	notGAME = false;	// 16K mode
}


// Ocean cartridge (banked 8K/16K ROM cartridge)
CartridgeOcean::CartridgeOcean(bool not_game) : ROMCartridge(64, 0x2000)
{
	notEXROM = false;
	notGAME = not_game;
}

void CartridgeOcean::Reset()
{
	bank = 0;
}

uint8_t CartridgeOcean::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

uint8_t CartridgeOcean::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + bank * bankSize] : ram_byte;
}

void CartridgeOcean::WriteIO1(uint16_t adr, uint8_t byte)
{
	bank = byte & 0x3f;
}


// Fun Play cartridge (banked 8K ROM cartridge)
CartridgeFunPlay::CartridgeFunPlay() : ROMCartridge(64, 0x2000)
{
	notEXROM = false;
}

void CartridgeFunPlay::Reset()
{
	notEXROM = false;

	bank = 0;
}

uint8_t CartridgeFunPlay::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

void CartridgeFunPlay::WriteIO1(uint16_t adr, uint8_t byte)
{
	bank = byte & 0x39;
	notEXROM = (byte & 0xc6) == 0x86;
}


// Super Games cartridge (banked 16K ROM cartridge)
CartridgeSuperGames::CartridgeSuperGames() : ROMCartridge(4, 0x4000)
{
	notEXROM = false;
	notGAME = false;
}

void CartridgeSuperGames::Reset()
{
	notEXROM = false;
	notGAME = false;

	bank = 0;
	disableIO2 = false;
}

uint8_t CartridgeSuperGames::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

uint8_t CartridgeSuperGames::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + bank * bankSize + 0x2000] : ram_byte;
}

void CartridgeSuperGames::WriteIO2(uint16_t adr, uint8_t byte)
{
	if (! disableIO2) {
		bank = byte & 0x03;
		notEXROM = notGAME = byte & 0x04;
		disableIO2 = byte & 0x08;
	}
}


// C64 Games System cartridge (banked 8K ROM cartridge)
CartridgeC64GS::CartridgeC64GS() : ROMCartridge(64, 0x2000)
{
	notEXROM = false;
}

void CartridgeC64GS::Reset()
{
	bank = 0;
}

uint8_t CartridgeC64GS::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

uint8_t CartridgeC64GS::ReadIO1(uint16_t adr, uint8_t bus_byte)
{
	bank = adr & 0x3f;
	return bus_byte;
}

void CartridgeC64GS::WriteIO1(uint16_t adr, uint8_t byte)
{
	bank = adr & 0x3f;
}


// Dinamic cartridge (banked 8K ROM cartridge)
CartridgeDinamic::CartridgeDinamic() : ROMCartridge(16, 0x2000)
{
	notEXROM = false;
}

void CartridgeDinamic::Reset()
{
	bank = 0;
}

uint8_t CartridgeDinamic::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

uint8_t CartridgeDinamic::ReadIO1(uint16_t adr, uint8_t bus_byte)
{
	bank = adr & 0x0f;
	return bus_byte;
}


// Zaxxon cartridge (banked 16K ROM cartridge)
CartridgeZaxxon::CartridgeZaxxon() : ROMCartridge(3, 0x2000)
{
	notEXROM = false;
	notGAME = false;
}

void CartridgeZaxxon::Reset()
{
	bank = 0;
}

uint8_t CartridgeZaxxon::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	if (notLoram) {
		if (adr < 0x1000) {
			bank = 0;
			return rom[adr];
		} else {
			bank = 1;
			return rom[adr & 0xfff];
		}
	} else {
		return ram_byte;
	}
}

uint8_t CartridgeZaxxon::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + bank * bankSize + 0x2000] : ram_byte;
}


// Magic Desk / Marina64 cartridge (banked 8K ROM cartridge)
CartridgeMagicDesk::CartridgeMagicDesk() : ROMCartridge(128, 0x2000)
{
	notEXROM = false;
}

void CartridgeMagicDesk::Reset()
{
	notEXROM = false;

	bank = 0;
}

uint8_t CartridgeMagicDesk::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

void CartridgeMagicDesk::WriteIO1(uint16_t adr, uint8_t byte)
{
	bank = byte & 0x7f;
	notEXROM = byte & 0x80;
}


// COMAL 80 cartridge (banked 16K ROM cartridge)
CartridgeComal80::CartridgeComal80() : ROMCartridge(4, 0x4000)
{
	notEXROM = false;
	notGAME = false;
}

void CartridgeComal80::Reset()
{
	bank = 0;
}

uint8_t CartridgeComal80::ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
{
	return notLoram ? rom[adr + bank * bankSize] : ram_byte;
}

uint8_t CartridgeComal80::ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
{
	return notHiram ? rom[adr + bank * bankSize + 0x2000] : ram_byte;
}

void CartridgeComal80::WriteIO1(uint16_t adr, uint8_t byte)
{
	bank = byte & 0x03;
}


/*
 *  Check whether file is a cartridge image file
 */

bool IsCartridgeFile(const std::string & path)
{
	// Reject directories
	if (fs::is_directory(path))
		return false;

	// Read file header
	FILE * f = fopen(path.c_str(), "rb");
	if (f == nullptr)
		return false;

	uint8_t header[64];
	if (fread(header, sizeof(header), 1, f) != 1) {
		fclose(f);
		return false;
	}

	fclose(f);

	// Check for signature and version
	uint16_t version = (header[0x14] << 8) | header[0x15];
	return memcmp(header, "C64 CARTRIDGE   ", 16) == 0 && version == 0x0100;
}


/*
 *  Construct cartridge object from image file path, return nullptr on error
 */

Cartridge * Cartridge::FromFile(const std::string & path, std::string & ret_error_msg)
{
	// Empty path = no cartridge
	if (path.empty())
		return nullptr;

	ROMCartridge * cart = nullptr;
	FILE * f = nullptr;
	{
		// Read file header
		f = fopen(path.c_str(), "rb");
		if (f == nullptr) {
			ret_error_msg = "Can't open cartridge file";
			return nullptr;
		}

		uint8_t header[64];
		if (fread(header, sizeof(header), 1, f) != 1)
			goto error_read;

		// Check for signature and version
		uint16_t version = (header[0x14] << 8) | header[0x15];
		if (memcmp(header, "C64 CARTRIDGE   ", 16) != 0 || version != 0x0100)
			goto error_unsupp;

		// Create cartridge object according to type
		uint16_t type = (header[0x16] << 8) | header[0x17];
		uint8_t exrom = header[0x18];
		uint8_t game = header[0x19];

		switch (type) {
			case 0:
				if (exrom != 0)		// Ultimax or not a ROM cartridge
					goto error_unsupp;
				if (game == 0) {
					cart = new Cartridge16K;
				} else {
					cart = new Cartridge8K;
				}
				break;
			case 4:
				cart = new CartridgeSimonsBasic;
				break;
			case 5:
				cart = new CartridgeOcean(game);
				break;
			case 7:
				cart = new CartridgeFunPlay;
				break;
			case 8:
				cart = new CartridgeSuperGames;
				break;
			case 15:
				cart = new CartridgeC64GS;
				break;
			case 17:
				cart = new CartridgeDinamic;
				break;
			case 18:
				cart = new CartridgeZaxxon;
				break;
			case 19:
				cart = new CartridgeMagicDesk;
				break;
			case 21:
				cart = new CartridgeComal80;
				break;
			default:
				goto error_unsupp;
		}

		// Load CHIP packets
		while (true) {

			// Load packet header
			size_t actual = fread(header, 1, 16, f);
			if (actual == 0)
				break;
			if (actual != 16)
				goto error_read;

			// Check for signature and size
			uint16_t chip_type  = (header[0x08] << 8) | header[0x09];
			uint16_t chip_bank  = (header[0x0a] << 8) | header[0x0b];
			uint16_t chip_start = (header[0x0c] << 8) | header[0x0d];
			uint16_t chip_size  = (header[0x0e] << 8) | header[0x0f];
			if (memcmp(header, "CHIP", 4) != 0 || chip_type != 0 || chip_bank >= cart->numBanks || chip_size > cart->bankSize)
				goto error_unsupp;

			// Load packet contents
			uint32_t offset = chip_bank * cart->bankSize;

			if (type == 4 && chip_start == 0xa000) {			// Special mapping for Simons' BASIC
				offset = 0x2000;
			} else if (type == 18 && chip_start == 0xa000) {	// Special mapping for Zaxxon
				offset += 0x2000;
			}

			if (fread(cart->ROM() + offset, chip_size, 1, f) != 1)
				goto error_read;
		}

		fclose(f);
	}
	return cart;

error_read:
	delete cart;
	fclose(f);

	ret_error_msg = "Error reading cartridge file";
	return nullptr;

error_unsupp:
	delete cart;
	fclose(f);

	ret_error_msg = "Unsupported cartridge type";
	return nullptr;
}
