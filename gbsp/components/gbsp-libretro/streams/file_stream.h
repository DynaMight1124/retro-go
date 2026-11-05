#if defined(RISCV_ARCH)
/* filestream shim: maps libretro filestream API to stdio for ESP-IDF */
#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef FILE RFILE;

static inline RFILE *filestream_open(const char *path, unsigned mode, unsigned hints)
{
    (void)mode; (void)hints;
    return fopen(path, "rb");
}

/* One scratch owner in gpsp_rom_stream.c, not one copy per including file.
 * Reads/cleanup are serialized on the emulation thread. Battery saves use stdio
 * independently and the renderer never reads this ROM stream. */
#ifdef __cplusplus
extern "C" {
#endif
int64_t filestream_read(RFILE *f, void *buf, int64_t len);
void filestream_read_cleanup(void);
#ifdef __cplusplus
}
#endif

static inline void filestream_close(RFILE *f)
{
    if (f) {
        fclose(f);
        filestream_read_cleanup();
    }
}

static inline int64_t filestream_get_size(RFILE *f)
{
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, cur, SEEK_SET);
    return (int64_t)sz;
}

static inline int64_t filestream_seek(RFILE *f, int64_t offset, int whence)
{
    return (int64_t)fseek(f, (long)offset, whence);
}

static inline int64_t filestream_write(RFILE *f, const void *buf, int64_t len)
{
    return (int64_t)fwrite(buf, 1, (size_t)len, f);
}

#endif /* FILE_STREAM_H */
#else
#include "../libretro/libretro-common/include/streams/file_stream.h"
#endif
