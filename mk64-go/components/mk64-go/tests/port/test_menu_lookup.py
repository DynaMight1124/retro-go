"""Check menu texture selection without relying on adjacent linker tables."""
from pathlib import Path
import subprocess
import tempfile
import unittest

from test_rg_render import ROOT, compile_host, function


class MenuLookupTests(unittest.TestCase):
    def test_all_main_menu_items_select_their_own_table(self):
        source = (ROOT / "src/menu_items.c").read_text()
        lookup = function(source, "static MenuTexture* main_menu_texture(s32 type)")
        self.assertIn("main_menu_texture(type)", function(source, "void add_menu_item("))
        harness = r'''
#include <assert.h>
#include <stddef.h>
typedef int s32;
typedef struct { int id; } MenuTexture;
static MenuTexture textures[16];
/* Reverse table order deliberately: the N64 linker order is not a contract. */
static MenuTexture* D_800E828C[] = {textures+14,textures+15};
static MenuTexture* D_800E8284[] = {textures+12,textures+13};
static MenuTexture* D_800E8274[] = {textures+8,textures+9,textures+10,textures+11};
static MenuTexture* D_800E8254[] = {
  textures,textures+1,textures+2,textures+3,
  textures+4,textures+5,textures+6,textures+7};
'''
        harness += lookup + r'''
int main(void) {
  for (int type=0xA;type<=0x19;++type)
    assert(main_menu_texture(type)==textures+type-0xA);
  assert(main_menu_texture(0x9)==NULL);
  assert(main_menu_texture(0x1A)==NULL);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
