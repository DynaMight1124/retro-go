#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct {uint64_t samples; unsigned frame_samples,frame_remainder; uint64_t epoch;} T386Schedule;
unsigned t386_schedule_chunk(const T386Schedule *s);
bool t386_schedule_advance(T386Schedule *s,unsigned count);
uint64_t t386_schedule_deadline(const T386Schedule *s);
uint64_t t386_schedule_target(T386Schedule *s,uint64_t now);
unsigned t386_cpu_budget(uint64_t now,uint64_t deadline,unsigned estimated_512_us);
unsigned t386_cpu_observe(unsigned estimate,uint64_t elapsed_us,unsigned budget);
typedef struct {uint64_t next_us,cost_us;unsigned limit_fps;} T386Video;
/* 0 retains measured Auto; supported optional ceilings are 20,15,10 fps. */
void t386_video_set_limit(T386Video *v,unsigned fps);
bool t386_video_due(const T386Video *v,uint64_t now,unsigned audio_late_us,bool busy);
void t386_video_record(T386Video *v,uint64_t started_us,uint64_t ended_us);
bool t386_present_copy(const void *work,void *present,size_t size,bool busy);
bool t386_present_copy_rows(const void *source,size_t source_stride,void *dest,
                           size_t dest_stride,size_t row_bytes,size_t rows,bool busy);
/* Snapshot both indices and256RGB565 colours only when the output is free.
 * Render and submission palettes must be separate. No writes on rejection. */
bool t386_present_copy_indexed(const void *source,size_t source_stride,void *dest,
                              size_t dest_stride,size_t row_bytes,size_t rows,
                              const uint16_t *palette,uint16_t *dest_palette,bool busy);
/* Adapter-local diagnostic window; bytes count successful copies only. */
typedef struct {uint64_t us,bytes; unsigned calls,rejected,max_us;} T386PresentCopyProfile;
void t386_present_copy_profile_reset(void);
const T386PresentCopyProfile *t386_present_copy_profile_get(void);

/* Retro-Go reference frameskip: only presentation is suppressed. */
typedef struct {unsigned skip;} T386SharedVideo;
bool t386_shared_video_due(const T386SharedVideo *v,bool busy);
void t386_shared_video_record(T386SharedVideo *v,int frameskip,uint64_t busy_us,
                             unsigned frame_us,bool display_busy);
