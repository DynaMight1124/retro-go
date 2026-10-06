"""Ensure image groups select image addresses, rather than individual bytes."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_rg_render import ROOT, compile_host

sys.path.insert(0, str(ROOT / "tools"))
import gen_asset_headers as generator
from yaml_unique import load


class AssetHeaderTests(unittest.TestCase):
    def test_texture_group_strides_in_c(self):
        manifest = load((ROOT / "yamls/us/common_data.yml").read_text())
        strides = {"common_texture_particle_smoke": 1024,
                   "common_texture_particle_spark": 1024,
                   "common_texture_bomb": 1024,
                   "common_texture_hud_place": 4096,
                   "D_0D015258": 2048,
                   "common_texture_player_emblem": 2048,
                   "common_texture_hud_type_C_rank_font": 256,
                   "common_texture_hud_type_C_rank_tiny_font": 64}
        harness = '#include <stdint.h>\n#include <assert.h>\ntypedef uint8_t u8;\n'
        for symbol, stride in strides.items():
            harness += generator.table_declaration(symbol, manifest) + '\n'
            harness += f'u8 {symbol}[2][{stride}];\n'
        harness += 'int main(void) {\n'
        for symbol, stride in strides.items():
            harness += f'assert((uintptr_t){symbol}[1]-(uintptr_t){symbol}[0]=={stride});\n'
        harness += 'return 0; }\n'
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)
