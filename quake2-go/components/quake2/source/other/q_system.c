
// Author: Alejandro Villegas Alonso
//         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
//
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client/keys.h"
#include "../qcommon/qcommon.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "q_arena.h"
#include <rg_system.h>
#include "glob.h"
extern void QG_Quit(void) __attribute__((noreturn));
extern void QG_Error(const char *) __attribute__((noreturn));

#include "../quakegeneric.h"

// Single global debug switch (menuconfig -> QUAKE2_ESP32P4_DEBUG).
#ifdef CONFIG_QUAKE2_ESP32P4_DEBUG
#define QUAKE2_ESP32P4_DEBUG
#endif

// All permanent engine memory lives in this arena (hunk + zone + images +
// surface cache + software framebuffers). Tune at build time with
// -DQUAKE_MEMSIZE. The hunk pool is carved out of the front.
#ifndef QUAKE_MEMSIZE
#define QUAKE_MEMSIZE (16 * 1024 * 1024)
#endif

#define K_LAST 256
static unsigned char KeyStates[K_LAST];

void ProcessKeyEvent(int key, qboolean down);

struct {
  int key;
  int down;
} keyq[64];

int keyq_head = 0;
int keyq_tail = 0;

cvar_t *nostdout = NULL;

extern cvar_t *vid_fullscreen;

unsigned sys_frame_time;

qboolean stdin_active = false;

// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput(char *string) {
  if (nostdout && nostdout->value)
    return;

  fputs(string, stdout);
}

void Sys_Printf(char *fmt, ...) {
  va_list argptr;
  char text[1024];
  unsigned char *p;

  va_start(argptr, fmt);
  vsnprintf(text, sizeof(text), fmt, argptr);
  va_end(argptr);

  if (strlen(text) > sizeof(text))
    Sys_Error("memory overwrite in Sys_Printf");

  if (nostdout && nostdout->value)
    return;

  for (p = (unsigned char *)text; *p; p++) {
    *p &= 0x7f;
    if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
      printf("[%02x]", *p);
    else
      putc(*p, stdout);
  }
}

void Sys_Quit(void) {
  // Com_Quit owns server/client shutdown. Do not enter CL_Shutdown twice.
  Qcommon_Shutdown();
  // fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
  QG_Quit();
}

void Sys_Init(void) {
  memset(KeyStates, 0, sizeof(KeyStates));

#if id386
//	Sys_SetFPCW();
#endif
}

void Sys_Error(char *error, ...) {
  va_list argptr;
  char string[1024];

  // change stdin to non blocking
  // fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

  va_start(argptr, error);
  vsnprintf(string, sizeof(string), error, argptr);
  va_end(argptr);
  fprintf(stderr, "Error: %s\n", string);

  // Fatal errors can happen during partial initialization; do not reenter it.
  QG_Error(string);
}

void Sys_Warn(char *warning, ...) {
  va_list argptr;
  char string[1024];

  va_start(argptr, warning);
  vsnprintf(string, sizeof(string), warning, argptr);
  va_end(argptr);
  fprintf(stderr, "Warning: %s", string);
}

char *Sys_ConsoleInput(void) { return NULL; }

/*****************************************************************************/

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame(void) {}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI(void *parms) {
  extern void *GetGameAPI(void *);
  return GetGameAPI(parms);
}

/*****************************************************************************/

void Sys_AppActivate(void) {}

void Sys_SendKeyEvents(void) {
#ifndef DEDICATED_ONLY
  static int reenterGuard = 0;

  if (reenterGuard == 0) {
    reenterGuard = 1;

    // Consume key queue
    while (keyq_head != keyq_tail) {
      ProcessKeyEvent(keyq[keyq_tail].key, keyq[keyq_tail].down);
      keyq_tail = (keyq_tail + 1) & 63;
    }

    reenterGuard = 0;
  }
#endif

  // grab frame time
  sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/

char *Sys_GetClipboardData(void) { return NULL; }

void Sys_CopyProtect(void) {}

int curtime;
int Sys_Milliseconds(void) {
  curtime = QG_Milliseconds();
  return curtime;
}

void Sys_Mkdir(char *path) { QG_Mkdir(path); }

// The standalone stubs never enumerated saved levels. Native save copying and
// map transitions require full paths from this iterator, including *.sav/*.sv2.
typedef struct find_entry_s {
  struct find_entry_s *next;
  unsigned attributes;
  char path[MAX_OSPATH];
} find_entry_t;
static find_entry_t *find_list, *find_current;
static char find_pattern[MAX_OSPATH];

static int find_entry(const rg_scandir_t *file, void *arg) {
  (void)arg;
  if (strlen(file->path) >= MAX_OSPATH ||
      !glob_match(find_pattern, (char *)file->basename)) return RG_SCANDIR_CONTINUE;
  find_entry_t *entry = malloc(sizeof(*entry));
  if (!entry) Sys_Error("File enumeration allocation failed");
  strcpy(entry->path, file->path);
  entry->attributes = (file->is_dir ? SFF_SUBDIR : 0) |
                      (file->basename[0] == '.' ? SFF_HIDDEN : 0);
  entry->next = find_list;
  find_list = entry;
  return RG_SCANDIR_CONTINUE;
}

void Sys_FindClose(void) {
  while (find_list) {
    find_entry_t *next = find_list->next;
    free(find_list);
    find_list = next;
  }
  find_current = NULL;
}

char *Sys_FindNext(unsigned musthave, unsigned canhave) {
  while (find_current) {
    find_entry_t *entry = find_current;
    find_current = entry->next;
    if ((entry->attributes & musthave) == musthave &&
        !(entry->attributes & ~(musthave | canhave))) return entry->path;
  }
  return NULL;
}

char *Sys_FindFirst(char *path, unsigned musthave, unsigned canhave) {
  Sys_FindClose();
  const char *slash = strrchr(path, '/');
  if (!slash || slash == path || strlen(path) >= MAX_OSPATH) return NULL;
  char directory[MAX_OSPATH];
  size_t length = slash - path;
  memcpy(directory, path, length);
  directory[length] = 0;
  strcpy(find_pattern, slash + 1);
  rg_storage_scandir(directory, find_entry, NULL, RG_SCANDIR_FILES | RG_SCANDIR_DIRS | RG_SCANDIR_STAT);
  find_current = find_list;
  return Sys_FindNext(musthave, canhave);
}

void ProcessKeyEvent(int key, qboolean down) {
  if (key >= 0 && key < K_LAST) {
    KeyStates[key] = down ? 1 : 0;
  }

  Key_Event(key, down, Sys_Milliseconds());

  if (KeyStates[K_ALT] && key == K_ENTER && down) {
    Cvar_SetValue("vid_fullscreen", vid_fullscreen->value ? 0 : 1);
  }
}

void Quake2_Init(int argc, char **argv) {
  keyq_head = 0;
  keyq_tail = 0;
  memset(keyq, 0, sizeof(keyq));

  // Carve the whole engine arena from PSRAM before any engine allocation.
  if (!Q_Arena_Init(QUAKE_MEMSIZE)) Sys_Error("Not enough PSRAM for Quake II arena");
  Hunk_Init();

  Qcommon_Init(argc, argv);

  nostdout = Cvar_Get("nostdout", "0", 0);
}

void Quake2_Frame(int msec) {
  Qcommon_Frame(msec);

#ifdef QUAKE2_ESP32P4_DEBUG
  static int frame_count;
  extern int z_bytes;

  if ((frame_count++ & 63) == 0) {
    ESP_LOGI(
        "MEM",
        "psram_free=%d psram_largest=%d internal_free=%d hunk_pool_used=%d "
        "z_bytes=%d arena_used=%d arena_free=%d arena_largest=%d",
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        Hunk_PoolUsed(), z_bytes, Q_Arena_Used(), Q_Arena_FreeSize(),
        Q_Arena_Largest());
  }
#endif
}

void ri_esp_log_model(const char *name, int filelen, int datasize) {
#ifdef QUAKE2_ESP32P4_DEBUG
  ESP_LOGI("MODEL", "%s file=%d data=%d", name, filelen, datasize);
#endif
}

int Quake2_Milliseconds() { return Sys_Milliseconds(); }

void Quake2_SendKey(int key, int down) {
  keyq[keyq_head].key = key;
  keyq[keyq_head].down = down;
  keyq_head = (keyq_head + 1) & 63;
}
