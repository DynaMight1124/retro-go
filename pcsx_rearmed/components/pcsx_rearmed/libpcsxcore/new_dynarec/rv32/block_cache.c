/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "block_cache.h"
#include <string.h>

#define CACHE_PROBES 8u

uint32_t ndrc_rv32_source_hash(const uint32_t *words, unsigned count)
{
  uint32_t hash = 2166136261u;
  if (!words) return 0;
  for (unsigned i = 0; i < count; i++) {
    uint32_t word = words[i];
    for (unsigned byte = 0; byte < 4; byte++) {
      hash ^= (uint8_t)(word >> (byte * 8));
      hash *= 16777619u;
    }
  }
  /* Zero is useful as an unmistakable empty/debug value. */
  return hash ? hash : 1;
}

static unsigned cache_index(const struct ndrc_rv32_block_cache *cache,
    uint32_t pc)
{
  /* Guest instructions are word aligned; Knuth multiplication spreads nearby
   * basic blocks before the modulo for non-power-of-two test tables too. */
  return (unsigned)(((pc >> 2) * 2654435761u) % cache->entry_count);
}

static unsigned cache_probe_index(const struct ndrc_rv32_block_cache *cache,
    unsigned first, unsigned probe)
{
  unsigned index = first + probe;
  return index < cache->entry_count ? index : index - cache->entry_count;
}

int ndrc_rv32_cache_init(struct ndrc_rv32_block_cache *cache,
    struct ndrc_rv32_cache_entry *entries, unsigned entry_count,
    void *arena, size_t arena_size)
{
  if (!cache || !entries || !entry_count || !arena || !arena_size) return -1;
  memset(cache, 0, sizeof(*cache));
  cache->entries = entries;
  cache->entry_count = entry_count;
  cache->arena = arena;
  cache->arena_size = arena_size;
  cache->generation = 1;
  memset(entries, 0, entry_count * sizeof(*entries));
  return 0;
}

void ndrc_rv32_cache_reset(struct ndrc_rv32_block_cache *cache)
{
  if (!cache || !cache->entries) return;
  cache->generation++;
  if (!cache->generation) {
    memset(cache->entries, 0, cache->entry_count * sizeof(*cache->entries));
    cache->generation = 1;
  }
  cache->used_entries = 0;
  cache->arena_used = 0;
}

void *ndrc_rv32_cache_reserve(struct ndrc_rv32_block_cache *cache,
    size_t code_bytes, size_t alignment, uint32_t *offset_out)
{
  size_t start;
  if (!cache || !code_bytes || !alignment || (alignment & (alignment - 1)))
    return NULL;
  start = (cache->arena_used + alignment - 1) & ~(alignment - 1);
  if (start > cache->arena_size || code_bytes > cache->arena_size - start)
    return NULL;
  if (start > UINT32_MAX || code_bytes > UINT32_MAX) return NULL;
  cache->arena_used = start + code_bytes;
  if (offset_out) *offset_out = (uint32_t)start;
  return cache->arena + start;
}

const struct ndrc_rv32_cache_entry *ndrc_rv32_cache_lookup(
    const struct ndrc_rv32_block_cache *cache, uint32_t pc,
    uint32_t source_hash)
{
  unsigned first, probes;
  if (!cache || !cache->entries || !cache->entry_count) return NULL;
  first = cache_index(cache, pc);
  probes = cache->entry_count < CACHE_PROBES ? cache->entry_count : CACHE_PROBES;
  for (unsigned i = 0; i < probes; i++) {
    const struct ndrc_rv32_cache_entry *entry =
      &cache->entries[cache_probe_index(cache, first, i)];
    if (entry->generation == cache->generation && entry->pc == pc &&
        entry->source_hash == source_hash) return entry;
  }
  return NULL;
}

const struct ndrc_rv32_cache_entry *ndrc_rv32_cache_lookup_pc(
    const struct ndrc_rv32_block_cache *cache, uint32_t pc)
{
  unsigned first, probes;
  if (!cache || !cache->entries || !cache->entry_count) return NULL;
  first = cache_index(cache, pc);
  probes = cache->entry_count < CACHE_PROBES ? cache->entry_count : CACHE_PROBES;
  for (unsigned i = 0; i < probes; i++) {
    const struct ndrc_rv32_cache_entry *entry =
      &cache->entries[cache_probe_index(cache, first, i)];
    if (entry->generation == cache->generation && entry->pc == pc)
      return entry;
  }
  return NULL;
}

int ndrc_rv32_cache_insert(struct ndrc_rv32_block_cache *cache, uint32_t pc,
    uint32_t guest_bytes, uint32_t code_offset, uint32_t code_bytes,
    uint32_t dispatcher_offset, uint32_t source_hash)
{
  struct ndrc_rv32_cache_entry *entry = NULL, *victim;
  unsigned first, probes;
  if (!cache || !cache->entries || !cache->entry_count || !guest_bytes ||
      !code_bytes || code_offset > cache->arena_size ||
      code_bytes > cache->arena_size - code_offset ||
      dispatcher_offset >= code_bytes) return -1;
  first = cache_index(cache, pc);
  probes = cache->entry_count < CACHE_PROBES ? cache->entry_count : CACHE_PROBES;
  victim = &cache->entries[first];
  for (unsigned i = 0; i < probes; i++) {
    struct ndrc_rv32_cache_entry *candidate =
      &cache->entries[cache_probe_index(cache, first, i)];
    if (candidate->generation == cache->generation && candidate->pc == pc) {
      entry = candidate;
      break;
    }
    if (!entry && candidate->generation != cache->generation)
      entry = candidate;
  }
  if (!entry) entry = victim;
  if (entry->generation != cache->generation) cache->used_entries++;
  *entry = (struct ndrc_rv32_cache_entry) {
    pc, guest_bytes, code_offset, code_bytes, dispatcher_offset, source_hash,
    cache->generation, { 0, 0 }
  };
  return 0;
}

unsigned ndrc_rv32_cache_invalidate(struct ndrc_rv32_block_cache *cache,
    uint32_t start_pc, uint32_t length)
{
  unsigned invalidated = 0;
  uint64_t end = (uint64_t)start_pc + length;
  if (!cache || !length) return 0;
  for (unsigned i = 0; i < cache->entry_count; i++) {
    struct ndrc_rv32_cache_entry *entry = &cache->entries[i];
    uint64_t block_end = (uint64_t)entry->pc + entry->guest_bytes;
    if (entry->generation == cache->generation &&
        (uint64_t)entry->pc < end && block_end > start_pc) {
      entry->generation = 0;
      cache->used_entries--;
      invalidated++;
    }
  }
  return invalidated;
}

int ndrc_rv32_cache_selftest(void)
{
  struct ndrc_rv32_block_cache cache;
  struct ndrc_rv32_cache_entry entries[7];
  unsigned char arena[256];
  uint32_t first = UINT32_MAX, second = UINT32_MAX;
  void *a, *b;
#define CACHE_CHECK(v) do { if (!(v)) return __LINE__; } while (0)
  CACHE_CHECK(ndrc_rv32_cache_init(&cache, entries, 7, arena, sizeof(arena)) == 0);
  {
    const uint32_t words[2] = {0x24210001u, 0xac220000u};
    CACHE_CHECK(ndrc_rv32_source_hash(words, 2) == 0xe2b63d67u);
    CACHE_CHECK(ndrc_rv32_source_hash(words, 1) !=
      ndrc_rv32_source_hash(words, 2));
  }
  a = ndrc_rv32_cache_reserve(&cache, 13, 4, &first);
  b = ndrc_rv32_cache_reserve(&cache, 17, 16, &second);
  CACHE_CHECK(a == arena && first == 0 && b == arena + 16 && second == 16);
  CACHE_CHECK(ndrc_rv32_cache_insert(&cache, 0x80010000, 20, first, 13, 4,
    0x11111111) == 0);
  CACHE_CHECK(ndrc_rv32_cache_insert(&cache, 0x80010100, 16, second, 17, 8,
    0x22222222) == 0);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010000, 0x11111111) != NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010000,
    0x11111111)->dispatcher_offset == 4);
  CACHE_CHECK(ndrc_rv32_cache_lookup_pc(&cache, 0x80010000) != NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup_pc(&cache, 0x80010200) == NULL);
  /* Seven guest words apart maps to the same initial slot in this table. */
  CACHE_CHECK(ndrc_rv32_cache_insert(&cache, 0x8001001c, 4, first, 13, 4,
    0x33333333) == 0);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010000, 0x11111111) != NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x8001001c, 0x33333333) != NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010000, 0x99999999) == NULL);
  CACHE_CHECK(ndrc_rv32_cache_invalidate(&cache, 0x80010010, 4) == 1);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010000, 0x11111111) == NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup_pc(&cache, 0x80010000) == NULL);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010100, 0x22222222) != NULL);
  ndrc_rv32_cache_reset(&cache);
  CACHE_CHECK(cache.arena_used == 0 && cache.used_entries == 0);
  CACHE_CHECK(ndrc_rv32_cache_lookup(&cache, 0x80010100, 0x22222222) == NULL);
  CACHE_CHECK(ndrc_rv32_cache_reserve(&cache, sizeof(arena) + 1, 4, NULL) == NULL);
#undef CACHE_CHECK
  return 0;
}
