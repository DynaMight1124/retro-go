"""Half-resolution rectangles retain logical positions and texture steps."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import compile_host, renderer_harness


class RenderResolutionTests(unittest.TestCase):
    def test_half_resolution_rectangle_sampling_and_bounds(self):
        harness = renderer_harness().split('int main(int argc,char **argv)')[0]
        harness = harness.replace('#define MK64_SCREEN_WIDTH 320', '#define MK64_SCREEN_WIDTH 160').replace('#define MK64_SCREEN_HEIGHT 240', '#define MK64_SCREEN_HEIGHT 120')
        harness = harness.replace('#define MK64_RENDER_SCALE 1', '#define MK64_RENDER_SCALE 2')
        harness += r'''
int main(void) {
    sTextureImage=(const uint16_t *)texture;
    sLoadedImage=texture;sLoadedStride=sLoadedWidth=4;sLoadedHeight=2;
    output[160*120]=0x1234;
    texture_rect(0xe4010008,0,0,0x04000400,false);
    assert(output[0]==0xf800 && output[1]==0x001f);
    assert(output[2]==0 && output[160]==0 && sTexturePixels==2);
    memset(output,0,sizeof(output));sTexturePixels=0;output[160*120]=0x1234;
    /* Logical (316,238), width four: last two native pixels. */
    texture_rect(0xe45003c0,(1264u<<12)|952u,0,0x04000400,false);
    assert(output[119*160+158]==0xf800 && output[119*160+159]==0x001f);
    assert(sTexturePixels==2 && output[160*120]==0x1234);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            result = subprocess.run([str(executable)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
