/***************************************************************************
    SDL Audio Code.
    
    This is the SDL specific audio code.
    If porting to a non-SDL platform, you would need to replace this class.
    
    It takes the output from the PCM and YM chips, mixes them and then
    outputs appropriately.
***************************************************************************/
#include <SDL/SDL.h>
#include <stdint.h>
#include <string.h>
#include "sdl/audio.h"
#include "hwaudio/segapcm.h"
#include "hwaudio/ym2151.h"
#include "frontend/config.h" 
#include "engine/audio/osoundint.h"

// Sample Rate
const uint32_t AUDIO_FREQUENCY = REAL_AUDIO_FREQUENCY;

#ifdef COMPILE_SOUND_CODE

uint8_t Audio_sound_enabled;
const uint32_t CHANNELS = 2;
const uint32_t SAMPLES  = 1024;

uint16_t* Audio_mix_buffer;
struct wav_t Audio_wavfile;

rg_task_t *audio_task_handle = NULL;

// Required Dummy functions for Retro-Go linking
void Audio_load_wav(const char* filename) { Audio_wavfile.loaded = 0; }
void Audio_clear_wav() { Audio_wavfile.loaded = 0; }

static void audio_task(void *arg)
{
    rg_task_msg_t msg;
    while (rg_task_receive(&msg, -1))
    {
        if (msg.type == RG_TASK_MSG_STOP) break;

        // Perform synthesis on Core 1
        SegaPCM_stream_update();
        YM_stream_update();

        int16_t *pcm_buffer = SegaPCM_get_buffer();
        int16_t *ym_buffer  = YM_get_buffer();
        int16_t *wav_buffer = Audio_wavfile.data;

        int samples_written = (AUDIO_FREQUENCY / Config_fps) * CHANNELS;

        for (int i = 0; i < samples_written; i++)
        {
            int32_t mix_data = pcm_buffer[i];
            
            if (enable_ym2151_synth)
                mix_data += ym_buffer[i];
            
            if (Audio_wavfile.loaded)
            {
                mix_data += wav_buffer[Audio_wavfile.pos];
                if (++Audio_wavfile.pos >= Audio_wavfile.length)
                    Audio_wavfile.pos = 0;
            }

            if (mix_data >= (1 << 15)) mix_data = (1 << 15) - 1;
            else if (mix_data < -(1 << 15)) mix_data = -(1 << 15);

            Audio_mix_buffer[i] = (int16_t)mix_data;
        }
    }
}

void Audio_init()
{
    if (Config_sound.enabled)
        Audio_start_audio();
}

void Audio_start_audio()
{
    if (!Audio_sound_enabled)
    {
        uint16_t buffer_size = (AUDIO_FREQUENCY / 30) * CHANNELS; 
        Audio_mix_buffer = (uint16_t*)rg_alloc(buffer_size * sizeof(uint16_t), MEM_SLOW);
        memset(Audio_mix_buffer, 0, buffer_size * sizeof(uint16_t));
        
        // Create the background audio task on Core 1
        audio_task_handle = rg_task_create("audio_task", &audio_task, NULL, 4096, 1, RG_TASK_PRIORITY_6, 1);
        
        Audio_sound_enabled = 1;
    }
}

void Audio_stop_audio()
{
    if (Audio_sound_enabled)
    {
        Audio_sound_enabled = 0;
        if (audio_task_handle)
        {
            rg_task_msg_t msg = {RG_TASK_MSG_STOP, .dataInt = 0};
            rg_task_send(audio_task_handle, &msg, 100);
            audio_task_handle = NULL;
        }
    }
}

void Audio_tick()
{
    if (!Audio_sound_enabled || !audio_task_handle) return;

    // Trigger synthesis on Core 1 for the NEXT frame
    rg_task_msg_t msg = {1, .dataInt = 0};
    rg_task_send(audio_task_handle, &msg, 0);
}

void Audio_clear_buffers()
{
}

#endif
