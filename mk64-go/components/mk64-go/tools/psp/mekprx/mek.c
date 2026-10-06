/* mk64k.prx: a kernel-mode helper that boots the PSP's Media Engine for the
 * EBOOT (src/port/psp/me.c).  The recipe, the stub and the dispatch loop are
 * the ones the 007 Portable port runs on hardware (its tools/psp/crashprx);
 * this is that module without the crash handler.  A user module cannot drive
 * the ME's reset lines; this one does it at module start and again after a
 * resume. */
#include <pspkernel.h>
#include <pspsysreg.h>
#include <string.h>
#include "me_share.h"

PSP_MODULE_INFO("mk64k", 0x1000, 1, 0);

/* ---- Media Engine ------------------------------------------------------ */
/* The PSP's second Allegrex core.  me_boot copies me_stub.S to the ME's
 * reset entry and releases the core into me_loop, which polls the MeShare
 * line the EBOOT owns (me_share.h) and calls whatever user-memory function
 * it names -- the pspsdk samples/me recipe (mrbrown/crazyc's melib).  The
 * ME has no OS: me_loop and the functions it calls must not touch the
 * kernel.  ME data-cache handling per function: write back + invalidate
 * before (see the CPU's writes) and after (the results reach RAM). */
extern unsigned char me_stub[], me_stub_end[];

static void me_dcache_wbinv_all(void) {
    int i;
    for (i = 0; i < 8192; i += 64) {
        __builtin_allegrex_cache(0x14, i);
        __builtin_allegrex_cache(0x14, i);
    }
}

/* Runs on the ME, forever. */
static void me_loop(volatile struct MeShare* m) __attribute__((noinline, noreturn));
static void me_loop(volatile struct MeShare* m) {
    u32 idle = 0;
    {
        /* The ME's own FPU: disable its exception traps too (FCSR enable bits
         * 11..7) so a divide by zero in a function it runs returns inf like the
         * N64 instead of faulting the core. */
        u32 fcsr;
        __asm__ volatile ("cfc1 %0, $31" : "=r"(fcsr));
        fcsr &= ~0x00000F80u;
        __asm__ volatile ("ctc1 %0, $31" : : "r"(fcsr));
    }
    while (m->init) {
        if (!m->start) {
            int i;
            for (i = 0; i < 64; i++) {
                __asm__ volatile ("nop");
            }
            if ((++idle & 1023u) == 0) {
                m->spins = m->spins + 1;
            }
            continue;
        }
        m->start = 0;
        me_dcache_wbinv_all();
        m->result = ((int (*)(int)) m->func)((int) m->param);
        me_dcache_wbinv_all();
        __asm__ volatile ("sync");
        m->alive = m->alive + 1;
        m->done = 1;
    }
    for (;;) {
    }
}

/* Main CPU: the whole 16 KB data cache, index by index (no ForUser stub). */
static void cpu_dcache_wbinv_all(void) {
    int i;
    for (i = 0; i < 16384; i += 64) {
        __builtin_allegrex_cache(0x14, i);
        __builtin_allegrex_cache(0x14, i);
    }
}

/* The ME control line the EBOOT passed at module_start; the watch thread
 * re-boots the ME from it after a resume. */
static volatile struct MeShare* sMe;

static void me_boot(volatile struct MeShare* m) {
    volatile u32* dst = (volatile u32*) 0xbfc00040u;
    const u32* src = (const u32*) me_stub;
    int n = (int) ((me_stub_end - me_stub + 3) / 4);
    int i;
    m->start = 0;
    m->done = 1;
    m->result = 0;
    m->init = 1;
    m->alive = 0;
    m->spins = 0;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
    *(volatile u32*) 0xbfc00600u = (u32) me_loop; /* entry */
    *(volatile u32*) 0xbfc00604u = (u32) m;       /* argument (the uncached alias) */
    cpu_dcache_wbinv_all();
    __asm__ volatile ("sync");
    sceSysregMeResetEnable();
    sceSysregMeBusClockEnable();
    sceSysregMeResetDisable();
    sceSysregVmeResetDisable();
    m->boot_status = 1;
}

/* A resume resets the ME core (RAM is preserved, so me_loop's code and the
 * shared line survive, but the core is stopped).  The EBOOT bumps reboot_req
 * from user mode; only kernel code can drive the sysreg reset, so this
 * low-priority kernel thread does it and acknowledges through reboot_done. */
static int me_watch_thread(SceSize args, void* argp) {
    (void) args; (void) argp;
    for (;;) {
        sceKernelDelayThread(100 * 1000);
        if (sMe != NULL && sMe->reboot_req != sMe->reboot_done) {
            unsigned int req = sMe->reboot_req;
            me_boot(sMe);
            sMe->reboot_done = req;
        }
    }
    return 0;
}

/* module_start is the entry (ENTRY(module_start) in linkfile.prx).  No
 * argument plumbing (neither module arguments nor module_start's return
 * survive ARK-4): the EBOOT lays a magic signature directly before the pointer
 * to its Media Engine control line, and this scans the EBOOT image for it. */
int module_start(SceSize args, void* argp) {
    const u32 MAGIC0 = 0x4B34364Du, MAGIC1 = 0x45524853u, MAGIC2 = 0x454D5F5Fu, MAGIC3 = 0x21432149u;
    const u32* p = (const u32*) 0x08804000u;
    const u32* end = (const u32*) 0x09F00000u;
    volatile struct MeShare* me = NULL;
    (void) args; (void) argp;
    for (; p < end; p += 4) { /* the EBOOT aligns the block to 16 */
        if (p[0] == MAGIC0 && p[1] == MAGIC1 && p[2] == MAGIC2 && p[3] == MAGIC3) {
            me = (volatile struct MeShare*) p[4];
            break;
        }
    }
    if (me != NULL) {
        int th;
        sMe = me;
        me_boot(me);
        th = sceKernelCreateThread("me_watch", me_watch_thread, 0x30, 0x1000, 0, NULL);
        if (th >= 0) {
            sceKernelStartThread(th, 0, NULL);
        }
    }
    return 0;
}

int module_stop(SceSize args, void* argp) {
    (void) args; (void) argp;
    return 0;
}
