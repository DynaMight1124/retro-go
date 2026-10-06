/* Media Engine transport (main-CPU side), -DPORT_ME_AUDIO builds.
 *
 * mk64k.prx (tools/psp/mekprx) boots the PSP's second Allegrex core at module
 * start with a dispatch loop that spins on the MeShare line (me_share.h) and
 * calls the user-memory function the EBOOT asks for.  This transport, the PRX
 * and the cache discipline are the 007 Portable port's, which runs on
 * hardware.  The ME has the same instruction set minus the VFPU and reaches
 * user memory directly; it has no OS, so what it runs must be pure computation
 * (no syscalls, no logging).  The mixer job executor is such a function.
 *
 * Cache discipline: the ME loop writes back and invalidates its own data
 * cache before and after every function, so it sees what the CPU wrote back
 * and its results reach RAM.  The CPU writes back its data cache before each
 * start and reads what the ME wrote through the uncached alias.
 *
 * The emulator never loads the PRX (PPSSPP does not emulate the ME), so the
 * probe fails there and everything stays on the CPU. */
#ifdef PORT_ME_AUDIO
#include <ultra64.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>
#include "../port.h"
#include "me_share.h"

extern u32 port_time_us(void);

static struct MeShare sMeShareMem __attribute__((aligned(64)));
static volatile struct MeShare* sMe; /* the uncached alias; the only pointer ever used */
static int sMeAvailable;

/* mk64k.prx finds this by scanning the EBOOT image for the magic. */
static struct { u32 magic[4]; volatile struct MeShare* me; u32 pad[3]; } sMeShareBlock __attribute__((aligned(16), used)) = {
    { 0x4B34364Du, 0x45524853u, 0x454D5F5Fu, 0x21432149u }, NULL, { 0, 0, 0 }
};

/* Runs on the ME: proves user-memory code executes there and returns. */
static int me_ping(int x) {
    return x ^ (int) ME_PING_MAGIC;
}

int port_me_available(void) { return sMeAvailable; }
int port_me_done(void) { return sMe != NULL && sMe->done; }

/* Park the ME before the app exits: its loop polls the share line, which
 * lives in this app's memory and is freed on exit. */
void port_me_park(void) {
    if (sMe != NULL) {
        sMe->init = 0;
        __asm__ volatile ("sync");
    }
}

int port_me_begin(int (*func)(int), int param) {
    if (!sMeAvailable) return -1;
    if (!sMe->done) return -2; /* one function at a time */
    sMe->done = 0;
    sMe->result = 0;
    sMe->func = (u32) func;
    sMe->param = (u32) param;
    sceKernelDcacheWritebackAll(); /* everything the CPU wrote reaches RAM before the ME reads it */
    __asm__ volatile ("sync");
    sMe->start = 1;
    return 0;
}

/* 1 when done, 0 on timeout. */
int port_me_wait(u32 timeout_us) {
    u32 t0 = port_time_us();
    int polls = 0;
    while (!sMe->done) {
        if (port_time_us() - t0 > timeout_us) return 0;
        if (++polls > 200) sceKernelDelayThread(50);
    }
    return 1;
}

void port_me_disable(const char* why) {
    if (sMeAvailable) {
        PORT_LOG("me: disabled: %s (alive %u, spins %u)\n", why, sMe->alive, sMe->spins);
    }
    sMeAvailable = 0;
}

static void me_probe(void) {
    u32 t0;
    if (sMe->boot_status != 1) {
        PORT_LOG("me: not booted (status %d): mixing on the CPU\n", sMe->boot_status);
        return;
    }
    sMeAvailable = 1;
    t0 = port_time_us();
    if (port_me_begin(me_ping, 0x1234) != 0 || !port_me_wait(200 * 1000) || sMe->result != (0x1234 ^ (int) ME_PING_MAGIC)) {
        PORT_LOG("me: booted but no answer to the ping (done %u, result %08X, alive %u, spins %u): mixing on the CPU\n",
                 sMe->done, (unsigned) sMe->result, sMe->alive, sMe->spins);
        sMeAvailable = 0;
        return;
    }
    PORT_LOG("me: online (ping %u us, idle spins %u)\n", (unsigned) (port_time_us() - t0), sMe->spins);
}

/* Boot: load mk64k.prx from next to the EBOOT; it boots the ME. */
void port_me_load(void) {
    char path[256], arg[16];
    SceUID mod;
    int n, status = 0, r;
    /* Drop any cached copy of the line: from here on nothing touches it
     * cached, so a later writeback cannot clobber the uncached stores. */
    sceKernelDcacheWritebackInvalidateAll();
    sMe = (volatile struct MeShare*) ((u32) &sMeShareMem | 0x40000000u);
    memset((void*) sMe, 0, sizeof(*sMe));
    sMe->done = 1;
    sMe->init = 1;
    sMeShareBlock.me = sMe;
    sceKernelDcacheWritebackAll(); /* the PRX scans RAM for the block */
    /* Under an emulator sceKernelStartModule of a kernel PRX never returns. */
    if (sceIoDevctl("kemulator:", 0x00000003 /* IS_EMULATOR */, NULL, 0, NULL, 0) >= 0) {
        PORT_LOG("me: emulator detected, mk64k.prx not loaded\n");
        return;
    }
    /* The helper rides inside the EBOOT (tools/psp/embed_prx.py) and is written
     * to data/ next to the asset cache; only a kernel module loaded from a file
     * can boot the ME from user mode.  Rewritten when the bytes differ (a new
     * build), left alone otherwise. */
    snprintf(path, sizeof(path), "%smk64k.prx", port_save_dir());
    {
        extern const unsigned char gPortMeKprx[];
        extern const unsigned int gPortMeKprxSize;
        static unsigned char have[4096];
        FILE* f = fopen(path, "rb");
        int same = 0;
        if (f != NULL) {
            size_t got = fread(have, 1, sizeof(have), f);
            fclose(f);
            same = got == gPortMeKprxSize && gPortMeKprxSize <= sizeof(have) && memcmp(have, gPortMeKprx, gPortMeKprxSize) == 0;
        }
        if (!same) {
            f = fopen(path, "wb");
            if (f == NULL || fwrite(gPortMeKprx, 1, gPortMeKprxSize, f) != gPortMeKprxSize || fclose(f) != 0) {
                if (f != NULL) fclose(f);
                remove(path);
                PORT_LOG("me: could not write %s: mixing on the CPU\n", path);
                return;
            }
            PORT_LOG("me: wrote %s (%u bytes)\n", path, gPortMeKprxSize);
        }
    }
    mod = sceKernelLoadModule(path, 0, NULL);
    if (mod < 0) {
        PORT_LOG("me: %s not loaded (%08X): mixing on the CPU\n", path, (unsigned) mod);
        return;
    }
    n = snprintf(arg, sizeof(arg), "mk64k") + 1;
    r = sceKernelStartModule(mod, n, arg, &status, NULL);
    PORT_LOG("me: %s loaded (%08X), start %08X status %d\n", path, (unsigned) mod, (unsigned) r, status);
    me_probe();
}
#endif
