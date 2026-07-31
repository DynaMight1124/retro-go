#include <rg_system.h>
#include <rg_utils.h>
#include <rg_gui.h>
#include <rg_settings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "globals.h"
#include "main.h"
#include "setup.h"
#include "video.h"
#include "romloader.h"
#include "trackloader.h"
#include "roms.h"
#include "frontend/config.h"
#include "sdl/input.h"
#include "sdl/audio.h"
#include "hwaudio/segapcm.h"
#include "hwaudio/ym2151.h"
#include "hwvideo/hwtiles.h"
#include "hwvideo/hwsprites.h"
#include "hwvideo/hwroad.h"
#include "engine/outrun.h"

#define AUDIO_SAMPLE_RATE REAL_AUDIO_FREQUENCY

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_565_BE
#define FB_SWAP_BYTES 1
#else
#define FB_PIXEL_FORMAT RG_PIXEL_565_LE
#define FB_SWAP_BYTES 0
#endif

int enable_ym2151_synth = 1;

static rg_surface_t *screens[2];
static rg_surface_t *screen;
static rg_surface_t *presented_screen;
static rg_app_t *app;
static rg_task_t *video_worker_task;
static const uint16_t *video_convert_source;
static volatile bool video_worker_pending;
static int video_sprite_split_y = 144;

enum
{
    VIDEO_WORK_CONVERT = 1,
    VIDEO_WORK_SPRITES = 2,
    VIDEO_WORK_ROAD_BACKGROUND = 3,
    VIDEO_WORK_ROAD_FOREGROUND = 4,
};

extern char rom_base_path[256];
extern uint16_t Render_rgb[S16_PALETTE_ENTRIES * 3];
extern uint16_t* Audio_mix_buffer;

// Explicit declarations
void Audio_init(void);
void Audio_tick(void);
void Audio_wait(void);
void tick(bool draw_frame);
void Outrun_init(void);

// Dummy Render/SDL functions to satisfy linker
uint8_t Render_init(int src_width, int src_height, int scale, int video_mode, int scanlines) { return 1; }
void Render_disable(void) {}
uint8_t Render_start_frame(void) { return 1; }
uint8_t Render_finalize_frame(void) { return 1; }

Packet* Interface_get_packet() { return NULL; }

void process_events(void) {
    // Required by engine
}

// Retro-Go Hooks
static bool screenshot_handler(const char *filename, int width, int height)
{
    rg_surface_t *source = presented_screen ? presented_screen : screen;
    return rg_surface_save_image_file(source, filename, width, height);
}

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        rg_display_submit(presented_screen ? presented_screen : screen, 0);
    }
}

static bool reset_handler(bool hard)
{
    Outrun_init();
    return true;
}

// Emulator Options Callbacks
static rg_gui_event_t ym2151_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        enable_ym2151_synth = !enable_ym2151_synth;
        rg_settings_set_number(NS_APP, "ym2151_enable", enable_ym2151_synth);
    }
    strcpy(option->value, enable_ym2151_synth ? "Enabled" : "Disabled");
    return RG_DIALOG_VOID;
}

static rg_gui_event_t mode_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV)
        Outrun_cannonball_mode = (Outrun_cannonball_mode + 2) % 3;
    else if (event == RG_DIALOG_NEXT)
        Outrun_cannonball_mode = (Outrun_cannonball_mode + 1) % 3;
    
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
        rg_settings_set_number(NS_APP, "game_mode", Outrun_cannonball_mode);
        
    if (Outrun_cannonball_mode == 0) strcpy(option->value, "Original");
    else if (Outrun_cannonball_mode == 1) strcpy(option->value, "Time Trial");
    else strcpy(option->value, "Continuous");
    return RG_DIALOG_VOID;
}

static rg_gui_event_t freeplay_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Config_engine.freeplay = !Config_engine.freeplay;
        rg_settings_set_number(NS_APP, "freeplay", Config_engine.freeplay);
    }
    strcpy(option->value, Config_engine.freeplay ? "On" : "Off");
    return RG_DIALOG_VOID;
}

static rg_gui_event_t attract_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Config_engine.new_attract = !Config_engine.new_attract;
        rg_settings_set_number(NS_APP, "new_attract", Config_engine.new_attract);
    }
    strcpy(option->value, Config_engine.new_attract ? "Enhanced" : "Original");
    return RG_DIALOG_VOID;
}

static rg_gui_event_t bugs_update_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Config_engine.fix_bugs = !Config_engine.fix_bugs;
        rg_settings_set_number(NS_APP, "fix_bugs", Config_engine.fix_bugs);
    }
    strcpy(option->value, Config_engine.fix_bugs ? "On" : "Off");
    return RG_DIALOG_VOID;
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, "Music (YM2151)", "-", RG_DIALOG_FLAG_NORMAL, &ym2151_update_cb};
    *dest++ = (rg_gui_option_t){0, "Game Mode",      "-", RG_DIALOG_FLAG_NORMAL, &mode_update_cb};
    *dest++ = (rg_gui_option_t){0, "Free Play",      "-", RG_DIALOG_FLAG_NORMAL, &freeplay_update_cb};
    *dest++ = (rg_gui_option_t){0, "New Attract",    "-", RG_DIALOG_FLAG_NORMAL, &attract_update_cb};
    *dest++ = (rg_gui_option_t){0, "Fix Engine Bugs", "-", RG_DIALOG_FLAG_NORMAL, &bugs_update_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    adr >>= 1;
    if (adr >= S16_PALETTE_ENTRIES) return;

    // Correct scaling: arcade 5-bit (0-31) to 5-6-5 RGB
    // This exact math was previously confirmed as "perfect colors".
    uint32_t r4 = r >> 1;
    uint32_t g4 = g >> 1;
    uint32_t b4 = b >> 1;
    
    uint32_t r5 = (r4 << 1) | (r4 >> 3);
    uint32_t g6 = (g4 << 2) | (g4 >> 2);
    uint32_t b5 = (b4 << 1) | (b4 >> 3);
    
    uint16_t color = (r5 << 11) | (g6 << 5) | b5;
    
#if FB_SWAP_BYTES
    color = (color << 8) | (color >> 8);
#endif
    Render_rgb[adr] = color;
    
    // Shadows / Highlights: Apply 0.78 brightness multiplier
    // Crucial: Must use same R/G/B balance logic to avoid purple tint
    uint32_t sr4 = (r4 * 202) / 256;
    uint32_t sg4 = (g4 * 202) / 256;
    uint32_t sb4 = (b4 * 202) / 256;
    
    uint32_t sr5 = (sr4 << 1) | (sr4 >> 3);
    uint32_t sg6 = (sg4 << 2) | (sg4 >> 2);
    uint32_t sb5 = (sb4 << 1) | (sb4 >> 3);
    
    uint16_t scolor = (sr5 << 11) | (sg6 << 5) | sb5;
#if FB_SWAP_BYTES
    scolor = (scolor << 8) | (scolor >> 8);
#endif
    Render_rgb[adr + S16_PALETTE_ENTRIES] = Render_rgb[adr + (S16_PALETTE_ENTRIES * 2)] = scolor;
}

uint16_t IRAM_ATTR Render_shadow_color(uint16_t color)
{
#if FB_SWAP_BYTES
    color = (color << 8) | (color >> 8);
#endif

    // Render_rgb expands a 4-bit arcade component into RGB565. Recover those
    // exact source nibbles and apply the same 202/256 shadow multiplier used
    // when the shadow/highlight palette banks are built.
    uint32_t r4 = ((color >> 11) & 0x1f) >> 1;
    uint32_t g4 = ((color >> 5) & 0x3f) >> 2;
    uint32_t b4 = (color & 0x1f) >> 1;
    r4 = (r4 * 202) >> 8;
    g4 = (g4 * 202) >> 8;
    b4 = (b4 * 202) >> 8;

    uint16_t shadow = (((r4 << 1) | (r4 >> 3)) << 11) |
                      (((g4 << 2) | (g4 >> 2)) << 5) |
                      ((b4 << 1) | (b4 >> 3));
#if FB_SWAP_BYTES
    shadow = (shadow << 8) | (shadow >> 8);
#endif
    return shadow;
}

typedef uint32_t packed_u32_t __attribute__((__may_alias__));

static inline void convert_pixels(uint16_t * restrict dest,
                                  const uint16_t * restrict src,
                                  int first, int end)
{
    const uint16_t * restrict colors = Render_rgb;
    const uint32_t mask = (S16_PALETTE_ENTRIES * 3) - 1;
    const packed_u32_t * restrict src_pairs =
        (const packed_u32_t *)(src + first);
    packed_u32_t * restrict dest_pairs = (packed_u32_t *)(dest + first);
    const int pair_count = (end - first) / 2;

    // Both frame buffers and each half-frame boundary are 32-bit aligned.
    // Packing two RGB565 pixels per access halves the PSRAM load/store count;
    // each 16-bit value is unchanged, including target-specific byte order.
    for (int i = 0; i < pair_count; i++)
    {
        uint32_t selectors = src_pairs[i];
        uint32_t pixel0 = colors[(selectors & 0xffff) & mask];
        uint32_t pixel1 = colors[(selectors >> 16) & mask];
        dest_pairs[i] = pixel0 | (pixel1 << 16);
    }
}

static void video_worker(void *arg)
{
    rg_task_msg_t msg;
    while (rg_task_receive(&msg, -1))
    {
        if (msg.type == RG_TASK_MSG_STOP)
            break;

        if (msg.type == VIDEO_WORK_CONVERT)
        {
            convert_pixels((uint16_t *)screen->data, video_convert_source,
                           (320 * 224) / 2, 320 * 224);
        }
        else if (msg.type == VIDEO_WORK_SPRITES)
        {
            uint32_t start = rg_system_timer();
            HWSprites_render_region(8, video_sprite_split_y, Config_s16_height);
            Video_profile.sprite_bottom = rg_system_timer() - start;
        }
        else if (msg.type == VIDEO_WORK_ROAD_BACKGROUND)
        {
            HWRoad_render_background_lores_rows(Video_pixels, 1, 2);
        }
        else if (msg.type == VIDEO_WORK_ROAD_FOREGROUND)
        {
            HWRoad_render_foreground_lores_rows(Video_pixels, 1, 2);
        }
        __atomic_store_n(&video_worker_pending, false, __ATOMIC_RELEASE);
    }
}

static void Render_draw_road_rows(int work_type,
                                  void (*render_rows)(uint16_t *, int, int),
                                  void (*render_full)(uint16_t *))
{
    // The high-resolution renderer interpolates adjacent scanlines and cannot
    // be separated into odd/even work without changing that dependency.
    if (Config_video.hires)
    {
        render_full(Video_pixels);
        return;
    }

    __atomic_store_n(&video_worker_pending, true, __ATOMIC_RELEASE);
    rg_task_msg_t msg = {.type = work_type};
    if (video_worker_task && rg_task_send(video_worker_task, &msg, 0))
    {
        render_rows(Video_pixels, 0, 2);
        while (__atomic_load_n(&video_worker_pending, __ATOMIC_ACQUIRE))
            rg_task_yield();
    }
    else
    {
        __atomic_store_n(&video_worker_pending, false, __ATOMIC_RELEASE);
        render_rows(Video_pixels, 0, 1);
    }
}

void Render_draw_road_background(void)
{
    Render_draw_road_rows(VIDEO_WORK_ROAD_BACKGROUND,
                          HWRoad_render_background_lores_rows,
                          HWRoad_render_background);
}

void Render_draw_road_foreground(void)
{
    Render_draw_road_rows(VIDEO_WORK_ROAD_FOREGROUND,
                          HWRoad_render_foreground_lores_rows,
                          HWRoad_render_foreground);
}

void Render_draw_sprites(void)
{
    HWSprites_prepare_frame();
    Video_profile.sprite_top = 0;
    Video_profile.sprite_bottom = 0;
    __atomic_store_n(&video_worker_pending, true, __ATOMIC_RELEASE);
    rg_task_msg_t msg = {.type = VIDEO_WORK_SPRITES};
    if (video_worker_task && rg_task_send(video_worker_task, &msg, 0))
    {
        uint32_t start = rg_system_timer();
        HWSprites_render_region(8, 0, video_sprite_split_y);
        Video_profile.sprite_top = rg_system_timer() - start;
        while (__atomic_load_n(&video_worker_pending, __ATOMIC_ACQUIRE))
            rg_task_yield();

        // Move the boundary toward the slower region. Limit each correction so
        // scene changes cannot cause a large one-frame oscillation.
        uint32_t region_time = Video_profile.sprite_top + Video_profile.sprite_bottom;
        if (region_time)
        {
            int32_t difference = (int32_t)Video_profile.sprite_bottom -
                                 (int32_t)Video_profile.sprite_top;
            int adjustment = (int)(((int64_t)difference * 32) / region_time);
            if (adjustment > 12) adjustment = 12;
            if (adjustment < -12) adjustment = -12;
            video_sprite_split_y += adjustment;
            if (video_sprite_split_y < 48) video_sprite_split_y = 48;
            if (video_sprite_split_y > Config_s16_height - 24)
                video_sprite_split_y = Config_s16_height - 24;
        }
    }
    else
    {
        __atomic_store_n(&video_worker_pending, false, __ATOMIC_RELEASE);
        HWSprites_render(8);
    }
}

void Render_draw_frame(uint16_t* pixels)
{
#if defined(RETRO_GO) && CANNONBALL_DIRECT_RGB565
    // The engine has already rendered target-native RGB565 into screen->data.
    (void)pixels;
#else
    uint16_t *dest = (uint16_t *)screen->data;

    // Preserve the complete 12-bit selector, including shadow/highlight banks.
    // The two halves are independent and produce bit-identical RGB565 output.
    video_convert_source = pixels;
    __atomic_store_n(&video_worker_pending, true, __ATOMIC_RELEASE);
    rg_task_msg_t msg = {.type = VIDEO_WORK_CONVERT};
    if (video_worker_task && rg_task_send(video_worker_task, &msg, 0))
    {
        convert_pixels(dest, pixels, 0, (320 * 224) / 2);
        while (__atomic_load_n(&video_worker_pending, __ATOMIC_ACQUIRE))
            rg_task_yield();
    }
    else
    {
        __atomic_store_n(&video_worker_pending, false, __ATOMIC_RELEASE);
        convert_pixels(dest, pixels, 0, 320 * 224);
    }
#endif
}

static int try_load(RomLoader* loader, const char* name1, const char* name2, int off, int len, int crc, int inter)
{
    if (RomLoader_load(loader, name1, off, len, crc, inter) == 0) return 0;
    if (name2 && RomLoader_load(loader, name2, off, len, crc, inter) == 0) return 0;
    return 1;
}

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate = AUDIO_SAMPLE_RATE,
        .frameRate = 30, // 30 FPS target
        .storageRequired = true,
        .romRequired = true,
        .handlers.reset = &reset_handler,
        .handlers.screenshot = &screenshot_handler,
        .handlers.event = &event_handler,
        .handlers.options = &options_handler,
    };
    app = rg_system_init(&config);

    // Retro-Go defaults applications to frameskip 1. Cannonball already has
    // a render-only adaptive skipper below, so inheriting that default forced
    // every light scene to display only 15 of its 30 emulated frames even
    // when rendering completed comfortably inside the 33.3 ms budget.
    app->frameskip = 0;
    
    strncpy(rom_base_path, ROM_PATH, sizeof(rom_base_path) - 1);
    
    RG_LOGI("Stage 1: Allocating global buffers...\n");
    TrackLoader_Create();
    HWTiles_Create();
    HWSprites_Create();
    HWRoad_Create();
    Video_Create();
    
    RG_LOGI("Stage 2: Creating directories...\n");
    rg_storage_mkdir(SAVE_PATH);
    rg_storage_mkdir(CONFIG_PATH);

    RG_LOGI("Stage 3: Loading critical ROMs...\n");
    int s = 0;
    
     RomLoader_init(&Roms_z80, 0x10000);
    s += try_load(&Roms_z80, "epr-10187.88", NULL, 0, 0x8000, 0xa10abaa9, ROMLOADER_NORMAL);

    RomLoader_init(&Roms_pcm, 0x60000);
    s += try_load(&Roms_pcm, "opr-10193.66", NULL, 0x00000, 0x08000, 0xbcd10dde, ROMLOADER_NORMAL);
    s += try_load(&Roms_pcm, "opr-10192.67", NULL, 0x10000, 0x08000, 0x770f1270, ROMLOADER_NORMAL);
    s += try_load(&Roms_pcm, "opr-10191.68", NULL, 0x20000, 0x08000, 0x20a284ab, ROMLOADER_NORMAL);
    s += try_load(&Roms_pcm, "opr-10190.69", NULL, 0x30000, 0x08000, 0x7cab70e2, ROMLOADER_NORMAL);
    s += try_load(&Roms_pcm, "opr-10189.70", NULL, 0x40000, 0x08000, 0x01366b54, ROMLOADER_NORMAL);
    s += try_load(&Roms_pcm, "opr-10188.71", "opr-10188.71f", 0x50000, 0x08000, 0xbad30ad9, ROMLOADER_NORMAL);

    RomLoader_init(&Roms_rom0, 0x40000);
    s += try_load(&Roms_rom0, "epr-10380b.133", "epr-10380.133", 0x00000, 0x10000, 0x1f6cadad, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom0, "epr-10382b.118", "epr-10382.118", 0x00001, 0x10000, 0xc4c3fa1a, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom0, "epr-10381a.132", "epr-10381b.132", 0x20000, 0x10000, 0xbe8c412b, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom0, "epr-10383b.117", "epr-10383.117", 0x20001, 0x10000, 0x10a2014a, ROMLOADER_INTERLEAVE2);

    RomLoader_init(&Roms_rom1, 0x40000);
    s += try_load(&Roms_rom1, "epr-10327a.76", "epr-10327.76", 0x00000, 0x10000, 0xe28a5baf, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom1, "epr-10329a.58", "epr-10329.58", 0x00001, 0x10000, 0xda131c81, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom1, "epr-10328a.75", "epr-10328.75", 0x20000, 0x10000, 0xd5ec5e5d, ROMLOADER_INTERLEAVE2);
    s += try_load(&Roms_rom1, "epr-10330a.57", "epr-10330.57", 0x20001, 0x10000, 0xba9ec82a, ROMLOADER_INTERLEAVE2);

    if (s > 0)
    {
        rg_system_panic("app_main", "Critical ROMs failed to load! Check /sd/roms/cannonball/");
    }
    
    Roms_rom0p = &Roms_rom0;
    Roms_rom1p = &Roms_rom1;

    RG_LOGI("Stage 4: Loading configuration...\n");
    Config_load(CONFIG_PATH "/config.xml");
    
    // Load Emulator Options from Retro-Go Settings
    enable_ym2151_synth = rg_settings_get_number(NS_APP, "ym2151_enable", 1);
    Outrun_cannonball_mode = rg_settings_get_number(NS_APP, "game_mode", 0);
    Config_engine.jap = 0; // Force International
    Config_engine.freeplay = rg_settings_get_number(NS_APP, "freeplay", 0);
    Config_engine.new_attract = rg_settings_get_number(NS_APP, "new_attract", 1);
    Config_engine.fix_bugs = rg_settings_get_number(NS_APP, "fix_bugs", 1);
    
    Config_video.widescreen  = 0;
    Config_video.hires       = 0;
    Config_set_fps(0); // 0 = 30 FPS logic
    Config_sound.enabled     = 1;

    RG_LOGI("Stage 5: Loading Tile/Road/Sprite ROMs and initializing hardware...\n");
    RomLoader_init(&Roms_tiles, 0x30000);
    try_load(&Roms_tiles, "opr-10268.99",  NULL, 0x00000, 0x08000, 0x95344b04, ROMLOADER_NORMAL);
    try_load(&Roms_tiles, "opr-10232.102", NULL, 0x08000, 0x08000, 0x776ba1eb, ROMLOADER_NORMAL);
    try_load(&Roms_tiles, "opr-10267.100", NULL, 0x10000, 0x08000, 0xa85bb823, ROMLOADER_NORMAL);
    try_load(&Roms_tiles, "opr-10231.103", NULL, 0x18000, 0x08000, 0x8908bcbf, ROMLOADER_NORMAL);
    try_load(&Roms_tiles, "opr-10266.101", NULL, 0x20000, 0x08000, 0x9f6f1a74, ROMLOADER_NORMAL);
    try_load(&Roms_tiles, "opr-10230.104", NULL, 0x28000, 0x08000, 0x686f5e50, ROMLOADER_NORMAL);
    HWTiles_init(Roms_tiles.rom, 0);
    RomLoader_unload(&Roms_tiles);

    RomLoader_init(&Roms_road, 0x10000);
    try_load(&Roms_road, "opr-10185.11", NULL, 0x000000, 0x08000, 0x22794426, ROMLOADER_NORMAL);
    try_load(&Roms_road, "opr-10186.47", NULL, 0x008000, 0x08000, 0x22794426, ROMLOADER_NORMAL);
    HWRoad_init(Roms_road.rom, 0);
    RomLoader_unload(&Roms_road);

    RomLoader_init(&Roms_sprites, 0x100000);
    try_load(&Roms_sprites, "mpr-10371.9",  NULL, 0x000000, 0x20000, 0x7cc86208, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10373.10", NULL, 0x000001, 0x20000, 0xb0d26ac9, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10375.11", NULL, 0x000002, 0x20000, 0x59b60bd7, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10377.12", NULL, 0x000003, 0x20000, 0x17a1b04a, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10372.13", NULL, 0x080000, 0x20000, 0xb557078c, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10374.14", NULL, 0x080001, 0x20000, 0x8051e517, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10376.15", NULL, 0x080002, 0x20000, 0xf3b8f318, ROMLOADER_INTERLEAVE4);
    try_load(&Roms_sprites, "mpr-10378.16", NULL, 0x080003, 0x20000, 0xa1062984, ROMLOADER_INTERLEAVE4);
    HWSprites_init(Roms_sprites.rom);

    RG_LOGI("Stage 6: Finalizing Video/Audio...\n");
#if defined(RETRO_GO) && CANNONBALL_DIRECT_RGB565
    RG_LOGI("Video path: direct RGB565\n");
#else
    RG_LOGI("Video path: selector conversion\n");
#endif
    Config_load_scores(SAVE_PATH "/hiscore.xml");
    // The original ESP32 cannot fit this complete surface in internal RAM after
    // engine initialization. Keep the allocation explicitly in PSRAM instead of
    // requesting MEM_FAST and silently falling back there.
    screens[0] = rg_surface_create(320, 224, FB_PIXEL_FORMAT, MEM_SLOW);
    screens[1] = rg_surface_create(320, 224, FB_PIXEL_FORMAT, MEM_SLOW);
    if (!screens[0])
        rg_system_panic("app_main", "Framebuffer allocation failed");

    screen = screens[0];
    memset(screens[0]->data, 0, screens[0]->stride * screens[0]->height);
    if (screens[1])
    {
        memset(screens[1]->data, 0, screens[1]->stride * screens[1]->height);
        RG_LOGI("Framebuffer mode: double-buffered PSRAM\n");
    }
    else
    {
        RG_LOGI("Framebuffer mode: single-buffered PSRAM fallback\n");
    }
#if defined(RETRO_GO) && CANNONBALL_DIRECT_RGB565
    Video_set_framebuffer((uint16_t *)screen->data);
#endif
    video_worker_task = rg_task_create("video_worker", &video_worker,
                                       NULL, 2048, 1, RG_TASK_PRIORITY_6, 1);
    Video_init(&Config_video);
    Audio_init();
    
    RG_LOGI("Stage 7: Initializing Outrun engine...\n");
    Outrun_init();

    cannonball_state = STATE_GAME;
    
    RG_LOGI("Cannonball started!\n");
    
    int skipFrames = 0;

    while (true)
    {
        uint32_t startTime = rg_system_timer();
        
        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION))
        {
            if (joystick & RG_KEY_MENU)
                rg_gui_game_menu();
            else
                rg_gui_options_menu();
            continue;
        }
        
        memset(Input_keys, 0, sizeof(Input_keys));
        if (joystick & RG_KEY_UP)    Input_keys[INPUT_UP] = 1;
        if (joystick & RG_KEY_DOWN)  Input_keys[INPUT_DOWN] = 1;
        if (joystick & RG_KEY_LEFT)  Input_keys[INPUT_LEFT] = 1;
        if (joystick & RG_KEY_RIGHT) Input_keys[INPUT_RIGHT] = 1;
        if (joystick & RG_KEY_A)     Input_keys[INPUT_ACCEL] = 1;
        if (joystick & RG_KEY_B)     Input_keys[INPUT_BRAKE] = 1;
        if (joystick & RG_KEY_SELECT) Input_keys[INPUT_COIN] = 1; // Map Select to Coin
        if (joystick & RG_KEY_START)  Input_keys[INPUT_START] = 1;
        if (joystick & RG_KEY_X)     Input_keys[INPUT_GEAR1] = 1;
        if (joystick & RG_KEY_Y)     Input_keys[INPUT_GEAR2] = 1;
        
        bool drawFrame = skipFrames == 0;

        // A single-buffer fallback must wait before modifying the submitted
        // surface. With two buffers the previous target is not reused until a
        // later submit has blocked long enough for Retro-Go to release it.
        if (drawFrame && !screens[1])
        {
            while (rg_display_is_busy())
                rg_task_yield();
        }

        tick(drawFrame);
        Audio_wait();
        
        // Calculate CPU busy time BEFORE blocking submits
        uint32_t elapsed = rg_system_timer() - startTime;
        rg_system_tick(elapsed);
        
        if (drawFrame)
        {
            rg_surface_t *submitted = screen;
            rg_display_submit(submitted, 0);
            presented_screen = submitted;

            if (screens[1])
            {
                screen = screens[submitted == screens[0]];
#if defined(RETRO_GO) && CANNONBALL_DIRECT_RGB565
                Video_set_framebuffer((uint16_t *)screen->data);
#endif
            }
        }
        
        uint32_t audioSubmitElapsed = 0;
        if (Config_sound.enabled)
        {
            // Sync: 22050Hz / 30 FPS = 735 stereo frames
            uint32_t audioSubmitStart = rg_system_timer();
            rg_audio_submit((rg_audio_sample_t*)Audio_mix_buffer, 735);
            audioSubmitElapsed = rg_system_timer() - audioSubmitStart;
        }

        if (skipFrames > 0)
            skipFrames--;

        int requiredSkip = skipFrames;
        if (drawFrame)
        {
            requiredSkip = RG_MAX(requiredSkip, app->frameskip);
            if (elapsed > app->frameTime + 1500)
                requiredSkip = RG_MAX(requiredSkip, (int)(elapsed / app->frameTime));
        }

        // A nearly non-blocking submit means the hardware queue is close to
        // starvation. Keep a small jitter cushion, but do not require the old
        // 4 ms reserve: that converted otherwise valid 29-32 ms renders into
        // an artificial draw/skip alternation.
        if (Config_sound.enabled && audioSubmitElapsed < app->frameTime / 32)
            requiredSkip = RG_MAX(requiredSkip, 1);

        // Avoid an unbounded period without a visual update after an
        // exceptional transition frame.
        skipFrames = RG_MIN(requiredSkip, 3);
        
        Input_frame_done();
    }
}
