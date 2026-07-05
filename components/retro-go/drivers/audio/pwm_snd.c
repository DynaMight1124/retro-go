#include "rg_system.h"
#include "rg_audio.h"
#include <driver/ledc.h>
#include <driver/gptimer.h>
#include <soc/ledc_struct.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define RING_BUFFER_SIZE 2048
static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint32_t ring_buffer_w = 0;
static volatile uint32_t ring_buffer_r = 0;

#define LEDC_PWM_SPEED_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_PWM_CHANNEL    LEDC_CHANNEL_1
#define LEDC_PWM_TIMER      LEDC_TIMER_1

static struct {
    const char *last_error;
    int volume;
    bool muted;
} state;

static gptimer_handle_t audio_timer = NULL;

static bool IRAM_ATTR audio_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    uint32_t r = ring_buffer_r;
    uint32_t w = ring_buffer_w;
    uint8_t s;
    
    if (r < w) {
        s = ring_buffer[r & (RING_BUFFER_SIZE - 1)];
        ring_buffer_r = r + 1;
    } else {
        s = 0x40; // Midpoint silence
    }

    // Direct register write to LEDC channel 1 for speed (LEDC_HIGH_SPEED_MODE)
    // 7-bit resolution maps to s << 4 in the duty cycle register.
    LEDC.channel_group[0].channel[1].duty.duty = s << 4;
    LEDC.channel_group[0].channel[1].conf0.sig_out_en = 1;
    LEDC.channel_group[0].channel[1].conf1.duty_start = 1;

    return false; // No context switch needed
}

static bool driver_init(int device, int sample_rate) {
    state.last_error = NULL;
    state.volume = 100;
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
        .resolution_hz = 1000000, // 1MHz, 1 tick = 1us
    };
    err = gptimer_new_timer(&timer_config, &audio_timer);
    if (err != ESP_OK) {
        state.last_error = "gptimer_new_timer failed";
        RG_LOGE("Failed to create gptimer: %s", esp_err_to_name(err));
        return false;
    }

    gptimer_event_callbacks_t cbs = {
        .on_alarm = audio_timer_on_alarm_cb,
    };
    gptimer_register_event_callbacks(audio_timer, &cbs, NULL);
    gptimer_enable(audio_timer);

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000 / sample_rate, // 1us tick resolution
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(audio_timer, &alarm_config);
    gptimer_start(audio_timer);

    return true;
}

static bool driver_set_volume(int volume) {
    state.volume = volume;
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
    if (!audio_timer) return false;
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000 / sample_rate,
        .flags.auto_reload_on_alarm = true,
    };
    return gptimer_set_alarm_action(audio_timer, &alarm_config) == ESP_OK;
}

static bool IRAM_ATTR driver_submit(const rg_audio_frame_t *frames, size_t count) {
    float volume = state.muted ? 0.f : (state.volume * 0.01f);

    size_t needed_space = count;
    if (needed_space >= RING_BUFFER_SIZE) {
        needed_space = RING_BUFFER_SIZE - 1;
    }

    while ((RING_BUFFER_SIZE - 1 - (ring_buffer_w - ring_buffer_r)) < needed_space) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    for (size_t i = 0; i < count; ++i) {
        int sample = ((int)frames[i].left + (int)frames[i].right) >> 1;
        sample = sample * volume;

        // Clip to 16-bit signed
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;

        // Map range [-32768, 32767] to [0, 127] for 7-bit resolution
        int32_t val = (sample + 32768) >> 9;
        if (val > 127) val = 127;
        else if (val < 0) val = 0;

        ring_buffer[ring_buffer_w & (RING_BUFFER_SIZE - 1)] = val;
        ring_buffer_w++;
    }
    return true;
}

static bool driver_deinit(void) {
    if (audio_timer) {
        gptimer_stop(audio_timer);
        gptimer_disable(audio_timer);
        gptimer_del_timer(audio_timer);
        audio_timer = NULL;
    }

    // Stop LEDC
    ledc_stop(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, 0);
    gpio_reset_pin(RG_GPIO_AUDIO_OUT);
    return true;
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
