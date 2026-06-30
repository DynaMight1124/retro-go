#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Core.h"
#include "Game.h"
#include "Window.h"
#include "Platform.h"
#include "Options.h"
#include "Logger.h"
#include "Input.h"

static void cc_main_task(void* arg)
{
    RG_LOGI("ClassiCube task started.\n");

    Logger_Hook();
    Platform_Init();
    
    // Window_PreInit is usually first
    Window_PreInit();
    
    Options_Load();
    
    Window_Init();
    
    // Game_Setup will call Window_Init and Window_Create
    Game_Setup();
    
    RG_LOGI("ClassiCube setup complete. Entering main loop.\n");

    int frameCount = 0;
    while (Game_Running)
    {
        int64_t startTime = rg_system_timer();
        float delta = 1.0f / 30.0f; // TODO: Calculate actual delta
        
        // Window_ProcessEvents will handle input
        Window_ProcessEvents(delta);
        
        // Game_RenderFrame handles logic update and drawing
        Game_RenderFrame();
        
        // Handle retro-go system tasks and timing
        rg_system_tick(rg_system_timer() - startTime);

        if (++frameCount % 60 == 0) {
            RG_LOGI("Main loop running... Frame: %d\n", frameCount);
        }
    }
    
    Game_Free();
    rg_system_exit();
    vTaskDelete(NULL);
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = 22050,
        .frameRate = 30,
        .storageRequired = true,
        .romRequired = false,
        .mallocAlwaysInternal = 0,
    };
    
    rg_system_init(&config);
    
    RG_LOGI("ClassiCube starting...\n");

    // We use rg_task_create so the system monitor tracks this task.
    // We use 48KB stack because ClassiCube's software rendering and builder are stack-heavy.
    rg_task_create("CCMain", cc_main_task, NULL, 48 * 1024, 1, RG_TASK_PRIORITY_5, 0);

    // Keep the main task alive to avoid LoadProhibited in rg_system's monitor.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
