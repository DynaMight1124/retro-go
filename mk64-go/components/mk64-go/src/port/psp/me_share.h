/* The control line shared by the EBOOT (src/port/psp/me.c, user mode, main
 * CPU) and mk64k.prx (tools/psp/mekprx/mek.c, kernel mode), whose
 * dispatch loop runs on the Media Engine.  Exactly one 64-byte cache line,
 * only ever accessed through the uncached alias (0x4xxxxxxx) on both sides:
 * no cache maintenance is needed for the line itself.  Plain C, no SDK
 * types: both a user and a kernel build include it. */
#ifndef ME_SHARE_H
#define ME_SHARE_H

struct MeShare {
    volatile unsigned int start;       /* CPU -> ME: run func(param) */
    volatile unsigned int done;        /* ME -> CPU: func returned, result valid */
    volatile unsigned int func;        /* int (*)(int): user-memory code the ME jumps to */
    volatile unsigned int param;
    volatile int result;
    volatile unsigned int arg[2];      /* extra arguments func reads back (port_me_arg) */
    volatile unsigned int init;        /* the ME loop runs while this is set */
    volatile unsigned int alive;       /* ME: functions completed */
    volatile unsigned int spins;       /* ME: idle heartbeat (+1 per 1024 polls) */
    volatile int boot_status;          /* PRX: 1 booted, 0 never attempted */
    volatile unsigned int reboot_req;  /* CPU -> PRX watch thread: re-boot the ME (resume) */
    volatile unsigned int reboot_done; /* PRX -> CPU: reboot_req serviced */
    unsigned int pad[3];
};

#define ME_PING_MAGIC 0x5A5A5A5Au

#endif
