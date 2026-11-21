#include <rg_system.h>
#include <string.h>
#include <strings.h>
#include "runtime.h"
#include "persistence.h"
#include "lcd.h"

static rg_app_t *app;
static VMU *machine;
static VmuRuntime runtime;
static rg_surface_t *surfaces[2], *lastFrame;
static unsigned drawIndex;
static byte *stateBuffer;
static size_t stateSize;
static const size_t HEADER_SIZE = 32;
static char *sramPath;
static char biosPath[RG_PATH_MAX + 1];
static uint32_t contentCRC, biosCRC, savedFlashCRC;
static bool haveSavedFlash, icons;
static int lcdPalette, lcdGrid;
static uint16_t nativePixels[SCREEN_WIDTH * SCREEN_HEIGHT_ICONS];
static const int16_t *audioData;
static size_t audioCount;

static void put32(byte *p, uint32_t n)
{
    for (int i = 0; i < 4; ++i) p[i] = (byte)(n >> (i * 8));
}
static uint32_t get32(const byte *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool flushFlash()
{
    if (!machine || !sramPath) return true;
    byte *data = machine->flash->getDataPointer();
    uint32_t crc = vmu_crc32(data, VE_VMS_FLASH::DATA_SIZE);
    if (haveSavedFlash && savedFlashCRC == crc) return true;
    bool ok = rg_storage_mkdir(rg_dirname(sramPath)) &&
        vmu_atomic_write(sramPath, data, VE_VMS_FLASH::DATA_SIZE);
    if (ok) { savedFlashCRC = crc; haveSavedFlash = true; rg_storage_commit(); }
    else RG_LOGE("VMU flash save failed: %s", sramPath);
    return ok;
}

static void drawFrame()
{
    rg_surface_t *surface = surfaces[drawIndex];
    // A blocking submit waits for the previous queued surface to be consumed.
    // The alternate draw buffer is therefore free; descriptors stay immutable.
    int nativeHeight = icons ? SCREEN_HEIGHT_ICONS : SCREEN_HEIGHT;
    memset(nativePixels, 0xFF, SCREEN_WIDTH * nativeHeight * sizeof(uint16_t));
    machine->video->drawFrame(nativePixels);
    if (icons) machine->video->drawIcons(nativePixels);
    auto geometry = vmu_lcd_render(nativePixels, nativeHeight, (uint8_t *)surface->data,
        surface->stride, surface->palette, lcdPalette, lcdGrid);
    surface->width = geometry.width;
    surface->height = geometry.height;
    rg_display_submit(surface, 0);
    lastFrame = surface;
    drawIndex ^= 1;
}

static bool saveState(const char *filename)
{
    if (!machine->saveState(stateBuffer + HEADER_SIZE, stateSize)) return false;
    memcpy(stateBuffer, "RGVMU002", 8);
    put32(stateBuffer + 8, contentCRC);
    put32(stateBuffer + 12, biosCRC);
    put32(stateBuffer + 16, vmu_crc32(stateBuffer + HEADER_SIZE, stateSize));
    put32(stateBuffer + 20, (uint32_t)runtime.frameDebt);
    put32(stateBuffer + 24, (uint32_t)stateSize);
    put32(stateBuffer + 28, vmu_crc32(stateBuffer, 28));
    // Retro-Go promotes the temporary slot only after this callback succeeds.
    return vmu_write_file_checked(filename, stateBuffer, stateSize + HEADER_SIZE);
}

static bool loadState(const char *filename)
{
    size_t size = stateSize + HEADER_SIZE;
    if (rg_storage_stat(filename).size != size) return false;
    void *data = stateBuffer;
    if (!rg_storage_read_file(filename, &data, &size, RG_FILE_USER_BUFFER)) return false;
    int32_t debt = (int32_t)get32(stateBuffer + 20);
    if (memcmp(stateBuffer, "RGVMU002", 8) || get32(stateBuffer + 8) != contentCRC ||
        get32(stateBuffer + 12) != biosCRC || get32(stateBuffer + 24) != stateSize ||
        get32(stateBuffer + 28) != vmu_crc32(stateBuffer, 28) ||
        get32(stateBuffer + 16) != vmu_crc32(stateBuffer + HEADER_SIZE, stateSize) ||
        debt > 0 || debt <= -112500) return false;
    if (!machine->loadState(stateBuffer + HEADER_SIZE, stateSize)) return false;
    runtime.frameDebt = debt;
    drawFrame();
    return true;
}

static bool reset(bool hard)
{
    (void)hard;
    // A reset preserves the live flash, including changes not yet saved to SD.
    memcpy(stateBuffer, machine->flash->getDataPointer(), VE_VMS_FLASH::DATA_SIZE);
    machine->reset();
    if (!machine->flash->loadROM(stateBuffer, VE_VMS_FLASH::DATA_SIZE, 0, "", false)) return false;
    if (biosPath[0] && machine->loadBIOS(biosPath) != 0) {
        RG_LOGE("VMU BIOS could not be reloaded");
        RG_PANIC("VMU BIOS reload failed");
    }
    machine->startCPU();
    runtime.frameDebt = 0;
    drawFrame();
    return true;
}

static bool screenshot(const char *filename, int width, int height)
{
    return lastFrame && rg_surface_save_image_file(lastFrame, filename, width, height);
}
static void eventHandler(int event, void *)
{
    if (event == RG_EVENT_REDRAW && lastFrame) rg_display_submit(lastFrame, 0);
    if (event == RG_EVENT_SHUTDOWN && !flushFlash())
        rg_gui_alert(_("Save failed"), _("VMU flash could not be saved."));
}
static size_t captureAudio(const int16_t *data, size_t count)
{
    audioData = data;
    audioCount = count;
    return count;
}

static void loadBIOS()
{
    // Optional: the first valid user-provided BIOS wins. Never ship a BIOS.
    static const char *names[] = {
        "vmu_bios.bin", "vmu_bios_en.bin", "en1005-19991026-315-6208-05.bin",
        "vmu_bios_jp.bin", "jp1004-19980930-315-6208-01.bin"
    };
    if (!rg_settings_get_number(NS_APP, "bios", 0)) return;
    for (const char *name : names) {
        snprintf(biosPath, sizeof(biosPath), "%s/%s", RG_BASE_PATH_BIOS, name);
        if (machine->loadBIOS(biosPath) == 0) {
            biosCRC = vmu_crc32(machine->rom->getData(), 0x10000);
            RG_LOGI("VMU BIOS loaded: %s", name);
            return;
        }
    }
    biosPath[0] = 0;
    RG_LOGI("VMU BIOS not found; using HLE");
}

static rg_gui_event_t option(rg_gui_option_t *opt, rg_gui_event_t event)
{
    const char *key = opt->arg == 0 ? "bios" : "icons";
    int value = rg_settings_get_number(NS_APP, key, 0);
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT || event == RG_DIALOG_ENTER) {
        value = !value;
        rg_settings_set_number(NS_APP, key, value);
    }
    strcpy(opt->value, value ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}
static rg_gui_event_t lcdOption(rg_gui_option_t *opt, rg_gui_event_t event)
{
    static const char *paletteNames[] = {"Original", "VMU Green", "Olive LCD", "Blue Ink", "Gray"};
    static const char *gridNames[] = {"Off", "Low", "Medium", "High"};
    int &value = opt->arg == 0 ? lcdPalette : lcdGrid;
    int count = opt->arg == 0 ? VMU_LCD_PALETTE_COUNT : VMU_LCD_GRID_COUNT;
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT || event == RG_DIALOG_ENTER) {
        value = (value + (event == RG_DIALOG_PREV ? count - 1 : 1)) % count;
        rg_settings_set_number(NS_APP, opt->arg == 0 ? "lcd_palette" : "lcd_grid_v2", value);
    }
    strcpy(opt->value, _(opt->arg == 0 ? paletteNames[value] : gridNames[value]));
    return RG_DIALOG_VOID;
}

static void options(rg_gui_option_t *dest)
{
    static char blank[] = "";
    *dest++ = {0, _("BIOS (restart)"), blank, RG_DIALOG_FLAG_NORMAL, option};
    *dest++ = {1, _("Icon row (BIOS only)"), blank, RG_DIALOG_FLAG_NORMAL, option};
    *dest++ = {0, _("LCD palette"), blank, RG_DIALOG_FLAG_NORMAL, lcdOption};
    *dest++ = {1, _("Pixel grid"), blank, RG_DIALOG_FLAG_NORMAL, lcdOption};
    *dest = RG_DIALOG_END;
}

static void loadContent()
{
    const char *ext = strrchr(app->romPath, '.');
    if (!ext) RG_PANIC("VMU content has no extension");
    bool zip = !strcasecmp(ext, ".zip");
    size_t size = 0;
    void *data = NULL;
    int type = vmu_content_type(app->romPath);
    if (zip) {
        byte header[256];
        size_t length = sizeof(header);
        void *headerPtr = header;
        VmuZipInfo info;
        if (!rg_storage_read_file(app->romPath, &headerPtr, &length, RG_FILE_USER_BUFFER) ||
            !vmu_zip_info(header, length, rg_storage_stat(app->romPath).size, info))
            RG_PANIC("Invalid VMU ZIP (first member must be a deflated game)");
        size = info.size;
        type = info.type;
        data = rg_alloc(size, MEM_SLOW);
        if (!rg_storage_unzip_file(app->romPath, NULL, &data, &size, RG_FILE_USER_BUFFER) ||
            size != info.size || vmu_crc32(data, size) != info.crc)
            RG_PANIC("VMU ZIP extraction failed");
    } else {
        size = rg_storage_stat(app->romPath).size;
        if (!size || size > VE_VMS_FLASH::DATA_SIZE + 32) RG_PANIC("Invalid VMU content size");
        data = rg_alloc(size, MEM_SLOW);
        if (!rg_storage_read_file(app->romPath, &data, &size, RG_FILE_USER_BUFFER))
            RG_PANIC("VMU content read failed");
    }
    if (type < 0 || !machine->flash->loadROM((byte *)data, size, type, "", false))
        RG_PANIC("Invalid or unsupported VMU content");
    free(data);
    contentCRC = vmu_crc32(machine->flash->getDataPointer(), VE_VMS_FLASH::DATA_SIZE);
    sramPath = rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath);
    if (!vmu_recover_backup(sramPath, VE_VMS_FLASH::DATA_SIZE) &&
        rg_storage_exists((std::string(sramPath) + ".bak").c_str()))
        RG_PANIC("VMU flash save recovery failed");
    if (rg_storage_exists(sramPath)) {
        if (rg_storage_stat(sramPath).size != VE_VMS_FLASH::DATA_SIZE)
            RG_PANIC("Invalid VMU flash save size");
        void *flash = machine->flash->getDataPointer();
        size_t length = VE_VMS_FLASH::DATA_SIZE;
        if (!rg_storage_read_file(sramPath, &flash, &length, RG_FILE_USER_BUFFER))
            RG_PANIC("VMU flash save read failed");
        savedFlashCRC = vmu_crc32(flash, length);
        haveSavedFlash = true;
    }
}

extern "C" void app_main(void)
{
    rg_config_t config = {};
    config.sampleRate = SAMPLE_RATE;
    config.frameRate = FPS;
    config.storageRequired = true;
    config.romRequired = true;
    config.handlers.loadState = loadState;
    config.handlers.saveState = saveState;
    config.handlers.reset = reset;
    config.handlers.screenshot = screenshot;
    config.handlers.event = eventHandler;
    config.handlers.options = options;
    app = rg_system_init(&config);
    // Use Retro-Go's per-app scaling preference so the Options menu can override it.
    if (!rg_settings_exists(NS_APP, "DispScaling"))
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
    lcdPalette = vmu_lcd_clamp_palette((int)rg_settings_get_number(NS_APP, "lcd_palette", 1));
    int previousGrid = vmu_lcd_legacy_grid((int)rg_settings_get_number(NS_APP, "lcd_grid", 1));
    lcdGrid = vmu_lcd_clamp_grid((int)rg_settings_get_number(NS_APP, "lcd_grid_v2", previousGrid));
    machine = new VMU(NULL);
    loadContent();
    loadBIOS();
    // HLE never drives the etched segments. Retain the user's preference but
    // allocate and show the taller surface only when a BIOS actually loaded.
    icons = machine->hasBIOS() && rg_settings_get_number(NS_APP, "icons", 0);
    for (auto &surface : surfaces)
        surface = rg_surface_create(SCREEN_WIDTH * VMU_LCD_CELL,
            (icons ? SCREEN_HEIGHT_ICONS : SCREEN_HEIGHT) * VMU_LCD_CELL,
            RG_PIXEL_PAL565_LE, MEM_FAST);
    stateSize = machine->serializeSize();
    stateBuffer = (byte *)rg_alloc(stateSize + HEADER_SIZE, MEM_SLOW);
    machine->startCPU();
    rg_system_set_tick_rate(FPS);
    drawFrame();
    if (app->bootFlags & RG_BOOT_RESUME) rg_emu_load_state(app->saveSlot);
    RG_LOGI("VMU ready: state=%u bytes, BIOS=%d", (unsigned)stateSize, machine->hasBIOS());
    int skipFrames = 0;
    unsigned saveTicks = 0;
    retro_audio_sample_batch_t audioCallback = captureAudio;
    while (true) {
        uint32_t keys = rg_input_read_gamepad();
        if (keys & (RG_KEY_MENU | RG_KEY_OPTION)) {
            rg_audio_set_mute(true);
            if (!flushFlash()) rg_gui_alert(_("Save failed"), _("VMU flash could not be saved."));
            if (keys & RG_KEY_MENU) rg_gui_game_menu();
            else rg_gui_options_menu();
            rg_audio_set_mute(false);
            continue;
        }
        // Periodic persistence runs outside the measured emulation interval.
        if (++saveTicks >= FPS * 30) { flushFlash(); saveTicks = 0; }
        int64_t start = rg_system_timer();
        byte buttons = 0;
        if (keys & RG_KEY_UP) buttons |= 1;
        if (keys & RG_KEY_DOWN) buttons |= 2;
        if (keys & RG_KEY_LEFT) buttons |= 4;
        if (keys & RG_KEY_RIGHT) buttons |= 8;
        if (keys & RG_KEY_A) buttons |= 16;
        if (keys & RG_KEY_B) buttons |= 32;
        if (keys & RG_KEY_START) buttons |= 64;
        if (keys & RG_KEY_SELECT) buttons |= 128;
        runtime.input(*machine, buttons);
        runtime.runFrame(*machine);
        bool late = rg_display_is_busy();
        if (!skipFrames) drawFrame();
        machine->audio->generateSignal(audioCallback);
        int busy = (int)(rg_system_timer() - start);
        rg_system_tick(busy);
        rg_audio_submit((const rg_audio_frame_t *)audioData, audioCount);
        if (skipFrames) --skipFrames;
        else if (app->frameskip > 0) skipFrames = app->frameskip;
        else if (busy > app->frameTime + 1500 || late) skipFrames = 1;
    }
}
