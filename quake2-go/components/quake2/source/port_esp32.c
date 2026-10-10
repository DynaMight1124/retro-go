/*
 * port_esp32.c -- ESP32-P4 platform glue for Quake 2 (quake2generic engine)
 *
 * Author: Alejandro Villegas Alonso
 *         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
 *
 * Implements the quakegeneric.h porting interface and the SWimp_*
 * software-renderer video driver, wiring the engine to the ESP32-P4
 * hardware abstraction code in main/ (display, input, timing).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "client/keys.h"
#include <rg_system.h>
#include "other/q_arena.h"
#include "ref_soft/r_local.h"

#include "quake2.h"

/* Hardware abstraction routines provided by the main component */
void QG_SetPalette(unsigned char palette[768]);
void QG_DrawFrame(void *pixels);
int QG_GetKey(int *down, int *key);
void QG_GetMouseMove(int *x, int *y);
int QG_Milliseconds(void);

void S_FlushCaches(void);

// Called at the very start of a map change, before the new map's BSP is read
// and while the old map's media is still resident. Evicting the old renderer
// media (images, models and sounds) early frees several MB in both the engine
// arena and the model hunk pool so the new map's world ("Hunk_Begin 4MB"),
// models and big transient file buffers fit without a double-load peak.
void Q_MapLoading(void) {
  extern void QG_LoadBegin(void);
  QG_LoadBegin();
  registration_sequence++;
  R_FreeUnusedImages();
  Mod_FreeAll();
  S_FlushCaches();
}

/* ------------------------------------------------------------------ */
/* Porting interface (quakegeneric.h)                                  */
/* ------------------------------------------------------------------ */

#if defined(__cplusplus)
extern "C" {
#endif

extern void QG_GetMouseMove(int *x, int *y);
void QG_GetMouseDiff(int *dx, int *dy) { QG_GetMouseMove(dx, dy); }

void QG_CaptureMouse(void) {}

void QG_ReleaseMouse(void) {}

void QG_Mkdir(const char *path) { rg_storage_mkdir(path); }

#if defined(__cplusplus)
}
#endif

/* ------------------------------------------------------------------ */
/* Software renderer video driver (SWimp_*)                           */
/* ------------------------------------------------------------------ */

#define SW_DEFAULT_WIDTH 320
#define SW_DEFAULT_HEIGHT 240

static pixel_t *sw_screen = NULL;
static int sw_width = 0;
static int sw_height = 0;
static qboolean sw_screen_internal =
    false; // screen buffer came from internal RAM

rserr_t SWimp_SetMode(int *pwidth, int *pheight, int mode,
                      qboolean fullscreen) {
  int width = 0, height = 0;

  if (!ri.Vid_GetModeInfo(&width, &height, mode))
    return rserr_invalid_mode;

  /* ESP32-P4: target resolution is 320x240 for the software renderer. */
  width = SW_DEFAULT_WIDTH;
  height = SW_DEFAULT_HEIGHT;

  ri.Con_Printf(PRINT_ALL, "...setting mode %d: %d %d\n", mode, width, height);

  if (sw_screen) {
    if (sw_screen_internal) {
      free(sw_screen);
      sw_screen_internal = false;
    } else
      Q_Arena_Free(sw_screen);
  }

  // Try internal SRAM first: the vid buffer is written for every rendered
  // pixel. Falls back to the PSRAM arena when internal RAM cannot fit it.
  sw_screen = (pixel_t *)rg_alloc(width * height, MEM_FAST | MEM_NOPANIC);
  if (sw_screen)
    sw_screen_internal = true;
  else
    sw_screen = (pixel_t *)Q_Arena_Alloc(width * height);
  if (!sw_screen)
    return rserr_invalid_mode;
  memset(sw_screen, 0, width * height);

  sw_width = width;
  sw_height = height;

  vid.width = width;
  vid.height = height;
  vid.rowbytes = width;
  vid.buffer = sw_screen;

  *pwidth = width;
  *pheight = height;

  ri.Vid_NewWindow(width, height);

  return rserr_ok;
}

void SWimp_Shutdown(void) {
  if (sw_screen) {
    if (sw_screen_internal) {
      free(sw_screen);
      sw_screen_internal = false;
    } else
      Q_Arena_Free(sw_screen);
    sw_screen = NULL;
  }
}

int SWimp_Init(void *hInstance, void *wndProc) { return true; }

qboolean SWimp_InitGraphics(qboolean fullscreen) {
  vid.rowbytes = sw_width;
  vid.buffer = sw_screen;
  return rserr_ok;
}

void SWimp_SetPalette(const unsigned char *palette) {
  /* palette is 256 * 4 bytes (RGBA). Convert to 768-byte RGB. */
  unsigned char rgb[768];
  int i;

  for (i = 0; i < 256; i++) {
    rgb[i * 3 + 0] = palette[i * 4 + 0];
    rgb[i * 3 + 1] = palette[i * 4 + 1];
    rgb[i * 3 + 2] = palette[i * 4 + 2];
  }

  QG_SetPalette(rgb);
}

void SWimp_BeginFrame(float camera_separation) {}

void SWimp_EndFrame(void) { QG_DrawFrame(vid.buffer); }

void SWimp_AppActivate(qboolean active) {}

/* ------------------------------------------------------------------ */
/* Input                                                               */
/* ------------------------------------------------------------------ */

void HandleInput(void) {
  int down = 0;
  int key = 0;

  while (QG_GetKey(&down, &key))
    Quake2_SendKey(key, down);
}
