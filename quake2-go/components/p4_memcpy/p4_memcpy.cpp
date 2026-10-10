/*
 * ESP32-P4 SIMD (PIE) memcpy implementation.
 *
 * Author: Alejandro Villegas Alonso
 *         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
 *
 * Dispatches to mem_cpy::cpy_mem (fastest, from the ctag-fh-kiel benchmark)
 * or to libc memcpy depending on P4_MEMCPY_IMPL.
 */
#include "p4_memcpy.h"

#include <stdint.h>
#include <string.h>

#if P4_MEMCPY_IMPL == P4_MEMCPY_IMPL_SIMD
#include "mem_cpy_p4.hpp"
#endif

extern "C" void p4_memcpy(void *dst, const void *src, size_t n) {
#if P4_MEMCPY_IMPL == P4_MEMCPY_IMPL_SIMD
    if (n >= P4_MEMCPY_MIN_SIMD_BYTES && n <= UINT32_MAX) {
        // The imported PIE pipeline rounds source loads down to 16 bytes
        // and reads two vectors ahead. Keep those reads inside this source
        // object, rather than requiring undocumented padding from callers.
        size_t head = 16 + ((16 - ((uintptr_t)dst & 15)) & 15);
        if (n > head + 32) {
            size_t body = (n - head - 32) & ~(size_t)15;
            if (body >= P4_MEMCPY_MIN_SIMD_BYTES) {
                const uint8_t *input = (const uint8_t *)src;
                uint8_t *output = (uint8_t *)dst;
                memcpy(output, input, head);
                mem_cpy::cpy_mem(input + head, output + head, (uint32_t)body);
                memcpy(output + head + body, input + head + body, n - head - body);
                return;
            }
        }
    }
#endif
    memcpy(dst, src, n);
}
