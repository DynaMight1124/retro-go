#pragma once

#include "stdint.h"
#include "globals.h"
#include "roms.h"

#include "hwaudio/ym2151.h"

#include "engine/audio/commands.h"
#include "engine/audio/osoundadr.h"

/*
// PCM Sample Indexes
enum
{
    pcm_sample_CRASH1  = 0xD0, // 0xD0 - Crash 1
    pcm_sample_GURGLE  = 0xD1, // 0xD1 - Gurgle
    pcm_sample_SLIP    = 0xD2, // 0xD2 - Slip
    pcm_sample_CRASH2  = 0xD3, // 0xD3 - Crash 2
    pcm_sample_CRASH3  = 0xD4, // 0xD4 - Crash 3
    pcm_sample_SKID    = 0xD5, // 0xD5 - Skid
    pcm_sample_REBOUND = 0xD6, // 0xD6 - Rebound
    pcm_sample_HORN    = 0xD7, // 0xD7 - Horn
    pcm_sample_TYRES   = 0xD8, // 0xD8 - Tyre Squeal
    pcm_sample_SAFETY  = 0xD9, // 0xD9 - Safety Zone
    pcm_sample_LOSKID  = 0xDA, // 0xDA - Lofi skid (is this used)
    pcm_sample_CHEERS  = 0xDB, // 0xDB - Cheers
    pcm_sample_VOICE1  = 0xDC, // 0xDC - Voice 1, Checkpoint
    pcm_sample_VOICE2  = 0xDD, // 0xDD - Voice 2, Congratulations
    pcm_sample_VOICE3  = 0xDE, // 0xDE - Voice 3, Get Ready
    pcm_sample_VOICE4  = 0xDF, // 0xDF - Voice 4, You're doing great (unused, plays at wrong pitch)
    pcm_sample_WAVE    = 0xE0, // 0xE0 - Wave
    pcm_sample_CRASH4  = 0xE1, // 0xE1 - Crash 4
};
*/

// Internal Channel Offsets in RAM
// Channels 0-7: YM Channels
const static uint16_t channel_YM1 = 0x020; // f820
const static uint16_t channel_YM2 = 0x040;
const static uint16_t channel_YM3 = 0x060;
const static uint16_t channel_YM4 = 0x080;
const static uint16_t channel_YM5 = 0x0A0;
const static uint16_t channel_YM6 = 0x0C0;
const static uint16_t channel_YM7 = 0x0E0;
const static uint16_t channel_YM8 = 0x100; // f900

// Channels 8-13: PCM Drum Channels for music
const static uint16_t channel_PCM_DRUM1 = 0x120;
const static uint16_t channel_PCM_DRUM2 = 0x140;
const static uint16_t channel_PCM_DRUM3 = 0x160;
const static uint16_t channel_PCM_DRUM4 = 0x180;
const static uint16_t channel_PCM_DRUM5 = 0x1A0;
const static uint16_t channel_PCM_DRUM6 = 0x1C0;

// Channels 14-21: PCM Sound Effects
const static uint16_t channel_PCM_FX1 = 0x1E0; // f9e0: Crowd, Cheers, Wave
const static uint16_t channel_PCM_FX2 = 0x200;
const static uint16_t channel_PCM_FX3 = 0x220; // fa20: Slip, Safety Zone
const static uint16_t channel_PCM_FX4 = 0x240;
const static uint16_t channel_PCM_FX5 = 0x260; // fa60: Crash 1, Rebound, Crash2
const static uint16_t channel_PCM_FX6 = 0x280;
const static uint16_t channel_PCM_FX7 = 0x2A0; // faa0: Voices
const static uint16_t channel_PCM_FX8 = 0x2C0;

// Channel Mapping Info. Used to play sound effects and music at the same time. 
const static uint16_t channel_MAP1 = 0x2E0;
const static uint16_t channel_MAP2 = 0x300;
const static uint16_t channel_MAP3 = 0x320;
const static uint16_t channel_MAP4 = 0x340;
const static uint16_t channel_MAP5 = 0x360;
const static uint16_t channel_MAP6 = 0x380;
const static uint16_t channel_MAP7 = 0x3A0;

// Channels 22-23: YM Sound Effects
const static uint16_t channel_YM_FX1 = 0x3C0; // fbc0: Signal 1, Signal 2
const static uint16_t channel_YM_FX2 = 0x3E0;

// Engine Commands in RAM
const static int16_t channel_ENGINE_CH1 = 0x400; // 0xFC00: Engine Channel - Player's Car
const static int16_t channel_ENGINE_CH2 = 0x420; // 0xFC20: Engine Channel - Traffic 1
const static int16_t channel_ENGINE_CH3 = 0x440; // 0xFC40: Engine Channel - Traffic 2
const static int16_t channel_ENGINE_CH4 = 0x460; // 0xFC60: Engine Channel - Traffic 3
const static int16_t channel_ENGINE_CH5 = 0x480; // 0xFC80: Engine Channel - Traffic 4



// ------------------------------------------------------------------------------------------------
// Internal Format of Sound Data in RAM before sending to hardware
// ------------------------------------------------------------------------------------------------
//
//0x20 byte chunks of information per channel in memory. Format is as follows:
//
//+0x00: [Byte] Flags e65--cn-
//              n = FM noise channel (1 = yes, 0 = no)
//              c = Corresponding music channel is enabled
//              5 = Code exists to support this being set. But OutRun doesn't use it. Not sure of it's purpose.
//              6 = ???
//              e = channel enable (1 = active, 0 = disabled). 
//+0x01: [Byte] Flags -m---ccc
//	            c = YM Channel Number
//              m = possibly a channel mute? 
//                  Counters and positions still tick.  (1 = active, 0 = disabled).
//+0x02: [Byte] Used as end marker when bit 1 of 0x0D is set
//+0x03: [Word] Position in sequence
//+0x05: [Word] Sequence End Marker
//+0x07: [Word] Address of next command (see 0x2E7)
//              This is essentially within the same block of sound information
//+0x09: [Byte] Note Offset: From lowest note. Essentially an index into a table.
//+0x0A: [Byte] Offset into 0x20 block of memory. Used to store positioning info.
//+0x0B: [Byte] Use Phase and Amplitude Modulation Sensitivity Table
//+0x0C: [Byte] FM: Select FM Data Block To Send. 0 = No Block.
//+0x0D: [Byte] FM: End Marker Flags
//              Flags ------10
//              0 = Set high byte of end marker from data.
//              1 = Do not calculate end marker. Use value from data.
//+0x0E: [Byte] Sample Index / Command
//+0x0F: [Word] ?
//+0x10: [Byte] Offset into Phase and Amplitude Modulation Sensitivity Table (see 0x1DF)
//+0x11: [Byte] Volume: Left Channel
//+0x12: [Byte] Volume: Right Channel
//+0x13: [Word] PCM: Wave Start Address / Loop Address 
//              FM:  Note & Octave Info (top bit denotes noise channel?)
//+0x14: [Byte] FM Channels only. Phase and Amplitude Modulation Sensitivity
//+0x15: [Byte] Wave End Address HIGH 8 bits
//+0x16: [Byte] PCM Pitch
//+0x17: [Byte] Flags m-bbccla
//              a = active (0 = active,  1 = inactive) 
//              l = loop   (0 = enabled, 1 = disabled)
//              c = channel pair select
//              b = bank
//              m = Music Sample (Drums etc.)
//+0x18: [Byte] FM Loop Counter. Specifies number of times to trigger command sequence. 
//              Counter used at 0x45f
//
//+0x1C: [Word] Sequence Address #1
//+0x1E: [Word] Sequence Address #2

enum
{
    ch_FLAGS       = 0x00,
    ch_FM_FLAGS    = 0x01,
    ch_END_MARKER  = 0x02,
    ch_SEQ_POS     = 0x03,
    ch_SEQ_END     = 0x05,
    ch_SEQ_CMD     = 0x07,
    ch_NOTE_OFFSET = 0x09,
    ch_MEM_OFFSET  = 0x0A,
    ch_FM_PHASETBL = 0x0B,
    ch_FM_BLOCK    = 0x0C,
    ch_FM_MARKER   = 0x0D,
    ch_COMMAND     = 0x0E,
    ch_UNKNOWN     = 0x0F,
    ch_FM_PHASEOFF = 0x10,
    ch_VOL_L       = 0x11,
    ch_VOL_R       = 0x12,
    ch_PCM_ADR1L   = 0x13,
    ch_FM_NOTE     = 0x13,
    ch_PCM_ADR1H   = 0x14,
    ch_FM_PHASE_AMP= 0x14,
    ch_PCM_ADR2    = 0x15,
    ch_PCM_PITCH   = 0x16,
    ch_CTRL        = 0x17,
    ch_FM_LOOP     = 0x18,
    ch_SEQ_ADR1    = 0x1C,
    ch_SEQ_ADR2    = 0x1E,
};


// +0x00: [Byte] Engine Volume
// +0x01: [Byte] Engine Volume (seems same as 0x00)
// +0x02: [Byte] Flags -6543210
//               6 = 
//               5 = Set mutes channel completely
//               4 = 
//               3 = Set denotes loop address has been set
//               2 = Set denotes loop disabled
//               1 = Denote engine volume set
//               0 = Set denotes start address / end address has been set
// +0x03: [Byte] Engine Sample Loop counter (0 - 8) 
//               Used as offset into separate 0x20 block to store data at (e.g. Engine Pitch1, Pitch2, Vol)
// +0x04: [Byte] Engine Pitch Low
// +0x05: [Byte] Engine Pitch High
// +0x06: [Byte] Volume adjusted by 0x76D7 routine
// +0x07: [Byte]
// +0x08: [Byte] 0 = Channel Muted, 1 = Channel Active


enum
{
    ch_engines_VOL0    = 0x00,
    ch_engines_VOL1    = 0x01,
    ch_engines_FLAGS   = 0x02,
    ch_engines_OFFSET  = 0x03,
    ch_engines_LOOP    = 0x03,
    ch_engines_PITCH_L = 0x04,
    ch_engines_PITCH_H = 0x05,
    ch_engines_VOL6    = 0x06,
    ch_engines_ACTIVE  = 0x08,
};





// Command to process
extern uint8_t OSound_command_input;

// [+0] Unused
// [+1] Engine pitch high
// [+2] Engine pitch low
// [+3] Engine pitch vol
// [+4] Traffic data #1 
// [+5] Traffic data #2 
// [+6] Traffic data #3 
// [+7] Traffic data #4
extern uint8_t OSound_engine_data[8];

void OSound_init(uint8_t* pcm_ram);
void OSound_init_fm_chip();
void OSound_tick();
