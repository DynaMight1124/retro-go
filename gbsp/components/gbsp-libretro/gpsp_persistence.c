#include "common.h"
#include "gba_memory.h"
#include "gpsp_features.h"
#include "gpsp_clock.h"
#include "gpsp_log.h"
#include <time.h>

extern u8 gamepak_backup_dirty;
static gpsp_clock_t cartridge_clock;
static bool clock_ready;

void *gpsp_save_data(void) { return gamepak_backup; }
size_t gpsp_save_bytes(void)
{
    switch (backup_type) {
    case BACKUP_SRAM: return 32 * 1024;
    case BACKUP_FLASH: return 64 * 1024 * (flash_bank_cnt ? flash_bank_cnt : 1);
    case BACKUP_EEPROM: return 512 * (eeprom_size ? eeprom_size : 1);
    default: return sizeof(gamepak_backup);
    }
}
bool gpsp_save_dirty(void) { return gamepak_backup_dirty != 0; }
void gpsp_save_clean(void) { gamepak_backup_dirty = 0; }
void gpsp_save_mark_dirty(void) { gamepak_backup_dirty = 1; }

void gpsp_rtc_restore(int64_t saved)
{
    int64_t wall = (int64_t)time(NULL);
    gpsp_clock_start(&cartridge_clock, wall, saved, rg_system_timer());
    if (gpsp_rtc_is_enabled() && !gpsp_clock_time_valid(wall))
        RG_LOGI("RTC: device date out of range; using %s",
                gpsp_clock_time_valid(saved) ? "saved cartridge clock" : "offline clock (year 2000)");
    clock_ready = true;
}

int64_t gpsp_rtc_time(void)
{
    if (!clock_ready) gpsp_rtc_restore(0);
    return gpsp_clock_read(&cartridge_clock, (int64_t)time(NULL), rg_system_timer());
}
