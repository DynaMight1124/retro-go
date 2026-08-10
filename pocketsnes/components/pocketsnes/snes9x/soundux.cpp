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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define CLIP16(v) \
	if ((v) < -32768) \
    (v) = -32768; \
	else \
	if ((v) > 32767) \
(v) = 32767

#define CLIP16_latch(v,l) \
	if ((v) < -32768) \
{ (v) = -32768; (l)++; }\
	else \
	if ((v) > 32767) \
{ (v) = 32767; (l)++; }

#define CLIP24(v) \
	if ((v) < -8388608) \
    (v) = -8388608; \
	else \
	if ((v) > 8388607) \
(v) = 8388607

#define CLIP8(v) \
	if ((v) < -128) \
    (v) = -128; \
	else \
	if ((v) > 127) \
(v) = 127

#include "snes9x.h"
#include "soundux.h"
#include "apu.h"
#include "memmap.h"
#include "cpuexec.h"

extern int32 *Echo;
extern int32 *MixBuffer;
extern int32 *EchoBuffer;
extern int32 FilterTaps [8];
static uint8 FilterTapDefinitionBitfield;
// In the above, bit I is set if FilterTaps[I] is non-zero.
extern unsigned long Z;
extern int32 Loop [16];

extern long FilterValues[4][2];
extern int32 NoiseFreq [32];

uint32 AttackRate [16] =
{
   4100, 2600, 1500, 1000, 640, 380, 260, 160,
   96,   64,   40,   24,   16,  10,  6,   1
};

uint32 DecayRate [8] =
{
   1200, 740, 440, 290, 180, 110, 74, 37
};

uint32 DecreaseRateExp [32] =
{
   0xFFFFFFFF, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
   7100,       5900,  4700,  3500,  2900,  2400,  1800,  1500,
   1200,       880,   740,   590,   440,   370,   290,   220,
   180,        150,   110,   92,    74,    55,    37,    18
};

uint32 IncreaseRate [32] =
{
   0xFFFFFFFF, 4100, 3100, 2600, 2000, 1500, 1300, 1000,
   770,        640,  510,  380,  320,  260,  190,  160,
   130,        96,   80,   64,   48,   40,   32,   24,
   20,         16,   12,   10,   8,    6,    4,    2
};

#define SustainRate DecreaseRateExp

// precalculated env rates for S9xSetEnvRate
uint32 AttackERate     [16][10];
uint32 DecayERate       [8][10];
uint32 SustainERate    [32][10];
uint32 IncreaseERate   [32][10];
uint32 DecreaseERateExp[32][10];
uint32 KeyOffERate         [10];

static int32 noise_gen;

#undef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))

#define FIXED_POINT 0x10000UL
#define FIXED_POINT_REMAINDER 0xffffUL
#define FIXED_POINT_SHIFT 16

#define VOL_DIV8  0x8000
#define VOL_DIV16 0x0080
#define ENVX_SHIFT 24

// F is channel's current frequency and M is the 16-bit modulation waveform
// from the previous channel multiplied by the current envelope volume level.
#define PITCH_MOD(F,M) ((F) * ((((unsigned long) (M)) + 0x800000) >> 16) >> 7)
//#define PITCH_MOD(F,M) ((F) * ((((M) & 0x7fffff) >> 14) + 1) >> 8)

#define LAST_SAMPLE 0xffffff
#define JUST_PLAYED_LAST_SAMPLE(c) ((c)->sample_pointer >= LAST_SAMPLE)

STATIC inline uint8 *S9xGetSampleAddress (int sample_number)
{
    uint32 addr = (((APU.DSP[APU_DIR] << 8) + (sample_number << 2)) & 0xffff);
    return (IAPU.RAM + addr);
}

void S9xAPUSetEndOfSample (int i, Channel *ch)
{
    ch->state = SOUND_SILENT;
    ch->mode = MODE_NONE;
    APU.DSP [APU_ENDX] |= 1 << i;
    APU.DSP [APU_KON] &= ~(1 << i);
    APU.DSP [APU_KOFF] &= ~(1 << i);
    APU.KeyedChannels &= ~(1 << i);
}

void S9xAPUSetEndX (int ch)
{
    APU.DSP [APU_ENDX] |= 1 << ch;
}

void S9xSetEnvRate (Channel *ch, unsigned long rate, int direction, int target, unsigned int mode)
{
    ch->envx_target = target;
	
    if (rate == ~0UL)
    {
		ch->direction = 0;
		rate = 0;
    }
    else
		ch->direction = direction;
	
    if (rate == 0 || so.playback_rate == 0)
		ch->erate = 0;
    else
    {
		switch (mode >> 28)
		{
			case 0: // Attack
				ch->erate = AttackERate[ch->env_ind_attack][ch->state];
				break;
			case 1: // Decay
				ch->erate = DecayERate[ch->env_ind_decay][ch->state];
				break;
			case 2: // Sustain
				ch->erate = SustainERate[ch->env_ind_sustain][ch->state];
				break;
			case 3: // Increase
				ch->erate = IncreaseERate[mode & 0x1f][ch->state];
				break;
			case 4: // DecreaseExp
				ch->erate = DecreaseERateExp[mode & 0x1f][ch->state];
				break;
			case 5: // KeyOff
				ch->erate = KeyOffERate[ch->state];
				break;
		}
	}
}

void S9xSetEnvelopeRate (int channel, unsigned long rate, int direction, int target, unsigned int mode)
{
    S9xSetEnvRate (&SoundData.channels [channel], rate, direction, target, mode);
}

void S9xSetSoundVolume (int channel, short volume_left, short volume_right)
{
    Channel *ch = &SoundData.channels[channel];
    ch->volume_left = volume_left;
    ch->volume_right = volume_right;
    ch-> left_vol_level = (ch->envx * volume_left) / 128;
    ch->right_vol_level = (ch->envx * volume_right) / 128;
}

void S9xSetMasterVolume (short volume_left, short volume_right)
{
    if (Settings.DisableMasterVolume || SNESGameFixes.EchoOnlyOutput)
    {
		SoundData.master_volume [0] = SoundData.master_volume [1] = 127;
    }
    else
    {
		SoundData.master_volume [0] = volume_left;
		SoundData.master_volume [1] = volume_right;
    }
}

void S9xSetEchoVolume (short volume_left, short volume_right)
{
    SoundData.echo_volume [0] = volume_left;
    SoundData.echo_volume [1] = volume_right;
}

void S9xSetEchoEnable (uint8 byte)
{
    SoundData.echo_channel_enable = byte;
    if (!SoundData.echo_write_enabled || Settings.DisableSoundEcho)
		byte = 0;
    if (byte && !SoundData.echo_enable)
    {
		memset (Echo, 0, sizeof (Echo));
		memset (Loop, 0, sizeof (Loop));
    }
	
    SoundData.echo_enable = byte;
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
		if (byte & (1 << i))
			SoundData.channels [i].echo_buf_ptr = EchoBuffer;
		else
			SoundData.channels [i].echo_buf_ptr = NULL;
    }
}

void S9xSetEchoFeedback (int feedback)
{
    CLIP8(feedback);
    SoundData.echo_feedback = feedback;
}

void S9xSetEchoDelay (int delay)
{
	SoundData.echo_buffer_size = (512 * delay * so.playback_rate) / 32000;
	SoundData.echo_buffer_size <<= 1;
	if (SoundData.echo_buffer_size)
		SoundData.echo_ptr %= SoundData.echo_buffer_size;
	else
		SoundData.echo_ptr = 0;
	S9xSetEchoEnable (APU.DSP [APU_EON]);
}

void S9xSetEchoWriteEnable (uint8 byte)
{
    SoundData.echo_write_enabled = byte;
    S9xSetEchoDelay (APU.DSP [APU_EDL] & 15);
}

void S9xSetFrequencyModulationEnable (uint8 byte)
{
    SoundData.pitch_mod = byte & 0xFE;
}

void S9xSetSoundKeyOff (int channel)
{
    Channel *ch = &SoundData.channels[channel];
	
    if (ch->state != SOUND_SILENT)
    {
		ch->state = SOUND_RELEASE;
		ch->mode = MODE_RELEASE;
		S9xSetEnvRate (ch, 8, -1, 0, 5 << 28);
    }
}

void S9xFixSoundAfterSnapshotLoad ()
{
    SoundData.echo_write_enabled = !(APU.DSP [APU_FLG] & 0x20);
    SoundData.echo_channel_enable = APU.DSP [APU_EON];
    S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
    S9xSetEchoFeedback ((signed char) APU.DSP [APU_EFB]);
	
    S9xSetFilterCoefficient (0, (signed char) APU.DSP [APU_C0]);
    S9xSetFilterCoefficient (1, (signed char) APU.DSP [APU_C1]);
    S9xSetFilterCoefficient (2, (signed char) APU.DSP [APU_C2]);
    S9xSetFilterCoefficient (3, (signed char) APU.DSP [APU_C3]);
    S9xSetFilterCoefficient (4, (signed char) APU.DSP [APU_C4]);
    S9xSetFilterCoefficient (5, (signed char) APU.DSP [APU_C5]);
    S9xSetFilterCoefficient (6, (signed char) APU.DSP [APU_C6]);
    S9xSetFilterCoefficient (7, (signed char) APU.DSP [APU_C7]);
    for (int i = 0; i < 8; i++)
    {
		SoundData.channels[i].needs_decode = TRUE;
		S9xSetSoundFrequency (i, SoundData.channels[i].hertz);
		SoundData.channels [i].envxx = SoundData.channels [i].envx << ENVX_SHIFT;
    }
}

void S9xSetFilterCoefficient (int tap, int value)
{
	FilterTaps [tap & 7] = value;
	if (value == 0 || (tap == 0 && value == 127))
		FilterTapDefinitionBitfield &= ~(1 << (tap & 7));
	else
		FilterTapDefinitionBitfield |= 1 << (tap & 7);
}

void S9xSetSoundADSR (int channel, int attack_ind, int decay_ind, int sustain_ind, int sustain_level, int release_rate)
{
	int attack_rate  = AttackRate [attack_ind];
	int decay_rate   = DecayRate [decay_ind];
	int sustain_rate = SustainRate [sustain_ind];

	// Hack for ROMs that use a very short attack rate, key on a
	// channel, then switch to decay mode. e.g. Final Fantasy II.
	if(attack_rate == 1)
		attack_rate = 0;

	Channel* ch = &SoundData.channels[channel];
	ch->env_ind_attack = attack_ind;
	ch->env_ind_decay = decay_ind;
	ch->env_ind_sustain = sustain_ind;
	ch->attack_rate = attack_rate;
	ch->decay_rate = decay_rate;
	ch->sustain_rate = sustain_rate;
	ch->release_rate = release_rate;
	ch->sustain_level = sustain_level + 1;
	
    switch (SoundData.channels[channel].state)
    {
    case SOUND_ATTACK:
		S9xSetEnvRate (ch, attack_rate, 1, 127, 0);
		break;
		
    case SOUND_DECAY:
		S9xSetEnvRate (ch, decay_rate, -1,
			(MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3, 1 << 28);
		break;
    case SOUND_SUSTAIN:
		S9xSetEnvRate (ch, sustain_rate, -1, 0, 2 << 28);
		break;
    }
}

void S9xSetEnvelopeHeight (int channel, int level)
{
    Channel *ch = &SoundData.channels[channel];
	
    ch->envx = level;
    ch->envxx = level << ENVX_SHIFT;
	
    ch->left_vol_level = (level * ch->volume_left) / 128;
    ch->right_vol_level = (level * ch->volume_right) / 128;
	
    if (ch->envx == 0 && ch->state != SOUND_SILENT && ch->state != SOUND_GAIN)
    {
		S9xAPUSetEndOfSample (channel, ch);
    }
}

int S9xGetEnvelopeHeight (int channel)
{
    if ((Settings.SoundEnvelopeHeightReading ||
		SNESGameFixes.SoundEnvelopeHeightReading2) &&
        SoundData.channels[channel].state != SOUND_SILENT &&
        SoundData.channels[channel].state != SOUND_GAIN)
    {
        return (SoundData.channels[channel].envx);
    }

    //siren fix from XPP
    if (SNESGameFixes.SoundEnvelopeHeightReading2 &&
        SoundData.channels[channel].state != SOUND_SILENT)
    {
        return (SoundData.channels[channel].envx);
    }

    return (0);
}

#if 1
void S9xSetSoundSample (int, uint16)
{
}
#else
void S9xSetSoundSample (int channel, uint16 sample_number)
{
    register Channel *ch = &SoundData.channels[channel];

    if (ch->state != SOUND_SILENT &&
		sample_number != ch->sample_number)
    {
		int keep = ch->state;
		ch->state = SOUND_SILENT;
		ch->sample_number = sample_number;
		ch->loop = FALSE;
		ch->needs_decode = TRUE;
		ch->last_block = FALSE;
		ch->previous [0] = ch->previous[1] = 0;
		uint8 *dir = S9xGetSampleAddress (sample_number);
		ch->block_pointer = READ_WORD (dir);
		ch->sample_pointer = 0;
		ch->state = keep;
    }
}
#endif

void S9xSetSoundFrequency (int channel, int hertz)  // hertz [0~64K<<1]
{
	if (SoundData.channels[channel].type == SOUND_NOISE)
		hertz = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
	SoundData.channels[channel].frequency = (hertz * so.freqbase) >> 11;
}

void S9xSetSoundHertz (int channel, int hertz)
{
    SoundData.channels[channel].hertz = hertz;
    S9xSetSoundFrequency (channel, hertz);
}

void S9xSetSoundType (int channel, int type_of_sound)
{
    SoundData.channels[channel].type = type_of_sound;
}

bool8 S9xSetSoundMute (bool8 mute)
{
    bool8 old = so.mute_sound;
    so.mute_sound = mute;
    return (old);
}

void DecodeBlock (Channel *ch)
{
    int32 out;
	uint8 filter;
    unsigned char shift;
    signed char sample1, sample2;

    if (ch->block_pointer > 0x10000 - 9)
    {
		ch->last_block = TRUE;
		ch->loop = FALSE;
		ch->block = ch->decoded;
		return;
    }

   int8* compressed = (int8*) &IAPU.RAM [ch->block_pointer];

   filter = *compressed;
   if ((ch->last_block = filter & 1))
      ch->loop = (filter & 2) != 0;

   int16* raw = ch->block = ch->decoded;
   unsigned int i;

   compressed++;

   int32 prev0 = ch->previous [0];
   int32 prev1 = ch->previous [1];
   shift = filter >> 4;

   switch ((filter >> 2) & 3)
   {
   case 0:
      for (i = 8; i != 0; i--)
      {
         sample1 = *compressed++;
         sample2 = sample1 << 4;
         sample2 >>= 4;
         sample1 >>= 4;
         *raw++ = ((int32) sample1 << shift);
         *raw++ = ((int32) sample2 << shift);
      }
      prev1 = *(raw - 2);
      prev0 = *(raw - 1);
      break;
   case 1:
      for (i = 8; i != 0; i--)
      {
         sample1 = *compressed++;
         sample2 = sample1 << 4;
         sample2 >>= 4;
         sample1 >>= 4;
         prev0 = (int16) prev0;
         *raw++ = prev1 = ((int32) sample1 << shift) + prev0 - (prev0 >> 4);
         prev1 = (int16) prev1;
         *raw++ = prev0 = ((int32) sample2 << shift) + prev1 - (prev1 >> 4);
      }
      break;
   case 2:
      for (i = 8; i != 0; i--)
      {
         sample1 = *compressed++;
         sample2 = sample1 << 4;
         sample2 >>= 4;
         sample1 >>= 4;
         out = (sample1 << shift) - prev1 + (prev1 >> 4);
         prev1 = (int16) prev0;
         prev0 &= ~3;
         *raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) - (prev0 >> 4);
         out = (sample2 << shift) - prev1 + (prev1 >> 4);
         prev1 = (int16) prev0;
         prev0 &= ~3;
         *raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) - (prev0 >> 4);
      }
      break;
   case 3:
      for (i = 8; i != 0; i--)
      {
         sample1 = *compressed++;
         sample2 = sample1 << 4;
         sample2 >>= 4;
         sample1 >>= 4;
         out = (sample1 << shift);
         out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
         prev1 = (int16) prev0;
         prev0 &= ~3;
         *raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - (prev0 >> 4) - (prev1 >> 6);
         out = (sample2 << shift);
         out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
         prev1 = (int16) prev0;
         prev0 &= ~3;
         *raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - (prev0 >> 4) - (prev1 >> 6);
      }
      break;
   }
   ch->previous [0] = prev0;
   ch->previous [1] = prev1;
   ch->block_pointer += 9;
}

static inline void MixStereo (int sample_count)
{
	static int32 wave[SOUND_BUFFER_SIZE];

	int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];

	for (uint32 J = 0; J < NUM_CHANNELS; J++) 
	{
		Channel *ch = &SoundData.channels[J];

		if (ch->state == SOUND_SILENT)
			continue;

		int32 VL, VR;
		unsigned long freq0 = ch->frequency;

		bool8 mod = pitch_mod & (1 << J);

		if (ch->needs_decode) 
		{
			DecodeBlock(ch);
			ch->needs_decode = FALSE;
			ch->sample = ch->block[0];
			ch->sample_pointer = freq0 >> FIXED_POINT_SHIFT;
			if (ch->sample_pointer == 0)
				ch->sample_pointer = 1;
			if (ch->sample_pointer > SOUND_DECODE_LENGTH)
				ch->sample_pointer = SOUND_DECODE_LENGTH - 1;

			ch->next_sample=ch->block[ch->sample_pointer];
			ch->interpolate = 0;

			if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
				ch->interpolate = ((ch->next_sample - ch->sample) * 
					(long) freq0) / (long) FIXED_POINT;
		}
		VL = (ch->sample * ch-> left_vol_level) / 128;
		VR = (ch->sample * ch->right_vol_level) / 128;

		for (uint32 I = 0; I < (uint32) sample_count; I += 2)
		{
			unsigned long freq = freq0;

			if (mod)
				freq = PITCH_MOD(freq, wave [I / 2]);

			ch->env_error += ch->erate;
			if (ch->env_error >= FIXED_POINT) 
			{
				uint32 step = ch->env_error >> FIXED_POINT_SHIFT;

				switch (ch->state)
				{
				case SOUND_ATTACK:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;

					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_DECAY;
						if (ch->sustain_level != 8) 
						{
							S9xSetEnvRate (ch, ch->decay_rate, -1,
										   (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3, 1 << 28);
							break;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate(ch, ch->sustain_rate, -1, 0, 2 << 28);
					}
					break;

				case SOUND_DECAY:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= ch->envx_target)
					{
						if (ch->envx <= 0)
						{
							S9xAPUSetEndOfSample (J, ch);
							goto stereo_exit;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate(ch, ch->sustain_rate, -1, 0, 2 << 28);
					}
					break;

				case SOUND_SUSTAIN:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;

				case SOUND_RELEASE:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;

				case SOUND_INCREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;

					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0, 0);
					}
					break;

				case SOUND_INCREASE_BENT_LINE:
					if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
					{
						while (ch->env_error >= FIXED_POINT)
						{
							ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
							ch->env_error -= FIXED_POINT;
						}
						ch->envx = ch->envxx >> ENVX_SHIFT;
					}
					else
					{
						ch->env_error &= FIXED_POINT_REMAINDER;
						ch->envx += step << 1;
						ch->envxx = ch->envx << ENVX_SHIFT;
					}

					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0, 0);
					}
					break;

				case SOUND_DECREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx -= step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;

				case SOUND_DECREASE_EXPONENTIAL:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;

				case SOUND_GAIN:
					S9xSetEnvRate (ch, 0, -1, 0, 0);
					break;
				}
				ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
				ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
				VL = (ch->sample * ch-> left_vol_level) / 128;
				VR = (ch->sample * ch->right_vol_level) / 128;
			}

			ch->count += freq;
			if (ch->count >= FIXED_POINT)
			{
				VL = ch->count >> FIXED_POINT_SHIFT;
				ch->sample_pointer += VL;
				ch->count &= FIXED_POINT_REMAINDER;

				ch->sample = ch->next_sample;
				if (ch->sample_pointer >= SOUND_DECODE_LENGTH)
				{
					if (JUST_PLAYED_LAST_SAMPLE(ch))
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					do
					{
						ch->sample_pointer -= SOUND_DECODE_LENGTH;
						if (ch->last_block)
						{
							if (!ch->loop)
							{
								ch->sample_pointer = LAST_SAMPLE;
								ch->next_sample = ch->sample;
								break;
							}
							else
							{
								S9xAPUSetEndX (J);
								ch->last_block = FALSE;
								uint8 *dir = S9xGetSampleAddress (ch->sample_number);
								ch->block_pointer = READ_WORD(dir + 2);
							}
						}
						DecodeBlock (ch);
					} while (ch->sample_pointer >= SOUND_DECODE_LENGTH);
					if (!JUST_PLAYED_LAST_SAMPLE (ch))
						ch->next_sample = ch->block [ch->sample_pointer];
				}
				else
					ch->next_sample = ch->block [ch->sample_pointer];

				if (ch->type == SOUND_SAMPLE)
				{
					if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
					{
						ch->interpolate = ((ch->next_sample - ch->sample) * 
						(long) freq) / (long) FIXED_POINT;
						ch->sample = (int16) (ch->sample + (((ch->next_sample - ch->sample) * 
						(long) (ch->count)) / (long) FIXED_POINT));
					}		  
					else
						ch->interpolate = 0;
				}
				else
				{
					// Snes9x 1.53's SPC_DSP.cpp, by blargg
					int feedback = (noise_gen << 13) ^ (noise_gen << 14);
					noise_gen = (feedback & 0x4000) ^ (noise_gen >> 1);
					ch->sample = (noise_gen << 17) >> 17;
					ch->interpolate = 0;
				}

				VL = (ch->sample * ch-> left_vol_level) / 128;
				VR = (ch->sample * ch->right_vol_level) / 128;
			}
			else
			{
				if (ch->interpolate)
				{
					int32 s = (int32) ch->sample + ch->interpolate;

					CLIP16(s);
					ch->sample = (int16) s;
					VL = (ch->sample * ch-> left_vol_level) / 128;
					VR = (ch->sample * ch->right_vol_level) / 128;
				}
			}

			if (pitch_mod & (1 << (J + 1)))
				wave [I / 2] = ch->sample * ch->envx;

			MixBuffer [I    ] += VL;
			MixBuffer [I + 1] += VR;

			if (!ch->echo_buf_ptr)
				continue;

			ch->echo_buf_ptr [I    ] += VL;
			ch->echo_buf_ptr [I + 1] += VR;
		}
stereo_exit: ;
	}
}

void S9xMixSamples (uint16 *buffer, int sample_count)
{
	int J;
	int I;

	if (SoundData.echo_enable)
		memset (EchoBuffer, 0, sample_count * sizeof (EchoBuffer [0]));
	memset (MixBuffer, 0, sample_count * sizeof (MixBuffer [0]));

	MixStereo (sample_count);
	
	/* Mix and convert waveforms */
	int byte_count = sample_count << 1;
		
	// 16-bit sound
	if (so.mute_sound)
	{
		memset (buffer, 0, byte_count);
	}
	else
	{
		if (SoundData.echo_enable && SoundData.echo_buffer_size)
		{
			// 16-bit stereo sound with echo enabled ...
			if (FilterTapDefinitionBitfield == 0)
			{
				// ... but no filter defined.
				for (J = 0; J < sample_count; J++)
				{
					int E = Echo [SoundData.echo_ptr];
							
					Echo[SoundData.echo_ptr++] = (E * SoundData.echo_feedback) / 128 + EchoBuffer [J];
							
					if (SoundData.echo_ptr >= SoundData.echo_buffer_size)
						SoundData.echo_ptr = 0;
							
					I = (MixBuffer[J] * SoundData.master_volume [J & 1] + E * SoundData.echo_volume [J & 1]) / VOL_DIV16;
							
					CLIP16(I);
					buffer[J] = I;
				}
			}
			else
			{
				// ... with filter defined.
				for (J = 0; J < sample_count; J++)
				{
					Loop [(Z - 0) & 15] = Echo [SoundData.echo_ptr];
					int                                     E =  Loop [(Z -  0) & 15] * FilterTaps [0];
					if (FilterTapDefinitionBitfield & 0x02) E += Loop [(Z -  2) & 15] * FilterTaps [1];
					if (FilterTapDefinitionBitfield & 0x04) E += Loop [(Z -  4) & 15] * FilterTaps [2];
					if (FilterTapDefinitionBitfield & 0x08) E += Loop [(Z -  6) & 15] * FilterTaps [3];
					if (FilterTapDefinitionBitfield & 0x10) E += Loop [(Z -  8) & 15] * FilterTaps [4];
					if (FilterTapDefinitionBitfield & 0x20) E += Loop [(Z - 10) & 15] * FilterTaps [5];
					if (FilterTapDefinitionBitfield & 0x40) E += Loop [(Z - 12) & 15] * FilterTaps [6];
					if (FilterTapDefinitionBitfield & 0x80) E += Loop [(Z - 14) & 15] * FilterTaps [7];
					E /= 128;
					Z++;
							
					Echo[SoundData.echo_ptr++] = (E * SoundData.echo_feedback) / 128 + EchoBuffer[J];
							
					if (SoundData.echo_ptr >= SoundData.echo_buffer_size)
						SoundData.echo_ptr = 0;
							
					I = (MixBuffer[J] * SoundData.master_volume [J & 1] + E * SoundData.echo_volume [J & 1]) / VOL_DIV16;
							
					CLIP16(I);
					buffer[J] = I;
				}
			}
		}
		else
		{
			// 16-bit mono or stereo sound, no echo
			for (J = 0; J < sample_count; J++)
			{
				I = (MixBuffer[J] * SoundData.master_volume [J & 1]) / VOL_DIV16;
				
				CLIP16(I);
				buffer[J] = I;
			}
		}
	}
}

void S9xResetSound (bool8 full)
{
	for (int i = 0; i < 8; i++)
	{
		SoundData.channels[i].state = SOUND_SILENT;
		SoundData.channels[i].mode = MODE_NONE;
		SoundData.channels[i].type = SOUND_SAMPLE;
		SoundData.channels[i].volume_left = 0;
		SoundData.channels[i].volume_right = 0;
		SoundData.channels[i].hertz = 0;
		SoundData.channels[i].count = 0;
		SoundData.channels[i].loop = FALSE;
		SoundData.channels[i].envx_target = 0;
		SoundData.channels[i].env_error = 0;
		SoundData.channels[i].erate = 0;
		SoundData.channels[i].envx = 0;
		SoundData.channels[i].envxx = 0;
		SoundData.channels[i].left_vol_level = 0;
		SoundData.channels[i].right_vol_level = 0;
		SoundData.channels[i].direction = 0;
		SoundData.channels[i].attack_rate = 0;
		SoundData.channels[i].decay_rate = 0;
		SoundData.channels[i].sustain_rate = 0;
		SoundData.channels[i].release_rate = 0;
		SoundData.channels[i].sustain_level = 0;
		// notaz
		SoundData.channels[i].env_ind_attack = 0;
		SoundData.channels[i].env_ind_decay = 0;
		SoundData.channels[i].env_ind_sustain = 0;
		SoundData.echo_ptr = 0;
		SoundData.echo_feedback = 0;
		SoundData.echo_buffer_size = 1;
    }
    FilterTaps [0] = 127;
    FilterTaps [1] = 0;
    FilterTaps [2] = 0;
    FilterTaps [3] = 0;
    FilterTaps [4] = 0;
    FilterTaps [5] = 0;
    FilterTaps [6] = 0;
    FilterTaps [7] = 0;
    FilterTapDefinitionBitfield = 0;
    so.mute_sound = TRUE;
    noise_gen = 1;
    so.err_counter = 0;
	
    if (full)
    {
		SoundData.echo_enable = 0;
		SoundData.echo_write_enabled = 0;
		SoundData.echo_channel_enable = 0;
		SoundData.pitch_mod = 0;
		SoundData.master_volume[0] = 0;
		SoundData.master_volume[1] = 0;
		SoundData.echo_volume[0] = 0;
		SoundData.echo_volume[1] = 0;
		SoundData.noise_hertz = 0;
    }

    SoundData.master_volume [0] = SoundData.master_volume [1] = 127;
    if (so.playback_rate)
        so.err_rate = (uint32) (FIXED_POINT * SNES_SCANLINE_TIME / (1.0 / so.playback_rate));
    else
        so.err_rate = 0;
    so.mute_sound = true;
}

void S9xSetPlaybackRate (uint32 playback_rate)
{
    so.playback_rate = playback_rate;
    so.err_rate = (uint32) (SNES_SCANLINE_TIME * FIXED_POINT / (1.0 / (double) so.playback_rate));

	if (playback_rate)
	{
		// notaz: calculate a value (let's call it freqbase) to simplify channel freq calculations later.
		so.freqbase = (FIXED_POINT << 11) / (playback_rate * 33 / 32);
		// now precalculate env rates for S9xSetEnvRate
		static int steps [] =
			{
			0, 64, 619, 619, 128, 1, 64, 55, 64, 619
		};

		int i, u;

		for (u = 0 ; u < 10 ; u++)
		{
			int64_t fp1000su = ((int64_t) FIXED_POINT * 1000 * steps[u]);

			for (i = 0 ; i < 16 ; i++)
				AttackERate[i][u] = (uint32_t) (fp1000su / (AttackRate[i] * playback_rate));

			for (i = 0 ; i < 8 ; i++)
				DecayERate[i][u]  = (uint32_t) (fp1000su / (DecayRate[i]  * playback_rate));

			for (i = 0 ; i < 32 ; i++)
			{
				SustainERate[i][u] = (uint32_t) (fp1000su / (SustainRate[i] * playback_rate));
				IncreaseERate[i][u] = (uint32_t) (fp1000su / (IncreaseRate[i] * playback_rate));
				DecreaseERateExp[i][u] = (uint32_t) (fp1000su / (DecreaseRateExp[i] / 2 * playback_rate));
			}

			KeyOffERate[u] = (uint32_t) (fp1000su / (8 * playback_rate));
		}
	}

	S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
	for (int i = 0; i < 8; i++)
		S9xSetSoundFrequency (i, SoundData.channels [i].hertz);
}

bool8 S9xInitSound (int mode, bool8 stereo, int buffer_size)
{
    so.playback_rate = 0;
    
    S9xResetSound (TRUE);
	
    if (!(mode & 7))
		return (1);
	
    S9xSetSoundMute (TRUE);
    if (!S9xOpenSoundDevice (mode, stereo, buffer_size))
    {
#ifdef NOSOUND
		S9xMessage (S9X_WARNING, S9X_SOUND_NOT_BUILT,
                            "No sound support compiled in");
#else
		S9xMessage (S9X_ERROR, S9X_SOUND_DEVICE_OPEN_FAILED,
                            "Sound device open failed");
#endif
		return (0);
    }
	
    return (1);
}

bool8 S9xSetSoundMode (int channel, int mode)
{
    Channel *ch = &SoundData.channels[channel];
	
    switch (mode)
    {
    case MODE_RELEASE:
		if (ch->mode != MODE_NONE)
		{
			ch->mode = MODE_RELEASE;
			return (TRUE);
		}
		break;
		
    case MODE_DECREASE_LINEAR:
    case MODE_DECREASE_EXPONENTIAL:
    case MODE_GAIN:
    case MODE_INCREASE_LINEAR:
    case MODE_INCREASE_BENT_LINE:
		if (ch->mode != MODE_RELEASE)
		{
			ch->mode = mode;
			if (ch->state != SOUND_SILENT)
				ch->state = mode;
			
			return (TRUE);
		}
		break;
		
    case MODE_ADSR:
		if (ch->mode == MODE_NONE || ch->mode == MODE_ADSR)
		{
			ch->mode = mode;
			return (TRUE);
		}
    }
	
    return (FALSE);
}

void S9xPlaySample (int channel)
{
    Channel *ch = &SoundData.channels[channel];
    
    ch->state = SOUND_SILENT;
    ch->mode = MODE_NONE;
    ch->envx = 0;
    ch->envxx = 0;
	
    S9xFixEnvelope (channel,
		APU.DSP [APU_GAIN  + (channel << 4)], 
		APU.DSP [APU_ADSR1 + (channel << 4)],
		APU.DSP [APU_ADSR2 + (channel << 4)]);
	
    ch->sample_number = APU.DSP [APU_SRCN + channel * 0x10];
    if (APU.DSP [APU_NON] & (1 << channel))
		ch->type = SOUND_NOISE;
    else
		ch->type = SOUND_SAMPLE;
	
    S9xSetSoundFrequency (channel, ch->hertz);
    ch->loop = FALSE;
    ch->needs_decode = TRUE;
    ch->last_block = FALSE;
    ch->previous [0] = ch->previous[1] = 0;
    uint8 *dir = S9xGetSampleAddress (ch->sample_number);
    ch->block_pointer = READ_WORD (dir);
    ch->sample_pointer = 0;
    ch->env_error = 0;
    ch->next_sample = 0;
    ch->interpolate = 0;
    switch (ch->mode)
    {
    case MODE_ADSR:
		if (ch->attack_rate == 0)
		{
			if (ch->decay_rate == 0 || ch->sustain_level == 8)
			{
				ch->state = SOUND_SUSTAIN;
				ch->envx = (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3;
				S9xSetEnvRate (ch, ch->sustain_rate, -1, 0, 2 << 28);
			}
			else
			{
				ch->state = SOUND_DECAY;
				ch->envx = MAX_ENVELOPE_HEIGHT;
				S9xSetEnvRate (ch, ch->decay_rate, -1, 
					(MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3, 1 << 28);
			}
			ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
			ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
		}
		else
		{
			ch->state = SOUND_ATTACK;
			ch->envx = 0;
			ch->left_vol_level = 0;
			ch->right_vol_level = 0;
			S9xSetEnvRate (ch, ch->attack_rate, 1, MAX_ENVELOPE_HEIGHT, 0);
		}
		ch->envxx = ch->envx << ENVX_SHIFT;
		break;
		
    case MODE_GAIN:
		ch->state = SOUND_GAIN;
		break;
		
    case MODE_INCREASE_LINEAR:
		ch->state = SOUND_INCREASE_LINEAR;
		break;
		
    case MODE_INCREASE_BENT_LINE:
		ch->state = SOUND_INCREASE_BENT_LINE;
		break;
		
    case MODE_DECREASE_LINEAR:
		ch->state = SOUND_DECREASE_LINEAR;
		break;
		
    case MODE_DECREASE_EXPONENTIAL:
		ch->state = SOUND_DECREASE_EXPONENTIAL;
		break;
		
    default:
		break;
    }
	
    S9xFixEnvelope (channel,
		APU.DSP [APU_GAIN  + (channel << 4)], 
		APU.DSP [APU_ADSR1 + (channel << 4)],
		APU.DSP [APU_ADSR2 + (channel << 4)]);
}
