#include "world_worker.h"
#include <rg_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static StaticSemaphore_t go_storage, done_storage;
static SemaphoreHandle_t go, done;
static bool available;
static void (*job)(void *, int);
static void *job_arg;
static void worker(void *arg) {
    (void)arg;
    for (;;) {
        xSemaphoreTake(go, portMAX_DELAY);
        job(job_arg, 2);
        xSemaphoreGive(done);
    }
}
void q2_world_init(void) {
    go = xSemaphoreCreateBinaryStatic(&go_storage);
    done = xSemaphoreCreateBinaryStatic(&done_storage);
    available = go && done && rg_task_create("q2_world", worker, NULL, 4096, 0, RG_TASK_PRIORITY_1, 1);
    RG_LOGI("World rendering: %s", available ? "dual core" : "single-core fallback");
}
void QG_WorldDraw(void (*draw)(void *, int), void *arg, bool parallel) {
    // Engine is the sole producer. Both sides finish before this returns.
    if (!available || !parallel) { draw(arg, 0); return; }
    job = draw; job_arg = arg;
    xSemaphoreGive(go);
    draw(arg, 1);
    xSemaphoreTake(done, portMAX_DELAY);
    job = NULL; job_arg = NULL;
}
void q2_world_shutdown(void) {
    // Called with the engine quiescent. Worker stays blocked until app reboot;
    // static semaphores remain valid and it retains no engine-buffer pointers.
    available = false;
}
