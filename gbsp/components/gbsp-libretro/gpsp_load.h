#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define GPSP_BIOS_PATH RG_BASE_PATH_BIOS "/gba_bios.bin"
void gpsp_bios_init(const char *path);
extern void (*gamepak_load_progress)(int percent);

#ifdef __cplusplus
}
#endif
