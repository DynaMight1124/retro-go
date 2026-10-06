/**
 * Mario Kart 64 -> PSP port: shared declarations for the port layer.
 *
 * The N64 game addresses much of its data through RSP "segments"
 * (0xSSOOOOOO = segment SS, offset OOOOOO).  On the PSP the same data is
 * linked straight into the executable, so a segmented address is resolved
 * either linearly (segment base + offset, for buffers the game fills at run
 * time: vertices, decompressed textures, the gfx pool) or through a table of
 * {offset, symbol} pairs generated from the decomp's own naming (course
 * display lists, torch assets), see tools/psp/gen_seg_tables.py.
 */
#ifndef PORT_H
#define PORT_H

#include <PR/ultratypes.h>
#include <PR/gbi.h>

typedef struct {
    u32 offset;      // offset inside the segment
    u32 size;        // bytes covered from `offset` (up to the next entry)
    const void* ptr; // where that data lives on the PSP
} PortSegEntry;

typedef struct {
    const PortSegEntry* entries; // sorted by offset
    s32 count;
} PortSegTable;

typedef struct {
    PortSegTable seg6; // course_data.c      (0x06xxxxxx)
    PortSegTable seg7; // course_displaylists (0x07xxxxxx)
} PortCourseSegTables;

extern const PortCourseSegTables gPortCourseSegTables[]; // indexed by course id
extern const PortSegTable gPortCommonDataSegTable;       // segment 0x0D
extern const PortSegTable gPortCeremonyDataSegTable;     // segment 0x0B (ending)
extern const PortSegTable gPortStartupLogoSegTable;      // segment 0x06 (boot logo)

extern uintptr_t gSegmentTable[16];

void port_set_segment_table(s32 segment, const PortSegTable* table);
void port_clear_segment_table(s32 segment);
/** Segmented address (or real pointer) -> real pointer.  NULL if unmapped. */
void* port_seg_to_ptr(uintptr_t addr);
/** Real pointers pass straight through, everything else is treated as segmented. */
static inline int port_is_real_ptr(uintptr_t addr) {
    // PSP user memory starts at 0x08800000; MK64 only forms segments 0..0xF with
    // small offsets, so anything above the segment window is a real pointer.
    return addr >= 0x08800000u || addr < 0x00010000u ? (addr >= 0x08800000u) : 0;
}

/* Memory pool the game allocates course/texture data from (replaces the N64's
 * fixed RAM regions between the code segments). */
#define PORT_MEMORY_POOL_SIZE 0x300000
extern u8 gPortMemoryPool[PORT_MEMORY_POOL_SIZE];

/* Where the port keeps its files (save, log, screenshots). */
/* The port keeps everything it writes (save, cache, log, debug dumps) in a
 * data/ folder next to the EBOOT.  port_eboot_dir() is set from argv[0] at
 * boot; port_save_dir() is its data/ subfolder; port_save_path("x") joins. */
const char* port_eboot_dir(void);
const char* port_save_dir(void);
const char* port_save_path(const char* name);
void port_set_save_dir(const char* argv0);
void port_fs_init(void);

/* Debug builds only (psp_debug.c; no-ops otherwise): dump the displayed frame
 * / z-buffer to data/shotNNN.ppm / depthNNN.ppm, backend self-test at boot,
 * per-frame probes of the scripted run. */
void port_screenshot(int index);
void port_depthshot(int index);
void port_debug_selftest(void);
void port_debug_frame_begin(u32 frame);
void port_debug_frame_end(u32 frame);

/* Logging (goes to stdout / psplink / a file depending on the backend). */
void port_log(const char* fmt, ...);
#define PORT_LOG(...) port_log(__VA_ARGS__)

/* Stadium TV screens (Luigi Raceway, Wario Stadium, the award ceremony): the
 * game copies a patch of the previous frame's framebuffer into RGBA16 course
 * textures.  The port has no CPU-readable framebuffer, so copy_framebuffer()
 * files a request instead and the GE backend fills the tile from its own frame
 * at the end of the frame.  x/y/w/h are in the N64's 320x240 frame; `target`
 * is the tile in the course texture segment, written in N64 texel order. */
void port_fb_copy_request(s32 x, s32 y, s32 w, s32 h, u16* target);

/* 60 fps experiment (branch max_fps_experiments).  MK64 simulates two ticks per
 * displayed frame in 1P.  A "split frame" shows one picture per tick instead:
 * iteration A runs everything up to and including tick 1 and renders, iteration
 * B runs tick 2 and the rest of the frame and renders again.  The simulation
 * executes exactly the same operations in the same order, only with a picture
 * in between, so game speed, physics, ghosts and lockstep are untouched.
 * gPortHalfFrame: 0 = a whole frame (30 fps), 1 = first half, 2 = second half. */
extern s32 gPortHalfFrame;
extern s32 gPortVblanksPerFrame;  /* what end_frame waits for: 2 (30 fps) or 1 (60 fps) */
extern s32 gPortLastFrameVblanks; /* how many the last frame actually took */
extern u32 gPortLastFrameBusyUs;  /* its CPU+GE time, the vblank wait excluded */
/* The draw-distance cull (gfx_pc.c), in clip.w units: PORT_DRAW_DIST at most,
 * pulled in by the 60 fps governor (main.c) while pictures arrive late. */
extern float gPortDrawDist;
extern s32 gPortLogDefer;         /* port_log buffers in RAM (an unpaused race) */
void port_log_flush(void);

/* Frame hooks implemented by the platform backend. */
void port_gfx_run(Gfx* dl);        // execute a display list (F3DEX -> sceGu)
void port_gfx_start_frame(void);
void port_gfx_end_frame(void);     // present + vsync
void port_audio_frame(void);       // mix one game frame worth of audio
void port_input_poll(void);        // fill controller state

/* Display-list markers (gDPNoOpTag) bracketing the race HUD so the renderer
 * can anchor its 2D elements to the screen edges on the wide display. */
#define PORT_HUD_TAG_ON  0x48554431u /* 'HUD1' */
#define PORT_HUD_TAG_OFF 0x48554430u /* 'HUD0' */
#define PORT_HUD_TAG_CENTRE 0x48554432u /* 'HUD2': menus -- uniform scale, centred */

#endif /* PORT_H */
