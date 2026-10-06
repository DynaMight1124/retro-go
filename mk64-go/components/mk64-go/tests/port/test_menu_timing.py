"""Check menu timing accumulation without logging in measured operations."""
from pathlib import Path
import subprocess,tempfile,unittest
from test_rg_render import ROOT,compile_host
class MenuTimingTests(unittest.TestCase):
    def test_accumulates_and_resets_window(self):
        header=(ROOT/'src/port/rg/menu_timing.h').read_text()
        source=header+'''\n#include <assert.h>
Mk64MenuTiming sMk64MenuTiming;
int main(void) {
 mk64_menu_timing_reset();
 sMk64MenuTiming.enabled=true;
 mk64_menu_timing_add(MK64_MENU_ROM,1200,512);
 mk64_menu_timing_add(MK64_MENU_ROM,800,256);
 mk64_menu_timing_add(MK64_MENU_TKMK,80000,153600);
 mk64_menu_timing_tick(90000,0);
 mk64_menu_timing_tick(15000,3000);
 assert(sMk64MenuTiming.stage[MK64_MENU_ROM].calls==2);
 assert(sMk64MenuTiming.stage[MK64_MENU_ROM].us==2000);
 assert(sMk64MenuTiming.stage[MK64_MENU_ROM].max_us==1200);
 assert(sMk64MenuTiming.stage[MK64_MENU_ROM].bytes==768);
 assert(sMk64MenuTiming.stage[MK64_MENU_TKMK].max_us==80000);
 assert(sMk64MenuTiming.ticks==2 && sMk64MenuTiming.max_tick_us==90000);
 assert(sMk64MenuTiming.min_queued_bytes==0);
 mk64_menu_timing_reset();
 assert(sMk64MenuTiming.ticks==0 && sMk64MenuTiming.stage[MK64_MENU_ROM].calls==0);
 assert(sMk64MenuTiming.min_queued_bytes==UINT32_MAX);
 return 0;
}'''
        with tempfile.TemporaryDirectory() as d:
            exe=compile_host(source,Path(d))
            self.assertEqual(subprocess.run([str(exe)]).returncode,0)
