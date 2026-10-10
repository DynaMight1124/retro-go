#pragma once
/*
 * ESP32-P4 SIMD (PIE) memset wrapper for the Quake2 port.
 *
 * Author: Alejandro Villegas Alonso
 *         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1
 *
 * Broadcasts the byte through a 16-byte pattern buffer and fills with
 * 128-bit vector stores, mirroring the mem_cpy::cpy_mem approach from
 *   https://github.com/ctag-fh-kiel/esp32p4_memcpy_pie_benchmark
 *
 * Selection define -- edit P4_MEMSET_IMPL to choose the implementation:
 *   P4_MEMSET_IMPL_LIBC  -> plain libc memset
 *   P4_MEMSET_IMPL_SIMD  -> ESP32-P4 SIMD (fastest)
 */
#include <stddef.h>

#define P4_MEMSET_IMPL_LIBC 0
#define P4_MEMSET_IMPL_SIMD 1

#ifndef P4_MEMSET_IMPL
#define P4_MEMSET_IMPL P4_MEMSET_IMPL_SIMD
#endif

/* Fills smaller than this always go through libc memset. */
#ifndef P4_MEMSET_MIN_SIMD_BYTES
#define P4_MEMSET_MIN_SIMD_BYTES 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Standard memset signature: void p4_memset(void *dst, int value, size_t n). */
void p4_memset(void *dst, int value, size_t n);

#ifdef __cplusplus
}
#endif