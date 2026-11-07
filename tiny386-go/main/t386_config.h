#pragma once
#include "pc.h"
typedef struct {PCConfig pc; int keycodes[8];} T386Config;
bool t386_config_load(const char *path,T386Config *out,char *error,size_t error_size);
void t386_config_free(T386Config *config);
