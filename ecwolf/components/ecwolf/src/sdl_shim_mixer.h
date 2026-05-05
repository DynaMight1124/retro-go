#ifndef SDL_MIXER_SHIM_H
#define SDL_MIXER_SHIM_H

#include "sdl_shim.h"

#ifdef ESP_PLATFORM
#define MIX_CHANNELS 4
#else
#define MIX_CHANNELS 8
#endif
#define MIX_DEFAULT_FORMAT 0
#define MIX_MAXVOLUME 128

typedef struct Mix_Chunk {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;
} Mix_Chunk;

typedef struct _Mix_Music Mix_Music;

#ifdef __cplusplus
extern "C" {
#endif

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
void Mix_CloseAudio(void);
int Mix_AllocateChannels(int numchans);
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
void Mix_HaltChannel(int channel);
int Mix_Playing(int channel);
int Mix_Volume(int channel, int volume);
int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
void Mix_SetPostMix(void (*mix_func)(void *udata, Uint8 *stream, int len), void *arg);
void Mix_ChannelFinished(void (*channel_finished)(int channel));
int Mix_VolumeMusic(int volume);
void Mix_HookMusicFinished(void (*music_finished)(void));

Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc);
void Mix_FreeMusic(Mix_Music *music);
int Mix_PlayMusic(Mix_Music *music, int loops);
void Mix_HaltMusic(void);
int Mix_PlayingMusic(void);
int Mix_PausedMusic(void);
void Mix_ResumeMusic(void);
void Mix_PauseMusic(void);
int Mix_SetMusicPCMPosition(double seconds);
double Mix_GetMusicPCMPosition(void);
const char *Mix_GetError(void);

int Mix_GroupAvailable(int tag);
int Mix_GroupOldest(int tag);
int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels);
int Mix_ReserveChannels(int numchans);
int Mix_GroupChannels(int from, int to, int tag);
int Mix_SetPanning(int channel, Uint8 left, Uint8 right);

#ifdef __cplusplus
}
#endif

#endif
