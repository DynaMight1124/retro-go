#define NDEBUG
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rg_system.h>
#include <rg_display.h>

/* ROTT expectations */
#define PLATFORM_UNIX 1

/* External engine functions */
extern void allocate_rott_memory(void);
extern int rott_main(int argc, char *argv[]);
extern char ApogeePath[256];

static rg_app_t *app;

static bool screenshot_handler(const char *filename, int width, int height) { return false; }
static bool save_state_handler(const char *filename) { return false; }
static bool load_state_handler(const char *filename) { return false; }
static bool reset_handler(bool hard) { return false; }
static void event_handler(int event, void *arg) {
    if (event == RG_EVENT_SHUTDOWN) rg_audio_set_mute(true);
}
static void options_handler(rg_gui_option_t *dest) { *dest++ = (rg_gui_option_t)RG_DIALOG_END; }

void list_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        printf("  (Could not open directory)\n");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
    }
    closedir(dir);
}

typedef struct {
    int argc;
    char **argv;
} rott_task_args_t;

static void rott_task(void *pvParameters) {
    rott_task_args_t *args = (rott_task_args_t *)pvParameters;
    printf("ROTT Task started. Entering rott_main...\n");
    rott_main(args->argc, args->argv);
    printf("ROTT Task finished. Exiting...\n");
    rg_system_exit();
    vTaskDelete(NULL);
}

void app_main()
{
    const rg_config_t config = {
        .sampleRate = 11025,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
        .handlers = {
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
            .reset = &reset_handler,
            .screenshot = &screenshot_handler,
            .event = &event_handler,
            .options = &options_handler,
        },
    };

    app = rg_system_init(&config);
    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
    
    // Suppress serial flood during massive allocation
    rg_system_set_log_level(RG_LOG_WARN);

    // Give storage a moment to stabilize
    vTaskDelay(pdMS_TO_TICKS(200));

    allocate_rott_memory();
    
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "%s/rott", RG_BASE_PATH_ROMS);

    // Set ApogeePath to the Retro-Go save directory.
    snprintf(ApogeePath, 256, "%s/rott", RG_BASE_PATH_SAVES);
    rg_storage_mkdir(ApogeePath);

    // Debug file system
    list_dir(base_path);

    static char *rott_argv[10];
    static char rott_argv_dir[256];
    int rott_argc = 0;

    strncpy(rott_argv_dir, base_path, sizeof(rott_argv_dir));

    rott_argv[rott_argc++] = "rott";
    rott_argv[rott_argc++] = "-dir";
    rott_argv[rott_argc++] = rott_argv_dir;
    rott_argv[rott_argc] = NULL;

    printf("Starting ROTT task with argc=%d\n", rott_argc);
    
    static rott_task_args_t task_args;
    task_args.argc = rott_argc;
    task_args.argv = rott_argv;

    // Create the task with a safe stack size (64KB).
    vTaskDelay(pdMS_TO_TICKS(1000));
    xTaskCreatePinnedToCore(rott_task, "rott_task", 65536, &task_args, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
