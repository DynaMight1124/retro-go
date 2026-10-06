"""Exercise TV capture mapping, N64 byte order and skipped-frame reuse."""
from pathlib import Path
import subprocess,tempfile,unittest
from test_rg_render import ROOT,compile_host,function
class FramebufferCaptureTests(unittest.TestCase):
 def test_previous_surface_capture(self):
  source=(ROOT/'src/port/rg/gfx_rg.c').read_text()
  capture=function(source,'void mk64_rg_gfx_capture(')
  harness='''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#define __atomic_load_n(p,o) (*(p))
#define PORT_MEMORY_POOL_SIZE 0x300000
#define MK64_CAPTURE_SLOTS 8
static uint8_t gPortMemoryPool[PORT_MEMORY_POOL_SIZE];
typedef struct {int width,height,stride;void *data;} rg_surface_t;
typedef struct {void *target;unsigned frame;int x,y,width,height;bool valid;} RGCapture;
static RGCapture sCaptures[8];
static unsigned sCaptureSlot,sCompleteFrame;
typedef struct {unsigned copies,us,max_us;} Mk64CaptureStats;
static Mk64CaptureStats sCaptureStats;
static int64_t rg_system_timer(void) {static int64_t t;return ++t;}
static rg_surface_t *sCompleteSurface;
static uint16_t pixels[164*120];
static rg_surface_t low={160,120,328,pixels};
'''+capture+function(source,'Mk64CaptureStats mk64_rg_gfx_capture_stats(')+'''
int main(void) {
 uint8_t *a=gPortMemoryPool+16,*b=a+4096;
 memset(a,0xA5,8192);
 mk64_rg_gfx_capture(128,0,64,32,a);assert(a[0]==0xA5); /* No image yet. */
 for(unsigned y=0;y<120;y++)for(unsigned x=0;x<160;x++)pixels[y*164+x]=0xF800;
 pixels[64]=0x07E0;pixels[65]=0x001F;
 sCompleteSurface=&low;sCompleteFrame=1;
 mk64_rg_gfx_capture(128,0,64,32,a);
 assert(a[0]==0x07 && a[1]==0xC1 && a[2]==0x07 && a[3]==0xC1);
 assert(a[4]==0 && a[5]==0x3F); /* RGB565 blue -> big-endian RGBA5551. */
 assert(a[-1]==0 && a[4096]==0xA5);
 pixels[64]=0xFFFF;
 mk64_rg_gfx_capture(128,0,64,32,a);assert(a[0]==0x07); /* Same image/target reused. */
 mk64_rg_gfx_capture(128,0,64,32,b);assert(b[0]==0xFF && b[1]==0xFF);
 sCompleteFrame=2;mk64_rg_gfx_capture(128,0,64,32,a);assert(a[0]==0xFF);
 memset(a,0xA5,4096);
 mk64_rg_gfx_capture(-1,0,64,32,a);assert(a[0]==0xA5);
 mk64_rg_gfx_capture(300,0,64,32,a);assert(a[0]==0xA5);
 mk64_rg_gfx_capture(0,0,65,32,a);assert(a[0]==0xA5);
 mk64_rg_gfx_capture(0,0,64,32,gPortMemoryPool+PORT_MEMORY_POOL_SIZE-1);
 static uint16_t highpixels[320*240];
 rg_surface_t high={320,240,640,highpixels};
 highpixels[128]=0xF800;sCompleteSurface=&high;sCompleteFrame=3;
 mk64_rg_gfx_capture(128,0,64,32,a);assert(a[0]==0xF8 && a[1]==1);
 Mk64CaptureStats stats=mk64_rg_gfx_capture_stats();
 assert(stats.copies==4 && stats.us==4 && stats.max_us==1);
 assert(mk64_rg_gfx_capture_stats().copies==0);
 return 0;
}
'''
  with tempfile.TemporaryDirectory() as d:
   exe=compile_host(harness,Path(d));self.assertEqual(subprocess.run([str(exe)]).returncode,0)
