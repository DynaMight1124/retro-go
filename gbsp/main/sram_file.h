#pragma once
/* The battery-save file: replacing one atomically, and reading one back after a
   power cut in the middle of a replace.
 *
 * Plain stdio and unistd on purpose, with logging, chunking and yielding behind
 * macros, so gbsp/test/sram_file_test.c can compile and exercise exactly this
 * code on the host. The interesting cases -- a half-written file, a cut between
 * the remove and the rename, an empty file -- are the ones that are hard to
 * produce on the board and easy to produce in a test.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef SRAM_FILE_LOGE
#define SRAM_FILE_LOGE(...) ((void)0)
#endif
#ifndef SRAM_FILE_LOGI
#define SRAM_FILE_LOGI(...) ((void)0)
#endif
/* the card is also the ROM's backing store: writing in pieces and yielding
   between them keeps the SD lock short, so an on-demand ROM page read never
   waits for a whole save */
#ifndef SRAM_FILE_CHUNK
#define SRAM_FILE_CHUNK 4096
#endif
#ifndef SRAM_FILE_YIELD
#define SRAM_FILE_YIELD() ((void)0)
#endif

#define SRAM_FILE_TMP ".tmp"

/* gpSP's backup kinds, repeated here so the host test needs none of gpSP */
#define SRAM_BACKUP_SRAM   0
#define SRAM_BACKUP_FLASH  1
#define SRAM_BACKUP_EEPROM 2

/* How much of the 128 KB backup buffer the cartridge actually has. Older
   builds wrote all 128 KB whatever the game was, which is where most of the
   stall came from: a 512-byte EEPROM game pushed 128 KB through the card.
   Anything other than the three kinds below -- gpSP's BACKUP_UNKN -- writes
   the whole buffer, because guessing small there would throw a save away.
   banks/eeprom are gpSP's flash_bank_cnt (1 = 64 KB, 2 = 128 KB) and
   eeprom_size (1 = 512 B, 16 = 8 KB). */
static inline size_t sram_backup_size(unsigned backup_type, unsigned banks,
                                      unsigned eeprom, size_t full)
{
    switch (backup_type)
    {
    case SRAM_BACKUP_SRAM:   return 32 * 1024;
    case SRAM_BACKUP_FLASH:  return 64 * 1024 * (banks ? banks : 1);
    case SRAM_BACKUP_EEPROM: return 512 * (eeprom ? eeprom : 1);
    default:                 return full;
    }
}

static inline void sram_file_tmp_path(char *out, size_t cap, const char *path)
{
    snprintf(out, cap, "%s%s", path, SRAM_FILE_TMP);
}

/* Write len bytes so that a power cut can never leave a half-written save
   where a good one was: the data goes to <path>.tmp, is flushed all the way to
   the card, and only then replaces <path>. FAT will not rename onto a name
   that exists, so the old file is removed first; a cut in that window leaves
   only the .tmp, and sram_file_read below picks it up. */
static inline bool sram_file_write(const char *path, const void *data, size_t len)
{
    char tmp[320];
    const unsigned char *p = (const unsigned char *)data;
    FILE *fp;
    bool ok = true;
    size_t done = 0;

    if (!path || !data || !len)
        return false;
    sram_file_tmp_path(tmp, sizeof(tmp), path);

    fp = fopen(tmp, "wb");
    if (!fp)
    {
        SRAM_FILE_LOGE("battery save: cannot open %s", tmp);
        return false;
    }
    while (done < len)
    {
        const size_t n = (len - done) < SRAM_FILE_CHUNK ? (len - done) : (size_t)SRAM_FILE_CHUNK;
        if (fwrite(p + done, 1, n, fp) != n)
        {
            ok = false;
            break;
        }
        done += n;
        if (done < len)
        {
            /* Push the chunk all the way out before yielding: with stdio's
               buffer still holding it the yield would hand the card nothing
               and the whole save would go out in one bus-hogging burst at
               fclose(). */
            if (fflush(fp) != 0)
            {
                ok = false;
                break;
            }
            SRAM_FILE_YIELD();
        }
    }
    if (ok)
        ok = fflush(fp) == 0;
    if (ok)
    {
        /* best effort: a VFS without fsync must not fail the save */
        const int fd = fileno(fp);
        if (fd >= 0)
            fsync(fd);
    }
    if (fclose(fp) != 0)
        ok = false;
    if (!ok)
    {
        SRAM_FILE_LOGE("battery save: writing %s failed, the old save is untouched", tmp);
        remove(tmp);   /* a partial .tmp must never be mistaken for a recovery */
        return false;
    }

    remove(path);
    if (rename(tmp, path) != 0)
    {
        /* the save is only in the .tmp now; the next load recovers it */
        SRAM_FILE_LOGE("battery save: cannot put %s in place, left as %s", path, tmp);
        return false;
    }
    SRAM_FILE_LOGI("battery save written: %s (%u bytes)", path, (unsigned)len);
    return true;
}

/* Read a save, at most cap bytes, and return how many. 0 means there is
   nothing usable -- an empty or unreadable file counts as nothing, so a
   zero-length file left behind by a failed write never wipes the save already
   in memory. A missing <path> with a <path>.tmp beside it is a power cut
   caught between the remove and the rename: the .tmp is the newer save, so it
   is used and put back in place. */
static inline size_t sram_file_read(const char *path, void *data, size_t cap)
{
    char tmp[320];
    FILE *fp;
    size_t n;

    if (!path || !data || !cap)
        return 0;

    fp = fopen(path, "rb");
    if (!fp)
    {
        sram_file_tmp_path(tmp, sizeof(tmp), path);
        fp = fopen(tmp, "rb");
        if (!fp)
            return 0;
        n = fread(data, 1, cap, fp);
        fclose(fp);
        if (!n)
        {
            remove(tmp);
            return 0;
        }
        SRAM_FILE_LOGI("battery save recovered from %s (%u bytes)", tmp, (unsigned)n);
        remove(path);
        rename(tmp, path);   /* best effort */
        return n;
    }
    n = fread(data, 1, cap, fp);
    fclose(fp);
    if (!n)
        SRAM_FILE_LOGE("battery save: %s is empty, keeping what is in memory", path);
    return n;
}
