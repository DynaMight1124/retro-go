#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
// Resolve the directory containing the selected PAK (not its parent).
bool q2_resolve_basedir(const char *pak, char *dest, size_t capacity);
int q2_scale_msec(uint32_t elapsed, float speed, double *remainder);

enum {
    Q2_BUTTON_LEFT, Q2_BUTTON_RIGHT, Q2_BUTTON_UP, Q2_BUTTON_DOWN,
    Q2_BUTTON_START, Q2_BUTTON_SELECT, Q2_BUTTON_A, Q2_BUTTON_B,
    Q2_BUTTON_X, Q2_BUTTON_Y, Q2_BUTTON_L, Q2_BUTTON_R, Q2_BUTTON_COUNT
};
typedef struct {
    uint32_t candidate, stable, since[Q2_BUTTON_COUNT];
} q2_buttons_t;
typedef struct { int held[Q2_BUTTON_COUNT]; } q2_keymap_t;

uint32_t q2_buttons_sample(q2_buttons_t *state, uint32_t raw, uint32_t now_ms);
int q2_button_key(q2_keymap_t *state, unsigned button, int down, int menu);
uint16_t q2_rgb565_be(unsigned char r, unsigned char g, unsigned char b);

typedef struct { uint32_t pressed_ms; bool held, fired; } q2_menu_button_t;
enum { Q2_MENU_NONE, Q2_MENU_QUAKE, Q2_MENU_RETROGO };
int q2_menu_button(q2_menu_button_t *state, bool down, uint32_t now_ms);
