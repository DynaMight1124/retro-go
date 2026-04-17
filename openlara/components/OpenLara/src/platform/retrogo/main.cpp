#include <stdio.h>
#include <string.h>

#include <retro-go.h>

#include "game.h"

Inventory *inventory = NULL;

rg_surface_t *updates[2];
rg_surface_t *currentUpdate;

// --- Timing ---
int osGetTimeMS() {
    return (int)(rg_system_timer() / 1000);
}

// --- Input ---
bool osJoyReady(int index) { return index == 0; }
void osJoyVibrate(int index, float L, float R) {}

static const struct { uint32_t rg; JoyKey ol; } keymap[] = {
    {RG_KEY_UP, jkUp}, {RG_KEY_DOWN, jkDown}, {RG_KEY_LEFT, jkLeft}, {RG_KEY_RIGHT, jkRight},
    {RG_KEY_A, jkA}, {RG_KEY_B, jkB},
    {RG_KEY_X, jkX}, {RG_KEY_SELECT, jkX}, // X and SELECT for Duck (jkX)
    {RG_KEY_Y, jkY}, {RG_KEY_START, jkY},  // Y and START for Weapon (jkY)
    {RG_KEY_MENU, jkSelect}, {RG_KEY_OPTION, jkLT}, // MENU for Inventory (jkSelect), OPTION for Roll (jkLT)
    {RG_KEY_L, jkLB}, {RG_KEY_R, jkRB}
};

void joyUpdate() {
    uint32_t joystick = rg_input_read_gamepad();
    for (int i = 0; i < (int)(sizeof(keymap)/sizeof(keymap[0])); i++) {
        Input::setJoyDown(0, keymap[i].ol, (joystick & keymap[i].rg) != 0);
    }
}

// --- Sound ---
#define AUDIO_SAMPLES 1024
static int16_t sound_buffer[AUDIO_SAMPLES * 2];

namespace Game {
    volatile bool sound_active = false;
    volatile bool sound_running = false;
}

static void sound_task(void *arg) {
    while (1) {
        if (Game::sound_active) {
            Game::sound_running = true;

            Sound::fill((Sound::Frame*)sound_buffer, AUDIO_SAMPLES);

            Game::sound_running = false;

            rg_audio_submit((rg_audio_frame_t*)sound_buffer, AUDIO_SAMPLES);
        } else {
            Game::sound_running = false;
            rg_task_yield();
        }
    }
}

// --- Display / GAPI (Software Rasterizer Stub) ---
// TODO: Implement GAPI for software rasterizer or simple GL

// --- Entry Point ---
static void openlara_task(void *arg) {
    rg_app_t *app = (rg_app_t*)arg;

    // Set OpenLara paths
    char levelName[256] = "";
    if (app->romPath && app->romPath[0]) {
        strncpy(contentDir, app->romPath, sizeof(contentDir));
        char *lastSlash = strrchr(contentDir, '/');
        if (lastSlash) {
            strcpy(levelName, lastSlash + 1);
            *(lastSlash + 1) = 0;
        }

        // Try to find the assets directory by looking for TITLE files
        char base[256];
        strcpy(base, contentDir);
        const char *dataFolders[] = {"data/", "PSXDATA/", "DELDATA/", "TR1_PSX/PSXDATA/", "TR1_PC/DATA/", ""};
        const char *titles[] = {"TITLE.PHD", "title.phd", "TITLE.PSX", "title.psx", "GAME.PSX", "game.psx"};

        bool found = false;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                char path[256];
                sprintf(path, "%s%s%s", base, dataFolders[i], titles[j]);
                if (rg_storage_exists(path)) {
                    sprintf(contentDir, "%s%s", base, dataFolders[i]);
                    strcpy(levelName, titles[j]);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) {
            sprintf(contentDir, "%sdata/", base);
            strcpy(levelName, "TITLE.PHD");
            rg_system_log(RG_LOG_WARN, "OpenLara", "No game data found, using defaults.\n");
        } else {
            rg_system_log(RG_LOG_INFO, "OpenLara", "Found game data at: %s\n", contentDir);
        }
    } else {
        strcpy(contentDir, RG_BASE_PATH_ROMS "/openlara/data/");
        strcpy(levelName, "TITLE.PHD");
    }
    strcpy(saveDir, RG_BASE_PATH_SAVES "/openlara");
    strcpy(cacheDir, RG_BASE_PATH_CACHE "/openlara");

    rg_storage_mkdir(saveDir);
    rg_storage_mkdir(cacheDir);

    rg_system_log(RG_LOG_INFO, "OpenLara", "Paths: contentDir='%s' levelName='%s'\n", contentDir, levelName);

    // Initialize display surfaces - use double buffering to prevent screen tearing!
    updates[0] = rg_surface_create(320, 240, RG_PIXEL_565_LE, MEM_SLOW);
    updates[1] = rg_surface_create(320, 240, RG_PIXEL_565_LE, MEM_SLOW);
    currentUpdate = updates[0];

    // Force display to stretch and fill the screen (ignoring aspect ratio).
    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);

    Core::width = 320;
    Core::height = 240;
    GAPI::resize();
    GAPI::swColor = (uint16_t*)currentUpdate->data;

    // Initialize Sound Task
    rg_task_create("ol_sound", sound_task, NULL, 16384, 1, RG_TASK_PRIORITY_6, 1);

    // Initialize Game
    TR::useEasyStart = true;
    TR::isGameEnded = false;
    Game::sound_active = false;
    Game::init(levelName[0] ? levelName : NULL);
    Game::sound_active = true;

    uint32_t last_time = rg_system_timer();
    const uint32_t target_fps = 30;
    const uint32_t frame_time_us = 1000000 / target_fps;

    while (!Core::isQuit) {
        uint32_t current_time = rg_system_timer();
        int32_t frames = (current_time - last_time) / frame_time_us;

        if (frames > 0) {
            last_time = current_time;

            joyUpdate();
            GAPI::swColor = (uint16_t*)currentUpdate->data;
            Game::update();
            Game::render();

            // Wait for previous frame to finish DMA transfer before submitting a new one
            while (rg_display_is_busy()) {
                rg_task_yield();
            }

            rg_display_submit(currentUpdate, 0);
            currentUpdate = updates[currentUpdate == updates[0]];
        } else {
            rg_task_yield();
        }
    }

    Game::deinit();
    rg_system_restart();
}

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void app_main() {
    const rg_config_t config = {
        .sampleRate = 22050,
        .frameRate = 30,
        .storageRequired = true,
        .romRequired = false,
        .isLauncher = false,
        .handlers = {0},
        .mallocAlwaysInternal = 0
    };
    rg_app_t *app = rg_system_init(&config);

    // Launch main engine loop in a dedicated task with a huge stack
    // Allocate the stack in PSRAM (MEM_SLOW) to prevent starving Retro-Go's DMA queues!
    // The TCB MUST be in Internal RAM (MEM_FAST) per ESP-IDF FreeRTOS requirements!
    static StaticTask_t *ol_task_tcb = (StaticTask_t*)rg_alloc(sizeof(StaticTask_t), MEM_FAST);
    static StackType_t *ol_task_stack = (StackType_t*)rg_alloc(48 * 1024, MEM_SLOW);

    xTaskCreateStaticPinnedToCore(
        openlara_task,
        "ol_main",
        (48 * 1024) / sizeof(StackType_t),
        app,
        RG_TASK_PRIORITY_2,
        ol_task_stack,
        ol_task_tcb,
        0
    );

    while(1) {
        rg_task_delay(1000);
    }
}
