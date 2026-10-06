/**
 * PSP entry point and game loop host.
 *
 * On the N64 main_func() spins up idle/video/audio/game threads that talk
 * through message queues and RSP tasks.  Here the game loop runs on the main
 * thread: each iteration reads the pad, runs one game tick (which builds a
 * display list), executes that display list on the GE, mixes the audio for
 * the frame and waits for vsync.
 */
#include <ultra64.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <pspge.h>
#include <pspgu.h>
#include <stdio.h>
#include <string.h>

#include <ultra64.h>
#include <macros.h>
#include "port.h"
#include "gfx_pc.h"
#include "gfx_window_manager_api.h"
#include "gfx_rendering_api.h"

PSP_MODULE_INFO("MK64", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
/* The port mallocs almost nothing (file buffers, a few KB): a fixed C heap
 * leaves the rest of user RAM with the kernel, where thread stacks and system
 * dialogs draw from.  A negative value here handed everything but 1 MB to the
 * C heap and made every static saving invisible. */
PSP_HEAP_SIZE_KB(512);

extern struct GfxWindowManagerAPI gfx_psp;
extern void port_audio_out_init(void);
extern struct GfxRenderingAPI gfx_opengl_api; // gfx_scegu.c keeps the sm64-port name
#ifdef PORT_NET
#include "../net/port_net.h"
#endif

/* Game-side entry points (main.c, TARGET_PSP variants). */
extern void port_game_init(void);
extern void port_game_loop_one_iteration(void);

static int sRunning = 1;

static int exit_callback(UNUSED int arg1, UNUSED int arg2, UNUSED void* common) {
    extern void port_me_stop(void);
    port_me_stop(); /* park the Media Engine: its loop polls memory that is about to be freed */
    port_log_flush(); /* a race's log lines are held in RAM */
    sRunning = 0;
    sceKernelExitGame();
    return 0;
}

static int callback_thread(UNUSED SceSize args, UNUSED void* argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
}

void port_gfx_run(Gfx* dl) {
    gfx_run(dl);
}

void port_gfx_start_frame(void) {
    gfx_start_frame();
}

void port_gfx_end_frame(void) {
    gfx_end_frame();
}


void port_input_poll(void) {
}

/* Called by audio_psp.c's init; the mixer thread arrives with the audio work. */
void init_audiomanager(void) {
}

/* ------------------------------------------------------------------------- */
/* On-screen FPS counter: a 4x6 digit font drawn as 2D sprites at frame end.  */
/* ------------------------------------------------------------------------- */
static const u8 sDigitFont[11][6] = { // 4 wide x 6 tall, MSB = leftmost
    { 0x60, 0x90, 0x90, 0x90, 0x90, 0x60 }, // 0
    { 0x20, 0x60, 0x20, 0x20, 0x20, 0x70 }, // 1
    { 0x60, 0x90, 0x10, 0x20, 0x40, 0xF0 }, // 2
    { 0xE0, 0x10, 0x60, 0x10, 0x10, 0xE0 }, // 3
    { 0x90, 0x90, 0x90, 0xF0, 0x10, 0x10 }, // 4
    { 0xF0, 0x80, 0xE0, 0x10, 0x10, 0xE0 }, // 5
    { 0x60, 0x80, 0xE0, 0x90, 0x90, 0x60 }, // 6
    { 0xF0, 0x10, 0x20, 0x20, 0x40, 0x40 }, // 7
    { 0x60, 0x90, 0x60, 0x90, 0x90, 0x60 }, // 8
    { 0x60, 0x90, 0x90, 0x70, 0x10, 0x60 }, // 9
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 }, // .
};
static u16 sFontTex[64 * 8] __attribute__((aligned(16))); // 11 glyphs of 4x6 in a 64x8 5551 texture
static u32 sFontTexId;
static u32 sFpsShown; // fps * 10

static void overlay_build_font(void) {
    int g, y, x;
    for (g = 0; g < 11; g++) {
        for (y = 0; y < 6; y++) {
            for (x = 0; x < 4; x++) {
                sFontTex[y * 64 + g * 5 + x] = (sDigitFont[g][y] & (0x80 >> x)) ? 0xFFFF : 0x0000; // 5551: opaque white / transparent
            }
        }
    }
}

void port_gfx_overlay(void) {
    extern void gfx_scegu_draw_triangles_2d(float buf_vbo[], size_t len, size_t n);
    struct GfxRenderingAPI* r = &gfx_opengl_api;
    static u32 sLastT, sFrames;
    static int sInited;
    u32 now = sceKernelGetSystemTimeLow();
    char text[8];
    int i, x = 4;
    if (!sInited) {
        overlay_build_font();
        sInited = 1;
        sLastT = now;
    }
    sFrames++;
    if (now - sLastT >= 500000) { // update twice a second
        sFpsShown = (u32) ((u64) sFrames * 10000000 / (now - sLastT));
        sFrames = 0;
        sLastT = now;
    }
    snprintf(text, sizeof(text), "%u.%u", sFpsShown / 10, sFpsShown % 10);
    {
        struct ShaderProgram* prg = r->lookup_shader(0x05000045); // texture only, alpha test (texture edge)
        if (prg == NULL) {
            prg = r->create_and_load_new_shader(0x05000045);
        } else {
            r->load_shader(prg);
        }
    }
    r->set_depth_test(false);
    r->set_use_alpha(true);
    r->set_viewport(0, 0, 480, 272);
    r->set_scissor(0, 0, 480, 272);
    // One font texture, made again only after the arena was reset behind our
    // back.  It used to be a NEW texture every frame: at 60 pictures a second
    // that alone used up the arena's 512 slots every ~7 s, and each reset made
    // the race re-upload every texture in view (seen on hardware as an arena
    // reset every 6 s whenever the counter was on).
    {
        extern unsigned int texman_generation(void);
        static unsigned int sFontGen = (unsigned int) -1;
        if (sFontGen != texman_generation()) {
            sFontGen = texman_generation();
            sFontTexId = r->new_texture();
            r->select_texture(0, sFontTexId);
            r->upload_texture((const u8*) sFontTex, 64, 8, 1 /* GU_PSM_5551 */);
        } else {
            r->select_texture(0, sFontTexId);
        }
    }
    r->set_sampler_parameters(0, false, 2 /* G_TX_CLAMP */, 2);
    r->set_depth_mask(false);
    r->set_zmode_decal(false);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0x55, 0xff);
    sceGuEnable(GU_BLEND);
    for (i = 0; text[i] != 0; i++) {
        int g = text[i] == '.' ? 10 : text[i] - '0';
        struct { u16 u, v; u32 color; u16 x, y, z; } spr[2] = {
            { (u16) (g * 5), 0, 0xFF00FF00, (u16) x, 4, 0 },
            { (u16) (g * 5 + 4), 6, 0xFF00FF00, (u16) (x + 4), 10, 0 } // 1x: 4x6 px glyphs
        };
        gfx_scegu_draw_triangles_2d((float*) spr, 0, 1);
        x += 5;
    }
    // Force the interpreter to re-apply its own state next frame.
    {
        extern void gfx_overlay_state_dirty(void);
        gfx_overlay_state_dirty();
    }
}

u32 port_time_us(void) {
    return sceKernelGetSystemTimeLow();
}

/* Frame profile: accumulate per-slot microseconds, report every 300 frames. */
void port_profile_add(int slot, u32 us) {
    static u32 sum[8], frames;
    sum[slot] += us;
    if (slot == 3 && ++frames == 300) {
        extern u32 gfx_prof_cmds, gfx_prof_tris, gfx_prof_opcount[256];
        int k, best;
        extern u32 gfx_prof_rebuilds;
        extern u32 port_audio_out_underruns(void);
        PORT_LOG("audio underruns so far: %u\n", port_audio_out_underruns());
#ifdef PORT_PROFILE
        { extern void port_seg_report(void); port_seg_report(); } /* main.c: a split-frame picture outside the interpreter */
#endif
PORT_LOG("profile us/frame: logic %u  dl->ge %u (ge+vsync %u)  audio %u  total %u  (%u rebuilds)\n",
                 sum[0] / 300, sum[1] / 300, sum[2] / 300, sum[3] / 300,
                 (sum[0] + sum[1] + sum[2] + sum[3]) / 300, gfx_prof_rebuilds / 300);
#ifdef PORT_PROFILE_DL
        {
            extern u32 gfx_prof_vtx, gfx_prof_cullclip, gfx_prof_state, gfx_prof_emit, gfx_prof_flush;
            PORT_LOG("  dl breakdown us/frame: vtxproc %u  cull+clip %u  staterebuild %u  emit %u  flush %u\n",
                     gfx_prof_vtx / 300, gfx_prof_cullclip / 300, gfx_prof_state / 300, gfx_prof_emit / 300, gfx_prof_flush / 300);
            {
                extern u32 gfx_prof_tri1calls, gfx_prof_clipcalls, gfx_prof_vtxcount;
                PORT_LOG("  counts/frame: tri1 %u  clipped %u  verts %u\n", gfx_prof_tri1calls / 300, gfx_prof_clipcalls / 300, gfx_prof_vtxcount / 300);
                gfx_prof_tri1calls = gfx_prof_clipcalls = gfx_prof_vtxcount = 0;
            }
            gfx_prof_vtx = gfx_prof_cullclip = gfx_prof_state = gfx_prof_emit = gfx_prof_flush = 0;
        }
#endif
        gfx_prof_rebuilds = 0;
        for (k = 0; k < 10; k++) {
            int i;
            best = 0;
            for (i = 1; i < 256; i++) if (gfx_prof_opcount[i] > gfx_prof_opcount[best]) best = i;
            if (gfx_prof_opcount[best] == 0) break;
            PORT_LOG("  op %02X: %u/frame\n", best, gfx_prof_opcount[best] / 300);
            gfx_prof_opcount[best] = 0;
        }
        memset(gfx_prof_opcount, 0, sizeof(gfx_prof_opcount));
        gfx_prof_cmds = gfx_prof_tris = 0;
        memset(sum, 0, sizeof(sum));
        frames = 0;
    }
}

static char sEbootDir[192] = "ms0:/PSP/GAME/MK64Portable/";
static char sSaveDir[200] = "ms0:/PSP/GAME/MK64Portable/data/";
const char* port_eboot_dir(void) { return sEbootDir; }
const char* port_save_dir(void) { return sSaveDir; }
const char* port_save_path(const char* name) {
    static char buf[4][256];
    static int i;
    char* b = buf[i++ & 3];
    snprintf(b, 256, "%s%s", sSaveDir, name);
    return b;
}
void port_set_save_dir(const char* argv0) {
    const char* slash = argv0 ? strrchr(argv0, '/') : NULL;
    if (slash != NULL && (size_t) (slash + 1 - argv0) < sizeof(sEbootDir)) {
        memcpy(sEbootDir, argv0, slash + 1 - argv0);
        sEbootDir[slash + 1 - argv0] = 0;
    }
    {
        char dir[200];
        SceUID d;
        snprintf(dir, sizeof(dir), "%sdata", sEbootDir); /* no trailing slash: the PSP's FAT driver rejects it */
        sceIoMkdir(dir, 0777);                            /* harmless if it exists */
        d = sceIoDopen(dir);
        if (d >= 0) {
            sceIoDclose(d);
            snprintf(sSaveDir, sizeof(sSaveDir), "%s/", dir);
        } else {
            snprintf(sSaveDir, sizeof(sSaveDir), "%s", sEbootDir); /* could not create data/: write beside the EBOOT */
        }
    }
}
void port_fs_init(void) {
    /* data/ is created by port_set_save_dir(); nothing else to prepare. */
}

void port_fs_mkdir(const char* path) {
    sceIoMkdir(path, 0777);
}

static u32 sFrame;
#ifdef PORT_STACK_POISON
/* Reproduce two-PSP divergence in the emulator: two emulator instances execute
 * identically, so their stack garbage matches and uninitialized locals never
 * differ.  Fill the unused stack below us with an instance-specific byte
 * (data/poison, 1 byte) so any uninitialized local read shows up as a desync,
 * exactly as it would between two real PSPs with different RAM history. */
static int sPoison = -1;
static void poison_stack(void) {
    volatile char probe;
    char* sp = (char*) &probe;
    if (sPoison < 0) {
        FILE* f = fopen(port_save_path("poison"), "rb");
        sPoison = f ? (fgetc(f) & 0xFF) : 0;
        if (f) fclose(f);
        PORT_LOG("stack poison: 0x%02X\n", sPoison);
    }
    if (sPoison) memset(sp - 0xC000, sPoison, 0xC000 - 0x400); /* 48KB below SP, 1KB guard */
}
#endif
static void run_one_iteration(void) {
#ifdef PORT_STACK_POISON
    poison_stack();
#endif
#ifdef PORT_NET
    port_net_modal_update(); /* the drop-out prompts read the local pad directly */
    if (port_net_active() && !port_net_frame_begin()) {
        sceKernelDelayThread(2000); // a peer's input has not arrived: hold this frame
        return;
    }
#endif
    port_debug_frame_begin(sFrame); // no-ops unless a debug build (psp_debug.c)
    port_game_loop_one_iteration();
    port_debug_frame_end(sFrame);
    sFrame++;
#ifdef PORT_NET
    if (port_net_active()) port_net_frame_end();
#endif
}


int main(UNUSED int argc, char** argv) {
    extern void port_assets_load(const char* argv0);
    setup_callbacks();
    scePowerSetClockFrequency(333, 333, 166);
    pspDebugScreenInit();
    port_set_save_dir(argc > 0 ? argv[0] : NULL); // everything lives next to the EBOOT
    port_assets_load(argc > 0 ? argv[0] : NULL); // ROM-derived data lives outside the EBOOT
    // (no boot banner: the debug console is only initialised so the screen is black until the GE draws)
    port_fs_init();
    port_audio_out_init();
    PORT_LOG("boot\n");
#ifdef PORT_ME_AUDIO
    { extern void port_me_load(void); port_me_load(); } /* mk64k.prx boots the Media Engine */
#endif
#ifdef PORT_DEBUG_KNOBS
    { /* a data/showfps file starts with the FPS counter on (hold SELECT 3 s toggles it as always) */
        extern int gPortShowFps;
        FILE* sf = fopen(port_save_path("showfps"), "rb");
        if (sf != NULL) {
            fclose(sf);
            gPortShowFps = 1;
        }
    }
    { /* test knob: a data/cpu222 file runs the CPU at 222 MHz (the PSP-1000's WLAN is unhappy at 333) */
        FILE* f = fopen(port_save_path("cpu222"), "rb");
        if (f != NULL) {
            fclose(f);
            scePowerSetClockFrequency(222, 222, 111);
            PORT_LOG("cpu: 222 MHz (data/cpu222 present)\n");
        } else {
            PORT_LOG("cpu: 333 MHz\n");
        }
    }
#endif

    gfx_init(&gfx_psp, &gfx_opengl_api, "MK64 Portable", false);
    port_debug_selftest(); // PORT_GFX_SELFTEST builds only

    port_game_init();
    PORT_LOG("game init done\n");

    while (sRunning) {
        run_one_iteration();
    }

    sceKernelExitGame();
    return 0;
}
