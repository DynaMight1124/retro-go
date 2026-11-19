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

#ifndef _ROM_H_
#define _ROM_H_

#include "common.h"

class VE_VMS_ROM
{
public:
    VE_VMS_ROM();
    ~VE_VMS_ROM();

    //Setters and getters
    byte readByte(size_t address);

    void writeByte(size_t address, byte b);

    //Memory operations
    void loadData(byte *d, size_t buffSize, size_t size);

    void loadData(byte *d, size_t buffSize);

    byte *getData();

private:
	byte *data;
};

#endif // _ROM_H_
