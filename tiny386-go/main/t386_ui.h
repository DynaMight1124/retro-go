#pragma once
#include "rg_system.h"
#include "adlib.h"
#include "t386_runtime.h"
void t386_ui_init(AdlibState *chip);
void t386_ui_set_video(T386Video *video);
void t386_ui_options(rg_gui_option_t *dest);
bool t386_ui_unsupported_state(const char *path);

void t386_ui_set_keyboard_handler(void (*handler)(void));
void t386_ui_set_special_handler(void (*handler)(int key,unsigned modifiers));
/* Persistent cursor/case owned by the session; -1 closes, otherwise ASCII key. */
int t386_ui_keyboard_pick(int *cursor,bool *shift);
