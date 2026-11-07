#pragma once
#include <stdint.h>
uint64_t t386_host_time_us(void);
void t386_clock_init(void);
uint64_t t386_clock_now_us(void);
void t386_clock_pause(void);
void t386_clock_resume(void);
void t386_clock_set_speed(float speed);
