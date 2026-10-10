#include "adapter.h"
#include "port_logic.h"
#include "client/client.h"
#include "client/keys.h"

static q2_keymap_t keymap;
static q2_menu_button_t menu_button;
static uint32_t previous, blocked;
static const uint32_t keys[Q2_BUTTON_COUNT] = {
    RG_KEY_LEFT, RG_KEY_RIGHT, RG_KEY_UP, RG_KEY_DOWN,
    RG_KEY_START, RG_KEY_SELECT, RG_KEY_A, RG_KEY_B,
    RG_KEY_X, RG_KEY_Y, RG_KEY_L, RG_KEY_R
};
extern void ProcessKeyEvent(int key, qboolean down);

void q2_input_release(void)
{
    for (unsigned i = 0; i < Q2_BUTTON_COUNT; ++i) {
        int key = q2_button_key(&keymap, i, false, false);
        if (key) ProcessKeyEvent(key, false);
    }
    menu_button = (q2_menu_button_t){0};
    previous = 0;
    blocked = rg_input_read_gamepad();
}

void q2_input_poll(void)
{
    uint32_t raw = rg_input_read_gamepad();
    blocked &= raw;
    uint32_t mask = raw & ~blocked;
    int menu = q2_menu_button(&menu_button, (mask & RG_KEY_MENU) != 0,
                              (uint32_t)(rg_system_timer() / 1000));
    if ((mask & RG_KEY_OPTION) || menu == Q2_MENU_RETROGO) {
        q2_input_release();
        q2_menu((mask & RG_KEY_OPTION) != 0);
        q2_input_release();
        return;
    }
    if (menu == Q2_MENU_QUAKE) {
        q2_input_release();
        ProcessKeyEvent(K_ESCAPE, true);
        ProcessKeyEvent(K_ESCAPE, false);
        return;
    }
    uint32_t changed = mask ^ previous;
    previous = mask;
    for (unsigned i = 0; i < Q2_BUTTON_COUNT; ++i) {
        if (!(changed & keys[i])) continue;
        bool down = (mask & keys[i]) != 0;
        int key = q2_button_key(&keymap, i, down, cls.key_dest != key_game);
        if (key) ProcessKeyEvent(key, down);
    }
}

// Events are delivered directly on the engine thread, avoiding its unchecked ring.
int QG_GetKey(int *down, int *key) { *down = *key = 0; return 0; }
void QG_GetMouseMove(int *x, int *y) { *x = *y = 0; }
void QG_GetJoyAxes(float *axes) { (void)axes; }

void q2_input_configure(void)
{
    Key_SetBinding(K_UPARROW, "+forward");
    Key_SetBinding(K_DOWNARROW, "+back");
    Key_SetBinding(K_LEFTARROW, "+left");
    Key_SetBinding(K_RIGHTARROW, "+right");
    Key_SetBinding(K_CTRL, "+attack");
    Key_SetBinding(K_SPACE, "+moveup");
    Key_SetBinding('c', "+movedown");
    Key_SetBinding('e', "invuse");
    Key_SetBinding('q', "weapnext");
    Key_SetBinding('a', "+moveleft");
    Key_SetBinding('d', "+moveright");
    Key_SetBinding('i', "invnext");
    Cvar_Set("cl_run", "1");
}
