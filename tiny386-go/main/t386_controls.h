#pragma once
#include <stdbool.h>
void t386_controls_init(const char *game_path,const int defaults[8]);
int t386_controls_get(unsigned button);
bool t386_controls_set(unsigned button,const char *key_name);
void t386_controls_reset(void);
