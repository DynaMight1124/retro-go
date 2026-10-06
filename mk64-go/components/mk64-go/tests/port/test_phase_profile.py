"""Whole-frame profiling excludes log time and periodic renderer reports."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function


class PhaseProfileTests(unittest.TestCase):
    def test_quiet_samples_log_time_and_course_reset(self):
        source = (ROOT / "src/port/rg/port_rg.c").read_text()
        header = (ROOT / "src/port/rg/phase_profile.h").read_text()
        enum_start = header.index("enum {")
        globals_start = source.index("static u32 sLogTime;")
        harness = """
#include <stdint.h>
#include <string.h>
#include <assert.h>
typedef uint32_t u32;
static unsigned sFrame, sRenderFrames, reports;
static int sRenderEnabled=1;
#define MK64_RG_REPORT_INTERVAL 30
static u32 clock_us;
static u32 port_time_us(void) { return clock_us; }
#define PORT_LOG(...) (++reports)
"""
        harness += header[enum_start:header.index(";", enum_start) + 1]
        harness += source[globals_start:source.index("/* Subtract", globals_start)]
        for name in ("static u32 profile_clock", "void port_rg_profile_begin",
                     "void port_rg_profile_mark", "void port_rg_profile_finish"):
            harness += function(source, name)
        harness += """
int main(void) {
    for (sFrame = 0; sFrame < 59; ++sFrame) {
        sRenderFrames=sFrame;
        port_rg_profile_begin(1, 0);
        for (int phase = 0; phase < RG_PHASE_COUNT; ++phase) {
            clock_us += 110; sLogTime += 10;
            port_rg_profile_mark(phase);
        }
        port_rg_profile_finish();
    }
    /* Frames 1..4, 24, 30 excluded; every phase omits its 10us log cost. */
    assert(sPhaseSamples == 53);
    for (int phase = 0; phase < RG_PHASE_COUNT; ++phase)
        assert(sPhaseSum[phase] == 5300);
    /* The expensive report frame must not contaminate the samples. */
    port_rg_profile_begin(1, 0);
    clock_us += 100000; port_rg_profile_mark(RG_PHASE_DRAW);
    port_rg_profile_finish();
    assert(reports == 1 && sPhaseSamples == 0 && sPhaseFrames == 0);
    sFrame = 60;
    port_rg_profile_begin(1, 0);
    clock_us += 100; port_rg_profile_mark(RG_PHASE_SIM);
    port_rg_profile_finish();
    assert(sPhaseSamples == 1 && sPhaseSum[RG_PHASE_SIM] == 100);
    port_rg_profile_begin(1, 8);
    assert(sPhaseSamples == 0 && sPhaseSum[RG_PHASE_SIM] == 0);
    port_rg_profile_begin(0, 8);
    clock_us += 100; port_rg_profile_mark(RG_PHASE_SIM);
    port_rg_profile_finish();
    assert(sPhaseFrames == 0);
    /* Unsigned clock deltas also work across the timer's 32-bit wrap. */
    sLogTime = 0; clock_us = UINT32_MAX - 50;
    port_rg_profile_begin(1, 8);
    clock_us += 100; port_rg_profile_mark(RG_PHASE_SIM);
    assert(sPhaseCurrent[RG_PHASE_SIM] == 100);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            executable = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
