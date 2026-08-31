// gpSP memory arrays, split by access temperature.
#include "common.h"

#ifdef ESP_PLATFORM
#include "esp_attr.h"

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
// TODO : find a way to put memory_map_read in IRAM if double frame buffering is disabled
EXT_RAM_BSS_ATTR  u8 *memory_map_read[8 * 1024];   /* 32 KB - ROM page table, hit every ROM read */
u16 oam_ram[512];                /*  1 KB */
u16 palette_ram[512];            /*  1 KB */
u16 palette_ram_converted[512];  /*  1 KB - color LUT, read per pixel */
EXT_RAM_BSS_ATTR u8 ewram[1024 * 256 * 2];        /* 512 KB - stays in PSRAM */
u8 iwram[1024 * 32 * 2];         /* 64 KB - hottest RAM (code/stack), double for SMC sentinel */
EXT_RAM_BSS_ATTR u8 vram[1024 * 96];              /* 96 KB - PSRAM (won't fit internal: ~79 KB over) */
u16 io_registers[512];           /*  1 KB - timers/DMA/video/IRQ */
#else
EXT_RAM_BSS_ATTR  u8 *memory_map_read[8 * 1024];   /* 32 KB - ROM page table, hit every ROM read */
u16 oam_ram[512];                /*  1 KB */
u16 palette_ram[512];            /*  1 KB */
u16 palette_ram_converted[512];  /*  1 KB - color LUT, read per pixel */
EXT_RAM_BSS_ATTR u8 ewram[1024 * 256 * 2];        /* 512 KB - stays in PSRAM */
EXT_RAM_BSS_ATTR u8 iwram[1024 * 32 * 2];         /* 64 KB - hottest RAM (code/stack), double for SMC sentinel */
EXT_RAM_BSS_ATTR u8 vram[1024 * 96];              /* 96 KB - PSRAM (won't fit internal: ~79 KB over) */
u16 io_registers[512];           /*  1 KB - timers/DMA/video/IRQ */
#endif

/* From gba_memory.c */
EXT_RAM_BSS_ATTR u8 gamepak_backup[1024 * 128];

#ifdef HAVE_DYNAREC
/* Memory handler dispatch tables for dynarec (referenced by emit.h) */
/* tmemld[11][16]: load handlers, tmemst[4][16]: store handlers */
/* thnjal[15*16]: thumb handler jump table */
u32 tmemld[11][16];
u32 tmemst[4][16];
u32 thnjal[15 * 16];
#endif

#endif
