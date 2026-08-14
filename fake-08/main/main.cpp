#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <new>
#include <algorithm>
#include <stdint.h>

#include "host.h"
#include "hostVmShared.h"
#include "vm.h"
#include "graphics.h"
#include "Input.h"
#include "fontdata.h"
#include "logger.h"

#ifdef ESP_PLATFORM
#include <rg_utils.h>
#include <rg_settings.h>
#include <rg_storage.h>
#include <rg_audio.h>
#include <rg_display.h>
#include <rg_input.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

static rg_surface_t *surfaces[2];
static size_t surface_index;
static rg_surface_t *last_complete_surface;
static uint16_t _palette565[144];
static Vm *active_vm;
static PicoRam *active_memory;
static Audio *active_audio;
static bool shutdown_flushed;
static bool game_loop_active;
static bool pending_live_load;
static char pending_state_path[RG_PATH_MAX];

static uint32_t last_raw_keys;
static uint8_t last_pico_keys;
static bool suppress_input_until_release;

static constexpr int AUDIO_SAMPLE_RATE = 22050;
static constexpr size_t AUDIO_MAX_FRAMES = AUDIO_SAMPLE_RATE / 30;
static constexpr size_t LUA_STATE_MAX_SIZE = 1024 * 1024;

struct Fake08StateHeader {
    char magic[8];
    uint32_t version;
    uint32_t luaSize;
    uint32_t ramSize;
    uint32_t audioSize;
    int32_t frameCount;
    int32_t targetFps;
    uint32_t checksum;
};

static constexpr char STATE_MAGIC[8] = {'F', '0', '8', 'R', 'G', 'S', 'T', '1'};
static constexpr uint32_t STATE_VERSION = 1;

static uint32_t checksum_update(uint32_t checksum, const void *data, size_t size) {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    while (size--) {
        checksum ^= *bytes++;
        checksum *= 16777619u;
    }
    return checksum;
}

static void clear_input_after_dialog() {
    last_raw_keys = 0;
    last_pico_keys = 0;
    suppress_input_until_release = true;
}

static bool save_state_handler(const char *filename) {
    if (!active_vm || !active_memory || !active_audio || !filename) return false;

    char *luaState = static_cast<char *>(rg_alloc(LUA_STATE_MAX_SIZE, MEM_SLOW | MEM_NOPANIC));
    if (!luaState) {
        RG_LOGE("Unable to allocate Lua save-state buffer");
        return false;
    }

    const size_t luaSize = active_vm->serializeLuaState(luaState, LUA_STATE_MAX_SIZE);
    if (luaSize == 0 || luaSize > UINT32_MAX) {
        RG_LOGE("Lua save-state serialization failed or exceeded %u bytes", (unsigned)LUA_STATE_MAX_SIZE);
        free(luaState);
        return false;
    }

    Fake08StateHeader header = {};
    memcpy(header.magic, STATE_MAGIC, sizeof(header.magic));
    header.version = STATE_VERSION;
    header.luaSize = static_cast<uint32_t>(luaSize);
    header.ramSize = sizeof(PicoRam);
    header.audioSize = sizeof(audioState_t);
    header.frameCount = active_vm->GetFrameCount();
    header.targetFps = active_vm->GetTargetFps();

    uint32_t checksum = 2166136261u;
    checksum = checksum_update(checksum, luaState, luaSize);
    checksum = checksum_update(checksum, active_memory, sizeof(PicoRam));
    checksum = checksum_update(checksum, active_audio->getAudioState(), sizeof(audioState_t));
    header.checksum = checksum;

    FILE *file = fopen(filename, "wb");
    bool success = file
        && fwrite(&header, sizeof(header), 1, file) == 1
        && fwrite(luaState, luaSize, 1, file) == 1
        && fwrite(active_memory, sizeof(PicoRam), 1, file) == 1
        && fwrite(active_audio->getAudioState(), sizeof(audioState_t), 1, file) == 1;
    if (file && fclose(file) != 0) success = false;
    free(luaState);

    if (!success) RG_LOGE("Unable to write save state '%s'", filename);
    return success;
}

static bool load_state_now(const char *filename) {
    if (!active_vm || !active_memory || !active_audio || !filename) return false;

    FILE *file = fopen(filename, "rb");
    if (!file) return false;

    Fake08StateHeader header = {};
    bool valid = fread(&header, sizeof(header), 1, file) == 1
        && memcmp(header.magic, STATE_MAGIC, sizeof(header.magic)) == 0
        && header.version == STATE_VERSION
        && header.luaSize > 0
        && header.luaSize <= LUA_STATE_MAX_SIZE
        && header.ramSize == sizeof(PicoRam)
        && header.audioSize == sizeof(audioState_t)
        && (header.targetFps == 30 || header.targetFps == 60);

    const size_t payloadSize = static_cast<size_t>(header.luaSize)
        + sizeof(PicoRam) + sizeof(audioState_t);
    char *payload = valid
        ? static_cast<char *>(rg_alloc(payloadSize, MEM_SLOW | MEM_NOPANIC))
        : nullptr;
    if (!payload) valid = false;

    if (valid) {
        valid = fread(payload, payloadSize, 1, file) == 1 && fgetc(file) == EOF;
    }
    fclose(file);

    if (valid) {
        uint32_t checksum = checksum_update(2166136261u, payload, payloadSize);
        valid = checksum == header.checksum;
    }

    if (!valid) {
        RG_LOGE("Invalid or incompatible save state '%s'", filename);
        free(payload);
        return false;
    }

    const char *luaState = payload;
    const char *ramState = luaState + header.luaSize;
    const char *audioState = ramState + sizeof(PicoRam);

    // Validate and restore Lua before committing the plain-data portions.
    // A failed Eris restore leaves the current RAM/audio state untouched.
    if (!active_vm->deserializeLuaState(luaState, header.luaSize)) {
        RG_LOGE("Lua save-state restore failed");
        free(payload);
        return false;
    }

    memcpy(active_memory, ramState, sizeof(PicoRam));
    memcpy(active_audio->getAudioState(), audioState, sizeof(audioState_t));
    active_vm->RestoreFrameState(header.frameCount, header.targetFps);
    free(payload);
    clear_input_after_dialog();
    return true;
}

static bool validate_state_file(const char *filename) {
    FILE *file = filename ? fopen(filename, "rb") : nullptr;
    if (!file) return false;

    Fake08StateHeader header = {};
    bool valid = fread(&header, sizeof(header), 1, file) == 1
        && memcmp(header.magic, STATE_MAGIC, sizeof(header.magic)) == 0
        && header.version == STATE_VERSION
        && header.luaSize > 0
        && header.luaSize <= LUA_STATE_MAX_SIZE
        && header.ramSize == sizeof(PicoRam)
        && header.audioSize == sizeof(audioState_t)
        && (header.targetFps == 30 || header.targetFps == 60);

    const size_t payloadSize = static_cast<size_t>(header.luaSize)
        + sizeof(PicoRam) + sizeof(audioState_t);
    char *payload = valid
        ? static_cast<char *>(rg_alloc(payloadSize, MEM_SLOW | MEM_NOPANIC))
        : nullptr;
    if (!payload) valid = false;

    if (valid) {
        valid = fread(payload, payloadSize, 1, file) == 1
            && fgetc(file) == EOF
            && checksum_update(2166136261u, payload, payloadSize) == header.checksum;
    }

    fclose(file);
    free(payload);
    return valid;
}

static bool load_state_handler(const char *filename) {
    if (!filename) return false;

    // The game menu is opened from Host::shouldQuit(), which is itself called
    // inside Vm::GameLoop(). Eris cannot safely replace live Lua closures while
    // that VM remains on the C++ call stack. Defer menu loads until GameLoop
    // unwinds; cold-start resume can restore immediately.
    if (game_loop_active) {
        if (strlen(filename) >= sizeof(pending_state_path)) {
            RG_LOGE("Save-state path is too long");
            return false;
        }
        if (!validate_state_file(filename)) {
            RG_LOGE("Invalid or incompatible save state '%s'", filename);
            return false;
        }
        strcpy(pending_state_path, filename);
        pending_live_load = true;
        return true;
    }

    return load_state_now(filename);
}

static bool reset_handler(bool hard) {
    (void)hard;
    if (!active_vm) return false;
    active_vm->QueueCartChange(active_vm->CurrentCartFilename());
    clear_input_after_dialog();
    return true;
}

static bool screenshot_handler(const char *filename, int width, int height) {
    if (!last_complete_surface) return false;
    return rg_surface_save_image_file(last_complete_surface, filename, width, height);
}

static void event_handler(int event, void *arg) {
    (void)arg;
    if (event == RG_EVENT_REDRAW) {
        if (last_complete_surface) rg_display_submit(last_complete_surface, 0);
    } else if (event == RG_EVENT_SHUTDOWN) {
        rg_audio_set_mute(true);
        if (active_vm && !shutdown_flushed) {
            active_vm->CloseCart();
            shutdown_flushed = true;
        }
    }
}

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_BE
#else
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_LE
#endif

// ============================================================================
// RetroGo Host Implementation
// ============================================================================

static uint16_t make_rgb565(uint8_t r, uint8_t g, uint8_t b) {
#if RG_SCREEN_PIXEL_FORMAT == 0 // BE
    uint16_t val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return (val << 8) | (val >> 8);
#else // LE
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
#endif
}

Host::Host(int windowWidth, int windowHeight) {
    stretch = PixelPerfectStretch;
    _logFilePrefix = "fake08";
    _cartDirectory = RG_BASE_PATH_ROMS "/pico8";
    _audio = nullptr;
    quit = 0;
    
    setUpPaletteColors();
    loadSettingsIni();
}

void Host::setUpPaletteColors() {
    #define SET_COL(i, c) { Color col = c; _palette565[i] = make_rgb565(col.Red, col.Green, col.Blue); _paletteColors[i] = col; }
    SET_COL(0, COLOR_00);  SET_COL(1, COLOR_01);  SET_COL(2, COLOR_02);  SET_COL(3, COLOR_03);
    SET_COL(4, COLOR_04);  SET_COL(5, COLOR_05);  SET_COL(6, COLOR_06);  SET_COL(7, COLOR_07);
    SET_COL(8, COLOR_08);  SET_COL(9, COLOR_09);  SET_COL(10, COLOR_10); SET_COL(11, COLOR_11);
    SET_COL(12, COLOR_12); SET_COL(13, COLOR_13); SET_COL(14, COLOR_14); SET_COL(15, COLOR_15);
    
    for (int i = 16; i < 128; i++) {
        _palette565[i] = 0;
        _paletteColors[i] = {0, 0, 0, 0};
    }
    
    SET_COL(128, COLOR_128); SET_COL(129, COLOR_129); SET_COL(130, COLOR_130); SET_COL(131, COLOR_131);
    SET_COL(132, COLOR_132); SET_COL(133, COLOR_133); SET_COL(134, COLOR_134); SET_COL(135, COLOR_135);
    SET_COL(136, COLOR_136); SET_COL(137, COLOR_137); SET_COL(138, COLOR_138); SET_COL(139, COLOR_139);
    SET_COL(140, COLOR_140); SET_COL(141, COLOR_141); SET_COL(142, COLOR_142); SET_COL(143, COLOR_143);
    #undef SET_COL

    for (rg_surface_t *surface : surfaces) {
        if (surface) {
            memcpy(surface->palette, _palette565, sizeof(_palette565));
        }
    }
}

Color* Host::GetPaletteColors() {
    return _paletteColors;
}

void Host::oneTimeSetup(Audio* audio) {
    _audio = audio;
}

void Host::unpackCarts() {
}

void Host::setTargetFps(int targetFps) {
    targetFps = targetFps == 60 ? 60 : 30;
    if (_targetFps != targetFps) {
        _targetFps = targetFps;
        _audioSampleRemainder = 0;
    }
    rg_system_set_tick_rate(_targetFps);
}

bool Host::shouldRunMainLoop() {
    return true;
}

InputState_t Host::scanInput() {
    uint32_t keys = rg_input_read_gamepad();
    InputState_t state = {};

    if (suppress_input_until_release) {
        if (keys == 0) suppress_input_until_release = false;
        last_raw_keys = keys;
        last_pico_keys = 0;
        return state;
    }
    
    // Only trigger menu on the rising edge of the button press
    if ((((keys & RG_KEY_SELECT) && (keys & RG_KEY_START)) || (keys & RG_KEY_MENU)) &&
        !(((last_raw_keys & RG_KEY_SELECT) && (last_raw_keys & RG_KEY_START)) || (last_raw_keys & RG_KEY_MENU))) {
        quit = 1;
    }
    
    if ((keys & RG_KEY_OPTION) && !(last_raw_keys & RG_KEY_OPTION)) {
        rg_gui_options_menu();
        dialogOpened = true;
        clear_input_after_dialog();
        return state;
    }

    if (keys & RG_KEY_UP)     state.KHeld |= P8_KEY_UP;
    if (keys & RG_KEY_DOWN)   state.KHeld |= P8_KEY_DOWN;
    if (keys & RG_KEY_LEFT)   state.KHeld |= P8_KEY_LEFT;
    if (keys & RG_KEY_RIGHT)  state.KHeld |= P8_KEY_RIGHT;
    if (keys & RG_KEY_A)      state.KHeld |= P8_KEY_O;
    if (keys & RG_KEY_B)      state.KHeld |= P8_KEY_X;
    if (keys & RG_KEY_START)  state.KHeld |= P8_KEY_PAUSE;
    
    state.KDown = state.KHeld & ~last_pico_keys;
    last_pico_keys = state.KHeld;
    last_raw_keys = keys;
    
    return state;
}

bool Host::shouldQuit() {
    if (quit) {
        quit = 0;
        rg_gui_game_menu();
        dialogOpened = true;
        clear_input_after_dialog();
        if (pending_live_load) return true;
    }
    return false;
}

void Host::changeStretch() {
}

void Host::forceStretch(StretchOption newStretch) {
    stretch = newStretch;
}

void Host::waitForTargetFps() {
    // With a depth-one display queue, the alternate surface is safe to reuse
    // once the queued surface has been taken by the display task.
    while (rg_display_is_busy()) {
        rg_task_yield();
    }
}

void Host::drawFrame(uint8_t* picoFb, uint8_t* screenPaletteMap, uint8_t drawMode) {
    rg_surface_t *surface = surfaces[surface_index];
    if (!surface || !picoFb || !screenPaletteMap) return;

    static uint8_t cachedPaletteMap[16];
    static uint16_t expandedPixels[256];
    static bool expandedPixelsValid = false;

    if (!expandedPixelsValid || memcmp(cachedPaletteMap, screenPaletteMap, sizeof(cachedPaletteMap)) != 0) {
        memcpy(cachedPaletteMap, screenPaletteMap, sizeof(cachedPaletteMap));
        for (int packed = 0; packed < 256; ++packed) {
            expandedPixels[packed] = screenPaletteMap[packed & 0x0f]
                                   | (screenPaletteMap[packed >> 4] << 8);
        }
        expandedPixelsValid = true;
    }

    uint8_t *dest = (uint8_t *)surface->data;
    const uint8_t *src = picoFb;
    const int stride = surface->stride;

    for (int y = 0; y < 128; y++) {
        uint16_t *row = (uint16_t *)&dest[y * stride];
        for (int i = 0; i < 64; ++i) {
            *row++ = expandedPixels[*src++];
        }
    }

    rg_display_submit(surface, 0);
    last_complete_surface = surface;
    surface_index ^= 1;
}

bool Host::shouldFillAudioBuff() {
    return true; 
}

void* Host::getAudioBufferPointer() {
    static rg_audio_frame_t audioBuffer[AUDIO_MAX_FRAMES];
    static_assert(sizeof(rg_audio_frame_t) == sizeof(uint32_t));
    return audioBuffer;
}

size_t Host::getAudioBufferSize() {
    _audioSampleRemainder += AUDIO_SAMPLE_RATE;
    _audioBufferFrames = _audioSampleRemainder / _targetFps;
    _audioSampleRemainder %= _targetFps;
    return _audioBufferFrames;
}

void Host::playFilledAudioBuffer() {
    rg_audio_submit((const rg_audio_frame_t *)getAudioBufferPointer(), _audioBufferFrames);
}

void Host::oneTimeCleanup() {
}

double Host::deltaTMs() {
    return 1000.0 / _targetFps;
}

static int scandir_cb(const rg_scandir_t *file, void *arg) {
    std::vector<std::string> *list = (std::vector<std::string> *)arg;
    if (file->is_file && (strstr(file->path, ".p8") || strstr(file->path, ".png"))) {
        list->push_back(file->path);
    }
    return RG_SCANDIR_CONTINUE;
}

std::vector<std::string> Host::listcarts() {
    std::vector<std::string> carts;
    rg_storage_scandir(_cartDirectory.c_str(), scandir_cb, &carts, RG_SCANDIR_FILES);
    return carts;
}

void Host::overrideLogFilePrefix(const char* newPrefix) {
    _logFilePrefix = newPrefix;
}

const char* Host::logFilePrefix() {
    return _logFilePrefix.c_str();
}

std::string Host::customBiosLua() {
    return "";
}

std::string Host::getCartDataFile(std::string cartDataKey) {
    if (cartDataKey.empty()) return "";
    return std::string(RG_BASE_PATH_SAVES) + "/pico8/" + cartDataKey + ".p8d";
}

std::string Host::getCartDataFileContents(std::string cartDataKey) {
    std::string path = getCartDataFile(cartDataKey);
    if (path.empty()) return "";
    
    void *data;
    size_t len;
    if (rg_storage_read_file(path.c_str(), &data, &len, 0)) {
        std::string s((char *)data, len);
        free(data);
        return s;
    }
    return "";
}

void Host::saveCartData(std::string cartDataKey, std::string contents) {
    std::string path = getCartDataFile(cartDataKey);
    if (path.empty()) return;
    
    rg_storage_mkdir((std::string(RG_BASE_PATH_SAVES) + "/pico8").c_str());
    rg_storage_write_file(path.c_str(), contents.c_str(), contents.length(), 0);
}

size_t Host::getFileContents(std::string fileName, char* buffer) {
    void *data;
    size_t len;
    if (rg_storage_read_file((_cartDirectory + "/" + fileName).c_str(), &data, &len, RG_FILE_USER_BUFFER)) {
        return len;
    }
    return 0;
}

void Host::writeBufferToFile(std::string fileName, char* buffer, size_t length) {
    rg_storage_write_file((_cartDirectory + "/" + fileName).c_str(), buffer, length, 0);
}

std::string Host::getCartDirectory() {
    return _cartDirectory;
}

void Host::setCartDirectory(std::string cartDirectory) {
    _cartDirectory = cartDirectory;
}

static int scandir_dir_cb(const rg_scandir_t *file, void *arg) {
    std::vector<std::string> *list = (std::vector<std::string> *)arg;
    if (file->is_dir) {
        list->push_back(file->path);
    }
    return RG_SCANDIR_CONTINUE;
}

std::vector<std::string> Host::listdirs() {
    std::vector<std::string> dirs;
    rg_storage_scandir(_cartDirectory.c_str(), scandir_dir_cb, &dirs, RG_SCANDIR_DIRS);
    return dirs;
}

int Host::getSetting(std::string sname) {
    if (sname == "stretch") return stretch;
    if (sname == "p8_textcolor") {
        int c = rg_settings_get_number(NS_APP, "p8_textcolor", 7);
        return c == 0 ? 7 : c;
    }
    if (sname == "p8_bgcolor") {
        int c = rg_settings_get_number(NS_APP, "p8_bgcolor", 1);
        return c == 0 ? 1 : c;
    }
    if (sname == "kbmode") return rg_settings_get_number(NS_APP, "kbmode", 0);
    if (sname == "resizekey") return rg_settings_get_number(NS_APP, "resizekey", 0);
    if (sname == "menustyle") return rg_settings_get_number(NS_APP, "menustyle", 0);
    return 0;
}

void Host::setSetting(std::string sname, int sdata) {
    if (sname == "stretch") stretch = (StretchOption)sdata;
    rg_settings_set_number(NS_APP, sname.c_str(), sdata);
    rg_settings_commit();
}

void Host::loadSettingsIni() {
    stretch = (StretchOption)rg_settings_get_number(NS_APP, "stretch", PixelPerfectStretch);
}

void Host::saveSettingsIni() {
    rg_settings_set_number(NS_APP, "stretch", stretch);
    rg_settings_commit();
}

void Host::setPlatformParams(int windowWidth, int windowHeight, uint32_t sdlWindowFlags, uint32_t sdlRendererFlags, uint32_t sdlPixelFormat, std::string logFilePrefix, std::string customBiosLua, std::string cartDirectory) {
}

// ============================================================================
// RetroGo Application Entry
// ============================================================================

static bool load_selected_cart(Vm *vm, const rg_app_t *app) {
    if (!vm || !app || !app->romPath || !app->romPath[0]) {
        return vm && vm->LoadBiosCart();
    }

    if (rg_extension_match(app->romPath, "zip")) {
        void *cartData = nullptr;
        size_t cartSize = 0;
        if (!rg_storage_unzip_file(app->romPath, NULL, &cartData, &cartSize, 0)) {
            RG_LOGE("Unable to extract cart from '%s'", app->romPath);
            return false;
        }
        bool loaded = vm->LoadCart(static_cast<const unsigned char *>(cartData), cartSize, false);
        free(cartData);
        return loaded;
    }

    return vm->LoadCart(app->romPath, false);
}

static void log_initial_cart(const rg_app_t *app, Vm *vm) {
    if (!app || !app->romPath || !app->romPath[0] || !vm) return;

    const char *format = "unknown";
    if (rg_extension_match(app->romPath, "zip")) {
        format = "zip";
    } else if (rg_extension_match(app->romPath, "png")) {
        format = "p8.png";
    } else if (rg_extension_match(app->romPath, "p8")) {
        format = "p8";
    }

    RG_LOGI("CART file='%s' format=%s resume=%s path='%s'",
        rg_basename(app->romPath), format,
        (app->bootFlags & RG_BOOT_RESUME) ? "yes" : "no", app->romPath);
}

static void main_task(void *arg) {
    rg_app_t *app = rg_system_get_app();
    
    Host *host = new (rg_alloc(sizeof(Host), MEM_FAST)) Host(128, 128);
    
    PicoRam *memory = nullptr;
#ifdef ESP_PLATFORM
    memory = new (rg_alloc(sizeof(PicoRam), MEM_FAST)) PicoRam();
#else
    memory = new PicoRam();
#endif
    memory->Reset();
    
    Graphics *graphics = new (rg_alloc(sizeof(Graphics), MEM_FAST)) Graphics(get_font_data(), memory);
    Input *input = new (rg_alloc(sizeof(Input), MEM_FAST)) Input(memory);
    Audio *audio = new (rg_alloc(sizeof(Audio), MEM_FAST)) Audio(memory);

    Logger_Initialize(host->logFilePrefix());
    
    // Force core objects into internal RAM for speed
    Vm *vm = new (rg_alloc(sizeof(Vm), MEM_FAST)) Vm(host, memory, graphics, input, audio);
    active_vm = vm;
    active_memory = memory;
    active_audio = audio;

    host->oneTimeSetup(audio);
    host->setTargetFps(30);

    if (!load_selected_cart(vm, app)) RG_PANIC("Cart loading failed");

    vm->vm_run();
    log_initial_cart(app, vm);

    if (app && (app->bootFlags & RG_BOOT_RESUME)) {
        rg_emu_load_state(app->saveSlot);
    }
    
    // Restoration of system tick log
    while (host->shouldRunMainLoop()) {
        game_loop_active = true;
        vm->GameLoop(); // This returns for exit or a deferred live state load.
        game_loop_active = false;

        if (pending_live_load) {
            // Recreate Lua in the same clean state used by cold-start resume,
            // while retaining the adapter-owned host, RAM, graphics, input and
            // audio allocations.
            delete vm;
            vm = new (rg_alloc(sizeof(Vm), MEM_FAST)) Vm(host, memory, graphics, input, audio);
            active_vm = vm;

            bool cartLoaded = load_selected_cart(vm, app);

            if (cartLoaded) {
                vm->vm_run();
                if (!load_state_now(pending_state_path)) {
                    RG_LOGE("Deferred live state load failed");
                }
            } else {
                RG_LOGE("Unable to reload cart before deferred state load");
            }

            pending_live_load = false;
            pending_state_path[0] = '\0';
            continue;
        }

        if (host->shouldQuit()) break;
    }

    if (!shutdown_flushed) vm->CloseCart();
    host->oneTimeCleanup();

    delete vm;
    delete input;
    delete graphics;
    delete host;
#ifdef ESP_PLATFORM
    free(memory);
#else
    delete memory;
#endif
    delete audio;
    active_vm = nullptr;
    active_memory = nullptr;
    active_audio = nullptr;
    Logger_Exit();
    
    rg_system_exit();
}

extern "C" void app_main() {
    rg_config_t config = {};
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.frameRate = 60;
    config.storageRequired = true;
    config.romRequired = true;
    config.mallocAlwaysInternal = 0;
    config.handlers.loadState = &load_state_handler;
    config.handlers.saveState = &save_state_handler;
    config.handlers.reset = &reset_handler;
    config.handlers.screenshot = &screenshot_handler;
    config.handlers.event = &event_handler;
    
    rg_system_init(&config);
    
    surfaces[0] = rg_surface_create(128, 128, FB_PIXEL_FORMAT, MEM_FAST);
    surfaces[1] = rg_surface_create(128, 128, FB_PIXEL_FORMAT, MEM_FAST);
    
#ifdef ESP_PLATFORM
    rg_task_create("fake08_main", main_task, NULL, 64 * 1024, 1, RG_TASK_PRIORITY_2, 0);
    
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
#else
    main_task(NULL);
#endif
}
