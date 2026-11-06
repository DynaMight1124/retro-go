/* xjit_exec.c - executable memory on the ESP32-S3 (see xjit_exec.h). */
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_mmu_map.h"
#include "esp_cache.h"
#include "xjit_exec.h"

#define XJ_PAGE 0x10000   /* MMU page */

bool xj_exec_alloc_iram(xj_exec_t *x, size_t size)
{
    memset(x, 0, sizeof(*x));
    size = (size + 3) & ~3u;
    uint8_t *p = heap_caps_aligned_alloc(4, size, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
    if (!p)
        return false;
    x->data = p;   /* the instruction address takes 32-bit stores */
    x->exec = (uint32_t)(uintptr_t)p;
    x->size = size;
    return true;
}

bool xj_exec_alloc_psram(xj_exec_t *x, size_t size)
{
    memset(x, 0, sizeof(*x));
    size = (size + XJ_PAGE - 1) & ~(size_t)(XJ_PAGE - 1);
    uint8_t *p = heap_caps_aligned_alloc(XJ_PAGE, size, MALLOC_CAP_SPIRAM);
    esp_paddr_t paddr;
    mmu_target_t target;
    void *exec = NULL;
    if (!p)
        return false;
    if (esp_mmu_vaddr_to_paddr(p, &paddr, &target) != ESP_OK
        || esp_mmu_map(paddr, size, target, MMU_MEM_CAP_EXEC, 0, &exec) != ESP_OK)
    {
        heap_caps_free(p);
        return false;
    }
    x->data = p;
    x->exec = (uint32_t)(uintptr_t)exec;
    x->size = size;
    x->psram = true;
    return true;
}

void xj_exec_free(xj_exec_t *x)
{
    if (x->psram && x->exec)
        esp_mmu_unmap((void *)(uintptr_t)x->exec);
    if (x->data)
        heap_caps_free(x->data);
    memset(x, 0, sizeof(*x));
}

void xj_exec_write_word(void *dst, int word_index, uint32_t word)
{
    ((volatile uint32_t *)dst)[word_index] = word;
}

int xj_exec_sync(xj_exec_t *x, size_t off, size_t len)
{
    int err = 0;
    if (x->psram)
    {
        /* whole cache lines (64 bytes covers the S3's 32/64-byte D and I lines):
           the unaligned flag is only for write-back, an invalidate of a partial
           line is refused */
        size_t a = off & ~(size_t)63, n = ((off + len + 63) & ~(size_t)63) - a;
        if (esp_cache_msync(x->data + a, n, ESP_CACHE_MSYNC_FLAG_DIR_C2M) != ESP_OK)
            err |= 1;
        if (esp_cache_msync((void *)(uintptr_t)(x->exec + a), n, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_INST) != ESP_OK)
            err |= 2;
    }
    __asm__ volatile("memw; isync");
    return err;
}
