#pragma once

#include <cstdint>

constexpr unsigned DISPLAY_X = 0x180;
constexpr unsigned DISPLAY_Y = 0x110;

class C64;

class Display {
public:
    explicit Display(C64 *c64);
    ~Display();

    uint8_t *BitmapBase() { return pixels; }
    int BitmapXMod() const { return DISPLAY_X; }

    const uint8_t *Pixels() const { return pixels; }
    const uint32_t *Palette() const { return palette; }

private:
    uint8_t *pixels;
    uint32_t palette[16];
};
