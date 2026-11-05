#ifndef GBSP_CONTROLS_H
#define GBSP_CONTROLS_H

#include <stdint.h>
#include <rg_input.h>

/* Published by the main loop before menu handling or core execution. */
static uint32_t gp_input_keys;

/* GBA P1 mask: A B Select Start Right Left Up Down R L. */
static inline uint16_t gbsp_gba_buttons(uint32_t keys)
{
    uint16_t buttons = 0;
    if (keys & RG_KEY_A) buttons |= 0x001;
    if (keys & RG_KEY_B) buttons |= 0x002;
    if (keys & RG_KEY_SELECT) buttons |= 0x004;
    if (keys & RG_KEY_START) buttons |= 0x008;
    if (keys & RG_KEY_RIGHT) buttons |= 0x010;
    if (keys & RG_KEY_LEFT) buttons |= 0x020;
    if (keys & RG_KEY_UP) buttons |= 0x040;
    if (keys & RG_KEY_DOWN) buttons |= 0x080;
    if (keys & (RG_KEY_R | RG_KEY_Y)) buttons |= 0x100;
    if (keys & (RG_KEY_L | RG_KEY_X)) buttons |= 0x200;
    return buttons;
}

#if !defined(RISCV_ARCH)
/* The direct-core input callback uses libretro's mask, not GBA bit positions. */
static int16_t input_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port; (void)device; (void)index; (void)id;
    const uint16_t buttons = gbsp_gba_buttons(gp_input_keys);
    int16_t mask = 0;
    if (buttons & 0x001) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_A;
    if (buttons & 0x002) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_B;
    if (buttons & 0x004) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_SELECT;
    if (buttons & 0x008) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
    if (buttons & 0x010) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (buttons & 0x020) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (buttons & 0x040) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_UP;
    if (buttons & 0x080) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (buttons & 0x100) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_R;
    if (buttons & 0x200) mask |= 1 << RETRO_DEVICE_ID_JOYPAD_L;
    return mask;
}
#endif

#endif
