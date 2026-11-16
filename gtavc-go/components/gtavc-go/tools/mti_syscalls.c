/*
 * Minimal Linux/MIPS syscalls so the bare-metal GCC 7.4 (device toolchain)
 * build can be linked with newlib and run under qemu-mipsel for verification.
 * This file is NOT part of the console core.
 */
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include <errno.h>

static long mti_sys(long n, long a, long b, long c) {
    register long v0 asm("$2") = n;
    register long a0 asm("$4") = a;
    register long a1 asm("$5") = b;
    register long a2 asm("$6") = c;
    asm volatile("syscall" : "+r"(v0) : "r"(a0), "r"(a1), "r"(a2) : "memory");
    return v0;
}

static unsigned char mti_heap[24 * 1024 * 1024];
static unsigned int mti_hp;

void* sbrk(int inc) {
    if (inc < 0) return (void*)-1;
    if (mti_hp + (unsigned)inc > sizeof(mti_heap)) return (void*)-1;
    void* p = mti_heap + mti_hp;
    mti_hp += (unsigned)inc;
    return p;
}

int write(int fd, const void* buf, int n) { return (int)mti_sys(4004, fd, (long)buf, n); }
int read(int fd, void* buf, int n) { return (int)mti_sys(4003, fd, (long)buf, n); }
int open(const char* p, int flags, int mode) { return (int)mti_sys(4005, (long)p, flags, mode); }
int close(int fd) { return (int)mti_sys(4006, fd, 0, 0); }
long lseek(int fd, long off, int whence) { return mti_sys(4019, fd, off, whence); }
int isatty(int fd) { (void)fd; return 0; }

int fstat(int fd, struct stat* st) {
    (void)fd;
    if (st) {
        unsigned char* p = (unsigned char*)st;
        for (unsigned i = 0; i < sizeof(struct stat); i++) p[i] = 0;
        /* newlib checks st_mode; 0x8000 = S_IFREG */
        st->st_mode = 0x8000;
    }
    return 0;
}

/* SF2000 platform shim: monotonic-ish counter for the test build. */
uint32_t os_get_tick_count(void) {
    static uint32_t t;
    t += 33;
    return t;
}

int kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
int getpid(void) { return 1; }

void software_init_hook(void) { }
void hardware_exit_hook(void) { }
void _init(void) { }
void _fini(void) { }
int unlink(const char* p) { return (int)mti_sys(4010, (long)p, 0, 0); }
long times(void* p) { if (p) { long* q = (long*)p; for (int i = 0; i < 4; i++) q[i] = 0; } return 0; }
int mkdir(const char* p, mode_t m) { return (int)mti_sys(4039, (long)p, (long)m, 0); }

char __stack[262144];
void get_mem_info(void* p) { (void)p; }
void hardware_init_hook(void) { }
void hardware_hazard_hook(void) { }
static int mti_errno;
int* __errno(void) { return &mti_errno; }
