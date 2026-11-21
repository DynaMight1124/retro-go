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

#include "interrupts.h"

VE_VMS_INTERRUPTS::VE_VMS_INTERRUPTS()
{
   Reset    = 0;
   INT0     = 0;
   INT1     = 0;
   INT2     = 0;
   INT3     = 0;
   T0HOV    = 0;
   T1HLOV   = 0;
   SIO0     = 0;
   SIO1     = 0;
   RFB      = 0;
   P3_data  = 0;
   P3_Taken = true;
}

VE_VMS_INTERRUPTS::~VE_VMS_INTERRUPTS()
{
}

//Setters
void VE_VMS_INTERRUPTS::setReset(){
	Reset = 1;
}
void VE_VMS_INTERRUPTS::clearReset(){
	Reset = 0;
}

void VE_VMS_INTERRUPTS::setINT0(){
	INT0 = 1;
}
void VE_VMS_INTERRUPTS::clearINT0(){
	INT0 = 0;
}

void VE_VMS_INTERRUPTS::setINT1(){
	INT1 = 1;
}
void VE_VMS_INTERRUPTS::clearINT1(){
	INT1 = 0;
}

void VE_VMS_INTERRUPTS::setINT2(){
	INT2 = 1;
}
void VE_VMS_INTERRUPTS::clearINT2(){
	INT2 = 0;
}

void VE_VMS_INTERRUPTS::setINT3(){
	INT3 = 1;
}
void VE_VMS_INTERRUPTS::clearINT3(){
	INT3 = 0;
}

void VE_VMS_INTERRUPTS::setT0HOV(){
	T0HOV = 1;
}
void VE_VMS_INTERRUPTS::clearT0HOV(){
	T0HOV = 0;
}

void VE_VMS_INTERRUPTS::setT1HLOV(){
	T1HLOV = 1;
}
void VE_VMS_INTERRUPTS::clearT1HLOV(){
	T1HLOV = 0;
}

void VE_VMS_INTERRUPTS::setSIO0(){
	SIO0 = 1;
}
void VE_VMS_INTERRUPTS::clearSIO0(){
	SIO0 = 0;
}

void VE_VMS_INTERRUPTS::setSIO1(){
	SIO1 = 1;
}
void VE_VMS_INTERRUPTS::clearSIO1(){
	SIO1 = 0;
}

void VE_VMS_INTERRUPTS::setRFB(){
	RFB = 1;
}
void VE_VMS_INTERRUPTS::clearRFB(){
	RFB = 0;
}

void VE_VMS_INTERRUPTS::setP3(){
	P3_data = 1;
}
void VE_VMS_INTERRUPTS::clearP3(){
	P3_data = 0;
}

//Getters
byte VE_VMS_INTERRUPTS::getReset()
{
	return Reset;
}

byte VE_VMS_INTERRUPTS::getINT0()
{
	return  INT0;
}

byte VE_VMS_INTERRUPTS::getINT1()
{
	return INT1;
}

byte VE_VMS_INTERRUPTS::getINT2()
{
	return INT2;
}

byte VE_VMS_INTERRUPTS::getINT3()
{
	return  INT3;
}

byte VE_VMS_INTERRUPTS::getT0HOV()
{
	return T0HOV;
}

byte VE_VMS_INTERRUPTS::getT1HLOV()
{
	return T1HLOV;
}

byte VE_VMS_INTERRUPTS::getSIO0()
{
	return  SIO0;
}

byte VE_VMS_INTERRUPTS::getSIO1()
{
	return SIO1;
}

byte VE_VMS_INTERRUPTS::getRFB()
{
	return RFB;
}

byte VE_VMS_INTERRUPTS::getP3()
{
	return P3_data;
}

void VE_VMS_INTERRUPTS::serialize(VE_STATE &s)
{
   s.b(P3_Taken);
   s.u8(Reset);
   s.u8(INT0);
   s.u8(INT1);
   s.u8(INT2);
   s.u8(INT3);
   s.u8(T0HOV);
   s.u8(T1HLOV);
   s.u8(SIO0);
   s.u8(SIO1);
   s.u8(RFB);
   s.u8(P3_data);
}
