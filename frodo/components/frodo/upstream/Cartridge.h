/*
 *  Cartridge.h - Cartridge emulation
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

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "sysdeps.h"

#include <string>


// Base class for cartridges
class Cartridge {
public:
	Cartridge() { }
	virtual ~Cartridge() { }

	static Cartridge * FromFile(const std::string & path, std::string & ret_error_msg);

	virtual void Reset() { }

	// Default for $8000..$9fff is to read RAM
	virtual uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram)
	{
		return ram_byte;
	}

	// Default for $a000..$bfff is to read RAM or BASIC ROM, depending on /LORAM
	virtual uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram)
	{
		return notLoram ? basic_byte : ram_byte;
	}

	// Default for I/O 1 and 2 is open bus
	virtual uint8_t ReadIO1(uint16_t adr, uint8_t bus_byte) { return bus_byte; }
	virtual void WriteIO1(uint16_t adr, uint8_t byte) { }
	virtual uint8_t ReadIO2(uint16_t adr, uint8_t bus_byte) { return bus_byte; }
	virtual void WriteIO2(uint16_t adr, uint8_t byte) { }

	virtual void FF00Trigger() { }

	// Memory mapping control lines
	bool notEXROM = true;
	bool notGAME = true;
};


// No cartridge
class NoCartridge : public Cartridge {
public:
	NoCartridge() { }
	~NoCartridge() { }
};


// Base class for cartridge with ROM
class ROMCartridge : public Cartridge {
public:
	ROMCartridge(unsigned num_banks, unsigned bank_size);
	~ROMCartridge();

	uint8_t * ROM() const { return rom; }

	const unsigned numBanks;
	const unsigned bankSize;

protected:
	uint8_t * rom = nullptr;	// Pointer to ROM contents
};


// 8K ROM cartridge (EXROM = 0, GAME = 1)
class Cartridge8K : public ROMCartridge {
public:
	Cartridge8K();

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
};


// 16K ROM cartridge (EXROM = 0, GAME = 0)
class Cartridge16K : public ROMCartridge {
public:
	Cartridge16K();

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;
};


// Simons' BASIC cartridge (switchable 8K/16K ROM cartridge)
class CartridgeSimonsBasic : public ROMCartridge {
public:
	CartridgeSimonsBasic();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;

	uint8_t ReadIO1(uint16_t adr, uint8_t bus_byte) override;
	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// Ocean cartridge (banked 8K/16K ROM cartridge)
class CartridgeOcean : public ROMCartridge {
public:
	CartridgeOcean(bool not_game);

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;

	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// Fun Play / Power Play cartridge (banked 8K ROM cartridge)
class CartridgeFunPlay : public ROMCartridge {
public:
	CartridgeFunPlay();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;

	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// Super Games cartridge (banked 16K ROM cartridge)
class CartridgeSuperGames : public ROMCartridge {
public:
	CartridgeSuperGames();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;

	void WriteIO2(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;			// Selected bank
	bool disableIO2 = false;	// Flag: I/O 2 area disabled
};


// C64 Games System / System 3 cartridge (banked 8K ROM cartridge)
class CartridgeC64GS : public ROMCartridge {
public:
	CartridgeC64GS();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;

	uint8_t ReadIO1(uint16_t adr, uint8_t bus_byte) override;
	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// Dinamic cartridge (banked 8K ROM cartridge)
class CartridgeDinamic : public ROMCartridge {
public:
	CartridgeDinamic();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;

	uint8_t ReadIO1(uint16_t adr, uint8_t bus_byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// Zaxxon cartridge (banked 16K ROM cartridge)
class CartridgeZaxxon : public ROMCartridge {
public:
	CartridgeZaxxon();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;

protected:
	unsigned bank = 0;	// Selected ROMH bank
};


// Magic Desk cartridge (banked 8K ROM cartridge)
class CartridgeMagicDesk : public ROMCartridge {
public:
	CartridgeMagicDesk();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;

	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected bank
};


// COMAL 80 cartridge (banked 16K ROM cartridge)
class CartridgeComal80 : public ROMCartridge {
public:
	CartridgeComal80();

	void Reset() override;

	uint8_t ReadROML(uint16_t adr, uint8_t ram_byte, bool notLoram) override;
	uint8_t ReadROMH(uint16_t adr, uint8_t ram_byte, uint8_t basic_byte, bool notLoram, bool notHiram) override;

	void WriteIO1(uint16_t adr, uint8_t byte) override;

protected:
	unsigned bank = 0;	// Selected ROMH bank
};


/*
 *  Functions
 */

// Check whether file is a cartridge image file
extern bool IsCartridgeFile(const std::string & path);


#endif // ndef CARTRIDGE_H
