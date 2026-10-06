"""Release builds omit routine game logging, including formatting work."""
from pathlib import Path
import subprocess,tempfile,unittest
from test_rg_render import ROOT,compile_host,function

class ReleaseLoggingTests(unittest.TestCase):
 def test_game_bridge_logs_only_in_debug_builds(self):
  source=function((ROOT/'src/port/rg/port_rg.c').read_text(),'void port_log(')
  for release in [0,1]:
   with self.subTest(release=release),tempfile.TemporaryDirectory() as d:
    harness="""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t u32;
static unsigned info,debug,formats;
static u32 port_time_us(void){return 0;}
static int format_line(char *s,size_t n,const char *f,va_list a){++formats;return vsnprintf(s,n,f,a);}
#define vsnprintf format_line
#define RG_LOGI(...) (++info)
#if RG_BUILD_RELEASE
#define RG_LOGD(...)
#else
#define RG_LOGD(...) (++debug)
#endif
"""
    harness='#define RG_BUILD_RELEASE '+str(release)+'\n'+harness+source+"""
int main(void){
 port_log("game: iteration %u",120u);
 port_log("setup_race done");
 assert(info==0);
 assert(debug==(RG_BUILD_RELEASE?0:2));
 assert(formats==(RG_BUILD_RELEASE?0:2));
 return 0;
}
"""
    exe=compile_host(harness,Path(d));self.assertEqual(subprocess.run([str(exe)]).returncode,0)
