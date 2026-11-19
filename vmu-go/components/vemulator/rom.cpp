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

#include "rom.h"

VE_VMS_ROM::VE_VMS_ROM()
{
	data = new byte[0x10000]();
}

VE_VMS_ROM::~VE_VMS_ROM()
{
	delete []data;
}

//Setters and getters
byte VE_VMS_ROM::readByte(size_t address)
{
	return data[address & 0xFFFF] & 0xFF;
}

void VE_VMS_ROM::writeByte(size_t address, byte b)
{
	data[address & 0xFFFF] = b & 0xFF;
}

//Memory operations
void VE_VMS_ROM::loadData(byte *d, size_t buffSize, size_t size)
{
   size_t i;
	if(buffSize < size || size > 0x10000) return;
	for(i = 0; i < size; i++)
		data[i] = d[i];
}

void VE_VMS_ROM::loadData(byte *d, size_t buffSize)
{
   size_t i;
	for(i = 0; i < buffSize && i < 0x10000; i++)
		data[i] = d[i];
}

byte *VE_VMS_ROM::getData()
{
	return data;
}

