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

#ifndef _RAM_H_
#define _RAM_H_

#include "common.h"
#include "state.h"

//SFR
#define ACC 0x100
#define PSW 0x101
#define B 0x102
#define C 0x103
#define TRL 0x104
#define TRH 0x105
#define SP 0x106
#define PCON 0x107
#define IE 0x108
#define IP 0x109
#define EXT 0x10D
#define OCR 0x10E
#define T0CNT 0x110
#define T0PRR 0x111
#define T0L 0x112
#define T0LR 0x113
#define T0H 0x114
#define T0HR 0x115
#define T1CNT 0x118
#define T1LC 0x11a
#define T1LR 0x11b
#define T1HC 0x11c
#define T1HR 0x11d
#define MCR 0x120
#define STAD 0x122
#define CNR 0x123
#define TDR 0x124
#define XBNK 0x125
#define VCCR 0x127
#define SCON0 0x130
#define SBUF0 0x131
#define SBR 0x132
#define SCON1 0x134
#define SBUF1 0x135
#define P1 0x144
#define P1DDR 0x145
#define P1FCR 0x146
#define P3 0x14c
#define P3DDR 0x14d
#define P3INT 0x14e
#define P7 0x15c
#define I01CR 0x15d
#define I23CR 0x15e
#define ISL 0x15f
#define VSEL 0x163
#define VRMAD1 0x164
#define VRMAD2 0x165
#define VTRBF 0x166
#define BTCR 0x17f
#define XRAM 0x180

class VE_VMS_RAM
{
public:
    //Extra
    //The following 2 hold values for T1LC and T1HC until T1CNT bit 4 is set
    byte T1LC_Temp;
    byte T1HC_Temp;
    
    /* Timer 1's counters. Wider than a byte on purpose: the overflow test is
       "past 255", which a byte can never be. */
    int T1RL_data;
    int T1RH_data;


    VE_VMS_RAM();
    ~VE_VMS_RAM();

    void serialize(VE_STATE &s);

    //Setters and getters
    byte readByte(size_t adr);

    //No banking or checking, use carefully!
    byte readByte_RAW(size_t adr);

    ///This is used by the GPU only.
    byte readByteXRAM(size_t adr, int bank);

    void writeByte(size_t adr, byte b);

    void writeByte_RAW(size_t adr, byte b);

    //Stack operations (To ensure writing to bank 0 only)
    void stackPush(byte d);

    byte stackPop();

    byte *getData();
    
private:
    byte *data;
    byte *wram;
    byte *xram0;
    byte *xram1;
    byte *xram2;
};

#endif // _RAM_H_
