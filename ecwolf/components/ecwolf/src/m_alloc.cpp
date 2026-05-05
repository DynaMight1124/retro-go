#include <stdlib.h>
#include <string.h>
#include <rg_system.h>
#include <rg_utils.h>
#include "dobject.h"

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

// Use Retro-Go's memory allocation system. 
// We use MEM_SLOW (PSRAM) for most allocations to save internal RAM.

extern void SD_ClearSoundCache(void);

void *M_Malloc(size_t size)
{
	void *block = rg_alloc(size + sizeof(size_t), MEM_SLOW);

    // If allocation failed, or if a "large" block (e.g. >4KB) ended up in Internal RAM
    // (identified by address < 0x3f800000 on ESP32), purge sound cache and retry.
    if (block == NULL || ((uint32_t)block < 0x3f800000 && size > 4096))
    {
        SD_ClearSoundCache();
        if (block && (uint32_t)block < 0x3f800000) {
            free(block); // Free the internal RAM block before retrying for PSRAM
            block = NULL;
        }
        if (block == NULL) {
            block = rg_alloc(size + sizeof(size_t), MEM_SLOW);
        }
    }

	if (block == NULL)
		I_FatalError("Could not malloc %zu bytes (Free PSRAM: %d KB)", size, (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

	size_t *sizeStore = (size_t *) block;
	*sizeStore = size;
	block = sizeStore + 1;

	GC::AllocBytes += size;
	return block;
}

void *M_Realloc(void *memblock, size_t size)
{
	if (memblock == NULL)
		return M_Malloc(size);

	size_t *oldSizeStore = ((size_t *) memblock) - 1;
	size_t oldSize = *oldSizeStore;

#ifdef ESP_PLATFORM
	void *newBlock = heap_caps_realloc(oldSizeStore, size + sizeof(size_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if (!newBlock || ((uint32_t)newBlock < 0x3f800000 && size > 4096)) {
        SD_ClearSoundCache();
        // If it was a spill to internal RAM, we can't easily "undo" the realloc without losing data,
        // but we can try to realloc again now that space is cleared.
		newBlock = heap_caps_realloc(newBlock ? newBlock : oldSizeStore, size + sizeof(size_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	}
#else
	void *newBlock = realloc(oldSizeStore, size + sizeof(size_t));
#endif

	if (newBlock == NULL)
		I_FatalError("Could not realloc %zu bytes", size);

	size_t *newSizeStore = (size_t *) newBlock;
	*newSizeStore = size;
	void *actualNewBlock = newSizeStore + 1;

	GC::AllocBytes -= oldSize;
	GC::AllocBytes += size;

	return actualNewBlock;
}

void M_Free(void *block)
{
	if (block != NULL)
	{
		size_t *sizeStore = ((size_t *) block) - 1;
		GC::AllocBytes -= *sizeStore;
		free(sizeStore);
	}
}

size_t M_Size(void *block)
{
    if (block == NULL) return 0;
    return *(((size_t *)block) - 1);
}

// Debug versions just map to normal versions for now
void *M_Malloc_Dbg(size_t size, const char *file, int lineno)
{
    return M_Malloc(size);
}

void *M_Realloc_Dbg(void *memblock, size_t size, const char *file, int lineno)
{
    return M_Realloc(memblock, size);
}

// Global operator new/delete overrides
void* operator new(size_t size) { return M_Malloc(size); }
void* operator new[](size_t size) { return M_Malloc(size); }
void operator delete(void* p) noexcept { M_Free(p); }
void operator delete[](void* p) noexcept { M_Free(p); }
void operator delete(void* p, size_t size) noexcept { M_Free(p); }
void operator delete[](void* p, size_t size) noexcept { M_Free(p); }
