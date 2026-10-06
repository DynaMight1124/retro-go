"""Exercise production renderer gating and unchanged audio/VI frame advancement."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class RenderSkipTests(unittest.TestCase):
    def test_audio_and_clocks_advance_when_rendering_is_skipped(self):
        backend=(ROOT / "src/port/rg/port_rg.c").read_text()
        core=(ROOT / "src/main.c").read_text()
        display=function(core,"void display_and_vsync")
        # Compile the portable path; the original N64 thread path follows it.
        display=display[:display.index("    profiler_log_thread5_time(BEFORE_DISPLAY_LISTS)")] + "}"
        harness="""
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#define RG_LOGD(...)
#define RG_LOGI(...)
#define RG_LOGE(...)
#define PORT_LOG(...)
#define MK64_RG_REPORT_INTERVAL 120
#define GFX_POOL_SIZE 8
#define START_MENU_FROM_QUIT 0
#define START_MENU 10
#define V_BlANK_TIMER_ITER 1
typedef int32_t s32;
typedef struct {unsigned dummy;} Gfx;
static struct {Gfx gfxPool[8];} pool,*gGfxPool=&pool;
static Gfx *gDisplayListHead=pool.gfxPool+3;
static uintptr_t gSegmentTable[16];
static unsigned sFrame,sRenderFrames,begin_calls,walk_calls,submit_calls,audio_calls;
static bool sRenderEnabled=true;
static int gfx_trace_frames,gGamestate=4,gMenuSelection=14,gPortHalfFrame;
static int gVBlankTimer,sNumVBlanks,sRenderedFramebuffer,sRenderingFramebuffer,gGlobalTimer;
static void mk64_rg_gfx_begin(void) {begin_calls++;}
static void mk64_rg_gfx_walk(Gfx *p,unsigned n,unsigned f) {assert(n==3);walk_calls++;}
static bool mk64_rg_gfx_present(unsigned frame) {submit_calls++;return true;}
static void port_audio_frame(void) {audio_calls++;}
"""
        for sig in ("void mk64_rg_set_render_enabled", "void port_gfx_start_frame", "void port_gfx_run", "void port_gfx_end_frame"):
            harness+=function(backend,sig)
        harness+=display+"""
int main(void) {
    for(unsigned i=0;i<4;i++) {
        mk64_rg_set_render_enabled((i&1)==0);
        display_and_vsync();
    }
    assert(begin_calls==2 && walk_calls==2 && submit_calls==2);
    assert(audio_calls==4 && gGlobalTimer==4 && gVBlankTimer==8 && sNumVBlanks==8);
    assert(sFrame==4 && sRenderFrames==2 && sRenderedFramebuffer==1 && sRenderingFramebuffer==1);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe=compile_host(harness,Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode,0)


if __name__ == "__main__":
    unittest.main()
