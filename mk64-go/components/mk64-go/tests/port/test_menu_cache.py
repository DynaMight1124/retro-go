"""Exercise decoded-background cache hits, keys, ownership and allocation failure."""
from pathlib import Path
import re,subprocess,tempfile,unittest
from test_rg_render import ROOT,compile_host,function
class MenuCacheTests(unittest.TestCase):
 def test_background_cache(self):
  source=(ROOT/'src/port/rg/menu_cache.c').read_text()
  source=re.sub(r'^#include[^\n]*','',source,flags=re.M)
  harness='''
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define MEM_SLOW 1
#define MEM_NOPANIC 64
static bool fail_alloc=true;
static void *rg_alloc(unsigned bytes,unsigned flags) {
 assert(bytes==153600 && flags==(MEM_SLOW|MEM_NOPANIC));
 return fail_alloc?NULL:malloc(bytes);
}
'''+source+'''
int main(void) {
 static uint8_t input[153600],output[153600];
 memset(input,0xA5,sizeof(input));
 assert(!mk64_menu_cache_restore(123,320,240,1,output));
 mk64_menu_cache_store(123,320,240,1,input);
 assert(!mk64_menu_cache_restore(123,320,240,1,output));
 fail_alloc=false;
 mk64_menu_cache_store(123,320,240,1,input);
 memset(input,0x33,sizeof(input));
 assert(mk64_menu_cache_restore(123,320,240,1,output));
 for(unsigned i=0;i<sizeof(output);i++)assert(output[i]==0xA5);
 assert(!mk64_menu_cache_restore(124,320,240,1,output));
 assert(!mk64_menu_cache_restore(123,320,240,0xBE,output));
 assert(!mk64_menu_cache_restore(123,160,120,1,output));
 mk64_menu_cache_store(124,320,240,0xBE,input);
 assert(!mk64_menu_cache_restore(123,320,240,1,output));
 assert(mk64_menu_cache_restore(124,320,240,0xBE,output));
 assert(output[0]==0x33 && output[153599]==0x33);
 return 0;
}
'''
  with tempfile.TemporaryDirectory() as d:
   exe=compile_host(harness,Path(d));self.assertEqual(subprocess.run([str(exe)]).returncode,0)

 def test_loader_reuses_background_and_preserves_map(self):
  cache=re.sub(r'^#include[^\n]*','',(ROOT/'src/port/rg/menu_cache.c').read_text(),flags=re.M)
  loader=function((ROOT/'src/menu_items.c').read_text(),'void load_menu_img_comp_type(')
  harness=r"""
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define RETRO_GO 1
#define MEM_SLOW 1
#define MEM_NOPANIC 64
#define LOAD_MENU_IMG_MIO0_ONCE -1
#define LOAD_MENU_IMG_TKMK00_ONCE 0
#define LOAD_MENU_IMG_FORCE 0
#define LOAD_MENU_IMG_MIO0_FORCE 1
#define LOAD_MENU_IMG_TKMK00_FORCE 2
typedef int s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef struct {void *textureData;unsigned size,width,height,type;} MenuTexture;
typedef struct {void *textureData;unsigned offset;} TextureMap;
static TextureMap sMenuTextureMap[16];
static unsigned sMenuTextureEntries,sMenuTextureBufferIndex,reads,decodes;
static u16 gMenuTextureBuffer[100000];
static u8 gMenuCompressedBuffer[8],sTKMK00_LowResBuffer[8];
static void *segmented_to_virtual_dupe(void *p) {return p;}
static void *rg_alloc(unsigned n,unsigned flags) {return malloc(n);}
static const char *port_decoder_error(void) {return NULL;}
static void dma_tkmk00_textures(void*p,unsigned n,void*d) {reads++;}
static void dma_copy_mio0_segment(void*p,unsigned n,void*d) {reads++;}
static void mio0decode(void*p,void*d) {decodes++;}
static void tkmk00decode(void*p,void*t,void*d,unsigned alpha) {decodes++;memset(d,0xA5,153600);}
"""+cache+loader+r"""
int main(void) {
 MenuTexture images[]={{(void*)123,52576,320,240,0},{0}};
 for(unsigned menu=0;menu<2;menu++) {
  sMenuTextureEntries=sMenuTextureBufferIndex=0;
  memset(gMenuTextureBuffer,0,sizeof(gMenuTextureBuffer));
  load_menu_img_comp_type(images,LOAD_MENU_IMG_TKMK00_ONCE);
  assert(reads==1 && decodes==1);
  assert(sMenuTextureEntries==1 && sMenuTextureBufferIndex==76808);
  assert(sMenuTextureMap[0].textureData==(void*)123 && sMenuTextureMap[0].offset==0);
  assert(gMenuTextureBuffer[0]==0xA5A5 && gMenuTextureBuffer[76799]==0xA5A5);
 }
 sMenuTextureEntries=sMenuTextureBufferIndex=0;
 load_menu_img_comp_type(images,LOAD_MENU_IMG_TKMK00_FORCE);
 assert(reads==2 && decodes==2); /* Forced loads bypass the cache. */
 return 0;
}
"""
  with tempfile.TemporaryDirectory() as d:
   exe=compile_host(harness,Path(d));self.assertEqual(subprocess.run([str(exe)]).returncode,0)
