#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

void I_CAMD_ShutdownMusic(void);

uint8_t I_CAMD_InitMusic(void);

void I_CAMD_SetMusicVolume(int volume);

void I_CAMD_PlaySong(char *filename);

void I_CAMD_PauseSong(void);

void I_CAMD_ResumeSong(void);

void I_CAMD_StopSong(void);

#endif
