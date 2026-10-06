/* Retro-Go buttons translated to a single Mario Kart 64 controller. */
#include <ultra64.h>
#include <rg_input.h>

void controller_rg_init(void)
{
}

void controller_rg_read(OSContPad *pad)
{
    uint32_t keys = rg_input_read_gamepad();
    uint16_t buttons = 0;
    if (keys & RG_KEY_A) buttons |= A_BUTTON;
    if (keys & RG_KEY_B) buttons |= B_BUTTON;
    if (keys & RG_KEY_X) buttons |= Z_TRIG;
    if (keys & RG_KEY_Y) buttons |= U_CBUTTONS;
    if (keys & RG_KEY_L) buttons |= Z_TRIG;
    if (keys & RG_KEY_R) buttons |= R_TRIG;
    if (keys & RG_KEY_SELECT) buttons |= R_CBUTTONS;
    if (keys & RG_KEY_START) buttons |= START_BUTTON;
    if (keys & RG_KEY_UP) buttons |= U_JPAD;
    if (keys & RG_KEY_DOWN) buttons |= D_JPAD;
    if (keys & RG_KEY_LEFT) buttons |= L_JPAD;
    if (keys & RG_KEY_RIGHT) buttons |= R_JPAD;

    int x = !!(keys & RG_KEY_RIGHT) - !!(keys & RG_KEY_LEFT);
    int y = !!(keys & RG_KEY_UP) - !!(keys & RG_KEY_DOWN);
    int magnitude = x && y ? 57 : 80;
    pad->button = buttons;
    pad->stick_x = x * magnitude;
    pad->stick_y = y * magnitude;
    pad->errno = 0;
}
