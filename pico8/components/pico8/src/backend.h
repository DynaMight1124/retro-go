// backend.h
// ============================================================================
// Interface that every concrete backend (SDL, rawdraw, retro-go, ...) must
// implement. The retro-go backend lives in retro_go_backend.c; the unity-build
// engine TU consumes these via the prototypes below.
// ============================================================================

#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Engine TU is compiled as C++; the retro_go_backend.c TU is compiled as C.
// Without this `extern "C"` block, the C++ compiler would mangle every call
// in engine.cpp (e.g. to `_Z9gfx_flipv`) while the C TU produces the unmangled
// symbol `gfx_flip`, producing six "undefined reference" link errors. The
// `__cplusplus` guard makes the header safely include from C consumers too
// (e.g. from a hypothetical second C TU that wants the callbacks).
#ifdef __cplusplus
extern "C" {
#endif

bool    init_video(void);
void    video_close(void);
void    draw_hud(void);
void    gfx_flip(void);
void    gfx_redraw(void);
bool    gfx_screenshot(const char *filename, int width, int height);
void    delay(uint16_t ms);
bool    handle_input(void);
void    handle_input_reset(void);
uint32_t now(void);
bool    init_audio(void);
void    audio_task_set_paused(bool paused);
void    audio_task_stop(void);
bool    init_platform(void);

// PICO-8's cartdata()/dget()/dset() persistent 256-byte store.
bool    cartdata_open(const char *id, uint32_t *data, size_t size);
void    cartdata_mark_dirty(void);
bool    cartdata_flush(void);

uint8_t current_hour(void);
uint8_t current_minute(void);
uint8_t wifi_strength(void);
uint8_t battery_left(void);

#ifdef __cplusplus
}
#endif

#endif // BACKEND_H
