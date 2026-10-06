/**
 * The mixer as a coprocessor (branch max_fps_experiments).
 *
 * On the N64 the game's synthesis.c writes an audio command list and the RSP
 * executes it.  The port's mixer.h turned every command into an immediate C
 * call; here they are recorded into a job again, and an executor plays the
 * RSP: the main CPU (at submit, the default and the fallback) or the PSP's
 * Media Engine (src/port/psp/me_audio.c), which runs the job while the main
 * CPU carries on with the game.  The game already pushes each mixed buffer two
 * audio tasks after it was mixed (port_eu.c), so a job has that long to finish
 * and the ME adds no latency.
 */
#ifndef MIX_JOBS_H
#define MIX_JOBS_H

#include <stdint.h>

#define MIXJ_JOBS 4          /* ring; at most 3 are ever in flight */
#define MIXJ_WORDS 16384     /* command words per job (64 KB) */

/* Both structs fill whole 64-byte cache lines: a line one processor writes
 * back must never hold the other's data. */
typedef struct {
    uint32_t nwords;
    uint32_t words[MIXJ_WORDS];
    uint32_t line_pad[15];
} MixJob;

/* Shared between the two processors: accessed uncached on both sides. */
typedef struct {
    volatile uint32_t submitted; /* jobs handed over (main CPU writes) */
    volatile uint32_t completed; /* jobs finished (the executor writes) */
    volatile uint32_t alive;     /* the ME's handshake */
    volatile uint32_t stop;
    uint32_t line_pad[12];
} MixShared;

extern MixJob gMixJobs[MIXJ_JOBS];
extern MixShared gMixShared;

/* Run one job.  ptr_mask is OR-ed into every memory argument (0: both
 * processors run jobs through their caches; the ME flushes and invalidates its
 * own around each job, psp/me_audio.c). */
void mix_job_execute(const MixJob* job, uint32_t ptr_mask);

/* port_eu.c hooks */
void port_mix_begin(int ai_buffer);  /* before synthesis: start recording this task's job */
void port_mix_submit(void);          /* after synthesis: hand it to the executor */
void* port_mix_wait(int ai_buffer, void* buffer); /* before the buffer is played: its job is done; returns where to read it */
void port_mix_drain(void);           /* before an audio reset frees what jobs point at */

/* ME backend (me_audio.c); stubs when not built in */
int port_me_start(void);             /* 1 = the ME is executing jobs */
void port_me_kick(void);             /* start an ME task if jobs are pending and it is idle */
void port_me_stop(void);
#endif
