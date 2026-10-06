"""Texel conversion retains floor semantics across positive/negative UVs."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class TexelFloorTests(unittest.TestCase):
    def test_floor_boundaries_and_float_patterns(self):
        source = (ROOT / "src/port/rg/gfx_rg.c").read_text()
        harness = """
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
""" + function(source, "static int triangle_texel_floor")
        harness += """
static void check(float value) {
    /* Same finite, representable int domain required by the previous cast. */
    if (value >= -2147483648.0f && value < 2147483648.0f)
        assert(triangle_texel_floor(value) == (int)floorf(value));
}
int main(void) {
    check(0.0f); check(-0.0f);
    check(-2147483648.0f); check(2147483520.0f);
    for (int i = -65536; i <= 65536; ++i) {
        float x = (float)i;
        check(x); check(nextafterf(x, -INFINITY));
        check(nextafterf(x, INFINITY));
        check(x + 0.125f); check(x + 0.5f); check(x + 0.875f);
    }
    uint32_t bits = 123;
    for (unsigned i = 0; i < 250000; ++i) {
        bits = bits * 1664525u + 1013904223u;
        float value; memcpy(&value, &bits, sizeof(value));
        check(value);
    }
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
