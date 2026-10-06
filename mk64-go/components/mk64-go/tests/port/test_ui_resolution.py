"""Verify mixed-resolution composition preserves world pixels and draw order."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, function, compile_host, renderer_harness


class UiResolutionTests(unittest.TestCase):
    def test_promotion_preserves_world_and_does_not_erase_later_hud(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#define MK64_WORLD_WIDTH 160
#define MK64_WORLD_HEIGHT 120
#define MK64_RENDER_SCALE 2
#define MK64_OUTPUT_WIDTH 320
#define MK64_OUTPUT_HEIGHT 240
typedef struct {void *data;} rg_surface_t;
static uint16_t world[160*120],output[320*240];
static rg_surface_t low={world},high={output};
static rg_surface_t *sSurface=&low,*sOutputSurface=&high;
static bool sHighResolution;
static uint16_t *sRasterPixels,*sRasterDepth,*sUiDepth;
static unsigned sRasterWidth=160,sRasterHeight=120,sRasterScale=2;
""" + function(source, "static void promote_ui_resolution") + """
int main(void) {
    for(unsigned i=0;i<160*120;i++) world[i]=(uint16_t)(i+1);
    promote_ui_resolution();
    assert(sHighResolution && sRasterWidth==320 && sRasterHeight==240 && sRasterScale==1);
    for(unsigned y=0;y<240;y++) for(unsigned x=0;x<320;x++)
        assert(output[y*320+x]==world[(y/2)*160+x/2]);
    output[321]=0xabcd; /* A one-pixel HUD feature must survive repeat markers. */
    promote_ui_resolution();assert(output[321]==0xabcd);
    sHighResolution=false;sOutputSurface=0;sRasterWidth=160;sRasterHeight=120;sRasterScale=2;
    promote_ui_resolution();assert(!sHighResolution && sRasterWidth==160);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)

    def test_native_rectangles_keep_single_pixel_details_and_transparency(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = renderer_harness().split("int main(int argc")[0]
        harness = harness.replace("#define MK64_SCREEN_WIDTH 320", "#define MK64_SCREEN_WIDTH sRasterWidth")
        harness = harness.replace("#define MK64_SCREEN_HEIGHT 240", "#define MK64_SCREEN_HEIGHT sRasterHeight")
        harness = harness.replace("#define sRasterScale MK64_RENDER_SCALE", "static unsigned sRasterScale=2,sRasterWidth=160,sRasterHeight=120;")
        harness = harness.replace("#define sRasterPixels ((uint16_t *)sSurface->data)", "static uint16_t *sRasterPixels,*sRasterDepth,*sUiDepth;")
        harness = harness.replace("#define MK64_RENDER_SCALE 1", "#define MK64_RENDER_SCALE 2")
        harness = harness.replace(
            "static struct { void *data; } surface = {output}, *sSurface = &surface;",
            """typedef struct {void *data;} rg_surface_t;
static uint16_t world[160*120];
static rg_surface_t surface={world},composed={output},*sSurface=&surface,*sOutputSurface=&composed;
static bool sHighResolution;
#define MK64_WORLD_WIDTH 160
#define MK64_WORLD_HEIGHT 120
#define MK64_OUTPUT_WIDTH 320
#define MK64_OUTPUT_HEIGHT 240
""")
        harness += function(source, "static void promote_ui_resolution") + """
int main(void) {
    for(unsigned i=0;i<160*120;i++) world[i]=0x2222;
    promote_ui_resolution();
    sTextureImage=(const uint16_t *)texture;sImageWidth=4;
    sLoadedImage=texture;sLoadedStride=sLoadedWidth=4;sLoadedHeight=2;
    texture[3]=0xc0; /* Transparent green texel over the upscaled world. */
    texture_rect(0xe4010008,0,0,0x04000400,false);
    assert(output[0]==0xf800 && output[1]==0x2222);
    assert(output[2]==0x001f && output[3]==0xffff);
    assert(output[320]==0xffe0 && output[323]==0xf800);
    promote_ui_resolution();assert(output[3]==0xffff);
    assert(world[0]==0x2222 && world[1]==0x2222);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)

    def test_hidden_hud_setup_does_not_emit_native_ui_marker(self):
        source = (ROOT / "src/code_80057C60.c").read_text()
        harness = """
#include <assert.h>
#define RETRO_GO 1
#define SCREEN_MODE_1P 0
#define MK64_RG_UI_TAG 123
static int gDemoMode,gActiveScreenMode,D_800DC5B8,gDisplayListHead,D_0D0076F8,markers,setups;
#define gDPNoOpTag(p,t) (++markers)
#define gSPDisplayList(p,l) (++setups)
""" + function(source, "void func_80058BF4") + """
int main(void) {
    D_800DC5B8=0;func_80058BF4();assert(markers==0 && setups==1);
    D_800DC5B8=1;func_80058BF4();assert(markers==1 && setups==2);
    gDemoMode=1;func_80058BF4();assert(markers==1 && setups==3);
    gDemoMode=0;gActiveScreenMode=1;func_80058BF4();assert(markers==1 && setups==4);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
