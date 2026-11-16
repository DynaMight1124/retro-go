#pragma once

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

/* Native option: 0=320x240, 1=240x180, 2=160x120, 3=106x80.
   Saved options take precedence. */
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define GTAVC_DEFAULT_RESOLUTION 0
#else
/* ESP32-S3 and other targets retain the lower-cost hybrid default. */
#define GTAVC_DEFAULT_RESOLUTION 2
#endif
