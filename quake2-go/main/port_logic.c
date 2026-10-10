#include "port_logic.h"
#include <string.h>
#include <ctype.h>
int q2_scale_msec(uint32_t elapsed, float speed, double *remainder)
{
    double scaled = elapsed * (double)speed + *remainder;
    int whole = (int)scaled;
    *remainder = scaled - whole;
    return whole;
}
bool q2_resolve_basedir(const char *pak, char *dest, size_t capacity)
{
    if (!pak || !dest || !capacity || pak[0] != '/') return false;
    const char *file = strrchr(pak, '/');
    if (!file || !file[1]) return false;
    size_t n = strlen(file);
    if (n < 5 || tolower((unsigned char)file[n-4]) != '.' ||
        tolower((unsigned char)file[n-3]) != 'p' ||
        tolower((unsigned char)file[n-2]) != 'a' ||
        tolower((unsigned char)file[n-1]) != 'k') return false;
    for (const char *p = pak; *p; ++p) {
        if (*p == '\\') return false;
        if (*p == '/' && (p[1] == '/' ||
            (p[1] == '.' && (p[2] == '/' || !p[2] ||
             (p[2] == '.' && (p[3] == '/' || !p[3])))))) return false;
    }
    // The selected PAK owns the complete content tree, with or without baseq2.
    size_t length = (size_t)(file - pak);
    if (!length || length >= capacity) return false;
    memcpy(dest, pak, length);
    dest[length] = 0;
    return true;
}
#include "client/keys.h"

uint32_t q2_buttons_sample(q2_buttons_t *state, uint32_t raw, uint32_t now_ms)
{
    uint32_t changed = 0;
    for (unsigned i = 0; i < Q2_BUTTON_COUNT; ++i) {
        uint32_t bit = 1u << i;
        if ((raw ^ state->candidate) & bit) {
            state->candidate ^= bit;
            state->since[i] = now_ms;
        } else if (((raw ^ state->stable) & bit) &&
                   (uint32_t)(now_ms - state->since[i]) >= 20) {
            state->stable ^= bit;
            changed |= bit;
        }
    }
    return changed;
}
int q2_button_key(q2_keymap_t *state, unsigned button, int down, int menu)
{
    static const int game_keys[Q2_BUTTON_COUNT] = {
        K_LEFTARROW, K_RIGHTARROW, K_UPARROW, K_DOWNARROW,
        'i', 'q', K_CTRL, K_SPACE, 'c', 'e', 'a', 'd'
    };
    if (button >= Q2_BUTTON_COUNT) return 0;
    if (!down) {
        int key = state->held[button];
        state->held[button] = 0;
        return key;
    }
    int key = game_keys[button];
    if (menu && button == Q2_BUTTON_A) key = K_ENTER;
    if (menu && button == Q2_BUTTON_B) key = K_ESCAPE;
    state->held[button] = key;
    return key;
}
uint16_t q2_rgb565_be(unsigned char r, unsigned char g, unsigned char b)
{
    uint16_t rgb = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    return (uint16_t)((rgb >> 8) | (rgb << 8));
}

int q2_menu_button(q2_menu_button_t *state, bool down, uint32_t now_ms)
{
    if (!state->held) {
        if (down) {
            state->held = true;
            state->fired = false;
            state->pressed_ms = now_ms;
        }
        return Q2_MENU_NONE;
    }
    int action = Q2_MENU_NONE;
    if (!state->fired && (uint32_t)(now_ms - state->pressed_ms) >= 500) {
        state->fired = true;
        action = Q2_MENU_RETROGO;
    }
    if (!down) {
        if (!state->fired) action = Q2_MENU_QUAKE;
        state->held = false;
    }
    return action;
}
