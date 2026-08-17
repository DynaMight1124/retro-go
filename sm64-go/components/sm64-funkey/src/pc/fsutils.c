#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rg_system.h>
#include "unistd.h"
#include "fsutils.h"

#define FULLPATH_BUFFER_SIZE 1024

static void get_home_filename(const char* filename, char* outbuffer) {
    if (strcmp(filename, "sm64_save_file.bin") == 0) {
        /* This standalone, ROM-clean port has no ROM filename to key its SRAM
         * path. Use a stable application-local name instead of NULL, which
         * resolves to the saves directory itself rather than a file. */
        char *path = rg_emu_get_path(RG_PATH_SAVE_SRAM, "sm64-go");
        strncpy(outbuffer, path, FULLPATH_BUFFER_SIZE);
        outbuffer[FULLPATH_BUFFER_SIZE - 1] = '\0';
        free(path);
    } else {
        char *path = rg_emu_get_path(RG_PATH_SAVE_STATE, filename);
        strncpy(outbuffer, path, FULLPATH_BUFFER_SIZE);
        outbuffer[FULLPATH_BUFFER_SIZE - 1] = '\0';
        free(path);
    }
}

FILE *fopen_home(const char *filename, const char *mode)
{
    char fnamepath[FULLPATH_BUFFER_SIZE];
    get_home_filename(filename, fnamepath);
    return fopen(fnamepath, mode);
} 

int file_exists_home(const char *filename) {
    char fnamepath[FULLPATH_BUFFER_SIZE];
    get_home_filename(filename, fnamepath);
    return access(fnamepath, F_OK) == 0;
}
