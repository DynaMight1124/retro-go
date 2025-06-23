#include "rg_system.h"
#include "rg_i2c.h"

#include <stdlib.h>
#include <stdbool.h> 
#include <stdint.h>  

#if defined(ESP_PLATFORM) && defined(RG_GPIO_I2C_SDA) && defined(RG_GPIO_I2C_SCL)
#include <driver/i2c.h>
#include <esp_err.h>
#define USE_I2C_DRIVER 1
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#define USE_I2C_DRIVER 0
#ifndef RG_LOGI
#include <stdio.h>
#define RG_LOGI(format, ...) printf("[INFO] (%s:%d) " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define RG_LOGW(format, ...) printf("[WARN] (%s:%d) " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define RG_LOGE(format, ...) printf("[ERROR] (%s:%d) " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef rg_usleep
#define rg_usleep(us) ((void)0)
#endif
typedef int esp_err_t; 
const char* esp_err_to_name(esp_err_t err) { static const char* na = "N/A"; return na; }
#define ESP_OK 0
#define I2C_NUM_0 0 
#define GPIO_PULLUP_ENABLE 1 
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms) 
#endif
typedef struct { int mode; int sda_io_num; int scl_io_num; int sda_pullup_en; int scl_pullup_en; struct { uint32_t clk_speed; } master; } i2c_config_t;
#define I2C_MODE_MASTER 0
#define I2C_MASTER_WRITE true
#define I2C_MASTER_READ true
#define I2C_MASTER_LAST_NACK 0
typedef void* i2c_cmd_handle_t;
#if !USE_I2C_DRIVER 
esp_err_t i2c_param_config(int i, const i2c_config_t *c){return ESP_OK;}
esp_err_t i2c_driver_install(int i, int m, size_t s1, size_t s2, int f){return ESP_OK;}
esp_err_t i2c_driver_delete(int i){return ESP_OK;}
i2c_cmd_handle_t i2c_cmd_link_create(void){return (i2c_cmd_handle_t)1;}
void i2c_cmd_link_delete(i2c_cmd_handle_t c){}
esp_err_t i2c_master_start(i2c_cmd_handle_t c){return ESP_OK;}
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t c, uint8_t d, bool b){return ESP_OK;}
esp_err_t i2c_master_write(i2c_cmd_handle_t c, const uint8_t *d, size_t l, bool b){return ESP_OK;}
esp_err_t i2c_master_read(i2c_cmd_handle_t c, uint8_t *d, size_t l, int a){return ESP_OK;}
esp_err_t i2c_master_stop(i2c_cmd_handle_t c){return ESP_OK;}
esp_err_t i2c_master_cmd_begin(int i, i2c_cmd_handle_t c, TickType_t t){return ESP_OK;}
#endif
#endif


static bool i2c_initialized = false;
static bool gpio_extender_initialized = false;
static uint8_t gpio_extender_address = 0x00;
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // For PCF8575
static uint16_t pcf8575_port_cache = 0xFFFF; // Cache for PCF8575 RMW operations if needed for output later
#endif

#if USE_I2C_DRIVER 
#define TRY(x)                 \
    if ((err = (x)) != ESP_OK) \
    {                          \
        goto fail;             \
    }
#endif

// --- PCF8575 Default Address ---
#define PCF8575_ADDRESS_DEFAULT 0x20 // Common PCF8575 address if A0-A2 are LOW

// --- Register Defines based on RG_I2C_DRIVER ---
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    // PCF8575 doesn't use registers like AW9523. These are conceptual for helpers.
    #define AW9523_REG_INPUT0     0 // Conceptual: Port 0 for PCF8575
    #define AW9523_REG_OUTPUT0    0 // Conceptual: Port 0 for PCF8575
    #define AW9523_REG_POLARITY0  0 // Not applicable to PCF8575
    #define AW9523_REG_CONFIG0    0 // Not applicable to PCF8575
#else // AW9523 @ 0x58 Path (RG_I2C_DRIVER == 0 or undefined)
    #define AW9523_REG_CHIPID     0x10
    #define AW9523_REG_SOFTRESET  0x7F
    #define AW9523_REG_INPUT0     0x00
    #define AW9523_REG_OUTPUT0    0x02
    #define AW9523_REG_CONFIG0    0x04
    #define AW9523_REG_INTENABLE0 0x06 
    #define AW9523_REG_GCR        0x11
    #define AW9523_REG_LEDMODE    0x12
    // Note: The original RG_I2C_DRIVER == 1 path also had defines for AW9523_REG_...
    // but they were different values (0x00, 0x02, 0x04, 0x06) than this #else block's.
    // This indicates the original code had *two different* configurations for AW9523-like devices.
#endif


bool rg_i2c_init(void) { /* ... Same as your version ... */
#if USE_I2C_DRIVER
    const i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER, .sda_io_num = RG_GPIO_I2C_SDA, .scl_io_num = RG_GPIO_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // Kept at 100kHz
    };
    esp_err_t err = ESP_FAIL;
    if (i2c_initialized) return true;
    TRY(i2c_param_config(I2C_NUM_0, &i2c_config));
    TRY(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
    RG_LOGI("I2C driver ready (SDA:%d SCL:%d).\n", i2c_config.sda_io_num, i2c_config.scl_io_num);
    i2c_initialized = true; return true;
fail:
    RG_LOGE("Failed to initialize I2C driver. err=0x%x (%s)\n", err, esp_err_to_name(err));
#else
    RG_LOGE("I2C is not available on this device.\n");
#endif
    i2c_initialized = false; return false;
}

bool rg_i2c_deinit(void) { /* ... Same as your version ... */
#if USE_I2C_DRIVER
    if (i2c_initialized && i2c_driver_delete(I2C_NUM_0) == ESP_OK) RG_LOGI("I2C driver terminated.\n");
    else if (i2c_initialized) RG_LOGE("Failed to delete I2C driver.\n");
#endif
    i2c_initialized = false; gpio_extender_initialized = false; return true;
}

// I2C Read/Write primitives are kept as they were in your original code,
// as PCF8575 uses direct read/write to its address without a register pointer
// for its main I/O data. So, reg will often be -1 for these.
bool rg_i2c_read(uint8_t addr, int reg, void *read_data, size_t read_len) {
#if USE_I2C_DRIVER
    i2c_cmd_handle_t cmd = NULL; esp_err_t err = ESP_FAIL;    
    if (!i2c_initialized) { RG_LOGE("rg_i2c_read: I2C not initialized.\n"); return false; }
    cmd = i2c_cmd_link_create(); if (!cmd) { RG_LOGE("rg_i2c_read: cmd_link_create failed.\n"); return false; }

    if (reg >= 0) { // PCF8575 doesn't use this for data read, but other devices might
        TRY(i2c_master_start(cmd));
        TRY(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true));
        TRY(i2c_master_write_byte(cmd, (uint8_t)reg, true));
    }
    // For PCF8575, this is the actual read sequence (reg is ignored or should be -1)
    TRY(i2c_master_start(cmd)); 
    TRY(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true));
    if (read_len > 0) TRY(i2c_master_read(cmd, (uint8_t*)read_data, read_len, I2C_MASTER_LAST_NACK));
    TRY(i2c_master_stop(cmd));
    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100)); 
    if (err != ESP_OK) {goto fail;}
    i2c_cmd_link_delete(cmd); return true;
fail:
    RG_LOGE("Read from 0x%02X failed. reg=%d, err=0x%x (%s)\n", addr, reg, err, esp_err_to_name(err));
    if (cmd) i2c_cmd_link_delete(cmd);
#endif
    return false;
}

bool rg_i2c_write(uint8_t addr, int reg, const void *write_data, size_t write_len) {
#if USE_I2C_DRIVER
    i2c_cmd_handle_t cmd = NULL; esp_err_t err = ESP_FAIL;
    if (!i2c_initialized) { RG_LOGE("rg_i2c_write: I2C not initialized.\n"); return false; }
    cmd = i2c_cmd_link_create(); if (!cmd) { RG_LOGE("rg_i2c_write: cmd_link_create failed.\n"); return false; }

    TRY(i2c_master_start(cmd));
    TRY(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true));
    if (reg >= 0) { // PCF8575 doesn't use this for data write, but other devices might
        TRY(i2c_master_write_byte(cmd, (uint8_t)reg, true));
    }
    // For PCF8575, this is the actual data write (reg is ignored or should be -1)
    if (write_len > 0 && write_data) { TRY(i2c_master_write(cmd, (const uint8_t*)write_data, write_len, true));
    } else if (write_len > 0 && !write_data) { RG_LOGE("rg_i2c_write: data is NULL\n"); err=ESP_FAIL; goto fail;}
    TRY(i2c_master_stop(cmd));
    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {goto fail;}
    i2c_cmd_link_delete(cmd); return true;
fail:
    RG_LOGE("Write to 0x%02X failed. reg=%d, err=0x%x (%s)\n", addr, reg, err, esp_err_to_name(err));
    if (cmd) i2c_cmd_link_delete(cmd);
#endif
    return false;
}

// rg_i2c_read_byte and rg_i2c_write_byte are less relevant for PCF8575 direct I/O,
// as it typically involves 2-byte reads/writes for all 16 pins.
// However, we keep them for structural compatibility if other parts of the code use them.
uint8_t rg_i2c_read_byte(uint8_t addr, uint8_t reg) {
    uint8_t value = 0;
    // For PCF8575, direct register reading isn't how I/O ports are accessed.
    // This function would need specific handling if meant to read a single byte from a PCF8575 concept.
    // For now, it passes through to rg_i2c_read.
    RG_LOGW("rg_i2c_read_byte may not be suitable for PCF8575 port access without modification.\n");
    return rg_i2c_read(addr, (int)reg, &value, 1) ? value : 0;
}
bool rg_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t value) {
    RG_LOGW("rg_i2c_write_byte may not be suitable for PCF8575 port access without modification.\n");
    return rg_i2c_write(addr, (int)reg, &value, 1);
}


bool rg_i2c_gpio_init(void)
{
    RG_LOGI("--- rg_i2c_gpio_init: ENTERED ---");
    if (gpio_extender_initialized) {
        RG_LOGI("--- rg_i2c_gpio_init: Already initialized. Skipping.\n");
        return true;
    }
    if (!i2c_initialized && !rg_i2c_init()) {
        RG_LOGE("--- rg_i2c_gpio_init: I2C master init FAILED.\n");
        return false;
    }
    RG_LOGI("--- rg_i2c_gpio_init: Passed initial checks.\n");

#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    RG_LOGI("--- rg_i2c_gpio_init: RG_I2C_DRIVER == 1. Initializing PCF8575.\n");
    gpio_extender_address = PCF8575_ADDRESS_DEFAULT; // e.g., 0x20
    RG_LOGI("PCF8575: Target I2C address is 0x%02X.\n", gpio_extender_address);

    // For PCF8575, to set all 16 pins as inputs, write 0xFFFF (two bytes: 0xFF, 0xFF)
    // to its device address. The 'reg' parameter in rg_i2c_write is not used by PCF8575
    // for this operation, so we can pass -1 or 0.
    uint8_t init_data[2] = {0xFF, 0xFF};
    if (rg_i2c_write(gpio_extender_address, -1, init_data, 2)) {
        RG_LOGI("PCF8575: Successfully wrote 0xFFFF to set all pins as inputs.\n");
        pcf8575_port_cache = 0xFFFF; // Initialize cache to all inputs state
        gpio_extender_initialized = true;
        return true;
    } else {
        RG_LOGE("PCF8575: FAILED to write 0xFFFF to set pins as inputs!\n");
        gpio_extender_initialized = false;
        return false;
    }

#else // AW9523 @ 0x58 Path (RG_I2C_DRIVER == 0 or undefined)
    RG_LOGI("--- rg_i2c_gpio_init: RG_I2C_DRIVER != 1 path. Initializing AW9523 @ 0x58.\n");
    gpio_extender_address = 0x20;
    
    if (!rg_i2c_write_byte(gpio_extender_address, AW9523_REG_SOFTRESET, 0)) {
         RG_LOGE("AW9523(0x58): Failed SOFTRESET.\n"); return false;
    }
    rg_usleep(10 * 1000); 
    uint8_t id = rg_i2c_read_byte(gpio_extender_address, AW9523_REG_CHIPID); 
    if (id != 0x23) { 
        RG_LOGE("AW9523(0x58) invalid ID 0x%x found.\n", id); return false; 
    }
    RG_LOGI("AW9523(0x58) ID 0x%x OK.\n", id);

    bool aw_ok = true;
    aw_ok &= rg_i2c_write_byte(gpio_extender_address, AW9523_REG_CONFIG0, 0xFF);
    aw_ok &= rg_i2c_write_byte(gpio_extender_address, AW9523_REG_CONFIG0 + 1, 0xFF);
    aw_ok &= rg_i2c_write_byte(gpio_extender_address, AW9523_REG_LEDMODE, 0xFF);
    aw_ok &= rg_i2c_write_byte(gpio_extender_address, AW9523_REG_LEDMODE + 1, 0xFF);
    aw_ok &= rg_i2c_write_byte(gpio_extender_address, AW9523_REG_GCR, 1 << 4);

    if (aw_ok) {
        RG_LOGI("AW9523(0x58) configuration successful.\n");
        gpio_extender_initialized = true;
        return true;
    } else {
        RG_LOGE("AW9523(0x58) configuration FAILED.\n");
        gpio_extender_initialized = false;
        return false;
    }
#endif
}

bool rg_i2c_gpio_deinit(void) {
    RG_LOGI("--- rg_i2c_gpio_deinit ---");
    gpio_extender_initialized = false; 
    gpio_extender_address = 0;
    return true;
}

// GPIO helper functions need adaptation for PCF8575
bool rg_i2c_gpio_set_direction(int pin, int mode) {
    if (!gpio_extender_initialized) return false; 
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    // For PCF8575, direction is set by writing '1' for input to the pin.
    // This is a read-modify-write operation on the 16-bit port.
    // For simplicity now, as init sets all to input, this could be a no-op
    // or log a warning that per-pin direction change is complex for PCF8575.
    RG_LOGW("PCF8575: rg_i2c_gpio_set_direction is a no-op or needs full RMW.\n");
    if (mode == 1) { // If setting to input, ensure the bit is 1 in cache
        pcf8575_port_cache |= (1 << pin);
        uint8_t data_to_write[2] = {(uint8_t)(pcf8575_port_cache & 0xFF), (uint8_t)(pcf8575_port_cache >> 8)};
        return rg_i2c_write(gpio_extender_address, -1, data_to_write, 2);
    }
    return true; // Or false if not implemented
#else // AW9523 Path
    uint8_t reg = AW9523_REG_CONFIG0 + (pin >> 3);
    uint8_t mask = 1 << (pin & 7);
    uint8_t val = rg_i2c_read_byte(gpio_extender_address, reg);
    return rg_i2c_write_byte(gpio_extender_address, reg, mode ? (val | mask) : (val & ~mask));
#endif
}

uint8_t rg_i2c_gpio_read_port(int port) {
    if (!gpio_extender_initialized) return 0;
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    if (port < 0 || port > 1) { RG_LOGE("PCF8575: Invalid port %d for read_port.\n", port); return 0; }
    uint8_t received_data[2] = {0,0};
    if (rg_i2c_read(gpio_extender_address, -1, received_data, 2)) {
        pcf8575_port_cache = (received_data[1] << 8) | received_data[0]; // Update cache
        return received_data[port]; // Port 0 is first byte, Port 1 is second byte
    }
    RG_LOGE("PCF8575: Failed to read ports.\n");
    return 0; // Error
#else // AW9523 Path
    if (port < 0 || port > 1) { RG_LOGE("AW9523: Invalid port %d for read_port.\n", port); return 0; }
    return rg_i2c_read_byte(gpio_extender_address, AW9523_REG_INPUT0 + port);
#endif
}

bool rg_i2c_gpio_write_port(int port, uint8_t value) {
    if (!gpio_extender_initialized) return false;
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    if (port < 0 || port > 1) { RG_LOGE("PCF8575: Invalid port %d for write_port.\n", port); return false; }
    // This requires modifying either port 0 or port 1 of the 16-bit cache and writing both back
    if (port == 0) {
        pcf8575_port_cache = (pcf8575_port_cache & 0xFF00) | value;
    } else { // port == 1
        pcf8575_port_cache = (pcf8575_port_cache & 0x00FF) | ((uint16_t)value << 8);
    }
    uint8_t data_to_write[2] = {(uint8_t)(pcf8575_port_cache & 0xFF), (uint8_t)(pcf8575_port_cache >> 8)};
    return rg_i2c_write(gpio_extender_address, -1, data_to_write, 2);
#else // AW9523 Path
     if (port < 0 || port > 1) { RG_LOGE("AW9523: Invalid port %d for write_port.\n", port); return false; }
    return rg_i2c_write_byte(gpio_extender_address, AW9523_REG_OUTPUT0 + port, value);
#endif
}

int rg_i2c_gpio_get_level(int pin) {
    if (!gpio_extender_initialized) return -1;
    if (pin < 0 || pin > 15) { return -1; }
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    // Read both ports first to get current state
    uint8_t p0 = rg_i2c_gpio_read_port(0);
    uint8_t p1 = rg_i2c_gpio_read_port(1); // This read updates pcf8575_port_cache
    if (pin < 8) { // Port 0
        return (p0 >> pin) & 1;
    } else { // Port 1
        return (p1 >> (pin - 8)) & 1;
    }
#else // AW9523 Path
    return (rg_i2c_gpio_read_port(pin >> 3) >> (pin & 7)) & 1;
#endif
}

bool rg_i2c_gpio_set_level(int pin, int level) {
    if (!gpio_extender_initialized) return false;
    if (pin < 0 || pin > 15) { return false; }
#if defined(RG_I2C_DRIVER) && (RG_I2C_DRIVER == 1) // PCF8575 Path
    // This is a read-modify-write for the specific pin on PCF8575
    uint16_t current_val_16bit = ((uint16_t)rg_i2c_gpio_read_port(1) << 8) | rg_i2c_gpio_read_port(0); //Ensure cache is up to date via read_port
    uint16_t new_val_16bit;
    if (level) {
        new_val_16bit = current_val_16bit | (1 << pin);
    } else {
        new_val_16bit = current_val_16bit & ~(1 << pin);
    }
    if (new_val_16bit == current_val_16bit) return true;
    pcf8575_port_cache = new_val_16bit; // Update cache
    uint8_t data_to_write[2] = {(uint8_t)(new_val_16bit & 0xFF), (uint8_t)(new_val_16bit >> 8)};
    return rg_i2c_write(gpio_extender_address, -1, data_to_write, 2);
#else // AW9523 Path
    uint8_t reg = AW9523_REG_OUTPUT0 + (pin >> 3);
    uint8_t mask = 1 << (pin & 7);
    uint8_t val = rg_i2c_read_byte(gpio_extender_address, reg);
    uint8_t new_val = level ? (val | mask) : (val & ~mask);
    return (new_val == val) ? true : rg_i2c_write_byte(gpio_extender_address, reg, new_val);
#endif
}

