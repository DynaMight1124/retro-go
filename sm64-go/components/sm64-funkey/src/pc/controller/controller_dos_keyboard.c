#ifdef TARGET_DOS

#include <stdbool.h>
#include <ultra64.h>

#include <stdio.h>
#include <allegro.h>

#include "controller_api.h"
#include "../configfile.h"
#include "../common.h"

#include "controller_dos_keyboard.h"

static int mapping_length = 13;
static int keyboard_mapping[13][2];

static void set_keyboard_mapping(int index, int mask, int scancode) {
    keyboard_mapping[index][0] = scancode;
    keyboard_mapping[index][1] = mask;
}

static void keyboard_init(void) {
    install_keyboard();

    int i = 0;
    set_keyboard_mapping(i++, 0x80000,      configKeyStickUp);
    set_keyboard_mapping(i++, 0x10000,      configKeyStickLeft);
    set_keyboard_mapping(i++, 0x40000,      configKeyStickDown);
    set_keyboard_mapping(i++, 0x20000,      configKeyStickRight);
    set_keyboard_mapping(i++, A_BUTTON,     configKeyA);
    set_keyboard_mapping(i++, B_BUTTON,     configKeyB);
    set_keyboard_mapping(i++, Z_TRIG,       configKeyZ);
    set_keyboard_mapping(i++, U_CBUTTONS,   configKeyCUp);
    set_keyboard_mapping(i++, L_CBUTTONS,   configKeyCLeft);
    set_keyboard_mapping(i++, D_CBUTTONS,   configKeyCDown);
    set_keyboard_mapping(i++, R_CBUTTONS,   configKeyCRight);
    set_keyboard_mapping(i++, R_TRIG,       configKeyR);
    set_keyboard_mapping(i++, START_BUTTON, configKeyStart);
}

static void keyboard_read(OSContPad *pad) {
    if (keyboard_needs_poll())
        poll_keyboard();

    for (int i = 0; i < mapping_length; i++) {
        int scan_code = keyboard_mapping[i][0];
        int mapping = keyboard_mapping[i][1];

        if (key[scan_code & 0x7F]) {
            if (mapping == 0x10000)
                pad->stick_x = -128;
            else if (mapping  == 0x20000)
                pad->stick_x = 127;
            else if (mapping  == 0x40000)
                pad->stick_y = -128;
            else if (mapping == 0x80000)
                pad->stick_y = 127;
            else
                pad-> button |= mapping;
        }
    }

    // check for ALT+F4
    if ((key[119] || key[120]) && key[50])
        game_exit();
}


struct ControllerAPI controller_dos_keyboard = {
    keyboard_init,
    keyboard_read
};

#endif
