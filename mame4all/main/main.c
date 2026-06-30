#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Undefine BIT to avoid conflict between MAME and ESP-IDF/Xtensa headers
#ifdef BIT
#undef BIT
#endif

#include "driver.h"
#include "mame.h"

extern void osd_set_rom_path(const char *path);

static rg_app_t *app;
static int g_selected_game_index = -1;

static void mame_task(void *pvParameters)
{
    RG_LOGI("MAME task started. Running game index %d\n", g_selected_game_index);
    
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    RG_LOGI("Heap Internal: Free=%d, Largest=%d\n", info.total_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    RG_LOGI("Heap PSRAM: Free=%d, Largest=%d\n", info.total_free_bytes, info.largest_free_block);

    // Initialize MAME options
    memset(&options, 0, sizeof(options));
    options.samplerate = 11025;
    options.use_samples = 1;
    options.color_depth = 8; // Use 8bpp for speed and RAM savings

    run_game(g_selected_game_index);

    RG_LOGI("MAME4ALL exiting...\n");
    rg_system_exit();
    vTaskDelete(NULL);
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = 11025,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
    };
    app = rg_system_init(&config);

    RG_LOGI("MAME4ALL starting...\n");

    osd_set_rom_path(app->romPath);

    // Extract game name from romPath
    char game_name[64];
    const char *p = strrchr(app->romPath, '/');
    if (!p) p = app->romPath;
    else p++;
    
    strncpy(game_name, p, sizeof(game_name) - 1);
    game_name[sizeof(game_name) - 1] = 0;
    
    char *dot = strrchr(game_name, '.');
    if (dot) *dot = 0;

    RG_LOGI("Loading game: %s\n", game_name);

    int found_index = -1;
    RG_LOGI("Drivers table at: %p\n", drivers);
    for (int i = 0; drivers[i] != 0; i++)
    {
        if (i < 5) RG_LOGI("Driver[%d] at %p (name: %s)\n", i, drivers[i], drivers[i]->name);
        if (strcasecmp(drivers[i]->name, game_name) == 0)
        {
            found_index = i;
            break;
        }
    }

    if (found_index == -1)
    {
        RG_LOGE("Game '%s' not found in driver list!\n", game_name);
        RG_PANIC("Game not found!");
    }

    g_selected_game_index = found_index;

    // Create MAME task in a separate task with enough stack and higher priority
    xTaskCreatePinnedToCore(mame_task, "mame_task", 128 * 1024, NULL, 10, NULL, 0);


    while (1) {
        rg_system_tick(0);
        vTaskDelay(pdMS_TO_TICKS(10)); // Faster tick
    }
}
