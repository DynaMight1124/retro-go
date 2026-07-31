#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize audio system */
void audio_init(int sample_rate);

/* Stop all audio and cleanup (safe to call from main) */
void audio_deinit(void);

/* Quick shutdown (safe to call from event handler) */
void audio_shutdown(void);

/* Play a sound effect by ID */
void audio_sfx_play(int id);

/* Play music by index. -1 to stop. */
void audio_music_play(int index, int fade_ms);

/* Stop all audio */
void audio_stop_all(void);

#endif
