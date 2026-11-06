#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Only while CPU execution and scanline rendering are stopped. */
uint8_t *gamepak_borrow_block(void);
bool gamepak_return_block(void);
void gamepak_menu_begin(void);
void gamepak_menu_end(void);
#ifdef __cplusplus
}
#endif
