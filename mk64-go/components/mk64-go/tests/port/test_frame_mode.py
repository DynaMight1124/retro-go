"""The RG loop must not select PSP-only split-frame execution."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class FrameModeTests(unittest.TestCase):
    def test_retrogo_uses_complete_frames(self):
        source = (ROOT / "src/main.c").read_text()
        harness = """
#include <assert.h>
typedef int s32;
#define RETRO_GO 1
#define RACING 4
#define SCREEN_MODE_1P 0
static int gGamestate=RACING,gActiveScreenMode=SCREEN_MODE_1P;
static int gIsGamePaused,gIsInQuitToMenuTransition,sPortSplitHoldoff;
""" + function(source, "static s32 port_frame_can_split") + """
int main(void) { assert(port_frame_can_split()==0); return 0; }
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
