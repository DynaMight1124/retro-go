"""Exercise production sprite sampling and triangle coverage on a host."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class SpriteRenderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#define MK64_RG_PIXEL_PROBE 1
#define sRasterPixels ((uint16_t *)sSurface->data)
#define sRasterDepth sDepth
#define sUiDepth sDepth
#define MK64_WORLD_WIDTH MK64_SCREEN_WIDTH
#define MK64_OUTPUT_WIDTH MK64_SCREEN_WIDTH
#define MK64_OUTPUT_HEIGHT MK64_SCREEN_HEIGHT
static bool sHighResolution,sUiDepthCleared;
#define MK64_SCREEN_WIDTH 320
#define MK64_SCREEN_HEIGHT 240
#define MK64_VERTEX_SLOTS 64
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_CULL_BOTH 6
#define G_CULL_BACK 4
#define G_CULL_FRONT 2
#define G_ZBUFFER 1
#define Z_CMP 16
#define Z_UPD 32
#define FORCE_BL 64
#define G_TX_CLAMP 2
#define G_TX_MIRROR 1
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_ENVIRONMENT 5
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_ENVIRONMENT 5
#define RG_LOGD(...)
#define RG_LOGI(...)
static uint16_t pixels[320*240], depths[320*240];
static struct { void *data; } surface={pixels}, *sSurface=&surface;
static uint16_t *sDepth=depths;
static bool sColorTarget=true,sTextureEnabled=true,sTraceGeometry=false;
static bool sTraceAssets=false;
static bool sProfileFrame;
static uint32_t sPixelUs,sBoxPixels,sWrittenPixels,sEarlyDepthRejects;
static uint32_t sSpanPixels;
static unsigned sClipRejects[7],sAlphaRejects,sTextureSampleFailures;
static int64_t rg_system_timer(void) { return 0; }
static unsigned sSpriteTraces;
static unsigned sCombineCalls;
static unsigned sTextureSamples;
static unsigned sFastPixels;
static unsigned sWhitePixels;
static unsigned sLookupPixels;
static unsigned sSkippedAttributeSteps;
static unsigned sUniformShaderReuse;
static unsigned sDecalPixels;
static uint8_t modulation_table[256*(32+64)], *sModulateTable;
static uint32_t texel_cache[4096], *sTextureCache;
static unsigned sTextureCacheCount,sTextureCacheHits,sTextureCacheMisses;
enum { COMBINE_GENERAL, COMBINE_DIRECT, COMBINE_MODULATE, COMBINE_MODULATE_WHITE, COMBINE_MODULATE_DECAL, COMBINE_MODULATE_DECAL_WHITE };
static float sTextureScaleS=1,sTextureScaleT=1;
static unsigned sTextureModeS,sTextureModeT;
static uint32_t sGeometryMode=G_ZBUFFER,sOtherModeL=Z_CMP|Z_UPD;
static uint32_t sPrimitiveColor=0xffffffff,sEnvironmentColor=0xffffffff;
static unsigned sRectColorSource,sRectAlphaSource;
static unsigned sCycleType;
static struct {unsigned rgb[4],alpha[4];} sTriangleCombiner[2];
static unsigned sTriangles,sBadGeometry,sCulledTriangles,sClippedTriangles,sTexturedTriangles;
static unsigned sDepthTriangles,sBlendedTriangles,sVisibleTriangles;
static unsigned sTextureWidth=2,sTextureHeight=2,sTextureFormat=0,sTextureSize=2;
static unsigned sTexturePaletteBank,sPaletteEntries,sPaletteLoadOffset;
static uint16_t sTexturePalette[256];
static uint8_t texels[512],palette[512];
static const uint16_t *sTextureImage=(const uint16_t *)texels;
static bool sLoadedTile;
static const uint8_t *sLoadedImage=texels;
static unsigned sLoadedStride,sLoadedSize,sLoadedNibbleOffset;
typedef struct {float x,y,z,w,u,v;uint8_t red,green,blue,alpha;bool valid;} RGVertex;
static RGVertex sVertices[64];
static bool readable_data(const void *p,size_t n) {
  return ((uintptr_t)p>=(uintptr_t)texels && (uintptr_t)p+n<=(uintptr_t)texels+512) ||
         ((uintptr_t)p>=(uintptr_t)palette && (uintptr_t)p+n<=(uintptr_t)palette+512);
}
'''
        harness += (ROOT / "src/port/rg/gfx_rg_rows.h").read_text()
        harness += r"""
static unsigned sPixelProbeCursor;
static RGPixelProbe sPixelProbe[PIX_STAGE_COUNT];
static unsigned cycle_reads;
static uint32_t esp_cpu_get_cycle_count(void) {
    ++cycle_reads;
    static uint32_t cycles = UINT32_MAX-20; return cycles += 10;
}
"""
        harness += function(source, "static void record_pixel_probe")
        for name in ("static uint16_t rgba5551_to_565", "static uint16_t rgba8888_to_565", "static uint16_t blend_565",
                     "static void load_texture_palette", "static bool sample_triangle_texture_uncached",
                     "static void invalidate_triangle_texture_cache", "static void prepare_triangle_texture_cache",
                     "static bool sample_triangle_texture(",
                     "static void set_triangle_combiner", "static int triangle_rgb_source",
                     "static int triangle_alpha_source", "static uint16_t combine_triangle_pixel",
                     "static unsigned triangle_combiner_fast_mode", "static void build_modulate_table", "static uint16_t combine_triangle_pixel_fast",
                     "static float triangle_edge", "static int triangle_texel_floor", "static unsigned triangle_texture_coordinate",
                     "static bool triangle_row_span", "MK64_RASTER_INLINE unsigned raster_triangle_rows", "static void raster_triangle(",
                     "static float clip_distance", "static RGVertex clip_interpolate",
                     "static void draw_triangle"):
            extracted = function(source, name)
            if name == "static bool sample_triangle_texture(":
                extracted = extracted.replace("sample_triangle_texture(", "reference_sample_triangle_texture(", 1)
                extracted += r'''
static bool sample_triangle_texture(unsigned index, uint16_t *color, unsigned *alpha) {
    ++sTextureSamples;
    return reference_sample_triangle_texture(index,color,alpha);
}
'''
            if name == "static uint16_t combine_triangle_pixel_fast":
                extracted = extracted.replace("combine_triangle_pixel_fast(", "reference_combine_triangle_pixel_fast(", 1)
                extracted += r'''
static uint16_t combine_triangle_pixel_fast(unsigned mode, uint16_t texel, unsigned ta,
    unsigned r, unsigned g, unsigned b, unsigned sa, unsigned *alpha) {
    ++sCombineCalls;
    return reference_combine_triangle_pixel_fast(mode,texel,ta,r,g,b,sa,alpha);
}
'''
            harness += extracted + "\n"
        harness += (ROOT / "tests/port/raster_reference.c").read_text()
        harness += r'''
int main(int argc,char **argv) {
  build_modulate_table(modulation_table);
  sModulateTable=modulation_table;
  unsigned alpha; uint16_t color;
  if(argv[1][0]=='e') {
    const unsigned rgb[]={1,15,4,7};
    const unsigned am[]={7,7,7,1};
    memcpy(sTriangleCombiner[1].rgb,rgb,sizeof(rgb));
    memcpy(sTriangleCombiner[1].alpha,am,sizeof(am));
    assert(triangle_combiner_fast_mode()==COMBINE_MODULATE_DECAL);
    uint32_t random=43;
    for(unsigned table=0;table<2;++table) {
      sModulateTable=table?modulation_table:NULL;
      for(unsigned i=0;i<32768;++i) {
        random=random*1664525u+1013904223u;
        unsigned ea,aa,ta=(random>>16)&255,sa=(random>>12)&255;
        unsigned r=random&511,g=(random>>8)&511,b=(random>>20)&511;
        uint16_t texel=(uint16_t)random;
        uint16_t e=combine_triangle_pixel(texel,ta,r,g,b,sa,&ea);
        uint16_t a=combine_triangle_pixel_fast(COMBINE_MODULATE_DECAL,texel,ta,r,g,b,sa,&aa);
        assert(a==e && aa==ea && aa==ta);
      }
    }
    for(unsigned ta=0;ta<256;++ta)for(unsigned sa=0;sa<256;++sa) {
      unsigned ea,aa;
      uint16_t e=combine_triangle_pixel(0xabcd,ta,255,255,255,sa,&ea);
      uint16_t a=combine_triangle_pixel_fast(COMBINE_MODULATE_DECAL_WHITE,0xabcd,ta,255,255,255,sa,&aa);
      assert(a==e && aa==ea && aa==ta);
    }
    sTriangleCombiner[1].alpha[3]=2;
    assert(triangle_combiner_fast_mode()==COMBINE_MODULATE_DECAL);
    sCycleType=1;assert(triangle_combiner_fast_mode()==COMBINE_GENERAL);
    /* The white raster shortcut must retain texel alpha even when the
       vertex alpha is zero; otherwise sprite artwork disappears. */
    sCycleType=0;sProfileFrame=true;sGeometryMode=0;
    sTextureEnabled=true;texels[0]=0xf8;texels[1]=1;
    for(unsigned i=0;i<3;++i)
      sVertices[i]=(RGVertex){.w=1,.red=255,.green=255,.blue=255,.alpha=0,.valid=true};
    sVertices[0].x=-1;sVertices[0].y=1;
    sVertices[1].x=-0.9f;sVertices[1].y=1;
    sVertices[2].x=-1;sVertices[2].y=0.9f;
    draw_triangle(0,1,2);
    assert(pixels[0]==0xf800 && sDecalPixels>0 && sWhitePixels>0);
    return 0;
  }
  if(argv[1][0]=='j') {
    static uint16_t expected[320*240], expected_depth[320*240];
    sTextureEnabled=false;sGeometryMode=G_ZBUFFER;sOtherModeL=Z_CMP|Z_UPD;
    const unsigned alphas[]={0,127,255};
    for(unsigned cycle=0;cycle<2;++cycle)for(unsigned a=0;a<3;++a) {
      sCycleType=cycle;
      for(unsigned i=0;i<2;++i) {
        unsigned rgb[]={15,15,31,4},am[]={7,7,7,4};
        memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
        memcpy(sTriangleCombiner[i].alpha,am,sizeof(am));
      }
      for(unsigned i=0;i<3;++i) {
        sVertices[i]=(RGVertex){.w=1,.z=0,.red=150,.green=80,.blue=220,
                               .alpha=alphas[a],.valid=true};
      }
      sVertices[0].x=-1;sVertices[0].y=1;
      sVertices[1].x=1;sVertices[1].y=1;
      sVertices[2].x=-1;sVertices[2].y=-1;
      for(unsigned i=0;i<320*240;++i){pixels[i]=(uint16_t)(i*17);depths[i]=0xffff;}
      reference_raster_triangle(sVertices);
      memcpy(expected,pixels,sizeof(pixels));memcpy(expected_depth,depths,sizeof(depths));
      for(unsigned i=0;i<320*240;++i){pixels[i]=(uint16_t)(i*17);depths[i]=0xffff;}
      sCombineCalls=0;raster_triangle(sVertices);
      assert(memcmp(pixels,expected,sizeof(pixels))==0);
      assert(memcmp(depths,expected_depth,sizeof(depths))==0);
      assert(sCombineCalls==1); /* Uniform shader is evaluated once, even for alpha zero. */
      memset(depths,0,sizeof(depths));sCombineCalls=0;
      raster_triangle(sVertices);assert(sCombineCalls==0);
    }
    return 0;
  }
  if(argv[1][0]=='x') {
    sTextureCache=texel_cache;sProfileFrame=true;
    for(unsigned i=0;i<512;++i)texels[i]=(uint8_t)(i*37+11);
    for(unsigned i=0;i<256;++i)sTexturePalette[i]=(uint16_t)(i*97+3);
    sTextureWidth=8;sTextureHeight=4;
    const unsigned formats[]={G_IM_FMT_RGBA,G_IM_FMT_CI,G_IM_FMT_IA,G_IM_FMT_I};
    for(unsigned loaded=0;loaded<2;++loaded) {
      sLoadedTile=loaded;sLoadedImage=texels+7;sLoadedStride=8;
      for(unsigned f=0;f<4;++f)for(unsigned size=0;size<4;++size) {
        sTextureFormat=formats[f];sTextureSize=size;sLoadedSize=size;
        sLoadedNibbleOffset=size==G_IM_SIZ_4b?1:0;
        sTexturePaletteBank=3;sPaletteEntries=83;
        invalidate_triangle_texture_cache();
        prepare_triangle_texture_cache(1);assert(sTextureCacheCount==0);
        prepare_triangle_texture_cache(256);assert(sTextureCacheCount==32);
        for(unsigned pass=0;pass<3;++pass)for(unsigned index=0;index<32;++index) {
          uint16_t actual=0,expected=0;unsigned aa=0,ba=0;
          bool reference=sample_triangle_texture_uncached(index,&expected,&ba);
          bool cached=sample_triangle_texture(index,&actual,&aa);
          assert(cached==reference);
          if(reference)assert(actual==expected && aa==ba);
        }
      }
    }
    assert(sTextureCacheMisses==32*32 && sTextureCacheHits==2*32*32);
    sLoadedTile=true;sLoadedSize=G_IM_SIZ_8b;sTextureSize=G_IM_SIZ_4b;
    sTextureFormat=G_IM_FMT_I;sLoadedNibbleOffset=1;
    invalidate_triangle_texture_cache();prepare_triangle_texture_cache(256);
    for(unsigned index=0;index<32;++index) {
      uint16_t actual,expected;unsigned aa,ba;
      assert(sample_triangle_texture_uncached(index,&expected,&ba));
      assert(sample_triangle_texture(index,&actual,&aa) && actual==expected && aa==ba);
    }
    /* Reloading a mutable source or palette must discard decoded values. */
    sLoadedTile=false;sTextureFormat=G_IM_FMT_CI;sTextureSize=G_IM_SIZ_8b;
    sTexturePaletteBank=0;sPaletteEntries=256;texels[0]=0;sTexturePalette[0]=0xffff;
    invalidate_triangle_texture_cache();prepare_triangle_texture_cache(256);
    assert(sample_triangle_texture(0,&color,&alpha) && color==0xffff && alpha==255);
    texels[0]=1;sTexturePalette[1]=0xf800;
    invalidate_triangle_texture_cache();prepare_triangle_texture_cache(256);
    uint16_t expected;unsigned expected_alpha;
    assert(sample_triangle_texture_uncached(0,&expected,&expected_alpha));
    assert(sample_triangle_texture(0,&color,&alpha) && color==expected && alpha==expected_alpha);
    sTextureWidth=128;sTextureHeight=64;
    invalidate_triangle_texture_cache();prepare_triangle_texture_cache(100000);
    assert(sTextureCacheCount==0);
    return 0;
  }
  if(argv[1][0]=='l') {
    const unsigned rgb[]={1,15,4,7},am[]={1,7,4,7};
    memcpy(sTriangleCombiner[1].rgb,rgb,sizeof(rgb));
    memcpy(sTriangleCombiner[1].alpha,am,sizeof(am));
    for(unsigned channel=0;channel<3;++channel) {
      unsigned maximum=channel==1?63:31;
      unsigned shift=channel==0?11:channel==1?5:0;
      for(unsigned shade=0;shade<256;++shade) {
        for(unsigned tex=0;tex<=maximum;++tex) {
          unsigned r=255,g=255,b=255,aa,ba;
          if(channel==0)r=shade;else if(channel==1)g=shade;else b=shade;
          uint16_t value=(uint16_t)(tex<<shift);
          uint16_t a=combine_triangle_pixel_fast(COMBINE_MODULATE,value,173,r,g,b,119,&aa);
          uint16_t expected=combine_triangle_pixel(value,173,r,g,b,119,&ba);
          assert(a==expected && aa==ba);
          sModulateTable=NULL;
          a=combine_triangle_pixel_fast(COMBINE_MODULATE,value,173,r,g,b,119,&aa);
          assert(a==expected && aa==ba);
          sModulateTable=modulation_table;
        }
      }
    }
    /* Prove that the production shader actually consults the table. */
    modulation_table[31]=1;
    assert(combine_triangle_pixel_fast(COMBINE_MODULATE,0xf800,255,0,0,0,255,&alpha)==0x0800);
    return 0;
  }
  if(argv[1][0]=='w') {
    unsigned rgb[]={1,15,4,7},am[]={1,7,4,7};
    memcpy(sTriangleCombiner[1].rgb,rgb,sizeof(rgb));
    memcpy(sTriangleCombiner[1].alpha,am,sizeof(am));
    /* Every RGB565 value must preserve the general expand/truncate result. */
    for(unsigned texel=0;texel<65536;++texel) {
      unsigned ea,aa;
      uint16_t e=combine_triangle_pixel((uint16_t)texel,173,255,255,255,119,&ea);
      uint16_t a=combine_triangle_pixel_fast(COMBINE_MODULATE_WHITE,(uint16_t)texel,173,255,255,255,119,&aa);
      assert(a==e && aa==ea);
    }
    for(unsigned ta=0;ta<256;++ta)
      for(unsigned sa=0;sa<256;++sa) {
        unsigned ea,aa;
        uint16_t e=combine_triangle_pixel(0x1234,ta,255,255,255,sa,&ea);
        uint16_t a=combine_triangle_pixel_fast(COMBINE_MODULATE_WHITE,0x1234,ta,255,255,255,sa,&aa);
        assert(a==e && aa==ea);
      }
    sProfileFrame=true;sGeometryMode=0;
    for(unsigned i=0;i<3;++i) {
      sVertices[i].w=1;sVertices[i].valid=true;sVertices[i].z=0;
      sVertices[i].red=sVertices[i].green=sVertices[i].blue=255;
      sVertices[i].alpha=255;
    }
    sVertices[0].x=-1;sVertices[0].y=1;
    sVertices[1].x=-0.9f;sVertices[1].y=1;
    sVertices[2].x=-1;sVertices[2].y=0.9f;
    texels[0]=0xf8;texels[1]=1;
    draw_triangle(0,1,2);
    assert(sWhitePixels>0 && pixels[0]==0xf800);
    return 0;
  }
  if(argv[1][0]=='s' || argv[1][0]=='t') {
    /* Independent frozen renderer checks coverage and quantisation while
       work counts prove that thin triangles skip their empty bounding area. */
    static uint16_t expected[320*240], expected_depth[320*240];
    sProfileFrame=true;sGeometryMode=G_ZBUFFER;sOtherModeL=Z_CMP|Z_UPD;
    bool texture_test=argv[1][0]=='t';
    unsigned rgb[]={15,15,31,4}, am[]={7,7,7,4};
    if(texture_test) {
      sTextureCache=texel_cache;
      rgb[0]=1;rgb[2]=4;rgb[3]=7;
      am[0]=1;am[2]=4;am[3]=7;
      /* Distinct texels and an alpha hole reveal stride/UV/depth changes. */
      texels[0]=0xf8;texels[1]=1;texels[2]=0x07;texels[3]=0xc1;
      texels[4]=0;texels[5]=0x3f;texels[6]=0xff;texels[7]=0xfe;
    }
    memcpy(sTriangleCombiner[1].rgb,rgb,sizeof(rgb));
    memcpy(sTriangleCombiner[1].alpha,am,sizeof(am));
    uint32_t random=23;
    for(unsigned test=0;test<240;++test) {
      sTextureEnabled=texture_test;
      sTextureModeS=test%3;sTextureModeT=(test/3)%3;
      for(unsigned i=0;i<3;++i) {
        RGVertex *v=&sVertices[i];v->valid=true;
        random=random*1664525u+1013904223u;
        v->w=0.75f+(random&255)/64.0f;
        v->x=((int)((random>>8)&255)-128)/140.0f*v->w;
        v->y=((int)((random>>16)&255)-128)/140.0f*v->w;
        v->z=((int)((random>>24)&255)-128)/140.0f*v->w;
        v->red=(random>>8)&255;v->green=(random>>16)&255;v->blue=(random>>24)&255;
        v->alpha=255;v->u=v->v=0;
        if(texture_test) {
          v->u=((random>>4)&63)/7.0f-3.137f;
          v->v=((random>>12)&63)/9.0f-2.317f;
          v->alpha=80+((random>>20)&127);
        }
      }
      for(unsigned i=0;i<320*240;++i) {pixels[i]=0x1234;depths[i]=0xffff;}
      invalidate_triangle_texture_cache(); /* Reference samples the original source. */
      reference_raster_triangle(sVertices);
      memcpy(expected,pixels,sizeof(pixels));memcpy(expected_depth,depths,sizeof(depths));
      for(unsigned i=0;i<320*240;++i) {pixels[i]=0x1234;depths[i]=0xffff;}
      raster_triangle(sVertices);
      unsigned texture_boundary_differences=0;
      for(unsigned i=0;i<320*240;++i) {
        if(texture_test && ((depths[i]==0xffff)!=(expected_depth[i]==0xffff) ||
           abs((int)(pixels[i]>>11)-(int)(expected[i]>>11))>1 ||
           abs((int)((pixels[i]>>5)&63)-(int)((expected[i]>>5)&63))>1 ||
           abs((int)(pixels[i]&31)-(int)(expected[i]&31))>1)) {
          ++texture_boundary_differences;continue;
        }
        assert((depths[i]==0xffff)==(expected_depth[i]==0xffff));
        if(depths[i]!=0xffff) {
          assert(abs((int)depths[i]-(int)expected_depth[i])<=2);
          assert(abs((int)(pixels[i]>>11)-(int)(expected[i]>>11))<=1);
          assert(abs((int)((pixels[i]>>5)&63)-(int)((expected[i]>>5)&63))<=1);
          assert(abs((int)(pixels[i]&31)-(int)(expected[i]&31))<=1);
        }
      }
      /* Nearest-neighbour ties can select either texel after float reassociation.
         Bound those differences; coverage/shading tolerances remain strict. */
      assert(texture_boundary_differences<=3);
    }
    if (MK64_RG_PIXEL_PROBE) {
    assert(sPixelProbe[PIX_DEPTH].samples>0 && sPixelProbe[PIX_COORD].samples>0);
    assert(sPixelProbe[PIX_SHADE].samples>0);
    for(unsigned i=0;i<PIX_STAGE_COUNT;++i)
      assert(sPixelProbe[i].cycles==10*sPixelProbe[i].samples);
    if(texture_test) {
      assert(sPixelProbe[PIX_FETCH].samples>0);
    } else assert(sPixelProbe[PIX_FETCH].samples==0);
    } else {
      assert(cycle_reads==0);
      for(unsigned i=0;i<PIX_STAGE_COUNT;++i)
        assert(sPixelProbe[i].samples==0 && sPixelProbe[i].cycles==0);
    }
    if(texture_test)assert(sTextureCacheHits>0 && sTextureCacheMisses>0);
    sSpanPixels=sBoxPixels=sSkippedAttributeSteps=0;
    for(unsigned i=0;i<3;++i) {
      sVertices[i].w=1;sVertices[i].z=0;sVertices[i].alpha=255;
      sVertices[i].red=sVertices[i].green=sVertices[i].blue=255;
    }
    sVertices[0].x=-0.9f;sVertices[0].y=-0.9f;
    sVertices[1].x=0.9f;sVertices[1].y=0.9f;
    sVertices[2].x=0.89f;sVertices[2].y=0.9f;
    raster_triangle(sVertices);
    assert(sSpanPixels>0 && sSpanPixels<sBoxPixels/20);
    assert(sSkippedAttributeSteps==sSpanPixels*(texture_test?3:6));
    return 0;
  }
  if(argv[1][0]=='f') {
    const unsigned rgb[][4]={{15,15,31,1},{15,15,31,3},{15,15,31,4},
                            {15,15,31,5},{1,15,4,7},{6,5,1,3}};
    const unsigned am[][4]={{7,7,7,1},{7,7,7,3},{7,7,7,4},
                           {7,7,7,5},{1,7,4,7},{7,7,7,7}};
    uint32_t random=1;
    for(unsigned cycle=0;cycle<2;++cycle) {
      sCycleType=cycle;
      for(unsigned kind=0;kind<6;++kind) {
        memcpy(sTriangleCombiner[1].rgb,rgb[kind],sizeof(rgb[0]));
        memcpy(sTriangleCombiner[1].alpha,am[kind],sizeof(am[0]));
        unsigned mode=triangle_combiner_fast_mode();
        assert(mode==(cycle || kind==5 ? COMBINE_GENERAL : kind==4 ? COMBINE_MODULATE : COMBINE_DIRECT));
        for(unsigned i=0;i<4096;++i) {
          random=random*1664525u+1013904223u;
          sPrimitiveColor=random;sEnvironmentColor=~random;
          unsigned ta=(random>>16)&255,r=(random>>24)&255,g=(random>>8)&255;
          unsigned b=random&255,sa=(random>>12)&255,expected_alpha,actual_alpha;
          uint16_t texel=(uint16_t)random;
          uint16_t expected=combine_triangle_pixel(texel,ta,r,g,b,sa,&expected_alpha);
          uint16_t actual=combine_triangle_pixel_fast(mode,texel,ta,r,g,b,sa,&actual_alpha);
          assert(actual==expected && actual_alpha==expected_alpha);
        }
      }
    }
    sCycleType=0;
    /* Smoke mixes primitive RGB and texture alpha; exercise every direct
       selector pairing, including ONE, ZERO and the TEXEL1 alias. */
    for(unsigned rgb_d=0;rgb_d<8;++rgb_d)
      for(unsigned alpha_d=0;alpha_d<8;++alpha_d) {
        unsigned direct_rgb[]={15,15,31,rgb_d},direct_alpha[]={7,7,7,alpha_d};
        memcpy(sTriangleCombiner[1].rgb,direct_rgb,sizeof(direct_rgb));
        memcpy(sTriangleCombiner[1].alpha,direct_alpha,sizeof(direct_alpha));
        assert(triangle_combiner_fast_mode()==COMBINE_DIRECT);
        for(unsigned i=0;i<256;++i) {
          random=random*1664525u+1013904223u;
          sPrimitiveColor=random;sEnvironmentColor=~random;
          unsigned ta=random&255,r=(random>>20)&511,g=(random>>8)&511;
          unsigned b=random&511,sa=(random>>12)&511,ea,aa;
          uint16_t e=combine_triangle_pixel((uint16_t)random,ta,r,g,b,sa,&ea);
          uint16_t a=combine_triangle_pixel_fast(COMBINE_DIRECT,(uint16_t)random,ta,r,g,b,sa,&aa);
          assert(a==e && aa==ea);
        }
      }
    return 0;
  }
  if(argv[1][0]=='n') {
    sProfileFrame=true;
    sTextureEnabled=false;sGeometryMode=0;
    for(int i=0;i<2;++i) {
      unsigned rgb[]={15,15,31,4},a[]={7,7,7,4};
      memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
      memcpy(sTriangleCombiner[i].alpha,a,sizeof(a));
    }
    for(int i=0;i<3;++i) {
      sVertices[i].valid=true;sVertices[i].w=1;
      sVertices[i].red=255;sVertices[i].alpha=255;
    }
    sVertices[0].x=-0.8f;sVertices[0].y=-0.8f;
    sVertices[1].x=0.8f;sVertices[1].y=-0.8f;
    sVertices[2].y=0.8f;sVertices[2].z=-2;
    /* Keep the visible bottom of a road triangle crossing the near plane. */
    draw_triangle(0,1,2);
    assert(pixels[200*320+160]==0xf800);
    assert(pixels[50*320+160]==0);
    memset(pixels,0,sizeof(pixels));
    sVertices[2].w=-1;sVertices[2].y=1.6f;
    /* A vertex behind the eye must not discard its visible road section. */
    draw_triangle(0,1,2);
    assert(pixels[200*320+160]==0xf800);
    memset(pixels,0,sizeof(pixels));
    for(int i=0;i<3;++i) sVertices[i].z=-3;
    draw_triangle(0,1,2);
    assert(sVisibleTriangles>0);
    assert(sBoxPixels>=sWrittenPixels && sWrittenPixels>0);
    for(int i=0;i<320*240;++i) assert(pixels[i]==0);
    return 0;
  }
  if(argv[1][0]=='r') {
    /* Glyph LOADTILE: 13 bytes per row, rendered as 27 I4 texels
       including the extra edge column. Row stride remains 26 texels. */
    sTextureFormat=G_IM_FMT_I;sTextureSize=0;sTextureWidth=27;sTextureHeight=17;
    sLoadedTile=true;sLoadedStride=13;sLoadedSize=1;
    for(unsigned row=0;row<16;++row) texels[row*13+1]=0xf0;
    for(unsigned row=0;row<16;++row) {
      assert(sample_triangle_texture(row*27+2,&color,&alpha));
      assert(color==0xffff && alpha==255);
      assert(sample_triangle_texture(row*27+3,&color,&alpha) && alpha==0);
    }
    return 0;
  }
  for(int i=0;i<2;++i) {
    unsigned rgb[]={1,15,4,7},a[]={1,7,6,7};
    memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
    memcpy(sTriangleCombiner[i].alpha,a,sizeof(a));
  }
  if(argv[1][0]=='c') {
    /* Racers: (ONE - ENVIRONMENT) * TEXEL0 + PRIMITIVE.
       Black primitive and black environment must preserve the kart color. */
    uint32_t w0=(6u<<20)|(1u<<15)|(3u<<12)|(1u<<9)|(6u<<5)|1u;
    uint32_t w1=(5u<<28)|(5u<<24)|(3u<<21)|(1u<<18)|(3u<<15)|(7u<<12)|(7u<<9)|(3u<<6)|(7u<<3)|7u;
    set_triangle_combiner(w0,w1);
    sPrimitiveColor=0x000000ff;sEnvironmentColor=0x000000ff;
    color=combine_triangle_pixel(0xf800,255,0,0,0,255,&alpha);
    assert(color==0xf800 && alpha==255);
    sCycleType=1; /* same equation in both cycles, no extra tint */
    assert(combine_triangle_pixel(0xf800,255,0,0,0,255,&alpha)==0xf800);
    /* SHADE ignores stale green primitive and stale red texture. */
    for(int i=0;i<2;++i) {
      unsigned rgb[]={15,15,31,4},a[]={7,7,7,4};
      memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
      memcpy(sTriangleCombiner[i].alpha,a,sizeof(a));
    }
    sPrimitiveColor=0x00ff00ff;
    assert(combine_triangle_pixel(0xf800,255,0,0,255,255,&alpha)==0x001f);
    /* White smoke RGB can still use a zero intensity texel as alpha. */
    sTextureFormat=G_IM_FMT_I;sTextureSize=G_IM_SIZ_8b;texels[0]=0;
    assert(sample_triangle_texture(0,&color,&alpha) && alpha==0);
    return 0;
  }
  if(argv[1][0]=='p') {
    palette[2]=0xf8;palette[3]=1; palette[4]=0;palette[5]=0x3f;
    sTextureImage=(const uint16_t *)palette;
    load_texture_palette(255u<<14);
    assert(sPaletteEntries==256);
    sTextureImage=(const uint16_t *)texels;sTextureFormat=G_IM_FMT_CI;sTextureSize=1;
    texels[0]=1;texels[1]=2;texels[2]=0;
    assert(sample_triangle_texture(0,&color,&alpha) && color==0xf800 && alpha==255);
    assert(sample_triangle_texture(1,&color,&alpha) && color==0x001f && alpha==255);
    assert(sample_triangle_texture(2,&color,&alpha) && alpha==0);
    texels[0]=0x12;sTextureSize=0;
    assert(sample_triangle_texture(1,&color,&alpha) && color==0x001f);
  } else if(argv[1][0]=='i') {
    sTextureFormat=G_IM_FMT_IA;sTextureSize=1;texels[0]=0xf0;texels[1]=0x88;
    assert(sample_triangle_texture(0,&color,&alpha) && alpha==0);
    assert(sample_triangle_texture(1,&color,&alpha) && alpha==136);
    sTextureSize=0;texels[0]=0xf0;
    assert(sample_triangle_texture(0,&color,&alpha) && color==0xffff && alpha==255);
    assert(sample_triangle_texture(1,&color,&alpha) && alpha==0);
  } else {
    for(int i=0;i<3;++i) {
      sVertices[i].w=1;sVertices[i].valid=true;
      sVertices[i].red=sVertices[i].green=sVertices[i].blue=255;
      sVertices[i].alpha=255;
    }
    sVertices[0].x=-1;sVertices[0].y=1;
    sVertices[1].x=-0.9f;sVertices[1].y=1;
    sVertices[2].x=-1;sVertices[2].y=0.9f;
    for(int i=0;i<320*240;++i) { pixels[i]=0x07e0;depths[i]=0xffff; }
    if(argv[1][0]=='u') {
      /* A shaded sky triangle may inherit an enabled texture from objects.
         Its mux ignores that texture: do not fetch it, including validation. */
      sTextureEnabled=true;
      for(int i=0;i<2;++i) {
        unsigned rgb[]={15,15,31,4},a[]={7,7,7,4};
        memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
        memcpy(sTriangleCombiner[i].alpha,a,sizeof(a));
      }
      draw_triangle(0,1,2);
      assert(sTextureSamples==0 && pixels[0]==0xffff);
      /* Smoke needs texture alpha even when RGB comes from primitive. */
      sPrimitiveColor=0xffffffff;
      sTriangleCombiner[1].rgb[3]=3;sTriangleCombiner[1].alpha[3]=1;
      sGeometryMode=0;
      draw_triangle(0,1,2);
      assert(sTextureSamples>0);
    } else if(argv[1][0]=='b') {
      /* Billboard UVs extend beyond the image; clamping must not repeat it. */
      sGeometryMode=0;sTextureModeS=G_TX_CLAMP;
      texels[0]=0xf8;texels[1]=1;texels[2]=0;texels[3]=0x3f;
      for(int i=0;i<3;++i) sVertices[i].u=-1;
      draw_triangle(0,1,2);
      assert(pixels[0]==0xf800);
      for(int i=0;i<3;++i) sVertices[i].u=2;
      draw_triangle(0,1,2);
      assert(pixels[0]==0x001f);
      sTextureModeS=0;
      draw_triangle(0,1,2);
      assert(pixels[0]==0xf800);
      sTextureModeS=G_TX_MIRROR;
      draw_triangle(0,1,2);
      assert(pixels[0]==0x001f);
    } else if(argv[1][0]=='d') {
      texels[0]=0xf8;texels[1]=1;
      for(int i=0;i<320*240;++i) depths[i]=0;
      draw_triangle(0,1,2);
      /* Hidden fragments must skip expensive texture/combiner work. */
      assert(sCombineCalls==0 && pixels[0]==0x07e0 && depths[0]==0);
      for(int i=0;i<320*240;++i) depths[i]=0xffff;
      texels[1]=0;
      draw_triangle(0,1,2);
      assert(sCombineCalls>0 && pixels[0]==0x07e0 && depths[0]==0xffff);
      texels[1]=1;
      draw_triangle(0,1,2);
      assert(pixels[0]==0xf800 && depths[0]<0xffff);
    } else if(argv[1][0]=='a') {
      texels[0]=0xf8;texels[1]=0; /* transparent red RGBA16 */
      draw_triangle(0,1,2);
      assert(pixels[0]==0x07e0 && depths[0]==0xffff);
      texels[1]=1;draw_triangle(0,1,2);
      assert(pixels[0]==0xf800 && depths[0]<0xffff);
    } else {
      sTextureEnabled=false;
      for(int i=0;i<2;++i) {
        unsigned rgb[]={15,15,31,4};memcpy(sTriangleCombiner[i].rgb,rgb,sizeof(rgb));
      }
      sVertices[1].red=0;sVertices[2].red=0;
      draw_triangle(0,1,2);
      assert((pixels[0]>>11)>(pixels[8]>>11));
    }
  }
  return 0;
}
'''
        try:
            cls.exe = compile_host(harness, Path(cls.temp.name))
            lowres = Path(cls.temp.name) / "160x120"
            lowres.mkdir()
            lowres_source = harness.replace("#define MK64_SCREEN_WIDTH 320", "#define MK64_SCREEN_WIDTH 160").replace("#define MK64_SCREEN_HEIGHT 240", "#define MK64_SCREEN_HEIGHT 120")
            cls.lowres_exe = compile_host(lowres_source, lowres)
            quiet = Path(cls.temp.name) / "quiet"
            quiet.mkdir()
            cls.quiet_exe = compile_host(lowres_source.replace("#define MK64_RG_PIXEL_PROBE 1", "#define MK64_RG_PIXEL_PROBE 0"), quiet)
        except subprocess.CalledProcessError as error:
            raise RuntimeError(error.stdout + error.stderr) from error

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def check(self, mode, lowres=False, quiet=False):
        result = subprocess.run([str(self.quiet_exe if quiet else self.lowres_exe if lowres else self.exe), mode], capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_disabled_probes_keep_texture_rendering_and_make_no_clock_reads(self): self.check("textures", quiet=True)
    def test_modulate_decal_keeps_texture_alpha_and_matches_general(self): self.check("edecal")
    def test_uniform_shader_reuse_preserves_alpha_blending_and_depth(self): self.check("jreuse")
    def test_low_resolution_uniform_shader_reuse(self): self.check("jreuse", lowres=True)
    def test_road_crossing_near_plane_keeps_visible_section(self): self.check("near")
    def test_occluded_fragments_skip_shading_but_alpha_holes_keep_depth(self): self.check("depth")
    def test_fast_combiner_matches_general_equations(self): self.check("fast")
    def test_unused_texture_is_skipped_but_smoke_alpha_is_sampled(self): self.check("unused")
    def test_ci_palette_and_transparent_indices(self): self.check("palette")
    def test_intensity_alpha_packed_nibbles(self): self.check("intensity")
    def test_alpha_holes_preserve_color_and_depth(self): self.check("alpha")
    def test_vertex_color_gradient(self): self.check("gradient")
    def test_racer_combiner_sky_shade_and_smoke_mask(self): self.check("combiner")
    def test_packed_glyph_rows_keep_source_stride(self): self.check("rows")
    def test_billboard_edges_clamp_instead_of_repeat(self): self.check("billboard")
    def test_scanlines_match_reference_and_skip_empty_box_area(self): self.check("scanlines")
    def test_incremental_perspective_alpha_and_texture_addressing(self): self.check("textures")
    def test_low_resolution_scanline_coverage(self): self.check("scanlines", lowres=True)
    def test_low_resolution_perspective_alpha(self): self.check("textures", lowres=True)
    def test_white_modulation_matches_all_texels_and_alpha_values(self): self.check("white")
    def test_modulation_lookup_matches_all_channel_and_shade_values(self): self.check("lookup")
    def test_texel_cache_formats_alpha_stride_and_reload(self): self.check("xcache")


if __name__ == "__main__":
    unittest.main()
