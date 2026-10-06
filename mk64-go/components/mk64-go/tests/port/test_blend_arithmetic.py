"""Check RGB565 blending against the independent scalar channel formula."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class BlendArithmeticTests(unittest.TestCase):
    def test_all_channel_pairs_and_alpha_values(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <assert.h>
""" + function(source, "static uint16_t blend_565") + r"""
static uint16_t scalar(uint16_t f, uint16_t b, unsigned a) {
    unsigned r=(((f>>11)&31)*a+((b>>11)&31)*(255-a)+127)/255;
    unsigned g=(((f>>5)&63)*a+((b>>5)&63)*(255-a)+127)/255;
    unsigned z=((f&31)*a+(b&31)*(255-a)+127)/255;
    return (uint16_t)(r<<11|g<<5|z);
}
int main(void) {
    /* Cover every 5/6-bit channel pair at every alpha; odd blue permutations
       also vary low-lane products independently of the upper red lane. */
    for(unsigned f=0;f<64;++f)for(unsigned b=0;b<64;++b)
      for(unsigned a=0;a<256;++a) {
        uint16_t fg=(uint16_t)((f&31)<<11|f<<5|((f*7)&31));
        uint16_t bg=(uint16_t)((b&31)<<11|b<<5|((b*19)&31));
        assert(blend_565(fg,bg,a)==scalar(fg,bg,a));
      }
    /* Full-word combinations exercise lane isolation beyond correlated pairs. */
    uint32_t random=27;
    for(unsigned i=0;i<250000;++i) {
        random=random*1664525u+1013904223u;uint16_t f=(uint16_t)random;
        random=random*1664525u+1013904223u;uint16_t b=(uint16_t)random;
        unsigned a=(random>>24)&255;
        assert(blend_565(f,b,a)==scalar(f,b,a));
    }
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
