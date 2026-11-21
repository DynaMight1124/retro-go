// Native file/time boundary. The libretro frontend and VFS are not linked.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

extern "C" {
struct RFILE { FILE *file; };
RFILE *rfopen(const char *path, const char *mode)
{
    FILE *file = fopen(path, mode);
    if (!file) return NULL;
    RFILE *stream = (RFILE *)malloc(sizeof(RFILE));
    if (!stream) { fclose(file); return NULL; }
    stream->file = file;
    return stream;
}
int rfclose(RFILE *stream)
{
    int result = fclose(stream->file);
    free(stream);
    return result;
}
int64_t rfseek(RFILE *stream, int64_t offset, int origin)
{
    return fseek(stream->file, (long)offset, origin);
}
int64_t rftell(RFILE *stream) { return ftell(stream->file); }
int rfgetc(RFILE *stream) { return fgetc(stream->file); }
int rfputc(int value, RFILE *stream) { return fputc(value, stream->file); }
struct tm *rtime_localtime(const time_t *time, struct tm *result)
{
    return localtime_r(time, result);
}
}
