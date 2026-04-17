/***************************************************************************
    Interface to Ported Z80 Code.
    Handles the interface between 68000 program code and Z80.

    Also abstracted here, so the more complex OSound class isn't exposed
    to the main code directly
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "engine/outrun.h"
#include "engine/audio/osound.h"
#include "engine/audio/osoundint.h"
#include "hwaudio/ym2151.h"
#include "hwaudio/segapcm.h"
#include "main.h"

// SoundChip: Sega Custom Sample Generator
uint8_t OSoundInt_has_booted = 0;
uint8_t OSoundInt_engine_data[8];

// 4 MHz
static const uint32_t SOUND_CLOCK = 4000000;

// Reference to 0xFF bytes of PCM Chip RAM
uint8_t *OSOoundInt_pcm_ram = NULL;

// Controls what type of sound we're going to process in the interrupt routine
uint8_t sound_counter;

#define QUEUE_LENGTH 0x1F
uint8_t queue[QUEUE_LENGTH + 1];

// Number of sounds queued
uint8_t sounds_queued;

// Positions in the queue
uint8_t sound_head, sound_tail;


uint8_t OSoundInt_created_pcm = 0;
uint8_t OSoundInt_created_ym = 0;

void OSoundInt_init()
{
    uint16_t i;
    
    if (!OSOoundInt_pcm_ram)
    {
        OSOoundInt_pcm_ram = (uint8_t*)malloc(OSoundInt_PCM_RAM_SIZE);
    }
    memset(OSOoundInt_pcm_ram, 0, OSoundInt_PCM_RAM_SIZE);

    if (!OSoundInt_created_pcm)
    {
        SegaPCM_Create(SOUND_CLOCK, &Roms_pcm, OSOoundInt_pcm_ram, SEGAPCM_BANK_512);
        OSoundInt_created_pcm = 1;
    }

    if (!OSoundInt_created_ym)
    {
        YM_Create(0.5f, SOUND_CLOCK);
        OSoundInt_created_ym = 1;
    }

    SegaPCM_init(Config_fps);
    YM_init(REAL_AUDIO_FREQUENCY, Config_fps);

    OSoundInt_reset();

    for (i = 0; i < 8; i++)
        OSoundInt_engine_data[i] = 0;

    OSound_init(OSOoundInt_pcm_ram);
}

// Clear sound queue
// Source: 0x5086
void OSoundInt_reset()
{
    sound_counter = 0;
    sound_head    = 0;
    sound_tail    = 0;
    sounds_queued = 0;
}

// ------------------------------------------------------------------------------------------------
//                                MAIN INTERRUPT ROUTINE
// ------------------------------------------------------------------------------------------------

// To be called 3 times per frame (roughly every 5ms)
// to emulate the Z80 interrupt speed.
//
// Source: 0x38
void OSoundInt_tick()
{
    // Wait until OutRun Engine has finished its own initialization
    if (cannonball_state == STATE_BOOT)
        return;

    // Reset Z80 Program counter and other bits. 
    if (OSound_command_input == sound_FM_RESET)
    {
        OSoundInt_reset();
        OSoundInt_has_booted = 0;
    }

    // Standard Tick from main program code
    OSound_tick();
    OSoundInt_play_queued_sound();
    OSound_tick();
    OSoundInt_play_queued_sound();
    OSound_tick();
    OSoundInt_play_queued_sound();
}

// Check for and play a queued sound
// Source: 0x5094
void OSoundInt_play_queued_sound()
{
    if (sounds_queued == 0)
        return;

    OSound_command_input = queue[sound_head];
    sound_head = (sound_head + 1) & QUEUE_LENGTH;
    sounds_queued--;
}

// Queue Sound Service (Used by Z80 only)
// Source: 0x50A2
void OSoundInt_queue_sound_service(uint8_t snd)
{
    if (sounds_queued > QUEUE_LENGTH)
        return;

    queue[sound_tail] = snd;
    sound_tail = (sound_tail + 1) & QUEUE_LENGTH;
    sounds_queued++;
}

// Queue sound effect for playing
// Source: Z80 code (non-standard)
void OSoundInt_queue_sound(uint8_t snd)
{
    OSoundInt_queue_sound_service(snd);
}

// Clear the sound queue
void OSoundInt_queue_clear()
{
    sounds_queued = 0;
    sound_head    = 0;
    sound_tail    = 0;
}
