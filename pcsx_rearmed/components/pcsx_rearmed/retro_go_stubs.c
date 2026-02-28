#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <rg_system.h>
#include <rg_surface.h>

#undef _
#include "libpcsxcore/psxcommon.h"
#include "frontend/plugin_lib.h"
#include "libpcsxcore/new_dynarec/new_dynarec.h"
#include "libpcsxcore/plugins.h"

#ifdef __cplusplus
extern "C" {
#endif

// Full definition of ndrc_globals
struct ndrc_globals ndrc_g;

// Pointer for rearmed callbacks
void (*GPU_rearmedCallbacks_ptr)(const struct rearmed_cbs *cbs);

// Forward declarations of builtin plugin entry points
extern long builtin_GPUinit(void);
extern long builtin_GPUshutdown(void);
extern long builtin_GPUclose(void);
extern void builtin_GPUwriteStatus(uint32_t);
extern void builtin_GPUwriteData(uint32_t);
extern void builtin_GPUwriteDataMem(uint32_t *, int);
extern uint32_t builtin_GPUreadStatus(void);
extern uint32_t builtin_GPUreadData(void);
extern void builtin_GPUreadDataMem(uint32_t *, int);
extern long builtin_GPUdmaChain(uint32_t *, uint32_t, uint32_t *, int32_t *);
extern void builtin_GPUupdateLace(void);
extern void builtin_GPUvBlank(int, int);
extern void builtin_GPUgetScreenInfo(int *, int *);
extern long builtin_GPUopen(unsigned long *, char *, char *);
extern void builtin_GPUrearmedCallbacks(const struct rearmed_cbs *cbs);

extern long builtin_SPUinit(void);
extern long builtin_SPUshutdown(void);
extern long builtin_SPUopen(void);
extern long builtin_SPUclose(void);
extern void builtin_SPUwriteRegister(unsigned long, unsigned short, unsigned int);
extern unsigned short builtin_SPUreadRegister(unsigned long, unsigned int);
extern void builtin_SPUwriteDMAMem(unsigned short *, int, unsigned int);
extern void builtin_SPUreadDMAMem(unsigned short *, int, unsigned int);
extern void builtin_SPUplayADPCMchannel(xa_decode_t *, unsigned int, int);
extern void builtin_SPUasync(unsigned int, unsigned int);
extern int  builtin_SPUplayCDDAchannel(short *, int, unsigned int, int);
extern void builtin_SPUsetCDvol(unsigned char, unsigned char, unsigned char, unsigned char, unsigned int);
extern void builtin_SPUregisterCallback(void (*callback)(int));
extern void builtin_SPUregisterScheduleCb(void (*callback)(unsigned int));

void SysPrintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void SysMessage(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void *SysLoadLibrary(const char *lib) {
    return (void *)0x1234;
}

void *SysLoadSym(void *lib, const char *sym) {
    if (strcmp(sym, "GPUinit") == 0) return (void *)builtin_GPUinit;
    if (strcmp(sym, "GPUshutdown") == 0) return (void *)builtin_GPUshutdown;
    if (strcmp(sym, "GPUclose") == 0) return (void *)builtin_GPUclose;
    if (strcmp(sym, "GPUopen") == 0) return (void *)builtin_GPUopen;
    if (strcmp(sym, "GPUwriteStatus") == 0) return (void *)builtin_GPUwriteStatus;
    if (strcmp(sym, "GPUwriteData") == 0) return (void *)builtin_GPUwriteData;
    if (strcmp(sym, "GPUwriteDataMem") == 0) return (void *)builtin_GPUwriteDataMem;
    if (strcmp(sym, "GPUreadStatus") == 0) return (void *)builtin_GPUreadStatus;
    if (strcmp(sym, "GPUreadData") == 0) return (void *)builtin_GPUreadData;
    if (strcmp(sym, "GPUreadDataMem") == 0) return (void *)builtin_GPUreadDataMem;
    if (strcmp(sym, "GPUdmaChain") == 0) return (void *)builtin_GPUdmaChain;
    if (strcmp(sym, "GPUupdateLace") == 0) return (void *)builtin_GPUupdateLace;
    if (strcmp(sym, "GPUvBlank") == 0) return (void *)builtin_GPUvBlank;
    if (strcmp(sym, "GPUgetScreenInfo") == 0) return (void *)builtin_GPUgetScreenInfo;
    if (strcmp(sym, "GPUrearmedCallbacks") == 0) return (void *)builtin_GPUrearmedCallbacks;

    if (strcmp(sym, "SPUinit") == 0) return (void *)builtin_SPUinit;
    if (strcmp(sym, "SPUshutdown") == 0) return (void *)builtin_SPUshutdown;
    if (strcmp(sym, "SPUopen") == 0) return (void *)builtin_SPUopen;
    if (strcmp(sym, "SPUclose") == 0) return (void *)builtin_SPUclose;
    if (strcmp(sym, "SPUwriteRegister") == 0) return (void *)builtin_SPUwriteRegister;
    if (strcmp(sym, "SPUreadRegister") == 0) return (void *)builtin_SPUreadRegister;
    if (strcmp(sym, "SPUwriteDMAMem") == 0) return (void *)builtin_SPUwriteDMAMem;
    if (strcmp(sym, "SPUreadDMAMem") == 0) return (void *)builtin_SPUreadDMAMem;
    if (strcmp(sym, "SPUplayADPCMchannel") == 0) return (void *)builtin_SPUplayADPCMchannel;
    if (strcmp(sym, "SPUregisterCallback") == 0) return (void *)builtin_SPUregisterCallback;
    if (strcmp(sym, "SPUregisterScheduleCb") == 0) return (void *)builtin_SPUregisterScheduleCb;
    if (strcmp(sym, "SPUasync") == 0) return (void *)builtin_SPUasync;
    if (strcmp(sym, "SPUplayCDDAchannel") == 0) return (void *)builtin_SPUplayCDDAchannel;
    if (strcmp(sym, "SPUsetCDvol") == 0) return (void *)builtin_SPUsetCDvol;

    return NULL;
}

const char *SysLibError() { return NULL; }
void SysCloseLibrary(void *lib) {}
void SysRunGui() {}
void SysClose() {
    EmuShutdown();
    ReleasePlugins();
}

void SysReset() {
    EmuReset();
}

void pl_timing_prepare(int is_pal) {}

void pl_init(void) {
    extern unsigned int hSyncCount;
    extern unsigned int frame_counter;
    pl_rearmed_cbs.gpu_hcnt = &hSyncCount;
    pl_rearmed_cbs.gpu_frame_count = &frame_counter;
}

void netpacket_poll_receive() {}
void netpacket_send(uint16_t client_id, const void *buf, size_t len) {}

void *GPU_prepare_screenshot(int *w, int *h, int *bpp) { return NULL; }

void plat_trigger_vibrate(int pad, int low, int high) {}

extern unsigned short in_keystate[8];

long PAD1_readPort(PadDataS *pad) {
    if (pad) {
        pad->controllerType = 4; // PSE_PAD_TYPE_STANDARD
        pad->buttonStatus = ~in_keystate[0];
    }
    return 0;
}

long PAD2_readPort(PadDataS *pad) {
    if (pad) {
        pad->controllerType = 4;
        pad->buttonStatus = ~in_keystate[1];
    }
    return 0;
}

void in_update(void) {}
void in_update_analog(int pad, int axis, int value) {}

struct {
    int video_depth;
    int frame_skip;
    int show_fps;
    int show_hud;
} g_opts;

char hud_msg[256];
int hud_new_msg;
int g_scaler;
int g_menuscreen_w, g_menuscreen_h;
int g_emu_resetting;
int emu_action, emu_action_old;
int ready_to_go;
int g_emu_want_quit;
unsigned long gpuDisp;
int state_slot;

int in_type[8];
unsigned short in_keystate[8];

void pl_gun_byte2(int port, unsigned char byte) {}
void pl_frame_limit(void) {}

struct rearmed_cbs pl_rearmed_cbs;

void menu_notify_mode_change(int w, int h, int bpp) {}
void basic_text_out16_nf(void *fb, int w, int x, int y, const char *text) {}
void spu_get_debug_info(void *info) {}

extern rg_surface_t *display_surface;

static int wrap_rg_display_init(void) {
    rg_display_init();
    return 0;
}

static void wrap_pl_vout_set_mode(int w, int h, int raw_w, int raw_h, int bpp) {
    int target_w = (w > 320) ? 320 : w;
    int target_h = (h > 240) ? 240 : h;

    if (display_surface && (display_surface->width != target_w || display_surface->height != target_h)) {
        rg_surface_free(display_surface);
        display_surface = rg_surface_create(target_w, target_h, RG_PIXEL_565_LE, 0);
    } else if (!display_surface) {
        display_surface = rg_surface_create(target_w, target_h, RG_PIXEL_565_LE, 0);
    }
    rg_display_set_geometry(target_w, target_h, NULL);
}

static void wrap_pl_vout_flip(const void *vram, int vram_offset, int bgr24,
                              int x, int y, int w, int h, int dims_changed) {
    if (display_surface && vram) {
        int target_w = (w > 320) ? 320 : w;
        int target_h = (h > 240) ? 240 : h;
        
        uint16_t *src_base = (uint16_t *)((uint8_t *)vram + vram_offset);
        uint16_t *dst = (uint16_t *)display_surface->data;
        
        // Stride is always 1024 for PS1 VRAM
        if (w > 320 || h > 240) {
            int step_x = (w << 8) / target_w;
            int step_y = (h << 8) / target_h;

            for (int j = 0; j < target_h; j++) {
                uint16_t *line_src = src_base + (((j * step_y) >> 8) * 1024);
                uint16_t *line_dst = dst + (j * target_w);
                for (int i = 0; i < target_w; i++) {
                    uint16_t c = line_src[(i * step_x) >> 8];
                    line_dst[i] = ((c & 0x001F) << 11) | (c & 0x03E0) | ((c & 0x7C00) >> 10);
                }
            }
        } else {
            for (int j = 0; j < h; j++) {
                uint16_t *line_src = src_base + (j * 1024);
                uint16_t *line_dst = dst + (j * w);
                for (int i = 0; i < w; i++) {
                    uint16_t c = line_src[i];
                    line_dst[i] = ((c & 0x001F) << 11) | (c & 0x03E0) | ((c & 0x7C00) >> 10);
                }
            }
        }
        rg_display_submit(display_surface, 0);
    }
}

uint16_t *g_vram_p = NULL;

static void *wrap_mmap(unsigned int size) {
    if (size == 1024 * 512 * 2 && g_vram_p) {
        return g_vram_p;
    }
    size_t alignment = 64; 
    void *ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    RG_LOGI("wrap_mmap: size=%u -> ptr=%p", size, ptr);
    return ptr;
}

static void wrap_munmap(void *ptr, unsigned int size) {
    if (ptr) free(ptr);
}

void emu_set_default_config(void)
{
	Config.Xa = Config.Cdda = 0;
	Config.icache_emulation = 0;
	Config.PsxAuto = 1;
	Config.cycle_multiplier = 400; 
	Config.GpuListWalking = 1;
	Config.FractionalFramerate = -1;
    Config.HLE = 1;

	pl_rearmed_cbs.frameskip = 7;
	pl_rearmed_cbs.only_16bpp = 1;
	pl_rearmed_cbs.dithering = 0;
	pl_rearmed_cbs.thread_rendering = 1;
	pl_rearmed_cbs.gpu_neon.allow_interlace = 0; 
	pl_rearmed_cbs.gpu_peops.dwActFixes = 1<<7;
	pl_rearmed_cbs.gpu_unai.lighting = 0;
	pl_rearmed_cbs.gpu_unai.fast_lighting = 0;
	pl_rearmed_cbs.gpu_unai.blending = 0;
	pl_rearmed_cbs.gpu_unai.pixel_skip = 1;

    pl_rearmed_cbs.mmap = wrap_mmap;
    pl_rearmed_cbs.munmap = wrap_munmap;
    pl_rearmed_cbs.pl_vout_open = wrap_rg_display_init;
    pl_rearmed_cbs.pl_vout_set_mode = wrap_pl_vout_set_mode;
    pl_rearmed_cbs.pl_vout_flip = wrap_pl_vout_flip;
    pl_rearmed_cbs.pl_vout_close = rg_display_deinit;
}

extern int cdra_open(void);

int OpenPlugins(void) {
    GPU_init = builtin_GPUinit;
    GPU_shutdown = builtin_GPUshutdown;
    GPU_open = builtin_GPUopen;
    GPU_close = builtin_GPUclose;
    GPU_readStatus = builtin_GPUreadStatus;
    GPU_readData = builtin_GPUreadData;
    GPU_readDataMem = builtin_GPUreadDataMem;
    GPU_writeStatus = builtin_GPUwriteStatus;
    GPU_writeData = builtin_GPUwriteData;
    GPU_writeDataMem = builtin_GPUwriteDataMem;
    GPU_dmaChain = builtin_GPUdmaChain;
    GPU_updateLace = builtin_GPUupdateLace;
    GPU_vBlank = builtin_GPUvBlank;
    GPU_getScreenInfo = builtin_GPUgetScreenInfo;
    GPU_rearmedCallbacks_ptr = builtin_GPUrearmedCallbacks;

    SPU_init = builtin_SPUinit;
    SPU_shutdown = builtin_SPUshutdown;
    SPU_open = builtin_SPUopen;
    SPU_close = builtin_SPUclose;
    SPU_writeRegister = builtin_SPUwriteRegister;
    SPU_readRegister = builtin_SPUreadRegister;
    SPU_writeDMAMem = builtin_SPUwriteDMAMem;
    SPU_readDMAMem = builtin_SPUreadDMAMem;
    SPU_playADPCMchannel = builtin_SPUplayADPCMchannel;
    SPU_registerCallback = builtin_SPUregisterCallback;
    SPU_registerScheduleCb = builtin_SPUregisterScheduleCb;
    SPU_async = builtin_SPUasync;
    SPU_playCDDAchannel = builtin_SPUplayCDDAchannel;
    SPU_setCDvol = builtin_SPUsetCDvol;

    pl_init();
    GPU_rearmedCallbacks_ptr(&pl_rearmed_cbs);

    const char *iso = GetIsoFile();
    if (iso && iso[0]) {
        if (cdra_open() < 0) return -1;
    }
	if (GPU_open(NULL, NULL, NULL) < 0) return -1;
	if (SPU_open() < 0) return -1;
	return 0;
}

void new_dynarec_init() {}
void new_dyna_start(void *context) {}
void new_dynarec_cleanup() {}
void new_dynarec_clear_full() {}
void new_dynarec_invalidate_all_pages() {}
void new_dynarec_invalidate_range(unsigned int start, unsigned int end) {}
void new_dyna_pcsx_mem_init(void) {}
void new_dyna_pcsx_mem_reset(void) {}
void new_dyna_pcsx_mem_load_state(void) {}
void new_dyna_pcsx_mem_isolate(int enable) {}
void new_dyna_pcsx_mem_isolate_2(int enable) {}
void new_dyna_pcsx_mem_shutdown(void) {}
int  new_dynarec_save_blocks(void *save, int size) { return 0; }
void new_dynarec_load_blocks(const void *save, int size) {}

void *plat_mmap(unsigned long addr, size_t size, int prot, int flags) {
    size_t alignment = 1024 * 1024;
    void *ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    RG_LOGI("plat_mmap: addr=%lx size=%x -> ptr=%p", addr, (int)size, ptr);
    return ptr;
}

void plat_munmap(void *ptr, size_t size) {
    if (ptr) {
        RG_LOGI("plat_munmap: ptr=%p", ptr);
        free(ptr);
    }
}

#ifdef __cplusplus
}
#endif
