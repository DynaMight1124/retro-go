/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#ifndef _spc7110_h_
#define _spc7110_h_

#include <time.h>
#include "port.h"
#include <stdint.h>

#define DECOMP_BUFFER_SIZE 0x10000

void Del7110Gfx(void);
uint8_t S9xGetSPC7110(uint16_t Address);
uint8_t S9xGetSPC7110Byte(uint32_t Address);
uint8_t* Get7110BasePtr(uint32_t);
void S9xSetSPC7110(uint8_t data, uint16_t Address);
void S9xSpc7110Init(void);
uint8_t* Get7110BasePtr(uint32_t);
void S9xSpc7110Reset(void);
void S9xUpdateRTC(void);
void Do7110Logging(void);
int32_t S9xRTCDaysInMonth(int32_t month, int32_t year);

typedef struct
{
   uint8_t reg[16];
   int16_t index;
   uint8_t control;
   bool    init;
   time_t  last_used;
} S7RTC;

typedef struct
{
   uint8_t  reg4800;
   uint8_t  reg4801;
   uint8_t  reg4802;
   uint8_t  reg4803;
   uint8_t  reg4804;
   uint8_t  reg4805;
   uint8_t  reg4806;
   uint8_t  reg4807;
   uint8_t  reg4808;
   uint8_t  reg4809;
   uint8_t  reg480A;
   uint8_t  reg480B;
   uint8_t  reg480C;
   uint8_t  reg4811;
   uint8_t  reg4812;
   uint8_t  reg4813;
   uint8_t  reg4814;
   uint8_t  reg4815;
   uint8_t  reg4816;
   uint8_t  reg4817;
   uint8_t  reg4818;
   uint8_t  reg4820;
   uint8_t  reg4821;
   uint8_t  reg4822;
   uint8_t  reg4823;
   uint8_t  reg4824;
   uint8_t  reg4825;
   uint8_t  reg4826;
   uint8_t  reg4827;
   uint8_t  reg4828;
   uint8_t  reg4829;
   uint8_t  reg482A;
   uint8_t  reg482B;
   uint8_t  reg482C;
   uint8_t  reg482D;
   uint8_t  reg482E;
   uint8_t  reg482F;
   uint8_t  reg4830;
   uint8_t  reg4831;
   uint8_t  reg4832;
   uint8_t  reg4833;
   uint8_t  reg4834;
   uint8_t  reg4840;
   uint8_t  reg4841;
   uint8_t  reg4842;
   uint8_t  AlignBy;
   uint8_t  written;
   uint8_t  offset_add;
   uint32_t DataRomOffset;
   uint32_t DataRomSize;
   uint32_t bank50Internal;
   uint8_t  bank50[DECOMP_BUFFER_SIZE];
} SPC7110Regs;

EXTERN_C SPC7110Regs *s7rPtr;
#define s7r (*s7rPtr)
extern S7RTC rtc_f9;

bool8 S9xSaveSPC7110RTC (S7RTC *rtc_f9);
bool8 S9xLoadSPC7110RTC (S7RTC *rtc_f9);

#endif
