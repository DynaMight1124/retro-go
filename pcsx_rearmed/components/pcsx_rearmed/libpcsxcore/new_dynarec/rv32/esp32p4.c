/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "selftest.h"
#include <stdint.h>
#include <string.h>
#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_mmu_map.h>
#include <esp32p4/rom/cache.h>
#include <rg_system.h>

#if !defined(CONFIG_IDF_TARGET_ESP32P4)
#error This executable-PSRAM diagnostic is ESP32-P4 only
#endif

void *ndrc_rv32_p4_alloc_exec(size_t size, void **backing)
{
  const size_t page_size = CONFIG_MMU_PAGE_SIZE;
  size_t mapped_size = (size + page_size - 1) & ~(page_size - 1);
  esp_paddr_t paddr;
  mmu_target_t target;
  void *physical, *mapping = NULL;

  if (!backing) return NULL;
  *backing = NULL;
  physical = heap_caps_aligned_alloc(page_size, mapped_size,
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!physical) return NULL;
  if (esp_mmu_vaddr_to_paddr(physical, &paddr, &target) != ESP_OK ||
      esp_mmu_map(paddr, mapped_size, target, MMU_MEM_CAP_EXEC,
        ESP_MMU_MMAP_FLAG_PADDR_SHARED, &mapping) != ESP_OK) {
    free(physical);
    return NULL;
  }
  memset(mapping, 0, mapped_size);
  *backing = physical;
  return mapping;
}

void ndrc_rv32_p4_free_exec(void *mapping, void *backing)
{
  if (mapping) esp_mmu_unmap(mapping);
  free(backing);
}

/* Match the working port's P4 cache publication sequence, but propagate errors
 * and do not depend on Lightning. The caller owns the surrounding cache lines.
 */
int ndrc_rv32_p4_publish(void *code, size_t size)
{
  uintptr_t start = (uintptr_t)code & ~(uintptr_t)63;
  uintptr_t end = ((uintptr_t)code + size + 63) & ~(uintptr_t)63;
  esp_err_t err = esp_cache_msync((void *)start, end - start, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if (err != ESP_OK) return (int)err;
  err = esp_cache_msync((void *)start, end - start, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if (err != ESP_OK) return (int)err;
  Cache_Invalidate_All(CACHE_MAP_L1_ICACHE_MASK);
  __asm__ volatile ("fence.i" ::: "memory");
  return 0;
}

void ndrc_rv32_p4_diagnostic(void *code, size_t size)
{
  int result;
  if (!code || size < 16384 || ((uintptr_t)code & 63)) {
    RG_LOGE("new_dynarec RV32 selftest: invalid scratch buffer");
    return;
  }
  RG_LOGI("new_dynarec RV32 emitter selftest starting (game core remains Lightrec)");
  result = ndrc_rv32_selftest(code, 16384, ndrc_rv32_p4_publish);
  if (result)
    RG_LOGE("new_dynarec RV32 emitter selftest FAILED at selftest.c:%d", result);
  else
    RG_LOGI("new_dynarec RV32 emitter selftest PASSED (not a guest CPU test)");
  if (!result) {
    RG_LOGI("new_dynarec RV32 ABI selftest starting");
    result = ndrc_rv32_abi_selftest(code, 16384, ndrc_rv32_p4_publish);
    if (result)
      RG_LOGE("new_dynarec RV32 ABI selftest FAILED at abi_test.c:%d", result);
    else
      RG_LOGI("new_dynarec RV32 ABI selftest PASSED (game core remains Lightrec)");
  }
  if (!result) {
    RG_LOGI("new_dynarec RV32 flags selftest starting");
    result = ndrc_rv32_flags_selftest(code, 16384, ndrc_rv32_p4_publish);
    if (result)
      RG_LOGE("new_dynarec RV32 flags selftest FAILED at flags_test.c:%d", result);
    else
      RG_LOGI("new_dynarec RV32 flags selftest PASSED (game core remains Lightrec)");
  }
  if (!result) {
    RG_LOGI("new_dynarec RV32 shared compiler selftest starting (inline RAM LUT + safe scratch/MMIO fallback)");
    result = ndrc_rv32_compiler_selftest(code, 16384, ndrc_rv32_p4_publish);
    if (result)
      RG_LOGE("new_dynarec RV32 shared compiler selftest FAILED at compiler_test.c:%d", result);
    else
      RG_LOGI("new_dynarec RV32 shared compiler selftest PASSED (game core remains Lightrec)");
  }
  if (!result) {
    result = ndrc_rv32_cache_selftest();
    if (result)
      RG_LOGE("new_dynarec RV32 block cache selftest FAILED at block_cache.c:%d", result);
    else
      RG_LOGI("new_dynarec RV32 block cache selftest PASSED (not live guest execution)");
  }
  /* Do not leave test instructions in Lightrec's not-yet-initialized arena. */
  memset(code, 0, 16384);
  if (ndrc_rv32_p4_publish(code, 16384))
    RG_LOGE("new_dynarec RV32 selftest scratch cache sync failed");
}
