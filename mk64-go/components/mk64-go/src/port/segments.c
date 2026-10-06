/**
 * Segmented-address resolution and the memory regions the N64 linker used to
 * provide (see port.h for the overall scheme).
 */
#include <ultra64.h>
#include <macros.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "port.h"

/* ------------------------------------------------------------------------- */
/* Memory pool                                                                */
/* ------------------------------------------------------------------------- */

/* On the N64 the pool sits between the main code segment and the racing/ending
 * overlays; gHeapEndPtr allocates downward from the overlay boundary and
 * gNextFreeMemoryAddress upward from the pool start.  The port gives the game
 * one contiguous pool and makes every boundary symbol point at its ends. */
ALIGNED16 u8 gPortMemoryPool[PORT_MEMORY_POOL_SIZE];

/* The decomp's segments.h reads these as `extern u8 sym[]`; alias them onto
 * the pool with the assembler so they resolve to the right addresses. */
__asm__(
    ".globl _memoryPoolSegmentNoloadStart\n"
    ".set _memoryPoolSegmentNoloadStart, gPortMemoryPool\n"
    ".globl _memoryPoolSegmentNoloadEnd\n"
    ".set _memoryPoolSegmentNoloadEnd, gPortMemoryPool + 0x300000\n"
    /* SEG_ENDING / SEG_RACING are the top of the downward-growing heap. */
    ".globl _endingSegmentStart\n"
    ".set _endingSegmentStart, gPortMemoryPool + 0x300000\n"
    ".globl _endingSegmentNoloadEnd\n"
    ".set _endingSegmentNoloadEnd, gPortMemoryPool + 0x300000\n"
    ".globl _racingSegmentStart\n"
    ".set _racingSegmentStart, gPortMemoryPool + 0x300000\n"
    ".globl _racingSegmentNoloadEnd\n"
    ".set _racingSegmentNoloadEnd, gPortMemoryPool + 0x300000\n"
);

/* Code overlays: nothing to DMA on the PSP (everything is linked in).  The
 * *End symbols alias the *Start ones so every computed size is 0. */
ALIGNED16 u8 _racingSegmentRomStart[16];
ALIGNED16 u8 _endingSegmentRomStart[16];
__asm__(
    ".globl _racingSegmentRomEnd\n"
    ".set _racingSegmentRomEnd, _racingSegmentRomStart\n"
    ".globl _endingSegmentRomEnd\n"
    ".set _endingSegmentRomEnd, _endingSegmentRomStart\n"
);

/* Trig tables: on the N64 gSineTable & co are DMA'd from ROM into a bss
 * buffer the code is linked against; here they are initialised data, so the
 * "DMA" is a 0-byte copy (aliases via --defsym in Makefile.psp). */
__asm__(".globl _trigTablesSegmentSize\n.set _trigTablesSegmentSize, 0\n");

/* data_segment2 / common textures / ceremony / startup logo: all compiled in
 * and reached through the segment tables, so the loads become no-ops. */
ALIGNED16 u8 _data_segment2SegmentRomStart[16];
ALIGNED16 u8 _common_texturesSegmentRomStart[16];
ALIGNED16 u8 _ceremonyDataSegmentRomStart[16];
ALIGNED16 u8 _startupLogoSegmentRomStart[16];
__asm__(
    ".globl _data_segment2SegmentRomEnd\n"
    ".set _data_segment2SegmentRomEnd, _data_segment2SegmentRomStart\n"
    ".globl _common_texturesSegmentRomEnd\n"
    ".set _common_texturesSegmentRomEnd, _common_texturesSegmentRomStart\n"
    ".globl _ceremonyDataSegmentRomEnd\n"
    ".set _ceremonyDataSegmentRomEnd, _ceremonyDataSegmentRomStart\n"
    ".globl _startupLogoSegmentRomEnd\n"
    ".set _startupLogoSegmentRomEnd, _startupLogoSegmentRomStart\n"
);

/* Texture blobs the game DMAs by (segment offset) - on the PSP the callers
 * pass real pointers, so these bases are never dereferenced (see the
 * PORT_ROM_PTR sites in the game code). */
u8 _kart_texturesSegmentRomStart[16];
u8 _other_texturesSegmentRomStart[16];
u8 _textures_0aSegmentRomStart[16];
u8 _textures_0bSegmentRomStart[16];

/* The sound data and the segment-0x0B noise texture are aliased with
 * --defsym in Makefile.psp (the assembler cannot alias external symbols). */

/* RSP microcode symbols referenced when building SP tasks - never executed. */
u64 rspF3DBootStart[2], rspF3DBootEnd[1];
u64 gspF3DEXTextStart[2], gspF3DEXDataStart[2];
u64 gspF3DLXTextStart[2], gspF3DLXDataStart[2];
u64 rspAspMainStart[2], rspAspMainDataStart[2], rspAspMainDataEnd[1];

/* ------------------------------------------------------------------------- */
/* Segment translation                                                        */
/* ------------------------------------------------------------------------- */

static PortSegTable sSegTables[16];

void port_set_segment_table(s32 segment, const PortSegTable* table) {
    if (table != NULL) {
        sSegTables[segment] = *table;
    } else {
        sSegTables[segment].entries = NULL;
        sSegTables[segment].count = 0;
    }
}

void port_clear_segment_table(s32 segment) {
    port_set_segment_table(segment, NULL);
}

static const PortSegEntry* seg_table_lookup(const PortSegTable* t, u32 offset) {
    s32 lo = 0;
    s32 hi = t->count - 1;
    while (lo <= hi) {
        s32 mid = (lo + hi) >> 1;
        const PortSegEntry* e = &t->entries[mid];
        if (offset < e->offset) {
            hi = mid - 1;
        } else if (offset >= e->offset + e->size) {
            lo = mid + 1;
        } else {
            return e;
        }
    }
    return NULL;
}

void* port_seg_to_ptr(uintptr_t addr) {
    u32 segment;
    u32 offset;
    const PortSegTable* table;

    if (addr >= 0x08800000u) {
        return (void*) addr; // already a PSP pointer
    }
    segment = addr >> 24;
    offset = addr & 0x00FFFFFF;
    if (segment >= 16) {
        return (void*) addr;
    }
    table = &sSegTables[segment];
    if (table->count != 0) {
        const PortSegEntry* e = seg_table_lookup(table, offset);
        if (e != NULL) {
            return (void*) ((const u8*) e->ptr + (offset - e->offset));
        }
        if (gSegmentTable[segment] == 0) {
            PORT_LOG("seg %X: offset %06X not in table\n", segment, offset);
            return NULL;
        }
    }
    return (void*) (gSegmentTable[segment] + offset);
}

/* ------------------------------------------------------------------------- */
/* Logging                                                                    */
/* ------------------------------------------------------------------------- */

extern u32 port_time_us(void);
static u32 sLogCostMs;
/* A memory-stick write holds the game for tens of milliseconds -- several
 * pictures at 60 fps.  While gPortLogDefer is set (main.c: an unpaused race)
 * lines collect here and reach the file when the race pauses or ends, or when
 * the buffer fills.  A crash in a race loses them; an empty data/logsync file
 * writes every line through as before. */
s32 gPortLogDefer;
static char sLogRam[24 * 1024];
static u32 sLogRamUsed;
static void log_write(const char* text);
static u32 sLogDropped; /* bytes of older lines given up while a race filled the buffer */
void port_log_flush(void) {
    if (sLogDropped != 0) {
        char note[80];
        snprintf(note, sizeof(note), "[log: %u bytes of older race lines dropped here]\n", (unsigned) sLogDropped);
        sLogDropped = 0;
        log_write(note);
    }
    if (sLogRamUsed != 0) {
        sLogRam[sLogRamUsed] = 0;
        sLogRamUsed = 0;
        log_write(sLogRam);
    }
}
void port_log(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    u32 t = port_time_us();
    int n = snprintf(buf, sizeof(buf), "[%u.%03u] ", (unsigned) (t / 1000000u), (unsigned) ((t / 1000u) % 1000u));
    va_start(ap, fmt);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);
    /* No I/O of any kind while a race is deferred to RAM -- stdout included: it
     * goes through the kernel's I/O layer too, and a 2.3 s freeze on hardware
     * sat inside this function with the RAM buffer a quarter full. */
    if (!gPortLogDefer) {
        fputs(buf, stdout);
    }
    {
        static s32 sSync = -1;
        u32 len = (u32) strlen(buf);
        if (sSync < 0) {
            sSync = 0;
#ifdef PORT_DEBUG_KNOBS
            {
                FILE* f = fopen(port_save_path("logsync"), "rb");
                sSync = f != NULL;
                if (f != NULL) fclose(f);
            }
#endif
        }
        if (gPortLogDefer && !sSync) {
            if (sLogRamUsed + len + 1 > sizeof(sLogRam)) {
                /* Full in the middle of a race (about six minutes without a
                 * pause): writing it out now is the stall this buffer exists to
                 * avoid.  Drop the older half, at a line boundary, and say so. */
                u32 cut = sLogRamUsed / 2;
                while (cut < sLogRamUsed && sLogRam[cut - 1] != '\n') cut++;
                memmove(sLogRam, sLogRam + cut, sLogRamUsed - cut);
                sLogRamUsed -= cut;
                sLogDropped += cut;
            }
            memcpy(sLogRam + sLogRamUsed, buf, len);
            sLogRamUsed += len;
            return;
        }
        port_log_flush();
    }
    log_write(buf);
}
static void log_write(const char* buf) {
    /* The PSP's FAT driver only updates the directory entry (the visible file
     * size) on close, so a HOME exit or a crash used to leave log.txt
     * truncated at the last close.  Close and reopen at most twice a second:
     * at most half a second of log can be lost, and a debug trace of
     * thousands of lines does not pay an open per line (PPSSPP charges
     * milliseconds each). */
    {
        static FILE* sLog;
        static int sTruncated;
        static u32 sLastClose;
        u32 now = port_time_us();
        if (sLog == NULL) {
            if (!sTruncated) { /* first open: keep the previous run's log next to this one */
                char prev[256];
                snprintf(prev, sizeof(prev), "%s", port_save_path("log_prev.txt"));
                remove(prev);
                rename(port_save_path("log.txt"), prev);
            }
            sLog = fopen(port_save_path("log.txt"), sTruncated ? "a" : "w");
            sTruncated = 1;
        }
        if (sLog != NULL) {
            fputs(buf, sLog);
            fflush(sLog);
            if (now - sLastClose >= 500000u) {
                fclose(sLog);
                sLog = NULL;
                sLastClose = now;
            }
        }
        { /* how long the memory stick held us up (the frame log reports the worst) */
            u32 took = (port_time_us() - now) / 1000u;
            if (took > sLogCostMs) sLogCostMs = took;
        }
    }
}
u32 port_log_cost_ms(void) { u32 m = sLogCostMs; sLogCostMs = 0; return m; }
