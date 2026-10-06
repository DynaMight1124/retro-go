#include <ultra64.h>
#include <macros.h>
#include <segments.h>

#define MEMORY_POOL_SIZE 0xAB630

/**
 * Memory pool variable prevents code segments flowing into the memory pool
 * for easier portability.
 * @warning should not really be used.
 */
#ifndef TARGET_PSP
u8 sMemoryPool[MEMORY_POOL_SIZE];
#else
/* The PSP port allocates from gPortMemoryPool (the pool segment symbols are
 * aliased to it in src/port/segments.c); this 686 KB placeholder is never
 * referenced there and a PSP-1000 needs the RAM. */
#endif
