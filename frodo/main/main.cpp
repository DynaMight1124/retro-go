#include <rg_system.h>

#include <frodo/frodo.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>

namespace {

constexpr uint32_t AUDIO_SAMPLE_RATE = 22050;
constexpr int OUTPUT_WIDTH = 320;
constexpr int OUTPUT_HEIGHT = 240;
constexpr rg_keyboard_layout_t C64_KEYBOARD_LAYOUT = {
    .layout = "1234567890"
              "qwertyuiop"
              "asdfghjkl "
              "zxcvbnm.,?",
    .columns = 10,
    .rows = 4,
    .label = "C64",
};

#if RG_SCREEN_PIXEL_FORMAT == 0
constexpr int OUTPUT_PIXEL_FORMAT = RG_PIXEL_PAL565_BE;
#else
constexpr int OUTPUT_PIXEL_FORMAT = RG_PIXEL_PAL565_LE;
#endif

rg_app_t *app = nullptr;
frodo_instance_t *core = nullptr;
rg_surface_t *output = nullptr;
bool output_valid = false;
int joystick_port = 2;
bool output_palette_initialized = false;
uint16_t pending_keyboard_mask = 0;
unsigned pending_keyboard_mask_frames = 0;

std::string lowercase_extension(const std::string &path)
{
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return {};

    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

bool supported_content_extension(const std::string &extension)
{
    return extension == ".d64" || extension == ".x64" ||
           extension == ".g64" || extension == ".prg" ||
           extension == ".t64" || extension == ".crt";
}

// Retro-Go's compact unzip implementation expands the first local-file entry.
// Read the same header here so the extracted file retains its content type.
std::string first_zip_entry_name(const char *zip_path)
{
    FILE *file = std::fopen(zip_path, "rb");
    if (file == nullptr) return {};

    uint8_t header[30] = {};
    if (std::fread(header, 1, sizeof(header), file) != sizeof(header) ||
        std::memcmp(header, "PK\x03\x04", 4) != 0) {
        std::fclose(file);
        return {};
    }

    const uint16_t name_length = static_cast<uint16_t>(
        header[26] | (static_cast<uint16_t>(header[27]) << 8));
    if (name_length == 0 || name_length > 512) {
        std::fclose(file);
        return {};
    }

    std::string name(name_length, '\0');
    const bool read_ok = std::fread(name.data(), 1, name_length, file) == name_length;
    std::fclose(file);
    return read_ok ? name : std::string{};
}

bool prepare_content_path(const char *source_path, std::string &prepared_path,
                          char *error_text, size_t error_text_size)
{
    const std::string source_extension = lowercase_extension(source_path);
    if (source_extension != ".zip") {
        // The emulated 1541 can write D64/X64 sectors. Keep those mutations in
        // Retro-Go's cache so gameplay and snapshot restore never alter the
        // user's original image.
        if (source_extension == ".d64" || source_extension == ".x64") {
            void *content = nullptr;
            size_t content_size = 0;
            if (!rg_storage_read_file(source_path, &content, &content_size, 0)) {
                std::snprintf(error_text, error_text_size,
                    "Unable to read disk image.");
                return false;
            }
            if (content_size == 0 || content_size > 256 * 1024) {
                std::free(content);
                std::snprintf(error_text, error_text_size,
                    "Unsupported disk image size.");
                return false;
            }

            prepared_path = std::string(RG_BASE_PATH_CACHE) +
                "/frodo_disk" + source_extension;
            const bool write_ok = rg_storage_write_file(prepared_path.c_str(),
                content, content_size, RG_FILE_ATOMIC_WRITE);
            std::free(content);
            if (!write_ok) {
                std::snprintf(error_text, error_text_size,
                    "Unable to create writable disk working copy.");
                return false;
            }
            std::printf("Frodo: cached writable disk image (%u bytes)\n",
                        static_cast<unsigned>(content_size));
            return true;
        }
        prepared_path = source_path;
        return true;
    }

    const std::string entry_name = first_zip_entry_name(source_path);
    const std::string extension = lowercase_extension(entry_name);
    if (entry_name.empty() || !supported_content_extension(extension)) {
        std::snprintf(error_text, error_text_size,
            "Unsupported ZIP content. Expected CRT, PRG, D64, T64, X64 or G64.");
        return false;
    }

    void *content = nullptr;
    size_t content_size = 0;
    if (!rg_storage_unzip_file(source_path, nullptr, &content, &content_size, 0)) {
        std::snprintf(error_text, error_text_size,
            "Unable to extract '%s' from ZIP.", entry_name.c_str());
        return false;
    }

    prepared_path = std::string(RG_BASE_PATH_CACHE) + "/frodo_zip" + extension;
    const bool write_ok = rg_storage_write_file(
        prepared_path.c_str(), content, content_size, RG_FILE_ATOMIC_WRITE);
    std::free(content);
    if (!write_ok) {
        std::snprintf(error_text, error_text_size,
            "Unable to cache extracted ZIP content.");
        return false;
    }

    std::printf("Frodo: extracted ZIP entry '%s' (%u bytes)\n",
                entry_name.c_str(), static_cast<unsigned>(content_size));
    return true;
}

uint16_t rgb888_to_native_565(uint32_t color)
{
    const uint16_t rgb565 = static_cast<uint16_t>(
        ((color >> 8) & 0xf800u) |
        ((color >> 5) & 0x07e0u) |
        ((color >> 3) & 0x001fu));

    if (OUTPUT_PIXEL_FORMAT == RG_PIXEL_PAL565_BE) {
        return static_cast<uint16_t>((rgb565 << 8) | (rgb565 >> 8));
    }
    return rgb565;
}

bool copy_video_frame()
{
    frodo_video_frame_t frame = {};
    if (output == nullptr || !frodo_get_video_frame(core, &frame) ||
        frame.pixels == nullptr || frame.palette_rgb888 == nullptr) {
        return false;
    }

    const int copy_width = std::min<int>(frame.visible_width, OUTPUT_WIDTH);
    const int copy_height = std::min<int>(frame.visible_height, OUTPUT_HEIGHT);
    const int source_x = frame.visible_x + (frame.visible_width - copy_width) / 2;
    const int source_y = frame.visible_y + (frame.visible_height - copy_height) / 2;
    const int destination_x = (OUTPUT_WIDTH - copy_width) / 2;
    const int destination_y = (OUTPUT_HEIGHT - copy_height) / 2;

    // The normal 384x272 Frodo frame crops to exactly 320x240. Avoid clearing
    // the whole internal staging surface when every destination pixel will be
    // overwritten by the copy below.
    if (copy_width != OUTPUT_WIDTH || copy_height != OUTPUT_HEIGHT) {
        std::memset(output->data, 0, output->stride * output->height);
    }
    for (int y = 0; y < copy_height; ++y) {
        const uint8_t *source = frame.pixels +
            (source_y + y) * frame.stride + source_x;
        uint8_t *destination = static_cast<uint8_t *>(output->data) +
            (destination_y + y) * output->stride + destination_x;
        std::memcpy(destination, source, copy_width);
    }

    if (!output_palette_initialized) {
        for (int i = 0; i < 16; ++i) {
            output->palette[i] = rgb888_to_native_565(frame.palette_rgb888[i]);
        }
        output_palette_initialized = true;
    }
    return true;
}

frodo_input_t read_input()
{
    const uint32_t buttons = rg_input_read_gamepad();
    frodo_input_t input = {};

    const bool start = buttons & RG_KEY_START;
    const bool select = buttons & RG_KEY_SELECT;
    const bool yes_chord = start && (buttons & RG_KEY_A);
    const bool no_chord = start && (buttons & RG_KEY_B);
    const bool return_chord = select && (buttons & RG_KEY_A);
    const bool run_stop_chord = start && select;
    const bool keyboard_chord = yes_chord || no_chord || return_chord ||
                                run_stop_chord;

    uint8_t joystick = 0;
    if (buttons & RG_KEY_UP)    joystick |= FRODO_JOY_UP;
    if (buttons & RG_KEY_DOWN)  joystick |= FRODO_JOY_DOWN;
    if (buttons & RG_KEY_LEFT)  joystick |= FRODO_JOY_LEFT;
    if (buttons & RG_KEY_RIGHT) joystick |= FRODO_JOY_RIGHT;
    if (!keyboard_chord && (buttons & (RG_KEY_A | RG_KEY_B))) joystick |= FRODO_JOY_FIRE;
    if (joystick_port == 1) input.joystick_port_1 = joystick;
    else input.joystick_port_2 = joystick;

    // Match the established Retro-Go C64 controls: START is F1 for title
    // screens and SELECT is Space for common prompts. Extra face/shoulder
    // mappings make the same actions reachable on larger targets.
    if (run_stop_chord) input.keyboard |= FRODO_KEY_RUN_STOP;
    else if (yes_chord) input.keyboard |= FRODO_KEY_Y;
    else if (no_chord) input.keyboard |= FRODO_KEY_N;
    else if (return_chord) input.keyboard |= FRODO_KEY_RETURN;
    else {
        if (buttons & (RG_KEY_Y | RG_KEY_L)) input.keyboard |= FRODO_KEY_F1;
        if (buttons & (RG_KEY_SELECT | RG_KEY_X | RG_KEY_R)) input.keyboard |= FRODO_KEY_SPACE;
    }

    if (pending_keyboard_mask_frames > 0) {
        input.keyboard |= pending_keyboard_mask;
        if (--pending_keyboard_mask_frames == 0) pending_keyboard_mask = 0;
    }

    return input;
}

bool deliver_virtual_key(uint8_t character)
{
    for (unsigned frame = 0; frame < 3; ++frame) {
        frodo_input_t input = {};
        input.character = character;
        if (!frodo_run_frame(core, &input, false)) return false;
    }

    // Always run one frame with an empty keyboard matrix. Without this
    // explicit release, selecting the same character again looks like one
    // continuous key hold to software that detects keyboard edges.
    const frodo_input_t released = {};
    if (!frodo_run_frame(core, &released, true)) return false;

    while (rg_display_is_busy()) rg_task_yield();
    if (copy_video_frame()) {
        rg_display_submit(output, 0);
        output_valid = true;
    }
    return true;
}

int virtual_keyboard_input(int &cursor)
{
    const int count = static_cast<int>(C64_KEYBOARD_LAYOUT.columns *
                                       C64_KEYBOARD_LAYOUT.rows);
    cursor = std::clamp(cursor, 0, count - 1);
    rg_gui_draw_virtual_keyboard(RG_GUI_CENTER, RG_GUI_BOTTOM,
                                 &C64_KEYBOARD_LAYOUT, cursor, false);
    rg_input_wait_for_key(RG_KEY_ALL, false, 1000);

    while (true) {
        const uint32_t buttons = rg_input_read_gamepad();
        int next_cursor = cursor;

        if (buttons & RG_KEY_A) return C64_KEYBOARD_LAYOUT.layout[cursor];
        if (buttons & RG_KEY_B) return -1;
        if (buttons & RG_KEY_LEFT) --next_cursor;
        else if (buttons & RG_KEY_RIGHT) ++next_cursor;
        else if (buttons & RG_KEY_UP) next_cursor -= C64_KEYBOARD_LAYOUT.columns;
        else if (buttons & RG_KEY_DOWN) next_cursor += C64_KEYBOARD_LAYOUT.columns;

        if (next_cursor >= 0 && next_cursor < count && next_cursor != cursor) {
            cursor = next_cursor;
            rg_gui_draw_virtual_keyboard(RG_GUI_CENTER, RG_GUI_BOTTOM,
                                         &C64_KEYBOARD_LAYOUT, cursor, false);
        }

        rg_input_wait_for_key(RG_KEY_ALL, false, 500);
        rg_input_wait_for_key(RG_KEY_ANY, true, 500);
        rg_system_tick(0);
    }
}

void virtual_keyboard_session()
{
    rg_audio_set_mute(true);
    int cursor = 0;
    while (true) {
        const int key = virtual_keyboard_input(cursor);
        if (key < 0) break;
        if (key <= 0xff && !deliver_virtual_key(static_cast<uint8_t>(key))) break;

        // Let the newly rendered C64 screen become visible before the picker
        // is redrawn for the next character.
        rg_task_delay(60);
    }
    rg_audio_set_mute(false);
    rg_input_wait_for_key(RG_KEY_ALL, false, 1000);
}

rg_gui_event_t joystick_port_update(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT) {
        joystick_port = joystick_port == 2 ? 1 : 2;
        rg_settings_set_number(NS_APP, "joystick_port", joystick_port);
    }
    std::snprintf(option->value, 16, "Port %d", joystick_port);
    return RG_DIALOG_VOID;
}

void options_handler(rg_gui_option_t *options)
{
    *options++ = {0, const_cast<char *>("Joystick"), const_cast<char *>("-"),
                  RG_DIALOG_FLAG_NORMAL, &joystick_port_update};
    *options = RG_DIALOG_END;
}

bool screenshot_handler(const char *filename, int width, int height)
{
    while (rg_display_is_busy()) rg_task_yield();
    return output_valid &&
        rg_surface_save_image_file(output, filename, width, height);
}

bool save_state_handler(const char *filename)
{
    return core != nullptr && frodo_save_snapshot(core, filename);
}

bool load_state_handler(const char *filename)
{
    pending_keyboard_mask = 0;
    pending_keyboard_mask_frames = 0;
    return core != nullptr && frodo_load_snapshot(core, filename);
}

bool reset_handler(bool hard)
{
    if (core == nullptr) {
        return false;
    }
    pending_keyboard_mask = 0;
    pending_keyboard_mask_frames = 0;
    return frodo_reset(core, hard);
}

void event_handler(int event, void *)
{
    if (event == RG_EVENT_REDRAW && output_valid) {
        while (rg_display_is_busy()) rg_task_yield();
        rg_display_submit(output, 0);
    } else if (event == RG_EVENT_SHUTDOWN) {
        rg_audio_set_mute(true);
        while (rg_display_is_busy()) rg_task_yield();
        rg_surface_free(output);
        output = nullptr;
        frodo_destroy(core);
        core = nullptr;
    }
}

} // namespace

extern "C" void app_main(void)
{
    static_assert(sizeof(frodo_audio_frame_t) == sizeof(rg_audio_frame_t));
    static_assert(alignof(frodo_audio_frame_t) == alignof(rg_audio_frame_t));

    const rg_config_t system_config = {
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 50,
        .storageRequired = true,
        .romRequired = true,
        .isLauncher = false,
        .handlers = {
            .loadState = &load_state_handler,
            .saveState = &save_state_handler,
            .reset = &reset_handler,
            .screenshot = &screenshot_handler,
            .event = &event_handler,
            .memRead = nullptr,
            .memWrite = nullptr,
            .options = &options_handler,
            .about = nullptr,
        },
        // Leave the 64 KiB C64 RAM under ESP-IDF's normal PSRAM policy. A CYD
        // A/B test forcing it internal reduced heap headroom by 64 KiB without
        // improving throughput; the indexed output remains explicitly fast.
        .mallocAlwaysInternal = 0,
    };

    app = rg_system_init(&system_config);
    // Retro-Go defaults applications to frameskip 1. Frodo Lite has enough
    // headroom on faster targets, so begin at full rendering and let the
    // system monitor raise frameskip only if emulation actually falls behind.
    app->frameskip = 0;
    if (!rg_settings_exists(NS_APP, "DispScaling")) {
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
    }
    joystick_port = rg_settings_get_number(NS_APP, "joystick_port", 2);

    char error_text[160] = {};
    std::string content_path;
    if (!prepare_content_path(app->romPath, content_path,
                              error_text, sizeof(error_text))) {
        std::printf("Frodo startup failed: %s\n", error_text);
        rg_gui_alert("Frodo port", error_text);
        rg_system_exit();
        return;
    }

    const frodo_config_t core_config = {
        .content_path = content_path.c_str(),
        .audio_sample_rate = AUDIO_SAMPLE_RATE,
        .full_1541 = true,
        .cycle_exact = false,
    };

    core = frodo_create(&core_config, error_text, sizeof(error_text));
    if (core == nullptr) {
        std::printf("Frodo startup failed: %s\n",
                    error_text[0] ? error_text : "Core initialization failed.");
        rg_gui_alert("Frodo port", error_text[0] ? error_text : "Core initialization failed.");
        rg_system_exit();
        return;
    }

    output = rg_surface_create(OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_PIXEL_FORMAT, MEM_FAST);
    RG_ASSERT(output != nullptr, "Unable to allocate Frodo output surface");

    if (app->bootFlags & RG_BOOT_RESUME) {
        rg_emu_load_state(app->saveSlot);
    }

    unsigned skip_frames = 0;
    int64_t start_pressed_us = 0;
    bool start_chord_seen = false;
    while (true) {
        const int64_t started_us = rg_system_timer();
        const uint32_t buttons = rg_input_read_gamepad();

        const bool start_down = buttons & RG_KEY_START;
        if (start_down) {
            if (start_pressed_us == 0) start_pressed_us = started_us;
            if (buttons & ~uint32_t(RG_KEY_START)) start_chord_seen = true;
            if (!start_chord_seen && started_us - start_pressed_us >= 700000) {
                virtual_keyboard_session();
                start_pressed_us = 0;
                start_chord_seen = false;
                skip_frames = 0;
                continue;
            }
        } else if (start_pressed_us != 0) {
            if (!start_chord_seen) {
                pending_keyboard_mask = FRODO_KEY_F1;
                pending_keyboard_mask_frames = 3;
            }
            start_pressed_us = 0;
            start_chord_seen = false;
        }

        if (buttons & (RG_KEY_MENU | RG_KEY_OPTION)) {
            rg_audio_set_mute(true);
            if (buttons & RG_KEY_MENU) rg_gui_game_menu();
            else rg_gui_options_menu();
            rg_audio_set_mute(false);
            while (rg_input_read_gamepad() & (RG_KEY_MENU | RG_KEY_OPTION)) {
                rg_task_delay(20);
            }
            continue;
        }
        const frodo_input_t input = read_input();
        // Do not start drawing a frame if the previous asynchronous SPI
        // transfer still owns the output surface. Retest every emulated frame
        // instead of forcing an additional skipped frame: this lets fast
        // targets submit at the display's natural throughput without ever
        // stalling C64 simulation or touching a queued surface.
        const bool display_busy = rg_display_is_busy();
        const bool render_video = skip_frames == 0 && !display_busy;

        if (!frodo_run_frame(core, &input, render_video)) {
            rg_gui_alert("Frodo port", "The emulator stopped unexpectedly.");
            rg_system_exit();
        }

        int busy_us = static_cast<int>(rg_system_timer() - started_us);
        if (render_video) {
            const int64_t copy_started_us = rg_system_timer();
            if (copy_video_frame()) {
                busy_us += static_cast<int>(rg_system_timer() - copy_started_us);
                rg_display_submit(output, 0);
                output_valid = true;
            }
        }

        rg_system_tick(busy_us);

        size_t audio_frame_count = 0;
        const frodo_audio_frame_t *audio_frames =
            frodo_get_audio_frames(core, &audio_frame_count);
        if (audio_frames != nullptr && audio_frame_count > 0) {
            rg_audio_submit(reinterpret_cast<const rg_audio_frame_t *>(audio_frames),
                            audio_frame_count);
        }

        if (skip_frames > 0) {
            --skip_frames;
        } else if (app->frameskip > 0) {
            skip_frames = app->frameskip;
        }
    }
}
