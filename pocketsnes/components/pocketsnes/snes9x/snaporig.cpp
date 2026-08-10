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

#include <rg_system.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#if defined(__unix) || defined(__linux)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "cpuexec.h"
#include "snapshot.h"
#include "snaporig.h"
#include "display.h"
#include "apu.h"
#include "soundux.h"

struct SOrigSnapshot {
    struct SOrigPPU OrigPPU;
    struct SOrigDMA OrigDMA [8];
    struct SOrigRegisters OrigRegisters;
    struct SOrigCPUState OrigCPU;
    struct SOrigAPU OrigAPU;
    SOrigSoundData OrigSoundData;
    struct SOrigAPURegisters OrigAPURegisters;
    char ROMFilename [1025];
};

static struct SOrigSnapshot *os = NULL;

static int ReadBlock (const char *key, void *block, int max_len, STREAM snap);
static int ReadOrigSnapshot (STREAM);

bool8 S9xLoadOrigSnapshot (const char *filename)
{
    FILE* fp;

    fp = fopen(filename, "rb");
    if(NULL == fp)
        return (FALSE);

    if (!os)
        os = (struct SOrigSnapshot *)rg_alloc(sizeof(struct SOrigSnapshot), MEM_SLOW);

    int result;
    result = ReadOrigSnapshot (fp);

    fclose(fp);
    free(os);
    os = NULL;
    return (result == SUCCESS);
}

static int ReadBlock (const char *key, void *block, int max_len, STREAM snap)
{
    char buffer [20];
    int len = 0;
    int rem = 0;
    
    if (READ_STREAM (buffer, 11, snap) != 11 ||
	strncmp (buffer, key, 4) != 0 ||
	(len = atoi (&buffer [4])) == 0)
	return (WRONG_FORMAT);

    if (len > max_len)
    {
	rem = len - max_len;
	len = max_len;
    }
    if (READ_STREAM (block, len, snap) != len)
	return (WRONG_FORMAT);

    if (rem)
    {
	char *junk = new char [rem];
	READ_STREAM (junk, rem, snap);
	delete[] junk;
    }

    return (SUCCESS);
}

static int ReadOrigSnapshot (STREAM snap)
{
    char buffer [_MAX_PATH];
    char rom_filename [_MAX_PATH];
    int result;
    int i;
    int j;

    int version;
    int len = strlen (ORIG_SNAPSHOT_MAGIC) + 1 + 4 + 1;
    if (READ_STREAM (buffer, len, snap) != len)
	return (WRONG_FORMAT);
    if (strncmp (buffer, ORIG_SNAPSHOT_MAGIC, strlen (ORIG_SNAPSHOT_MAGIC)) != 0)
	return (WRONG_FORMAT);
    if ((version = atoi (&buffer [strlen (SNAPSHOT_MAGIC) + 1])) > ORIG_SNAPSHOT_VERSION)
	return (WRONG_VERSION);

    if ((result = ReadBlock ("NAM:", rom_filename, _MAX_PATH, snap)) != SUCCESS)
	return (result);

    if ((result = ReadBlock ("CPU:", &os->OrigRegisters, sizeof (struct SOrigRegisters), snap)) != SUCCESS)
	return (result);

    if ((result = ReadBlock ("CP0:", &os->OrigCPU, sizeof (struct SOrigCPUState), snap)) != SUCCESS)
	return (result);

    if ((result = ReadBlock ("PPU:", &os->OrigPPU, sizeof (struct SOrigPPU), snap)) != SUCCESS)
	return (result);

    if ((result = ReadBlock ("DMA:", os->OrigDMA, sizeof (os->OrigDMA), snap)) != SUCCESS)
	return (result);

    if ((result = ReadBlock ("VRA:", Memory.VRAM, 0x10000, snap)) != SUCCESS)
	return (result);
    if ((result = ReadBlock ("RAM:", Memory.RAM, 0x20000, snap)) != SUCCESS)
	return (result);
    if ((result = ReadBlock ("SRA:", ::SRAM, 0x20000, snap)) != SUCCESS)
	return (result);
    if ((result = ReadBlock ("FIL:", Memory.FillRAM, 0x8000, snap)) != SUCCESS)
	return (result);
    if (ReadBlock ("APU:", &os->OrigAPURegisters, sizeof (struct SOrigAPURegisters), snap) == SUCCESS)
    {
	if ((result = ReadBlock ("ARE:", &os->OrigAPU, sizeof (struct SOrigAPU), snap)) != SUCCESS)
	    return (result);
	if ((result = ReadBlock ("ARA:", IAPU.RAM, 0x10000, snap)) != SUCCESS)
	    return (result);
	if ((result = ReadBlock ("SOU:", &os->OrigSoundData,
				 sizeof (SOrigSoundData), snap)) != SUCCESS)
	    return (result);

	SoundData.master_volume [0] = os->OrigSoundData.master_volume_left;
	SoundData.master_volume [1] = os->OrigSoundData.master_volume_right;
	SoundData.echo_volume [0] = os->OrigSoundData.echo_volume_left;
	SoundData.echo_volume [1] = os->OrigSoundData.echo_volume_right;
	SoundData.echo_enable = os->OrigSoundData.echo_enable;
	SoundData.echo_feedback = os->OrigSoundData.echo_feedback;
	SoundData.echo_ptr = os->OrigSoundData.echo_ptr;
	SoundData.echo_buffer_size = os->OrigSoundData.echo_buffer_size;
	SoundData.echo_write_enabled = os->OrigSoundData.echo_write_enabled;
	SoundData.echo_channel_enable = os->OrigSoundData.echo_channel_enable;
	SoundData.pitch_mod = os->OrigSoundData.pitch_mod;

	for (i = 0; i < 3; i++)
	    SoundData.dummy [i] = os->OrigSoundData.dummy [i];
	for (i = 0; i < NUM_CHANNELS; i++)
	{
	    SoundData.channels [i].state = os->OrigSoundData.channels [i].state;
	    SoundData.channels [i].type = os->OrigSoundData.channels [i].type;
	    SoundData.channels [i].volume_left = os->OrigSoundData.channels [i].volume_left;
	    SoundData.channels [i].volume_right = os->OrigSoundData.channels [i].volume_right;
	    SoundData.channels [i].hertz = os->OrigSoundData.channels [i].frequency;
	    SoundData.channels [i].count = os->OrigSoundData.channels [i].count;
	    SoundData.channels [i].loop = os->OrigSoundData.channels [i].loop;
	    SoundData.channels [i].envx = os->OrigSoundData.channels [i].envx;
	    SoundData.channels [i].left_vol_level = os->OrigSoundData.channels [i].left_vol_level;
	    SoundData.channels [i].right_vol_level = os->OrigSoundData.channels [i].right_vol_level;
	    SoundData.channels [i].envx_target = os->OrigSoundData.channels [i].envx_target;
	    SoundData.channels [i].env_error = os->OrigSoundData.channels [i].env_error;
	    SoundData.channels [i].erate = os->OrigSoundData.channels [i].erate;
	    SoundData.channels [i].direction = os->OrigSoundData.channels [i].direction;
	    SoundData.channels [i].attack_rate = os->OrigSoundData.channels [i].attack_rate;
	    SoundData.channels [i].decay_rate = os->OrigSoundData.channels [i].decay_rate;
	    SoundData.channels [i].sustain_rate = os->OrigSoundData.channels [i].sustain_rate;
	    SoundData.channels [i].release_rate = os->OrigSoundData.channels [i].release_rate;
	    SoundData.channels [i].sustain_level = os->OrigSoundData.channels [i].sustain_level;
	    SoundData.channels [i].sample = os->OrigSoundData.channels [i].sample;
	    for (j = 0; j < 16; j++)
		SoundData.channels [i].decoded [j] = os->OrigSoundData.channels [i].decoded [j];

	    for (j = 0; j < 2; j++)
		SoundData.channels [i].previous [j] = os->OrigSoundData.channels [i].previous [j];

	    SoundData.channels [i].sample_number = os->OrigSoundData.channels [i].sample_number;
	    SoundData.channels [i].last_block = os->OrigSoundData.channels [i].last_block;
	    SoundData.channels [i].needs_decode = os->OrigSoundData.channels [i].needs_decode;
	    SoundData.channels [i].block_pointer = os->OrigSoundData.channels [i].block_pointer;
	    SoundData.channels [i].sample_pointer = os->OrigSoundData.channels [i].sample_pointer;
	    SoundData.channels [i].mode = os->OrigSoundData.channels [i].mode;
	}

	S9xSetSoundMute (FALSE);
	IAPU.PC = IAPU.RAM + IAPU.Registers.PC;
	S9xAPUUnpackStatus ();
    }

    return (SUCCESS);
}
