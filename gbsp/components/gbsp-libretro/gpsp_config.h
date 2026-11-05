
#ifndef GPSP_CONFIG_H
#define GPSP_CONFIG_H

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
#define GPSP_NAME                "gpSP"
#define GPSP_VERSION             "v1.1.0"
#define GPSP_NETPACKET_VERSION   "gpSP v1.0"
#else /* GBSP_BACKEND_VARIANT */
#define GPSP_NAME                "gpSP"
#define GPSP_VERSION             "v1.0.0"
#define GPSP_NETPACKET_VERSION   "gpSP v1.0"
#endif /* GBSP_BACKEND_VARIANT */

/* Default ROM buffer size in megabytes (this is a maximum value!) */
#ifndef ROM_BUFFER_SIZE
#define ROM_BUFFER_SIZE 32
#endif

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
/* Cache sizes and their config knobs */
#if defined(SMALL_TRANSLATION_CACHE)
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 1024)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 256)
#else
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 1024 * 10)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 512)
#endif
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */
/* Cache sizes and their config knobs */
#if defined(XT_IRAM_CACHE)   /* xtensa experiment: both caches in internal RAM */
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 32)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 16)
#elif defined(SMALL_TRANSLATION_CACHE)
  /* xtensa (2026-10-13): 256 KB moved from the ROM cache to the RAM cache,
     the same PSRAM in all. NFS Underground's race runs ~250 hot blocks of
     IWRAM code (~200 KB translated) and the RAM cache, rewound at half of
     384 KB, was emptied 8-11 times a second, each time translating it all
     again (about a third of core 0).
     2026-10-20: Mario Kart's race path fills 1752 KB of the ROM cache and
     then flushes it, recompiling the working set for several seconds (exec
     went from ~15 to ~24 ms). Use the free PSRAM tail for 640 KB more; the
     cache no longer flushes during that race. */
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 2432)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 640)
#else
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 1024 * 10)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 512)
#endif
#else /* GBSP_BACKEND_VARIANT */
/* Cache sizes and their config knobs */
#if defined(SMALL_TRANSLATION_CACHE)
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 1024 * 2)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 384)
#else
  #define ROM_TRANSLATION_CACHE_SIZE (1024 * 1024 * 10)
  #define RAM_TRANSLATION_CACHE_SIZE (1024 * 512)
#endif
#endif /* GBSP_BACKEND_VARIANT */

/* Should be an upperbound to the maximum number of bytes a single JIT'ed
   instruction can take. STM/LDM are tipically the biggest ones */
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024 * 2)

/* Hash table size for ROM trans cache lookups */
#define ROM_BRANCH_HASH_BITS                           16
#define ROM_BRANCH_HASH_SIZE   (1 << ROM_BRANCH_HASH_BITS)

/* RFU Multiplayer config, do not mess around too much with it */
#define MAX_RFU_NETPLAYERS       32
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

/* Serial modes (multiplayer serial). */
#define MAX_SERMULT_NETPLAYERS    4

#else /* GBSP_BACKEND_VARIANT */

#endif /* GBSP_BACKEND_VARIANT */
#endif
