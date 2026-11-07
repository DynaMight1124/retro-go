#pragma once
#include <stdbool.h>
#include <stdint.h>
int t386_keycode(const char *name);
void t386_input_emit(int key, int down);
void t386_input_init(const int keycodes[8]);
/* Changes release old bindings before installing new ones. */
void t386_input_set_mapping(const int keycodes[8]);
void t386_input_poll(uint32_t buttons);
void t386_input_release_all(void);
bool t386_input_queue_text(const char *text);
enum {T386_MOD_CTRL=1,T386_MOD_ALT=2,T386_MOD_SHIFT=4};
bool t386_input_queue_key(int key,unsigned modifiers);
void t386_input_step(void);

/* START alone: tap on release, hold 700ms to open keyboard. Chords pass through. */
uint32_t t386_input_shortcuts(uint32_t buttons,uint64_t now,bool *open);
bool t386_input_pending(void);
