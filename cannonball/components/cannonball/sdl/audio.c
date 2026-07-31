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
const uint32_t AUDIO_FREQUENCY = SYNTH_AUDIO_FREQUENCY;

#ifdef COMPILE_SOUND_CODE

uint8_t Audio_sound_enabled;
const uint32_t CHANNELS = 2;
const uint32_t SAMPLES  = 1024;

uint16_t* Audio_mix_buffer;
struct wav_t Audio_wavfile;

rg_task_t *audio_task_handle = NULL;
static volatile bool audio_pending;

static inline int16_t clamp_audio_sample(int32_t sample)
{
    if (sample > INT16_MAX) return INT16_MAX;
    if (sample < INT16_MIN) return INT16_MIN;
    return (int16_t)sample;
}

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
        if (enable_ym2151_synth)
            YM_stream_update();

        int16_t *pcm_buffer = SegaPCM_get_buffer();
        int16_t *ym_buffer  = YM_get_buffer();
        int16_t *wav_buffer = Audio_wavfile.data;

        const int source_frames = AUDIO_FREQUENCY / Config_fps;
        const int output_frames = REAL_AUDIO_FREQUENCY / Config_fps;
        if (!Audio_wavfile.loaded)
        {
            // 11040 -> 22050 Hz uses almost every synthesized frame twice.
            // Mix and clamp once per source frame, then reuse that exact stereo
            // pair for its repeated output frame.
            const bool ym_enabled = enable_ym2151_synth;
            int source_frame = 0;
            int cached_source_frame = -1;
            int resample_accum = 0;
            int16_t mixed_left = 0;
            int16_t mixed_right = 0;

            for (int frame = 0; frame < output_frames; frame++)
            {
                if (source_frame != cached_source_frame)
                {
                    const int source = source_frame * CHANNELS;
                    int32_t left = pcm_buffer[source];
                    int32_t right = pcm_buffer[source + 1];

                    if (ym_enabled)
                    {
                        left += ym_buffer[source];
                        right += ym_buffer[source + 1];
                    }

                    mixed_left = clamp_audio_sample(left);
                    mixed_right = clamp_audio_sample(right);
                    cached_source_frame = source_frame;
                }

                const int output = frame * CHANNELS;
                Audio_mix_buffer[output] = mixed_left;
                Audio_mix_buffer[output + 1] = mixed_right;

                resample_accum += source_frames;
                if (resample_accum >= output_frames)
                {
                    resample_accum -= output_frames;
                    source_frame++;
                }
            }
        }
        else
        {
            // Retain the original per-channel WAV cursor behavior for builds
            // that replace the Retro-Go dummy WAV loader.
            int source_frame = 0;
            int resample_accum = 0;

            for (int frame = 0; frame < output_frames; frame++)
            {
                const int source = source_frame * CHANNELS;
                const int output = frame * CHANNELS;

                for (int channel = 0; channel < CHANNELS; channel++)
                {
                    int32_t mix_data = pcm_buffer[source + channel];
                    if (enable_ym2151_synth)
                        mix_data += ym_buffer[source + channel];

                    mix_data += wav_buffer[Audio_wavfile.pos];
                    if (++Audio_wavfile.pos >= Audio_wavfile.length)
                        Audio_wavfile.pos = 0;

                    Audio_mix_buffer[output + channel] = clamp_audio_sample(mix_data);
                }

                resample_accum += source_frames;
                if (resample_accum >= output_frames)
                {
                    resample_accum -= output_frames;
                    source_frame++;
                }
            }
        }

        __atomic_store_n(&audio_pending, false, __ATOMIC_RELEASE);
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
        uint16_t buffer_size = (REAL_AUDIO_FREQUENCY / 30) * CHANNELS;
        Audio_mix_buffer = (uint16_t*)rg_alloc(buffer_size * sizeof(uint16_t), MEM_FAST);
        memset(Audio_mix_buffer, 0, buffer_size * sizeof(uint16_t));
        
        // Core 1 also runs the parallel road/sprite worker. Give that worker
        // priority over synthesis so audio fills the main-core-only gaps between
        // render stages instead of delaying the rendering critical path. The
        // main loop still calls Audio_wait() before submitting this buffer.
        audio_task_handle = rg_task_create("audio_task", &audio_task, NULL, 4096, 1, RG_TASK_PRIORITY_5, 1);
        
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

    // Trigger synthesis on Core 1 while the main core renders this frame.
    __atomic_store_n(&audio_pending, true, __ATOMIC_RELEASE);
    rg_task_msg_t msg = {1, .dataInt = 0};
    if (!rg_task_send(audio_task_handle, &msg, 0))
        __atomic_store_n(&audio_pending, false, __ATOMIC_RELEASE);
}

void Audio_wait()
{
    while (__atomic_load_n(&audio_pending, __ATOMIC_ACQUIRE))
        rg_task_yield();
}

void Audio_clear_buffers()
{
}

#endif
