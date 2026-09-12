#ifndef TFT_LCD_H
#define TFT_LCD_H
#include <stdint.h>

static inline void spi_lcd_wait_finish(void) {}
static inline void spi_lcd_send(uint16_t *scr) {}
static inline void spi_lcd_send_boarder(uint16_t *scr, int boarder) {}
static inline void spi_lcd_clear(void) {}
static inline void spi_lcd_init(void) {}

#endif
