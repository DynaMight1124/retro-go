"""Check repeated valid decodes and reject an impossible TKMK00 tree."""
from pathlib import Path
import subprocess
import tempfile
import unittest

from test_rg_render import ROOT, compile_host


class TkmkDecodeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        source = '#include "' + (ROOT / "tools/libtkmk00.c").as_posix() + '"\n'
        source += r'''
#include <assert.h>
static void tree_bit(uint8_t *p,unsigned *bit,unsigned value) {
  p[*bit/8] |= value << (7-*bit%8); ++*bit;
}
static void full_tree(uint8_t *p,unsigned *bit,unsigned depth,unsigned value) {
  tree_bit(p,bit,depth!=0);
  if (depth) {
    full_tree(p,bit,depth-1,value*2);
    full_tree(p,bit,depth-1,value*2+1);
  } else {
    for (int i=4;i>=0;--i) tree_bit(p,bit,(value>>i)&1);
  }
}
int main(int argc,char **argv) {
  uint8_t compressed[4096]={0},scratch[1]={0},output[2]={0};
  memcpy(compressed,"TKMK00",6);
  compressed[9]=compressed[11]=1;
  for (unsigned i=0;i<8;++i) compressed[0x0c+i*4+3]=0x40;
  if (argv[1][0]=='t') {
    unsigned bit=0;
    full_tree(compressed+0x40,&bit,5,0);
    /* Keep the pixel-repeat streams separate from the color tree. */
    for (unsigned i=1;i<8;++i) compressed[0x0c+i*4+3]=0x80;
  }
  if (argv[1][0]=='b') memset(compressed+0x40,0xff,sizeof(compressed)-0x40);
  for (unsigned i=0;i<3;++i) {
    int status=tkmk00_decode(compressed,scratch,output,1);
    assert(status == (argv[1][0]=='b' ? -1 : 0));
    assert(output[0]==0 && output[1]==0);
  }
  return 0;
}
'''
        cls.executable = compile_host(source, Path(cls.temp.name))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def check(self, mode):
        result = subprocess.run([str(self.executable), mode], capture_output=True,
                                text=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_repeat_valid_decode(self):
        self.check("valid")

    def test_invalid_tree_does_not_overflow_arrays_or_stack(self):
        self.check("bad")

    def test_all_32_colors_fit_the_valid_tree_limit(self):
        self.check("tree")


if __name__ == "__main__":
    unittest.main()
