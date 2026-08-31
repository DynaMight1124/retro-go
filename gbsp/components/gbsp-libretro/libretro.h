/* Minimal libretro compatibility shim for standalone gpSP on ESP32-P4 */
#ifndef LIBRETRO_H
#define LIBRETRO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Pixel format */
#define RETRO_PIXEL_FORMAT_RGB565 2

/* VFS constants (unused but referenced) */
#define RETRO_VFS_FILE_ACCESS_READ 1
#define RETRO_VFS_FILE_ACCESS_HINT_NONE 0

/* Joypad button IDs (for input.h mapping table) */
#define RETRO_DEVICE_JOYPAD       1
#define RETRO_DEVICE_ID_JOYPAD_B      0
#define RETRO_DEVICE_ID_JOYPAD_Y      1
#define RETRO_DEVICE_ID_JOYPAD_SELECT 2
#define RETRO_DEVICE_ID_JOYPAD_START  3
#define RETRO_DEVICE_ID_JOYPAD_UP     4
#define RETRO_DEVICE_ID_JOYPAD_DOWN   5
#define RETRO_DEVICE_ID_JOYPAD_LEFT   6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT  7
#define RETRO_DEVICE_ID_JOYPAD_A      8
#define RETRO_DEVICE_ID_JOYPAD_X      9
#define RETRO_DEVICE_ID_JOYPAD_L     10
#define RETRO_DEVICE_ID_JOYPAD_R     11
#define RETRO_DEVICE_ID_JOYPAD_L2    12
#define RETRO_DEVICE_ID_JOYPAD_R2    13
#define RETRO_DEVICE_ID_JOYPAD_MASK  256

/* Rumble */
#define RETRO_RUMBLE_STRONG 0
#define RETRO_RUMBLE_WEAK   1

/* Log levels */
#define RETRO_LOG_DEBUG 0
#define RETRO_LOG_INFO  1
#define RETRO_LOG_WARN  2
#define RETRO_LOG_ERROR 3

/* Network packet (unused, but referenced by serial_proto.c) */
#define RETRO_NETPACKET_BROADCAST 0

/* Game info struct - minimal */
struct retro_game_info {
    const char *path;
    const void *data;
    size_t size;
    const char *meta;
};

/* Callback typedefs (stubbed) */
typedef int16_t (*retro_input_state_t)(unsigned, unsigned, unsigned, unsigned);
typedef void (*retro_set_rumble_state_t)(unsigned, unsigned, uint16_t);

/* Network stubs */
static inline void netpacket_send(uint16_t client_id, const void *buf, size_t len) {
    (void)client_id; (void)buf; (void)len;
}
static inline void netpacket_poll_receive(void) { }

#endif /* LIBRETRO_H */
