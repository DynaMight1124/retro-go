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

#include "ram.h"

VE_VMS_RAM::VE_VMS_RAM()
{
   T1LC_Temp = 0;
   T1HC_Temp = 0;

   data      = new byte[1024]();
   wram      = new byte[512]();
   // A bank holds 0x7C bytes, but every access adds STAD (0-255) to the
   // index, which reaches up to 0xFF past the end of the bank.
   xram0     = new byte[0x7C + 0x100]();
   xram1     = new byte[0x7C + 0x100]();
   xram2     = new byte[0x7C + 0x100]();

   T1RL_data = 0;
   T1RH_data = 0;
}

VE_VMS_RAM::~VE_VMS_RAM()
{
	delete []data;
	delete []wram;
	delete []xram0;
	delete []xram1;
	delete []xram2;
}

//Setters and getters
byte VE_VMS_RAM::readByte(size_t adr)
{
	size_t xaddress = adr + (data[STAD] & 0xFF);    //Remove start address of XRAM
	size_t address = adr;

	//If first half of ram is accessed (Not SFR), check Bank to choose whether to access first bank of RAM or second.
	//Second bank starts at 256
	if(address < 0x100) 
	{
		byte mainBank = ((data[PSW] & 0x2) >> 1) & 0x1; //Get RAM bank

		if(mainBank == 1) address += 512;
	}
	//If XRAM is accessed, we choose the address depending on XBNK SFR.
	else if(address >= 0x180 && address <= 0x1FB)
	{
		byte XRAMBank = data[XBNK] & 0xFF;

		switch(XRAMBank)
		{
			case 0:
				return xram0[xaddress - 0x180] & 0xFF;
			case 1:
				return xram1[xaddress - 0x180] & 0xFF;
			case 2:
				return xram2[xaddress - 0x180] & 0xFF;
			default:
				return xram0[xaddress - 0x180] & 0xFF;
		}
	}

	//Work RAM access
	if(adr == 0x166) 
	{
		byte VSEL_ = data[VSEL] & 0xFF;
		size_t VRMAD = (data[VRMAD1] | (data[VRMAD2] << 8)) & 0x1FF;
		size_t VRMAD_original = VRMAD;

		//Check if auto increment is enabled
		if((VSEL_ & 16) != 0)
		{
			VRMAD++;
			data[VRMAD1] = VRMAD & 0xFF;
			data[VRMAD2] = (VRMAD >> 8) & 1;
		}

		return (wram[VRMAD_original] & 0xFF);
	}


	if(adr != T1LR && adr != T1HR)    //T1 reload registers are the same as T1, so we need to separate them
		return (data[address] & 0xFF);
	else if(adr == T1LR) return T1RL_data & 0xFF;
	else return T1RH_data & 0xFF;
}

//No banking or checking, use carefully!
byte VE_VMS_RAM::readByte_RAW(size_t adr)
{
	return data[adr] & 0xFF;
}

//This is used by the GPU only.
byte VE_VMS_RAM::readByteXRAM(size_t adr, int bank)
{
	size_t address = adr + (data[STAD] & 0xFF);

	switch(bank)
	{
		case 0:
			return xram0[address - 0x180] & 0xFF;
		case 1:
			return xram1[address - 0x180] & 0xFF;
		case 2:
			return xram2[address - 0x180] & 0xFF;
		default:
			return xram0[address - 0x180] & 0xFF;
	}
}

void VE_VMS_RAM::writeByte(size_t adr, byte b)
{
	size_t xaddress = adr + (data[STAD] & 0xFF);
	size_t address = adr;

	//If first half of ram is accessed (Not SFR), check Bank to choose whether to access first bank of RAM or second.
	//Second bank starts at 256
	if(address < 0x100) 
	{
		byte mainBank = ((data[PSW] & 0x2) >> 1) & 0x1; //Get RAM bank

		if(mainBank == 1) address += 512;   //To access second bank in "data[]"
	}
	//If XRAM is accessed, we choose the address depending on XBNK SFR.
	else if(address >= 0x180 && address <= 0x1FB)
	{
		byte XRAMBank = data[XBNK] & 0xFF;

		switch(XRAMBank)
		{
			case 0:
				//VMU_Debug.debugGFX("Writing to XRAM Bank 0, Data= " + Integer.toHexString(b & 0xFF));
				xram0[xaddress - 0x180] = b & 0xFF;
				return;
			case 1:
				//VMU_Debug.debugGFX("Writing to XRAM Bank 1, Data= " + Integer.toHexString(b & 0xFF));
				xram1[xaddress - 0x180] = b & 0xFF;
				return;
			case 2:
				//VMU_Debug.debugGFX("Writing to XRAM Bank 2, Data= " + Integer.toHexString(b & 0xFF));
				xram2[xaddress - 0x180] = b & 0xFF;
				return;
			default:
				//VMU_Debug.debugGFX("(Fault) Writing to XRAM Bank 0, Data= " + Integer.toHexString(b & 0xFF));
				xram0[xaddress - 0x180] = b & 0xFF;
				return;
		}
	}

	//Work RAM access
	if(adr == 0x166) 
	{
		byte VSEL_ = data[VSEL] & 0xFF;
		size_t VRMAD = (data[VRMAD1] | (data[VRMAD2] << 8)) & 0x1FF;
		size_t VRMAD_original = VRMAD;

		//Check if auto increment is enabled
		if((VSEL_ & 16) != 0)
		{
			VRMAD++;
			data[VRMAD1] = VRMAD & 0xFF;
			data[VRMAD2] = (VRMAD >> 8) & 1;
		}

		wram[VRMAD_original] = b & 0xFF;
		return;
	}

	if(address != T1LC && address != T1HC) 
	{
		data[address] = b & 0xFF;
		return;
	}

	//T1LC and T1HC are updated only when bit4 of T1CNT is enabled (This is checked each cycle).
	if(address == T1LC) T1LC_Temp = b & 0xFF;
	else T1HC_Temp = b & 0xFF;
}

void VE_VMS_RAM::writeByte_RAW(size_t adr, byte b)
{
	data[adr] = b & 0xFF;
}

//Stack operations (To ensure writing to bank 0 only)
void VE_VMS_RAM::stackPush(byte d)
{
	data[SP] += 1;
	data[data[SP]] = d & 0xFF;
}

byte VE_VMS_RAM::stackPop()
{
	data[SP] -= 1;
	return data[data[SP] + 1] & 0xFF;
}

byte *VE_VMS_RAM::getData()
{
	return data;
}

/* Everything the guest can change: the register file and the two banks behind
   it, the extra display banks, and the four timer 1 values kept here because
   they are read back through this class. */
void VE_VMS_RAM::serialize(VE_STATE &s)
{
   s.raw(data,  1024);
   s.raw(wram,   512);
   s.raw(xram0, 0x7C + 0x100);
   s.raw(xram1, 0x7C + 0x100);
   s.raw(xram2, 0x7C + 0x100);

   s.u8(T1LC_Temp);
   s.u8(T1HC_Temp);
   s.i32(T1RL_data);
   s.i32(T1RH_data);
}
