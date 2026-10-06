"""Verify menu pause/flush/resume and deferred restart boundaries."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT,compile_host,function

class MenuLifecycleTests(unittest.TestCase):
    def test_menu_pause_release_and_restart(self):
        source=(ROOT.parents[1]/"main/main.c").read_text()
        harness=r"""
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#define RG_KEY_MENU 1
#define RG_KEY_OPTION 2
#define RG_KEY_ALL 255
#define RG_BOOT_NORMAL 0
#define _(x) (x)
static bool sRestartRequested,paused,flush_ok=true,want_reset;
static unsigned keys,game_menus,option_menus,alerts,inits,restarts,flushes;
typedef struct {const char *configNs,*romPath;} rg_app_t;
static rg_app_t app={"mk64-go","/roms/mk64/game.z64"};
static rg_app_t *rg_system_get_app(void) {return &app;}
static unsigned rg_input_read_gamepad(void) {return keys;}
static void mk64_rg_audio_pause(bool p) {paused=p;}
static bool mk64_eeprom_flush(void) {assert(paused);++flushes;return flush_ok;}
static void rg_gui_alert(const char *title,const char *text) {(void)title;(void)text;assert(paused);++alerts;}
static bool rg_input_wait_for_key(unsigned mask,bool pressed,int timeout) {assert(mask==RG_KEY_ALL && !pressed && timeout==-1 && paused);keys=0;return true;}
static void mk64_rg_audio_ensure_init(void) {assert(!paused);++inits;}
static void rg_system_switch_app(const char *partition,const char *name,const char *rom,int slot,unsigned flags) {
 assert(!partition && paused && slot==0 && flags==RG_BOOT_NORMAL);
 assert(!strcmp(name,app.configNs) && !strcmp(rom,app.romPath));++restarts;
}
"""+function(source,"static bool reset_handler(")+r"""
static void rg_gui_game_menu(void) {assert(paused && flushes);++game_menus;if(want_reset)assert(reset_handler(false));}
static void rg_gui_options_menu(void) {assert(paused && flushes);++option_menus;}
"""+function(source,"static bool handle_system_menu(")+r"""
int main(void) {
 assert(!handle_system_menu() && !inits && !flushes);
 keys=RG_KEY_MENU;assert(handle_system_menu());assert(game_menus==1 && !paused && inits==1);
 keys=RG_KEY_OPTION;assert(handle_system_menu());assert(option_menus==1 && !paused && inits==2);
 flush_ok=false;keys=RG_KEY_MENU;assert(handle_system_menu());assert(alerts==1 && game_menus==1 && !paused);
 flush_ok=true;want_reset=true;keys=RG_KEY_MENU;assert(handle_system_menu());
 assert(restarts==1 && paused && inits==3);
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe=compile_host(harness,Path(directory))
            result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=="__main__":unittest.main()
