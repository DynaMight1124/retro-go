"""Reserve internal headroom and use PSRAM on smaller/fragmented heaps."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class RenderMemoryTests(unittest.TestCase):
    def test_depth_placement_headroom_and_allocation_lifetime(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#define MK64_SCREEN_WIDTH 320
#define MK64_SCREEN_HEIGHT 240
#define MK64_WORLD_WIDTH 160
#define MK64_WORLD_HEIGHT 120
#define RG_PIXEL_565_LE 1
#define MEM_FAST 2
#define MEM_SLOW 4
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define RG_LOGD(...)
#define RG_LOGI(...)
typedef struct { void *data; unsigned width,height,stride,format; } rg_surface_t;
static rg_surface_t color_surface,*sSurface;
static unsigned surface_flags;
static uint16_t *sDepth;
static size_t available,largest;
static unsigned selected,allocations;
static uint16_t buffer[MK64_WORLD_WIDTH*MK64_WORLD_HEIGHT];
static size_t heap_caps_get_free_size(unsigned caps) { return available; }
static size_t heap_caps_get_largest_free_block(unsigned caps) { return largest; }
static int esp_ptr_internal(const void *p) { return selected==MEM_FAST; }
static void *rg_alloc(size_t bytes,unsigned flags) {
    assert(bytes==sizeof(buffer)); ++allocations; selected=flags;
    if(flags==MEM_FAST) { available-=bytes; largest-=bytes; }
    return buffer;
}
static rg_surface_t *rg_surface_create(int width,int height,int format,unsigned flags) {
    ++allocations;surface_flags=flags;color_surface.data=buffer;
    if(flags==MEM_FAST) {
        size_t bytes=sizeof(buffer)+sizeof(rg_surface_t);
        available-=bytes;largest-=bytes;
    }
    return &color_surface;
}
""" + function(source, "static void allocate_color_buffer") + function(source, "static void allocate_depth_buffer") + """
int main(void) {
    const size_t bytes=sizeof(buffer),reserve=128*1024;
    available=bytes+reserve;largest=bytes;
    allocate_depth_buffer();
    assert(selected==MEM_FAST && allocations==1 && sDepth==buffer);
    allocate_depth_buffer(); assert(allocations==1);
    sDepth=0;available=bytes+reserve-1;largest=bytes;
    allocate_depth_buffer();assert(selected==MEM_SLOW);
    sDepth=0;available=bytes+reserve;largest=bytes-1;
    allocate_depth_buffer();assert(selected==MEM_SLOW);
    sDepth=0;available=0;largest=0;
    allocate_depth_buffer();assert(selected==MEM_SLOW);
    /* Give the color framebuffer first claim on a realistic internal heap.
       Depth falls back so combined allocations retain the reserve. */
    sDepth=0;available=bytes*2+reserve-1;largest=bytes*2;allocations=0;
    allocate_color_buffer(); assert(surface_flags==MEM_FAST && sSurface);
    allocate_depth_buffer(); assert(selected==MEM_SLOW && allocations==2);
    assert(available>=reserve);
    allocate_color_buffer();allocate_depth_buffer(); assert(allocations==2);
    sSurface=0;available=128*1024;largest=128*1024;
    allocate_color_buffer();assert(surface_flags==MEM_SLOW);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
