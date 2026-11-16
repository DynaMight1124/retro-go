#include "car.h"
#include <string.h>

const int Car_COLORS[10] = {
    0xC8302C, 0x2C5FC8, 0xF0D840, 0x2CA05A, 0xE8E8E8,
    0x2E2E34, 0xFF8FD0, 0x40C0D0, 0xE87A20, 0x8A2BE2
};

void Car_init(Car* c) {
    if (!c) return;
    memset(c, 0, sizeof(Car));
    c->color2 = 0xE8E8E8;
    c->hp = 1000;
}
