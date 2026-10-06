#ifndef MK64_RG_DIAGNOSTIC_H
#define MK64_RG_DIAGNOSTIC_H

/* Keep detailed renderer snapshots without frequent serial stalls. */
#define MK64_RG_REPORT_INTERVAL 120

/* Enable through the application component's MK64_RG_PROFILE CMake option. */
#ifndef MK64_RG_PROFILE
#define MK64_RG_PROFILE 0
#endif

#ifndef MK64_RG_MENU_TIMING
#define MK64_RG_MENU_TIMING 0
#endif

/* Temporary pixel-stage cycle probes: enable only for a profiling build. */
#ifndef MK64_RG_PIXEL_PROBE
#define MK64_RG_PIXEL_PROBE 0
#endif

#endif
