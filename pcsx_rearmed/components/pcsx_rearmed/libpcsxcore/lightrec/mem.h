/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_MEM_H__
#define __LIGHTREC_MEM_H__

#ifdef LIGHTREC

#ifdef HW_WUP /* WiiU */
#    define WUP_RWX_MEM_BASE		0x00802000
#    define WUP_RWX_MEM_END		0x01000000
#    define CODE_BUFFER_SIZE_DFT	(WUP_RWX_MEM_END - WUP_RWX_MEM_BASE)
#elif defined(ESP_PLATFORM) && defined(CONFIG_IDF_TARGET_ESP32P4)
/* The P4 stores generated code in executable PSRAM. Use Lightrec's normal
 * cache size: the earlier 128 KiB mapping fills during PSX startup before
 * any blocks are old enough for the cache reaper to discard. */
#    define CODE_BUFFER_SIZE_DFT	(8 * 1024 * 1024)
#elif defined(ESP_PLATFORM)
#    define CODE_BUFFER_SIZE_DFT	(32 * 1024)
#else
#    define CODE_BUFFER_SIZE_DFT	(8 * 1024 * 1024)
#endif

#ifndef CODE_BUFFER_SIZE
#define CODE_BUFFER_SIZE CODE_BUFFER_SIZE_DFT
#endif

extern void *code_buffer;

int lightrec_init_mmap(void);
void lightrec_free_mmap(void);

#else /* if !LIGHTREC */

#define lightrec_init_mmap() -1 /* should not be called */
#define lightrec_free_mmap()

#undef LIGHTREC_CUSTOM_MAP
#define LIGHTREC_CUSTOM_MAP 0

#endif

#endif /* __LIGHTREC_MEM_H__ */
