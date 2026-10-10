#pragma once
/*
 * ESP32-P4 SIMD (PIE) memcpy wrapper for the Quake2 port.
 *
 * Author: Alejandro Villegas Alonso
 *         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
 *
 * Implements of the fastest memcpy from
 *   https://github.com/ctag-fh-kiel/esp32p4_memcpy_pie_benchmark
 * (the C++ template version mem_cpy::cpy_mem, ~2x faster than libc in SRAM).
 *
 * Selection define -- edit P4_MEMCPY_IMPL to choose the implementation:
 *   P4_MEMCPY_IMPL_LIBC  -> plain libc memcpy
 *   P4_MEMCPY_IMPL_SIMD  -> ESP32-P4 SIMD (fastest)
 */
#include <stddef.h>

#define P4_MEMCPY_IMPL_LIBC 0
#define P4_MEMCPY_IMPL_SIMD 1

#ifndef P4_MEMCPY_IMPL
#define P4_MEMCPY_IMPL P4_MEMCPY_IMPL_SIMD
#endif

/* Copies smaller than this always go through libc memcpy. */
#ifndef P4_MEMCPY_MIN_SIMD_BYTES
#define P4_MEMCPY_MIN_SIMD_BYTES 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Standard memcpy signature: void *p4_memcpy(void *dst, const void *src, size_t n). */
void p4_memcpy(void *dst, const void *src, size_t n);

#ifdef __cplusplus
}
#endif