/*
 * Retro-Go host interface for the Frodo C64 emulator.
 *
 * Keep this interface independent of both SDL and Retro-Go. The application
 * adapter in frodo/main owns display, audio, input, menus and lifecycle; the
 * implementation behind this API owns the complete C64 machine.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct frodo_instance frodo_instance_t;

enum {
    FRODO_JOY_UP    = 1u << 0,
    FRODO_JOY_DOWN  = 1u << 1,
    FRODO_JOY_LEFT  = 1u << 2,
    FRODO_JOY_RIGHT = 1u << 3,
    FRODO_JOY_FIRE  = 1u << 4,
};

enum {
    FRODO_KEY_F1    = 1u << 0,
    FRODO_KEY_SPACE = 1u << 1,
    FRODO_KEY_RETURN = 1u << 2,
    FRODO_KEY_Y      = 1u << 3,
    FRODO_KEY_N      = 1u << 4,
    FRODO_KEY_RUN_STOP = 1u << 5,
};

typedef struct {
    const char *content_path;
    uint32_t audio_sample_rate;
    bool full_1541;
    bool cycle_exact;
} frodo_config_t;

typedef struct {
    uint8_t joystick_port_1;
    uint8_t joystick_port_2;
    uint16_t keyboard;
    uint8_t character; // ASCII character selected through the host keyboard UI
} frodo_input_t;

typedef struct {
    const uint8_t *pixels;
    const uint32_t *palette_rgb888;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint16_t visible_x;
    uint16_t visible_y;
    uint16_t visible_width;
    uint16_t visible_height;
} frodo_video_frame_t;

typedef struct {
    int16_t left;
    int16_t right;
} frodo_audio_frame_t;

/*
 * Creates one complete C64 instance. On failure, returns NULL and writes a
 * human-readable message to error_text when the supplied buffer is non-empty.
 */
frodo_instance_t *frodo_create(const frodo_config_t *config,
                               char *error_text,
                               size_t error_text_size);

void frodo_destroy(frodo_instance_t *instance);

/* Execute one PAL/NTSC machine frame. Video work may be skipped, but CPU,
 * drive, CIA, SID and audio progression must never be skipped. */
bool frodo_run_frame(frodo_instance_t *instance,
                     const frodo_input_t *input,
                     bool render_video);

/* Views remain valid until the next call that advances or destroys the core. */
bool frodo_get_video_frame(const frodo_instance_t *instance,
                           frodo_video_frame_t *frame);
const frodo_audio_frame_t *frodo_get_audio_frames(const frodo_instance_t *instance,
                                                  size_t *frame_count);

bool frodo_save_snapshot(frodo_instance_t *instance, const char *filename);
bool frodo_load_snapshot(frodo_instance_t *instance, const char *filename);
bool frodo_reset(frodo_instance_t *instance, bool hard);

#ifdef __cplusplus
}
#endif
