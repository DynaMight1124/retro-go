#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Initialize control metadata before the mixer starts using this module. */
bool RadioStream_init(void);
/* Run once on the existing idle app_main task; creates no Retro-Go task. */
void RadioStream_run(void);
/* Asynchronous open; seed chooses an offset leaving at least 30 seconds. */
bool RadioStream_open(const char *path, uint32_t seed);
void RadioStream_play(void);
void RadioStream_cancel(void);
/* Drain/close worker I/O before releasing storage or changing game resources. */
void RadioStream_close(void);
bool RadioStream_active(void);
bool RadioStream_failed(void);
/* Add buffered radio to existing interleaved stereo; never waits on SD. */
void RadioStream_mix(int16_t *out, int frames, int rate);
