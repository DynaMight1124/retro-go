#include <rg_system.h>
#include <rg_display.h>
#include <rg_input.h>
#include <rg_audio.h>
#include <rg_gui.h>
#include <rg_settings.h>

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// WL_Main is a C++ function that we need to call from our C code.
extern int WL_Main(int argc, char *argv[]);

static void ecwolf_task(void *arg) {
    const char *romPath = rg_system_get_app()->romPath;
    char data_type[16] = "wl6"; // Default
    
    if (romPath && strrchr(romPath, '.')) {
        const char *ext = strrchr(romPath, '.') + 1;
        // Map common extensions or just pass the 3-letter extension directly
        if (strlen(ext) == 3) {
            strncpy(data_type, ext, sizeof(data_type) - 1);
            data_type[sizeof(data_type) - 1] = '\0';
        }
    }

    char saves_path[128];
    snprintf(saves_path, sizeof(saves_path), "%s/wolf3d", RG_BASE_PATH_SAVES);
    rg_storage_mkdir(saves_path);

    const char *res_w = "320";
    const char *res_h = "240";

#ifdef CONFIG_IDF_TARGET_ESP32
    res_w = "320";
    res_h = "200";
#endif

    char *argv[] = {
        (char *)"ecwolf", 
        (char *)"--data", data_type, 
        (char *)"--savedir", saves_path,
        (char *)"--res", (char *)res_w, (char *)res_h,
        NULL
    };
    
    printf("ecwolf_task: Booting with --data %s (ROM: %s) Res: %sx%s\n", 
           data_type, romPath ? romPath : "None", res_w, res_h);
    
    WL_Main(8, argv);
    rg_system_switch_app(NULL, "launcher", NULL, 0, 0);
}

extern "C" void app_main(void) {
    rg_config_t config = {};
    config.sampleRate = 22050;
    config.frameRate = 70;
    
    rg_system_init(&config);
    // ECWolf already catches up multiple 70 Hz game tics before rendering one
    // video frame. Disable Retro-Go's second, competing frameskip controller.
    rg_system_get_app()->frameskip = -1;
    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);

    // Start ECWolf in a dedicated task with a large stack (64KB) on Core 1 (Stable)
    if (rg_task_create("ecwolf_task", ecwolf_task, NULL, 65536, 1, RG_TASK_PRIORITY_2, 1) == NULL) {
        printf("Could not create ecwolf_task!\n");
        rg_system_panic("Task Error", "Could not create ecwolf_task");
    }

    // Keep the main task alive to avoid uxTaskGetStackHighWaterMark panic.
    while (1) {
        rg_task_delay(1000);
    }
}
