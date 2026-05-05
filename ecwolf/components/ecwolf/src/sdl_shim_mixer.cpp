#include "sdl_shim_mixer.h"
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <rg_audio.h>
#include <rg_system.h>
#include <esp_heap_caps.h>
}

struct Channel {
    Mix_Chunk *chunk;
    Uint32 pos;
    int loops;
    int volume;
    uint8_t pan_l;
    uint8_t pan_r;
};

static Channel channels[MIX_CHANNELS];
static int channel_groups[MIX_CHANNELS];
static void (*post_mix_func)(void *udata, Uint8 *stream, int len) = NULL;
static void *post_mix_udata = NULL;
static int audio_frequency = 11025;
static int audio_channels = 2;
static int master_volume_music = 128;
static volatile bool audio_terminate = false;
extern "C" rg_task_t *audio_task_handle;
rg_task_t *audio_task_handle = NULL;

extern "C" SDL_mutex *audioMutex;

static void audio_task(void *arg) {
    const int samples = 512; // Increased from 256 for better stability
    const int len = samples * audio_channels * 2; 
    Uint8 *buffer = (Uint8 *)rg_alloc(len, MEM_FAST);
    
    // Submit buffer is twice as large for 22050Hz expansion
    const int submit_samples = samples * 2;
    const int submit_len = submit_samples * audio_channels * 2;
    Uint8 *submit_buffer = (Uint8 *)rg_alloc(submit_len, MEM_FAST);

    RG_LOGI("Audio task started on Core 0: %dHz, %d channels\n", audio_frequency, audio_channels);

    while (!audio_terminate) {
        memset(buffer, 0, len);

        if (audioMutex) SDL_LockMutex(audioMutex);

        // Mix digitized channels
        int16_t *mix_buffer = (int16_t *)buffer;

        for (int i = 0; i < MIX_CHANNELS; i++) {
            if (channels[i].chunk && channels[i].chunk->abuf) {
                Mix_Chunk *chunk = channels[i].chunk;
                int16_t *chunk_data = (int16_t *)chunk->abuf;
                uint32_t chunk_samples = chunk->alen / 2;
                
                int combined_vol = (channels[i].volume * chunk->volume) / 128;
                int pan_l = channels[i].pan_l;
                int pan_r = channels[i].pan_r;
                
                for (int s = 0; s < samples * audio_channels; s += audio_channels) {
                    if (channels[i].pos < chunk_samples) {
                        int16_t sample_val = chunk_data[channels[i].pos++];
                        
                        // Apply volume and panning (divisor 32640 = 128 * 255)
                        int32_t s_l = (int32_t)sample_val * combined_vol * pan_l / 32640;
                        int32_t s_r = (int32_t)sample_val * combined_vol * pan_r / 32640;
                        
                        // Mix into L
                        int32_t mixed_l = mix_buffer[s] + s_l;
                        if (mixed_l > 32767) mixed_l = 32767;
                        else if (mixed_l < -32768) mixed_l = -32768;
                        mix_buffer[s] = (int16_t)mixed_l;
                        
                        // Mix into R
                        if (audio_channels == 2) {
                            int32_t mixed_r = mix_buffer[s+1] + s_r;
                            if (mixed_r > 32767) mixed_r = 32767;
                            else if (mixed_r < -32768) mixed_r = -32768;
                            mix_buffer[s+1] = (int16_t)mixed_r;
                        }
                    } else {
                        if (channels[i].loops > 0 || channels[i].loops == -1) {
                            if (channels[i].loops > 0) channels[i].loops--;
                            channels[i].pos = 0;
                        } else {
                            channels[i].chunk = NULL;
                            break;
                        }
                    }
                }
            }
        }

        // Call post-mix (AdLib/Music)
        if (post_mix_func) {
            post_mix_func(post_mix_udata, buffer, len);
        }

        if (audioMutex) SDL_UnlockMutex(audioMutex);

        // Double-sample 11025Hz to 22050Hz
        int16_t *src = (int16_t *)buffer;
        int16_t *dst = (int16_t *)submit_buffer;
        for (int i = 0; i < samples; i++) {
            dst[i*4 + 0] = src[i*2 + 0];
            dst[i*4 + 1] = src[i*2 + 1];
            dst[i*4 + 2] = src[i*2 + 0];
            dst[i*4 + 3] = src[i*2 + 1];
        }

        rg_audio_submit((rg_audio_sample_t *)submit_buffer, submit_samples);
    }
    
    RG_LOGI("Audio task terminating...\n");
    free(buffer);
    free(submit_buffer);
    audio_task_handle = NULL;
}

extern "C" {

int Mix_OpenAudio(int frequency, Uint16 format, int channels_count, int chunksize) {
    audio_frequency = frequency;
    audio_channels = channels_count;
    memset(channels, 0, sizeof(channels));
    for (int i = 0; i < MIX_CHANNELS; i++) {
        channels[i].volume = 128;
        channels[i].pan_l = 255;
        channels[i].pan_r = 255;
    }
    audio_terminate = false;
    audio_task_handle = rg_task_create("ecwolf_audio", audio_task, NULL, 16384, 1, RG_TASK_PRIORITY_2, 0);
    return 0;
}

void Mix_CloseAudio(void) {
    if (audio_task_handle) {
        audio_terminate = true;
        // Wait for the task to signal exit by clearing its handle
        int timeout = 100;
        while (audio_task_handle && --timeout > 0) {
            rg_task_delay(10);
        }
        audio_task_handle = NULL;
    }
}
int Mix_AllocateChannels(int numchans) { return MIX_CHANNELS; }

Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc) {
    Mix_Chunk *chunk = (Mix_Chunk *)heap_caps_malloc(sizeof(Mix_Chunk), MALLOC_CAP_SPIRAM);
    if (!chunk) {
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }
    chunk->allocated = 1;
    chunk->abuf = NULL;

    Uint32 raw_len = SDL_RWsize(src);
    if (raw_len == 0) {
        free(chunk);
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }
    SDL_RWseek(src, 0, RW_SEEK_SET);

    Uint8 *raw_buf = (Uint8 *)heap_caps_malloc(raw_len, MALLOC_CAP_SPIRAM);
    if (!raw_buf) {
        free(chunk);
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }
    SDL_RWread(src, raw_buf, raw_len, 1);
    
    if (raw_len > 44 && memcmp(raw_buf, "RIFF", 4) == 0 && memcmp(raw_buf + 8, "WAVE", 4) == 0) {
        Uint32 data_size = raw_len - 44;
        chunk->alen = data_size & ~1; // Ensure 16-bit alignment
        chunk->abuf = (Uint8 *)heap_caps_malloc(chunk->alen, MALLOC_CAP_SPIRAM);
        if (chunk->abuf) memcpy(chunk->abuf, raw_buf + 44, chunk->alen);
    } else {
        chunk->alen = raw_len * 2;
        chunk->abuf = (Uint8 *)heap_caps_malloc(chunk->alen, MALLOC_CAP_SPIRAM);
        if (chunk->abuf) {
            int16_t *dest = (int16_t *)chunk->abuf;
            for (Uint32 i = 0; i < raw_len; i++) {
                dest[i] = (raw_buf[i] - 128) * 256;
            }
        }
    }

    free(raw_buf);

    if (!chunk->abuf) {
        free(chunk);
        if (freesrc) SDL_RWclose(src);
        return NULL;
    }
    
    chunk->allocated = 1;
    chunk->volume = 128;
    if (freesrc) SDL_RWclose(src);
    return chunk;
}

void Mix_FreeChunk(Mix_Chunk *chunk) {
    if (chunk && chunk->allocated) {
        if (audioMutex) SDL_LockMutex(audioMutex);
        for (int i = 0; i < MIX_CHANNELS; i++) {
            if (channels[i].chunk == chunk) channels[i].chunk = NULL;
        }
        if (chunk->abuf) free(chunk->abuf);
        chunk->allocated = 0;
        free(chunk);
        if (audioMutex) SDL_UnlockMutex(audioMutex);
    }
}

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops) {
    if (audioMutex) SDL_LockMutex(audioMutex);
    if (channel == -1) {
        for (int i = 0; i < MIX_CHANNELS; i++) {
            if (!channels[i].chunk) {
                channel = i;
                break;
            }
        }
    }
    if (channel >= 0 && channel < MIX_CHANNELS) {
        channels[channel].chunk = chunk;
        channels[channel].pos = 0;
        channels[channel].loops = loops;
        if (audioMutex) SDL_UnlockMutex(audioMutex);
        return channel;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return -1;
}

void Mix_HaltChannel(int channel) {
    if (audioMutex) SDL_LockMutex(audioMutex);
    if (channel == -1) {
        for (int i = 0; i < MIX_CHANNELS; i++) channels[i].chunk = NULL;
    } else if (channel >= 0 && channel < MIX_CHANNELS) {
        channels[channel].chunk = NULL;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
}

int Mix_Playing(int channel) {
    int playing = 0;
    if (audioMutex) SDL_LockMutex(audioMutex);
    if (channel == -1) {
        for (int i = 0; i < MIX_CHANNELS; i++) {
            if (channels[i].chunk) { playing = 1; break; }
        }
    } else if (channel >= 0 && channel < MIX_CHANNELS) {
        playing = channels[channel].chunk != NULL;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return playing;
}

int Mix_Volume(int channel, int volume) {
    if (audioMutex) SDL_LockMutex(audioMutex);
    int prev = 128;
    if (channel == -1) {
        for (int i = 0; i < MIX_CHANNELS; i++) channels[i].volume = volume;
    } else if (channel >= 0 && channel < MIX_CHANNELS) {
        prev = channels[channel].volume;
        channels[channel].volume = volume;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return prev;
}

int Mix_VolumeChunk(Mix_Chunk *chunk, int volume) {
    if (chunk) {
        int prev = chunk->volume;
        chunk->volume = volume;
        return prev;
    }
    return 128;
}

void Mix_SetPostMix(void (*mix_func)(void *udata, Uint8 *stream, int len), void *arg) {
    post_mix_func = mix_func;
    post_mix_udata = arg;
}

void Mix_ChannelFinished(void (*channel_finished)(int channel)) {}

int Mix_VolumeMusic(int volume) {
    int prev = master_volume_music;
    master_volume_music = volume;
    return prev;
}

void Mix_HookMusicFinished(void (*music_finished)(void)) {}
Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc) { return NULL; }
void Mix_FreeMusic(Mix_Music *music) {}
int Mix_PlayMusic(Mix_Music *music, int loops) { return 0; }
void Mix_HaltMusic(void) {}
int Mix_PlayingMusic(void) { return 0; }
int Mix_PausedMusic(void) { return 0; }
void Mix_ResumeMusic(void) {}
void Mix_PauseMusic(void) {}
int Mix_SetMusicPCMPosition(double seconds) { return 0; }
double Mix_GetMusicPCMPosition(void) { return 0; }
const char *Mix_GetError(void) { return "Mixer error"; }

int Mix_GroupAvailable(int tag) {
    int channel = -1;
    if (audioMutex) SDL_LockMutex(audioMutex);
    for (int i = 0; i < MIX_CHANNELS; i++) {
        if (channel_groups[i] == tag && !channels[i].chunk) {
            channel = i;
            break;
        }
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return channel;
}

int Mix_GroupOldest(int tag) {
    int channel = -1;
    if (audioMutex) SDL_LockMutex(audioMutex);
    for (int i = 0; i < MIX_CHANNELS; i++) {
        if (channel_groups[i] == tag) {
            channel = i;
            break;
        }
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return channel;
}

int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels_out) {
    if (frequency) *frequency = audio_frequency;
    if (format) *format = AUDIO_S16;
    if (channels_out) *channels_out = audio_channels;
    return 1;
}

int Mix_ReserveChannels(int numchans) { return numchans; }

int Mix_GroupChannels(int from, int to, int tag) {
    if (audioMutex) SDL_LockMutex(audioMutex);
    for (int i = from; i <= to && i < MIX_CHANNELS; i++) {
        channel_groups[i] = tag;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return 0;
}

int Mix_SetPanning(int channel, Uint8 left, Uint8 right) {
    if (audioMutex) SDL_LockMutex(audioMutex);
    if (channel >= 0 && channel < MIX_CHANNELS) {
        channels[channel].pan_l = left;
        channels[channel].pan_r = right;
    }
    if (audioMutex) SDL_UnlockMutex(audioMutex);
    return 1;
}

}
