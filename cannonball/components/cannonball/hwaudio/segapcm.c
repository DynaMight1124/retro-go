/***************************************************************************
    Sega 8-Bit PCM Driver.
    
    This driver is based upon the MAME source code, with some minor 
    modifications to integrate it into the Cannonball framework.

    Note, that I've altered this driver to output at a fixed 44,100Hz.
    This is to avoid the need for downsampling.  
    
    See http://mamedev.org/source/docs/license.txt for more details.
***************************************************************************/

/**
 * RAM DESCRIPTION ===============
 * 
 * 0x00 - 0x07, 0x80 - 0x87 : CHANNEL #1  
 * 0x08 - 0x0F, 0x88 - 0x8F : CHANNEL #2
 * 0x10 - 0x17, 0x90 - 0x97 : CHANNEL #3  
 * 0x18 - 0x1F, 0x98 - 0x9F : CHANNEL #4
 * 0x20 - 0x27, 0xA0 - 0xA7 : CHANNEL #5  
 * 0x28 - 0x2F, 0xA8 - 0xAF : CHANNEL #6
 * 0x30 - 0x37, 0xB0 - 0xB7 : CHANNEL #7  
 * 0x38 - 0x3F, 0xB8 - 0xBF : CHANNEL #8
 * 0x40 - 0x47, 0xC0 - 0xC7 : CHANNEL #9  
 * 0x48 - 0x4F, 0xC8 - 0xCF : CHANNEL #10
 * 0x50 - 0x57, 0xD0 - 0xD7 : CHANNEL #11 
 * 0x58 - 0x5F, 0xD8 - 0xDF : CHANNEL #12
 * 0x60 - 0x67, 0xE0 - 0xE7 : CHANNEL #13 
 * 0x68 - 0x6F, 0xE8 - 0xEF : CHANNEL #14
 * 0x70 - 0x77, 0xF0 - 0xF7 : CHANNEL #15 
 * 0x78 - 0x7F, 0xF8 - 0xFF : CHANNEL #16
 * 
 * 
 * CHANNEL DESCRIPTION ===================
 * 
 * OFFS | BITS     | DESCRIPTION 
 * -----+----------+---------------------------------
 * 0x00 | -------- | (unknown) 
 * 0x01 | -------- | (unknown) 
 * 0x02 | vvvvvvvv | Volume LEFT 
 * 0x03 | vvvvvvvv | Volume RIGHT 
 * 0x04 | aaaaaaaa | Wave Start Address LOW 8 bits 
 * 0x05 | aaaaaaaa | Wave Start Address HIGH 8 bits 
 * 0x06 | eeeeeeee | Wave End Address HIGH 8 bits 
 * 0x07 | dddddddd | Delta (pitch) 
 * 0x80 | -------- | (unknown) 
 * 0x81 | -------- | (unknown) 
 * 0x82 | -------- | (unknown)
 * 0x83 | -------- | (unknown) 
 * 0x84 | llllllll | Wave Loop Address LOW 8 bits
 * 0x85 | llllllll | Wave Loop Address HIGH 8 bits 
 * 0x86 | ------la | Flags: a = active (0 = active,  1 = inactive) 
 *      |          |        l = loop   (0 = enabled, 1 = disabled)
 * 
 */
#include <stdint.h>
#include "hwaudio/segapcm.h"

uint8_t SegaPCM_initalized;

// Sample Frequency in use
uint32_t SegaPCM_sample_freq;

// How many channels to support (mono/stereo)
uint8_t SegaPCM_channels;

uint32_t SegaPCM_buffer_size;

const static uint8_t SEGAPCM_MONO             = 1;
const static uint8_t SEGAPCM_STEREO           = 2;

const static uint8_t SEGAPCM_LEFT             = 0;
const static uint8_t SEGAPCM_RIGHT            = 1;

//  Buffer size for one frame (excluding channel info)
uint32_t SegaPCM_frame_size;

// Volume of sound chip
float SegaPCM_volume;

void SegaPCM_clear_buffer();
void SegaPCM_write_buffer(const uint8_t, uint32_t, int16_t);
int16_t SegaPCM_read_buffer(const uint8_t, uint32_t);

uint32_t PCM_BUFFER_SIZE;

// Sound buffer stream
int16_t SegaPCM_buffer[(REAL_AUDIO_FREQUENCY / 30) * 2]; // maximum possible buffer size.  

// Frames per second
uint32_t SegaPCM_fps; 

// PCM Chip Emulation
uint8_t* SegaPCM_ram;
uint8_t SegaPCM_low[16];
uint8_t* SegaPCM_pcm_rom;
int32_t SegaPCM_max_addr;
int32_t SegaPCM_bankshift;
int32_t SegaPCM_bankmask;
int32_t SegaPCM_rgnmask;

uint32_t SegaPCM_downsample_fixed;

void SegaPCM_Create(uint32_t clock, RomLoader* rom, uint8_t* ram, int32_t bank)
{
    int32_t i;
    SegaPCM_volume     = 1.0;
    SegaPCM_initalized = 0;

    SegaPCM_ram = ram;
    SegaPCM_pcm_rom = rom->rom;  
    SegaPCM_max_addr = rom->length;
    SegaPCM_bankshift = bank & 0xFF;
    SegaPCM_rgnmask = SegaPCM_max_addr - 1;

    int32_t mask = bank >> 16;
    if (mask == 0)
        mask = SEGAPCM_BANK_MASK7 >> 16;

    int32_t rom_mask;
    for (rom_mask = 1; rom_mask < SegaPCM_max_addr; rom_mask *= 2);
    rom_mask--;

    SegaPCM_bankmask = mask & (rom_mask >> SegaPCM_bankshift);

    for (i = 0; i < 0x100; i++)
        ram[i] = 0xff;
}

void SegaPCM_Destroy()
{
}

void SegaPCM_init(int32_t fps)
{
	/* 30 = max fps, 2 = number of channels (Stereo) */
	PCM_BUFFER_SIZE = (AUDIO_FREQUENCY / 30) * 2;

    SegaPCM_downsample_fixed = (uint32_t)((32000.0 / (double)AUDIO_FREQUENCY) * 65536.0);

    SegaPCM_fps = fps;
    SegaPCM_sample_freq = AUDIO_FREQUENCY;
    SegaPCM_channels    = SEGAPCM_STEREO;
    SegaPCM_frame_size =  SegaPCM_sample_freq / SegaPCM_fps;
    SegaPCM_buffer_size = SegaPCM_frame_size * SegaPCM_channels;

    SegaPCM_initalized = 1;
}

void SegaPCM_stream_update()
{
    int ch;
    SegaPCM_clear_buffer();

    // loop over channels
    for (ch = 0; ch < 16; ch++)
    {
        uint8_t *regs = SegaPCM_ram + 8 * ch;

        // only process active channels
        if ((regs[0x86] & 1) == 0) 
        {             
            uint8_t *rom  = SegaPCM_pcm_rom + ((regs[0x86] & SegaPCM_bankmask) << SegaPCM_bankshift);

            uint32_t addr = (regs[0x85] << 16) | (regs[0x84] << 8) | SegaPCM_low[ch];
            uint32_t loop = (regs[0x05] << 16) | (regs[0x04] << 8);
            uint8_t end   =  regs[0x06] + 1;

            uint32_t i;

            // loop over samples on this channel
            for (i = 0; i < SegaPCM_frame_size; i++) 
            {
                int8_t v = 0;

                // handle looping if we've hit the end
                if ((addr >> 16) == end) 
                {
                    if ((regs[0x86] & 2) == 0) 
                    {
                        addr = loop;
                    } 
                    else 
                    {
                        regs[0x86] |= 1;
                        break;
                    }
                }

                // fetch the sample
                v = rom[(addr >> 8) & SegaPCM_rgnmask] - 0x80;

                // apply panning
                SegaPCM_write_buffer(SEGAPCM_LEFT,  i, SegaPCM_read_buffer(SEGAPCM_LEFT,  i) + (v * regs[2]));
                SegaPCM_write_buffer(SEGAPCM_RIGHT, i, SegaPCM_read_buffer(SEGAPCM_RIGHT, i) + (v * regs[3]));

                // Advance.
                // Cannonball Change: Output at a fixed 44,100Hz.
                uint32_t increment = (regs[7] * SegaPCM_downsample_fixed) >> 16;
                addr = (addr + increment) & 0xffffff;
            }

            // store back the updated address and info
            regs[0x84] = addr >> 8;
            regs[0x85] = addr >> 16;
            SegaPCM_low[ch] = regs[0x86] & 1 ? 0 : addr;
        }
    }
}

// Set soundchip volume (0 = Off, 10 = Loudest)
void SegaPCM_set_volume(uint8_t v)
{
    if (v > 10) 
        return;
    
    SegaPCM_volume = (float) (v / 10.0);
}

void SegaPCM_clear_buffer()
{
    uint32_t i;
    for (i = 0; i < SegaPCM_buffer_size; i++)
        SegaPCM_buffer[i] = 0;
}

void SegaPCM_write_buffer(const uint8_t channel, uint32_t address, int16_t value)
{
    //buffer[channel + (address * channels)] = (int16_t) (value * volume); // Unused for now
    SegaPCM_buffer[channel + (address * SegaPCM_channels)] = value;
}

int16_t SegaPCM_read_buffer(const uint8_t channel, uint32_t address)
{
    return SegaPCM_buffer[channel + (address * SegaPCM_channels)];
}

int16_t* SegaPCM_get_buffer()
{
    return SegaPCM_buffer;
}
