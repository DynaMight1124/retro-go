#pragma once
#include "pc.h"
#include <time.h>
void t386_platform_init(long guest_ram_size);
time_t t386_guest_time(void);
uint64_t t386_guest_timer_us(void);
