#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#ifndef GP_ROM_READ
#define GP_ROM_READ(fp, dst, n) ((int64_t)fread(dst, 1, n, fp))
#endif

/* Keep the expanded-ROM limit accepted by the P4 backend. No full-ROM buffer. */
static bool gp_rom_size(FILE *fp, uint32_t *size, const char **why)
{
    *why = "Could not read ROM size";
    if (fseek(fp, 0, SEEK_END)) return false;
    long n = ftell(fp);
    if (n < 0xc0 || n > 0x20000000L) { *why="Invalid ROM size"; return false; }
    if (fseek(fp, 0, SEEK_SET)) return false;
    unsigned char magic[4];
    if (fread(magic, 1, 4, fp)!=4 || ferror(fp)) return false;
    if (magic[0]=='P' && magic[1]=='K' &&
        ((magic[2]==3 && magic[3]==4) || (magic[2]==5 && magic[3]==6) ||
         (magic[2]==7 && magic[3]==8))) {
        *why="ZIP is not supported; extract the .gba file first"; return false;
    }
    if (fseek(fp, 0, SEEK_SET)) return false;
    *size=(uint32_t)n;
    return true;
}

/* Read exactly the on-disk part. Only the known final tail may be padded.
 * A short/error read is never published as a valid mapped ROM page. */
static bool gp_rom_read_at(FILE *fp, void *dst, size_t capacity, uint32_t offset, uint32_t file_size)
{
    size_t expected=offset < file_size ? file_size-offset : 0;
    if (expected>capacity) expected=capacity;
    memset((uint8_t *)dst+expected, 0xff, capacity-expected);
    if (!expected) return true;
    clearerr(fp);
    if (fseek(fp, (long)offset, SEEK_SET)) return false;
    int64_t got=GP_ROM_READ(fp,dst,expected);
    return got==(int64_t)expected && !ferror(fp);
}
