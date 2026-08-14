#include "frodo/frodo.h"

#include "C64.h"
#include "Display.h"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <new>
#include <string>

struct frodo_instance {
    C64 *machine;
    std::string content_path;
    uint32_t audio_sample_rate;
    bool full_1541;
};

namespace {

void set_error(char *text, size_t size, const char *message)
{
    if (text != nullptr && size > 0) {
        std::snprintf(text, size, "%s", message);
    }
}

bool supported_content(const char *path)
{
    const char *dot = path == nullptr ? nullptr : std::strrchr(path, '.');
    if (dot == nullptr) return false;
    char extension[5] = {};
    size_t length = std::strlen(dot + 1);
    if (length == 0 || length >= sizeof(extension)) return false;
    for (size_t i = 0; i < length; ++i) {
        extension[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(dot[i + 1])));
    }
    return std::strcmp(extension, "d64") == 0 ||
           std::strcmp(extension, "x64") == 0 ||
           std::strcmp(extension, "g64") == 0 ||
           std::strcmp(extension, "prg") == 0 ||
           std::strcmp(extension, "t64") == 0 ||
           std::strcmp(extension, "crt") == 0;
}

} // namespace

extern "C" frodo_instance_t *frodo_create(const frodo_config_t *config,
                                           char *error_text,
                                           size_t error_text_size)
{
#ifdef FRODO_SC
    std::printf("Frodo engine: SC (cycle exact)\n");
#else
    std::printf("Frodo engine: Lite (line based)\n");
#endif

    if (config == nullptr || config->content_path == nullptr) {
        set_error(error_text, error_text_size, "No C64 program or image was supplied.");
        return nullptr;
    }
    if (!supported_content(config->content_path)) {
        set_error(error_text, error_text_size,
                  "Unsupported C64 media. Use CRT, PRG, D64, T64, X64 or G64.");
        return nullptr;
    }
    if (config->cycle_exact) {
        set_error(error_text, error_text_size,
                  "Cycle-exact Frodo is not enabled in this embedded build.");
        return nullptr;
    }

    auto *instance = new (std::nothrow) frodo_instance_t{};
    if (instance == nullptr) {
        set_error(error_text, error_text_size, "Unable to allocate Frodo instance.");
        return nullptr;
    }

    instance->machine = new (std::nothrow) C64(config->content_path,
                                               config->full_1541,
                                               config->audio_sample_rate);
    if (instance->machine == nullptr) {
        delete instance;
        set_error(error_text, error_text_size, "Unable to allocate Frodo machine memory.");
        return nullptr;
    }
    instance->content_path = config->content_path;
    instance->audio_sample_rate = config->audio_sample_rate;
    instance->full_1541 = config->full_1541;
    if (!instance->machine->StartupError().empty()) {
        set_error(error_text, error_text_size,
                  instance->machine->StartupError().c_str());
        delete instance->machine;
        delete instance;
        return nullptr;
    }

    set_error(error_text, error_text_size, "");
    return instance;
}

extern "C" void frodo_destroy(frodo_instance_t *instance)
{
    if (instance == nullptr) {
        return;
    }
    delete instance->machine;
    delete instance;
}

extern "C" bool frodo_run_frame(frodo_instance_t *instance,
                                const frodo_input_t *input,
                                bool render_video)
{
    if (instance == nullptr || instance->machine == nullptr || input == nullptr) {
        return false;
    }
    return instance->machine->RunFrame(input->joystick_port_1,
                                       input->joystick_port_2,
                                       input->keyboard,
                                       input->character,
                                       render_video);
}

extern "C" bool frodo_get_video_frame(const frodo_instance_t *instance,
                                      frodo_video_frame_t *frame)
{
    if (instance == nullptr || instance->machine == nullptr || frame == nullptr) {
        return false;
    }

    *frame = {
        .pixels = instance->machine->VideoPixels(),
        .palette_rgb888 = instance->machine->VideoPalette(),
        .width = DISPLAY_X,
        .height = DISPLAY_Y,
        .stride = DISPLAY_X,
        .visible_x = 0,
        .visible_y = 0,
        .visible_width = DISPLAY_X,
        .visible_height = DISPLAY_Y,
    };
    return true;
}

extern "C" const frodo_audio_frame_t *frodo_get_audio_frames(const frodo_instance_t *instance,
                                                              size_t *frame_count)
{
    if (frame_count == nullptr || instance == nullptr || instance->machine == nullptr) return nullptr;
    const int16_t *samples = instance->machine->AudioSamples(*frame_count);
    return reinterpret_cast<const frodo_audio_frame_t *>(samples);
}

extern "C" bool frodo_save_snapshot(frodo_instance_t *instance, const char *filename)
{
    return instance != nullptr && instance->machine != nullptr &&
           instance->machine->SaveSnapshot(filename);
}

extern "C" bool frodo_load_snapshot(frodo_instance_t *instance, const char *filename)
{
    return instance != nullptr && instance->machine != nullptr &&
           instance->machine->LoadSnapshot(filename);
}

extern "C" bool frodo_reset(frodo_instance_t *instance, bool hard)
{
    if (instance == nullptr || instance->machine == nullptr) return false;

    C64 *replacement = new (std::nothrow) C64(instance->content_path.c_str(),
                                               instance->full_1541,
                                               instance->audio_sample_rate);
    if (replacement == nullptr || !replacement->StartupError().empty()) {
        delete replacement;
        return false;
    }

    if (!hard) {
        // A warm C64 reset preserves RAM but starts all chips from a coherent
        // power-on state. Suppress the one-shot host autostart so PRG/disk
        // content is not silently reloaded over the preserved machine RAM.
        std::memcpy(replacement->RAM, instance->machine->RAM, C64_RAM_SIZE);
        std::memcpy(replacement->Color, instance->machine->Color, COLOR_RAM_SIZE);
        replacement->DisableAutoStart();
    }

    delete instance->machine;
    instance->machine = replacement;
    return true;
}
