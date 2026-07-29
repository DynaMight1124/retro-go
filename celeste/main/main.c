#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int celeste_game_main(int argc, char* argv[]);
extern bool celeste_reset(bool hard);
extern void celeste_after_state_load(void);

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
extern size_t vfx_get_state_size(void);
extern void vfx_get_state(void *dest);
extern void vfx_set_state(const void *src);

static volatile bool app_running = true;
static int gameplay_speed = 0;

#define SETTING_GAMEPLAY_SPEED "GameplaySpeed"

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        extern void gfx_redraw(void);
        gfx_redraw();
    }
    else if (event == RG_EVENT_SHUTDOWN)
    {
        extern void audio_shutdown(void);
        audio_shutdown();
        app_running = false;
    }
    else if (event == RG_EVENT_SPEEDUP)
    {
        rg_app_t *app = rg_system_get_app();
        if (app && app->speed == 1.0f)
            app->frameskip = 0;
    }
}

static bool screenshot_handler(const char *filename, int width, int height)
{
    extern bool gfx_screenshot(const char *filename, int width, int height);
    return gfx_screenshot(filename, width, height);
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

    strcpy(option->value, classic ? _("Classic (restart)") : _("Widescreen (restart)"));

    return RG_DIALOG_VOID;
}

static rg_gui_event_t gameplay_speed_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    static const char *const values[] = {"Normal", "1.25x", "1.5x"};

    if (event == RG_DIALOG_PREV)
        gameplay_speed = gameplay_speed > 0 ? gameplay_speed - 1 : 2;
    else if (event == RG_DIALOG_NEXT)
        gameplay_speed = gameplay_speed < 2 ? gameplay_speed + 1 : 0;

    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        rg_settings_set_number(NS_APP, SETTING_GAMEPLAY_SPEED, gameplay_speed);
        return RG_DIALOG_REDRAW;
    }

    strcpy(option->value, _(values[gameplay_speed]));
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Video Mode"), "-", RG_DIALOG_FLAG_NORMAL, &classic_mode_update_cb};
    *dest++ = (rg_gui_option_t){0, _("Gameplay Speed"), "-", RG_DIALOG_FLAG_NORMAL, &gameplay_speed_update_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

/* Save State Handlers */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t player_size;
    uint32_t level_size;
    uint32_t game_size;
} state_header_v1_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t player_size;
    uint32_t level_size;
    uint32_t game_size;
    uint32_t vfx_size;
} state_header_t;

#define STATE_MAGIC 0x43454C53 /* "CELS" */
#define STATE_VERSION_V1 1
#define STATE_VERSION 2

static bool load_state_handler(const char *filename)
{
    size_t size;
    void *buffer = NULL;
    if (!rg_storage_read_file(filename, &buffer, &size, 0)) return false;

    if (size < sizeof(state_header_v1_t)) {
        free(buffer);
        return false;
    }

    const state_header_v1_t *base = (const state_header_v1_t *)buffer;
    if (base->magic != STATE_MAGIC ||
        base->player_size != player_get_state_size() ||
        base->level_size != level_get_state_size() ||
        base->game_size != game_get_state_size()) {
        free(buffer);
        return false;
    }

    size_t header_size;
    size_t vfx_size = 0;
    if (base->version == STATE_VERSION_V1) {
        header_size = sizeof(state_header_v1_t);
    } else if (base->version == STATE_VERSION) {
        if (size < sizeof(state_header_t)) {
            free(buffer);
            return false;
        }
        const state_header_t *header = (const state_header_t *)buffer;
        if (header->vfx_size != vfx_get_state_size()) {
            free(buffer);
            return false;
        }
        header_size = sizeof(state_header_t);
        vfx_size = header->vfx_size;
    } else {
        free(buffer);
        return false;
    }

    size_t expected_size = header_size +
                           base->player_size +
                           base->level_size +
                           base->game_size +
                           vfx_size;
    if (size != expected_size) {
        free(buffer);
        return false;
    }

    uint8_t *ptr = (uint8_t *)buffer + header_size;
    player_set_state(ptr); ptr += base->player_size;
    level_set_state(ptr);  ptr += base->level_size;
    game_set_state(ptr);   ptr += base->game_size;
    if (vfx_size)
        vfx_set_state(ptr);

    celeste_after_state_load();

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
        .vfx_size = vfx_get_state_size(),
    };

    size_t total_size = sizeof(header) + header.player_size + header.level_size +
                        header.game_size + header.vfx_size;
    uint8_t *buffer = calloc(1, total_size);
    if (!buffer) return false;

    uint8_t *ptr = buffer;
    memcpy(ptr, &header, sizeof(header)); ptr += sizeof(header);
    player_get_state(ptr); ptr += header.player_size;
    level_get_state(ptr);  ptr += header.level_size;
    game_get_state(ptr);   ptr += header.game_size;
    vfx_get_state(ptr);

    bool success = rg_storage_write_file(filename, buffer, total_size, RG_FILE_ATOMIC_WRITE);
    free(buffer);
    return success;
}

static bool reset_handler(bool hard)
{
    return celeste_reset(hard);
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
            .screenshot = &screenshot_handler,
            .reset = &reset_handler,
        },
    };

    rg_system_init(&config);
    gameplay_speed = rg_settings_get_number(NS_APP, SETTING_GAMEPLAY_SPEED, 0);
    if (gameplay_speed < 0 || gameplay_speed > 2)
        gameplay_speed = 0;

    RG_LOGI("Starting Celeste...\n");

    char *argv[] = {"celeste", NULL};

    app_running = true;
    celeste_game_main(1, argv);

    RG_LOGI("Celeste exited loop.\n");

    RG_LOGI("Celeste shutting down.\n");
    rg_system_exit();
}

bool celeste_is_running(void) {
    return app_running;
}

int celeste_get_gameplay_speed(void) {
    return gameplay_speed;
}
