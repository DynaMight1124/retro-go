#include <rg_system.h>
#include <rg_surface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Fix MAXPATHLEN conflict
#include <sys/param.h>
#undef MAXPATHLEN
#include "../components/pcsx_rearmed/include/config.h"

#undef _
#include "../components/pcsx_rearmed/libpcsxcore/psxcommon.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxmem.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxmem_map.h"

// Fix EPC conflict on Xtensa
#ifdef EPC
#undef EPC
#endif
#include "../components/pcsx_rearmed/libpcsxcore/r3000a.h"
#include "../components/pcsx_rearmed/frontend/plat.h"
#include "../components/pcsx_rearmed/frontend/plugin_lib.h"
#include "../components/pcsx_rearmed/libpcsxcore/misc.h"
#include "../components/pcsx_rearmed/libpcsxcore/plugins.h"
#include "../components/pcsx_rearmed/libpcsxcore/cdrom-async.h"
#include "../components/pcsx_rearmed/libpcsxcore/psxbios.h"

#define MAP_FAILED ((void *)-1)

rg_surface_t *display_surface = NULL;
static rg_app_t *app;

extern struct rearmed_cbs pl_rearmed_cbs;
extern void emu_set_default_config(void);
extern int OpenPlugins(void);
extern void SysReset(void);
extern unsigned short in_keystate[8];

static void *wrap_psxMapHook(unsigned long addr, size_t size, enum psxMapTag tag, int *can_retry_addr) {
    size_t alignment = 512 * 1024;
    void *ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    RG_LOGI("psxMapHook: addr=%lx size=%x tag=%d -> ptr=%p", addr, (int)size, (int)tag, ptr);
    *can_retry_addr = 1;
    return ptr ? ptr : MAP_FAILED;
}

static void wrap_psxUnmapHook(void *ptr, size_t size, enum psxMapTag tag) {
    RG_LOGI("psxUnmapHook: ptr=%p size=%x", ptr, (int)size);
    if (ptr && ptr != MAP_FAILED) free(ptr);
}

// Platform hooks for PCSX-ReARMed
void plat_init(void) {}
void plat_finish(void) {}
void plat_minimize(void) {}
void *plat_prepare_screenshot(int *w, int *h, int *bpp) { return NULL; }
void plat_gvideo_open(int is_pal) {}
void *plat_gvideo_set_mode(int *w, int *h, int *bpp) {
    *w = 320; *h = 240; *bpp = 16;
    if (display_surface) return display_surface->data;
    return NULL;
}
void *plat_gvideo_flip(void) {
    if (display_surface) rg_display_submit(display_surface, 0);
    return display_surface ? display_surface->data : NULL;
}
void plat_gvideo_close(void) {}

void emu_task(void *pvParameters) {
    RG_LOGI("Emulator Task starting initialization...");

    emu_set_default_config();
    Config.Cpu = CPU_INTERPRETER;
    Config.PsxType = PSX_TYPE_NTSC;
    Config.SlowBoot = 0;
    Config.HLE = 1; 
    
    // cycle_multiplier: Adjusts the virtual clock speed of the PS1 CPU.
    // Higher values (200-500) speed up the game logic but can cause glitches.
    // Lower values (50-100) are more accurate but much slower on an interpreter.
    // Current Peak: 400
    Config.cycle_multiplier = 400; 
    
    psxMapHook = wrap_psxMapHook;
    psxUnmapHook = wrap_psxUnmapHook;

    strcpy(Config.Bios, "HLE");
    strcpy(Config.Gpu, "builtin_gpu");
    strcpy(Config.Spu, "builtin_spu");
    SetIsoFile(app->romPath);

    RG_LOGI("Loading plugins...");
    if (LoadPlugins() == -1) RG_PANIC("LoadPlugins failed");
    
    RG_LOGI("Initializing core...");
    if (psxInit() != 0) RG_PANIC("psxInit failed");
    
    RG_LOGI("Opening plugins...");
    if (OpenPlugins() == -1) RG_PANIC("OpenPlugins failed");

    CheckCdrom();
    
    RG_LOGI("Resetting core...");
    SysReset();

    // frameskip: Instructs the GPU plugin to render only every Nth frame.
    // 0: Render every frame (slowest).
    // 3-5: Good balance for heavy games.
    // 7-9: Maximum speed, very choppy motion.
    // Current Peak: 7
    pl_rearmed_cbs.frameskip = 7;

    RG_LOGI("Loading CDROM EXE from %s...", app->romPath);
    if (LoadCdrom() == -1) {
        RG_LOGE("LoadCdrom failed!");
    }
    
    psxBiosSetupBootState();

    RG_LOGI("Emulation loop starting at PC: 0x%08x", (unsigned int)psxRegs.pc);

    int frame_count = 0;
    while (true) {
        int64_t start_time = rg_system_timer();

        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & RG_KEY_MENU) {
            rg_gui_game_menu();
        }

        in_keystate[0] = 0;
        if (joystick & RG_KEY_UP)     in_keystate[0] |= (1 << DKEY_UP);
        if (joystick & RG_KEY_DOWN)   in_keystate[0] |= (1 << DKEY_DOWN);
        if (joystick & RG_KEY_LEFT)   in_keystate[0] |= (1 << DKEY_LEFT);
        if (joystick & RG_KEY_RIGHT)  in_keystate[0] |= (1 << DKEY_RIGHT);
        if (joystick & RG_KEY_SELECT) in_keystate[0] |= (1 << DKEY_SELECT);
        if (joystick & RG_KEY_START)  in_keystate[0] |= (1 << DKEY_START);
        if (joystick & RG_KEY_A)      in_keystate[0] |= (1 << DKEY_CIRCLE);
        if (joystick & RG_KEY_B)      in_keystate[0] |= (1 << DKEY_CROSS);
        if (joystick & RG_KEY_X)      in_keystate[0] |= (1 << DKEY_TRIANGLE);
        if (joystick & RG_KEY_Y)      in_keystate[0] |= (1 << DKEY_SQUARE);
        if (joystick & RG_KEY_L)      in_keystate[0] |= (1 << DKEY_L1);
        if (joystick & RG_KEY_R)      in_keystate[0] |= (1 << DKEY_R1);

        // Loop Slice: The number of virtual cycles executed before returning to the host OS.
        // 564480: 1 frame (most responsive input).
        // 1128960+: 2-4 frames (reduces OS overhead, higher logic speed).
        // Max Stable: 2257920
        uint32_t end_cycle = psxRegs.cycle + 2257920;
        psxRegs.stop = 0;
        while (psxRegs.cycle < end_cycle && !psxRegs.stop) {
            psxCpu->ExecuteBlock(&psxRegs, EXEC_CALLER_OTHER);
        }

        if (++frame_count % 60 == 0) {
            RG_LOGI("Loop: %d, PC: 0x%08x, Cycle: %u", frame_count, (unsigned int)psxRegs.pc, (unsigned int)psxRegs.cycle);
        }

        rg_system_tick(rg_system_timer() - start_time);
    }
}

void app_main(void) {
    app = rg_system_init(&(const rg_config_t){
        .sampleRate = 44100,
        .frameRate = 60,
        .storageRequired = true,
        .romRequired = true,
    });

    RG_LOGI("Starting PCSX-ReARMed for retro-go...");
    display_surface = rg_surface_create(320, 240, RG_PIXEL_565_LE, 0);

    // Pre-allocate VRAM
    extern uint16_t *g_vram_p;
    if (!g_vram_p) {
        g_vram_p = heap_caps_aligned_alloc(64, 1024 * 512 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!g_vram_p) RG_PANIC("VRAM allocation failed! Out of SPIRAM?");
        memset(g_vram_p, 0, 1024 * 512 * 2);
    }

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if (!rg_task_create("emu_task", emu_task, NULL, 64 * 1024, 0, RG_TASK_PRIORITY_5, 1)) {
#else
    if (!rg_task_create("emu_task", emu_task, NULL, 48 * 1024, 0, RG_TASK_PRIORITY_5, 1)) {
#endif
        RG_PANIC("Failed to create emu_task");
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
