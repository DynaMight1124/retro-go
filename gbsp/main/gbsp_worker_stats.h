#pragma once
#include "gpsp_video.h"

#if !RG_BUILD_RELEASE
/* Minimum unused stack over each worker's lifetime; -1 means no worker.
 * A null FreeRTOS handle means the caller, so never query it here. */
static void gp_worker_report(int render, int save)
{
    static int last_render = -2, last_save = -2;
    if (render == last_render && save == last_save) return;
    RG_LOGD("stacks: min-free bytes, render %d/6144, save %d/4096 (-1 = off)", render, save);
    last_render = render;
    last_save = save;
}
#define GP_PERF_WORKERS() gp_worker_report(gbsp_render_stack_free(), \
    gp_save_task_h ? (int)(uxTaskGetStackHighWaterMark(gp_save_task_h) * sizeof(StackType_t)) : -1)
#endif
