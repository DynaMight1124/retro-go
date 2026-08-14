#include "Display.h"

#include <cstring>

Display::Display(C64 *) : pixels(new uint8_t[DISPLAY_X * DISPLAY_Y])
{
    std::memset(pixels, 0, DISPLAY_X * DISPLAY_Y);

    static constexpr uint32_t c64_palette[16] = {
        0x000000, 0xffffff, 0x813338, 0x75cec8,
        0x8e3c97, 0x56ac4d, 0x2e2c9b, 0xedf171,
        0x8e5029, 0x553800, 0xc46c71, 0x4a4a4a,
        0x7b7b7b, 0xa9ff9f, 0x706deb, 0xb2b2b2,
    };
    std::memcpy(palette, c64_palette, sizeof(palette));
}

Display::~Display()
{
    delete[] pixels;
}
