"""Exercise bounded reads of Lakitu's native animation buffer."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class SpriteBufferTests(unittest.TestCase):
    def test_animation_buffer_and_bounds(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = '''
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#define MK64_ASSET_REGION_SIZE 16
#define PORT_MEMORY_POOL_SIZE 64
static unsigned char gMk64AssetRegion[16],gPortMemoryPool[64],D_802BFB80[16];
static unsigned char D_80183FA8[4][8192],gPlayerPalettesList[2][512];
static unsigned char gGfxPools[2][64];
static unsigned char gTLUTRedShell[512];
static int port_skybox_readable(const void *p,size_t n) { return 0; }
'''
        harness += function(source, "static bool in_byte_span")
        harness += function(source, "static bool readable_data")
        harness += '''
int main(void) {
  assert(readable_data(D_80183FA8[0],56*36));
  assert(readable_data(D_80183FA8[3]+4096,4096));
  assert(!readable_data(D_80183FA8[3]+4096,4097));
  assert(!readable_data((void *)1,16));
  assert(readable_data(gTLUTRedShell,512));
  assert(!readable_data(gTLUTRedShell+1,512));
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)
