"""Check the application selects its native 4:3 projection."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host


class ProjectionTests(unittest.TestCase):
    def test_one_player_projection_matches_surface(self):
        options = (ROOT / 'CMakeLists.txt').read_text()
        wide = re.search(r'\bPORT_NO_WIDE_FOV=(\d+)', options)
        source = (ROOT / 'src/racing/skybox_and_splitscreen.c').read_text()
        source = source[source.index('void set_perspective_and_aspect_ratio'):]
        body = source.split('case SCREEN_MODE_1P:', 1)[1].split('case SCREEN_MODE_2P_', 1)[0]
        harness = '#include <assert.h>\n#include <math.h>\n'
        if wide:
            harness += '#define PORT_NO_WIDE_FOV '+wide[1]+'\n'
        harness += 'static float gScreenAspect;\nstatic void select_aspect(void) {'+body+'}\n'
        harness += 'int main(void) {select_aspect();assert(fabsf(gScreenAspect-4.0f/3.0f)<0.00001f);return 0;}\n'
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)
