/*
 * ESP32-P4 SIMD (PIE) memset implementation.
 *
 * Author: Alejandro Villegas Alonso
 *         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
 *
 * SIMD path per the reference version: the fill byte is replicated into a
 * 16-byte pattern, loaded into a vector register with ESP.VLD.128.IP, and
 * stored repeatedly with ESP.VST.128.IP in a plain software counter loop
 * with a scalar alignment prefix and tail (no hardware loop).
 */
#include "p4_memset.h"

#include <stdint.h>
#include <string.h>

#if P4_MEMSET_IMPL == P4_MEMSET_IMPL_SIMD

static inline void *p4_memset_simd(void *dst, int c, size_t n) {
    uint8_t *p = (uint8_t *)dst;

    // PIE vector stores can force-align addresses down. Never depend on the
    // caller's alignment or the task's current misaligned-store CFG setting.
    while (n && ((uintptr_t)p & 15)) {
        *p++ = (uint8_t)c;
        --n;
    }

    if (n < 16) {
        while (n--)
            *p++ = (uint8_t)c;
        return dst;
    }

    uint32_t val = (uint8_t)c;
    val |= (val << 8) | (val << 16) | (val << 24);

    uint32_t pattern[4] __attribute__((aligned(16))) = {val, val, val, val};

    size_t blocks = n / 16;
    size_t tail = n % 16;

    uint32_t *src_ptr = pattern;

    __asm__ __volatile__(
        "esp.vld.128.ip q0, %0, 0 \n"

        "1: \n"
        "esp.vst.128.ip q0, %1, 16 \n"

        "addi %2, %2, -1 \n"

        "bnez %2, 1b \n"

        : "+r"(src_ptr), "+r"(p), "+r"(blocks)
        :
        : "memory");

    while (tail--) {
        *p++ = (uint8_t)c;
    }

    return dst;
}
#endif

extern "C" void p4_memset(void *dst, int value, size_t n) {
#if P4_MEMSET_IMPL == P4_MEMSET_IMPL_SIMD
    if (n >= P4_MEMSET_MIN_SIMD_BYTES && n <= UINT32_MAX) {
        p4_memset_simd(dst, value, (uint32_t)n);
        return;
    }
#endif
    memset(dst, value, n);
}
