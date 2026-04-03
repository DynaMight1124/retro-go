#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <new>
#include <algorithm>

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

static rg_surface_t *surface;
static uint16_t _palette565[144];

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
    rg_app_t *app = rg_system_get_app();
    if (app) app->frameTime = 1000000 / targetFps;
}

bool Host::shouldRunMainLoop() {
    return true;
}

InputState_t Host::scanInput() {
    uint32_t keys = rg_input_read_gamepad();
    static uint32_t last_keys = 0;
    InputState_t state = {};
    
    // Only trigger menu on the rising edge of the button press
    if ((((keys & RG_KEY_SELECT) && (keys & RG_KEY_START)) || (keys & RG_KEY_MENU)) &&
        !(((last_keys & RG_KEY_SELECT) && (last_keys & RG_KEY_START)) || (last_keys & RG_KEY_MENU))) {
        quit = 1;
    }
    
    if ((keys & RG_KEY_OPTION) && !(last_keys & RG_KEY_OPTION)) {
        rg_gui_options_menu();
    }

    if (keys & RG_KEY_UP)     state.KHeld |= P8_KEY_UP;
    if (keys & RG_KEY_DOWN)   state.KHeld |= P8_KEY_DOWN;
    if (keys & RG_KEY_LEFT)   state.KHeld |= P8_KEY_LEFT;
    if (keys & RG_KEY_RIGHT)  state.KHeld |= P8_KEY_RIGHT;
    if (keys & RG_KEY_A)      state.KHeld |= P8_KEY_O;
    if (keys & RG_KEY_B)      state.KHeld |= P8_KEY_X;
    if (keys & RG_KEY_START)  state.KHeld |= P8_KEY_PAUSE;
    
    static uint8_t lastKHeld = 0;
    state.KDown = state.KHeld & ~lastKHeld;
    lastKHeld = state.KHeld;
    last_keys = keys;
    
    return state;
}

bool Host::shouldQuit() {
    if (quit) {
        quit = 0;
        rg_gui_game_menu();
    }
    return false;
}

void Host::changeStretch() {
}

void Host::forceStretch(StretchOption newStretch) {
    stretch = newStretch;
}

void Host::waitForTargetFps() {
    static int64_t last_time = 0;
    rg_app_t *app = rg_system_get_app();
    int64_t frame_time = (app ? app->frameTime : 16666);
    int64_t now = rg_system_timer();
    
    if (last_time == 0) last_time = now;
    
    int64_t target = last_time + frame_time;
    
    if (now < target) {
        int64_t delay = target - now;
        if (delay > 1000) { 
            rg_task_delay(delay / 1000);
        }
        while (rg_system_timer() < target) {
            // spin
        }
        last_time = target;
    } else {
        if (now - target > frame_time * 2) {
            last_time = now;
        } else {
            last_time += frame_time;
        }
    }
}

void Host::drawFrame(uint8_t* picoFb, uint8_t* screenPaletteMap, uint8_t drawMode) {
    if (!surface || !picoFb || !screenPaletteMap) return;

    uint8_t *dest = (uint8_t *)surface->data;
    const uint32_t *src = (const uint32_t *)picoFb;
    const int stride = surface->width;

    for (int y = 0; y < 128; y++) {
        uint8_t *row = &dest[y * stride];
        for (int i = 0; i < 16; i++) {
            uint32_t pixels = *src++;
            row[0] = screenPaletteMap[pixels & 0x0F];
            row[1] = screenPaletteMap[(pixels >> 4) & 0x0F];
            row[2] = screenPaletteMap[(pixels >> 8) & 0x0F];
            row[3] = screenPaletteMap[(pixels >> 12) & 0x0F];
            row[4] = screenPaletteMap[(pixels >> 16) & 0x0F];
            row[5] = screenPaletteMap[(pixels >> 20) & 0x0F];
            row[6] = screenPaletteMap[(pixels >> 24) & 0x0F];
            row[7] = screenPaletteMap[pixels >> 28];
            row += 8;
        }
    }

    memcpy(surface->palette, _palette565, 144 * 2);
    rg_display_submit(surface, 0);
}

bool Host::shouldFillAudioBuff() {
    return true; 
}

void* Host::getAudioBufferPointer() {
    static uint32_t audioBuffer[1024]; 
    return audioBuffer;
}

size_t Host::getAudioBufferSize() {
    return 1024;
}

void Host::playFilledAudioBuffer() {
    uint32_t *src = (uint32_t *)getAudioBufferPointer();
    size_t count = getAudioBufferSize(); 
    
    static rg_audio_frame_t frames[1024];
    size_t to_submit = std::min(count, (size_t)1024);
    for (size_t i = 0; i < to_submit; i++) {
        frames[i].left = (int16_t)(src[i] & 0xFFFF);
        frames[i].right = (int16_t)(src[i] >> 16);
    }
    rg_audio_submit(frames, to_submit);
}

void Host::oneTimeCleanup() {
}

double Host::deltaTMs() {
    return 1000.0 / 60.0;
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

    host->oneTimeSetup(audio);
    host->setTargetFps(60);

    if (app && app->romPath && strlen(app->romPath) > 0) {
        vm->LoadCart(app->romPath, true);
    } else {
        vm->LoadBiosCart();
    }

    vm->vm_run();
    
    // Restoration of system tick log
    while (host->shouldRunMainLoop()) {
        int64_t busy_start = rg_system_timer();
        
        vm->GameLoop(); // This now returns on quit/exit
        
        rg_system_tick(rg_system_timer() - busy_start);
        
        if (host->shouldQuit()) break;
    }

    vm->CloseCart();
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
    Logger_Exit();
    
    rg_system_exit();
}

extern "C" void app_main() {
    rg_config_t config = {};
    config.sampleRate = 22050;
    config.frameRate = 60;
    config.storageRequired = true;
    config.romRequired = false;
    config.mallocAlwaysInternal = 0;
    
    rg_system_init(&config);
    
    surface = rg_surface_create(128, 128, FB_PIXEL_FORMAT, MEM_FAST);
    
#ifdef ESP_PLATFORM
    rg_task_create("fake08_main", main_task, NULL, 64 * 1024, 1, RG_TASK_PRIORITY_2, 0);
    
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
#else
    main_task(NULL);
#endif
}
