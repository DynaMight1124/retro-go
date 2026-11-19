/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cpu.h"

VE_VMS_CPU::VE_VMS_CPU(VE_VMS_RAM *_ram, VE_VMS_ROM *_rom, VE_VMS_FLASH *_flash, VE_VMS_INTERRUPTS *_intHandler, bool hle)
{
	PC = 0;

	EXTOld = 0;
	EXTNew = 0;

	interruptLevel = 0;
	currentInterrupt = 0;
	interruptsMasked = false;
	state = 0;
	instructionCount = 0;
	interruptDepth = 0;
	for (int i = 0; i < 16; ++i) interruptQueue[i] = 0;
	
	ram = _ram;
	rom = _rom;
	flash = _flash;
	intHandler = _intHandler;
	IsHLE = hle;
	
	frequency = 100000.0;	//Default frequency (~879khz/6)
	
	P3_taken = true;
}

VE_VMS_CPU::~VE_VMS_CPU()
{
}

double VE_VMS_CPU::getCurrentFrequency()
{
	return frequency;
}
	
void VE_VMS_CPU::setFrequency(double f)
{
	frequency = f;
}

//Memory operations
///Reads a byte either from ROM or Flash, depending on value of EXT
byte VE_VMS_CPU::readByteRF(size_t address)
{
	if((ram->readByte_RAW(EXT) & 0x1) == 1)
		return flash->getByte(address);
	else return rom->readByte(address);
}

///Writes a byte either from ROM or Flash, depending on value of EXT
void VE_VMS_CPU::writeByteRF(size_t address, byte d)
{
	if((ram->readByte_RAW(EXT) & 0x1) == 1)
		flash->writeByte(address, d);
	else rom->writeByte(address, d);
}


//Read
byte *VE_VMS_CPU::readFromROM(size_t address, size_t byteCount)
{
	byte *data = new byte[byteCount];

	//Write data
	for(byte i = 0; i < byteCount; ++i)
		data[i] = readByteRF(address);

	return data;
}
byte *VE_VMS_CPU::readFromRAM(size_t address, size_t byteCount)
{
	byte *data = new byte[byteCount];

	//PSW decides to which bank we read/write in RAM
	byte PSW_data = ram->readByte_RAW(PSW);
	if((PSW_data & 2) != 0) address += 256;

	//Read data
	for(byte i = 0; i < byteCount; ++i)
		data[i] = ram->readByte(address);

	return data;
}

//Write
void VE_VMS_CPU::writeToROM(byte *data, size_t dataSize, size_t address)
{
	//Write data
	for(byte i = 0; i < dataSize; ++i)
		writeByteRF(address, data[i]);
}

void VE_VMS_CPU::writeToRAM(byte *data, size_t dataSize, size_t address)
{
	//PSW decides to which bank we read/write in RAM
	byte PSW_data = ram->readByte_RAW(PSW);
	if((PSW_data & 2) != 0) address += 256;

	//Write data
	for(byte i = 0; i < dataSize; ++i)
		ram->writeByte(address, data[i]);
}

//Address/Data functions
///Immediate
byte VE_VMS_CPU::getAddress_i8()
{

	//if(Debug) VMU_Debug.debug("Address i8: " + Integer.toHexString(address));

	return readByteRF(PC + 1) & 0xFF;
}
///Relative 8-bit (signed)
signed char VE_VMS_CPU::getAddress_r8()
{
	//if(Debug) VMU_Debug.debug("Address r8: " + Integer.toHexString(readByteRF(PC + 1)));
	return (signed char)readByteRF(PC + 1);
}
///Relative 8-bit when the opcode has bit specifiers (signed)
signed char VE_VMS_CPU::getAddress_r8_b3()
{
	//if(Debug) VMU_Debug.debug("Address r8 b3: " + Integer.toHexString(readByteRF(PC + 2)));
	return (signed char)readByteRF(PC + 2);
}
///Relative 16-bit
size_t VE_VMS_CPU::getAddress_r16()
{
	//if(Debug) VMU_Debug.debug("Address r16: " + Integer.toHexString((readByteRF(PC + 1) | (readByteRF(PC + 2) << 8)) & 0xFFFF));
	return (readByteRF(PC + 1) | (readByteRF(PC + 2) << 8)) & 0xFFFF;
}
///Direct 9-bit
size_t VE_VMS_CPU::getAddress_d9()
{
	//We need opcode to get 9th bit
	byte opcode = readByteRF(PC);

	byte d9_bit = (opcode & 0x01);
	byte operand = readByteRF(PC + 1) & 0xFF;

	//if(Debug) VMU_Debug.debug("Address d9: " + Integer.toHexString(((d9_bit << 8) | operand) & 0x1FF));


	//if(mainBank > 0) address += 256;

	return ((d9_bit << 8) | operand) & 0x1FF;
}
///Direct 9-bit with bit assignment
size_t VE_VMS_CPU::getAddress_d9_b3()
{
	//We need opcode to get 9th bit
	byte opcode = readByteRF(PC);

	byte d9_bit = ((opcode & 0x10) >> 4);
	byte operand = readByteRF(PC + 1) & 0xFF;

	//if(Debug) VMU_Debug.debug("Address d9 b3: " + Integer.toHexString(((d9_bit << 8) | operand) & 0x1FF));

	//if(mainBank > 0) address += 256;

	return ((d9_bit << 8) | operand) & 0x1FF;
}
///Get bits (b3)
byte VE_VMS_CPU::getAddress_b3()
{
	//We need opcode to get the bits
	//if(Debug) VMU_Debug.debug("Bits= " + Integer.toHexString((readByteRF(PC) & 0x7)));
	return (readByteRF(PC) & 0x7);
}
///Indirect
size_t VE_VMS_CPU::getAddress_R()
{
	//We need opcode to get mode
	byte opcode = readByteRF(PC);

	//The most significant bit of mode number will also be used later (As MSB of address after being read).
	byte indirectionMSB = ((opcode & 0x2) >> 1);

	//if(Debug) VMU_Debug.debug("Indirection MSB= " + Integer.toHexString(indirectionMSB));

	byte mode = ((indirectionMSB << 1) | (opcode & 0x1));
	//if(Debug) VMU_Debug.debug("Mode= " + Integer.toHexString(mode));

	byte bank = (((ram->readByte_RAW(PSW) & 0x8) >> 3) | ((ram->readByte_RAW(PSW) & 0x10) >> 3));
	//if(Debug) VMU_Debug.debug("Bank= " + Integer.toHexString(bank));

	byte RAMPlace = mode & 0xFF;

	if(bank == 1) RAMPlace += 4;
	else if(bank == 2) RAMPlace += 8;
	else if(bank > 2) RAMPlace += 12;

	byte offset = 0;
	size_t address = ram->readByte(RAMPlace + offset);

	//if(Debug) VMU_Debug.debug("Address in= " + Integer.toHexString(RAMPlace + offset));

	//Now put most significant bit so result is 9-bits long
	address &= 0xFF;
	address |= (indirectionMSB << 8);

	address &= 0x1FF;

	//if(Debug) VMU_Debug.debug("Address R: " + Integer.toHexString(address));

	//if(Debug) VMU_Debug.debug("R Operand= " + Integer.toHexString(ram->readByte(address)));

	return address;
}
///Absolute 12-bit
size_t VE_VMS_CPU::getAddress_a12()
{
	//We need opcode since it has bits 9-12 of the address
	byte opcode = readByteRF(PC);

	byte bit_12 = ((opcode & 0x10) >> 1);

	byte bits_9to12 = ((opcode & 0x7) | bit_12);

	//if(Debug) VMU_Debug.debug("Address a12: " + Integer.toHexString((readByteRF(PC+1) | (bits_9to12 << 0x8)) & 0xFFF));

	//Big-endian
	return (readByteRF(PC+1) | (bits_9to12 << 0x8)) & 0xFFF;
}
///Absolute 16-bit
size_t VE_VMS_CPU::getAddress_a16()
{
	//Big-endian
	byte MSB = readByteRF(PC+1);
	byte LSB = readByteRF(PC+2);

	//if(Debug) VMU_Debug.debug("Address a16: " + Integer.toHexString((((MSB << 8) | LSB) & 0xFFFF)));

	return ((MSB << 8) | LSB) & 0xFFFF;
}

///Absolute 16-bit (But can select to get operand from ROM (EXT = 0) or Flash (EXT = 1), important for when an instruction changes EXT, it will always be followed by a JMPF)
size_t VE_VMS_CPU::getAddress_a16(byte _EXT) 
{
	//Big-endian
	byte MSB;
	byte LSB;

	if(_EXT == 1)
	{
		//If want to read address from flash
		MSB = flash->getByte(PC + 1);
		LSB = flash->getByte(PC + 2);
	}
	else 
	{
		//If want to read address from ROM (BIOS)
		MSB = rom->readByte(PC + 1);
		LSB = rom->readByte(PC + 2);
	}

	//if(Debug) VMU_Debug.debug("Address a16 (EXT): " + Integer.toHexString((((MSB << 8) | LSB) & 0xFFFF)));

	return ((MSB << 8) | LSB) & 0xFFFF;
}

//FLAG operations
void VE_VMS_CPU::FLAG_setP()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data | 0x1));
}
void VE_VMS_CPU::FLAG_clearP()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data & 0xFE));
}
byte VE_VMS_CPU::FLAG_getP()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	return (PSW_data & 0x1) & 0xFF;
}
void VE_VMS_CPU::FLAG_setOV()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data | 0x4));
}
void VE_VMS_CPU::FLAG_clearOV()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data & 0xFB));
}
byte VE_VMS_CPU::FLAG_getOV()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	return ((PSW_data & 0x4) >> 2) & 0xFF;
}
void VE_VMS_CPU::FLAG_setAC()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data | 0x40));
}
void VE_VMS_CPU::FLAG_clearAC()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data & 0xBF));
}
byte VE_VMS_CPU::FLAG_getAC()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	return ((PSW_data & 0x40) >> 6) & 0xFF;
}
void VE_VMS_CPU::FLAG_setCY()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data | 0x80));
}
void VE_VMS_CPU::FLAG_clearCY()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	ram->writeByte_RAW(PSW, (PSW_data & 0x7F));
}
byte VE_VMS_CPU::FLAG_getCY()
{
	byte PSW_data = ram->readByte_RAW(PSW);
	return ((PSW_data & 0x80) >> 7) & 0xFF;
}


/******************************************************
 ******************************************************
 *  Instructions and interrupts
 ******************************************************
 ******************************************************/

void VE_VMS_CPU::processInterrupts()
{
	if (interruptDepth >= 16) return; // Defer until an active interrupt returns.
	byte IE_data = ram->readByte_RAW(IE); //Interrupt Enable control

	//Interrupts are not blocked, and nested interrupts are less than 3 levels deep.
	if(IE_data & 128)
	{
		if(intHandler->getReset() == 1)
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0;
			interruptLevel++;
			currentInterrupt = 0;
			interruptsMasked = true;
			intHandler->clearReset();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return; //It is important to return after each interrupt
		}
		if(intHandler->getINT0() == 1 && (currentInterrupt == 0 || currentInterrupt >= 1))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x3;
			interruptLevel++;
			currentInterrupt = 1;
			interruptsMasked = true;
			intHandler->clearINT0();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getINT1() == 1 && (currentInterrupt == 0 || currentInterrupt >= 2))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0xB;
			interruptLevel++;
			currentInterrupt = 2;
			interruptsMasked = true;
			intHandler->clearINT1();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getINT2() == 1 && (currentInterrupt == 0 || currentInterrupt >= 3))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x13;
			interruptLevel++;
			currentInterrupt = 3;
			interruptsMasked = true;
			intHandler->clearINT2();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getINT3() == 1 && (currentInterrupt == 0 || currentInterrupt >= 4))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x1B;
			interruptLevel++;
			currentInterrupt = 4;
			interruptsMasked = true;
			intHandler->clearINT3();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getT0HOV() == 1 && (currentInterrupt == 0 || currentInterrupt >= 5))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x23;
			interruptLevel++;
			currentInterrupt = 5;
			interruptsMasked = true;
			intHandler->clearT0HOV();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getT1HLOV() == 1 && (currentInterrupt == 0 || currentInterrupt >= 6))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x2B;
			interruptLevel++;
			currentInterrupt = 6;
			interruptsMasked = true;
			intHandler->clearT1HLOV();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getSIO0() == 1 && (currentInterrupt == 0 || currentInterrupt >= 7))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x33;
			interruptLevel++;
			currentInterrupt = 7;
			interruptsMasked = true;
			intHandler->clearSIO0();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getSIO1() == 1 && (currentInterrupt == 0 || currentInterrupt >= 8))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x3B;
			interruptLevel++;
			currentInterrupt = 8;
			interruptsMasked = true;
			intHandler->clearSIO1();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getRFB() == 1 && (currentInterrupt == 0 || currentInterrupt >= 9))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x43;
			interruptLevel++;
			currentInterrupt = 9;
			interruptsMasked = true;
			intHandler->clearRFB();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
		if(intHandler->getP3() == 1 && (currentInterrupt == 0 || currentInterrupt >= 10))
		{
			//Push address to stack (16-bits, little endian).
			//SP is increased by 2 afterwards
			ram->stackPush(PC & 0xFF);
			ram->stackPush((PC >> 8) & 0xFF);

			//if(EXTNew == 1) ram->writeByte(EXT, 0);
			PC = 0x4B;
			interruptLevel++;
			currentInterrupt = 10;
			interruptsMasked = true;
			intHandler->clearP3();
			//Enable CPU
			ram->writeByte_RAW(PCON, 0);
			if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
			return;
		}
	}
	//Interrupts are blocked except for non-maskables (INT0 and INT1
	else 
	{
		//If INT0 and INT1 are not blocked
		if((IE_data & 1) == 0)
		{
			if(intHandler->getINT0() == 1 && (currentInterrupt == 0 || currentInterrupt >= 1))
			{
				//Push address to stack (16-bits, little endian).
				//SP is increased by 2 afterwards
				ram->stackPush(PC & 0xFF);
				ram->stackPush((PC >> 8) & 0xFF);


				//if(EXTNew == 1) ram->writeByte(EXT, 0);
				PC = 0x3;
				interruptLevel++;
				currentInterrupt = 1;
				interruptsMasked = true;
				intHandler->clearINT0();
				//Enable CPU
				ram->writeByte_RAW(PCON, 0);
				if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
				return;
			}
			//If INT1 is not blocked, process it
			if((IE_data & 2) == 0)
			{
				if(intHandler->getINT1() == 1 && (currentInterrupt == 0 || currentInterrupt >= 2)){
					//Push address to stack (16-bits, little endian).
					//SP is increased by 2 afterwards
					ram->stackPush(PC & 0xFF);
					ram->stackPush((PC >> 8) & 0xFF);

					//if(EXTNew == 1) ram->writeByte(EXT, 0);
					PC = 0xB;
					interruptLevel++;
					currentInterrupt = 2;
					interruptsMasked = true;
					intHandler->clearINT1();
					//Enable CPU
					ram->writeByte_RAW(PCON, 0);
					if (interruptDepth < 16) interruptQueue[interruptDepth++] = currentInterrupt;
					return;
				}
			}
		}
	}
}


size_t VE_VMS_CPU::getPC() const
{
	return PC;
}

void VE_VMS_CPU::setHLE(bool hle)
{
	IsHLE = hle;
}

//System call emulation
void VE_VMS_CPU::performHLE(size_t entryAddress)
{
	//We must never go back to BIOS when there is no BIOS...
	ram->writeByte_RAW(EXT, 1);    //Never ever...

	//Emulate system calls

	//Write
	if(entryAddress == 0x100)
	{
		ram->writeByte(0, 0);
		ram->writeByte(B, 0);
		ram->writeByte(C, 0);
		ram->writeByte(TRL, 0);
		ram->writeByte(TRH, 0);
		ram->writeByte(XBNK, 0);


		size_t startAddress = ((ram->readByte(0x7E) << 8) | ram->readByte(0x7F)) & 0xFFFF;

		byte flashBank = ram->readByte(0x7D);
		if(flashBank == 1) startAddress += 0x10000;

		/* Flash is 0x20000 bytes and startAddress can already be 0x1FFFF, so a
		   blind run of 128 walks off the end of the buffer. */
		for(size_t i = 0; i < 128 && startAddress < 0x20000; ++i, startAddress++)
		{
			flash->writeByte_RAW(startAddress, ram->readByte(0x80 + i));
		}

		ram->writeByte(ACC, 0);

		PC = 0x105;
	}
	//Verify
	else if(entryAddress == 0x110)
	{
		ram->writeByte(0, 0);
		ram->writeByte(TRL, 0);
		ram->writeByte(TRH, 0);

		size_t startAddress = ((ram->readByte(0x7E) << 8) | ram->readByte(0x7F)) & 0xFFFF;

		byte flashBank = ram->readByte(0x7D);
		if(flashBank == 1) startAddress += 0x10000;

		for(size_t i = 0; i < 128 && startAddress < 0x20000; ++i, startAddress++)
		{
			if(flash->getByte(startAddress) != ram->readByte(0x80 + i))
			{
				ram->writeByte(ACC, 0xFF);
				break;
			}
			else ram->writeByte(ACC, 0);
		}

		PC = 0x115;
	}
	//Read
	else if(entryAddress == 0x120)
	{
		ram->writeByte(0, 0);
		ram->writeByte(ACC, 0);
		ram->writeByte(TRL, 0);
		ram->writeByte(TRH, 0);

		size_t startAddress = ((ram->readByte(0x7E) << 8) | ram->readByte(0x7F)) & 0xFFFF;

		byte flashBank = ram->readByte(0x7D);
		if(flashBank == 1) startAddress += 0x10000;

		for(size_t i = 0; i < 128 && startAddress < 0x20000; ++i, startAddress++)
		{
			ram->writeByte(0x80 + i, flash->getByte(startAddress));
		}

		PC = 0x125;
	}
	//Update time
	else if(entryAddress == 0x130)
	{
		//Set time and date
		byte day = ram->readByte_RAW(0x1A);
		byte month = ram->readByte_RAW(0x19);
		byte yearH = ram->readByte_RAW(0x17);
		byte yearL = ram->readByte_RAW(0x18);
		byte hour = ram->readByte_RAW(0x1B);
		byte min = ram->readByte_RAW(0x1C);
		byte sec = ram->readByte_RAW(0x1D);
		byte halfSec = ram->readByte_RAW(0x1E);

		halfSec = (~halfSec) & 0x1;

		if(halfSec == 0)
		{
			sec++;
			sec %= 60;
			if(sec == 0)
			{
				min++;
				min %= 60;
				if(min == 0)
				{
					hour++;
					hour %= 24;
					if(hour == 0)
					{
						day++;
						day %= 365;
						if(day == 0)
						{
							month++;
							month %= 12;
							if(month == 0)
							{
								yearL++;
							}
						}
					}
				}
			}
		}

		ram->writeByte_RAW(0x17, yearH);
		ram->writeByte_RAW(0x18, yearL);
		ram->writeByte_RAW(0x19, month);

		ram->writeByte_RAW(0x1B, hour);
		ram->writeByte_RAW(0x1C, min);
		ram->writeByte_RAW(0x1D, sec);
		ram->writeByte_RAW(0x1E, halfSec);

		ram->writeByte_RAW(0x50, yearH / 4);
		ram->writeByte_RAW(0x51, yearL / 4);

		PC = 0x139;
	}
	//Exit
	else if(entryAddress == 0x1F0)
	{
		//vmu.halt();
		state = 0;
	}

}

int VE_VMS_CPU::processInstruction(bool dbg) 
{
	if(state != 1) return 0;  //CPU in halt state

	EXTNew = ram->readByte_RAW(EXT) & 0x1;

	if(EXTOld != EXTNew) 
	{
		if(EXTNew == 0)
		{
				//We are in ROM now, so read JMPF from Flash and jump to it here.
				if(!IsHLE) PC = getAddress_a16(1);
				else 
				{
					performHLE(getAddress_a16(1));
					//return;
				}
		} 
		else 
		{
				if(!IsHLE) PC = getAddress_a16(0);
		}
	}

	if(!IsHLE) EXTOld = ram->readByte_RAW(EXT) & 0x1;

	byte opcode = readByteRF(PC);
	
	//printf("Instruction: %#04x\n", opcode);

	byte cycles = 0;

	//NOP
	if(opcode == 0x00)
	{
		//Does nothing


		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//ADD i8
	else if(opcode == 0x81)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte operand = getAddress_i8();

		byte result[2];
		add8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//ADD d9
	else if(opcode == 0x82 || opcode == 0x83)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_d9();
		byte operand = ram->readByte(address);

		byte result[2];
		add8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//ADD @R
	else if(opcode >= 0x84 && opcode <= 0x87)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_R();
		byte operand = ram->readByte(address);
		

		byte result[2];
		add8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
		
		
	}
	//ADC i8
	else if(opcode == 0x91)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte operand = getAddress_i8();
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;


		byte result[2];
		add8(ACC_data, operand, c, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//ADC d9
	else if(opcode == 0x92 || opcode == 0x93)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_d9();
		byte operand = ram->readByte(address);
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;


		byte result[2];
		add8(ACC_data, operand, c, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//ADC @R
	else if(opcode >= 0x94 && opcode <= 0x97)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_R();
		byte operand = ram->readByte(address);
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;


		byte result[2];
		add8(ACC_data, operand, c, result);
		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUB i8
	else if(opcode == 0xA1)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte operand = getAddress_i8();

		byte result[2];
		sub8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUB d9
	else if(opcode == 0xA2 || opcode == 0xA3)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_d9();
		byte operand = ram->readByte(address);

		byte result[2];
		sub8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUB @R
	else if(opcode >= 0xA4 && opcode <= 0xA7)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_R();
		byte operand = ram->readByte(address);

		byte result[2];
		sub8(ACC_data, operand, 0, result);

		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUBC i8
	else if(opcode == 0xB1)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte operand = getAddress_i8();
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;

		byte result[2];
		sub8(ACC_data, operand, c, result);
		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUBC d9
	else if(opcode == 0xB2 || opcode == 0xB3)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_d9();
		byte operand = ram->readByte(address);
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;


		byte result[2];
		sub8(ACC_data, operand, c, result);
		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
		
		
	}
	//SUBC @R
	else if(opcode >= 0xB4 && opcode <= 0xB7)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_R();
		byte operand = ram->readByte(address);
		byte c = 0;
		if(FLAG_getCY() == 1)
			c = 1;


		byte result[2];
		sub8(ACC_data, operand, c, result);
		//Flags (It is more efficient to compare to 0 rather than do shift and compare to 1)
		if((result[1] & 0x1) == 0) FLAG_clearCY(); else FLAG_setCY();
		if((result[1] & 0x2) == 0) FLAG_clearAC(); else FLAG_setAC();
		if((result[1] & 0x4) == 0) FLAG_clearOV(); else FLAG_setOV();

		ram->writeByte_RAW(ACC, result[0]);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
		
		
	}
	//INC d9
	else if(opcode == 0x62 || opcode == 0x63)
	{
		size_t address = getAddress_d9();

		byte data = ram->readByte(address);
		data += 1;
		ram->writeByte(address, data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//INC @R
	else if(opcode >= 0x64 && opcode <= 0x67)
	{
		//if(dbg) VMU_Debug.debug("INC @R:");
		size_t address = getAddress_R();

		byte data = ram->readByte(address);
		data += 1;
		ram->writeByte(address, data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//DEC d9
	else if(opcode == 0x72 || opcode == 0x73)
	{
		size_t address = getAddress_d9();

		byte data = ram->readByte(address);
		data -= 1;
		ram->writeByte(address, data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//DEC @R
	else if(opcode >= 0x74 && opcode <= 0x77)
	{
		size_t address = getAddress_R();

		byte data = ram->readByte(address);
		data -= 1;
		ram->writeByte(address, data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//MUL
	else if(opcode == 0x30)
	{
		uint32_t operand = ram->readByte_RAW(C) | (ram->readByte(ACC) << 8);
		byte operand2 = ram->readByte_RAW(B);
		operand *= operand2;    //This is 24-bits

		//B: High 8-bits
		//ACC_data: Mid 8-bits
		//C: Low 8-bits
		ram->writeByte_RAW(B, ((operand & 0xFF0000) >> 16));
		ram->writeByte_RAW(ACC, ((operand & 0xFF00) >> 8));
		ram->writeByte_RAW(C, (operand & 0xFF));

		//Flags
		FLAG_clearCY();
		if(((operand & 0xFF0000) >> 16) != 0) FLAG_setOV(); else FLAG_clearOV();

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 7;
	}
	//DIV
	else if(opcode == 0x40)
	{
		uint16_t operand = ram->readByte_RAW(C) | (ram->readByte(ACC) << 8);
		byte operand2 = ram->readByte_RAW(B);

		uint16_t quotient = operand;
		uint16_t remainder = operand;

		if(operand2 != 0) 
		{
			quotient /= operand2;
			remainder %= operand2;

			//Store quotient
			ram->writeByte_RAW(ACC, ((quotient & 0xFF00) >> 8));
			ram->writeByte_RAW(C, (quotient & 0xFF));

			//Store remainder
			ram->writeByte_RAW(B, (remainder & 0xFF));

			FLAG_clearOV();
		}
		else 
		{
			ram->writeByte_RAW(ACC, 0xFF);
			FLAG_setOV();
		}

		//Flags
		FLAG_clearCY();

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 7;
	}
	//AND i8
	else if(opcode == 0xE1)
	{
		byte operand = getAddress_i8();
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data &= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//AND d9
	else if(opcode == 0xE2 || opcode == 0xE3)
	{
		byte operand = ram->readByte(getAddress_d9());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data &= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//AND @R
	else if(opcode >= 0xE4 && opcode <= 0xE7)
	{
		byte operand = ram->readByte(getAddress_R());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data &= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//OR i8
	else if(opcode == 0xD1)
	{
		byte operand = getAddress_i8();
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data |= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//OR d9
	else if(opcode == 0xD2 || opcode == 0xD3)
	{
		byte operand = ram->readByte(getAddress_d9());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data |= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//OR @R
	else if(opcode >= 0xD4 && opcode <= 0xD7)
	{
		byte operand = ram->readByte(getAddress_R());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data |= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//XOR i8
	else if(opcode == 0xF1)
	{
		byte operand = getAddress_i8();
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data ^= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//XOR d9
	else if(opcode == 0xF2 || opcode == 0xF3)
	{
		byte operand = ram->readByte(getAddress_d9());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data ^= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//XOR @R
	else if(opcode >= 0xF4 && opcode <= 0xF7)
	{
		byte operand = ram->readByte(getAddress_R());
		byte ACC_data = ram->readByte_RAW(ACC);

		ACC_data ^= operand;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//ROL
	else if(opcode == 0xE0)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte MSB = ((ACC_data & 0x80) >> 7);

		ACC_data <<= 1;
		ACC_data &= 0xFE;
		ACC_data |= MSB;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//ROLC
	else if(opcode == 0xF0)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte MSB1 = (ACC_data & 0x80);   //No need to shift since it will be copied to 8th bit of PSW (CY flag)
		byte MSB2 = ((ram->readByte_RAW(PSW) & 0x80) >> 7);

		byte PSW_data = ram->readByte_RAW(PSW);
		PSW_data &= 0x7F;
		PSW_data |= MSB1;
		ram->writeByte(PSW, PSW_data);

		ACC_data <<= 1;
		ACC_data &= 0xFE;    //Just to make sure
		ACC_data |= MSB2;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//ROR
	else if(opcode == 0xC0)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		byte LSB = ((ACC_data & 0x1) << 7) & 0x80;

		ACC_data >>= 1;
		ACC_data &= 0x7F;    //To set MSB to 0
		ACC_data |= LSB;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//RORC
	else if(opcode == 0xD0)
	{
		byte ACC_data = ram->readByte_RAW(ACC);
		byte PSW_data = ram->readByte_RAW(PSW);

		byte LSB1 = ((ACC_data & 0x1) << 7) & 0x80;
		byte LSB2 = (PSW_data & 0x80); //No need to shift

		PSW_data &= 0x7F;
		PSW_data |= LSB1;
		ram->writeByte(PSW, PSW_data);

		ACC_data >>= 1;
		ACC_data &= 0x7F;    //Just to make sure
		ACC_data |= LSB2;

		ram->writeByte_RAW(ACC, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//LD d9
	else if(opcode == 0x02 || opcode == 0x03)
	{
		ram->writeByte_RAW(ACC, ram->readByte(getAddress_d9()));

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//LD @R
	else if(opcode >= 0x04 && opcode <= 0x07)
	{
		ram->writeByte_RAW(ACC, ram->readByte(getAddress_R()));

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//ST d9
	else if(opcode == 0x12 || opcode == 0x13)
	{
		ram->writeByte(getAddress_d9(), ram->readByte(ACC));

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//ST @R
	else if(opcode >= 0x14 && opcode <= 0x17)
	{
		size_t address = getAddress_R();
		ram->writeByte(address, ram->readByte(ACC));

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//MOV d9
	else if(opcode == 0x22 || opcode == 0x23)
	{
		byte operand = readByteRF(PC + 2);

		ram->writeByte(getAddress_d9(), operand);

		//Instruction pointer
		PC += 3;

		//Set its cycles
		cycles = 2;
	}
	//MOV @R
	else if(opcode >= 0x24 && opcode <= 0x27)
	{
		byte operand = getAddress_i8();

		ram->writeByte(getAddress_R(), operand);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//LDC
	else if(opcode == 0xC1)
	{
		byte ACC_data = ram->readByte_RAW(ACC);
		size_t address = ram->readByte(TRL) | (ram->readByte(TRH) << 8);

		address += ACC_data;

		byte data = readByteRF(address);

		ram->writeByte_RAW(ACC, data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 2;
	}
	//PUSH d9
	else if(opcode == 0x60 || opcode == 0x61)
	{
		//Increase SP by 1
		byte operand = ram->readByte(getAddress_d9());

		ram->stackPush(operand);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 2;
	}
	//POP d9
	else if(opcode == 0x70 || opcode == 0x71)
	{
		size_t address = getAddress_d9();

		ram->writeByte(address, ram->stackPop());

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 2;
	}
	//XCHG d9
	else if(opcode == 0xC2 || opcode == 0xC3)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_d9();
		byte operand = ram->readByte(address);

		ram->writeByte_RAW(ACC, operand);
		ram->writeByte(address, ACC_data);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//XCHG @R
	else if(opcode >= 0xC4 && opcode <= 0xC7)
	{
		byte ACC_data = ram->readByte_RAW(ACC);

		size_t address = getAddress_R();
		byte operand = ram->readByte(address);

		ram->writeByte_RAW(ACC, operand);
		ram->writeByte(address, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 1;
	}
	//JMP a12
	else if((opcode >= 0x28 && opcode <= 0x2F) || (opcode >= 0x38 && opcode <= 0x3F))
	{
		size_t address = getAddress_a12();
		uint16_t PC_MSN = (PC+2) & 0xF000;   //Take most significant nibble from PC+2 (High 4-bits), +2 because we are calculating from the address of the next instruction.
		address |= PC_MSN;

		PC = address;

		//Set its cycles
		cycles = 2;
	}
	//JMPF a16
	else if(opcode == 0x21)
	{
		PC = getAddress_a16();

		//Set its cycles
		cycles = 2;
	}
	//BR r8
	else if(opcode == 0x01)
	{
		//Branch unconditionally
		signed char address = getAddress_r8();

		//Instruction pointer
		PC += 2;
		PC += address;

		//Set its cycles
		cycles = 2;
	}
	//BRF r16
	else if(opcode == 0x11)
	{
		size_t address = getAddress_r16();

		//Instruction pointer
		PC += 2; //3-1
		PC = (PC + address) % 0x10000;  //Any extra bits after 0xFFFF are removed. e.g. 0x10001 becomes 1

		//Set its cycles
		cycles = 4;
	}
	//BZ r8
	else if(opcode == 0x80)
	{
		signed char address = getAddress_r8();

		//Instruction pointer
		PC += 2;
		if(ram->readByte(ACC) == 0x00) PC += address;

		//Set its cycles
		cycles = 2;
	}
	//BNZ r8
	else if(opcode == 0x90)
	{
		signed char address = getAddress_r8();

		//Instruction pointer
		PC += 2;
		if(ram->readByte(ACC) != 0x00) PC += address;;

		//Set its cycles
		cycles = 2;
	}
	//BP d9,b3,r8
	else if((opcode >= 0x68 && opcode <= 0x6F) || (opcode >= 0x78 && opcode <= 0x7F))
	{
		byte bits = getAddress_b3();
		size_t op_address = getAddress_d9_b3();
		signed char address = getAddress_r8_b3();

		byte operand = ram->readByte(op_address);

		PC += 3;

		//Do test (Just shift the bit to be LSB, then AND the result to get only that bit ;) )
		if(((operand >> bits) & 0x1) != 0)
			PC += address;

		//Set its cycles
		cycles = 2;
	}
	//BPC d9,b3,r8
	else if((opcode >= 0x48 && opcode <= 0x4F) || (opcode >= 0x58 && opcode <= 0x5F))
	{
		byte bits = getAddress_b3();
		size_t op_address = getAddress_d9_b3();
		signed char address = getAddress_r8_b3();

		byte operand = ram->readByte(op_address);

		PC += 3;

		//Do test (Just shift the bit to be LSB, then AND the result to get only that bit ;) )
		byte temp = ((operand >> bits) & 0x1); //00000001
		if(temp != 0) 
		{
			//We want to clear that bit before we branch
			temp <<= bits;    //00001000
			temp = (~temp) & 0xFF; //11110111  (Now we AND this with the original operand)
			operand &= temp;

			ram->writeByte(op_address, operand);

			PC += address;
		}

		//Set its cycles
		cycles = 2;
	}
	//BN d9,b3,r8
	else if((opcode >= 0x88 && opcode <= 0x8F) || (opcode >= 0x98 && opcode <= 0x9F))
	{
		byte bits = getAddress_b3();
		size_t op_address = getAddress_d9_b3();
		signed char address = getAddress_r8_b3();

		byte operand = ram->readByte(op_address);

		PC += 3;

		//Do test (Just shift the bit to be LSB, then AND the result to get only that bit ;) )
		if(((operand >> bits) & 0x1) == 0)
			PC += address;

		//Set its cycles
		cycles = 2;
	}
	//DBNZ d9,r8
	else if(opcode == 0x52 || opcode == 0x53)
	{
		size_t op_address = getAddress_d9();
		signed char address = getAddress_r8_b3(); //Just because r8 is the second parameter

		byte operand = ram->readByte(op_address);
		operand--;
		operand &= 0xFF;

		ram->writeByte(op_address, operand);

		PC += 3;

		if(operand != 0)
			PC += address;

		//Set its cycles
		cycles = 2;
	}
	//DBNZ @R,r8
	else if(opcode >= 0x54 && opcode <= 0x57)
	{
		size_t op_address = getAddress_R();
		signed char address = getAddress_r8();

		byte operand = ram->readByte(op_address);
		operand--;
		operand &= 0xFF;

		ram->writeByte(op_address, operand);

		PC += 2;

		if(operand != 0)
			PC += address;

		//Set its cycles
		cycles = 2;
	}
	//BE i8, r8
	else if(opcode == 0x31)
	{
		byte operand = getAddress_i8();
		byte ACC_data = ram->readByte_RAW(ACC);

		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand == ACC_data)
			PC += address;

		if(ACC_data < operand) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//BE d9, r8
	else if(opcode == 0x32 || opcode == 0x33)
	{
		size_t op_address = getAddress_d9();
		byte operand = ram->readByte(op_address);

		byte ACC_data = ram->readByte_RAW(ACC);

		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand == ACC_data)
			PC += address;

		if(ACC_data < operand) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//BE @R, i8, r8
	else if(opcode >= 0x34 && opcode <= 0x37)
	{
		size_t op_address = getAddress_R();
		byte operand1 = ram->readByte(op_address);
		byte operand2 = getAddress_i8();
		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand1 == operand2)
			PC += address;

		if(operand1 < operand2) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//BNE i8, r8
	else if(opcode == 0x41)
	{
		byte operand = getAddress_i8();
		byte ACC_data = ram->readByte_RAW(ACC);

		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand != ACC_data)
			PC += address;

		if(ACC_data < operand) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//BNE d9, r8
	else if(opcode == 0x42 || opcode == 0x43)
	{
		size_t op_address = getAddress_d9();
		byte operand = ram->readByte(op_address) & 0xFF;

		byte ACC_data = ram->readByte_RAW(ACC);

		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand != ACC_data)
			PC += address;

		if(ACC_data < operand) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//BNE @R, i8, r8
	else if(opcode >= 0x44 && opcode <= 0x47)
	{
		size_t op_address = getAddress_R();
		byte operand1 = ram->readByte(op_address) & 0xFF;
		byte operand2 = getAddress_i8();

		signed char address = getAddress_r8_b3();

		PC += 3;

		if(operand1 != operand2)
			PC += address;

		if(operand1 < operand2) FLAG_setCY(); else FLAG_clearCY();

		//Set its cycles
		cycles = 2;
	}
	//CALL a12
	else if((opcode >= 0x08 && opcode <= 0x0F) || (opcode >= 0x18 && opcode <= 0x1F))
	{
		size_t address = getAddress_a12();
		PC += 2;
		uint16_t PC_MSN = PC & 0xF000;   //Take most significant nibble from PC+2 (High 4-bits), +2 because we are calculating from the address of the next instruction.
		address |= PC_MSN;

		//Push address (Of next instruction) on stack
		byte LSB = (PC & 0xFF);
		byte MSB = ((PC & 0xFF00) >> 8);

		ram->stackPush(LSB);
		ram->stackPush(MSB);

		//Jump to address
		PC = address;

		//Set its cycles
		cycles = 2;
	}
	//CALLF a16
	else if(opcode == 0x20)
	{
		size_t address = getAddress_a16();

		PC += 3;
		//Push address (Of next instruction) on stack
		byte LSB = (PC & 0xFF);
		byte MSB = ((PC & 0xFF00) >> 8);

		ram->stackPush(LSB);
		ram->stackPush(MSB);

		PC = address;

		//Set its cycles
		cycles = 2;
	}
	//CALLR r16
	else if(opcode == 0x10)
	{
		size_t address = getAddress_r16();

		//Push address (Of next instruction) on stack
		PC += 3;
		byte LSB = (PC & 0xFF);
		byte MSB = ((PC & 0xFF00) >> 8);

		ram->stackPush(LSB);
		ram->stackPush(MSB);

		PC -= 1;
		//Instruction pointer
		PC = (PC + address) % 0x10000;  //Any extra bits after 0xFFFF are removed. e.g. 0x10001 becomes 1
		//2 = 3 - 1 (Instruction after - 1)

		//Set its cycles
		cycles = 4;
	}
	//RET
	else if(opcode == 0xA0)
	{
		//Get back to PC which we stored in stack when call occurred.
		//And of course decrease SP
		byte PC1 = ram->stackPop();    //High bits
		byte PC2 = ram->stackPop();    //Low bits

		PC = PC2 | (PC1 << 8);

		//Set its cycles
		cycles = 2;
	}
	//RETI
	else if(opcode == 0xB0)
	{
		//Get back to PC which we stored in stack when interrupt occurred.
		//And of course decrease SP
		byte PC1 = ram->stackPop();    //High bits
		byte PC2 = ram->stackPop();    //Low bits

		PC = PC2 | (PC1 << 8);

		//Handle interrupts
		if(interruptLevel > 0) interruptLevel--;

		int interruptReturned = interruptDepth ? interruptQueue[--interruptDepth] : 0;
		interruptQueue[interruptDepth] = 0;
		
		//printf("RETI %d\n", interruptReturned);


		if(interruptReturned == 10) P3_taken = true;
		//if(interruptReturned == 5) printf("Exited T0HOV\n");
		//if(interruptReturned == 4) intHandler->BT_Taken = true;

		currentInterrupt = 0;

		//Re-accept interrupts
		//interruptsMasked = false;


		//Set back 'currentInterrupt' to the other active highest priority interrupt
		/*byte IE = ram->readByte(IE); //Interrupt Enable control

		if((IE & 128) != 0) {
			if (intHandler->getINT0() == 1) currentInterrupt = 1;
			else if (intHandler->getINT1() == 1) currentInterrupt = 2;
			else if (intHandler->getINT2() == 1) currentInterrupt = 3;
			else if (intHandler->getINT3() == 1) currentInterrupt = 4;
			else if (intHandler->getT0HOV() == 1) currentInterrupt = 5;
			else if (intHandler->getT1HLOV() == 1) currentInterrupt = 6;
			else if (intHandler->getSIO0() == 1) currentInterrupt = 7;
			else if (intHandler->getSIO1() == 1) currentInterrupt = 8;
			else if (intHandler->getRFB() == 1) currentInterrupt = 9;
			else if (intHandler->getP3() == 1) currentInterrupt = 10;
		}
		else{
			if((IE & 1) == 0)
			{
				if (intHandler->getINT0() == 1) currentInterrupt = 1;

				if((IE & 2) == 0) {
					if (intHandler->getINT1() == 1) currentInterrupt = 2;
				}
			}
		}*/

		//Set its cycles
		cycles = 2;
	}
	//CLR1 d9, b3
	else if((opcode >= 0xC8 && opcode <= 0xCF) || (opcode >= 0xD8 && opcode <= 0xDF)) 
	{
		size_t op_address = getAddress_d9_b3();
		byte operand = ram->readByte(op_address);
		byte bits = getAddress_b3();

		byte temp = 1; //00000001
		//We want to clear that bit
		temp <<= bits;    //00001000
		temp = (~temp) & 0xFF; //11110111  (Now we AND this with the original operand)
		operand &= temp;

		ram->writeByte(op_address, operand);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//SET1 d9, b3
	else if((opcode >= 0xE8 && opcode <= 0xEF) || (opcode >= 0xF8)) 
	{
		size_t op_address = getAddress_d9_b3();
		byte operand = ram->readByte(op_address);
		byte bits = getAddress_b3();

		byte temp = (1 << bits); //00001000 (Now we OR this with the original operand)
		//We want to set that bit
		operand |= temp;

		ram->writeByte(op_address, operand);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//NOT1 d9, b3
	else if((opcode >= 0xA8 && opcode <= 0xAF) || (opcode >= 0xB8 && opcode <= 0xBF)) 
	{
		size_t op_address = getAddress_d9_b3();
		byte operand = ram->readByte(op_address);
		byte bits = getAddress_b3();

		byte temp = (1 << bits); //00001000
		//We want to set that bit
		if(((operand >> bits) & 0x1) == 0) 
		{
			//00001000 (Now we OR this with the original operand)
			operand |= temp;
		}
		else
		{
			temp = (~temp) & 0xFF; //11110111  (Now we AND this with the original operand)
			operand &= temp;
		}
		ram->writeByte(op_address, operand);

		//Instruction pointer
		PC += 2;

		//Set its cycles
		cycles = 1;
	}
	//Special low level opcodes
	//LDC Flash
	else if(opcode == 0x50)
	{
		size_t address = ram->readByte(TRL) | (ram->readByte(TRH) << 8);

		byte data = flash->readByte(address);

		ram->writeByte_RAW(ACC, data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 2;
	}
	//STC Flash
	else if(opcode == 0x51)
	{
		byte ACC_data = ram->readByte_RAW(ACC);
		size_t address = ram->readByte(TRL) | (ram->readByte(TRH) << 8);

		flash->writeByte(address, ACC_data);

		//Instruction pointer
		PC += 1;

		//Set its cycles
		cycles = 2;
	}


	instructionCount = (instructionCount == 2147483647) ? 0 : instructionCount + 1;
	PC &= 0xFFFF;
	
	return cycles;
}

void VE_VMS_CPU::serialize(VE_STATE &s)
{
   unsigned pc = (unsigned)PC;

   s.i32(state);
   s.i32(EXTOld);
   s.i32(EXTNew);
   s.b(P3_taken);
   s.u32(pc);
   s.f64(frequency);
   s.i32(interruptLevel);
   s.i32(currentInterrupt);
   s.b(interruptsMasked);
   s.i32(instructionCount);
   s.b(IsHLE);
   s.i32(interruptDepth);
   if (interruptDepth < 0 || interruptDepth > 16) { s.fail(); return; }
   for (int i = 0; i < 16; ++i) s.i32(interruptQueue[i]);

   PC = (size_t)pc;
}
