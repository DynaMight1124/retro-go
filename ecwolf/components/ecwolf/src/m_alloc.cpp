#include <stdlib.h>
#include <string.h>
#include <rg_system.h>
#include <rg_utils.h>
#include "dobject.h"

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

// Use the platform's ordinary allocator for general engine allocations.
// Retro-Go targets configure malloc/calloc to keep small allocations in
// internal RAM and place larger allocations in PSRAM. Explicit MEM_SLOW and
// MEM_FAST allocations elsewhere remain reserved for data with known needs.

extern void SD_ClearSoundCache(void);

void *M_Malloc(size_t size)
{
	void *block = calloc(1, size + sizeof(size_t));

	// Cached sounds are disposable. Reclaim them before declaring an
	// allocation failure, but do not second-guess the allocator's placement.
	if (block == NULL)
	{
		SD_ClearSoundCache();
		block = calloc(1, size + sizeof(size_t));
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

	void *newBlock = realloc(oldSizeStore, size + sizeof(size_t));
	if (newBlock == NULL)
	{
		// A failed realloc leaves oldSizeStore valid, so it is safe to purge
		// disposable data and retry with the original block.
		SD_ClearSoundCache();
		newBlock = realloc(oldSizeStore, size + sizeof(size_t));
	}

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
