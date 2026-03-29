#include "gfx.h"
#include "sprites.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <rg_system.h>

int SCREEN_W = 256;
int SCREEN_H = 150;

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_565_BE
#else
#define FB_PIXEL_FORMAT RG_PIXEL_565_LE
#endif

/* Triple buffering */
static rg_surface_t *fb_surfaces[3] = {NULL, NULL, NULL};
static int current_fb_idx = 0;
static rg_surface_t *fb_surface = NULL;

/* Input state */
static uint32_t keys_held = 0;
static uint32_t keys_prev = 0;

/* PICO-8 palette in RGB565 format (LE) */
static const uint16_t pico8_palette[16] = {
    0x0000,  /* 0  black */
    0x194A,  /* 1  dark blue */
    0x792A,  /* 2  purple */
    0x0429,  /* 3  green */
    0xAB49,  /* 4  brown */
    0x5AEB,  /* 5  dark grey */
    0xC618,  /* 6  light grey */
    0xFFDF,  /* 7  white */
    0xF809,  /* 8  red */
    0xFD20,  /* 9  orange */
    0xFFE4,  /* 10 yellow */
    0x07E4,  /* 11 bright green */
    0x2D7F,  /* 12 blue */
    0x83B3,  /* 13 lavender */
    0xFBB5,  /* 14 pink */
    0xFE75,  /* 15 peach */
};

/* Internal palette ready for the current screen format */
static uint16_t local_palette[16];

/* Palette remap table (for palette swapping like PICO-8's pal()) */
static uint8_t pal_remap[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

/* Simple 3x5 digit bitmaps for number display */
static const uint8_t digit_bitmaps[10][5] = {
    {0x7,0x5,0x5,0x5,0x7}, /* 0 */
    {0x2,0x2,0x2,0x2,0x2}, /* 1 */
    {0x7,0x1,0x7,0x4,0x7}, /* 2 */
    {0x7,0x1,0x7,0x1,0x7}, /* 3 */
    {0x5,0x5,0x7,0x1,0x1}, /* 4 */
    {0x7,0x4,0x7,0x1,0x7}, /* 5 */
    {0x7,0x4,0x7,0x5,0x7}, /* 6 */
    {0x7,0x1,0x1,0x1,0x1}, /* 7 */
    {0x7,0x5,0x7,0x5,0x7}, /* 8 */
    {0x7,0x5,0x7,0x1,0x7}, /* 9 */
};

/* 3x5 letter bitmaps for text display (A-Z, plus common punctuation) */
static const uint8_t letter_bitmaps[34][5] = {
    {0x2,0x5,0x7,0x5,0x5}, /* A */
    {0x6,0x5,0x6,0x5,0x6}, /* B */
    {0x3,0x4,0x4,0x4,0x3}, /* C */
    {0x6,0x5,0x5,0x5,0x6}, /* D */
    {0x7,0x4,0x6,0x4,0x7}, /* E */
    {0x7,0x4,0x6,0x4,0x4}, /* F */
    {0x3,0x4,0x5,0x5,0x3}, /* G */
    {0x5,0x5,0x7,0x5,0x5}, /* H */
    {0x7,0x2,0x2,0x2,0x7}, /* I */
    {0x1,0x1,0x1,0x5,0x2}, /* J */
    {0x5,0x5,0x6,0x5,0x5}, /* K */
    {0x4,0x4,0x4,0x4,0x7}, /* L */
    {0x5,0x7,0x7,0x5,0x5}, /* M */
    {0x5,0x7,0x7,0x7,0x5}, /* N */
    {0x2,0x5,0x5,0x5,0x2}, /* O */
    {0x6,0x5,0x6,0x4,0x4}, /* P */
    {0x2,0x5,0x5,0x7,0x3}, /* Q */
    {0x6,0x5,0x6,0x5,0x5}, /* R */
    {0x3,0x4,0x2,0x1,0x6}, /* S */
    {0x7,0x2,0x2,0x2,0x2}, /* T */
    {0x5,0x5,0x5,0x5,0x7}, /* U */
    {0x5,0x5,0x5,0x5,0x2}, /* V */
    {0x5,0x5,0x7,0x7,0x5}, /* W */
    {0x5,0x5,0x2,0x5,0x5}, /* X */
    {0x5,0x5,0x2,0x2,0x2}, /* Y */
    {0x7,0x1,0x2,0x4,0x7}, /* Z */
    {0x0,0x2,0x0,0x2,0x0}, /* : (colon) - index 26 */
    {0x0,0x0,0x0,0x0,0x2}, /* . (period) - index 27 */
    {0x0,0x0,0x7,0x0,0x0}, /* - (dash) - index 28 */
    {0x7,0x1,0x7,0x4,0x7}, /* x - index 29 */
    {0x0,0x0,0x0,0x0,0x0}, /* space - index 30 */
    {0x2,0x2,0x2,0x0,0x2}, /* ! - index 31 */
    {0x0,0x7,0x0,0x7,0x0}, /* = - index 32 */
    {0x0,0x2,0x0,0x2,0x4}, /* ; - index 33 */
};

int gfx_init(void) {
    if (fb_surfaces[0]) return 0;

    int classic = rg_settings_get_number(NS_APP, "ClassicMode", 0);
    if (classic) {
        SCREEN_W = 128;
        SCREEN_H = 128;
    } else {
        SCREEN_W = 256;
        SCREEN_H = 150;
    }

    for (int i = 0; i < 3; i++) {
        fb_surfaces[i] = rg_surface_create(SCREEN_W, SCREEN_H, FB_PIXEL_FORMAT, MEM_FAST);
        if (!fb_surfaces[i]) return -1;
        rg_surface_fill(fb_surfaces[i], NULL, 0);
    }
    
    current_fb_idx = 0;
    fb_surface = fb_surfaces[current_fb_idx];

    /* Set up internal palette */
    for (int i = 0; i < 16; i++) {
        uint16_t color = pico8_palette[i];
        if (FB_PIXEL_FORMAT == RG_PIXEL_565_BE)
            color = (color << 8) | (color >> 8);
        local_palette[i] = color;
    }

    return 0;
}

int gfx_load_sprites(const char* path) {
    (void)path;
    return 0;
}

void gfx_quit(void) {
    for (int i = 0; i < 3; i++) {
        if (fb_surfaces[i]) {
            rg_surface_free(fb_surfaces[i]);
            fb_surfaces[i] = NULL;
        }
    }
    fb_surface = NULL;
}

void gfx_clear(uint8_t color) {
    uint16_t c16 = local_palette[color & 0xF];
    uint16_t *data = (uint16_t *)fb_surface->data;
    int count = SCREEN_W * SCREEN_H;
    while (count--) *data++ = c16;
}

void gfx_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    uint16_t *fb = (uint16_t *)fb_surface->data;
    fb[y * (fb_surface->stride / 2) + x] = local_palette[color & 0xF];
}

/* User noticed "black long boxes". level.c uses gfx_rect for Ice Borders.
   In original ccleste, gfx_rect might have been a fill. Let's make it solid. */
void gfx_rect(int x, int y, int w, int h, uint8_t color) {
    gfx_rectfill(x, y, x + w - 1, y + h - 1, color);
}

void gfx_rectfill(int x0, int y0, int x1, int y1, uint8_t color) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    
    if (x0 > x1 || y0 > y1) return;

    uint16_t *fb = (uint16_t *)fb_surface->data;
    int pitch = fb_surface->stride / 2;
    uint16_t c16 = local_palette[color & 0xF];
    
    for (int y = y0; y <= y1; y++) {
        uint16_t *row = &fb[y * pitch + x0];
        for (int x = x0; x <= x1; x++) {
            *row++ = c16;
        }
    }
}

void gfx_blit(const uint8_t* data, int x, int y, int w, int h, int key_color) {
    uint16_t *fb = (uint16_t *)fb_surface->data;
    int pitch = fb_surface->stride / 2;
    for (int py = 0; py < h; py++) {
        int dy = y + py;
        if (dy < 0 || dy >= SCREEN_H) continue;
        for (int px = 0; px < w; px++) {
            int dx = x + px;
            if (dx < 0 || dx >= SCREEN_W) continue;
            uint8_t c = data[py * w + px];
            if (c != (uint8_t)key_color) fb[dy * pitch + dx] = local_palette[c & 0xF];
        }
    }
}

void gfx_blit_ex(const uint8_t* data, int x, int y, int w, int h,
                 int key_color, bool flip_x, bool flip_y) {
    uint16_t *fb = (uint16_t *)fb_surface->data;
    int pitch = fb_surface->stride / 2;
    for (int py = 0; py < h; py++) {
        int dy = y + py;
        if (dy < 0 || dy >= SCREEN_H) continue;
        int src_py = flip_y ? (h - 1 - py) : py;
        for (int px = 0; px < w; px++) {
            int dx = x + px;
            if (dx < 0 || dx >= SCREEN_W) continue;
            int src_px = flip_x ? (w - 1 - px) : px;
            uint8_t c = data[src_py * w + src_px];
            if (c != (uint8_t)key_color) fb[dy * pitch + dx] = local_palette[c & 0xF];
        }
    }
}

void gfx_spr(int id, int x, int y, bool flip_x, bool flip_y) {
    if (id < 0 || id >= SHEET_COLS * SHEET_ROWS) return;
    uint16_t *fb = (uint16_t *)fb_surface->data;
    int pitch = fb_surface->stride / 2;

    int sx = (id % SHEET_COLS) * SPRITE_SIZE;
    int sy = (id / SHEET_COLS) * SPRITE_SIZE;

    for (int py = 0; py < SPRITE_SIZE; py++) {
        int dy = y + py;
        if (dy < 0 || dy >= SCREEN_H) continue;
        int src_py = flip_y ? (SPRITE_SIZE - 1 - py) : py;

        for (int px = 0; px < SPRITE_SIZE; px++) {
            int dx = x + px;
            if (dx < 0 || dx >= SCREEN_W) continue;
            int src_px = flip_x ? (SPRITE_SIZE - 1 - px) : px;

            uint8_t c = sprite_sheet[(sy + src_py) * SPRITE_SHEET_W + (sx + src_px)];
            if (c != 0) {
                uint8_t color = pal_remap[c & 0xF];
                fb[dy * pitch + dx] = local_palette[color & 0xF];
            }
        }
    }
}

static void draw_digit(uint16_t *fb, int x, int y, int digit, uint8_t color) {
    if (digit < 0 || digit > 9) return;
    int pitch = fb_surface->stride / 2;
    uint16_t c16 = local_palette[color & 0xF];
    for (int row = 0; row < 5; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= SCREEN_H) continue;
        uint8_t bits = digit_bitmaps[digit][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (4 >> col)) {
                int dx = x + col;
                if (dx >= 0 && dx < SCREEN_W)
                    fb[dy * pitch + dx] = c16;
            }
        }
    }
}

static void draw_letter(uint16_t *fb, int x, int y, int index, uint8_t color) {
    if (index < 0 || index > 33) return;
    int pitch = fb_surface->stride / 2;
    uint16_t c16 = local_palette[color & 0xF];
    for (int row = 0; row < 5; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= SCREEN_H) continue;
        uint8_t bits = letter_bitmaps[index][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (4 >> col)) {
                int dx = x + col;
                if (dx >= 0 && dx < SCREEN_W)
                    fb[dy * pitch + dx] = c16;
            }
        }
    }
}

int gfx_print_num(int x, int y, int num, uint8_t color) {
    uint16_t *fb = (uint16_t *)fb_surface->data;
    int start_x = x;
    if (num < 0) {
        gfx_pixel(x, y, color);
        gfx_pixel(x + 1, y, color);
        x += 4;
        num = -num;
    }
    int temp = num;
    int digits = 0;
    do { digits++; temp /= 10; } while (temp > 0);
    int draw_x = x + (digits - 1) * 4;
    do {
        draw_digit(fb, draw_x, y, num % 10, color);
        num /= 10;
        draw_x -= 4;
    } while (num > 0);
    return x + digits * 4 - start_x;
}

int gfx_print(int x, int y, const char* text, uint8_t color) {
    uint16_t *fb = (uint16_t *)fb_surface->data;
    int start_x = x;
    while (*text) {
        char c = *text++;
        int index = -1;
        if (c >= 'A' && c <= 'Z') index = c - 'A';
        else if (c >= 'a' && c <= 'z') index = c - 'a';
        else if (c >= '0' && c <= '9') {
            draw_digit(fb, x, y, c - '0', color);
            x += 4;
            continue;
        }
        else if (c == ':') index = 26;
        else if (c == '.') index = 27;
        else if (c == '-') index = 28;
        else if (c == ' ') { x += 4; continue; }
        else if (c == '!') index = 31;
        else if (c == '=') index = 32;
        else if (c == ';') index = 33;

        if (index >= 0) draw_letter(fb, x, y, index, color);
        x += 4;
    }
    return x - start_x;
}

void gfx_flip(void) {
    rg_display_submit(fb_surface, 0);
    current_fb_idx = (current_fb_idx + 1) % 3;
    fb_surface = fb_surfaces[current_fb_idx];
}

void gfx_pal(uint8_t from, uint8_t to) {
    if (from < 16) pal_remap[from] = to;
}

void gfx_pal_reset(void) {
    for (int i = 0; i < 16; i++) pal_remap[i] = i;
}

void input_update(void) {
    keys_prev = keys_held;
    keys_held = rg_input_read_gamepad();

    if (keys_held & RG_KEY_MENU) {
        rg_gui_game_menu();
        keys_held = 0;
    }
    if (keys_held & RG_KEY_OPTION) {
        rg_gui_options_menu();
        keys_held = 0;
    }
}

bool input_left(void)     { return (keys_held & RG_KEY_LEFT) != 0; }
bool input_right(void)    { return (keys_held & RG_KEY_RIGHT) != 0; }
bool input_up(void)       { return (keys_held & RG_KEY_UP) != 0; }
bool input_down(void)     { return (keys_held & RG_KEY_DOWN) != 0; }
bool input_jump(void)     { return (keys_held & RG_KEY_A) != 0; }
bool input_dash(void)     { return (keys_held & RG_KEY_B) != 0; }
bool input_quit(void)     { return false; }
bool input_restart(void)  { return (keys_held & RG_KEY_START) != 0; }
bool input_teleport(void) { return (keys_held & RG_KEY_OPTION) != 0; }

float p8sin(float x) {
    return -sinf(x * 6.28318530718f);
}
