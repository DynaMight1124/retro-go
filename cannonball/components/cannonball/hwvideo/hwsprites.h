#pragma once

#include <stdint.h>

extern uint32_t *sprites;

void HWSprites_Create();
void HWSprites_Destroy();
void HWSprites_init(const uint8_t*);
void HWSprites_reset();
void HWSprites_set_x_clip(uint8_t);
void HWSprites_swap();
uint8_t HWSprites_read(const uint16_t adr);
void HWSprites_write(const uint16_t adr, const uint16_t data);
void HWSprites_render(const uint8_t);
