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

#ifndef _BITWISEMATH_H_
#define _BITWISEMATH_H_

#include "common.h"

//These functions perform low-level bitwise operations.
//Flags [0|0|0|0|0|O|A|C/B]
///This is an 8-bit adder. Returns sum as first in array, then flags.
void add8(byte i1, byte i2, byte carry, byte *result);

void sub8(byte i1, byte i2, byte carry, byte *result);

///This is an 8-bit subtractor (op1 - op2). Returns difference as first in array, then flags.
void sub8_old(byte i1, byte i2, byte *result);

//Take complement
byte comp8(byte op);

//Convert to signed
byte toSigned8(byte op);

///Converts normal number byteo BCD
int int2BCD(int dec);

#endif // _BITWISEMATH_H_
