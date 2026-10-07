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
#include "libpcsxcore/spu.h"
#include "plugins/dfsound/spu_config.h"
#include "port_video.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PCSX_NDRC_RV32_FULL_CORE
/* The Retro-Go frontend's periodic diagnostics are shared with the Lightrec
 * build. Keep that interface available while the full RV32 core owns CPU
 * execution; dedicated new_dynarec counters can replace these zeroes later. */
void lightrec_plugin_debug_poll(void) {}

bool lightrec_plugin_failed(void)
{
    return false;
}

#endif

#if !defined(PCSX_NDRC_RV32_FULL_CORE) && !defined(PCSX_DUAL_DYNAREC)
// The Lightrec build still references the frontend's new_dynarec hooks.
struct ndrc_globals ndrc_g;

/* Keep the P4 frontend's temporary statistics hook linkable when a build is
 * explicitly switched back to Lightrec. */
void new_dynarec_print_stats(void) {}
#endif

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
/* gpulib exports GPUfreeze without the builtin_ prefix. Use an alias because
 * plugins.h already uses GPUfreeze as the function-pointer type name. */
extern long builtin_GPUfreeze(uint32_t, GPUFreeze_t *, uint16_t **)
    __asm__("GPUfreeze");
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
extern long builtin_SPUfreeze(int, SPUFreeze_t *, unsigned short **, void *, unsigned int);
extern void builtin_SPUregisterCallback(void (*)(int));
extern void builtin_SPUregisterScheduleCb(void (*)(unsigned int));
extern void builtin_SPUasync(unsigned int, unsigned int);
extern int builtin_SPUplayCDDAchannel(short *, int, unsigned int, int);
extern void builtin_SPUsetCDvol(unsigned char, unsigned char,
        unsigned char, unsigned char, unsigned int);
extern void dfsound_set_output_enabled(int enabled, unsigned int cycles);

static bool retrogo_sound_enabled = true;

static void retrogo_SPUasync(unsigned int cycles, unsigned int flags)
{
    /* Even with audio output disabled, the SPU must advance its clock and
     * deliver emulated IRQs. Games can wait for those during boot. dfsound's
     * muted path skips decoding, mixing, reverb and output. */
    builtin_SPUasync(cycles, flags);
}

static void retrogo_SPUplayADPCMchannel(xa_decode_t *xap,
        unsigned int cycles, int is_start)
{
    if (retrogo_sound_enabled)
        builtin_SPUplayADPCMchannel(xap, cycles, is_start);
}

static int retrogo_SPUplayCDDAchannel(short *pcm, int bytes,
        unsigned int cycles, int is_start)
{
    if (!retrogo_sound_enabled)
        return 0;
    return builtin_SPUplayCDDAchannel(pcm, bytes, cycles, is_start);
}

void retrogo_spu_set_options(bool enabled, bool fast_mode)
{
    /* This is a no-op before SPUinit. At runtime it also drains any pending
     * worker item before changing mixer mode and discards stale output. */
    dfsound_set_output_enabled(enabled, psxRegs.cycle);
    retrogo_sound_enabled = enabled;
    /* In PCSX's historical config, non-zero means disable XA/CDDA. Avoid
     * decoding and feeding those streams as well as bypassing SPU mixing. */
    Config.Xa = Config.Cdda = enabled ? 0 : 1;
    spu_config.iUseReverb = fast_mode ? 0 : 1;
    spu_config.iUseInterpolation = fast_mode ? 0 : 1;
    RG_LOGI("SPU: %s, quality=%s, worker=%s",
            enabled ? "enabled" : "disabled",
            fast_mode ? "fast" : "accurate",
            spu_config.iUseThread ? "enabled" : "disabled");
}

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
    if (strcmp(sym, "GPUfreeze") == 0) return (void *)builtin_GPUfreeze;
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
    if (strcmp(sym, "SPUfreeze") == 0) return (void *)builtin_SPUfreeze;
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
void pl_frame_limit(void) {
    /* Match the libretro frontend contract: psxCpu->Execute() returns after
     * the core reaches the next emulated video frame. Retro-Go performs its
     * own pacing in the application loop. */
    psxRegs.stop++;
}

struct rearmed_cbs pl_rearmed_cbs;

void menu_notify_mode_change(int w, int h, int bpp) {}
void basic_text_out16_nf(void *fb, int w, int x, int y, const char *text) {}
extern rg_surface_t *display_surface;
volatile uint32_t retrogo_vout_flip_count;
uint64_t retrogo_display_wait_us;
static int output_width = 320, output_height = 240;

static int wrap_pl_vout_open(void) {
    /* rg_system_init() owns the Retro-Go display lifecycle. */
    return 0;
}

static void wrap_pl_vout_close(void) {
    /* The launcher, not the emulated GPU, owns the Retro-Go display. */
}

static void wait_for_display(void) {
    int64_t start = rg_system_timer();
    while (rg_display_is_busy())
        rg_task_yield();
    retrogo_display_wait_us += (uint64_t)(rg_system_timer() - start);
}


static void wrap_pl_vout_set_mode(int w, int h, int raw_w, int raw_h, int bpp) {
    if (w <= 0 || h <= 0)
        return;
    output_width = w;
    output_height = h;
    int target_w = (w > 320) ? 320 : w;
    int target_h = (h > 240) ? 240 : h;

    if (display_surface && (display_surface->width != target_w || display_surface->height != target_h)) {
        wait_for_display();
        rg_surface_free(display_surface);
        display_surface = rg_surface_create(target_w, target_h, RG_PIXEL_565_LE, 0);
    } else if (!display_surface) {
        display_surface = rg_surface_create(target_w, target_h, RG_PIXEL_565_LE, 0);
    }
    if (!display_surface)
        RG_PANIC("Display surface allocation failed");
    wait_for_display();
    memset(display_surface->data, 0, display_surface->stride * display_surface->height);
    rg_display_set_geometry(target_w, target_h, NULL);
}

static void wrap_pl_vout_flip(const void *vram, int vram_offset, int bgr24,
                              int x, int y, int w, int h, int dims_changed) {
    if (display_surface) {
        int target_w = display_surface->width;
        int target_h = display_surface->height;
        int stride = display_surface->stride / 2;

        /* rg_display_submit() is asynchronous; do not overwrite its surface. */
        wait_for_display();

        if (!vram || dims_changed)
            memset(display_surface->data, 0, display_surface->stride * target_h);
        if (!vram) {
            rg_display_submit(display_surface, 0);
            retrogo_vout_flip_count++;
            return;
        }
        if (w <= 0 || h <= 0)
            return;

        pcsx_port_blit(display_surface->data, stride, target_w, target_h,
            vram, vram_offset, bgr24, x, y, w, h, output_width, output_height);
        rg_display_submit(display_surface, 0);
        retrogo_vout_flip_count++;
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
	/* Use the upstream compatibility database. Forced slow walking breaks GPU
	 * DMA chains into many small callbacks and is particularly expensive now
	 * that each chunk must also be queued to the asynchronous renderer. */
	Config.GpuListWalking = -1;
	Config.FractionalFramerate = -1;
    Config.HLE = 1;

    /* Start with upstream's balanced SPU defaults. These preserve XA/CDDA,
     * interpolation and reverb accuracy independently of the worker thread. */
    spu_config.iUseReverb = 1;
    spu_config.iUseInterpolation = 1;
    spu_config.iXAPitch = 0;
    spu_config.iVolume = 768;
    spu_config.iTempo = 0;
    /* dfsound's channel worker runs on the other CPU core. The option remains
     * harmless on a single-core target because SPUinit marks it unavailable. */
    spu_config.iUseThread = 1;

	/* Let gpulib skip only while emulation is behind. Fixed skip values also
	 * discard frames when the core is keeping up, making 30 Hz games choppy. */
	pl_rearmed_cbs.frameskip = -1;
	pl_rearmed_cbs.only_16bpp = 1;
	pl_rearmed_cbs.dithering = 0;
	pl_rearmed_cbs.thread_rendering = 1;
	pl_rearmed_cbs.gpu_neon.allow_interlace = 0; 
	pl_rearmed_cbs.gpu_peops.dwActFixes = 1<<7;
	/* These are core PSX GPU operations, not optional presentation effects.
	 * Disabling them turns colour-modulated shadows white and renders
	 * semi-transparent highlights as opaque pixels. Match upstream UNAI's
	 * correctness defaults; fast_lighting remains off for accurate colour. */
	pl_rearmed_cbs.gpu_unai.lighting = 1;
	pl_rearmed_cbs.gpu_unai.fast_lighting = 0;
	pl_rearmed_cbs.gpu_unai.blending = 1;
	pl_rearmed_cbs.gpu_unai.ilace_force = 0;
	pl_rearmed_cbs.gpu_unai.pixel_skip = 1;

    pl_rearmed_cbs.mmap = wrap_mmap;
    pl_rearmed_cbs.munmap = wrap_munmap;
    pl_rearmed_cbs.pl_vout_open = wrap_pl_vout_open;
    pl_rearmed_cbs.pl_vout_set_mode = wrap_pl_vout_set_mode;
    pl_rearmed_cbs.pl_vout_flip = wrap_pl_vout_flip;
    pl_rearmed_cbs.pl_vout_close = wrap_pl_vout_close;
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
    GPU_freeze = builtin_GPUfreeze;
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
    SPU_playADPCMchannel = retrogo_SPUplayADPCMchannel;
    SPU_freeze = builtin_SPUfreeze;
    SPU_registerCallback = builtin_SPUregisterCallback;
    SPU_registerScheduleCb = builtin_SPUregisterScheduleCb;
    SPU_async = retrogo_SPUasync;
    SPU_playCDDAchannel = retrogo_SPUplayCDDAchannel;
    SPU_setCDvol = builtin_SPUsetCDvol;

    pl_init();
    GPU_rearmedCallbacks_ptr(&pl_rearmed_cbs);

    const char *iso = GetIsoFile();
    if (iso && iso[0]) {
        if (cdra_open() < 0) return -1;
    }
	if (GPU_open(NULL, NULL, NULL) < 0) return -1;
	if (SPU_open() < 0) return -1;
	SPU_registerCallback(SPUirq);
	SPU_registerScheduleCb(SPUschedule);
	return 0;
}

/* Differential tracing is disabled in normal builds. Keep its cold-reset
 * hook available now that the old null SPU no longer supplies one. */
void builtin_SPUresetDiagnostic(void) {}

#if !defined(PCSX_NDRC_RV32_FULL_CORE) && !defined(PCSX_DUAL_DYNAREC)
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
#endif

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
