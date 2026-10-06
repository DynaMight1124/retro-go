"""Check continuous logical pacing with expensive rendered frames."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host


class FramePacingTests(unittest.TestCase):
    def test_skipped_ticks_recover_time_and_reset_after_pause(self):
        header = (ROOT.parents[1] / "main/frame_pacing.h").read_text()
        harness = header + """
#include <assert.h>
int main(void) {
    mk64_pacer_t p={0};
    int64_t now=1000000;
    unsigned drawn=0,skipped=0;
    for(unsigned tick=0;tick<300;tick++) {
        bool draw=mk64_pacer_begin(&p,now,33333,1);
        if(draw) {now+=65000;drawn++;} else {now+=13000;skipped++;}
        int64_t wait=mk64_pacer_finish(&p,now,draw,1);
        if(wait>0) now+=wait;
    }
    assert(now>=10900000 && now<11100000); /* 300 real updates in ~10s. */
    assert(drawn>90 && drawn<130 && skipped>170);
    p=(mk64_pacer_t){0};
    assert(mk64_pacer_begin(&p,now+5000000,33333,1));
    assert(mk64_pacer_finish(&p,now+5001000,true,1)==32333);
    assert(mk64_pacer_begin(&p,now+6000000,33333,1)); /* Loading rebases. */
    assert(mk64_pacer_begin(&p,now+6001000,16666,0)); /* Speed changes rebase. */
    p=(mk64_pacer_t){0};
    assert(mk64_pacer_begin(&p,0,33333,-1));
    mk64_pacer_finish(&p,65000,true,-1);
    assert(mk64_pacer_begin(&p,65000,33333,-1)); /* Explicit disable honored. */
    p=(mk64_pacer_t){0};
    now=0;
    for(unsigned tick=0;tick<60;tick++) {
        assert(mk64_pacer_begin(&p,now,33333,1)); /* Fast menus draw at 30Hz. */
        now+=10000;
        now+=mk64_pacer_finish(&p,now,true,1);
    }
    /* Audio can block even skipped ticks near the frame budget. Rendering
     * must not disappear while a small timing debt cannot be recovered. */
    p=(mk64_pacer_t){0};
    now=0;
    unsigned gap=0;
    drawn=0;
    for(unsigned tick=0;tick<120;tick++) {
        bool draw=mk64_pacer_begin(&p,now,33333,1);
        if(draw) {gap=0;drawn++;} else {gap++;assert(gap<=5);}
        now+=draw?60000:33300;
        int64_t wait=mk64_pacer_finish(&p,now,draw,1);
        now+=wait;
    }
    assert(drawn>=20);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe=compile_host(harness,Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode,0)


if __name__ == "__main__":
    unittest.main()
