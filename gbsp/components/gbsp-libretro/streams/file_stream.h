/* filestream shim: maps libretro filestream API to stdio for ESP-IDF */
#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef ESP_PLATFORM
#include <unistd.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#endif

typedef FILE RFILE;

static inline RFILE *filestream_open(const char *path, unsigned mode, unsigned hints)
{
    (void)mode; (void)hints;
    return fopen(path, "rb");
}

static inline void filestream_close(RFILE *f)
{
    if (f) fclose(f);
}

static inline int64_t filestream_read(RFILE *f, void *buf, int64_t len)
{
#ifdef ESP_PLATFORM
    /* Stage through internal DMA buffer for ~5x faster SD→PSRAM reads.
     * SDMMC DMA targets internal RAM; we memcpy to the PSRAM destination. */
    static const size_t try_sizes[] = { 128*1024, 64*1024, 32*1024, 16*1024, 0 };
    uint8_t *stage = NULL;
    size_t stage_size = 0;
    for (int i = 0; try_sizes[i]; i++) {
        stage = (uint8_t *)heap_caps_malloc(try_sizes[i],
                     MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (!stage)
            stage = (uint8_t *)heap_caps_malloc(try_sizes[i],
                         MALLOC_CAP_INTERNAL);
        if (stage) {
            stage_size = try_sizes[i];
            break;
        }
    }
    if (stage) {
        int fd = fileno(f);
        long fpos = ftell(f);
        lseek(fd, fpos, SEEK_SET);
        uint8_t *dst = (uint8_t *)buf;
        int64_t remaining = len;
        int64_t total = 0;
        while (remaining > 0) {
            size_t chunk = remaining > (int64_t)stage_size ? stage_size : (size_t)remaining;
            ssize_t got = read(fd, stage, chunk);
            if (got <= 0) break;
            memcpy(dst, stage, got);
            dst += got;
            remaining -= got;
            total += got;
        }
        fseek(f, fpos + total, SEEK_SET);
        free(stage);
        return total;
    }
#endif
    return (int64_t)fread(buf, 1, (size_t)len, f);
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
