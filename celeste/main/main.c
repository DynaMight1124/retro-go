#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int celeste_game_main(int argc, char* argv[]);
extern void audio_deinit(void);

/* State support */
extern size_t player_get_state_size(void);
extern void player_get_state(void *dest);
extern void player_set_state(const void *src);
extern size_t level_get_state_size(void);
extern void level_get_state(void *dest);
extern void level_set_state(const void *src);
extern size_t game_get_state_size(void);
extern void game_get_state(void *dest);
extern void game_set_state(const void *src);

static bool app_running = true;

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        extern void gfx_flip(void);
        gfx_flip();
    }
    else if (event == RG_EVENT_SHUTDOWN)
    {
        extern void audio_shutdown(void);
        audio_shutdown();
        app_running = false;
    }
}

static rg_gui_event_t classic_mode_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    int classic = rg_settings_get_number(NS_APP, "ClassicMode", 0);

    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        classic = !classic;
        rg_settings_set_number(NS_APP, "ClassicMode", classic);
        return RG_DIALOG_REDRAW;
    }

    strcpy(option->value, classic ? "Classic (128x128)" : "Widescreen (256x150)");

    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, "Video Mode", "-", RG_DIALOG_FLAG_NORMAL, &classic_mode_update_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

/* Save State Handlers */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t player_size;
    uint32_t level_size;
    uint32_t game_size;
} state_header_t;

#define STATE_MAGIC 0x43454C53 /* "CELS" */
#define STATE_VERSION 1

static bool load_state_handler(const char *filename)
{
    size_t size;
    void *buffer = NULL;
    if (!rg_storage_read_file(filename, &buffer, &size, 0)) return false;

    state_header_t *header = (state_header_t *)buffer;
    if (header->magic != STATE_MAGIC || header->version != STATE_VERSION) {
        free(buffer);
        return false;
    }

    uint8_t *ptr = (uint8_t *)buffer + sizeof(state_header_t);
    player_set_state(ptr); ptr += header->player_size;
    level_set_state(ptr);  ptr += header->level_size;
    game_set_state(ptr);

    free(buffer);
    return true;
}

static bool save_state_handler(const char *filename)
{
    state_header_t header = {
        .magic = STATE_MAGIC,
        .version = STATE_VERSION,
        .player_size = player_get_state_size(),
        .level_size = level_get_state_size(),
        .game_size = game_get_state_size(),
    };

    size_t total_size = sizeof(header) + header.player_size + header.level_size + header.game_size;
    uint8_t *buffer = malloc(total_size);
    if (!buffer) return false;

    uint8_t *ptr = buffer;
    memcpy(ptr, &header, sizeof(header)); ptr += sizeof(header);
    player_get_state(ptr); ptr += header.player_size;
    level_get_state(ptr);  ptr += header.level_size;
    game_get_state(ptr);

    bool success = rg_storage_write_file(filename, buffer, total_size, 0);
    free(buffer);
    return success;
}

void app_main()
{
    const rg_config_t config = {
        .sampleRate = 22050,
        .frameRate = 30,
        .storageRequired = true, 
        .romRequired = false,
        .handlers = {
            .event = &event_handler,
            .options = &options_handler,
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
        },
    };

    rg_system_init(&config);

    RG_LOGI("Starting Celeste...\n");

    char *argv[] = {"celeste", NULL};
    
    app_running = true;
    celeste_game_main(1, argv);

    RG_LOGI("Celeste exited loop.\n");

    audio_deinit();

    RG_LOGI("Celeste shutting down.\n");
    rg_system_exit();
}

bool celeste_is_running(void) {
    return app_running;
}
