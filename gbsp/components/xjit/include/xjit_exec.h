/* xjit_exec.h - executable memory for translated code on the ESP32-S3.
 *
 * Two kinds of code cache:
 *  - internal RAM (heap MALLOC_CAP_EXEC; needs CONFIG_ESP_SYSTEM_MEMPROT_FEATURE
 *    off), written 32 bits at a time through its instruction address;
 *  - PSRAM, written through its data address and mapped a second time for
 *    instruction fetch with esp_mmu_map(MMU_MEM_CAP_EXEC); after writing,
 *    xj_exec_sync() writes the data cache back and drops stale instruction
 *    cache lines.
 */
#ifndef XJIT_EXEC_H
#define XJIT_EXEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *data;      /* where to write (data alias) */
    uint32_t exec;      /* where the code runs (instruction alias) */
    size_t size;
    bool psram;
} xj_exec_t;

bool xj_exec_alloc_iram(xj_exec_t *x, size_t size);
bool xj_exec_alloc_psram(xj_exec_t *x, size_t size);   /* rounded up to 64 KB MMU pages */
void xj_exec_free(xj_exec_t *x);
/* xjb_finalize() writer: dst = x->data + byte offset of the block */
void xj_exec_write_word(void *dst, int word_index, uint32_t word);
/* make [off, off + len) visible to instruction fetch; 0, or bit 0 write-back / bit 1 invalidate failed */
int xj_exec_sync(xj_exec_t *x, size_t off, size_t len);
static inline void *xj_exec_ptr(const xj_exec_t *x, size_t off) { return (void *)(uintptr_t)(x->exec + off); }

#endif
