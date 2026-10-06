#pragma once
#include <stdbool.h>
#include <stdint.h>
bool mk64_menu_cache_restore(uintptr_t source, unsigned width, unsigned height, unsigned alpha, void *output);
void mk64_menu_cache_store(uintptr_t source, unsigned width, unsigned height, unsigned alpha, const void *pixels);
