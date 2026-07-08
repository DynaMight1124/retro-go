#include "savestate.h"
#include "PR/ultratypes.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fsutils.h"

#define SAVESTATE_FILENAME "sm64_savestate"
#define SAVESTATE_VERSION 1 

extern char __data_start[];
extern char end[];

#ifdef RETRO_GO
static char __start_dontsave[1];
static char __stop_dontsave[1];
#else
extern char __start_dontsave[];
extern char __stop_dontsave[];
#endif

extern char *prog_name;
extern uint8_t *texcache;

static int savestate_request SAVESTATE_EXCLUDE = 0;

static void* memstart = __data_start;
static void* memstop = end;

void savestate_get_name(int slot, char* namebuffer) {
    if (slot == QUICKSAVE_SLOT) {
        sprintf(namebuffer, "%s.quick", SAVESTATE_FILENAME);
    } else {
        sprintf(namebuffer, "%s_%d", SAVESTATE_FILENAME, slot);
    }
}

void savestate_request_save(int slot) {
    savestate_request = slot + 1;
}

void savestate_request_load(int slot) {
    savestate_request = -(slot + 1);
}

static void savestate_save(int slot) {
    char filename[1024];
    savestate_get_name(slot, filename);
    FILE* fp = fopen_home(filename, "wb");
    if (fp == NULL) return;

    int version = SAVESTATE_VERSION;
    fwrite(&version, sizeof(int), 1, fp);

    const size_t preserve_size = __stop_dontsave - __start_dontsave;
    void* preserve = malloc(preserve_size);
    memcpy(preserve, __start_dontsave, preserve_size);

    size_t size = memstop - memstart;
    fwrite(&size, sizeof(size_t), 1, fp);
    fwrite(memstart, 1, size, fp);

    memcpy(__start_dontsave, preserve, preserve_size);
    free(preserve);

    fclose(fp);
}

static void savestate_load(int slot) {
    char filename[1024];
    savestate_get_name(slot, filename);
    FILE* fp = fopen_home(filename, "rb");
    if (fp == NULL) return;

    int version;
    fread(&version, sizeof(int), 1, fp);
    if (version != SAVESTATE_VERSION) {
        fclose(fp);
        return;
    }

    const size_t preserve_size = __stop_dontsave - __start_dontsave;
    void* preserve = malloc(preserve_size);
    memcpy(preserve, __start_dontsave, preserve_size);

    size_t size;
    fread(&size, sizeof(size_t), 1, fp);
    if (size != (size_t)(memstop - memstart)) {
        free(preserve);
        fclose(fp);
        return;
    }
    fread(memstart, 1, size, fp);

    memcpy(__start_dontsave, preserve, preserve_size);
    free(preserve);

    fclose(fp);
}

void savestate_check() {
    if (savestate_request > 0) {
        savestate_save(savestate_request - 1);
        savestate_request = 0;
    } else if (savestate_request < 0) {
        savestate_load(-savestate_request - 1);
        savestate_request = 0;
    }
}
