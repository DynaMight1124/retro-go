#ifndef RG_MALLOC_H
#define RG_MALLOC_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Standard library malloc usually uses internal RAM on ESP32-P4.
   MAME requires much more memory than available in internal RAM.
   We redirect all large allocations to PSRAM. 
   
   WE DO NOT USE rg_alloc here because it contains logging (RG_LOGI) 
   which calls vprintf, which might call malloc, leading to 
   infinite recursion and stack overflow/panic.
*/

static inline void *mame_malloc(size_t size) {
#ifdef ESP_PLATFORM
    return heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return calloc(1, size);
#endif
}

static inline void mame_free(void *ptr) {
#ifdef ESP_PLATFORM
    heap_caps_free(ptr);
#else
    free(ptr);
#endif
}

static inline void *mame_realloc(void *ptr, size_t size) {
#ifdef ESP_PLATFORM
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return realloc(ptr, size);
#endif
}

static inline void *mame_calloc(size_t nmemb, size_t size) {
#ifdef ESP_PLATFORM
    return heap_caps_calloc(nmemb, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return calloc(nmemb, size);
#endif
}

static inline void *mame_malloc_fast(size_t size) {
#ifdef ESP_PLATFORM
    return heap_caps_calloc(1, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    return calloc(1, size);
#endif
}

static inline void mame_free_fast(void *ptr) {
#ifdef ESP_PLATFORM
    heap_caps_free(ptr);
#else
    free(ptr);
#endif
}

#ifdef __cplusplus
}
#endif

/* Redefine standard functions to our PSRAM wrappers */
#ifndef RG_NO_MALLOC_REDEFINE
#define malloc(s) mame_malloc(s)
#define free(p) mame_free(p)
#define realloc(p, s) mame_realloc(p, s)
#define calloc(n, s) mame_calloc(n, s)
#endif

#endif
