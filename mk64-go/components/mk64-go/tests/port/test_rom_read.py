"""Verify bulk native ROM reads and arbitrary slices of swapped formats."""
from pathlib import Path
import re,subprocess,tempfile,unittest
from test_rg_render import ROOT,compile_host
class RomReadTests(unittest.TestCase):
 def test_byte_orders_and_native_bulk_io(self):
  header=(ROOT/'rom.h').read_text()
  source=(ROOT/'rom.c').read_text().replace('#include "rom.h"','')
  harness=header+'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned read_calls;
static size_t tracked_read(void *p,size_t size,size_t count,FILE *f) {
 ++read_calls;return fread(p,size,count,f);
}
#define fread tracked_read
'''+source+'''
int main(void) {
 static uint8_t native[65536],encoded[65536],output[65538];
 for(unsigned i=0;i<sizeof(native);i++)native[i]=(i*17+i/257)&255;
 unsigned lengths[]={0,1,3,4,511,512,513,8193,32768};
 for(unsigned order=0;order<3;order++) {
  for(unsigned i=0;i<sizeof(native);i++)encoded[i]=native[order==1?(i^1):order==2?(i^3):i];
  FILE *f=tmpfile();assert(f);assert(fwrite(encoded,1,sizeof(encoded),f)==sizeof(encoded));
  mk64_rom_t rom={f,sizeof(encoded),order};
  for(unsigned offset=0;offset<8;offset++)for(unsigned l=0;l<sizeof(lengths)/sizeof(lengths[0]);l++) {
   unsigned n=lengths[l];memset(output,0xA5,sizeof(output));read_calls=0;
   assert(mk64_rom_read(&rom,offset,output+1,n));
   assert(memcmp(output+1,native+offset,n)==0 && output[0]==0xA5 && output[n+1]==0xA5);
   if(order==0)assert(read_calls==(n?1:0));
  }
  assert(mk64_rom_read(&rom,65535,output,1));assert(output[0]==native[65535]);
  assert(!mk64_rom_read(&rom,65535,output,2));
  assert(!mk64_rom_read(&rom,0,NULL,1));
  assert(mk64_rom_read(&rom,65536,NULL,0));
  fclose(f);
 }
 return 0;
}
'''
  with tempfile.TemporaryDirectory() as d:
   exe=compile_host(harness,Path(d));self.assertEqual(subprocess.run([str(exe)]).returncode,0)
