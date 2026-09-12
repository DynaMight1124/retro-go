/***************************************************************************
    SDL Audio Code.
    
    This is the SDL specific audio code.
    If porting to a non-SDL platform, you would need to replace this class.
    
    It takes the output from the PCM and YM chips, mixes them and then
    outputs appropriately.
    
    In order to achieve seamless audio, when audio is enabled the framerate
    is adjusted to essentially sync the video to the audio output.
    
    This is based upon code from the Atari800 emulator project.
    Copyright (c) 1998-2008 Atari800 development team
***************************************************************************/

#pragma once
#include <stdint.h>
#include "globals.h"

#ifdef COMPILE_SOUND_CODE

struct wav_t {
    uint8_t loaded;
    int16_t *data;
    uint32_t pos;
    uint32_t length;
};

extern uint8_t Audio_sound_enabled;
extern uint16_t* Audio_mix_buffer;

void Audio_init();
void Audio_tick();
void Audio_wait();
void Audio_start_audio();
void Audio_stop_audio();
double Audio_adjust_speed();
void Audio_load_wav(const char* filename);
void Audio_clear_wav();

#endif
