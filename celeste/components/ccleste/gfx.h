/*
 * gfx.h - Minimal Graphics API for 256x150 8-bit display
 *
 * Platform-agnostic interface. Implement gfx_sdl.c for desktop,
 * or gfx_esp32.c for ESP32 target.
 */

#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include <stdbool.h>

extern int SCREEN_W;
extern int SCREEN_H;

/* Sprite sheet: 128x64 pixels, 16x8 grid of 8x8 sprites */
#define SPRITE_SIZE 8
#define SHEET_COLS  16
#define SHEET_ROWS  8

/* Initialize the graphics system. Returns 0 on success. */
int gfx_init(void);

/* Shutdown and cleanup. */
void gfx_quit(void);

/* Clear screen to a single color index. */
void gfx_clear(uint8_t color);

/* Draw a single pixel. */
void gfx_pixel(int x, int y, uint8_t color);

/* Draw a filled rectangle. */
void gfx_rect(int x, int y, int w, int h, uint8_t color);

/* Blit a sprite. key_color is transparent (-1 for none). */
void gfx_blit(const uint8_t* data, int x, int y, int w, int h, int key_color);

/* Blit a sprite with flip options. */
void gfx_blit_ex(const uint8_t* data, int x, int y, int w, int h,
                 int key_color, bool flip_x, bool flip_y);

/* Draw sprite from sheet by ID (0-127). Transparent color 0. */
void gfx_spr(int id, int x, int y, bool flip_x, bool flip_y);

/* Present the frame buffer and return time blocked on display ownership. */
int gfx_flip(bool *display_late);
void gfx_redraw(void);

/* Save the last complete frame. */
bool gfx_screenshot(const char *filename, int width, int height);

/* Draw a number at position. Returns width in pixels. */
int gfx_print_num(int x, int y, int num, uint8_t color);

/* Draw text string at position. Returns width in pixels. */
int gfx_print(int x, int y, const char* text, uint8_t color);

/* Draw a filled rectangle with outline (like PICO-8 rectfill) */
void gfx_rectfill(int x0, int y0, int x1, int y1, uint8_t color);

/* Input functions - returns true if key is currently held */
bool input_left(void);
bool input_right(void);
bool input_up(void);
bool input_down(void);
bool input_jump(void);
bool input_dash(void);
bool input_quit(void);
bool input_restart(void);   /* R key - restart game */
bool input_teleport(void);  /* T key - debug teleport */

/* Process input events. Returns true when a Retro-Go menu was opened. */
bool input_update(void);

/* Synchronize held/edge state after a reset or state load. */
void input_sync(void);

/* PICO-8 style sin (input 0-1 maps to full cycle, output -1 to 1) */
float p8sin(float x);

/* Palette swapping (like PICO-8 pal()) */
void gfx_pal(uint8_t from, uint8_t to);  /* Remap color 'from' to 'to' */
void gfx_pal_reset(void);                 /* Reset all palette mappings */

#endif /* GFX_H */
