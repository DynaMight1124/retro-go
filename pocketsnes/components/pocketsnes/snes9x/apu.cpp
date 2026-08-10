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

#include "snes9x.h"
#include <rg_system.h>
#include "spc700.h"
#include "apu.h"
#include "soundux.h"
#include "cpuexec.h"

/* For note-triggered SPC dump support */
#include "snapshot.h"

extern "C" {const char *S9xGetFilenameInc (const char *);}

int spc_is_dumping=0;
int spc_is_dumping_temp;
uint8 spc_dump_dsp[0x100]; 

extern int32  NoiseFreq       [32];
#ifdef DEBUGGER
void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
					   int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0);
#endif

bool8 S9xInitAPU ()
{
    if (IAPU.RAM) return TRUE;

    IAPU.RAM = (uint8 *) rg_alloc (0x10000, MEM_SLOW);
    
    if (!IAPU.RAM)
    {
		S9xDeinitAPU ();
		return (FALSE);
    }
    
    memset(IAPU.RAM, 0, 0x10000);

    return (TRUE);
}

void S9xDeinitAPU ()
{
    if (IAPU.RAM)
    {
		free ((void *) IAPU.RAM);
		IAPU.RAM = NULL;
    }
}

EXTERN_C uint8 APUROM [64];

void S9xResetAPU ()
{

    int i;

    Settings.APUEnabled = TRUE;

	ZeroMemory(spc_dump_dsp, 0x100);
	ZeroMemory(IAPU.RAM, 0x100);
	memset(IAPU.RAM+0x20, 0xFF, 0x20);
	memset(IAPU.RAM+0x60, 0xFF, 0x20);
	memset(IAPU.RAM+0xA0, 0xFF, 0x20);
	memset(IAPU.RAM+0xE0, 0xFF, 0x20);

	for(i=1;i<256;i++)
	{
		memmove(IAPU.RAM+(i<<8), IAPU.RAM, 0x100);
	}
	
    ZeroMemory (APU.OutPorts, sizeof(APU.OutPorts));
    IAPU.DirectPage = IAPU.RAM;
    // memmove converted: Different mallocs [Neb]
    // DS2 DMA notes: The APU ROM is not 32-byte aligned [Neb]
    memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
    // memmove converted: Different mallocs [Neb]
    // DS2 DMA notes: The APU ROM is not 32-byte aligned [Neb]
    memmove (APU.ExtraRAM, APUROM, sizeof (APUROM));
    IAPU.PC = IAPU.RAM + IAPU.RAM [0xfffe] + (IAPU.RAM [0xffff] << 8);
    APU.Cycles = 0;
    IAPU.Registers.YA.W = 0;
    IAPU.Registers.X = 0;
    IAPU.Registers.S = 0xef;
    IAPU.Registers.P = 0x02;
    S9xAPUUnpackStatus ();
    IAPU.Registers.PC = 0;
    IAPU.APUExecuting = Settings.APUEnabled;
#ifdef SPC700_SHUTDOWN
    IAPU.WaitAddress1 = NULL;
    IAPU.WaitAddress2 = NULL;
    IAPU.WaitCounter = 1;
#endif
    APU.ShowROM = TRUE;
    IAPU.RAM [0xf1] = 0x80;

    for (i = 0; i < 3; i++)
    {
		APU.TimerEnabled [i] = FALSE;
		APU.TimerValueWritten [i] = 0;
		APU.TimerTarget [i] = 0;
		APU.Timer [i] = 0;
    }
    for (int j = 0; j < 0x80; j++)
		APU.DSP [j] = 0;

    IAPU.TwoCycles = IAPU.OneCycle * 2;

    for (i = 0; i < 256; i++)
		S9xAPUCycles [i] = S9xAPUCycleLengths [i] * IAPU.OneCycle;

	APU.DSP [APU_ENDX] = 0;
	APU.DSP [APU_KOFF] = 0;
	APU.DSP [APU_KON] = 0;
	APU.DSP [APU_FLG] = APU_SOFT_RESET | APU_MUTE;
	APU.KeyedChannels = 0;

	S9xResetSound (TRUE);
	S9xSetEchoEnable (0);
}

void S9xSetAPUDSP (uint8 byte)
{
    uint8 reg = IAPU.RAM [0xf2];
	static uint8 KeyOn;
	static uint8 KeyOnPrev;
    int i;

	spc_dump_dsp[reg] = byte;

    switch (reg)
    {
    case APU_FLG:
		if (byte & APU_SOFT_RESET)
		{
			APU.DSP [reg] = APU_MUTE | APU_ECHO_DISABLED | (byte & 0x1f);
			APU.DSP [APU_ENDX] = 0;
			APU.DSP [APU_KOFF] = 0;
			APU.DSP [APU_KON] = 0;
			S9xSetEchoWriteEnable (FALSE);
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] DSP reset\n", ICPU.Scanline);
#endif
			// Kill sound
			S9xResetSound (FALSE);
		}
		else
		{
			S9xSetEchoWriteEnable (!(byte & APU_ECHO_DISABLED));
			so.mute_sound = !!(byte & APU_MUTE);
			if (byte & APU_MUTE)
			{
#ifdef DEBUGGER
				if (Settings.TraceSoundDSP)
					S9xTraceSoundDSP ("[%d] Mute sound\n", ICPU.Scanline);
#endif
				S9xSetSoundMute (TRUE);
			}
			else
				S9xSetSoundMute (FALSE);
			
			SoundData.noise_hertz = NoiseFreq [byte & 0x1f];
			for (i = 0; i < 8; i++)
			{
				if (SoundData.channels [i].type == SOUND_NOISE)
					S9xSetSoundFrequency (i, SoundData.noise_hertz);
			}
		}
		break;
    case APU_NON:
		if (byte != APU.DSP [APU_NON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Noise:", ICPU.Scanline);
#endif
			uint8 mask = 1;
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				int type;
				if (byte & mask)
				{
					type = SOUND_NOISE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d,", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
#endif
				}
				else
				{
					type = SOUND_SAMPLE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
#endif
				}
				S9xSetSoundType (c, type);
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		break;
    case APU_MVOL_LEFT:
		if (byte != APU.DSP [APU_MVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume left:%d\n", 
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((int8) byte, (int8) APU.DSP [APU_MVOL_RIGHT]);
		}
		break;
    case APU_MVOL_RIGHT:
		if (byte != APU.DSP [APU_MVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((int8) APU.DSP [APU_MVOL_LEFT], (int8) byte);
		}
		break;
    case APU_EVOL_LEFT:
		if (byte != APU.DSP [APU_EVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume left:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((int8) byte, (int8) APU.DSP [APU_EVOL_RIGHT]);
		}
		break;
    case APU_EVOL_RIGHT:
		if (byte != APU.DSP [APU_EVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((int8) byte, (int8) APU.DSP [APU_EVOL_RIGHT]);
		}
		break;
    case APU_ENDX:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] Reset ENDX\n", ICPU.Scanline);
#endif
		byte = 0;
		break;
		
    case APU_KOFF:
		//		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key off:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER
					
					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif		    
					if (APU.KeyedChannels & mask)
					{
						KeyOnPrev&=~mask;
						APU.KeyedChannels &= ~mask;
						APU.DSP [APU_KON] &= ~mask;
						S9xSetSoundKeyOff (c);
					}
				}
				else if((KeyOnPrev&mask)!=0)
				{
					KeyOnPrev&=~mask;
					APU.KeyedChannels |= mask;
					APU.DSP [APU_KOFF] &= ~mask;
					APU.DSP [APU_ENDX] &= ~mask;
					S9xPlaySample (c);
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
			APU.DSP [APU_KOFF] = byte;
			return;
		}
    case APU_KON:
		if (spc_is_dumping)
		{
			if (byte & ~spc_is_dumping_temp)
			{
				IAPU.Registers.PC = IAPU.PC - IAPU.RAM;
				S9xAPUPackStatus();
				S9xSPCDump (S9xGetFilenameInc (".spc"));
				spc_is_dumping = 0;
			}
		}
		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER
			
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key on:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif		    
					// Pac-In-Time requires that channels can be key-on
					// regardeless of their current state.
					if((APU.DSP [APU_KOFF] & mask) ==0)
					{
						KeyOnPrev&=~mask;
						APU.KeyedChannels |= mask;
						//APU.DSP [APU_KON] |= mask;
						//APU.DSP [APU_KOFF] &= ~mask;
						APU.DSP [APU_ENDX] &= ~mask;
						S9xPlaySample (c);
					}
					else KeyOn|=mask;
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		spc_is_dumping_temp = byte;
		return;
		
    case APU_VOL_LEFT + 0x00:
    case APU_VOL_LEFT + 0x10:
    case APU_VOL_LEFT + 0x20:
    case APU_VOL_LEFT + 0x30:
    case APU_VOL_LEFT + 0x40:
    case APU_VOL_LEFT + 0x50:
    case APU_VOL_LEFT + 0x60:
    case APU_VOL_LEFT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume left: %d\n", 
				ICPU.Scanline, reg>>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (int8) byte, (int8) APU.DSP [reg + 1]);
		}
		break;
    case APU_VOL_RIGHT + 0x00:
    case APU_VOL_RIGHT + 0x10:
    case APU_VOL_RIGHT + 0x20:
    case APU_VOL_RIGHT + 0x30:
    case APU_VOL_RIGHT + 0x40:
    case APU_VOL_RIGHT + 0x50:
    case APU_VOL_RIGHT + 0x60:
    case APU_VOL_RIGHT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume right: %d\n", 
				ICPU.Scanline, reg >>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (int8) APU.DSP [reg - 1], (int8) byte);
		}
		break;
		
    case APU_P_LOW + 0x00:
    case APU_P_LOW + 0x10:
    case APU_P_LOW + 0x20:
    case APU_P_LOW + 0x30:
    case APU_P_LOW + 0x40:
    case APU_P_LOW + 0x50:
    case APU_P_LOW + 0x60:
    case APU_P_LOW + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq low: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		S9xSetSoundHertz(reg >> 4, ((((int16_t) byte + ((int16_t) APU.DSP [reg + 1] << 8)) & FREQUENCY_MASK) * 32000) >> 12);
		break;
		
    case APU_P_HIGH + 0x00:
    case APU_P_HIGH + 0x10:
    case APU_P_HIGH + 0x20:
    case APU_P_HIGH + 0x30:
    case APU_P_HIGH + 0x40:
    case APU_P_HIGH + 0x50:
    case APU_P_HIGH + 0x60:
    case APU_P_HIGH + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq high: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		S9xSetSoundHertz(reg >> 4, (((byte << 8) + APU.DSP [reg - 1]) & FREQUENCY_MASK) * 8);
		break;
		
    case APU_ADSR1 + 0x00:
    case APU_ADSR1 + 0x10:
    case APU_ADSR1 + 0x20:
    case APU_ADSR1 + 0x30:
    case APU_ADSR1 + 0x40:
    case APU_ADSR1 + 0x50:
    case APU_ADSR1 + 0x60:
    case APU_ADSR1 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr1: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			S9xFixEnvelope (reg >> 4, APU.DSP [reg + 2], byte, APU.DSP [reg + 1]);
		}
		break;
		
    case APU_ADSR2 + 0x00:
    case APU_ADSR2 + 0x10:
    case APU_ADSR2 + 0x20:
    case APU_ADSR2 + 0x30:
    case APU_ADSR2 + 0x40:
    case APU_ADSR2 + 0x50:
    case APU_ADSR2 + 0x60:
    case APU_ADSR2 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr2: %02x\n", 
				ICPU.Scanline, reg>>4, byte);
#endif
			S9xFixEnvelope (reg >> 4, APU.DSP [reg + 1], APU.DSP [reg - 1], byte);
		}
		break;
		
    case APU_GAIN + 0x00:
    case APU_GAIN + 0x10:
    case APU_GAIN + 0x20:
    case APU_GAIN + 0x30:
    case APU_GAIN + 0x40:
    case APU_GAIN + 0x50:
    case APU_GAIN + 0x60:
    case APU_GAIN + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d gain: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			S9xFixEnvelope (reg >> 4, byte, APU.DSP [reg - 2], APU.DSP [reg - 1]);
		}
		break;
		
    case APU_PMON:
		if (byte != APU.DSP [APU_PMON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] FreqMod:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetFrequencyModulationEnable (byte);
		}
		break;
		
    case APU_EON:
		if (byte != APU.DSP [APU_EON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] Echo:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetEchoEnable (byte);
		}
		break;
		
    case APU_EFB:
		S9xSetEchoFeedback ((int8) byte);
		break;
		
    case APU_EDL:
		S9xSetEchoDelay (byte & 0xf);
		break;
		
    case APU_C0:
    case APU_C1:
    case APU_C2:
    case APU_C3:
    case APU_C4:
    case APU_C5:
    case APU_C6:
    case APU_C7:
		S9xSetFilterCoefficient (reg >> 4, (int8) byte);
		break;
    default:
		// XXX
		//printf ("Write %02x to unknown APU register %02x\n", byte, reg);
		break;
    }
	
	KeyOnPrev|=KeyOn;
	KeyOn=0;
	
    if (reg < 0x80)
		APU.DSP [reg] = byte;
}

void S9xFixEnvelope (int channel, uint8 gain, uint8 adsr1, uint8 adsr2)
{
    if (adsr1 & 0x80) // ADSR mode
    {
		// XXX: can DSP be switched to ADSR mode directly from GAIN/INCREASE/
		// DECREASE mode? And if so, what stage of the sequence does it start
		// at?
		if (S9xSetSoundMode (channel, MODE_ADSR))
            S9xSetSoundADSR(channel, adsr1 & 0xf, (adsr1 >> 4) & 7, adsr2 & 0x1f, (adsr2 >> 5) & 7, 8);
    }
    else
    {
		// Gain mode
		if (!(gain & 0x80))
		{
			if (S9xSetSoundMode (channel, MODE_GAIN))
			{
				S9xSetEnvelopeRate (channel, 0, 0, gain & 0x7f, 0);
				S9xSetEnvelopeHeight (channel, gain & 0x7f);
			}
		}
		else if (gain & 0x40)
		{
			// Increase mode
			if(S9xSetSoundMode(channel, (gain & 0x20) ?
							   MODE_INCREASE_BENT_LINE :
							   MODE_INCREASE_LINEAR))
				S9xSetEnvelopeRate(channel, gain, 1, 127, (3 << 28) | gain);
		}
		else if (gain & 0x20)
		{
			if(S9xSetSoundMode(channel, MODE_DECREASE_EXPONENTIAL))
				S9xSetEnvelopeRate(channel, gain, -1, 0, (4 << 28) | gain);
		}
		else
		{
			if (S9xSetSoundMode(channel, MODE_DECREASE_LINEAR))
				S9xSetEnvelopeRate(channel, gain, -1, 0, (3 << 28) | gain);
		}
    }
}

void S9xSetAPUControl (uint8 byte)
{
	//if (byte & 0x40)
	//printf ("*** Special SPC700 timing enabled\n");
	if ((byte & 1) && !APU.TimerEnabled [0])
	{
		APU.Timer [0] = 0;
		IAPU.RAM [0xfd] = 0;
		if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
			APU.TimerTarget [0] = 0x100;
	}
	if ((byte & 2) && !APU.TimerEnabled [1])
	{
		APU.Timer [1] = 0;
		IAPU.RAM [0xfe] = 0;
		if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
			APU.TimerTarget [1] = 0x100;
	}
	if ((byte & 4) && !APU.TimerEnabled [2])
	{
		APU.Timer [2] = 0;
		IAPU.RAM [0xff] = 0;
		if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
			APU.TimerTarget [2] = 0x100;
	}
	APU.TimerEnabled [0] = !!(byte & 1);
	APU.TimerEnabled [1] = !!(byte & 2);
	APU.TimerEnabled [2] = !!(byte & 4);
	
	if (byte & 0x10)
		IAPU.RAM [0xF4] = IAPU.RAM [0xF5] = 0;
	
	if (byte & 0x20)
		IAPU.RAM [0xF6] = IAPU.RAM [0xF7] = 0;
	
	if (byte & 0x80)
	{
		if (!APU.ShowROM)
		{
			// memmove converted: Different mallocs [Neb]
			// DS2 DMA notes: The APU ROM is not 32-byte aligned [Neb]
			memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
			APU.ShowROM = TRUE;
		}
	}
	else if (APU.ShowROM)
	{
		APU.ShowROM = FALSE;
		// memmove converted: Different mallocs [Neb]
		// DS2 DMA notes: The APU ROM is not 32-byte aligned [Neb]
		memmove (&IAPU.RAM [0xffc0], APU.ExtraRAM, sizeof (APUROM));
	}
	IAPU.RAM [0xf1] = byte;
}

uint8 S9xGetAPUDSP ()
{
    uint8 reg = IAPU.RAM [0xf2] & 0x7f;
    uint8 byte = APU.DSP [reg];
	
    switch (reg)
    {
    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
		if (SoundData.channels [reg >> 4].state == SOUND_SILENT)
			return 0;
		return (SoundData.channels [reg >> 4].sample >> 8) | (SoundData.channels [reg >> 4].sample & 0xff);
	case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
		// DrUm78: Those games require the old code to be playable (most of them have the "long note" bug though)
		// The list may be updated if some other games are identifed as non-working with the new code
		if (strcasestr (Memory.ROMName, "HU TENGAI MAKYO ZERO" ) != NULL || /* Tengai Makyo Zero */
			strcasestr (Memory.ROMName, "JUMP TENGAIMAKYO ZERO") != NULL ||
			strcasestr (Memory.ROMName, "CLAY FIGHTER"         ) != NULL || /* Clay Fighter 1 & 2 */
			strcasestr (Memory.ROMName, "CLAYFIGHTER"          ) != NULL ||
			strcasestr (Memory.ROMName, "CLAYMATES"            ) != NULL ||
			strcasestr (Memory.ROMName, "WEAPONLORD"           ) != NULL)   /* WeaponLord */
		{
			return ((uint8) S9xGetEnvelopeHeight (reg >> 4));
		}
		else
		{
			int32_t eVal = SoundData.channels [reg >> 4].envx;
			return (eVal > 0x7F) ? 0x7F : (eVal < 0 ? 0 : eVal);
		}
	default:
		break;
    }
    return byte;
}

