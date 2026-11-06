#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "gpsp_features.h"
#include "gpsp_clock.h"
#include "gbsp_save_state.h"

/* Shared SD/LCD buses need short transfers. Yield on the worker, never wait
 * for SD from the emulation loop. Synchronous startup/lifecycle is deliberate. */
static void gp_save_yield(void)
{
    /* One display opportunity per chunk, never a wait for an idle display.
     * At 100 Hz these millisecond delays otherwise truncate to zero ticks. */
    const TickType_t display_wait = pdMS_TO_TICKS(1);
    const TickType_t chunk_wait = pdMS_TO_TICKS(6);
    if (rg_display_is_busy()) vTaskDelay(display_wait ? display_wait : 1);
    vTaskDelay(chunk_wait ? chunk_wait : 1);
}
#define SRAM_FILE_LOGE(...) RG_LOGE(__VA_ARGS__)
#define SRAM_FILE_LOGI(...) RG_LOGI(__VA_ARGS__)
#define SRAM_FILE_CHUNK 4096
#define SRAM_FILE_YIELD() gp_save_yield()
#include "sram_file.h"

typedef struct {
    bool data, rtc;
    size_t len;
    unsigned char metadata[16];
} gp_save_job_t;

static char gp_save_path[280], gp_save_rtc_path[288];
static unsigned char *gp_save_snap;
static QueueHandle_t gp_save_jobs, gp_save_done;
static TaskHandle_t gp_save_task_h;
static bool gp_save_enabled;
static bool gp_save_last_ok = true;
static gbsp_save_state_t gp_save_state;

static bool gp_save_write(const gp_save_job_t *job, const void *data)
{
    if (job->data && !sram_file_write(gp_save_path, data, job->len)) return false;
    if (job->rtc && !sram_file_write(gp_save_rtc_path, job->metadata, 16)) return false;
    return true;
}

#if defined(XTENSA_ARCH) || defined(RISCV_ARCH)
static void gp_save_task(void *arg)
{
    gp_save_job_t job;
    for (;;) {
        if (xQueueReceive(gp_save_jobs, &job, portMAX_DELAY) == pdTRUE) {
            bool ok = gp_save_write(&job, gp_save_snap);
            xQueueSend(gp_save_done, &ok, portMAX_DELAY);
        }
    }
}
#endif

static void gp_save_alloc(void)
{
#if defined(XTENSA_ARCH) || defined(RISCV_ARCH)
    /* Reserve before the ROM cache. Original ESP32 keeps its cache budget and
     * commits at menus/shutdown rather than reserving another 128 KiB. */
    gp_save_snap = rg_alloc(128 * 1024, MEM_SLOW | MEM_NOPANIC);
    gp_save_jobs = xQueueCreate(1, sizeof(gp_save_job_t));
    gp_save_done = xQueueCreate(1, sizeof(bool));
    if (gp_save_snap && gp_save_jobs && gp_save_done &&
        xTaskCreatePinnedToCore(gp_save_task, "gba_save", 4096, NULL, 1,
                                &gp_save_task_h, 0) == pdPASS) {
        RG_LOGI("battery save: 128 KB background snapshot ready");
        return;
    }
    if (gp_save_jobs) vQueueDelete(gp_save_jobs);
    if (gp_save_done) vQueueDelete(gp_save_done);
    free(gp_save_snap);
    gp_save_snap = NULL;
    gp_save_jobs = gp_save_done = NULL;
    gp_save_task_h = NULL;
    RG_LOGW("battery save: background resources unavailable; saving at menus/shutdown");
#endif
}

static void gp_save_init(const char *rom_path)
{
    char *path = rg_emu_get_path(RG_PATH_SAVE_SRAM, rom_path);
    RG_ASSERT(path && strlen(path) < sizeof(gp_save_path), "SRAM path too long");
    strcpy(gp_save_path, path);
    char *slash = strrchr(path, '/');
    RG_ASSERT(slash, "Invalid SRAM path");
    *slash = '\0';
    RG_ASSERT(rg_storage_mkdir(path), "Cannot create SRAM directory");
    free(path);
    snprintf(gp_save_rtc_path, sizeof(gp_save_rtc_path), "%s.rtc", gp_save_path);
    gp_save_state = (gbsp_save_state_t){0};
    size_t got = sram_file_read(gp_save_path, gpsp_save_data(), 128 * 1024);
    /* Read existing data, but let observed cartridge capacity determine writes.
     * File length is not a chip-identification hint. */
    if (got) RG_LOGI("battery save loaded: %s (%u bytes)", gp_save_path, (unsigned)got);
    else RG_LOGI("battery save: nothing at %s yet", gp_save_path);
    unsigned char metadata[17] = {0};
    int64_t saved = 0;
    if (gpsp_rtc_is_enabled()) {
        size_t len = sram_file_read(gp_save_rtc_path, metadata, sizeof(metadata));
        const char *reason = len ? gpsp_clock_decode_error(metadata, (unsigned)len, &saved) : NULL;
        if (reason) {
            saved = 0;
            RG_LOGW("RTC: ignored invalid clock metadata (%s, %u bytes read); "
                    "data=%02x%02x%02x%02x/%02x%02x%02x%02x/%02x%02x%02x%02x/%02x%02x%02x%02x",
                    reason, (unsigned)len,
                    metadata[0], metadata[1], metadata[2], metadata[3],
                    metadata[4], metadata[5], metadata[6], metadata[7],
                    metadata[8], metadata[9], metadata[10], metadata[11],
                    metadata[12], metadata[13], metadata[14], metadata[15]);
        }
    }
    gpsp_rtc_restore(saved);
    if (gpsp_rtc_is_enabled())
        RG_LOGI("RTC: real elapsed time; %s",
                saved ? "saved clock restored" : "starting new cartridge clock");
    gpsp_save_clean();
    gp_save_state.last_rtc = gp_save_state.last_attempt = rg_system_timer();
    gp_save_enabled = true;
}

static void gp_save_collect(void)
{
    if (gpsp_save_dirty()) {
        gbsp_save_touch(&gp_save_state, gpsp_save_bytes(), rg_system_timer());
        gpsp_save_clean();
    }
}

static bool gp_save_reap(TickType_t wait)
{
    if (!gp_save_state.inflight) return true;
    bool ok;
    if (xQueueReceive(gp_save_done, &ok, wait) != pdTRUE) return false;
    gp_save_last_ok = ok;
    gbsp_save_complete(&gp_save_state, ok, rg_system_timer());
    if (!ok) RG_LOGE("battery save: write failed; pending data retained for retry");
    return true;
}

static void gp_save_schedule(bool force, bool quiet)
{
    int64_t now = rg_system_timer();
    bool rtc_due = gpsp_rtc_is_enabled() && (force || now-gp_save_state.last_rtc >= GBSP_SAVE_RTC_INTERVAL_US);
    if (!gbsp_save_begin(&gp_save_state, now, force, quiet, rtc_due)) return;
    gp_save_job_t job = {.data=gp_save_state.flight_data, .rtc=gpsp_rtc_is_enabled(),
                         .len=gp_save_state.capacity};
    if (job.rtc) gpsp_clock_encode(job.metadata, gpsp_rtc_time());
    if (gp_save_task_h) {
        if (job.data) memcpy(gp_save_snap, gpsp_save_data(), job.len);
        if (xQueueSend(gp_save_jobs, &job, 0) != pdTRUE) {
            gp_save_last_ok = false;
            gbsp_save_complete(&gp_save_state, false, now);
        } else {
            RG_LOGD("battery save: background snapshot queued (data %u bytes, RTC %s)",
                    (unsigned)(job.data ? job.len : 0), job.rtc ? "yes" : "no");
        }
    } else {
        bool ok = gp_save_write(&job, gpsp_save_data());
        gp_save_last_ok = ok;
        gbsp_save_complete(&gp_save_state, ok, now);
        if (!ok) RG_LOGE("battery save: write failed; pending data retained for retry");
    }
}

static bool gp_save_flush_sync(void)
{
    if (!gp_save_enabled) return true;
    gp_save_collect();
    gp_save_last_ok = true;
    /* A lifecycle caller may unmount storage immediately afterwards. Never
     * return while a worker can still be inside a filesystem operation. */
    if (!gp_save_reap(portMAX_DELAY)) return false;
    gp_save_schedule(true, true);
    if (!gp_save_reap(portMAX_DELAY)) return false;
    return gp_save_last_ok && !gp_save_state.dirty;
}

static void gp_save_shutdown(void)
{
    while (!gp_save_flush_sync()) {
        if (!rg_gui_confirm(_("Save failed"), _("Could not save cartridge data or clock. Retry?"), true))
            break;
    }
}

static void gp_save_tick(bool quiet)
{
    if (!gp_save_enabled) return;
    gp_save_collect();
    if (!gp_save_reap(0)) return;
    /* Low-memory fallback never introduces SD writes into gameplay. */
    if (gp_save_task_h) gp_save_schedule(false, quiet);
}
