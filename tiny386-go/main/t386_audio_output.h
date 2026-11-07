#pragma once
#include <stddef.h>
#include "rg_audio.h"
/* Signed PCM only. The shared driver owns DAC bias and hardware mute. */
void t386_audio_output_init(void);
void t386_audio_output_prime(void);
void t386_audio_output_pause(bool paused);
bool t386_audio_output_is_paused(void);
void t386_audio_output_prepare(rg_audio_frame_t *frames,size_t count);
