#pragma once
#include <stdint.h>

// Presentation only: core pixels and save-state data remain monochrome.
enum { VMU_LCD_PALETTE_COUNT = 5, VMU_LCD_GRID_COUNT = 4, VMU_LCD_CELL = 4 };
struct VmuLcdGeometry { int width, height; };

inline int vmu_lcd_clamp_palette(int value)
{
    return value >= 0 && value < VMU_LCD_PALETTE_COUNT ? value : 0;
}
inline int vmu_lcd_clamp_grid(int value)
{
    return value >= 0 && value < VMU_LCD_GRID_COUNT ? value : 0;
}
inline int vmu_lcd_legacy_grid(int value)
{
    return value == 1 ? 2 : (value == 2 ? 3 : 0);
}
inline uint16_t vmu_lcd_rgb(unsigned rgb)
{
    return (uint16_t)(((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 31));
}
inline uint16_t vmu_lcd_mix(uint16_t from, uint16_t to, unsigned percent)
{
    unsigned r = (((from >> 11) & 31) * (100 - percent) + ((to >> 11) & 31) * percent + 50) / 100;
    unsigned g = (((from >> 5) & 63) * (100 - percent) + ((to >> 5) & 63) * percent + 50) / 100;
    unsigned b = ((from & 31) * (100 - percent) + (to & 31) * percent + 50) / 100;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

// Output must hold (48*4)*(nativeHeight*4) bytes. Each surface owns its palette.
// The caller supplies its real stride; native mode writes only 48 bytes per row.
inline VmuLcdGeometry vmu_lcd_render(const uint16_t *source, int nativeHeight,
    uint8_t *pixels, int stride, uint16_t *palette, int paletteChoice, int gridChoice)
{
    static const unsigned colors[VMU_LCD_PALETTE_COUNT][2] = {
        {0xFFFFFF, 0x000000}, // Original
        {0x83B083, 0x133B25}, // VMU Green
        {0xB4B87D, 0x343E19}, // Olive LCD
        {0x85B2C2, 0x202466}, // Blue Ink, like the photographed segments
        {0xB6BDB7, 0x28302E}, // Gray
    };
    paletteChoice = vmu_lcd_clamp_palette(paletteChoice);
    gridChoice = vmu_lcd_clamp_grid(gridChoice);
    palette[0] = vmu_lcd_rgb(colors[paletteChoice][0]);
    palette[1] = vmu_lcd_rgb(colors[paletteChoice][1]);
    // Medium is the original Subtle. High is the rounded midpoint between
    // that and old Strong (5/35 -> 12/100); Low uses roughly half the contrast.
    static const unsigned backgroundGap[] = {0, 3, 5, 9};
    static const unsigned inkGap[] = {0, 18, 35, 68};
    palette[2] = vmu_lcd_mix(palette[0], palette[1], backgroundGap[gridChoice]);
    palette[3] = vmu_lcd_mix(palette[1], palette[0], inkGap[gridChoice]);
    int cell = gridChoice ? VMU_LCD_CELL : 1;
    for (int y = 0; y < nativeHeight; ++y) {
        for (int x = 0; x < 48; ++x) {
            uint8_t ink = source[y * 48 + x] == 0 ? 1 : 0;
            for (int sy = 0; sy < cell; ++sy) {
                uint8_t *out = pixels + (y * cell + sy) * stride + x * cell;
                for (int sx = 0; sx < cell; ++sx) {
                    // The icon strip represents continuous etched segments.
                    bool gap = gridChoice && y < 32 && (sx == cell - 1 || sy == cell - 1);
                    out[sx] = gap ? (uint8_t)(ink ? 3 : 2) : ink;
                }
            }
        }
    }
    return {48 * cell, nativeHeight * cell};
}
