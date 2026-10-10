#include "lifecycle.h"
#include <rg_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

typedef struct { int action; const char *message; } request_t;
static QueueHandle_t requests;
static SemaphoreHandle_t completed;
static TaskHandle_t owner;
static void (*dispatch)(int, const char *);

void q2_control_init(void (*handler)(int, const char *))
{
    owner = xTaskGetCurrentTaskHandle();
    dispatch = handler;
    requests = xQueueCreate(1, sizeof(request_t));
    completed = xSemaphoreCreateBinary();
    RG_ASSERT(requests && completed && dispatch, "Lifecycle allocation failed");
}

void q2_control_request(int action, const char *message)
{
    // One engine producer. Its stack/message and engine state remain stable
    // until main finishes a resumable action, or restarts for an exit action.
    if (xTaskGetCurrentTaskHandle() == owner) {
        dispatch(action, message);
        return;
    }
    request_t request = {action, message};
    RG_ASSERT(xQueueSend(requests, &request, portMAX_DELAY) == pdTRUE, "Lifecycle request failed");
    xSemaphoreTake(completed, portMAX_DELAY);
}

_Noreturn void q2_control_run(void)
{
    RG_ASSERT(xTaskGetCurrentTaskHandle() == owner, "Lifecycle must run on main stack");
    for (;;) {
        request_t request;
        if (xQueueReceive(requests, &request, portMAX_DELAY) == pdTRUE) {
            dispatch(request.action, request.message);
            xSemaphoreGive(completed);
        }
    }
}
