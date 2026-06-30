#ifdef __cplusplus
extern "C" {
#endif

#ifndef RG_MEMORY_H
#define RG_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *mame_alloc_slow(size_t size);
void *mame_alloc_fast(size_t size);
void mame_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif

#ifdef __cplusplus
}
#endif
