"""Timing buckets must not double count recursive display lists."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class GfxPerfTests(unittest.TestCase):
    def test_nested_lists_are_excluded_from_timing_buckets(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        names = ("G_TRI1", "G_TRI2", "G_QUAD", "G_TEXRECT", "G_TEXRECTFLIP",
                 "G_VTX", "G_MTX", "G_POPMTX", "G_LOADTLUT", "G_LOADTILE",
                 "G_LOADBLOCK", "G_FILLRECT", "G_DL", "G_ENDDL")
        harness = "#include <stdint.h>\n#include <assert.h>\n"
        harness += "\n".join(f"#define {name} {i}" for i, name in enumerate(names))
        start = source.index("enum { PERF_TRIANGLE")
        harness += "\n" + source[start:source.index(";", start)+1] + "\n"
        harness += function(source, "static int profile_bucket")
        harness += """
int main(void) {
    assert(profile_bucket(G_DL)==-1);
    assert(profile_bucket(G_ENDDL)==-1);
    assert(profile_bucket(G_TRI1)==PERF_TRIANGLE);
    assert(profile_bucket(G_TRI2)==PERF_TRIANGLE);
    assert(profile_bucket(G_QUAD)==PERF_TRIANGLE);
    assert(profile_bucket(G_TEXRECT)==PERF_RECTANGLE);
    assert(profile_bucket(G_TEXRECTFLIP)==PERF_RECTANGLE);
    assert(profile_bucket(G_VTX)==PERF_VERTEX);
    assert(profile_bucket(G_MTX)==PERF_MATRIX);
    assert(profile_bucket(G_LOADTLUT)==PERF_TEXTURE);
    assert(profile_bucket(G_FILLRECT)==PERF_FILL);
    assert(profile_bucket(255)==-1);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
