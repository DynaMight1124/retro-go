#ifdef __cplusplus
extern "C" {
#endif

#ifndef RG_MAME_WRAPPER
#define RG_MAME_WRAPPER

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void odx_clear_video();
void odx_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#define printf odx_printf

#endif

#ifdef __cplusplus
}
#endif
