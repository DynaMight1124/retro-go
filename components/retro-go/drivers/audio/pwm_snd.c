#include "rg_system.h"
#include "rg_audio.h"
#include <driver/ledc.h>
#include <driver/gptimer.h>
#include <soc/ledc_struct.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// GPTimer interrupts are allocated on the core that registers the callback.
// Core 1 keeps the high-rate PWM ISR away from emulators running on Core 0.
// Set to 0 or 1 to select a core, or -1 to allocate on the calling core.
// Override at build time if a target needs different task/core placement.
#ifndef RG_AUDIO_PWM_ISR_CORE
#define RG_AUDIO_PWM_ISR_CORE 1
#endif

#if !defined(CONFIG_FREERTOS_UNICORE) && RG_AUDIO_PWM_ISR_CORE >= 0
#include <esp_ipc.h>
#endif

#define RING_BUFFER_SIZE 2048
#define SUBMIT_BATCH_SIZE 64
#define AUDIO_TIMER_RESOLUTION_HZ 40000000
#define AUDIO_TEARDOWN_TASK_STACK 2048
static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint32_t ring_buffer_w = 0;
static volatile uint32_t ring_buffer_r = 0;

#define LEDC_PWM_SPEED_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_PWM_CHANNEL    LEDC_CHANNEL_1
#define LEDC_PWM_TIMER      LEDC_TIMER_1

static struct {
    const char *last_error;
    int volume;
    int volume_q15;
    bool muted;
} state;

static gptimer_handle_t audio_timer = NULL;
static uint32_t audio_timer_resolution_hz = AUDIO_TIMER_RESOLUTION_HZ;
static int audio_timer_interrupt_core = -1;

static uint64_t get_alarm_count(int sample_rate)
{
    if (sample_rate <= 0) return 0;
    return ((uint64_t)audio_timer_resolution_hz + (sample_rate / 2)) / sample_rate;
}

static bool IRAM_ATTR audio_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    uint32_t r = ring_buffer_r;
    uint32_t w = ring_buffer_w;
    uint8_t s;
    
    if (r != w) {
        s = ring_buffer[r & (RING_BUFFER_SIZE - 1)];
        ring_buffer_r = r + 1;
    } else {
        s = 0x40; // Midpoint silence
    }

    // Direct register write to LEDC channel 1 for speed (LEDC_HIGH_SPEED_MODE)
    // 7-bit resolution maps to s << 4 in the duty cycle register.
    LEDC.channel_group[0].channel[1].duty.duty = s << 4;
    LEDC.channel_group[0].channel[1].conf1.duty_start = 1;

    return false; // No context switch needed
}

#if !defined(CONFIG_FREERTOS_UNICORE) && RG_AUDIO_PWM_ISR_CORE >= 0
typedef struct {
    const gptimer_event_callbacks_t *callbacks;
    esp_err_t result;
    int core;
} audio_timer_registration_t;

static void register_audio_timer_callback_on_core(void *context)
{
    audio_timer_registration_t *call = context;
    call->core = xPortGetCoreID();
    call->result = gptimer_register_event_callbacks(audio_timer, call->callbacks, NULL);
}
#endif

static esp_err_t register_audio_timer_callback(const gptimer_event_callbacks_t *callbacks)
{
#if !defined(CONFIG_FREERTOS_UNICORE) && RG_AUDIO_PWM_ISR_CORE >= 0
    if (xPortGetCoreID() != RG_AUDIO_PWM_ISR_CORE) {
        audio_timer_registration_t args = {
            .callbacks = callbacks,
            .result = ESP_FAIL,
            .core = -1,
        };

        esp_err_t err = esp_ipc_call_blocking(RG_AUDIO_PWM_ISR_CORE, register_audio_timer_callback_on_core, &args);
        if (err != ESP_OK) return err;
        if (args.result == ESP_OK) {
            audio_timer_interrupt_core = args.core;
            RG_LOGI("PWM audio interrupt allocated on Core %d", args.core);
        }
        return args.result;
    }
#endif

    esp_err_t err = gptimer_register_event_callbacks(audio_timer, callbacks, NULL);
    if (err == ESP_OK) {
        audio_timer_interrupt_core = xPortGetCoreID();
        RG_LOGI("PWM audio interrupt allocated on Core %d", audio_timer_interrupt_core);
    }
    return err;
}

static esp_err_t destroy_audio_timer_on_current_core(void)
{
    esp_err_t err = gptimer_stop(audio_timer);
    if (err != ESP_OK) return err;

    err = gptimer_disable(audio_timer);
    if (err != ESP_OK) return err;

    err = gptimer_del_timer(audio_timer);
    if (err == ESP_OK) {
        audio_timer = NULL;
        audio_timer_interrupt_core = -1;
    }
    return err;
}

#if !defined(CONFIG_FREERTOS_UNICORE) && RG_AUDIO_PWM_ISR_CORE >= 0
typedef struct {
    SemaphoreHandle_t done;
    esp_err_t result;
} audio_timer_destroy_t;

static void destroy_audio_timer_task(void *context)
{
    audio_timer_destroy_t *call = context;
    call->result = destroy_audio_timer_on_current_core();
    xSemaphoreGive(call->done);
    vTaskDelete(NULL);
}
#endif

static esp_err_t destroy_audio_timer(void)
{
#if !defined(CONFIG_FREERTOS_UNICORE) && RG_AUDIO_PWM_ISR_CORE >= 0
    if (audio_timer_interrupt_core >= 0 && xPortGetCoreID() != audio_timer_interrupt_core) {
        StaticSemaphore_t done_storage;
        audio_timer_destroy_t call = {
            .done = xSemaphoreCreateBinaryStatic(&done_storage),
            .result = ESP_FAIL,
        };

        BaseType_t created = xTaskCreatePinnedToCore(
            destroy_audio_timer_task, "pwm_deinit", AUDIO_TEARDOWN_TASK_STACK,
            &call, configMAX_PRIORITIES - 1, NULL, audio_timer_interrupt_core);
        if (created != pdPASS) return ESP_ERR_NO_MEM;

        xSemaphoreTake(call.done, portMAX_DELAY);
        return call.result;
    }
#endif
    return destroy_audio_timer_on_current_core();
}

static bool driver_init(int device, int sample_rate) {
    state.last_error = NULL;
    state.volume = 100;
    state.volume_q15 = 1 << 15;
    state.muted = false;
    ring_buffer_r = 0;
    ring_buffer_w = 0;

    RG_LOGI("Initializing LEDC PWM Audio (ESP-IDF v5) on GPIO %d, sample_rate=%d...", RG_GPIO_AUDIO_OUT, sample_rate);

    // 1. Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_PWM_SPEED_MODE,
        .timer_num        = LEDC_PWM_TIMER,
        .duty_resolution  = LEDC_TIMER_7_BIT, // 7-bit resolution (0-127)
        .freq_hz          = 625000,            // 625 kHz
        .clk_cfg          = LEDC_USE_APB_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        state.last_error = "LEDC timer config failed";
        RG_LOGE("Failed to config LEDC timer: %s", esp_err_to_name(err));
        return false;
    }

    // 2. Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = RG_GPIO_AUDIO_OUT,
        .speed_mode     = LEDC_PWM_SPEED_MODE,
        .channel        = LEDC_PWM_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_PWM_TIMER,
        .duty           = 0x40,                // 50% duty cycle (midpoint)
        .hpoint         = 0
    };
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        state.last_error = "LEDC channel config failed";
        RG_LOGE("Failed to config LEDC channel: %s", esp_err_to_name(err));
        return false;
    }

    // 3. Configure General Purpose Timer using gptimer (ESP-IDF v5 style)
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = AUDIO_TIMER_RESOLUTION_HZ,
        .intr_priority = 1,
    };
    err = gptimer_new_timer(&timer_config, &audio_timer);
    if (err != ESP_OK) {
        state.last_error = "gptimer_new_timer failed";
        RG_LOGE("Failed to create gptimer: %s", esp_err_to_name(err));
        return false;
    }

    err = gptimer_get_resolution(audio_timer, &audio_timer_resolution_hz);
    if (err != ESP_OK) {
        state.last_error = "gptimer_get_resolution failed";
        RG_LOGE("Failed to get GPTimer resolution: %s", esp_err_to_name(err));
        gptimer_del_timer(audio_timer);
        audio_timer = NULL;
        return false;
    }

    gptimer_event_callbacks_t cbs = {
        .on_alarm = audio_timer_on_alarm_cb,
    };
    err = register_audio_timer_callback(&cbs);
    if (err != ESP_OK) {
        state.last_error = "gptimer callback registration failed";
        RG_LOGE("Failed to register GPTimer callback: %s", esp_err_to_name(err));
        gptimer_del_timer(audio_timer);
        audio_timer = NULL;
        return false;
    }
    gptimer_enable(audio_timer);

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = get_alarm_count(sample_rate),
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(audio_timer, &alarm_config);
    gptimer_start(audio_timer);

    return true;
}

static bool driver_set_volume(int volume) {
    volume = RG_MIN(RG_MAX(volume, 0), 100);
    state.volume = volume;
    state.volume_q15 = (volume * (1 << 15) + 50) / 100;
    return true;
}

static bool driver_set_mute(bool mute) {
    state.muted = mute;
    if (mute) {
        ring_buffer_r = ring_buffer_w;
    }
    return true;
}

static bool driver_set_sample_rates(int sample_rate) {
    if (!audio_timer || sample_rate <= 0) return false;
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = get_alarm_count(sample_rate),
        .flags.auto_reload_on_alarm = true,
    };
    return gptimer_set_alarm_action(audio_timer, &alarm_config) == ESP_OK;
}

static bool driver_submit(const rg_audio_frame_t *frames, size_t count) {
    const int32_t volume_q15 = state.muted ? 0 : state.volume_q15;
    size_t offset = 0;

    while (offset < count) {
        size_t batch = RG_MIN(count - offset, SUBMIT_BATCH_SIZE);
        uint32_t w;

        while (true) {
            w = ring_buffer_w;
            uint32_t used = w - ring_buffer_r;
            if (used < RING_BUFFER_SIZE && (RING_BUFFER_SIZE - 1 - used) >= batch) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        for (size_t i = 0; i < batch; ++i) {
            const rg_audio_frame_t *frame = &frames[offset + i];
            int32_t sample = ((int32_t)frame->left + (int32_t)frame->right) >> 1;
            int32_t scaled = sample * volume_q15;

            // Match C float-to-int conversion by truncating negative values
            // toward zero after the Q15 volume multiplication.
            if (scaled < 0) scaled += (1 << 15) - 1;
            sample = scaled >> 15;

            // volume_q15 is limited to unity, so clipping is unnecessary.
            ring_buffer[w & (RING_BUFFER_SIZE - 1)] = (sample + 32768) >> 9;
            w++;
        }

        // Publish completed samples in small batches to reduce volatile index
        // traffic without delaying an entire frame's worth of audio.
        ring_buffer_w = w;
        offset += batch;
    }
    return true;
}

static bool driver_deinit(void) {
    bool success = true;
    if (audio_timer) {
        RG_LOGI("Releasing PWM audio timer on Core %d", audio_timer_interrupt_core);
        esp_err_t err = destroy_audio_timer();
        if (err != ESP_OK) {
            state.last_error = "gptimer teardown failed";
            RG_LOGE("Failed to release GPTimer: %s", esp_err_to_name(err));
            success = false;
        }
    }

    // Stop LEDC
    ledc_stop(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, 0);
    gpio_reset_pin(RG_GPIO_AUDIO_OUT);
    return success;
}

static const char *driver_get_error(void) {
    return state.last_error;
}

const rg_audio_driver_t rg_audio_driver_pwm_snd = {
    .name = "pwm_snd",
    .init = driver_init,
    .deinit = driver_deinit,
    .submit = driver_submit,
    .set_mute = driver_set_mute,
    .set_volume = driver_set_volume,
    .set_sample_rate = driver_set_sample_rates,
    .get_error = driver_get_error,
};
