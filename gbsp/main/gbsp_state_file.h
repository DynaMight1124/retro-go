#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#ifndef GP_STATE_SYNC
#ifdef _WIN32
#include <io.h>
#define GP_STATE_SYNC(fp) _commit(_fileno(fp))
#else
#include <unistd.h>
#define GP_STATE_SYNC(fp) fsync(fileno(fp))
#endif
#endif

/* The caller is Retro-Go's .new file, not the live slot. Its wrapper owns
 * .new/.bak renaming. Never report buffered failures as a successful state. */
static bool gp_state_write(const char *filename, const void *data, size_t size)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return false;
    bool ok = fwrite(data, 1, size, fp) == size && fflush(fp) == 0;
    if (ok && GP_STATE_SYNC(fp) != 0) {
        ok = errno == ENOSYS;
#ifdef ENOTSUP
        ok = ok || errno == ENOTSUP;
#endif
    }
    if (fclose(fp) != 0) ok = false;
    if (!ok) remove(filename);
    return ok;
}

/* Restore only a missing slot; never replace an existing file or promote .new.
 * A .bak is the previously committed slot, not a possibly unfinished write. */
static bool gp_state_recover(const char *filename, size_t size)
{
    struct stat info;
    if (stat(filename, &info) == 0 || errno != ENOENT) return false;
    char backup[320];
    int n = snprintf(backup, sizeof(backup), "%s.bak", filename);
    if (n < 0 || (size_t)n >= sizeof(backup)) return false;
    if (stat(backup, &info) || (size_t)info.st_size != size) return false;
    return rename(backup, filename) == 0;
}
