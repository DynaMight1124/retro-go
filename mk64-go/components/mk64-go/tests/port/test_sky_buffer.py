"""Check bounds of the native sky vertex buffers accepted by the renderer."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class SkyBufferTests(unittest.TestCase):
    def test_native_vertices_and_bounds(self):
        source = (ROOT / "src/racing/skybox_and_splitscreen.c").read_text()
        helper = function(source, "int port_skybox_readable")
        harness = '''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
static unsigned char sSkyboxP1[128],sSkyboxP2[128],sSkyboxP3[128],sSkyboxP4[128];
'''+helper+'''
int main(void) {
  assert(port_skybox_readable(sSkyboxP1,64));
  assert(port_skybox_readable(sSkyboxP1+64,64));
  assert(port_skybox_readable(sSkyboxP4,128));
  assert(!port_skybox_readable(sSkyboxP1+64,65));
  assert(!port_skybox_readable(sSkyboxP2,129));
  assert(!port_skybox_readable((void *)1,16));
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)
