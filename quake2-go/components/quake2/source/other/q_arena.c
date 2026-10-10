// q_arena.c -- first-fit block allocator over a single contiguous arena.
//
// Author: Alejandro Villegas Alonso
//         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
//
// The arena is carved into one region for the model hunk pool (Q_Arena_Carve,
// done once at startup) plus a variable region managed by this allocator that
// backs the Z zone, image pixels, the surface cache and the software
// framebuffers. Freed blocks are coalesced, so long-lived state does not get
// fragmented by the transients that in the unported code lived in the same
// heap.

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include <rg_system.h>

#include "q_arena.h"

// Block header, exactly 64 bytes, so that when the arena base plus the carved
// region are 64-byte aligned, every payload is 64-byte aligned and every block
// size stays a multiple of 64.
typedef struct arena_blk_s
{
	int		size;		// total bytes of this block including header
	int		inuse;		// 0 = free, 1 = used
	int		subsize;	// requested payload bytes (stats only)
	struct arena_blk_s	*prev, *next;	// linked list, ordered by address
	char	pad[40];	// pad to 64 bytes
} arena_blk_t;

#define ARENA_HDR	((int)sizeof (arena_blk_t))	// 64
#define MIN_ARENA	(8*1024*1024)

static const char *TAG = "ARENA";

static unsigned char	*arena_base;	// start of the allocatable region
static int		arena_total;		// total available bytes (after carve)
static int		arena_actual;		// bytes allocated from PSRAM (after alignment)
static int		arena_carved;		// carve cursor
static int		arena_used;			// bytes inside used blocks (incl headers)
static int		arena_blocks;		// number of blocks currently linked
static arena_blk_t	arena_head;		// sentinel, always inuse
static int		arena_ready;

static void blk_unlink (arena_blk_t *b)
{
	b->prev->next = b->next;
	b->next->prev = b->prev;
}

// Callers only link after `next` in address order and keep the list sorted; we
// only ever insert a block immediately after `at`.
static void blk_link_after (arena_blk_t *at, arena_blk_t *b)
{
	b->next = at->next;
	b->prev = at;
	at->next->prev = b;
	at->next = b;
}

// Reserve from PSRAM, trying smaller sizes if the full requested amount is
// not contiguous (the heap is ~24 MB after boot; 18 MB is typical).
int Q_Arena_Init (int want)
{
	int sizes[] = { want, want - 2*1024*1024, want - 4*1024*1024,
					14*1024*1024, 12*1024*1024, 10*1024*1024, 8*1024*1024 };
	int i, n = sizeof (sizes) / sizeof (sizes[0]);
	void *m = NULL;
	int actual = 0;

	for (i = 0 ; i < n ; i++)
	{
		if (sizes[i] < MIN_ARENA)
			continue;
		m = rg_alloc (sizes[i], MEM_SLOW | MEM_NOPANIC);
		if (m)
		{
			actual = sizes[i];
			break;
		}
		ESP_LOGI (TAG, "heap_caps_malloc %d failed", sizes[i]);
	}

	if (!m)
	{
		ESP_LOGE (TAG, "arena reservation FAILED");
		return 0;
	}

	uintptr_t raw = (uintptr_t)m;
	unsigned char *aligned = (unsigned char *)((raw + 63) & ~(uintptr_t)63);
	int align_off = (int)(aligned - (unsigned char *)m);
	int avail = actual - align_off;

	arena_base = aligned;
	arena_total = avail;
	arena_actual = actual;
	arena_carved = 0;
	arena_used = 0;
	arena_blocks = 0;
	arena_ready = 1;

	arena_head.size = 0;
	arena_head.inuse = 1;
	arena_head.subsize = 0;
	arena_head.prev = &arena_head;
	arena_head.next = &arena_head;

	ESP_LOGI (TAG, "reserved %d bytes, allocatable %d at %p (aligned +%d)",
		actual, avail, arena_base, align_off);

	return actual;
}

int Q_Arena_Reserved (void)
{
	return arena_actual;
}

// Reserve a fixed sub-region from the front of the arena (the model hunk pool).
// Must be called before the first allocation.
void *Q_Arena_Carve (int size)
{
	size = (size + 63) & ~63;

	if (!arena_ready || arena_carved + size > arena_total)
	{
		ESP_LOGE (TAG, "carve %d failed (carved=%d total=%d)", size, arena_carved, arena_total);
		return NULL;
	}

	void *p = arena_base + arena_carved;
	arena_carved += size;
	return p;
}

static void arena_ensure (void)
{
	if (arena_blocks == 0 && arena_carved < arena_total)
	{
		arena_blk_t *b = (arena_blk_t *)(arena_base + arena_carved);
		b->size = arena_total - arena_carved;
		b->inuse = 0;
		b->subsize = 0;
		b->prev = &arena_head;
		b->next = &arena_head;
		arena_head.next = b;
		arena_head.prev = b;
		arena_blocks = 1;
	}
}

void *Q_Arena_Alloc (int size)
{
	if (!arena_ready)
		return NULL;

	arena_ensure ();

	int need = ARENA_HDR + ((size + 63) & ~63);
	arena_blk_t *b;

	for (b = arena_head.next ; b != &arena_head ; b = b->next)
	{
		if (b->inuse || b->size < need)
			continue;

		// split off the tail if it is big enough to form a new block
		int rem = b->size - need;
		if (rem >= ARENA_HDR + 128)
		{
			arena_blk_t *nb = (arena_blk_t *)((unsigned char *)b + need);
			nb->size = rem;
			nb->inuse = 0;
			nb->subsize = 0;
			blk_link_after (b, nb);
			arena_blocks++;
			b->size = need;
		}

		b->inuse = 1;
		b->subsize = size;
		arena_used += b->size;
		return (void *)((unsigned char *)b + ARENA_HDR);
	}

	ESP_LOGE (TAG, "alloc %d failed (free=%d largest=%d)",
		size, Q_Arena_FreeSize (), Q_Arena_Largest ());
	return NULL;
}

void Q_Arena_Free (void *p)
{
	if (!p)
		return;

	arena_blk_t *b = (arena_blk_t *)((unsigned char *)p - ARENA_HDR);

	if (!b->inuse)
	{
		ESP_LOGW (TAG, "double free at %p", p);
		return;
	}

	b->inuse = 0;
	arena_used -= b->size;

	// coalesce with the previous block when free and contiguous
	if (b->prev != &arena_head && !b->prev->inuse &&
		(unsigned char *)b->prev + b->prev->size == (unsigned char *)b)
	{
		b->prev->size += b->size;
		blk_unlink (b);
		b = b->prev;
		arena_blocks--;
	}

	// coalesce with the next block when free and contiguous
	if (b->next != &arena_head && !b->next->inuse &&
		(unsigned char *)b + b->size == (unsigned char *)b->next)
	{
		b->size += b->next->size;
		blk_unlink (b->next);
		arena_blocks--;
	}
}

int Q_Arena_Used (void)
{
	return arena_used;
}

int Q_Arena_FreeSize (void)
{
	return arena_total - arena_carved - arena_used;
}

int Q_Arena_Largest (void)
{
	int max = 0;
	arena_blk_t *b;

	for (b = arena_head.next ; b != &arena_head ; b = b->next)
		if (!b->inuse && b->size > max)
			max = b->size;

	return max;
}

int Q_Arena_Count (void)
{
	return arena_blocks;
}

// True when the pointer lies inside the allocator-managed region of the arena
// (used by FS_FreeFile to decide whether a transient came from the arena).
int Q_Arena_Contains (void *p)
{
	if (!arena_ready || !p)
		return 0;
	unsigned char *c = (unsigned char *)p;
	return c >= arena_base + arena_carved && c < arena_base + arena_total;
}
