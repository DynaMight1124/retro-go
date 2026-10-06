/**
 * The mixer on the PSP's Media Engine (port/audio/mix_jobs.h), over the
 * transport in me.c.  One ME task runs every job that is pending and picks up
 * the ones submitted while it runs.  Built with -DPORT_ME_AUDIO.
 */
#include <stdint.h>
#include "../audio/mix_jobs.h"
#include "../port.h"

#ifdef PORT_ME_AUDIO
extern int port_me_available(void);
extern int port_me_done(void);
extern int port_me_begin(int (*func)(int), int param);
extern void port_me_disable(const char* why);
extern void port_me_park(void);

/* ME side: its whole 8 KB data cache, written back and invalidated.  The
 * dispatch loop does this around the task; a job submitted while the task runs
 * needs it again -- the ME may still hold the ring slot's previous contents.
 * (A privileged instruction: never call this on the main CPU.) */
static void me_dcache_wbinv_all(void) {
    int i;
    for (i = 0; i < 8192; i += 64) {
        __builtin_allegrex_cache(0x14, i);
        __builtin_allegrex_cache(0x14, i);
    }
}

/* Runs on the ME.  No system call, no logging. */
static int me_mix_run(int unused) {
    MixShared* sh = (MixShared*) ((uintptr_t) &gMixShared | 0x40000000u);
    int n = 0;
    (void) unused;
    for (;;) {
        uint32_t done = sh->completed;
        if (done == sh->submitted) break;
        me_dcache_wbinv_all();
        mix_job_execute(&gMixJobs[done % MIXJ_JOBS], 0);
        me_dcache_wbinv_all();
        sh->completed = done + 1;
        n++;
    }
    return n;
}

int port_me_start(void) { return port_me_available(); }

/* Main CPU: start a task if jobs are pending and the ME is idle. */
void port_me_kick(void) {
    MixShared* sh = (MixShared*) ((uintptr_t) &gMixShared | 0x40000000u);
    if (port_me_available() && port_me_done() && sh->completed != sh->submitted) {
        port_me_begin(me_mix_run, 0);
    }
}

void port_me_stop(void) {
    port_me_disable("stopped");
    port_me_park();
}
#else
int port_me_start(void) { return 0; }
void port_me_kick(void) {}
void port_me_stop(void) {}
#endif
