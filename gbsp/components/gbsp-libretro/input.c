/* Stubbed input.c for ESP32-P4 standalone build.
   update_input() is provided by gpsp_esp.c */
#include "common.h"

bool libretro_supports_bitmasks    = false;
bool libretro_supports_ff_override = false;
bool libretro_ff_enabled           = false;
bool libretro_ff_enabled_prev      = false;

unsigned turbo_period      = 6;
unsigned turbo_pulse_width = 3;
unsigned turbo_a_counter   = 0;
unsigned turbo_b_counter   = 0;

/* Savestate stubs */
bool input_check_savestate(const u8 *src) { (void)src; return true; }
bool input_read_savestate(const u8 *src)  { (void)src; return true; }
unsigned input_write_savestate(u8 *dst)   { (void)dst; return 0; }

void set_fastforward_override(bool ff)    { (void)ff; }
