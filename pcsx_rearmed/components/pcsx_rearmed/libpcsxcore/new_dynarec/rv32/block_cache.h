/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PCSX_NDRC_RV32_BLOCK_CACHE_H
#define PCSX_NDRC_RV32_BLOCK_CACHE_H

#include <stddef.h>
#include <stdint.h>

struct ndrc_rv32_cache_entry {
  uint32_t pc;
  uint32_t guest_bytes;
  uint32_t code_offset;
  uint32_t code_bytes;
  uint32_t dispatcher_offset;
  uint32_t source_hash;
  uint32_t generation;
  /* Client-owned source-page generations. The generic cache initializes
   * these to zero; users with write tracking may stamp and validate them. */
  uint32_t source_generation[2];
};

struct ndrc_rv32_block_cache {
  struct ndrc_rv32_cache_entry *entries;
  unsigned entry_count;
  unsigned generation;
  unsigned used_entries;
  unsigned char *arena;
  size_t arena_size;
  size_t arena_used;
};

uint32_t ndrc_rv32_source_hash(const uint32_t *words, unsigned count);

int ndrc_rv32_cache_init(struct ndrc_rv32_block_cache *cache,
  struct ndrc_rv32_cache_entry *entries, unsigned entry_count,
  void *arena, size_t arena_size);
void ndrc_rv32_cache_reset(struct ndrc_rv32_block_cache *cache);
void *ndrc_rv32_cache_reserve(struct ndrc_rv32_block_cache *cache,
  size_t code_bytes, size_t alignment, uint32_t *offset_out);
const struct ndrc_rv32_cache_entry *ndrc_rv32_cache_lookup(
  const struct ndrc_rv32_block_cache *cache, uint32_t pc,
  uint32_t source_hash);
const struct ndrc_rv32_cache_entry *ndrc_rv32_cache_lookup_pc(
  const struct ndrc_rv32_block_cache *cache, uint32_t pc);
int ndrc_rv32_cache_insert(struct ndrc_rv32_block_cache *cache, uint32_t pc,
  uint32_t guest_bytes, uint32_t code_offset, uint32_t code_bytes,
  uint32_t dispatcher_offset, uint32_t source_hash);
unsigned ndrc_rv32_cache_invalidate(struct ndrc_rv32_block_cache *cache,
  uint32_t start_pc, uint32_t length);
int ndrc_rv32_cache_selftest(void);

#endif
