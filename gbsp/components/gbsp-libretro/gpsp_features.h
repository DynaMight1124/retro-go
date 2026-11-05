#ifndef GPSP_FEATURES_H
#define GPSP_FEATURES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Shared with load_gamepak's force-feature arguments. */
#define FEAT_AUTODETECT  -1
#define FEAT_DISABLE      0
#define FEAT_ENABLE       1

/* Effective ROM detection/override status, after load_gamepak(). */
#ifdef __cplusplus
extern "C" {
#endif
bool gpsp_rtc_is_enabled(void);
int64_t gpsp_rtc_time(void);
void gpsp_rtc_restore(int64_t saved);
void *gpsp_save_data(void);
size_t gpsp_save_bytes(void);
bool gpsp_save_dirty(void);
void gpsp_save_clean(void);
void gpsp_save_mark_dirty(void);
#ifdef __cplusplus
}
#endif

#endif
