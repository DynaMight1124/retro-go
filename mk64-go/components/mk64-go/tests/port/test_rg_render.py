"""Exercise production menu raster code on a Windows host (MSVC required)."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
VC = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207")
SDK = Path("C:/Program Files (x86)/Windows Kits/10")
SDK_VERSION = "10.0.26100.0"


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def branch(source, condition):
    return function(source, "if (" + condition + ")")


def compile_host(source, directory):
    cfile = directory / "test.c"
    executable = directory / "test.exe"
    cfile.write_text(source)
    command = [str(VC / "bin/Hostx64/x64/cl.exe"), "/nologo", "/O2", "/MD",
               str(cfile), "/Fo" + str(directory / "test.obj"), "/Fe" + str(executable),
               "/I" + str(VC / "include")]
    command += ["/I" + str(SDK / "Include" / SDK_VERSION / name)
                for name in ("ucrt", "shared", "um")]
    command += ["/link", "/LIBPATH:" + str(VC / "lib/x64")]
    command += ["/LIBPATH:" + str(SDK / "Lib" / SDK_VERSION / name / "x64")
                for name in ("ucrt", "um")]
    subprocess.run(command, check=True, capture_output=True, text=True)
    return executable


def renderer_harness():
    source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
    prefix = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#define MK64_SCREEN_WIDTH 320
#define MK64_SCREEN_HEIGHT 240
#define MK64_RENDER_SCALE 1
#define sRasterScale MK64_RENDER_SCALE
#define sRasterPixels ((uint16_t *)sSurface->data)
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_ENVIRONMENT 5
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_ENVIRONMENT 5
#define G_CYC_COPY 2
#define G_MDSFT_CYCLETYPE 0
#define G_TEXRECT 0xe4
#define G_TEXRECTFLIP 0xe5
#define G_LOADTILE 0xf4
#define G_RDPHALF_1 0xb4
#define G_RDPHALF_2 0xb3
#define G_RDPHALF_CONT 0xb2
#define F3D_OLD 1
typedef struct { struct { uint32_t w0,w1; } words; } Gfx;
static uint16_t output[320*240];
static struct { void *data; } surface = {output}, *sSurface = &surface;
static bool sColorTarget=true;
static unsigned sCycleType=0, sTextureFormat=0,sTextureSize=2;
static uint32_t sPrimitiveColor=0xffffffff;
static uint32_t sEnvironmentColor=0xffffffff;
static unsigned sRectColorSource,sRectAlphaSource;
static uint16_t sTexturePalette[256];
static unsigned sTexturePaletteBank,sPaletteEntries,sLoadedNibbleOffset;
static unsigned sLoadedSize;
static bool sLoadedTile;
static unsigned sTextureRects,sSkippedTextureRects,sTexturePixels;
static const uint16_t *sTextureImage;
static unsigned sImageWidth;
static const uint8_t *sLoadedImage;
static unsigned sLoadedStride,sLoadedWidth,sLoadedHeight,sLoadedOriginS,sLoadedOriginT;
static unsigned sCommands,sOpcodeCounts[256];
static unsigned sTextureCacheCount;
static uint8_t texture[4*2*2] = {
  0xf8,1, 7,0xc1, 0,0x3f, 0xff,0xff,
  0xff,0xc1, 0,0x3f, 7,0xc1, 0xf8,1
};
static bool readable_data(const void *p,size_t n) {
  return (uintptr_t)p >= (uintptr_t)texture && n<=sizeof(texture) &&
    (uintptr_t)p-(uintptr_t)texture <= sizeof(texture)-n;
}
'''
    functions = "\n".join(function(source, name) for name in (
        "static void invalidate_triangle_texture_cache",
        "static uint16_t blend_565", "static uint16_t rectangle_rgba5551_to_565", "static void texture_rect"))
    dispatch = "static void rectangle_command(Gfx *command, Gfx *end) {\n"
    dispatch += "uint32_t w0=command->words.w0,w1=command->words.w1; unsigned opcode=w0>>24;\n"
    dispatch += branch(source, "opcode == G_TEXRECT || opcode == G_TEXRECTFLIP") + "\n}\n"
    load = "static void load(uint32_t w0,uint32_t w1) { unsigned opcode=G_LOADTILE;\n"
    load += branch(source, "opcode == G_LOADTILE") + "\n}\n"
    main = r'''
int main(int argc,char **argv) {
  sTextureImage=(const uint16_t *)texture; sImageWidth=4;
  sLoadedImage=texture; sLoadedStride=4; sLoadedWidth=4; sLoadedHeight=2;
  if (argv[1][0]=='c') {
    Gfx cmds[3]={{{0xe4010008,0}},{{0xb3000000,0}},{{0xb2000000,0x04000400}}};
    rectangle_command(cmds,cmds+3);
    assert(sTextureRects==1 && sSkippedTextureRects==0);
    assert(output[0]==0xf800 && output[1]==0x07e0 && output[2]==0x001f);
  } else if (argv[1][0]=='s') {
    texture_rect(0xe4010008,0,0,0x04000400,false);
    assert(output[0]==0xf800 && output[1]==0x07e0 && output[2]==0x001f);
    assert(output[320]==0xffe0 && output[323]==0xf800);
    assert(sTexturePixels==8);
  } else if (argv[1][0]=='d') {
    sPrimitiveColor=0xff0000ff;
    texture_rect(0xe4010008,0,0,0x04000400,false);
    assert(output[1]==0x07e0 && output[2]==0x001f);
  } else if (argv[1][0]=='n') {
    sTextureFormat=G_IM_FMT_I;sTextureSize=G_IM_SIZ_4b;
    sLoadedStride=sLoadedWidth=4;sLoadedHeight=1;
    texture[0]=0xf0;texture[1]=0x8f;
    texture_rect(0xe4010004,0,0,0x04000400,false);
    assert(output[0]==0xffff && output[1]==0 && output[3]==0xffff);
    assert(sSkippedTextureRects==0 && sTexturePixels==3);
    /* An odd tile origin must start at the low nibble, not the next byte. */
    sImageWidth=4;load(0xf4004000,0x0700c000);
    assert(sLoadedImage==texture && sLoadedNibbleOffset==1);
  } else if (argv[1][0]=='p') {
    sTextureFormat=G_IM_FMT_CI;sTextureSize=G_IM_SIZ_8b;
    sLoadedStride=sLoadedWidth=4;sLoadedHeight=1;
    sPaletteEntries=3;sTexturePalette[1]=0xf801;sTexturePalette[2]=0x003f;
    texture[0]=1;texture[1]=0;texture[2]=2;texture[3]=1;
    texture_rect(0xe4010004,0,0,0x04000400,false);
    assert(output[0]==0xf800 && output[1]==0 && output[2]==0x001f);
    assert(sSkippedTextureRects==0 && sTexturePixels==3);
  } else if (argv[1][0]=='r') {
    const uint8_t rgba[16]={255,0,0,255, 0,0,255,0,
                          0,255,0,128, 255,255,255,255};
    memcpy(texture,rgba,sizeof(rgba));
    sTextureSize=3; sImageWidth=2;
    load(0xf4000000,0x07004004);
    texture_rect(0xe4008008,0,0,0x04000400,false);
    assert(output[0]==0xf800 && output[1]==0);
    assert(output[320]==0x0400 && output[321]==0xffff);
    assert(sSkippedTextureRects==0 && sTexturePixels==3);
    /* Loading the second row must advance by four bytes per pixel. */
    load(0xf4000004,0x07004004);
    assert(sLoadedImage==texture+8);
    texture_rect(0xe4008004,0,32,0x04000400,false);
    assert(output[1]==0xffff);
  } else {
    /* Menu code passes the right edge (width), so constrain the extra texel
       requested by LOADTILE to the actual image row. */
    load(0xf4000000,0x07010004);
    assert(sLoadedImage==texture && sLoadedWidth==4 && sLoadedStride==4);
    texture_rect(0xe4010004,0,0,0x04000400,false);
    assert(output[3]==0xffff && sTexturePixels==4);
  }
  return 0;
}
'''
    return prefix + functions + dispatch + load + main


class MenuRasterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.executable = compile_host(renderer_harness(), Path(cls.temp.name))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def check(self, mode):
        result = subprocess.run([str(self.executable), mode], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_old_f3d_rectangle_sequence(self):
        self.check("command")

    def test_unit_texture_steps_and_row_stride(self):
        self.check("steps")

    def test_load_tile_at_image_right_edge(self):
        self.check("edge")

    def test_decal_texture_ignores_previous_primitive_color(self):
        self.check("decal")

    def test_rgba32_logo_colors_alpha_and_source_stride(self):
        self.check("rgba32")

    def test_packed_intensity_minimap_and_odd_tile_origin(self):
        self.check("nibbles")

    def test_palette_hud_rectangles(self):
        self.check("palette")

    def test_direct_16bit_conversion_matches_rectangle_shader_for_every_texel(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <assert.h>
""" + function(source, "static uint16_t rectangle_rgba5551_to_565") + """
int main(void) {
    for(unsigned texel=0;texel<65536;texel++) {
        unsigned r=((texel>>11)&31)*255/31;
        unsigned g=((texel>>6)&31)*255/31;
        unsigned b=((texel>>1)&31)*255/31;
        unsigned expected=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
        assert(rectangle_rgba5551_to_565((uint16_t)texel)==expected);
    }
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
