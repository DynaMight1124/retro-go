"""Regression coverage for unsafe legacy decomp constructs."""
from pathlib import Path
import subprocess, tempfile, unittest
from test_rg_render import ROOT, compile_host, function

class LegacySafetyTests(unittest.TestCase):
 def run_c(self, code):
  with tempfile.TemporaryDirectory() as d:
   exe=compile_host(code,Path(d))
   self.assertEqual(subprocess.run([str(exe)]).returncode,0)

 def test_vector_helpers_return_callers_storage(self):
  source=(ROOT/'src/math_util_2.c').read_text()
  code="#include <math.h>\n#include <assert.h>\ntypedef float f32; typedef f32 Vec3f[3];\n"
  for name in ['vec3f_set_xyz','vec3f_normalize','vec3f_cross_product']:
   code+=function(source,'Vec3f* '+name)+'\n'
  self.run_c(code+"""
int main(void) {
 Vec3f a,b,c;
 assert(vec3f_set_xyz(a,3,0,0)==(Vec3f*)a);
 assert(vec3f_normalize(a)==(Vec3f*)a && a[0]==1);
 vec3f_set_xyz(b,0,1,0);
 assert(vec3f_cross_product(c,a,b)==(Vec3f*)c && c[2]==1);
 return 0;
}
""")

 def test_audio_filter_initializes_entire_recurrence(self):
  source=(ROOT/'src/audio/heap.c').read_text()
  code="#include <stdint.h>\n#include <assert.h>\ntypedef int s32;typedef float f32;typedef uint16_t u16;\n"
  code+=function(source,'void func_800B9BE4')
  self.run_c(code+"""
int main(void) {
 u16 out[16];float expected[16];float a=.25f,b=.125f;
 expected[0]=b*262159.0f;expected[8]=a*262159.0f;
 expected[1]=b*a*262159.0f;expected[9]=(a*a+b)*262159.0f;
 for(int i=2;i<8;i++){
  expected[i]=b*expected[i-2]+a*expected[i-1];
  expected[8+i]=b*expected[6+i]+a*expected[7+i];
 }
 func_800B9BE4(a,b,out);
 for(int i=0;i<16;i++)assert(out[i]==(u16)expected[i]);
 return 0;
}
""")

 def test_framebuffer_clear_respects_small_port_backing(self):
  source=(ROOT/'src/main.c').read_text()
  body=function(source,'void thread3_video')
  clear=body[body.index('// Clear framebuffer.'):body.index('    setup_mesg_queues();')]
  self.run_c("""
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define PORT_N64_FB_TEXELS 16
static uint16_t backing[80000];
#define gFramebuffer1 (backing+16)
int main(void) {
 int i;uint64_t *framebuffer1;
 memset(backing,0xA5,sizeof(backing));
"""+clear+"""
 for(i=0;i<80000;i++)assert(backing[i]==(i>=16 && i<32 ? 0 : 0xA5A5));
 return 0;
}
""")

 def test_copy_and_set_helpers_return_destination(self):
  source=(ROOT/'src/racing/math_util.c').read_text()
  code="#include <assert.h>\ntypedef float f32;typedef f32 Vec3f[3];\n#define UNUSED\n"
  code+=function(source,'void* vec3f_copy_return')
  code+=function(source,'UNUSED void* vec3f_set_return')
  self.run_c(code+"""
int main(void) {
 Vec3f a={1,2,3},b;
 assert(vec3f_copy_return(b,a)==b && b[2]==3);
 assert(vec3f_set_return(b,4,5,6)==b && b[1]==5);
 return 0;
}
""")
