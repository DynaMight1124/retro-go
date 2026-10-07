/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NDRC_RV32_SELFTEST_H
#define NDRC_RV32_SELFTEST_H
#include <stddef.h>

/* Destructively uses a caller-owned, writable/executable 16 KiB scratch range.
 * Run on the execution core, BEFORE installing any translated guest blocks.
 * publish() returns zero on success. Returns zero, or the failing source line.
 */
int ndrc_rv32_selftest(void *code, size_t size, int (*publish)(void *, size_t));
int ndrc_rv32_abi_selftest(void *code, size_t size, int (*publish)(void *, size_t));
int ndrc_rv32_flags_selftest(void *code, size_t size, int (*publish)(void *, size_t));
int ndrc_rv32_compiler_selftest(void *code, size_t size, int (*publish)(void *, size_t));
int ndrc_rv32_cache_selftest(void);
#ifdef ESP_PLATFORM
int ndrc_rv32_p4_publish(void *code, size_t size);
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
int ndrc_rv32_live_canary_try(void *regs);
void ndrc_rv32_live_canary_shutdown(void);
#endif
#endif
#endif
