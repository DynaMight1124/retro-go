#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/i2s.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <driver/dac.h>
#pragma GCC diagnostic pop
#include <driver/rtc_io.h>
#include <soc/rtc.h>
#include <soc/i2s_struct.h>
#include <soc/i2s_reg.h>
#include <rom/lldesc.h>
#include <string.h>
#include <math.h>
#include "rg_system.h"
#include "esp_private/periph_ctrl.h"

// Enforce mode for rg_display.c
#undef LCD_ACCESS_MODE
#define LCD_ACCESS_MODE   0 
#undef LCD_BUFFER_LENGTH
#define LCD_BUFFER_LENGTH (RG_SCREEN_WIDTH * 8)

#define CHUNK_LINES           8
#define NUM_DMA_BUFFERS       16

#define SAMPLES_PER_LINE (RG_TVOUT_STANDARD == 1 ? 1136 : 912)
#define ACTIVE_START     (RG_TVOUT_STANDARD == 1 ? 184 : 144)
#define ACTIVE_SAMPLES   (RG_TVOUT_STANDARD == 1 ? 896 : 720)
#define ACTIVE_END       (ACTIVE_START + ACTIVE_SAMPLES)

// EXACT BITLUNI IRE MATH (Scaled down to 8-bit for interleaving)
#define SYNC_SIZE        40
#define IRE(_x)          ((uint16_t)(((_x)+SYNC_SIZE)*255.0f/3.3f/147.5f) << 8)
#define SYNC_8BIT        (IRE(-40) >> 8)
#define BLANK_8BIT       (IRE(0) >> 8)
#define BLACK_8BIT       (IRE(7.5f) >> 8)
#define WHITE_8BIT       (IRE(100) >> 8)

static uint8_t *screen_buffer = NULL;
static bool screen_buffer_internal = false;
static lldesc_t *dma_desc = NULL;
static uint16_t *dma_buffers[NUM_DMA_BUFFERS] = {NULL};

static uint16_t *normal_template = NULL;
static uint16_t *equal_template = NULL;
static uint16_t *vsync_template = NULL;

static uint16_t *pal_burst0 = NULL;
static uint16_t *pal_burst1 = NULL;

static int burst_start = 0;
static int burst_width = 0;

static uint32_t color_lut_even_s0[256];
static uint32_t color_lut_even_s1[256];
static uint32_t color_lut_even_s2[256];
static uint32_t color_lut_even_s3[256];

static uint32_t color_lut_odd_s0[256];
static uint32_t color_lut_odd_s1[256];
static uint32_t color_lut_odd_s2[256];
static uint32_t color_lut_odd_s3[256];
#if RG_SCREEN_WIDTH <= 256
typedef uint8_t pixel_map_t;
#else
typedef uint16_t pixel_map_t;
#endif
static pixel_map_t pixel_map[896];

static intr_handle_t isr_handle = NULL;
static TaskHandle_t pump_task_handle = NULL;
static SemaphoreHandle_t vblank_sem = NULL;

static void IRAM_ATTR video_isr_handler(void *arg) {
    if (I2S0.int_st.out_eof) {
        if (pump_task_handle) {
            BaseType_t woken = pdFALSE;
            vTaskNotifyGiveFromISR(pump_task_handle, &woken);
            if (woken) portYIELD_FROM_ISR();
        }
    }
    I2S0.int_clr.val = I2S0.int_st.val;
}

static void IRAM_ATTR fill_chunk(int chunk_idx, int *line_counter) {
    int start_buf = chunk_idx * CHUNK_LINES;
    bool is_pal = (RG_TVOUT_STANDARD == 1);
    int active_line_start = is_pal ? (166 - RG_SCREEN_HEIGHT / 2) : 22;
    int total_lines = is_pal ? 312 : 262;


    for (int i = 0; i < CHUNK_LINES; i++) {
        int line = *line_counter;
        (*line_counter)++;
        if (*line_counter >= total_lines) {
            *line_counter = 0;
        }
        if (*line_counter == 0) {
            if (vblank_sem) {
                xSemaphoreGive(vblank_sem);
            }
        }
        uint16_t *buf = dma_buffers[start_buf + i];

        if (is_pal) {
            // PAL Sync & Active Video
            if (line < 3 || (line >= 6 && line < 9)) {
                memcpy(buf, equal_template, SAMPLES_PER_LINE * 2);
            } else if (line >= 3 && line < 6) {
                memcpy(buf, vsync_template, SAMPLES_PER_LINE * 2);
            } else {
                int video_y = line - active_line_start;
                bool active = video_y >= 0 && video_y < RG_SCREEN_HEIGHT && screen_buffer;

                if (active) {
                    // The active region is replaced below, so only copy the
                    // sync/burst prefix and the trailing blanking samples.
                    memcpy(buf, normal_template, ACTIVE_START * sizeof(*buf));
                    memcpy(buf + ACTIVE_END, normal_template + ACTIVE_END,
                           (SAMPLES_PER_LINE - ACTIVE_END) * sizeof(*buf));
                } else {
                    memcpy(buf, normal_template, SAMPLES_PER_LINE * sizeof(*buf));
                }
                if (pal_burst0 && pal_burst1) {
                    const uint16_t *burst_src = (line & 1) ? pal_burst1 : pal_burst0;
                    memcpy(buf + burst_start, burst_src, burst_width * 2);
                }
                if (active) {
                    uint8_t line_temp[RG_SCREEN_WIDTH];
                    const uint8_t *src = screen_buffer + (video_y * RG_SCREEN_WIDTH);
                    if (!screen_buffer_internal) {
                        memcpy(line_temp, src, RG_SCREEN_WIDTH);
                        src = line_temp;
                    }
                    uint16_t *dst = buf + ACTIVE_START;
                    const pixel_map_t *map_ptr = pixel_map;
                    uint32_t *lut_s0 = (line & 1) ? color_lut_odd_s0 : color_lut_even_s0;
                    uint32_t *lut_s1 = (line & 1) ? color_lut_odd_s1 : color_lut_even_s1;
                    uint32_t *lut_s2 = (line & 1) ? color_lut_odd_s2 : color_lut_even_s2;
                    uint32_t *lut_s3 = (line & 1) ? color_lut_odd_s3 : color_lut_even_s3;
                    int active_cc = 224; // 896 samples / 4
                    for (int cc = 0; cc < active_cc; cc++) {
                        uint8_t c0 = src[*map_ptr++];
                        uint8_t c1 = src[*map_ptr++];
                        uint8_t c2 = src[*map_ptr++];
                        uint8_t c3 = src[*map_ptr++];

                        uint32_t w0 = lut_s0[c0] | lut_s1[c1];
                        uint32_t w1 = lut_s2[c2] | lut_s3[c3];
                        ((uint32_t *)dst)[0] = w0;
                        ((uint32_t *)dst)[1] = w1;

                        dst += 4;
                    }
                }
            }
        } else {
            // NTSC Sync & Active Video (Simplified progressive VSync)
            if (line >= 3 && line < 6) {
                memcpy(buf, vsync_template, SAMPLES_PER_LINE * 2);
            } else {
                int video_y = line - active_line_start;
                bool active = video_y >= 0 && video_y < RG_SCREEN_HEIGHT && screen_buffer;

                if (active) {
                    // The active region is replaced below, so only copy the
                    // sync/burst prefix and the trailing blanking samples.
                    memcpy(buf, normal_template, ACTIVE_START * sizeof(*buf));
                    memcpy(buf + ACTIVE_END, normal_template + ACTIVE_END,
                           (SAMPLES_PER_LINE - ACTIVE_END) * sizeof(*buf));
                } else {
                    memcpy(buf, normal_template, SAMPLES_PER_LINE * sizeof(*buf));
                }
                if (active) {
                    uint8_t line_temp[RG_SCREEN_WIDTH];
                    const uint8_t *src = screen_buffer + (video_y * RG_SCREEN_WIDTH);
                    if (!screen_buffer_internal) {
                        memcpy(line_temp, src, RG_SCREEN_WIDTH);
                        src = line_temp;
                    }
                    uint16_t *dst = buf + ACTIVE_START;
                    const pixel_map_t *map_ptr = pixel_map;
                    int active_cc = 180; // 720 samples / 4
                    for (int cc = 0; cc < active_cc; cc++) {
                        uint8_t c0 = src[*map_ptr++];
                        uint8_t c1 = src[*map_ptr++];
                        uint8_t c2 = src[*map_ptr++];
                        uint8_t c3 = src[*map_ptr++];

                        uint32_t w0 = color_lut_even_s0[c0] | color_lut_even_s1[c1];
                        uint32_t w1 = color_lut_even_s2[c2] | color_lut_even_s3[c3];
                        ((uint32_t *)dst)[0] = w0;
                        ((uint32_t *)dst)[1] = w1;

                        dst += 4;
                    }
                }
            }
        }
    }
}

static void IRAM_ATTR tvout_signal_pump_task(void *arg) {
    int chunk_idx = 0; 
    int line_counter = 0;
    
    fill_chunk(0, &line_counter);
    fill_chunk(1, &line_counter);

    I2S0.conf.tx_start = 1;
    I2S0.int_clr.val = 0xFFFFFFFF;
    I2S0.int_ena.out_eof = 1;
    I2S0.out_link.start = 1;
    RG_LOGI("Raw TVout Engine Active (60fps Color Output).");

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        fill_chunk(chunk_idx, &line_counter);
        chunk_idx = (chunk_idx + 1) % 2;
    }
}

static void generate_luts(void) {
    bool is_pal = (RG_TVOUT_STANDARD == 1);
    float hue_angle = is_pal ? -0.2f : 0.0f; 
    float saturation = 0.35f;
    
    for (int c = 0; c < 256; c++) {
        int r_val = (c >> 5) & 0x7;
        int g_val = (c >> 2) & 0x7;
        int b_val = c & 0x3;
        
        float r = r_val / 7.0f;
        float g = g_val / 7.0f;
        float b = b_val / 3.0f;
        
        // RGB to YUV
        float y = 0.299f * r + 0.587f * g + 0.114f * b;
        float u = -0.14713f * r - 0.28886f * g + 0.436f * b;
        float v = 0.615f * r - 0.51499f * g - 0.10001f * b;
        
        float u_scaled = u * saturation * 44.0f;
        float v_scaled = v * saturation * 44.0f;
        
        float luma = 25.0f + y * 44.0f;
        
        for (int phase = 0; phase < 4; phase++) {
            float rad = phase * (M_PI / 2.0f) + hue_angle;
            
            // Even lines (Standard NTSC/PAL)
            float val_even = luma + u_scaled * sinf(rad) + v_scaled * cosf(rad);
            int sample_even = (int)val_even;
            if (sample_even < 21) sample_even = 21; // Clamp below blanking level
            if (sample_even > 254) sample_even = 254;
            
            // Odd lines (PAL phase alternation)
            float val_odd = luma + u_scaled * sinf(rad) - v_scaled * cosf(rad);
            int sample_odd = (int)val_odd;
            if (sample_odd < 21) sample_odd = 21;
            if (sample_odd > 254) sample_odd = 254;

            if (phase == 0) {
                color_lut_even_s0[c] = (uint32_t)sample_even << 24;
                color_lut_odd_s0[c] = (uint32_t)sample_odd << 24;
            } else if (phase == 1) {
                color_lut_even_s1[c] = (uint32_t)sample_even << 8;
                color_lut_odd_s1[c] = (uint32_t)sample_odd << 8;
            } else if (phase == 2) {
                color_lut_even_s2[c] = (uint32_t)sample_even << 24;
                color_lut_odd_s2[c] = (uint32_t)sample_odd << 24;
            } else if (phase == 3) {
                color_lut_even_s3[c] = (uint32_t)sample_even << 8;
                color_lut_odd_s3[c] = (uint32_t)sample_odd << 8;
            }
        }
    }
}

static void lcd_init_hw(void *arg) {
    RG_LOGI("Initializing Raw TVout Hardware...");

    bool is_pal = (RG_TVOUT_STANDARD == 1);
    uint32_t caps = MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    
    generate_luts();

    int active_width = is_pal ? 896 : 720;
    for (int t = 0; t < active_width; t++) {
        pixel_map[t] = (t * RG_SCREEN_WIDTH) / active_width;
    }

    int samples = is_pal ? 1136 : 912;
    int half_line = samples / 2;
    int hsync = is_pal ? 84 : 64;
    int eq_width = is_pal ? 40 : 32;
    int vsync_width = half_line - eq_width;
    burst_start = hsync + (is_pal ? 12 : 8);
    burst_width = is_pal ? 40 : 36;

    normal_template = heap_caps_malloc(samples * 2, caps);
    equal_template = heap_caps_malloc(samples * 2, caps);
    vsync_template = heap_caps_malloc(samples * 2, caps);

    if (!normal_template || !equal_template || !vsync_template) {
        RG_LOGE("Failed to allocate sync templates in internal RAM!");
        vTaskDelete(NULL);
        return;
    }

    for (int i = 0; i < samples; i++) {
        uint16_t val;
        // Equalization template (short sync pulses)
        val = (i < eq_width || (i >= half_line && i < half_line + eq_width)) ? SYNC_8BIT : BLANK_8BIT;
        equal_template[i] = val << 8;

        // VSync template (long sync pulses)
        if (is_pal) {
            val = (i < vsync_width || (i >= half_line && i < samples - eq_width)) ? SYNC_8BIT : BLANK_8BIT;
        } else {
            val = (i < samples - hsync) ? SYNC_8BIT : BLANK_8BIT;
        }
        vsync_template[i] = val << 8;

        // Normal template (standard HSync pulse)
        val = (i < hsync) ? SYNC_8BIT : BLANK_8BIT;
        normal_template[i] = val << 8;
    }
    
    // Add Color Burst to normal template (NTSC standard -U phase)
    if (!is_pal) {
        for (int i = burst_start; i < burst_start + burst_width; i += 4) {
            normal_template[(i+0)^1] = (BLANK_8BIT) << 8;
            normal_template[(i+1)^1] = (BLANK_8BIT - BLANK_8BIT / 2) << 8;
            normal_template[(i+2)^1] = (BLANK_8BIT) << 8;
            normal_template[(i+3)^1] = (BLANK_8BIT + BLANK_8BIT / 2) << 8;
        }
    } else {
        pal_burst0 = heap_caps_malloc(burst_width * 2, caps);
        pal_burst1 = heap_caps_malloc(burst_width * 2, caps);
        if (pal_burst0 && pal_burst1) {
            float phase0 = 0.0f;
            float phase1 = 0.0f;
            for (int i = 0; i < burst_width; i += 2) {
                float val0_even = BLANK_8BIT + sinf(phase0 + 3.0f * M_PI / 4.0f) * BLANK_8BIT / 1.5f;
                phase0 += 2.0f * M_PI / 4.0f;
                float val0_odd  = BLANK_8BIT + sinf(phase0 + 3.0f * M_PI / 4.0f) * BLANK_8BIT / 1.5f;
                phase0 += 2.0f * M_PI / 4.0f;
                
                float val1_even = BLANK_8BIT + sinf(phase1 - 3.0f * M_PI / 4.0f) * BLANK_8BIT / 1.5f;
                phase1 += 2.0f * M_PI / 4.0f;
                float val1_odd  = BLANK_8BIT + sinf(phase1 - 3.0f * M_PI / 4.0f) * BLANK_8BIT / 1.5f;
                phase1 += 2.0f * M_PI / 4.0f;
                
                pal_burst0[i ^ 1] = ((int)val0_even) << 8;
                pal_burst0[(i + 1) ^ 1] = ((int)val0_odd) << 8;
                
                pal_burst1[i ^ 1] = ((int)val1_even) << 8;
                pal_burst1[(i + 1) ^ 1] = ((int)val1_odd) << 8;
            }
        }
    }

    dma_desc = heap_caps_malloc(NUM_DMA_BUFFERS * sizeof(lldesc_t), caps);
    if (!dma_desc) {
        RG_LOGE("Failed to allocate DMA descriptors in internal RAM!");
        vTaskDelete(NULL);
        return;
    }
    for (int i = 0; i < NUM_DMA_BUFFERS; i++) {
        dma_buffers[i] = heap_caps_malloc(samples * 2, caps);
        if (!dma_buffers[i]) {
            RG_LOGE("Failed to allocate DMA buffer %d in internal RAM!", i);
            vTaskDelete(NULL);
            return;
        }
        dma_desc[i].buf = (uint8_t *)(dma_buffers[i]);
        dma_desc[i].owner = 1;
        dma_desc[i].eof = ((i + 1) % CHUNK_LINES == 0) ? 1 : 0; 
        dma_desc[i].length = samples * 2;
        dma_desc[i].size = samples * 2;
        dma_desc[i].empty = (uint32_t)(&dma_desc[(i + 1) % NUM_DMA_BUFFERS]);
    }

    // Hardware Hack Configuration
    periph_module_enable(PERIPH_I2S0_MODULE);
    I2S0.conf.val = 1; I2S0.conf.val = 0;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_mono = 1;
    I2S0.conf2.lcd_en = 1;
    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.sample_rate_conf.tx_bits_mod = 16;
    I2S0.conf_chan.tx_chan_mod = 1;
    I2S0.fifo_conf.tx_fifo_mod = 1;
    I2S0.out_link.addr = (uint32_t)dma_desc;

    // High-Precision APLL Setup
    if (is_pal) {
        rtc_clk_apll_enable(true);
        rtc_clk_apll_coeff_set(1, 4, 164, 6); // PAL 17.734MHz (o_div=1, sdm0=4, sdm1=164, sdm2=6)
    } else {
        rtc_clk_apll_enable(true);
        rtc_clk_apll_coeff_set(1, 70, 151, 4); // NTSC 14.318MHz (o_div=1, sdm0=70, sdm1=151, sdm2=4)
    }
    I2S0.clkm_conf.clkm_div_num = 1; 
    I2S0.clkm_conf.clkm_div_b = 0;
    I2S0.clkm_conf.clkm_div_a = 1;
    I2S0.sample_rate_conf.tx_bck_div_num = 1;
    I2S0.clkm_conf.clka_en = 1;
    vTaskDelay(pdMS_TO_TICKS(50)); // Allow APLL calibration loop to lock stably

    dac_output_enable(DAC_CHAN_0); 
    dac_i2s_enable();

    esp_err_t err = esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, video_isr_handler, NULL, &isr_handle);
    if (err != ESP_OK) {
        RG_LOGE("Failed to allocate interrupt for I2S0! error=%s", esp_err_to_name(err));
    }
    BaseType_t res = xTaskCreatePinnedToCore(tvout_signal_pump_task, "tv_pump", 4096, NULL, 15, &pump_task_handle, 1);
    if (res != pdPASS) {
        RG_LOGE("Failed to create tv_pump task! error=%d", res);
    } else {
        RG_LOGI("Successfully created tv_pump task.");
    }
    vTaskDelete(NULL);
}

static void lcd_init(void) {
    vblank_sem = xSemaphoreCreateBinary();
    screen_buffer = heap_caps_malloc(RG_SCREEN_WIDTH * RG_SCREEN_HEIGHT, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    screen_buffer_internal = screen_buffer != NULL;
    if (!screen_buffer) {
        RG_LOGW("Failed to allocate 8-bit screen buffer in internal RAM, falling back to PSRAM.");
        screen_buffer = rg_alloc(RG_SCREEN_WIDTH * RG_SCREEN_HEIGHT, MEM_ANY);
    }
    memset(screen_buffer, 0, RG_SCREEN_WIDTH * RG_SCREEN_HEIGHT);
    xTaskCreatePinnedToCore(lcd_init_hw, "lcd_hw", 4096, NULL, 5, NULL, 1);
}

static void lcd_sync(void) {
    if (vblank_sem) {
        const char *name = pcTaskGetName(NULL);
        if (name && strcmp(name, "rg_display") == 0) {
            xSemaphoreTake(vblank_sem, pdMS_TO_TICKS(100));
        }
    }
}
static void lcd_deinit(void) { I2S0.conf.tx_start = 0; }
static void lcd_set_rotation(int rotation) {}
static void lcd_set_backlight(float percent) {}

static int window_x = 0;
static int window_y = 0;
static int window_w = 0;
static int window_h = 0;
static int window_col = 0;
static int window_row = 0;

static void lcd_set_window(int left, int top, int width, int height) {
    window_x = left;
    window_y = top;
    window_w = width;
    window_h = height;
    window_col = 0;
    window_row = 0;
}

static inline uint16_t *lcd_get_buffer(size_t length) {
    static uint16_t temp_buffer[LCD_BUFFER_LENGTH];
    return temp_buffer;
}

static inline void lcd_send_buffer(uint16_t *buffer, size_t length) {
    if (!screen_buffer || window_w <= 0) return;

    size_t offset = 0;
    while (offset < length) {
        size_t span = RG_MIN(length - offset, (size_t)(window_w - window_col));
        int dst_x = window_x + window_col;
        int dst_y = window_y + window_row;

        if (dst_y >= 0 && dst_y < RG_SCREEN_HEIGHT) {
            int first = dst_x < 0 ? -dst_x : 0;
            int last = span;
            if (dst_x + last > RG_SCREEN_WIDTH) {
                last = RG_SCREEN_WIDTH - dst_x;
            }

            if (first < last) {
                const uint16_t *src = buffer + offset + first;
                uint8_t *dst = screen_buffer + (dst_y * RG_SCREEN_WIDTH) + dst_x + first;
                int count = last - first;

                for (int i = 0; i < count; i++) {
                    uint16_t pixel = src[i];
#if RG_SCREEN_PIXEL_FORMAT == 1 // Little Endian
                    uint8_t b0 = pixel & 0xFF;
                    uint8_t b1 = pixel >> 8;
                    dst[i] = (b1 & 0xE0) | ((b1 & 0x07) << 2) | ((b0 >> 3) & 0x03);
#else // Big Endian (Avoids byte-swapping in hot path)
                    uint8_t b0 = pixel & 0xFF;
                    uint8_t b1 = pixel >> 8;
                    dst[i] = (b0 & 0xE0) | ((b0 & 0x07) << 2) | ((b1 >> 3) & 0x03);
#endif
                }
            }
        }

        offset += span;
        window_col += span;
        if (window_col >= window_w) {
            window_col = 0;
            window_row++;
        }
    }
}

static inline uint16_t *lcd_get_buffer_ptr(int left, int top) {
    if (!screen_buffer) return NULL;
    return (uint16_t *)(screen_buffer + (top * RG_SCREEN_WIDTH) + left);
}

const rg_display_driver_t rg_display_driver_tvout = { .name = "tvout" };
