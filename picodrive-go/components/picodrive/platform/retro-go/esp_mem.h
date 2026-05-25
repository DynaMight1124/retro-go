#ifndef ESP_MEM_H
#define ESP_MEM_H

#ifdef ESP_PLATFORM
#include <esp_attr.h>
#else
#define EXT_RAM_BSS_ATTR
#endif

// Move huge arrays to PSRAM to free up Internal RAM.
#define JumpTable EXT_RAM_BSS_ATTR JumpTable
#define ym_tl_tab EXT_RAM_BSS_ATTR ym_tl_tab
#define ym_tl_tab2 EXT_RAM_BSS_ATTR ym_tl_tab2
#define lfo_pm_table EXT_RAM_BSS_ATTR lfo_pm_table
#define PicoMem EXT_RAM_BSS_ATTR PicoMem
#define Pico EXT_RAM_BSS_ATTR Pico
#define PicoDraw2FB_ EXT_RAM_BSS_ATTR PicoDraw2FB_
#define HighCache2A EXT_RAM_BSS_ATTR HighCache2A
#define HighCache2B EXT_RAM_BSS_ATTR HighCache2B
#define PsndBuffer EXT_RAM_BSS_ATTR PsndBuffer
#define HighLnSpr EXT_RAM_BSS_ATTR HighLnSpr
#define rom_buffer EXT_RAM_BSS_ATTR rom_buffer

#endif
