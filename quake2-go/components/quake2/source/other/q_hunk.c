// Author: Alejandro Villegas Alonso
//         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
//
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../qcommon/qcommon.h"
#include "esp_log.h"
#include "q_arena.h"

//===============================================================================
//
// Hunk allocator backed by a static PSRAM pool (EXT_RAM_BSS), outside the heap
// pool so it does not compete with the transient heap or the engine arena.
// Multiple model hunks must coexist (world + aliases + sprites), so we keep a
// free-list of blocks and carve each Hunk_Begin reserve out of the pool,
// releasing the block back to the pool on Hunk_Free.
//
//===============================================================================

#define HUNK_POOL_SIZE (7 * 1024 * 1024)
#define HUNK_MIN_SPLIT 128
#define HUNK_MAX_BLOCKS 1024

typedef struct {
  unsigned char *start;
  int size;
  int used; // 0 = free block, 1 = in use
} hunkblock_t;

unsigned char hunk_pool[HUNK_POOL_SIZE] EXT_RAM_BSS
    __attribute__((aligned(64)));
hunkblock_t blocks[HUNK_MAX_BLOCKS] EXT_RAM_BSS;
int numblocks EXT_RAM_BSS;
int hunkcount;

unsigned char *membase;
int hunkmaxsize;
int cursize;

static int pool_used;
static int pool_peak;

// Reset the pool metadata; the backing storage is the static PSRAM hunk_pool.
void Hunk_Init(void) {
  hunkcount = 0;
  pool_used = 0;
  pool_peak = 0;
  numblocks = 0;
  memset(hunk_pool, 0, HUNK_POOL_SIZE);
}

static void Hunk_UpdatePoolUsed(void) {
  int i, freebytes;

  freebytes = 0;
  for (i = 0; i < numblocks; i++)
    if (!blocks[i].used)
      freebytes += blocks[i].size;
  pool_used = HUNK_POOL_SIZE - freebytes;
  if (pool_used > pool_peak)
    pool_peak = pool_used;
}

static void Hunk_LogUsage(int maxsize, unsigned char *base) {
  ESP_LOGI("HUNK", "begin %6d -> %p  pool_used=%6d peak=%6d/%d", maxsize, base,
           pool_used, pool_peak, HUNK_POOL_SIZE);
}

static void Hunk_Defrag(void) {
  int i;

  for (i = 1; i < numblocks; i++) {
    if (!blocks[i - 1].used && !blocks[i].used &&
        blocks[i - 1].start + blocks[i - 1].size == blocks[i].start) {
      blocks[i - 1].size += blocks[i].size;
      memmove(&blocks[i], &blocks[i + 1],
              (numblocks - i - 1) * sizeof(hunkblock_t));
      numblocks--;
      i--;
    }
  }
}

void *Hunk_Begin(int maxsize) {
  int i, best, bestsize, rem;

  cursize = 0;
  hunkmaxsize = maxsize;

  if (!numblocks) {
    blocks[0].start = hunk_pool;
    blocks[0].size = HUNK_POOL_SIZE;
    blocks[0].used = 0;
    numblocks = 1;
  }

  Hunk_UpdatePoolUsed();

  //
  // best fit over the free blocks
  //
  best = -1;
  bestsize = 0x7fffffff;
  for (i = 0; i < numblocks; i++) {
    if (blocks[i].used)
      continue;
    if (blocks[i].size >= maxsize && blocks[i].size < bestsize) {
      best = i;
      bestsize = blocks[i].size;
    }
  }

  if (best < 0)
    Sys_Error("Hunk_Begin: hunk pool exhausted (%d needed, %d blocks live)",
              maxsize, numblocks);

  blocks[best].used = 1;
  membase = blocks[best].start;

  Hunk_LogUsage(maxsize, membase);

  // the pool reuses blocks, so the previous map's data would otherwise
  // survive into fields the model loaders leave unwritten (texinfo next
  // chains, surface cachespots, node/leaf state) and crash the renderer
  memset(blocks[best].start, 0, maxsize);

  //
  // give back the unused tail of the block
  //
  rem = blocks[best].size - maxsize;
  if (rem >= HUNK_MIN_SPLIT && numblocks < HUNK_MAX_BLOCKS) {
    memmove(&blocks[best + 2], &blocks[best + 1],
            (numblocks - best - 1) * sizeof(hunkblock_t));
    blocks[best + 1].start = blocks[best].start + maxsize;
    blocks[best + 1].size = rem;
    blocks[best + 1].used = 0;
    blocks[best].size = maxsize;
    numblocks++;
  } // Otherwise retain the entire block so the unsplit tail can be reclaimed.

  return (void *)membase;
}

void *Hunk_Alloc(int size) {
  if (size < 0 || size > INT_MAX - 31)
    Sys_Error("Hunk_Alloc invalid size");
  // round to cacheline
  size = (size + 31) & ~31;

  if (size > hunkmaxsize - cursize)
    Sys_Error("Hunk_Alloc overflow");
  cursize += size;

  return (void *)(membase + cursize - size);
}

int Hunk_End(void) {
  int i, newsize, excess, oldsize;

  hunkcount++;

  // shrink the current block back to the actually used size and give
  // the excess space back to the pool
  newsize = (cursize + 31) & ~31;
  for (i = 0; i < numblocks; i++) {
    if (blocks[i].used && blocks[i].start == (unsigned char *)membase) {
      oldsize = blocks[i].size;
      excess = oldsize - newsize;
      // Shrink only if the tail can be represented or merged. At metadata
      // capacity retain it in this block for reclamation by Hunk_Free.
      if (excess >= 32 && (numblocks < HUNK_MAX_BLOCKS ||
          (i + 1 < numblocks && !blocks[i + 1].used &&
           blocks[i].start + oldsize == blocks[i + 1].start))) {
        blocks[i].size = newsize;
        if (i + 1 < numblocks && !blocks[i + 1].used &&
            blocks[i].start + oldsize == blocks[i + 1].start) {
          // the block that used to follow us is free and
          // contiguous: adopt the excess
          blocks[i + 1].start = blocks[i].start + newsize;
          blocks[i + 1].size += excess;
        } else if (i + 1 < numblocks && numblocks < HUNK_MAX_BLOCKS) {
          // unrelated block follows: insert a new free block
          memmove(&blocks[i + 2], &blocks[i + 1],
                  (numblocks - i - 1) * sizeof(hunkblock_t));
          blocks[i + 1].start = blocks[i].start + newsize;
          blocks[i + 1].size = excess;
          blocks[i + 1].used = 0;
          numblocks++;
        } else if (numblocks < HUNK_MAX_BLOCKS) {
          // tail of the pool: append a free block
          blocks[numblocks].start = blocks[i].start + newsize;
          blocks[numblocks].size = excess;
          blocks[numblocks].used = 0;
          numblocks++;
        }
        Hunk_Defrag();
      }
      Hunk_UpdatePoolUsed();
      return cursize;
    }
  }
  return cursize;
}

void Hunk_Free(void *base) {
  int i;

  if (!base)
    return;

  for (i = 0; i < numblocks; i++) {
    if (blocks[i].start == (unsigned char *)base) {
      if (!blocks[i].used)
        return;
      blocks[i].used = 0;
      Hunk_Defrag();
      hunkcount--;
      Hunk_UpdatePoolUsed();
      return;
    }
  }

  ESP_LOGW("HUNK", "Hunk_Free: block %p not in pool", base);
}

int Hunk_PoolUsed(void) { return pool_used; }

//============================================
