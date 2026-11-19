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

#include "bitwisemath.h"

//Flags [0|0|0|0|0|O|A|C/B]
//This is an 8-bit adder. Returns sum as first in array, then flags.
void add8(byte i1, byte i2, byte carry, byte *result)
{
	byte op1 = i1 & 0xFF;
	byte op2 = i2 & 0xFF;

	byte s = 0; //8-bits
	byte c = carry; //Assume its 1-bit

	byte flags = 0;

	byte c6 = 0;
	byte c7 = 0;

	//Start operation
	for(byte i = 0; i < 8; ++i)
	{
		//Take next bit
		byte bit1 = ((op1 & ((1 << i) & 0xFF)) >> i) & 0xFF;
		byte bit2 = ((op2 & ((1 << i) & 0xFF)) >> i) & 0xFF;

		//Sum them together
		byte z1 = (bit1 ^ bit2) & 0xFF;
		s |= ((c ^ z1) << i) & 0xFF;

		//Calculate carry
		byte c1 = (c & z1) & 0xFF;
		byte c2 = (bit1 & bit2) & 0xFF;
		c = (c1 | c2) & 0xFF;

		//Auxiliary flag set
		if(c != 0 && i == 3)
			flags |= 0x2;

		if(i == 6) c6 = c & 1;
		else if(i == 7) c7 = c & 1;
	}

	c &= 1;

	//Check carry
	if(c == 1)
		flags |= 0x1;
	//Check overflow
	if(c6 != c7)
		flags |= 0x4;
	result[0] = s & 0xFF;
	result[1] = flags & 0xFF;
}

void sub8(byte i1, byte i2, byte carry, byte *result)
{

	byte op1 = i1 & 0xFF;
	byte op2 = i2 & 0xFF;

	op2 ^= 0xFF;    //2's Complement
	op2 &= 0xFF;

	byte s = 0; //8-bits
	byte c = carry ^ 0xFF; //Assume its 1-bit
	c &= 1;

	byte flags = 0;

	byte c6 = 0;
	byte c7 = 0;

	//Start operation
	for(byte i = 0; i < 8; ++i)
	{
		//Take next bit
		byte bit1 = ((op1 & ((1 << i) & 0xFF)) >> i) & 0xFF;
		byte bit2 = ((op2 & ((1 << i) & 0xFF)) >> i) & 0xFF;

		//Sum them together
		byte z1 = (bit1 ^ bit2) & 0xFF;
		s |= ((c ^ z1) << i) & 0xFF;

		//Calculate carry
		byte c1 = (c & z1) & 0xFF;
		byte c2 = (bit1 & bit2) & 0xFF;
		c = (c1 | c2) & 0xFF;

		//Auxiliary flag set
		if(c == 0 && i == 3)
			flags |= 0x2;

		if(i == 6) c6 = c & 1;
		else if(i == 7) c7 = c & 1;
	}

	c ^= 0xFF;
	c &= 1;

	//Check carry
	if(c == 1)
		flags |= 0x1;
	//Check overflow
	if(c6 != c7)
		flags |= 0x4;

	result[0] = s & 0xFF;
	result[1] = flags & 0xFF;
}

//This is an 8-bit subtractor (op1 - op2). Returns difference as first in array, then flags.
void sub8_old(byte i1, byte i2, byte *result)
{
	byte op1 = i1 & 0xFF;
	byte op2 = i2 & 0xFF;


	byte d = 0; //8-bits
	byte b = 0; //Assume its 1-bit

	byte flags = 0;

	//Those following two will be used to calculate overflow flag later
	byte MSB1 = (op1 & 0x80) & 0xFF; //No need to shift them, since all of them are in the same place
	byte MSB2 = (op2 & 0x80) & 0xFF;

	for(byte i = 0; i < 8; ++i) 
	{
		//Take next bit
		byte bit1 = ((op1 & ((1 << i) & 0xFF)) >> i) & 0xFF;
		byte bit2 = ((op2 & ((1 << i) & 0xFF)) >> i) & 0xFF;

		//Sub them from each other
		byte z1 = (bit1 ^ bit2) & 0xFF;
		d |= ((b ^ z1) << i) & 0xFF;

		//Calculate borrow
		byte bit1_neg = (~bit1) & 0xF;   //Invert the bit (Only the bit)
		byte z1_neg = (~z1) & 0xF;

		byte b1 = (z1_neg & b) & 0xFF;
		byte b2 = (bit1_neg & bit2) & 0xFF;
		b = (b1 | b2) & 0xFF;

		//Auxiliary flag set
		if(b != 0 && i == 1)
			flags |= 0x2;
	}

	//Check carry flag
	if(b == 1)
		flags |= 0x1;

	//Check overflow
	byte resultMSB = (d & 0x80) & 0xFF;
	if((MSB1 == MSB2) && MSB1 != resultMSB)
		flags |= 0x4;
	result[0] = d & 0xFF;
	result[1] = flags & 0xFF;
}

//Take complement
byte comp8(byte op)
{
	return (((~op) & 0xFF) + 1) & 0xFF;
}

//Convert to signed
byte toSigned8(byte op)
{
	if(op < -127)
		return comp8(op);
	return op;
}

//Converts normal number to BCD
//https://stackoverflow.com/questions/13247647/convert-integer-from-pure-binary-to-bcd
int int2BCD(int dec)
{
	int i = dec;
	int bcd = 0;
	int j = 0;
	
	for(; i > 0; i /= 10, j++)
		bcd |= (i % 10) << (j << 2);

	return bcd;
}
