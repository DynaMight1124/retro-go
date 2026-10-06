"""Exercise production submission and reuse with Retro-Go's one-slot queue."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class DisplayBufferTests(unittest.TestCase):
    def test_optional_buffer_reserve_and_actual_placement(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#define MK64_SCREEN_WIDTH 160
#define MK64_WORLD_WIDTH 160
#define MK64_WORLD_HEIGHT 120
#define MK64_SCREEN_HEIGHT 120
#define RG_PIXEL_565_LE 1
#define MEM_FAST 2
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define RG_LOGD(...)
#define RG_LOGI(...)
typedef struct { void *data; } rg_surface_t;
static uint16_t pixels[160*120];
static rg_surface_t front={pixels},back={pixels};
static rg_surface_t *sSurface=&front,*sBackSurface;
static rg_surface_t *sOutputSurface,*sOutputBackSurface;
static bool sBackSurfaceTried,internal=true;
static size_t available,largest;
static unsigned allocations,frees;
static size_t heap_caps_get_free_size(unsigned caps) { return available; }
static size_t heap_caps_get_largest_free_block(unsigned caps) { return largest; }
static bool esp_ptr_internal(const void *p) { return internal; }
static rg_surface_t *rg_surface_create(unsigned w,unsigned h,unsigned format,unsigned flags) {
    assert(w==160 && h==120 && flags==MEM_FAST);++allocations;
    available-=sizeof(pixels)+sizeof(back);return &back;
}
static void rg_surface_free(rg_surface_t *surface) {
    assert(surface==&back);++frees;available+=sizeof(pixels)+sizeof(back);
}
""" + function(source, "static void allocate_back_buffer") + """
int main(void) {
    size_t bytes=sizeof(pixels)+sizeof(back),reserve=128*1024;
    available=bytes+reserve-1;largest=bytes;
    allocate_back_buffer();assert(!sBackSurface && allocations==0);
    sBackSurfaceTried=false;available=bytes+reserve;largest=bytes-1;
    allocate_back_buffer();assert(!sBackSurface && allocations==0);
    sBackSurfaceTried=false;largest=bytes;
    allocate_back_buffer();assert(sBackSurface==&back && allocations==1 && available==reserve);
    allocate_back_buffer();assert(allocations==1);
    sBackSurfaceTried=false;sBackSurface=0;available=bytes+reserve;internal=false;
    allocate_back_buffer();assert(!sBackSurface && allocations==2 && frees==1);
    assert(available==bytes+reserve);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)

    def test_submit_before_swap_and_single_buffer_wait(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#define MK64_RG_VERBOSE_GFX 0
#define sRasterPixels ((uint16_t *)sSurface->data)
#define MK64_SCREEN_WIDTH 160
#define MK64_SCREEN_HEIGHT 120
#define RG_LOGD(...)
#define RG_LOGI(...)
#define PERF_BUCKETS 6
typedef struct { void *data; } rg_surface_t;
static uint16_t pixels[2][160*120];
static rg_surface_t surfaces[2]={{pixels[0]},{pixels[1]}};
static rg_surface_t *sSurface=&surfaces[0], *sBackSurface=&surfaces[1];
static const rg_surface_t *owned,*submitted;
static unsigned polls,yields,submits;
static rg_surface_t *sCompleteSurface;
static rg_surface_t *sOutputSurface,*sOutputBackSurface;
static bool sHighResolution;
static void promote_profiled_ui(void) {}
#define __ATOMIC_ACQUIRE 0
#define __ATOMIC_RELEASE 0
#define __atomic_load_n(p,o) (*(p))
#define __atomic_store_n(p,v,o) (*(p)=(v))
static const rg_surface_t *captured;
static bool rg_surface_save_image_file(const rg_surface_t *s,const char *f,int w,int h) {captured=s;return true;}
static bool sProfileFrame;
static unsigned sPerfUs[PERF_BUCKETS];
static int64_t rg_system_timer(void) { return 0; }
static bool rg_display_is_busy(void) { ++polls;return owned!=0; }
static void rg_task_yield(void) { ++yields;owned=0; }
static void rg_display_submit(const rg_surface_t *surface,unsigned flags) {
    /* A one-slot queue blocks until its previous surface is consumed. */
    assert(surface!=owned);owned=0;owned=surface;submitted=surface;++submits;
}
"""
        harness += function(source, "static void wait_for_color_buffer")
        harness += function(source, "static void swap_color_buffer")
        harness += "static unsigned sCompleteFrame;\n"
        harness += function(source, "bool mk64_rg_gfx_present")
        harness += function(source, "bool mk64_rg_gfx_screenshot")
        harness += function(source, "void mk64_rg_gfx_redraw")
        harness += """
int main(void) {
    assert(!mk64_rg_gfx_screenshot("test.png",0,0));
    for(unsigned frame=1;frame<=20;++frame) {
        wait_for_color_buffer();
        assert(sSurface!=owned); /* Writing this entire frame is safe. */
        rg_surface_t *completed=sSurface;
        assert(mk64_rg_gfx_present(frame));
        assert(submitted==completed && owned==completed);
        assert(sSurface!=completed && sBackSurface==completed);
        assert(mk64_rg_gfx_screenshot("test.png",80,60) && captured==completed);
    }
    assert(submits==20 && polls==0 && yields==0);
    owned=0;mk64_rg_gfx_redraw();assert(submitted==captured);
    /* The display owns the composed pair, never the internal world target. */
    static rg_surface_t composed[2]={{pixels[0]},{pixels[1]}};
    owned=0;sOutputSurface=&composed[0];sOutputBackSurface=&composed[1];
    for(unsigned frame=21;frame<=40;++frame) {
        /* Alternate a menu/HUD frame with a world-only demo frame. */
        sHighResolution = (frame & 1) == 0;
        rg_surface_t *completed=sHighResolution ? sOutputSurface : sSurface;
        wait_for_color_buffer();assert(completed!=owned);
        assert(mk64_rg_gfx_present(frame));
        assert(submitted==completed && owned==completed);
        if (sHighResolution)
            assert(sOutputSurface!=completed && sOutputBackSurface==completed);
        else
            assert(sSurface!=completed && sBackSurface==completed);
        assert(mk64_rg_gfx_screenshot("test.png",80,60) && captured==completed);
    }
    assert(submits==41 && polls==0);
    /* With one world target and a native pair, wait only when the display
     * owns that world target. A previous composed submission is independent. */
    rg_surface_t *spare=sBackSurface;sBackSurface=0;
    sHighResolution=false;
    wait_for_color_buffer();assert(polls==0);
    assert(mk64_rg_gfx_present(41) && submitted==sSurface);
    wait_for_color_buffer();assert(!owned && yields==1 && polls==2);
    sBackSurface=spare;polls=yields=0;
    owned=0;sOutputSurface=sOutputBackSurface=0;sHighResolution=false;
    /* Low-memory fallback retains the wait-before-reuse contract. */
    sBackSurface=0;owned=sSurface;
    wait_for_color_buffer();
    assert(!owned && yields==1 && polls==2);
    rg_surface_t *completed=sSurface;
    assert(mk64_rg_gfx_present(41));
    assert(sSurface==completed && submitted==completed);
    wait_for_color_buffer();assert(!owned && yields==2);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
